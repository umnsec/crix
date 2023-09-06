# LLVM No-inline Patch
> We recommanded to use 0001-disable-inline_llvm_15.patch, which is simple and easy to understand.
## Introduction

Here is a patch which can be used to disable clang's inline machinism. So you can see inline function in your .ll file.

This is a git patch for llvm-15 (git tag: llvmorg-15-init a2601c98873376bbbeff4b6eddf0f4d920535f8b) but can be easily extended to be applied into other versions. The Patch itself is extremely simple and easy to understand.

In practice, llvm set function attributes in `SetLLVMFunctionAttributesForDefinition`. It starts with an AttrBuilder `B` and then apply all the attributes in `B` to function `F`. So what I did is remove inline attribute and attach a `noinline` attribute in `B` before its very last use to build up `F`. Extending this patch to different version could be applied in the same way.

After applying this patch, usually, you need to change the source code of target project according to compile log. As I know, some of inline functions (especially those using assembly code) will raise compile-time errors when they are compiled as no-inline. I've given Linux5.18 as an example. (0001-disable_inline-linux5.18.patch)

## Usage

```shell
cd YOUR_LLVM_PATH
git checkout llvmorg-15-init
// Copy 0001-disable-inline_llvm_15.patch patch to your LLVM project path
git apply 0001-disable-inline_llvm_15.patch
// build llvm
```

## About llvm inline machinism

Basically, llvm has 4 function attributes to control the inline behavior of clang: noinline, nothing, inlinehint, alwaysinline. For example:

```c
#include <stdio.h>
__attribute_noinline__ int noinline_func(){} // a noinline function, which will never be inline-compiled, labeled as "noinline"

int func(){} // a "nothing" function, which has no inline-attribute or "noinline" as one of attributes.

inline int inline_func() {} // a inlinehint function with "inlinehint" attribute.

#include <stdio.h>
__always_inline always_inline_func() {} // a alwaysinline function with "alwaysinline"
```

Usually, a function won't be compiled to llvm-IR as inline without inline declearation, even in -O2 or -O3 optimization. An inlinehint function will not be inline-compiled when either -O0 or -fno-inline(which is deprecated before llvm-15, but you can see options like OPT_fno_inline in its source code) is set. An alwaysinline function will be inline-compiled in any situation. (Well, except using this patch :). )


## About Linux5.18 inline

I've used ```ctags``` to statistic the number of function declearations in Linux, which gives an approximate overview:

There are about 22572 inline function declearation (537 always inline), out of 576k declearations as total in Linux 5.18. 

## Note
As far as I know, this patch is **NOT** able to disable the inline behavior about C++ member function (which will be inlined as default), so only use it in pure-C program.