#include <llvm/IR/DebugInfo.h>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/CFG.h>

#include "MissingChecks.h"
#include "Config.h"


////////////////////////////////////////////////////////////
//
// Configuration
//
#define MAX_STAGE 2

//#define MC_DEBUG
//#define UNIT_TEST

#define REPORT_SRC
#define REPORT_USE

const string test_funcs[] = {
	"dccp_v6_send_response",
	"free_pmd_range",
	"palmas_set_mode_smps",
	"palmas_smps_init",
	"variax_startup6",
	"rt5663_parse_dp",
	"snd_ice1712_6fire_read_pca",
	"snd_sb16dsp_pcm",
	"snd_gus_init_control",
	"pod_startup4",
	"sst_media_hw_params",
	"tipc_nl_compat_sk_dump",
	"tipc_nl_compat_publ_dump",
	"rpcrdma_decode_msg",
	"nf_tables_fill_flowtable_info",
	"call_ad",
	"ipv6_sysctl_rtcache_flush",
	"net_ns_init",
	"batadv_init",
	"ethtool_get_rxnfc",
	"copy_params",
	"dev_rename",
	"rdma_create_trans",
	"netlbl_domhsh_add_default",
	"netlbl_cfg_calipso_map_add",
	"cfcnfg_remove",
	"xfrm_alloc_replay_state_esn",
	"tcindex_alloc_perfect_hash",
	"allocate_msrs",
	"nfc_llcp_send_i_frame",
	"dev_alloc_skb",
};
////////////////////////////////////////////////////////////

using namespace llvm;
using namespace std;

//
// Initialize the static variables
//
int MissingChecksPass::AnalysisStage = 1;
map<src_t, unsigned> MissingChecksPass::SrcCheckCount;
map<use_t, unsigned> MissingChecksPass::UseCheckCount;

map<src_t, unsigned> MissingChecksPass::SrcUncheckCount;
map<use_t, unsigned> MissingChecksPass::UseUncheckCount;

map<src_t, unsigned> MissingChecksPass::SrcTotalCount;
map<use_t, unsigned> MissingChecksPass::UseTotalCount;

set<src_t> MissingChecksPass::CheckedSrcSet;
set<use_t> MissingChecksPass::CheckedUseSet;

map<src_t, set<Value *>> MissingChecksPass::SrcUnchecksMap;
map<use_t, set<Value *>> MissingChecksPass::UseUnchecksMap;

map<src_t, set<ModelSC>> MissingChecksPass::SrcChecksMap;
map<use_t, set<ModelSC>> MissingChecksPass::UseChecksMap;

set<Value *> MissingChecksPass::TrackedSrcSet;
set<Value *> MissingChecksPass::TrackedUseSet;


// 
// Implementation of MissingCheckPass
//

/// Alias analysis
void MissingChecksPass::collectAliasPointers(Function *F, LoadInst
		*LI, set <Value *> &AliasSet) {

	Value *LPO = LI->getPointerOperand();
	AllocaInst *AI = dyn_cast<AllocaInst>(LPO);
	if (!AI)
		return;

	for (inst_iterator i = inst_begin(F), ei = inst_end(F); i != ei; ++i) {
		StoreInst *SI = dyn_cast<StoreInst>(&*i);
		if (!SI) 
			continue;
		if (SI->getPointerOperand() == LPO && DFA.possibleUseStResult(LI, SI))
			AliasSet.insert(SI->getValueOperand());
	}
}

/// Find the sources of a given value
void MissingChecksPass::findSourceCV(Value *V, set<Value *> &CVSet, 
		set<Value *> &TrackedSet) {

	if (TrackedSet.count(V) != 0)
		return;
	TrackedSet.insert(V);

	if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
		findSourceCV(CE->getOperand(0), CVSet, TrackedSet);
		return;
	}

	if (isa<Argument>(V)
			|| isa<GlobalVariable>(V) 
			|| isa<CallInst>(V)
	   ) {
		CVSet.insert(V);
		return;
	}

	if (isa<Constant>(V)) 
		return;

	SelectInst *SI = dyn_cast<SelectInst>(V);
	if (SI) {
		findSourceCV(SI->getTrueValue(), CVSet, TrackedSet);
		findSourceCV(SI->getFalseValue(), CVSet, TrackedSet);
		return;
	}

	GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V);
	if (GEP) {
		findSourceCV(GEP->getPointerOperand(), CVSet, TrackedSet);
		return;
	}

	PHINode *PN = dyn_cast<PHINode>(V);
	if (PN) {
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			Value *IV = PN->getIncomingValue(i);
			findSourceCV(IV, CVSet, TrackedSet);
		}
		return;
	}

	if (LoadInst *LI = dyn_cast<LoadInst>(V)) {

		Value *LPO = LI->getPointerOperand();
		Function *F = LI->getParent()->getParent();
		std::set<Value *> AliasSet;

		DFA.getAliasPointers(LI->getPointerOperand(), AliasSet,
				Ctx->FuncPAResults[F]);

		set<BasicBlock *> reachBBs;
		DFA.collectPredReachBlocks(LI->getParent(), reachBBs);
		//Check the storeInst that precedes the loadInst
		for (Value *A : AliasSet) {
			for (User *U : A->users()) {

				StoreInst *SI = dyn_cast<StoreInst>(U);
				if (SI && A == SI->getPointerOperand()) {
					// The StoreInst must be in a pred reachable block
					if (reachBBs.find(SI->getParent()) == reachBBs.end())
						continue;
					Value *SVO = SI->getValueOperand();
					findSourceCV(SVO, CVSet, TrackedSet);
				}
			}
		}
		// FIXME: further track LoadInst?
		findSourceCV(LI->getPointerOperand(), CVSet, TrackedSet);
		return;
	}

	if (ICmpInst *ICmp = dyn_cast<ICmpInst>(V)) {
		//CVSet.insert(ICmp);
		return;
	}

	// Default policy for unary and binary
	UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V);
	if (UI) {
		Value *UO = UI->getOperand(0);
		findSourceCV(UO, CVSet, TrackedSet);
		return;
	}

	BinaryOperator *BO = dyn_cast<BinaryOperator>(V);
	if (BO) {
		for (unsigned i = 0, e = BO->getNumOperands(); i != e; ++i) {
			Value *Opd = BO->getOperand(i);
			// Constant wouldn't be critical?
			if (dyn_cast<Constant>(Opd))
				continue;

			findSourceCV(Opd, CVSet, TrackedSet);
		}
		return;
	}

	return;
}

/// Given a sanity Check, identify direct targets or indirect targets
/// Indirect targets are the parameters of a function, when the
/// sanity check is a return value of the said function call.
/// NOTE: current impl does not support inter-procedural analysis
void MissingChecksPass::identifyCheckedTargets(Function *F,
		Value *V, 
		set<Value *> &CTSet) {

	set<Value *> TrackedSet;
	set<Value *> CVSet;
	Value *SCOpd;

	if (ICmpInst *ICmp = dyn_cast<ICmpInst>(V)) {
		for (unsigned i = 0, ie = ICmp->getNumOperands(); i < ie; i++) {
			SCOpd = ICmp->getOperand(i);
			// TODO: let's try intra-procedural analysis first
			DFA.findInFuncSourceCV(SCOpd, CTSet, CVSet, TrackedSet);
		}

	} else if (BinaryOperator *BO = dyn_cast<BinaryOperator>(V)) {
		for (unsigned i = 0, ie = BO->getNumOperands(); i < ie; i++) {
			SCOpd = BO->getOperand(i);
			DFA.findInFuncSourceCV(SCOpd, CTSet, CVSet, TrackedSet);
		}

	} else if (UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V)) {
		SCOpd = UI->getOperand(0);
		DFA.findInFuncSourceCV(SCOpd, CTSet, CVSet, TrackedSet);

	} else if (PHINode *PN = dyn_cast<PHINode>(V)) {
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			SCOpd = PN->getIncomingValue(i);
			DFA.findInFuncSourceCV(SCOpd, CTSet, CVSet, TrackedSet);
		}

	} else if (CallInst *CI = dyn_cast<CallInst>(V)) {

		for (auto AI = CI->arg_begin(), 
				E = CI->arg_end(); AI != E; ++AI) {
			Value* Param = dyn_cast<Value>(&*AI);
			DFA.findInFuncSourceCV(Param, CTSet, CVSet, TrackedSet);
		}
	} 

	return;
}

/// Identify sources for indirect targets
void MissingChecksPass::identifyIndirectTargets(Function *F,
		Value *V, 
		set<Value *> &CTSet) {

	set<Value *> TrackedSet;
	set<Value *> CVSet;

	CallInst *CI = dyn_cast<CallInst>(V);
	if (!CI) return;

	for (auto AI = CI->arg_begin(), E = CI->arg_end(); AI != E; ++AI) {
		Value* SCOpd = dyn_cast<Value>(&*AI);
		DFA.findInFuncSourceCV(SCOpd, CTSet, CVSet, TrackedSet);
	}
}

void MissingChecksPass::findClosestBranch (Value *Src, Value *SC, 
		set<Value *> &BrSet) {

	// The closest branch is a return instruction
	if (CallInst *CI = dyn_cast<CallInst>(Src)) {
		Function *CF = CI->getCalledFunction();
		if (!CF)
			return;

		if (Ctx->Callees[CI].size())
			CF = *(Ctx->Callees[CI].begin());
		if (!CF) 
			return;

		// FIXME: make sure it is a return instruction
		BrSet.insert(CF->back().getTerminator());
		return;
	}
	// The closest branch is an indirect call
	if (Argument *Arg = dyn_cast<Argument>(Src)) {
		if (!Arg->getParent()->hasAddressTaken())
			return;
		else {
			// Find the indirect call targetting this function
			// FIXME: could be multiple indirect calls
			CallInst *CI;
			BrSet.insert(CI);
			return;
		}
	}

	// The closest branch is a branch instruction
	set<BasicBlock *> PB;
	list<BasicBlock *> EB;

	Instruction *I = dyn_cast<Instruction>(SC);
	if (!I)
		return;

	EB.push_back(I->getParent());

	while (!EB.empty()) {
		BasicBlock *TB = EB.front();
		EB.pop_front();
		if (PB.count(TB) != 0)
			continue;
		PB.insert(TB);

		for (BasicBlock *Pred : predecessors(TB)) {
			// Check if it is a branch instruction
			Instruction *TI = Pred->getTerminator();
			if (TI->getNumSuccessors() > 1) {
				for (BasicBlock *Succ : successors(Pred)) {
					if (TB == Succ)
						continue;

					// The parallel path must have some blocks
					set<BasicBlock *> reachBBs;
					DFA.collectSuccReachBlocks(TB, reachBBs);
					if (reachBBs.find(Succ) == reachBBs.end())
						continue;
					else {
						BrSet.insert(TI);
						return;
					}
				}
			}
			EB.push_back(Pred);
		}
	}

	return;
}

void MissingChecksPass::findParallelPaths(set<Value *> &BrSet, 
		set<Path> &PPathSet) {
}

void MissingChecksPass::isCheckedBackward(Function *F, use_t Use,
		Value *V, set<BasicBlock *> &Scope, set<Value *> &VSet, 
		bool &isChecked, unsigned &Depth) {

	if (isChecked)
		return;

	if (VSet.find(V) != VSet.end())
		return;
	VSet.insert(V);

	if (Depth > 5)
		return;

	// Inter-procedural analysis
	if (Argument *Arg = dyn_cast<Argument>(V)) {

		unsigned ArgNo = -1;

		for (Function::arg_iterator ai = F->arg_begin(),
				ae = F->arg_end(); ai != ae; ++ai) {
			++ArgNo;
			if (Arg == &*ai)
				break;
		}
		if (ArgNo == -1)
			return;

		Function *PF = Arg->getParent();
		if (!PF || Ctx->Callers.find(PF) == Ctx->Callers.end())
			return;
		for (auto CI : Ctx->Callers[PF]) {
			if (ArgNo >= CI->getNumArgOperands())
				continue;

			Value *ArgC = CI->getArgOperand(ArgNo);

			if (!ArgC)
				continue;

			Function *CF = CI->getParent()->getParent(); 
			set<BasicBlock *> predBBs;
			DFA.collectPredReachBlocks(CI->getParent(), predBBs);

			unsigned New_Depth = Depth + 1;

			isCheckedBackward(CI->getParent()->getParent(), Use,
					ArgC, predBBs, VSet, isChecked, New_Depth);

			if (isChecked) {
				return;
			}
		}

		return;
	}


	for (auto UV : V->users()) {

		if (!UV) continue;
		if (isChecked) return;

		Instruction *UI = dyn_cast<Instruction>(UV);
		if (!UI)
			continue;

		if (Scope.find(UI->getParent()) == Scope.end())
			continue;

		if (CmpInst *CmpI = dyn_cast<CmpInst>(UV)) {

			// If it is not in modeled check set, we still do not
			// count it as a check
			if (inModeledCheckSet(CmpI, Use.first, Use.second,
						false)) { 
				isChecked = true;
				return;
			}
			// Not an actual check
			else
				continue;
		}

		if (isa<SwitchInst>(UV)) {
			isChecked = true;
			return;
		}

		if (CallInst *CI = dyn_cast<CallInst>(UV)) {
			if (Ctx->CheckInstSets[F].find(CI) != Ctx->CheckInstSets[F].end()) {
				isChecked = true;
				return;
			}
		}
	}

	if (isa<CallInst>(V)) {
		CallInst* CI = dyn_cast<CallInst>(V);
		return;
	}

	if (isConstant(V))
		return;

	if (LoadInst* LI = dyn_cast<LoadInst>(V)) {

		set<Value *> AliasSet;
		DFA.getAliasPointers(LI->getPointerOperand(), AliasSet,
				Ctx->FuncPAResults[F]);

		for (Value *A : AliasSet) {
			for (User *SU : A->users()) {
				Instruction *SUI = dyn_cast<Instruction>(SU);
				// Filtering
				if (!SUI)
					continue;
				if (Scope.find(SUI->getParent()) == Scope.end())
					continue;

				if (LoadInst *SLI = dyn_cast<LoadInst>(SU)) {
					isCheckedBackward(F, Use, SLI, 
							Scope, VSet, isChecked, Depth);
				}
				else if(StoreInst *SSI = dyn_cast<StoreInst>(SU)) {
					isCheckedBackward(F, Use, SSI->getValueOperand(),
							Scope, VSet, isChecked, Depth);
				}
			}
		}
		return;
	}

	PHINode *PN = dyn_cast<PHINode>(V);
	if (PN) {
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			Value *IV = PN->getIncomingValue(i);
			isCheckedBackward(F, Use, IV, 
					Scope, VSet, isChecked, Depth);
		}
		return;
	}

	if (isa<SelectInst>(V)) {
		SelectInst *SI = dyn_cast<SelectInst>(V);
		isCheckedBackward(F, Use, SI->getTrueValue(), 
				Scope, VSet, isChecked, Depth);
		isCheckedBackward(F, Use, SI->getFalseValue(), 
				Scope, VSet, isChecked, Depth);
		return;
	}

	if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
		isCheckedBackward(F, Use, GEP->getPointerOperand(), 
				Scope, VSet, isChecked, Depth);
		return;
	}

	if (isa<UnaryInstruction>(V)) {
		UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V);
		isCheckedBackward(F, Use, UI->getOperand(0), 
				Scope, VSet, isChecked, Depth);
		return;
	}

	if (isa<BinaryOperator>(V)) {
		BinaryOperator *BI = dyn_cast<BinaryOperator>(V);
		isCheckedBackward(F, Use, BI->getOperand(0),
				Scope, VSet, isChecked, Depth);
		isCheckedBackward(F, Use, BI->getOperand(1), 
				Scope, VSet, isChecked, Depth);
		return;
	}

	OP << "== Warning: unsupported LLVM IR:" << *V <<  " in " 
		<< F->getName() << "\n";

#ifdef DEBUG_PRINT
	assert(0);
#endif
}

void MissingChecksPass::isCheckedForward(Function *F, src_t Src,
		Value *V, set<BasicBlock *> &Scope, set<Value *> &VSet, 
		bool &isChecked, unsigned &Depth, bool enableAlias) {

	if (isChecked)
		return;

	if (VSet.find(V) != VSet.end())
		return;
	VSet.insert(V);

	for (auto UV : V->users()) {

		if (!UV) continue;
		if (isChecked) return;

		Instruction *UserI = dyn_cast<Instruction>(UV);
		if (!UserI)
			continue;

		if (Scope.find(UserI->getParent()) == Scope.end())
			continue;

		if (CmpInst *CmpI = dyn_cast<CmpInst>(UV)) {

			// Counted as a check only when the check is in the
			// modeled check set
			if (inModeledCheckSet(CmpI, Src.first, Src.second, true)) { 
				isChecked = true;
				return;
			}
			// Not an actual check
			else
				continue;
		}

		if (isa<SwitchInst>(UV)) {
			isChecked = true;
			return;
		}

		if (CallInst *CI = dyn_cast<CallInst>(UV)) {
			if (Ctx->CheckInstSets[F].find(CI) != Ctx->CheckInstSets[F].end()) {
				isChecked = true;
				return;
			}
			// FIXME: tracking is supposed to be stopped here
			//isCheckedForward(F, CI, Scope, VSet, isChecked, Depth);
		}

		// FIXME: here we conservatively assume returned value will be
		// checked in callers
		if (isa<ReturnInst>(UV)) {
			isChecked = true;
			return;
		}

		StoreInst *SI = dyn_cast<StoreInst>(UV);
		if (SI && V == SI->getValueOperand()) {
			std::set<Value *> AliasSet;
			DFA.getAliasPointers(SI->getPointerOperand(), AliasSet,
					Ctx->FuncPAResults[F]);
			for (Value *A : AliasSet) {
				for (User *SU : A->users()) {
					LoadInst *LI = dyn_cast<LoadInst>(SU);
					if (LI) {
						isCheckedForward(F, Src, LI, Scope, VSet,
								isChecked, Depth); }
				}
			}
			continue;
		}

		if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(UV)) {
			if (GEP->getPointerOperand() == V) {
				isCheckedForward(F, Src, UV, Scope, VSet, isChecked,
						Depth); 
				continue;
			}
		}

		if (PHINode *PN = dyn_cast<PHINode>(UV)) {
			isCheckedForward(F, Src, UV, Scope, VSet, isChecked, Depth);
			continue;
		}

		UnaryInstruction *UI = dyn_cast<UnaryInstruction>(UV);
		if (UI) {
			isCheckedForward(F, Src, UV, Scope, VSet, isChecked, Depth);
			continue;
		}

		BinaryOperator *BO = dyn_cast<BinaryOperator>(UV);
		if (BO) {
			isCheckedForward(F, Src, UV, Scope, VSet, isChecked,
					Depth); 
			continue;
		}
	}
}

ModelSC MissingChecksPass::modelCheck(CmpInst *CmpI, 
		Value *SrcUse, int8_t ArgNo) {

	ModelSC MSC;
	MSC.SrcUse = SrcUse;
	MSC.ArgNo = ArgNo;

	CmpInst::Predicate Pred = CmpI->getPredicate();
	switch (Pred) {
		case CmpInst::ICMP_EQ:
			MSC.SCO = ICMP_EQ;
			break;
		case CmpInst::ICMP_NE:
			MSC.SCO = ICMP_NE;
			break;
		case CmpInst::ICMP_SGT:
			MSC.SCO = ICMP_GT;
			break;
		case CmpInst::ICMP_SGE:
			MSC.SCO = ICMP_GT;
			break;
		case CmpInst::ICMP_SLT:
			MSC.SCO = ICMP_LT;
			break;
		case CmpInst::ICMP_SLE:
			MSC.SCO = ICMP_LT;
			break;
		case CmpInst::ICMP_UGT:
			MSC.SCO = ICMP_GT;
			break;
		case CmpInst::ICMP_UGE:
			MSC.SCO = ICMP_GT;
			break;
		case CmpInst::ICMP_ULT:
			MSC.SCO = ICMP_LT;
			break;
		case CmpInst::ICMP_ULE:
			MSC.SCO = ICMP_LT;
			break;
		default:
			MSC.SCO = ICMP_OTHER;
			break;
	}

	Value *Cond = CmpI->getOperand(1);
	if (isa<Constant>(Cond))
		MSC.SCC = SCC_CONST;
	// To support fine-grainded cases?
	else
		MSC.SCC = SCC_OTHER;

	return MSC;
}

void MissingChecksPass::addSrcCheck(src_t Src, ModelSC MSC) {
	SrcCheckCount[Src] += 1;
	CheckedSrcSet.insert(Src);
	SrcChecksMap[Src].insert(MSC);
}

void MissingChecksPass::addUseCheck(use_t Use, ModelSC MSC) {
	UseCheckCount[Use] += 1;
	CheckedUseSet.insert(Use);
	UseChecksMap[Use].insert(MSC);
}

void MissingChecksPass::addSrcUncheck(src_t Src,
		Value *V) {
	SrcUncheckCount[Src] += 1;
	SrcUnchecksMap[Src].insert(V);
}

void MissingChecksPass::addUseUncheck(use_t Use, 
		Value *V) {
	UseUncheckCount[Use] += 1;
	UseUnchecksMap[Use].insert(V);
}

bool MissingChecksPass::inModeledCheckSet(CmpInst *CmpI,
		Value *SrcUse, int8_t ArgNo, bool IsSrc) {

	ModelSC MSC = modelCheck(CmpI, SrcUse, ArgNo);

	if (IsSrc) {
		src_t Src = src_c(SrcUse, ArgNo);
		if (SrcChecksMap[Src].find(MSC) 
				!= SrcChecksMap[Src].end()) {
			return true;
		}
	}
	else {
		use_t Use = use_c(SrcUse, ArgNo);
		if (UseChecksMap[Use].find(MSC) 
				!= UseChecksMap[Use].end()) {
			return true;
		}
	}
	return false;
}

void MissingChecksPass::countSrcUseChecks(Function *F, 
		Instruction *SCI) {

	// Identify critical variables/functions used in each security
	// check.
	set <Value *> CTSet;
	set <Value *> TmpSet;

	identifyCheckedTargets(F, SCI, TmpSet);

	// Filtering out non-interesting targets
	for (Value *CT : TmpSet) {
		if (!isa<Constant>(CT) && !isa<GlobalVariable>(CT)) {
			CTSet.insert(CT);
		}
	}

	if (CTSet.empty())
		return;

	set<BasicBlock *> predBBs;
	DFA.collectPredReachBlocks(SCI->getParent(), predBBs);

	// Count checks for sources
	// Right now we consider two kinds of sources: 1) return value
	// and 2) argument. For statistical counting purposes, we
	// instead collect the corresponding 1) called function and 2)
	// indirect call with argument number 
	for (auto CT : CTSet) {

#ifdef MC_DEBUG
		OP << "\n== Checked target: " << *CT << "\n";
		printSourceCodeInfo(CT);
#endif 
		int8_t ArgNo = -1;
		// Argument as source: Collect indirect calls of argument sources
		if (Argument *Arg = dyn_cast<Argument>(CT)) {

			if (!F->hasAddressTaken())
				continue;

			for (Function::arg_iterator ai = F->arg_begin(),
					ae = F->arg_end(); ai != ae; ++ai) {
				++ArgNo;
				if (Arg == &*ai)
					break;
			}

			for (auto CI : Ctx->Callers[F]) {
				// Indirect call
				if (CI->getCalledFunction() != NULL) {
					continue;	
				}
				// Collect indirect calls and argument number as source
				src_t Src = src_c(CI, ArgNo);
				addSrcCheck(Src, modelCheck(dyn_cast<CmpInst>(SCI), 
							CI, ArgNo));
			}
		}
		// Return value as source: Collect called functions in direct calls
		else if (CallInst *CI = dyn_cast<CallInst>(CT)) {
			Function *CF = CI->getCalledFunction();
			if (!CF) continue;

			if (Ctx->Callees[CI].size())
				CF = *(Ctx->Callees[CI].begin());
			if (!CF) continue;

			src_t Src = src_c(CF, -1);
			addSrcCheck(Src, modelCheck(dyn_cast<CmpInst>(SCI),
						CF, -1));
		}
		// Output parameter as source: Also collect called functions
		// in direct calls
		else {
			for (User *U : CT->users()) {
				CallInst *CI = dyn_cast<CallInst>(U);
				if (!CI)
					continue;
				if (predBBs.find(CI->getParent()) != predBBs.end()) {
					int8_t ArgNo = -1;
					CallSite CS(CI);
					for (CallSite::arg_iterator ai = CS.arg_begin(),
							ae = CS.arg_end(); ai != ae; ++ai) {
						++ArgNo;
						if (CT == *ai)
							break;
					}
					if (ArgNo == -1)
						continue;

					Function *CF = CI->getCalledFunction();
					if (!CF) continue;

					if (Ctx->Callees[CI].size())
						CF = *(Ctx->Callees[CI].begin());
					if (!CF) continue;

					src_t Src = src_c(CF, ArgNo);
					addSrcCheck(Src, modelCheck(dyn_cast<CmpInst>(SCI),
								CF, ArgNo));
				}
			}
		}
	}

	// Find uses of direct targets
	map<Value *, set<use_t>>UseMap;
	for (Value * CT : CTSet) {
		// Determine uses for each critical variable
		set<use_t> UseSet;
		set<Value *>Visited;
		DFA.findUses(SCI, CT, UseSet, Visited); 
		if (!UseSet.empty()) 
			UseMap[CT] = UseSet;
	}

#ifdef MC_DEBUG
	for (auto UM : UseMap) {
		for (auto Use : UM.second) {
			if (isa<CallInst>(Use.first)) {
				OP << "\n== Checked use: " << *(Use.first) << " - "<<(int)Use.second<<"\n";
				printSourceCodeInfo(Use.first);
			}
		}
	}
#endif 

	// Count checks for uses
	if (!UseMap.empty()) {
		for (auto UM : UseMap) {
			for (auto Use : UM.second) {
				// Only include callinst
				if (CallInst *CI = dyn_cast<CallInst>(Use.first)) {
					Function *CF = CI->getCalledFunction();
					if (!CF) continue;

					if (Ctx->Callees[CI].size())
						CF = *(Ctx->Callees[CI].begin());
					if (!CF) continue;

					use_t PUse = use_c(CF, Use.second);
					addUseCheck(PUse, modelCheck(dyn_cast<CmpInst>(SCI),
								CF, Use.second));
				}
			}
		}
	}
}

void MissingChecksPass::countSrcUseUnchecks(Function *F) {

	// Do unchecks counting
	for (inst_iterator i = inst_begin(F), e = inst_end(F); 
			i != e; ++i) {

		CallInst *CI = dyn_cast<CallInst>(&*i);
		if (!CI)
			continue;

		CallSite CS(CI);
		// Source
		// Argument as a source
		if (CS.isIndirectCall()) {
			for (int8_t ArgNo = 0; ArgNo < CI->getNumArgOperands(); ++ArgNo) {
				auto Src = CheckedSrcSet.find(src_c(CI, ArgNo));
				if (Src == CheckedSrcSet.end())
					continue;
				for (auto Callee : Ctx->Callees[CI]) {

					Argument *PArg = getArgByNo(Callee, ArgNo);

					// Do forward slicing to find data flows of PArg
					// and see if they are checked
					bool isChecked = false;
					set<Value*> VSet = {};
					unsigned Depth = 1;
					set<BasicBlock *> reachBBs;
					if (Callee->size() < 1)
						continue;
					DFA.collectSuccReachBlocks(&(Callee->getEntryBlock()), reachBBs);

					set<Value *> ToTrackSet;
					ToTrackSet.insert(PArg);
					if (PArg->getType()->isPointerTy()) {
						set<Value *> AliasSet;
						// A check may target loaded variables
						DFA.getAliasPointers(PArg, AliasSet,
								Ctx->FuncPAResults[Callee]);
						for (Value *A : AliasSet) {
							for (User *U : A->users()) {
								LoadInst *LI = dyn_cast<LoadInst>(U);
								if (!LI)
									continue;
								if (reachBBs.find(LI->getParent()) == reachBBs.end())
									continue;
								ToTrackSet.insert(LI);
							}
						}
					}
					// Check each slice to see if it is ever checked
					for (Value *TV : ToTrackSet) {
						isCheckedForward(F, *Src, TV, 
								reachBBs, VSet, isChecked, Depth);
						if (isChecked)
							break;
					}

					if (!isChecked) {
						addSrcUncheck(*Src, PArg);
					}
					SrcTotalCount[*Src] += 1;
				}
			}
			continue;
		}

		// Skip intrinsic functions
		if(CS.getIntrinsicID() != 0) {
			continue;
		}

		// Skip functions without return value and parameters
		if (CI->user_empty() && CS.getNumArgOperands() == 0)
			continue;

		// Return value or parameter of a function call as a source
		Function *CF = NULL;
		if (Ctx->Callees[CI].size())
			CF = *(Ctx->Callees[CI].begin());
		if (CF) {
			// Skip the functions in the blacklist
			// TODO: move these functions to Config.h or a file
			if (CF->getName() == "outb"
					|| CF->getName() == "__const_udelay" 
					|| CF->getName() == "__udelay" 
					|| CF->getName() == "scnprintf" 
					|| CF->getName() == "__fswab16" 
					|| CF->getName() == "__fswab32" 
			   )
				continue;

			int8_t ArgNo = -2;
			Value *Param = NULL;

			do {
				++ArgNo;

				// Filtering
				if (ArgNo == -1) {
					if(CI->user_empty()) {
						continue;
					}
				} else {
					// Only consider 'output' parameters, thus they must be of
					// pointer type
					if (ArgNo >= CI->getNumArgOperands())
						break;

					Param = CI->getOperand(ArgNo);
					if (!Param->getType()->isPointerTy()) {
						continue;
					}
				}

				auto Src = CheckedSrcSet.find(src_c(CF, ArgNo));

				// Skip cases with only one check
				if (Src != CheckedSrcSet.end()) {
					// Do forward slicing and see if CI is ever checked
					bool isChecked = false;
					set<Value*> VSet = {};
					unsigned Depth = 1;
					set<BasicBlock *> reachBBs;
					DFA.collectSuccReachBlocks(CI->getParent(), reachBBs);

					// Return value
					if (ArgNo == -1) {
						src_t Src = src_c(CF, ArgNo);
						isCheckedForward(F, Src, CI, reachBBs, VSet,
								isChecked, Depth); }
						// Paramenter
					else {
						// A check should target the loaded value from the
						// parameter
						set<Value *> AliasSet;
						set<Value *> ToTrackSet;
						DFA.getAliasPointers(Param, AliasSet,
								Ctx->FuncPAResults[F]);
						for (Value *A : AliasSet) {
							for (User *U : A->users()) {
								LoadInst *LI = dyn_cast<LoadInst>(U);
								if (!LI)
									continue;
								if (reachBBs.find(LI->getParent()) == reachBBs.end())
									continue;
								ToTrackSet.insert(LI);
							}
						}
						for (Value *TV : ToTrackSet) {
							isCheckedForward(F, *Src, TV, 
									reachBBs, VSet, isChecked, Depth);
							if (isChecked)
								break;
						}
					}

					if (!isChecked) {
						//TODO: resolve the IS_ERR() issue
						addSrcUncheck(*Src, CI);
					}
					SrcTotalCount[*Src] += 1;
				}
			} while ((ArgNo + 1) < CF->arg_size());
		}

		// Use
		if (CF) {
			for (int8_t ArgNo = 0; ArgNo < CI->getNumArgOperands(); ++ArgNo) {

				use_t Use = use_c(CF, ArgNo);
				if (CheckedUseSet.find(Use) != CheckedUseSet.end()) {
					Value *Arg = CI->getArgOperand(ArgNo);

					// Also do backward slicing and see if Arg is
					// ever checked
					bool isChecked = false;
					set<Value*> VSet = {};
					unsigned Depth = 1;
					set<BasicBlock *> reachBBs;
					DFA.collectPredReachBlocks(CI->getParent(), reachBBs);
					reachBBs.erase(CI->getParent());

					isCheckedBackward(F, Use, Arg, reachBBs, VSet,
							isChecked, Depth);

					if (!isChecked) {
						addUseUncheck(Use, Arg);
					}
					UseTotalCount[Use] += 1;
				}
			}
		}
	}
}

void MissingChecksPass::processResults() {

	for (src_t Src : CheckedSrcSet) {
		unsigned Checks = 0, Unchecks = 0, Total = 0;
		float Rating = 0;
		if (SrcCheckCount.find(Src) != SrcCheckCount.end()) {
			Checks = SrcCheckCount[Src];
			Total = SrcTotalCount[Src];
		}

		if (SrcUncheckCount.find(Src) != SrcUncheckCount.end())
			Unchecks = SrcUncheckCount[Src];

		if (Checks && Unchecks) {
			if (Checks + Unchecks < Total)
				Total = Checks + Unchecks;
			Rating = (float)Unchecks/Total;

#ifndef UNIT_TEST
			if (Rating > 0.3)
				continue;
#endif

#ifdef REPORT_SRC

			string SrcTy;
			if (Src.second == -1)
				SrcTy = "retval";
			else if (isa<CallInst>(Src.first))
				SrcTy = "argmt";
			else
				SrcTy = "param";

#ifdef MC_ADDRTAKEN_FUNC_ONLY
			// 
			// Only consider address-taken functions?
			// 
			Instruction *SrcI = dyn_cast<Instruction>(Src.first);
			if (SrcI && (SrcTy == "retval" || SrcTy == "param")) {
				if (Ctx->AddressTakenFuncs.find(SrcI->getParent()->getParent())
						== Ctx->AddressTakenFuncs.end())
					continue;
			}
#endif
			OP<<format("== [Src-%s]: Rating: %.3f, Checks: %d, Unchecks: %d, Total: %d | Arg: %d\n",
					SrcTy.c_str(), Rating, Checks, Unchecks, Total, (int)Src.second);

			if (SrcTy == "argmt") {
				printSourceCodeInfo(Src.first);

				OP<<"\n\tUnchecks:";
			}

			for (Value *V : SrcUnchecksMap[Src]) {
				OP<<"\t"<<"\n";
				if (Argument *PArg = dyn_cast<Argument>(V)) {
					printSourceCodeInfo(PArg->getParent());
				}
				else
					printSourceCodeInfo(V);

			}

			// Print peer functions
			if (SrcTy == "argmt") {
				OP<<"\n\tPeer checks:\n";
				int count = 0;
				for (ModelSC MSC : SrcChecksMap[Src]) {
					++count;
					if (count == 10)
						break;
					//if (Function *CF = dyn_cast<Function>(MSC.CheckedV))
					//	printSourceCodeInfo(CF);
				}
			}

			OP<<"\n\n\n";
#endif
		}
	}

	for (use_t Use : CheckedUseSet) {
		unsigned Checks = 0, Unchecks = 0, Total = 0;
		float Rating = 0;
		if (UseCheckCount.find(Use) != UseCheckCount.end()) {
			Checks = UseCheckCount[Use];
			Total = UseTotalCount[Use];
		}
		if (UseUncheckCount.find(Use) != UseUncheckCount.end())
			Unchecks = UseUncheckCount[Use];

		if (Checks && Unchecks) {
			if (Checks + Unchecks < Total)
				Total = Checks + Unchecks;
			Rating = (float)Unchecks/Total;

#ifndef UNIT_TEST
			if (Rating > 0.1)
				continue;
#endif

#ifdef REPORT_USE

#ifdef MC_ADDRTAKEN_FUNC_ONLY
			// 
			// Only consider address-taken functions?
			// 

			Instruction *UseI = dyn_cast<Instruction>(Use.first);
			if (UseI) {
				if (Ctx->AddressTakenFuncs.find(UseI->getParent()->getParent())
						== Ctx->AddressTakenFuncs.end())
					continue;
			}
#endif

			OP<<format("== [Use]: Rating: %.3f, Checks: %d, Unchecks: %d, Total: %d | Arg: %d\n", 
					Rating, Checks, Unchecks, Total, (int)Use.second);

			for (Value *V : UseUnchecksMap[Use]) {
				OP<<"\t"<<"\n";
				printSourceCodeInfo(V);
			}
			OP<<"\n\n\n";
#endif
		}
	}
}

bool MissingChecksPass::doInitialization(Module *M) {
  return false;
}

bool MissingChecksPass::doFinalization(Module *M) {
  return false;
}

bool MissingChecksPass::doModulePass(Module *M) {

	++MIdx;

	for(Module::iterator f = M->begin(), fe = M->end();
			f != fe; ++f) {
		Function *F = &*f;

		if (F->empty())
			continue;

		if (F->size() > MAX_BLOCKS_SUPPORT)
			continue;

		if (Ctx->UnifiedFuncSet.find(F) == Ctx->UnifiedFuncSet.end()) 
			continue;

		// Stage 1: collect <source, check> and <<source, use>, check>
		if (AnalysisStage == 1) {

			// FunctionPass

#ifdef MC_DEBUG
#ifdef UNIT_TEST
			size_t sz = sizeof(test_funcs)/sizeof(test_funcs[0]);
			auto fstr = find(test_funcs, test_funcs + sz, F->getName().str());
			if (fstr == test_funcs + sz)
				continue;

			OP<<"[S"<<AnalysisStage<<"] on function: "
				<< "\033[32m" << F->getName() << "\033[0m" << '\n';
#endif

			OP<<"[S"<<AnalysisStage<<"] on function: "
				<< "\033[32m" << F->getName() << "\033[0m" << '\n';
#endif

			set<Value *>SCSet = Ctx->CheckInstSets[F];
			if (SCSet.empty())
				continue;

			for (auto SC : SCSet) {
#ifdef MC_DEBUG
				OP << "\n== Security check: " << *SC << "\n";
				printSourceCodeInfo(SC);
#endif 

				CmpInst *SCI = dyn_cast<CmpInst>(SC);
				if (!SCI)
					continue;

				// Count the check for checked sources and related uses
				countSrcUseChecks(F, SCI);
			}
		}

		// Stage 2: check if the sources and <source, use> pairs have
		// checks
		else if (AnalysisStage == 2) {

			// FunctionPass

#ifdef MC_DEBUG
#ifdef UNIT_TEST
			size_t sz = sizeof(test_funcs)/sizeof(test_funcs[0]);
			auto fstr = find(test_funcs, test_funcs + sz, F->getName().str());
			if (fstr == test_funcs + sz)
				continue;

			OP<<"[S"<<AnalysisStage<<"] on function: "
				<< "\033[32m" << F->getName() << "\033[0m" << '\n';
#endif

			OP<<"[S"<<AnalysisStage<<"] on function: "
				<< "\033[32m" << F->getName() << "\033[0m" << '\n';
#endif

			// Count unchecks for the function
			countSrcUseUnchecks(F);

		}
		// Stage 3: generate bug reports
		else if (AnalysisStage == 3) {

			// See processResults()

		}
	}

	if (Ctx->Modules.size() == MIdx) {
		++AnalysisStage;
		MIdx = 0;
		if (AnalysisStage <= MAX_STAGE) {
			OP<<"## Move to stage "<<AnalysisStage<<"\n";
			return true;
		}
	}

	return false;
}
