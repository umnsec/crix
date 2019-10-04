#ifndef SECURITY_CHECKS_H
#define SECURITY_CHECKS_H

#include "Analyzer.h"
#include "Common.h"



class SecurityChecksPass : public IterativeModulePass {

	typedef std::pair<Instruction *, BasicBlock *> CFGEdge;
	typedef std::pair<CFGEdge, Value *> EdgeValue;

	enum ErrFlag {
		// error returning, mask:0xF
		Must_Return_Err = 1,
		May_Return_Err = 2,
		Reserved_Return1 = 4,
		Reserved_Return2 = 8,
		// error handling, mask: 0xF0
		Must_Handle_Err = 16,
		May_Handle_Err = 32,
		Reserved_Handle1 = 64,
		Reserved_Handle2 = 128,

		Completed_Flag = 256,
	};

	public:

	typedef std::map<CFGEdge, int> EdgeErrMap;
	typedef std::map<BasicBlock *, int> BBErrMap;

	static set<Instruction *>ErrSelectInstSet;

	private:

	// Dump marked edges.
	void dumpErrEdges(EdgeErrMap &edgeErrMap);
	bool isValueErrno(Value *V, Function *F);

	///
	/// Identifying sanity checks
	///
	// Find and record blocks with error handling
	void checkErrReturn(Function *F, BBErrMap &bbErrMap);

	// Find and record blocks with error handling 
	void checkErrHandle(Function *F, BBErrMap &bbErrMap);

	// Mark the given block with an error flag.
	void markBBErr(BasicBlock *BB, ErrFlag flag, BBErrMap &bbErrMap);

	// A lighweiht and inprecise way to check if the function may
	// return an error
	bool mayReturnErr(Function *F);

	// Collect all blocks that influence the return value
	void checkErrValueFlow(Function *F, ReturnInst *RI, 
			std::set<Value *> &PV, BBErrMap &bbErrMap);

	// Traverse CFG to mark all edges with error flags
	bool markAllEdgesErrFlag(Function *F, BBErrMap &bbErrMap, EdgeErrMap &edgeErrMap);

	// Recursively mark all edges from the given block
	void recurMarkEdgesFromBlock(CFGEdge &CE, int flag, 
			BBErrMap &bbErrMap, EdgeErrMap &edgeErrMap);

	// Recursively mark all edges to the given block
	void recurMarkEdgesToBlock(CFGEdge &CE, int flag, 
			BBErrMap &bbErrMap, EdgeErrMap &edgeErrMap);

	// Recursively mark edges from the error-handling block to the
	// closest branches
	void recurMarkEdgesToErrHandle(BasicBlock *BB, EdgeErrMap &edgeErrMap);

	// Recursively mark edges to the error-returning block
	void recurMarkEdgesToErrReturn(BasicBlock *BB, int flag, EdgeErrMap &edgeErrMap);

	// Mark direct edges to the error-returning block
	void markEdgesToErrReturn(BasicBlock *BB, int flag, EdgeErrMap &edgeErrMap);

	// Add identified checks to the set
	void addSecurityCheck(Value *, Value *, std::set<SecurityCheck *> &);

	// Incorporate newFlag into existing flag
	void updateReturnFlag(int &errFlag, int &newFlag);
	void updateHandleFlag(int &errFlag, int &newFlag);
	void mergeFlag(int &errFlag, int &newFlag);

	// Find error code based on error handling functions
	void findErrorCodes(Function *F);

	// Find same-origin variables from the given variable
	void findSameVariablesFrom(Value *V, std::set<Value *> &VSet);

	// infer error-handling branch for a condition
	int inferErrBranch(Instruction *Cond);


	public:

	SecurityChecksPass(GlobalContext *Ctx_)
		: IterativeModulePass(Ctx_, "SecurityChecks") {
		}
	virtual bool doInitialization(llvm::Module *);
	virtual bool doFinalization(llvm::Module *);
	virtual bool doModulePass(llvm::Module *);

	// Identify security checks.
	void identifySecurityChecks(Function *F, 
			EdgeErrMap &edgeErrMap, 
			set<SecurityCheck *> &SCSet);

};

#endif
