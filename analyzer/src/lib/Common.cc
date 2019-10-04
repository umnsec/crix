#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/InstIterator.h>
#include <fstream>
#include <regex>
#include "Common.h"

// To print source code information, configure the path
#define LINUX_SOURCE "/your-path-of-the-linux-source/linux-5.3.0"

bool trimPathSlash(string &path, int slash) {
	while (slash > 0) {
		path = path.substr(path.find('/') + 1);
		--slash;
	}

	return true;
}

string getFileName(DILocation *Loc, DISubprogram *SP) {
	string FN;
	if (Loc)
		FN = Loc->getFilename().str();
	else if (SP)
		FN = SP->getFilename().str();
	else
		return "";

	// TODO: require config
	int slashToTrim = 2;
	trimPathSlash(FN, slashToTrim);
	FN = string(LINUX_SOURCE) + "/" + FN;
	return FN;
}

/// Check if the value is a constant.
bool isConstant(Value *V) {
  // Invalid input.
  if (!V) 
    return false;

  // The value is a constant.
  Constant *Ct = dyn_cast<Constant>(V);
  if (Ct) 
    return true;

  return false;
}

/// Get the source code line
string getSourceLine(string fn_str, unsigned lineno) {
	std::ifstream sourcefile(fn_str);
	string line;
	sourcefile.seekg(ios::beg);
	
	for(int n = 0; n < lineno - 1; ++n){
		sourcefile.ignore(std::numeric_limits<streamsize>::max(), '\n');
	}
	getline(sourcefile, line);

	return line;
}

string getSourceFuncName(Instruction *I) {

	DILocation *Loc = getSourceLocation(I);
	if (!Loc)
		return "";
	unsigned lineno = Loc->getLine();
	std::string fn_str = getFileName(Loc);
	string line = getSourceLine(fn_str, lineno);
	
	while(line[0] == ' ' || line[0] == '\t')
		line.erase(line.begin());
	line = line.substr(0, line.find('('));
	return line;
}

string extractMacro(string line, Instruction *I) {
	string macro, word, FnName;
	std::regex caps("[^\\(][_A-Z][_A-Z0-9]+[\\);,]+");
	smatch match;
	
	// detect function macros
	if (CallInst *CI = dyn_cast<CallInst>(I)) {
		FnName = getCalledFuncName(CI);
		caps = "[_A-Z][_A-Z0-9]{2,}";
		std::regex keywords("(\\s*)(for|if|while)(\\s*)(\\()");

		if (regex_search(line, match, keywords))
		  line = line.substr(match[0].length());
		
		if (line.find(FnName) != std::string::npos) {
			if (regex_search(FnName, match, caps))
				return FnName;

		} else {
			//identify non matching functions as macros
			//std::count(line.begin(), line.end(), '"') > 0
			std::size_t eq_pos = line.find_last_of("=");
			if (eq_pos == std::string::npos)
				eq_pos = 0;
			else
				++eq_pos;

			std::size_t paren = line.find('(', eq_pos);
			return line.substr(eq_pos, paren-eq_pos);
		}

	} else {
		// detect macro constant variables
		std::size_t lhs = -1;
		stringstream iss(line.substr(lhs+1));

		while (iss >> word) {
			if (regex_search(word, match, caps)) {
				macro = word;
				return macro;
			}
		}
	}

	return "";
}

/// Get called function name of V.
StringRef getCalledFuncName(Instruction *I) {

  Value *V;
	if (CallInst *CI = dyn_cast<CallInst>(I))
		V = CI->getCalledValue();
	else if (InvokeInst *II = dyn_cast<InvokeInst>(I))
		V = II->getCalledValue();
	assert(V);

  InlineAsm *IA = dyn_cast<InlineAsm>(V);
  if (IA)
    return StringRef(IA->getAsmString());

  User *UV = dyn_cast<User>(V);
  if (UV) {
    if (UV->getNumOperands() > 0) {
			Value *VUV = UV->getOperand(0);
			return VUV->getName();
		}
  }

	return V->getName();
}

DILocation *getSourceLocation(Instruction *I) {
  if (!I)
    return NULL;

  MDNode *N = I->getMetadata("dbg");
  if (!N)
    return NULL;

  DILocation *Loc = dyn_cast<DILocation>(N);
  if (!Loc || Loc->getLine() < 1)
		return NULL;

	return Loc;
}

/// Print out source code information to facilitate manual analyses.
void printSourceCodeInfo(Value *V) {
	Instruction *I = dyn_cast<Instruction>(V);
	if (!I)
		return;

	DILocation *Loc = getSourceLocation(I);
	if (!Loc)
		return;

	unsigned LineNo = Loc->getLine();
	std::string FN = getFileName(Loc);
	string line = getSourceLine(FN, LineNo);
	FN = Loc->getFilename().str();
	FN = FN.substr(FN.find('/') + 1);
	FN = FN.substr(FN.find('/') + 1);

	while(line[0] == ' ' || line[0] == '\t')
		line.erase(line.begin());
	OP << " ["
		<< "\033[34m" << "Code" << "\033[0m" << "] "
		<< FN
		<< " +" << LineNo << ": "
		<< "\033[35m" << line << "\033[0m" <<'\n';
}

void printSourceCodeInfo(Function *F) {

	DISubprogram *SP = F->getSubprogram();

	if (SP) {
		string FN = getFileName(NULL, SP);
		string line = getSourceLine(FN, SP->getLine());
		while(line[0] == ' ' || line[0] == '\t')
			line.erase(line.begin());

		FN = SP->getFilename().str();
		FN = FN.substr(FN.find('/') + 1);
		FN = FN.substr(FN.find('/') + 1);

		OP << " ["
			<< "\033[34m" << "Code" << "\033[0m" << "] "
			<< FN
			<< " +" << SP->getLine() << ": "
			<< "\033[35m" << line << "\033[0m" <<'\n';
	}
}

string getMacroInfo(Value *V) {

	Instruction *I = dyn_cast<Instruction>(V);
	if (!I) return "";

	DILocation *Loc = getSourceLocation(I);
	if (!Loc) return "";

	unsigned LineNo = Loc->getLine();
	std::string FN = getFileName(Loc);
	string line = getSourceLine(FN, LineNo);
	FN = Loc->getFilename().str();
	const char *filename = FN.c_str();
	filename = strchr(filename, '/') + 1;
	filename = strchr(filename, '/') + 1;
	int idx = filename - FN.c_str();

	while(line[0] == ' ' || line[0] == '\t')
		line.erase(line.begin());

	string macro = extractMacro(line, I);

	//clean up the ending and whitespaces
	macro.erase(std::remove (macro.begin(), macro.end(),' '), macro.end());
	unsigned length = 0;
	for (auto it = macro.begin(), e = macro.end(); it != e; ++it)
		if (*it == ')' || *it == ';' || *it == ',') {
			macro = macro.substr(0, length);
			break;
		} else {
			++length;
		}

	return macro;
}

/// Get source code information of this value
void getSourceCodeInfo(Value *V, string &file,
                               unsigned &line) {
  file = "";
  line = 0;

  auto I = dyn_cast<Instruction>(V);
  if (!I)
    return;

  MDNode *N = I->getMetadata("dbg");
  if (!N)
    return;

  DILocation *Loc = dyn_cast<DILocation>(N);
  if (!Loc || Loc->getLine() < 1)
    return;

  file = Loc->getFilename().str();
  line = Loc->getLine();
}

Argument *getArgByNo(Function *F, int8_t ArgNo) {

  if (ArgNo >= F->arg_size())
    return NULL;

  int8_t idx = 0;
  Function::arg_iterator ai = F->arg_begin();
  while (idx != ArgNo) {
    ++ai;
    ++idx;
  }
  return ai;
}

//#define HASH_SOURCE_INFO
size_t funcHash(Function *F, bool withName) {

	hash<string> str_hash;
	string output;

#ifdef HASH_SOURCE_INFO
	DISubprogram *SP = F->getSubprogram();

	if (SP) {
		output = SP->getFilename();
		output = output + to_string(uint_hash(SP->getLine()));
	}
	else {
#endif
		string sig;
		raw_string_ostream rso(sig);
		Type *FTy = F->getFunctionType();
		FTy->print(rso);
		output = rso.str();

		if (withName)
			output += F->getName();
#ifdef HASH_SOURCE_INFO
	}
#endif
	string::iterator end_pos = remove(output.begin(), 
			output.end(), ' ');
	output.erase(end_pos, output.end());

	return str_hash(output);
}

size_t callHash(CallInst *CI) {

	CallSite CS(CI);
	Function *CF = CI->getCalledFunction();

	if (CF)
		return funcHash(CF);
	else {
		hash<string> str_hash;
		string sig;
		raw_string_ostream rso(sig);
		Type *FTy = CS.getFunctionType();
		FTy->print(rso);

		string strip_str = rso.str();
		string::iterator end_pos = remove(strip_str.begin(), 
				strip_str.end(), ' ');
		strip_str.erase(end_pos, strip_str.end());
		return str_hash(strip_str);
	}
}

size_t typeHash(Type *Ty) {
	hash<string> str_hash;
	string sig;

	raw_string_ostream rso(sig);
	Ty->print(rso);
	string ty_str = rso.str();
	string::iterator end_pos = remove(ty_str.begin(), ty_str.end(), ' ');
	ty_str.erase(end_pos, ty_str.end());

	return str_hash(ty_str);
}

size_t hashIdxHash(size_t Hs, int Idx) {
	hash<string> str_hash;
	return Hs + str_hash(to_string(Idx));
}

size_t typeIdxHash(Type *Ty, int Idx) {
	return hashIdxHash(typeHash(Ty), Idx);
}

void getSourceCodeLine(Value *V, string &line) {

	line = "";
	Instruction *I = dyn_cast<Instruction>(V);
	if (!I)
		return;

	DILocation *Loc = getSourceLocation(I);
	if (!Loc)
		return;

	unsigned LineNo = Loc->getLine();
	std::string FN = getFileName(Loc);
	line = getSourceLine(FN, LineNo);
	FN = Loc->getFilename().str();
	FN = FN.substr(FN.find('/') + 1);
	FN = FN.substr(FN.find('/') + 1);

	while(line[0] == ' ' || line[0] == '\t')
		line.erase(line.begin());

	return;
}

