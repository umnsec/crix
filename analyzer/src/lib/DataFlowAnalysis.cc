//===-- DataFlowAnalysis.cc - Utils for data-flow analysis------===//
// 
// This file implements commonly used functions for data-flow
// analysis
//
//===-----------------------------------------------------------===//

#include <llvm/IR/InstIterator.h>

#include "DataFlowAnalysis.h"
#include "Config.h"


pair<Value *, int8_t> use_c(Value *V, int8_t Arg) {
	return make_pair((Value *)V, (int8_t)Arg);
}

pair<Value *, int8_t> src_c(Value *V, int8_t Arg) {
	return make_pair((Value *)V, (int8_t)Arg);
}

/// Get aliased pointers for this pointer.
void DataFlowAnalysis::getAliasPointers(Value *Addr,
		std::set<Value *> &aliasAddr,
		PointerAnalysisMap &aliasPtrs) {

	aliasAddr.clear();
	aliasAddr.insert(Addr);

	auto it = aliasPtrs.find(Addr);
	if (it == aliasPtrs.end())
		return;

	for (auto itt : it->second)
		aliasAddr.insert(itt);
}

/// Collect reachable basic blocks from a security check
void DataFlowAnalysis::collectSuccReachBlocks(BasicBlock *BB,
		set<BasicBlock *> &reachBB) {

	if (reachBB.find(BB) != reachBB.end())
		return;
	reachBB.insert(BB);

	succ_iterator si = succ_begin(BB), se = succ_end(BB);
	for (; si != se; ++si)
		collectSuccReachBlocks(*si, reachBB);
}

/// Collect pred reachable basic blocks
void DataFlowAnalysis::collectPredReachBlocks(BasicBlock *BB,
		set<BasicBlock *> &reachBB) {

	if (reachBB.find(BB) != reachBB.end())
		return;
	reachBB.insert(BB);

	pred_iterator pi = pred_begin(BB), pe = pred_end(BB);
	for (; pi != pe; ++pi)
		collectPredReachBlocks(*pi, reachBB);
}

/// Track the sources and same-origin critical variables of the
/// given alias of a critical variable.
void DataFlowAnalysis::findSourceCVAlias(
		Value *V,
		Value *A, // Alias 
		set<Value *> &SourceSet, 
		set<Value *> &CVSet, 
		set<Value *> &TrackedSet) {

	if (TrackedSet.count(A) != 0) 
		return;
	TrackedSet.insert(A);

	for (User *U : A->users()) {

		StoreInst *SI = dyn_cast<StoreInst>(U);
		if (SI && A == SI->getPointerOperand()) {
			Value *SVO = SI->getValueOperand();
			findSourceCV(SVO, SourceSet, CVSet, TrackedSet);
			continue;
		}

		CallInst *CI = dyn_cast<CallInst>(U);
		if (CI) {
			// Not consider indirect calls?
			Value *CaV = CI->getCalledValue();
			if (!CaV) 
				continue;

			StringRef FuncName = getCalledFuncName(CI);
			Value *Src = NULL;
			auto dit = Ctx->DataFetchFuncs.find(FuncName.str());
			if (dit != Ctx->DataFetchFuncs.end()) {
				// FIXME: assume the dst is arg 0
				if (dit->second.first == 0) {
					Src = CI->getArgOperand(dit->second.second);
				}
			}
			if (Src) {
				// FIXME
				SourceSet.insert(Src);
				// to delete
				SourceSet.insert(CI);
				CVSet.insert(Src);
				continue;
			}

			auto cit = Ctx->CopyFuncs.find(FuncName.str());
			if (cit != Ctx->CopyFuncs.end()) {
				// FIXME: assume the src is arg 1
				Value *Src = CI->getArgOperand(1);
				findSourceCV(Src, SourceSet, CVSet, TrackedSet);
				continue;
			}
			continue;
		}

	}
}

/// Track the source of the CriticalVariable. First, determine the
/// type of the LoadPointer, identify the alias if any. For each alias,
/// extract the store operand.
void DataFlowAnalysis::findSourceCV(Value *V, set<Value *> &SourceSet, 
		set<Value *> &CVSet, set<Value *> &TrackedSet) {

	if (TrackedSet.count(V) != 0) 
		return;
	TrackedSet.insert(V);

	if (isConstant(V)) {
		//SourceSet.insert(V);
		CVSet.insert(V);
		return;
	}

	if (dyn_cast<ConstantExpr>(V)) {
		ConstantExpr *CE = dyn_cast<ConstantExpr>(V);
		findSourceCV(CE->getOperand(0), SourceSet, CVSet, TrackedSet);
		return;
	}

	if (Argument *A = dyn_cast<Argument>(V)) {
		CVSet.insert(V);
		if (!A->getParent()) {
			SourceSet.insert(V);
			return;
		}
		// FIXME 
		SourceSet.insert(V);
		return;

		bool FoundCaller = false;
		for (CallInst *Caller : Ctx->Callers[A->getParent()]) {
			if (Caller) {
				if (A->getArgNo() >= Caller->getNumArgOperands())
					continue;
				Value *Arg = Caller->getArgOperand(A->getArgNo());
				if (!Arg)
					continue;
				findSourceCV(Arg, SourceSet, CVSet, TrackedSet);
				FoundCaller = true;
			}
		}

		if (!FoundCaller)
			SourceSet.insert(V);
		return;
	}

	if (dyn_cast<GlobalVariable>(V)) {
		SourceSet.insert(V);
		CVSet.insert(V);
		return;
	}

	CallInst *CI = dyn_cast<CallInst>(V);
	if (CI) {
		CVSet.insert(V);
		// TODO: track callers
		Value *CaV = CI->getCalledValue();
		if (CaV) {
			StringRef FuncName = getCalledFuncName(CI);
			Value *Src = NULL;
			auto it = Ctx->DataFetchFuncs.find(FuncName.str());
			if (it != Ctx->DataFetchFuncs.end()) {
				// Functions like memdup_user
				if (it->second.first == -1) {
					Src = CI->getArgOperand(it->second.second);
				}
			}
			else if (dyn_cast<InlineAsm>(CaV) 
					&& FuncName.contains("get_user")) {
				Src = CI->getArgOperand(0);
			}
			if (Src) {
				SourceSet.insert(Src);
				// to delete
				SourceSet.insert(CI);
				CVSet.insert(Src);
				return;
			}
			return;
		}
	}

	SelectInst *SI = dyn_cast<SelectInst>(V);
	if (SI) {
		CVSet.insert(V);
		findSourceCV(SI->getTrueValue(), SourceSet, CVSet, TrackedSet);
		findSourceCV(SI->getFalseValue(), SourceSet, CVSet, TrackedSet);
		return;
	}

	GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V);
	if (GEP) {
		CVSet.insert(V);
		// TODO: consider aliases
		findSourceCV(GEP->getPointerOperand(), SourceSet, CVSet, TrackedSet);
		return;
	}

	PHINode *PN = dyn_cast<PHINode>(V);
	if (PN) {
		CVSet.insert(V);
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			Value *IV = PN->getIncomingValue(i);
			findSourceCV(IV, SourceSet, CVSet, TrackedSet);
		}
		return;
	}

	if (LoadInst *LI = dyn_cast<LoadInst>(V)) {
		CVSet.insert(V);
		Value *LPO = LI->getPointerOperand();
		// Get aliases
		Function *F = LI->getParent()->getParent();
		std::set<Value *> AliasSet;
		getAliasPointers(LPO, AliasSet, Ctx->FuncPAResults[F]);

		// To find all stores using the pointer
		// TODO: use alias analysis
		for (Value *A : AliasSet) {
			findSourceCVAlias(V, A, SourceSet, CVSet, TrackedSet);
		}
		return;
	}

	if (isa<ICmpInst>(V)) {
		CVSet.insert(V);
		return;
	}

	// Default policy for unary and binary
	UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V);
	if (UI) {
		CVSet.insert(V);
		Value *UO = UI->getOperand(0);
		findSourceCV(UO, SourceSet, CVSet, TrackedSet);
		return;
	}

	BinaryOperator *BO = dyn_cast<BinaryOperator>(V);
	if (BO) {
		CVSet.insert(V);
		for (unsigned i = 0, e = BO->getNumOperands();
				i != e; ++i) {
			Value *Opd = BO->getOperand(i);
			// Constant wouldn't be critical?
			if (isConstant(Opd))
				continue;

			findSourceCV(Opd, SourceSet, CVSet, TrackedSet);
		}
		return;
	}

	return;

	OP << "== Warning: unsupported LLVM IR when find sources of critical variables:";
	OP << "\033[31m" << *V << "\033[0m\n";
	assert(0);
}

/// Track the source of the CriticalVariable in the current function.
/// First, determine the type of the LoadPointer, identify the alias
/// if any. For each alias, extract the store operand.
void DataFlowAnalysis::findInFuncSourceCV(Value *V, set<Value *> &SourceSet, 
		set<Value *> &CVSet, set<Value *> &TrackedSet) {

	if (TrackedSet.count(V) != 0) 
		return;
	TrackedSet.insert(V);

	if (isConstant(V)
			|| isa<Argument>(V)
			|| isa<GlobalVariable>(V)
			|| isa<AllocaInst>(V)
	   ) {
		SourceSet.insert(V);
		CVSet.insert(V);
		return;
	}

	if (isa<ConstantExpr>(V)) {
		ConstantExpr *CE = dyn_cast<ConstantExpr>(V);
		findInFuncSourceCV(CE->getOperand(0), SourceSet, CVSet, TrackedSet);
		return;
	}

	if (CallInst *CI = dyn_cast<CallInst>(V)) {
		SourceSet.insert(CI);
		CVSet.insert(CI);
		return;
	}

	SelectInst *SI = dyn_cast<SelectInst>(V);
	if (SI) {
		CVSet.insert(V);
		findInFuncSourceCV(SI->getTrueValue(), SourceSet, CVSet, TrackedSet);
		findInFuncSourceCV(SI->getFalseValue(), SourceSet, CVSet, TrackedSet);
		return;
	}

	GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V);
	if (GEP) {
		CVSet.insert(V);
		// TODO: consider aliases
		findInFuncSourceCV(GEP->getPointerOperand(), SourceSet, CVSet, TrackedSet);
		return;
	}

	if (PHINode *PN = dyn_cast<PHINode>(V)) {
		CVSet.insert(V);
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			Value *IV = PN->getIncomingValue(i);
			findInFuncSourceCV(IV, SourceSet, CVSet, TrackedSet);
		}
		return;
	}

	if (LoadInst *LI = dyn_cast<LoadInst>(V)) {
		CVSet.insert(V);
		Value *LPO = LI->getPointerOperand();
		// Get aliases
		Function *F = LI->getParent()->getParent();
		std::set<Value *> AliasSet;
		getAliasPointers(LPO, AliasSet, Ctx->FuncPAResults[F]);

		// To find all stores using the pointer
		// TODO: use alias analysis
		set<BasicBlock *> reachBBs;
		collectPredReachBlocks(LI->getParent(), reachBBs);

		for (Value *A : AliasSet) {
			for (User *U : A->users()) {

				Instruction *I = dyn_cast<Instruction>(U);
				if (!I)
					continue;
				if (reachBBs.find(I->getParent()) == reachBBs.end())
					continue;

				StoreInst *SI = dyn_cast<StoreInst>(U);
				if (SI && A == SI->getPointerOperand()) {
					Value *SVO = SI->getValueOperand();
					findInFuncSourceCV(SVO, SourceSet, CVSet, TrackedSet);
					continue;
				}

				// Find sources from external, e.g., like copy_from_user
				// TODO: need further investigation
				if (CallInst *CI = dyn_cast<CallInst>(U) ) {
					// Get ArgNo
					int8_t ArgNo = -1; 
					CallSite CS(CI);
					for (CallSite::arg_iterator ai = CS.arg_begin(),
							ae = CS.arg_end(); ai != ae; ++ai) {
						++ArgNo;
						if (A == *ai)
							break;
					}
					SourceSet.insert(CI->getOperand(ArgNo));
					CVSet.insert(CI->getOperand(ArgNo));
					continue;
				}
			}
		}

		// FIXME: further track LoadInst?
		findInFuncSourceCV(LI->getPointerOperand(), 
				SourceSet, CVSet, TrackedSet);

		return;
	}

	if (isa<ICmpInst>(V)) {
		CVSet.insert(V);
		return;
	}

	// Default policy for unary and binary
	UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V);
	if (UI) {
		CVSet.insert(V);
		Value *UO = UI->getOperand(0);
		findInFuncSourceCV(UO, SourceSet, CVSet, TrackedSet);
		return;
	}

#if 1
	BinaryOperator *BO = dyn_cast<BinaryOperator>(V);
	if (BO) {
		CVSet.insert(V);
		for (unsigned i = 0, e = BO->getNumOperands();
				i != e; ++i) {
			Value *Opd = BO->getOperand(i);
			findInFuncSourceCV(Opd, SourceSet, CVSet, TrackedSet);
		}
		return;
	}
#endif

	return;
}

/// Collect sources of a Target by backward Dataflow analysis
void DataFlowAnalysis::findSources(Value *V, set<Value *> &SrcSet) {

	// If the CV is modified by the return value of a function
	CallInst *CI = dyn_cast<CallInst>(V);
	if (CI) {
		Function *CF = CI->getCalledFunction();
		if (!CF) return;

		// Explicit handling of overflow operations, treat as binary operator
		if (CF->getName().contains("with.overflow")) {
			for (unsigned int i = 0; i < CI->getNumArgOperands(); ++i)
				findSources(CI->getArgOperand(i), SrcSet);
			return;
		}
		return;
	}

	if (dyn_cast<GlobalVariable>(V)) {
		// FIXME: ???
		Constant *Ct = dyn_cast<Constant>(V);
		if (!Ct) return;

		SrcSet.insert(V);
		return;
	}
	// FIXME: constants

	if (dyn_cast<ConstantExpr>(V)) {
		ConstantExpr *CE = dyn_cast<ConstantExpr>(V);
		findSources(CE->getOperand(0), SrcSet);
		return;
	}

	LoadInst *LI = dyn_cast<LoadInst>(V);
	if (LI) {
		// FIXME: this is wrong
		findSources(LI->getPointerOperand(), SrcSet);
		return;
	}

	SelectInst *SI = dyn_cast<SelectInst>(V);
	if (SI) {
		findSources(SI->getTrueValue(), SrcSet);
		findSources(SI->getFalseValue(), SrcSet);
		return;
	}

	GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V);
	if (GEP) 
		return findSources(GEP->getPointerOperand(), SrcSet);


	PHINode *PN = dyn_cast<PHINode>(V);
	if (PN) {
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			Value *IV = PN->getIncomingValue(i);
			findSources(IV, SrcSet);
		}
		return;
	}

	ICmpInst *ICmp = dyn_cast<ICmpInst>(V);
	if (ICmp) {
		for (unsigned i = 0; i < ICmp->getNumOperands(); ++i) {
			Value *Opd = ICmp->getOperand(i);
			findSources(Opd, SrcSet);
		}
		return;
	}

	BinaryOperator *BO = dyn_cast<BinaryOperator>(V);
	if (BO) {
		for (unsigned i = 0, e = BO->getNumOperands(); i != e; ++i) {
			Value *Opd = BO->getOperand(i);
			if (dyn_cast<Constant>(Opd))
				continue;
			findSources(Opd, SrcSet);
		}
		return;
	}

	UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V);
	if (UI) {
		findSources(UI->getOperand(0), SrcSet);
		return;
	}

	if (dyn_cast<Constant>(V)) {
		SrcSet.insert(V);
		return;
	}
}

/// Collect uses for a Target using forward dataflow analysis
void DataFlowAnalysis::findUses(Instruction *BorderInsn, Value *Target, 
		set<use_t> &UseSet, set<Value *> &Visited) {

	if (!BorderInsn)
		return;

	if (Visited.find(Target) != Visited.end())
		return;
	Visited.insert(Target);

	BasicBlock *BorderBB = BorderInsn->getParent();
	set<BasicBlock *> reachBBs;
	collectSuccReachBlocks(BorderBB, reachBBs);

	//Search the uses of the Target
	for (User *U : Target->users()) {

		Instruction *Insn = dyn_cast<Instruction>(U);
		if (!Insn || (Insn == BorderInsn))
			continue;

		BasicBlock *BB = Insn->getParent();

		//Use 1: Dereferencing a pointer or a struct
		if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(U)) {
			if (reachBBs.find(BB) != reachBBs.end()) {
				UseSet.insert(use_c(Insn, -1));

				if (GEP->getPointerOperand() == U)
					findUses(BorderInsn, U, UseSet, Visited);
			}
			continue;
		}
		//Use 2: Accessing memory
		LoadInst *LI = dyn_cast<LoadInst>(U);
		if (LI && (Target == LI->getPointerOperand())) {
			// Used as the address of a load operation
			// Mark it as a critical use
			if (reachBBs.find(BB) != reachBBs.end()) {
				UseSet.insert(use_c(Insn, -1));
			}
			// TODO: may further track?
			continue;
		}
		if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
			if (Target == SI->getPointerOperand()) {
				if (reachBBs.find(BB) != reachBBs.end()) {
					UseSet.insert(use_c(Insn, -1));
				}
			}
			// Used as the value operand
			else {
				set<Value *> AliasSet;
				getAliasPointers(SI->getPointerOperand(), AliasSet, 
						Ctx->FuncPAResults[SI->getParent()->getParent()]);
				for (Value *A : AliasSet) {
					for (User *AU : A->users()) {

						Instruction *I = dyn_cast<Instruction>(AU);
						if (!I) continue;

						if (reachBBs.find(I->getParent()) == reachBBs.end())
							continue;

						LoadInst *LI = dyn_cast<LoadInst>(AU);
						if (LI) 
							findUses(BorderInsn, AU, UseSet, Visited);
					}
				}
			}
			continue;
		}

		if (CmpInst *CI = dyn_cast<CmpInst>(U)) {
			if (reachBBs.find(BB) != reachBBs.end()) {
				UseSet.insert(use_c(Insn, -1));
			}
			continue;
		}

		//Use 3: Taking parameters in a function call
		if (CallInst *CI = dyn_cast<CallInst>(U)) {
			// Used as the callee or parameter of a CallInst
			// Mark it as a critical use
			if (reachBBs.find(BB) != reachBBs.end()) {
				int8_t ArgNo = -1;
				CallSite CS(CI);
				for (CallSite::arg_iterator ai = CS.arg_begin(),
						ae = CS.arg_end(); ai != ae; ++ai) {
					++ArgNo;
					if (Target == *ai)
						break;
				}
				UseSet.insert(use_c(Insn, ArgNo));
			}
			continue;
		}

		// Skip the following instructions 
		if (dyn_cast<ExtractElementInst>(U) ||
				dyn_cast<InsertElementInst>(U)  ||
				dyn_cast<ExtractValueInst>(U)   ||
				dyn_cast<InsertValueInst>(U)    ||
				dyn_cast<ShuffleVectorInst>(U)  ||
				dyn_cast<ReturnInst>(U))
			continue;

		// Skip the binary operator if one of the operand is a constant
		// Use 4: possible division operation
		if (auto BO = dyn_cast<BinaryOperator>(U)) {
			Value *opd0 = BO->getOperand(0);
			Value *opd1 = BO->getOperand(1);

			if (dyn_cast<Constant>(opd0) || dyn_cast<Constant>(opd1))
				continue;

			findUses(BorderInsn, U, UseSet, Visited);
			continue;
		}

		if (isa<UnaryInstruction>(U)) {
			findUses(BorderInsn, U, UseSet, Visited);
			continue;
		}
	}
}

/// Find code paths for data flows from Start to End
void DataFlowAnalysis::findPaths(Value *Start, Value *End, set<Path> &PathSet) {

}


/// Find code paths for data flows from Start
void DataFlowAnalysis::findPaths(Value *Start, set<Path> &PathSet) {
	
	// Use DFS to collect paths
}

bool DataFlowAnalysis::possibleUseStResult(Instruction *Inst, 
		Instruction *St) {

	BasicBlock *InstBB = Inst->getParent();
	BasicBlock *StBB = St->getParent();
	std::set<BasicBlock *> PB;
	std::list<BasicBlock *> EB;

	if (!InstBB || !StBB)
		return false;

	// Inst and St are in the same basic block.
	// Iterate over each instruction see which
	// one is seen firstly.
	if (InstBB == StBB) {
		for (BasicBlock::iterator I = InstBB->begin(),
				IE = InstBB->end(); I != IE; ++I) {
			Instruction *II = dyn_cast<Instruction>(&*I);
			if (II == St)
				return true;
			if (II == Inst)
				return false;
		}
		return false;
	}

	// See if there exists a path from StBB to LdBB.
	PB.clear();
	EB.clear();

	EB.push_back(StBB);
	while (!EB.empty()) {
		BasicBlock *TB = EB.front();
		Instruction *TI;

		EB.pop_front();
		if (PB.count(TB) != 0)
			continue;
		PB.insert(TB);

		if (TB == InstBB)
			return true;

		TI = TB->getTerminator();
		for (unsigned i = 0, e = TI->getNumSuccessors();
				i != e; ++i) {
			EB.push_back(TI->getSuccessor(i));
		}
	}

	return false;
}

void DataFlowAnalysis::performBackwardAnalysis(Function* F, Value* V, 
		set<Value *>& CISet) {

	if (isa<CallInst>(V)) {
		CISet.insert(V);
		return;
	}

	if (dyn_cast<Constant>(V))
		return;

	if (isa<Argument>(V))
		return;

	if (isa<LoadInst>(V)) {
		LoadInst* LI = dyn_cast<LoadInst>(V);
		if (LPSet.find(LI->getPointerOperand()) != LPSet.end())
			return;
		else
			LPSet.insert(LI->getPointerOperand());

		for (auto i = inst_begin(F), e = inst_end(F); i != e; ++i) {
			StoreInst *SI = dyn_cast<StoreInst>(&*i);
			if (!SI) 
				continue;
			if (!possibleUseStResult(LI, SI)) 
				continue;
			if (SI->getPointerOperand() != LI->getPointerOperand()) 
				continue;
			performBackwardAnalysis(F, SI->getValueOperand(), CISet);
		}
		return;
	}

	if (isa<UnaryInstruction>(V)) {
		UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V);
		performBackwardAnalysis(F, UI->getOperand(0), CISet);
		return;
	}

	if (isa<BinaryOperator>(V)) {
		BinaryOperator *BI = dyn_cast<BinaryOperator>(V);
		performBackwardAnalysis(F, BI->getOperand(0), CISet);
		performBackwardAnalysis(F, BI->getOperand(1), CISet);
		return;
	}

	if (isa<ICmpInst>(V)) {
		ICmpInst *CI = dyn_cast<ICmpInst>(V);
		performBackwardAnalysis(F, CI->getOperand(0), CISet);
		performBackwardAnalysis(F, CI->getOperand(1), CISet);
		return;
	}

	if (isa<FCmpInst>(V)) {
		FCmpInst *CI = dyn_cast<FCmpInst>(V);
		performBackwardAnalysis(F, CI->getOperand(0), CISet);
		performBackwardAnalysis(F, CI->getOperand(1), CISet);
		return;
	}

	PHINode *PN = dyn_cast<PHINode>(V);
	if (PN) {
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			Value *IV = PN->getIncomingValue(i);
			performBackwardAnalysis(F, IV, CISet);
		}
		return;
	}

	if (isa<GetElementPtrInst>(V)) {
		GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V);
		performBackwardAnalysis(F, GEP->getPointerOperand(), CISet);
		return;
	}

	if (isa<SelectInst>(V)) {
		SelectInst *SI = dyn_cast<SelectInst>(V);
		performBackwardAnalysis(F, SI->getTrueValue(), CISet);
		performBackwardAnalysis(F, SI->getFalseValue(), CISet);
		return;
	}
	OP << "== Warning: unsupported LLVM IR:" << *V <<  " in " 
		<< F->getName() << "\n";

#ifdef DEBUG_PRINT
	assert(0);
#endif
}
