#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/DebugInfo.h>

#include "TypeInitializer.h"
#include "Common.h"
// map type* to global value name
map<Type*, string> TypeValueMap;

// map global value name to type name
map<string,string> VnameToTypenameMap;
map<Type*, string> TypeToTNameMap;

bool TypeInitializerPass::doInitialization(Module *M) {
	
	// Initializing TypeValueMap
	// Map every gloable struct wihout type name to their
	// value name
	for(Module::global_iterator gi = M->global_begin(),
			ge = M->global_end(); gi != ge; ++gi) {
		GlobalValue *GV = &*gi;
		Type *GVTy = GV->getValueType();
		string Vname = GV->getName();
		if (StructType *GVSTy = dyn_cast<StructType>(GVTy)) {
			if(GVSTy->hasName())
				continue;
			//OP<<*GVTy<<"\tVName: "<<Vname<<"\n";
			TypeValueMap.insert(pair<Type*, string>(GVTy,Vname));
		}
	}
	
	// Initializing StructTNMap
	// Map global variable name to their struct type name
	for (Module::iterator ff = M->begin(),
			MEnd = M->end();ff != MEnd; ++ff) {
		Function *Func = &*ff;
		if (Func->isIntrinsic())
			continue;
		for (inst_iterator ii = inst_begin(Func), e = inst_end(Func);
					ii != e; ++ii) {
			Instruction *Inst = &*ii;
			unsigned T = Inst->getNumOperands();
			for(int i = 0; i < T; i++) {
				Value *VI = Inst->getOperand(i);
				if (!VI)
					continue;
				if (!VI->hasName())
					continue;
				Type *VT = VI->getType();
				while(VT && VT->isPointerTy())
					VT = VT->getPointerElementType();
				if (!VT)
					continue;

				if (StructType *SVT = dyn_cast<StructType>(VT)) {
					string ValueName = VI->getName();
					string StructName = SVT->getName();
					//OP<<ValueName<<"\t"<<StructName<<"\n";
					VnameToTypenameMap.insert(pair<string, string>(ValueName,StructName));
				}
	
			}
		}
	}
	
	return false;
}
bool TypeInitializerPass::doFinalization(Module *M) {
	return false;
}

void TypeInitializerPass::BuildTypeStructMap(){
	// build GlobalTypes based on TypeValueMap and VnameToTypenameMap	 
	for (auto const& P1 : TypeValueMap) {
		if (VnameToTypenameMap.find(P1.second) != VnameToTypenameMap.end()) {
			Ctx->GlobalTypes.insert(pair<Type*, string>(P1.first,VnameToTypenameMap[P1.second]));
			//OP<<*P1.first<<"\t"<<VnameToTypenameMap[P1.second]<<"\n";
		}
	
	}
	TypeToTNameMap = Ctx->GlobalTypes;
}

bool TypeInitializerPass::doModulePass(Module *M) {
	//
	return false;
}
