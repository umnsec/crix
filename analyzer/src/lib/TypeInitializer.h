#include "Analyzer.h"

class TypeInitializerPass : public IterativeModulePass {

	public:
		TypeInitializerPass(GlobalContext *Ctx_)
				: IterativeModulePass(Ctx_, "TypeInitializer") { }
		virtual bool doInitialization(llvm::Module *);
		virtual bool doFinalization(llvm::Module *);
		virtual bool doModulePass(llvm::Module *);
		void BuildTypeStructMap();
};
