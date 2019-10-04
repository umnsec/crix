#ifndef SACONFIG_H
#define SACONFIG_H

#include "llvm/Support/FileSystem.h"

#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <fstream>

//
// Configurations for compilation.
//
//#define SOUND_MODE 1

// Skip functions with more blocks to avoid scalability issues
#define MAX_BLOCKS_SUPPORT 500

//
// Function modeling
//

// Setup functions that handle errors
static void SetErrorHandleFuncs(set<string> &ErrorHandleFuncs) {

	string exepath = sys::fs::getMainExecutable(NULL, NULL);
	string exedir = exepath.substr(0, exepath.find_last_of('/'));
	string line;
  ifstream errfile(exedir	+ "/configs/err-funcs");
  if (errfile.is_open()) {
		while (!errfile.eof()) {
			getline (errfile, line);
			if (line.length() > 1) {
				ErrorHandleFuncs.insert(line);
			}
		}
    errfile.close();
  }

	string ErrorHandleFN[] = {
		"BUG",
		"BUG_ON",
		"ASM_BUG",
		"panic",
		"ASSERT",
		"assert",
		"dump_stack",
		"__warn_printk",
		"usercopy_warn",
		"signal_fault",
		"pr_err",
		"pr_warn",
		"pr_warning",
		"pr_alert",
		"pr_emerg",
		"pr_crit",
	};
	for (auto F : ErrorHandleFN) {
		ErrorHandleFuncs.insert(F);
	}
}

// Setup functions that copy/move/cast values.
static void SetCopyFuncs(
		// <src, dst, size>
		map<string, tuple<int8_t, int8_t, int8_t>> &CopyFuncs) {

	CopyFuncs["memcpy"] = make_tuple(1, 0, 2);
	CopyFuncs["__memcpy"] = make_tuple(1, 0, 2);
	CopyFuncs["llvm.memcpy.p0i8.p0i8.i32"] = make_tuple(1, 0, 2);
	CopyFuncs["llvm.memcpy.p0i8.p0i8.i64"] = make_tuple(1, 0, 2);
	CopyFuncs["strncpy"] = make_tuple(1, 0, 2);
	CopyFuncs["memmove"] = make_tuple(1, 0, 2);
	CopyFuncs["__memmove"] = make_tuple(1, 0, 2);
	CopyFuncs["llvm.memmove.p0i8.p0i8.i32"] = make_tuple(1, 0, 2);
	CopyFuncs["llvm.memmove.p0i8.p0i8.i64"] = make_tuple(1, 0, 2);
}

// Setup functions that fetch data from the external.
// <name, <dst_arg#, source_arg#>>
static void SetDataFetchFuncs(

		map<string, pair<int8_t, int8_t>> &DataFetchFuncs) {

	DataFetchFuncs["copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["__copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["raw_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["strncpy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["_strncpy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["__strncpy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["__copy_from_user_inatomic"] = make_pair(0, 1);
	DataFetchFuncs["strndup_user"] = make_pair(-1, 0);
	DataFetchFuncs["memdup_user"] = make_pair(-1, 0);
	DataFetchFuncs["vmemdup_user"] = make_pair(-1, 0);
	DataFetchFuncs["memdup_user_nul"] = make_pair(-1, 0);
	DataFetchFuncs["get_user"] = make_pair(0, 1);
	DataFetchFuncs["__get_user"] = make_pair(0, 1);
	DataFetchFuncs["copyin"] = make_pair(1, 0);
	DataFetchFuncs["copyin_str"] = make_pair(1, 0);
	DataFetchFuncs["copyin_nofault"] = make_pair(1, 0);
	DataFetchFuncs["fubyte"] = make_pair(-1, 0);
	DataFetchFuncs["fusword"] = make_pair(-1, 0);
	DataFetchFuncs["fuswintr"] = make_pair(-1, 0);
	DataFetchFuncs["fuword"] = make_pair(-1, 0);

	// more variants
	DataFetchFuncs["rds_message_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["ivtv_buf_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["snd_trident_synth_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["copy_from_user_toio"] = make_pair(0, 1);
	DataFetchFuncs["iov_iter_copy_from_user_atomic"] = make_pair(0, 1);
	DataFetchFuncs["__generic_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["__constant_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["copy_from_user_page"] = make_pair(0, 1);
	DataFetchFuncs["__copy_from_user_eva"] = make_pair(0, 1);
	DataFetchFuncs["__arch_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["__copy_from_user_flushcache"] = make_pair(0, 1);
	DataFetchFuncs["arm_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["__asm_copy_from_user"] = make_pair(0, 1);
	DataFetchFuncs["__copy_from_user_inatomic_nocache"] = make_pair(0, 1);
	DataFetchFuncs["copy_from_user_nmi"] = make_pair(0, 1);
	DataFetchFuncs["copy_from_user_proc"] = make_pair(0, 1);
}


#endif
