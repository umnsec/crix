#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/LegacyPassManager.h>

#include "PointerAnalysis.h"

/// Alias types used to do pointer analysis.
#define MUST_ALIAS

bool PointerAnalysisPass::doInitialization(Module *M) {
	return false;
}

bool PointerAnalysisPass::doFinalization(Module *M) {
	return false;
}

Value *PointerAnalysisPass::getSourcePointer(Value *P) {

	Value *SrcP = P;
	Instruction *SrcI = dyn_cast<Instruction>(SrcP);

	std::list<Value *> EI;

	EI.push_back(SrcP);
	while (!EI.empty()) {
		Value *TI = EI.front();
		EI.pop_front();

		// Possible sources
		if (isa<Argument>(TI)
				|| isa<AllocaInst>(TI)
				|| isa<CallInst>(TI)
				|| isa<GlobalVariable>(TI)
		   )
			return SrcP;

		if (UnaryInstruction *UI = dyn_cast<UnaryInstruction>(TI)) {
			Value *UO = UI->getOperand(0);
			if (UO->getType()->isPointerTy() && isa<Instruction>(UO)) {
				SrcP = UO;
				EI.push_back(SrcP);
			}
		}
		else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(TI)) {
			SrcP = GEP->getPointerOperand();
			EI.push_back(SrcP);
		}
	}

	return SrcP;
}

void PointerAnalysisPass::augmentMustAlias(Function *F, Value *P, 
		set<Value *> &ASet) {

	std::set<Value *> PV;
	std::list<Value *> EV;

	EV.push_back(P);
	while (!EV.empty()) {
		Value *TV = EV.front();
		EV.pop_front();
		if (PV.count(TV) != 0)
			continue;
		PV.insert(TV);

		if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(TV)) {
			EV.push_back(GEP->getPointerOperand());
			continue;
		}
		if (CastInst *CI = dyn_cast<CastInst>(TV)) {
			EV.push_back(CI->getOperand(0));
			continue;
		}
		if (AllocaInst *AI = dyn_cast<AllocaInst>(TV)) {
			EV.push_back(AI);
			continue;
		}
	}

	for (auto V : PV) {
		ASet.insert(V);
		for (auto U : V->users()) {
			if (isa<GetElementPtrInst>(U)) 
				ASet.insert(U);
			else if (isa<CastInst>(U))
				ASet.insert(U);
		}
	}
}

/// Collect all interesting pointers 
void PointerAnalysisPass::collectPointers(Function *F, 
		set<Value *> &PSet) {

	// Scan instructions to extract all pointers.
	for (inst_iterator i = inst_begin(F), ei = inst_end(F);
			i != ei; ++i) {

		Instruction *I = dyn_cast<Instruction>(&*i);
		if (!(I->getType()->isPointerTy()))
			continue;

		if (isa<LoadInst>(I)
				|| isa<GetElementPtrInst>(I)
				|| isa<CastInst>(I)
		   )
			PSet.insert(I);
	}
}

/// Detect aliased pointers in this function.
void PointerAnalysisPass::detectAliasPointers(Function *F,
		AAResults &AAR,
		PointerAnalysisMap &aliasPtrs) {

	std::set<Value *> addr1Set;
	std::set<Value *> addr2Set;
	Value *Addr1, *Addr2;

	// Collect interesting pointers
	for (inst_iterator i = inst_begin(F), ei = inst_end(F);
			i != ei; ++i) {

		Instruction *I = dyn_cast<Instruction>(&*i);

		if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
			addr1Set.insert(LI->getPointerOperand());
		}
		else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
			addr1Set.insert(SI->getPointerOperand());
		}
		else if ( CallInst *CI = dyn_cast<CallInst>(I)) {
			for (unsigned j = 0, ej = CI->getNumArgOperands();
					j < ej; ++j) {
				Value *Arg = CI->getArgOperand(j);

				if (!Arg->getType()->isPointerTy())
					continue;
				addr1Set.insert(Arg);
			}
		}
	}

	// FIXME: avoid being stuck
	if (addr1Set.size() > 1000) {
		return;
	}

	for (auto Addr1 : addr1Set) {

		for (auto Addr2 : addr1Set) {

			if (Addr1 == Addr2)
				continue;

			AliasResult AResult = AAR.alias(Addr1, Addr2);

			bool notAlias = true;

			if (AResult == MustAlias || AResult == PartialAlias)
				notAlias = false;

			else if (AResult == MayAlias) {
#ifdef MUST_ALIAS
				if (getSourcePointer(Addr1) == getSourcePointer(Addr2))
					notAlias = false;
#else
				notAlias = false;
#endif
			}

			if (notAlias)
				continue;

			auto as = aliasPtrs.find(Addr1);
			if (as == aliasPtrs.end()) {
				SmallPtrSet<Value *, 16> sv;
				sv.insert(Addr2);
				aliasPtrs[Addr1] = sv;
			} 
			else {
				as->second.insert(Addr2);
			}
		}
	}
}

bool PointerAnalysisPass::doModulePass(Module *M) {

	// Save TargetLibraryInfo.
	Triple ModuleTriple(M->getTargetTriple());
	TargetLibraryInfoImpl TLII(ModuleTriple);
	TLI = new TargetLibraryInfo(TLII);

	// Run BasicAliasAnalysis pass on each function in this module.
	// XXX: more complicated alias analyses may be required.
	legacy::FunctionPassManager *FPasses = new legacy::FunctionPassManager(M);
	AAResultsWrapperPass *AARPass = new AAResultsWrapperPass();

	FPasses->add(AARPass);

	FPasses->doInitialization();
	for (Function &F : *M) {
		if (F.isDeclaration())
			continue;
		FPasses->run(F);
	}
	FPasses->doFinalization();

	// Basic alias analysis result.
	AAResults &AAR = AARPass->getAAResults();

	for (Module::iterator f = M->begin(), fe = M->end();
			f != fe; ++f) {
		Function *F = &*f;
		PointerAnalysisMap aliasPtrs;

		if (F->empty())
			continue;

		detectAliasPointers(F, AAR, aliasPtrs);

		// Save pointer analysis result.
		Ctx->FuncPAResults[F] = aliasPtrs;
		Ctx->FuncAAResults[F] = &AAR;
	}

	return false;
}
