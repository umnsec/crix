//===-- Analyzer.cc - the kernel-analysis framework--------------===//
//
// This file implements the analysis framework. It calls the pass for
// building call-graph and the pass for finding security checks.
//
// ===-----------------------------------------------------------===//

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Path.h"

#include <memory>
#include <vector>
#include <sstream>
#include <sys/resource.h>

#include "Analyzer.h"
#include "CallGraph.h"
#include "Config.h"
#include "SecurityChecks.h"
#include "MissingChecks.h"
#include "PointerAnalysis.h"

using namespace llvm;

// Command line parameters.
cl::list<string> InputFilenames(
    cl::Positional, cl::OneOrMore, cl::desc("<input bitcode files>"));

cl::opt<unsigned> VerboseLevel(
    "verbose-level", cl::desc("Print information at which verbose level"),
    cl::init(0));

cl::opt<bool> SecurityChecks(
    "sc", 
    cl::desc("Identify sanity checks"), 
    cl::NotHidden, cl::init(false));

cl::opt<bool> MissingChecks(
		"mc",
		cl::desc("Identify missing-check bugs"),
		cl::NotHidden, cl::init(false));


GlobalContext GlobalCtx;


void IterativeModulePass::run(ModuleList &modules) {

  ModuleList::iterator i, e;
  OP << "[" << ID << "] Initializing " << modules.size() << " modules ";
  bool again = true;
  while (again) {
    again = false;
    for (i = modules.begin(), e = modules.end(); i != e; ++i) {
      again |= doInitialization(i->first);
      OP << ".";
    }
  }
  OP << "\n";

  unsigned iter = 0, changed = 1;
  while (changed) {
    ++iter;
    changed = 0;
    unsigned counter_modules = 0;
    unsigned total_modules = modules.size();
    for (i = modules.begin(), e = modules.end(); i != e; ++i) {
      OP << "[" << ID << " / " << iter << "] ";
      OP << "[" << ++counter_modules << " / " << total_modules << "] ";
      OP << "[" << i->second << "]\n";

      bool ret = doModulePass(i->first);
      if (ret) {
        ++changed;
        OP << "\t [CHANGED]\n";
      } else
        OP << "\n";
    }
    OP << "[" << ID << "] Updated in " << changed << " modules.\n";
  }

  OP << "[" << ID << "] Postprocessing ...\n";
  again = true;
  while (again) {
    again = false;
    for (i = modules.begin(), e = modules.end(); i != e; ++i) {
      // TODO: Dump the results.
      again |= doFinalization(i->first);
    }
  }

  OP << "[" << ID << "] Done!\n\n";
}

void LoadStaticData(GlobalContext *GCtx) {

	// Load error-handling functions
	SetErrorHandleFuncs(GCtx->ErrorHandleFuncs);
	// load functions that copy/move values
	SetCopyFuncs(GCtx->CopyFuncs);
	// load data-fetch functions
	SetDataFetchFuncs(GCtx->DataFetchFuncs);
}

void ProcessResults(GlobalContext *GCtx) {
}

void PrintResults(GlobalContext *GCtx) {

	OP<<"############## Result Statistics ##############\n";
	OP<<"# Number of sanity checks: \t\t\t"<<GCtx->NumSecurityChecks<<"\n";
	OP<<"# Number of conditional statements: \t\t"<<GCtx->NumCondStatements<<"\n";
}

int main(int argc, char **argv) {

	// Print a stack trace if we signal out.
	sys::PrintStackTraceOnErrorSignal(argv[0]);
	PrettyStackTraceProgram X(argc, argv);

	llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

	cl::ParseCommandLineOptions(argc, argv, "global analysis\n");
	SMDiagnostic Err;

	// Loading modules
	OP << "Total " << InputFilenames.size() << " file(s)\n";

	for (unsigned i = 0; i < InputFilenames.size(); ++i) {

		LLVMContext *LLVMCtx = new LLVMContext();
		unique_ptr<Module> M = parseIRFile(InputFilenames[i], Err, *LLVMCtx);

		if (M == NULL) {
			OP << argv[0] << ": error loading file '"
				<< InputFilenames[i] << "'\n";
			continue;
		}

		Module *Module = M.release();
		StringRef MName = StringRef(strdup(InputFilenames[i].data()));
		GlobalCtx.Modules.push_back(make_pair(Module, MName));
		GlobalCtx.ModuleMaps[Module] = InputFilenames[i];
	}

	// Main workflow
	LoadStaticData(&GlobalCtx);

	// Build global callgraph.
	CallGraphPass CGPass(&GlobalCtx);
	CGPass.run(GlobalCtx.Modules);

	// Identify sanity checks
	if (SecurityChecks) {
		SecurityChecksPass SCPass(&GlobalCtx);
		SCPass.run(GlobalCtx.Modules);
	}

	// Identify missing-check bugs
	if (MissingChecks) {
		// Pointer analysis
		PointerAnalysisPass PAPass(&GlobalCtx);
		PAPass.run(GlobalCtx.Modules);

		SecurityChecksPass SCPass(&GlobalCtx);
		SCPass.run(GlobalCtx.Modules);

		MissingChecksPass MCPass(&GlobalCtx);
		MCPass.run(GlobalCtx.Modules);
		MCPass.processResults();
	}

	// Print final results
	//PrintResults(&GlobalCtx);

	return 0;
}

