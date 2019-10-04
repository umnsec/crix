#ifndef DATA_FLOW_ANALYSIS_H
#define DATA_FLOW_ANALYSIS_H

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/CFG.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Format.h>
#include <set>
#include <list>
#include <map>
#include <string>
#include "Analyzer.h"
#include "Common.h"

using namespace llvm;

typedef pair<Value *, int8_t> use_t;
typedef pair<Value *, int8_t> src_t;

pair<Value *, int8_t> use_c(Value *V, int8_t Arg);
pair<Value *, int8_t> src_c(Value *V, int8_t Arg);

struct Path {
	Value *Start;
	Value *End;

	list<BasicBlock *>BList;
};

class DataFlowAnalysis {

	public:
		DataFlowAnalysis(GlobalContext *GCtx) {Ctx = GCtx;}
		~DataFlowAnalysis() {}

		void findSources(Value *V, set<Value *> &SrcSet);
		void findUses(Instruction *BorderInsn, Value *V, 
				set<use_t> &UseSet, set<Value *> &Visited);

		void performBackwardAnalysis(Function *F, Value *V, set<Value *> &);
		void resetStructures() { LPSet.clear(); } 

		bool possibleUseStResult(Instruction *, Instruction *);

		// Track the sources and same-origin critical variables of the
		// given critical variable.
		void findSourceCV(Value *V, set<Value *> &SourceSet, 
				set<Value *> &CVSet, set<Value *> &TrackedSet);
		void findInFuncSourceCV(Value *V, set<Value *> &SourceSet, 
				set<Value *> &CVSet, set<Value *> &TrackedSet);

		// Find code paths for data flows from Start to End
		void findPaths(Value *Start, Value *End, set<Path> &PathSet);
		// Find code paths for data flows from Start
		void findPaths(Value *Start, set<Path> &PathSet);

		// Track the sources and same-origin critical variables of the
		// given pointer of a critical variable.
		void findSourceCVAlias(Value *V, Value *Ptr, set<Value *> &SourceSet, 
				set<Value *> &CVSet, set<Value *> &TrackedSet);

		void collectSuccReachBlocks(BasicBlock *BB, 
				set<BasicBlock *> &reachBB);
		void collectPredReachBlocks(BasicBlock *BB,
				set<BasicBlock *> &reachBB);


		void getAliasPointers(Value *Addr,
				std::set<Value *> &aliasAddr,
				PointerAnalysisMap &aliasPtrs);
	private:
		// Set of LoadPointers
		std::set<Value *> LPSet; 
		GlobalContext *Ctx;
};

#endif

