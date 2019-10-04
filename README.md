# Crix: Detecting Missing-Check Bugs in OS Kernels

Missing a security check is a class of semantic bugs in software programs where erroneous execution states are not validated. Missing-check bugs are particularly common in OS kernels because they frequently interact with external untrusted user space and hardware, and carry out error-prone computation. Missing-check bugs may cause a variety of critical security consequences, including permission bypasses, out-of-bound accesses, and system crashes.

The tool, Crix, can quickly detect missing-check bugs in OS kernels. It evaluates whether any security checks are missing for critical variables, using an inter-procedural, semantic- and context-aware cross-checking. We have used Crix to find 278 new missing-check bugs in the Linux kernel. More details can be found in the paper shown at the bottom.

## How to use Crix

### Build LLVM 
```sh 
	$ cd llvm 
	$ ./build-llvm.sh 
	# The installed LLVM is of version 10.0.0 
```

### Build the Crix analyzer 
```sh 
	# Build the analysis pass of Crix 
	$ cd ../analyzer 
	$ make 
	# Now, you can find the executable, `kanalyzer`, in `build/lib/`
```
 
### Prepare LLVM bitcode files of OS kernels

* Replace error-code definition files of the Linux kernel with the ones in "encoded-errno"
* The code should be compiled with the built LLVM
* Compile the code with options: -O0 or -O2, -g, -fno-inline
* Generate bitcode files
	- We have our own tool to generate bitcode files: https://github.com/sslab-gatech/deadline/tree/release/work. Note that files (typically less than 10) with compilation errors are simply discarded
	- We also provided the pre-compiled bitcode files - https://github.com/umnsec/linux-bitcode

### Run the Crix analyzer
```sh
	# To analyze a single bitcode file, say "test.bc", run:
	$ ./build/lib/kanalyzer -sc test.bc
	# To analyze a list of bitcode files, put the absolute paths of the bitcode files in a file, say "bc.list", then run:
	$ ./build/lib/kalalyzer -mc @bc.list
```

## More details
* [The Crix paper (USENIX Security'19)](https://www-users.cs.umn.edu/~kjlu/papers/crix.pdf)
```sh
@inproceedings{crix-security19,
  title        = {{Detecting Missing-Check Bugs via Semantic- and Context-Aware Criticalness and Constraints Inferences}},
  author       = {Kangjie Lu and Aditya Pakki and Qiushi Wu},
  booktitle    = {Proceedings of the 28th USENIX Security Symposium (Security)},
  month        = August,
  year         = 2019,
  address      = {Santa Clara, CA},
}
```
