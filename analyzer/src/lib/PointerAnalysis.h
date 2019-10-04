#ifndef POINTER_ANALYSIS_H
#define POINTER_ANALYSIS_H

#include "Analyzer.h"

class PointerAnalysisPass : public IterativeModulePass {
	typedef std::pair<Value *, MemoryLocation *> AddrMemPair;

	private:
	TargetLibraryInfo *TLI;

	void collectPointers(Function *, set<Value *> &PSet);

	void detectAliasPointers(Function *, AAResults &,
			PointerAnalysisMap &);

	void augmentMustAlias(Function *F, Value *P, set<Value *> &ASet);
	Value *getSourcePointer(Value *);

	public:
	PointerAnalysisPass(GlobalContext *Ctx_)
		: IterativeModulePass(Ctx_, "PointerAnalysis") { }
	virtual bool doInitialization(llvm::Module *);
	virtual bool doFinalization(llvm::Module *);
	virtual bool doModulePass(llvm::Module *);
};

#endif
