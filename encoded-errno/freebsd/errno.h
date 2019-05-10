/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)errno.h		8.5 (Berkeley) 1/21/94
 * $FreeBSD$
 */

#ifndef _SYS_ERRNO_H_
#define _SYS_ERRNO_H_

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <sys/cdefs.h>
__BEGIN_DECLS
int *	__error(void);
__END_DECLS
#define	errno		(* __error())
#endif

/* Reserve 12 bits for real errno */
#define ERRNO_PREFIX 0x4cedb000
#define errno_encode(x) (ERRNO_PREFIX | (x))
#define errno_decode(x) ((~ERRNO_PREFIX) & (x))

#define	EPERM		errno_encode(1)		 	 /* Operation not permitted */
#define	ENOENT		errno_encode(2)		 	 /* No such file or directory */
#define	ESRCH		errno_encode(3)		 	 /* No such process */
#define	EINTR		errno_encode(4)		 	 /* Interrupted system call */
#define	EIO		errno_encode(5)		 	 /* Input/output error */
#define	ENXIO		errno_encode(6)		 	 /* Device not configured */
#define	E2BIG		errno_encode(7)		 	 /* Argument list too long */
#define	ENOEXEC		errno_encode(8)		 	 /* Exec format error */
#define	EBADF		errno_encode(9)		 	 /* Bad file descriptor */
#define	ECHILD		errno_encode(10)		 /* No child processes */
#define	EDEADLK		errno_encode(11)		 /* Resource deadlock avoided */
							 /* 11 was EAGAIN */
#define	ENOMEM		errno_encode(12)		 /* Cannot allocate memory */
#define	EACCES		errno_encode(13)		 /* Permission denied */
#define	EFAULT		errno_encode(14)		 /* Bad address */
#ifndef _POSIX_SOURCE
#define	ENOTBLK		errno_encode(15)		 /* Block device required */
#endif
#define	EBUSY		errno_encode(16)		 /* Device busy */
#define	EEXIST		errno_encode(17)		 /* File exists */
#define	EXDEV		errno_encode(18)		 /* Cross-device link */
#define	ENODEV		errno_encode(19)		 /* Operation not supported by device */
#define	ENOTDIR		errno_encode(20)		 /* Not a directory */
#define	EISDIR		errno_encode(21)		 /* Is a directory */
#define	EINVAL		errno_encode(22)		 /* Invalid argument */
#define	ENFILE		errno_encode(23)		 /* Too many open files in system */
#define	EMFILE		errno_encode(24)		 /* Too many open files */
#define	ENOTTY		errno_encode(25)		 /* Inappropriate ioctl for device */
#ifndef _POSIX_SOURCE
#define	ETXTBSY		errno_encode(26)		 /* Text file busy */
#endif
#define	EFBIG		errno_encode(27)		 /* File too large */
#define	ENOSPC		errno_encode(28)		 /* No space left on device */
#define	ESPIPE		errno_encode(29)		 /* Illegal seek */
#define	EROFS		errno_encode(30)		 /* Read-only filesystem */
#define	EMLINK		errno_encode(31)		 /* Too many links */
#define	EPIPE		errno_encode(32)		 /* Broken pipe */

/* math software */
#define	EDOM		errno_encode(33)		 /* Numerical argument out of domain */
#define	ERANGE		errno_encode(34)		 /* Result too large */

/* non-blocking and interrupt i/o */
#define	EAGAIN		errno_encode(35)		 /* Resource temporarily unavailable */
#ifndef _POSIX_SOURCE
#define	EWOULDBLOCK	EAGAIN				 /* Operation would block */
#define	EINPROGRESS	errno_encode(36)		 /* Operation now in progress */
#define	EALREADY	errno_encode(37)		 /* Operation already in progress */

/* ipc/network software -- argument errors */
#define	ENOTSOCK	errno_encode(38)		 /* Socket operation on non-socket */
#define	EDESTADDRREQ	errno_encode(39)		 /* Destination address required */
#define	EMSGSIZE	errno_encode(40)		 /* Message too long */
#define	EPROTOTYPE	errno_encode(41)		 /* Protocol wrong type for socket */
#define	ENOPROTOOPT	errno_encode(42)		 /* Protocol not available */
#define	EPROTONOSUPPORT	errno_encode(43)		 /* Protocol not supported */
#define	ESOCKTNOSUPPORT	errno_encode(44)		 /* Socket type not supported */
#define	EOPNOTSUPP	errno_encode(45)		 /* Operation not supported */
#define	ENOTSUP		EOPNOTSUPP			 /* Operation not supported */
#define	EPFNOSUPPORT	errno_encode(46)		 /* Protocol family not supported */
#define	EAFNOSUPPORT	errno_encode(47)		 /* Address family not supported by protocol family */
#define	EADDRINUSE	errno_encode(48)		 /* Address already in use */
#define	EADDRNOTAVAIL	errno_encode(49)		 /* Can't assign requested address */

/* ipc/network software -- operational errors */
#define	ENETDOWN	errno_encode(50)		 /* Network is down */
#define	ENETUNREACH	errno_encode(51)		 /* Network is unreachable */
#define	ENETRESET	errno_encode(52)		 /* Network dropped connection on reset */
#define	ECONNABORTED	errno_encode(53)		 /* Software caused connection abort */
#define	ECONNRESET	errno_encode(54)		 /* Connection reset by peer */
#define	ENOBUFS		errno_encode(55)		 /* No buffer space available */
#define	EISCONN		errno_encode(56)		 /* Socket is already connected */
#define	ENOTCONN	errno_encode(57)		 /* Socket is not connected */
#define	ESHUTDOWN	errno_encode(58)		 /* Can't send after socket shutdown */
#define	ETOOMANYREFS	errno_encode(59)		 /* Too many references: can't splice */
#define	ETIMEDOUT	errno_encode(60)		 /* Operation timed out */
#define	ECONNREFUSED	errno_encode(61)		 /* Connection refused */

#define	ELOOP		errno_encode(62)		 /* Too many levels of symbolic links */
#endif /* _POSIX_SOURCE */
#define	ENAMETOOLONG	errno_encode(63)		 /* File name too long */

/* should be rearranged */
#ifndef _POSIX_SOURCE
#define	EHOSTDOWN	errno_encode(64)		 /* Host is down */
#define	EHOSTUNREACH	errno_encode(65)		 /* No route to host */
#endif /* _POSIX_SOURCE */
#define	ENOTEMPTY	errno_encode(66)		 /* Directory not empty */

/* quotas & mush */
#ifndef _POSIX_SOURCE
#define	EPROCLIM	errno_encode(67)		 /* Too many processes */
#define	EUSERS		errno_encode(68)		 /* Too many users */
#define	EDQUOT		errno_encode(69)		 /* Disc quota exceeded */

/* Network File System */
#define	ESTALE		errno_encode(70)		 /* Stale NFS file handle */
#define	EREMOTE		errno_encode(71)		 /* Too many levels of remote in path */
#define	EBADRPC		errno_encode(72)		 /* RPC struct is bad */
#define	ERPCMISMATCH	errno_encode(73)		 /* RPC version wrong */
#define	EPROGUNAVAIL	errno_encode(74)		 /* RPC prog. not avail */
#define	EPROGMISMATCH	errno_encode(75)		 /* Program version wrong */
#define	EPROCUNAVAIL	errno_encode(76)		 /* Bad procedure for program */
#endif /* _POSIX_SOURCE */

#define	ENOLCK		errno_encode(77)		 /* No locks available */
#define	ENOSYS		errno_encode(78)		 /* Function not implemented */

#ifndef _POSIX_SOURCE
#define	EFTYPE		errno_encode(79)		 /* Inappropriate file type or format */
#define	EAUTH		errno_encode(80)		 /* Authentication error */
#define	ENEEDAUTH	errno_encode(81)		 /* Need authenticator */
#define	EIDRM		errno_encode(82)		 /* Identifier removed */
#define	ENOMSG		errno_encode(83)		 /* No message of desired type */
#define	EOVERFLOW	errno_encode(84)		 /* Value too large to be stored in data type */
#define	ECANCELED	errno_encode(85)		 /* Operation canceled */
#define	EILSEQ		errno_encode(86)		 /* Illegal byte sequence */
#define	ENOATTR		errno_encode(87)		 /* Attribute not found */

#define	EDOOFUS		errno_encode(88)		 /* Programming error */
#endif /* _POSIX_SOURCE */

#define	EBADMSG		errno_encode(89)		 /* Bad message */
#define	EMULTIHOP	errno_encode(90)		 /* Multihop attempted */
#define	ENOLINK		errno_encode(91)		 /* Link has been severed */
#define	EPROTO		errno_encode(92)		 /* Protocol error */

#ifndef _POSIX_SOURCE
#define	ENOTCAPABLE	errno_encode(93)		 /* Capabilities insufficient */
#define	ECAPMODE	errno_encode(94)		 /* Not permitted in capability mode */
#define	ENOTRECOVERABLE	errno_encode(95)		 /* State not recoverable */
#define	EOWNERDEAD	errno_encode(96)		 /* Previous owner died */
#define EINTEGRITY	errno_encode(97)		 /* Integrity check failed */
#endif /* _POSIX_SOURCE */

#ifndef _POSIX_SOURCE
#define	ELAST		errno_encode(97)		 /* Must be equal largest errno */
#endif /* _POSIX_SOURCE */

#if defined(_KERNEL) || defined(_WANT_KERNEL_ERRNO)
/* pseudo-errors returned inside kernel to modify return to process */
#define	ERESTART	errno_encode(-1)		 /* restart syscall */
#define	EJUSTRETURN	errno_encode(-2)		 /* don't modify regs, just return */
#define	ENOIOCTL	errno_encode(-3)		 /* ioctl not handled by this layer */
#define	EDIRIOCTL	errno_encode(-4)		 /* do direct ioctl in GEOM */
#define	ERELOOKUP	errno_encode(-5)		 /* retry the directory lookup */
#endif

#ifndef _KERNEL
#if __EXT1_VISIBLE
/* ISO/IEC 9899:2011 K.3.2.2 */
#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
#endif
#endif /* __EXT1_VISIBLE */
#endif

#endif
