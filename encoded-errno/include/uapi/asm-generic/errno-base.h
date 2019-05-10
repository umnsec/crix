#ifndef _ASM_GENERIC_ERRNO_BASE_H
#define _ASM_GENERIC_ERRNO_BASE_H

/* Reserve 12 bits for real errno */
#define ERRNO_PREFIX 0x4cedb000
#define errno_encode(x) (ERRNO_PREFIX | (x))
#define errno_decode(x) ((~ERRNO_PREFIX) & (x))

#define	EPERM		 errno_encode(1)	/* Operation not permitted */
#define	ENOENT		 errno_encode(2)	/* No such file or directory */
#define	ESRCH		 errno_encode(3)	/* No such process */
#define	EINTR		 errno_encode(4)	/* Interrupted system call */
#define	EIO		 errno_encode(5)	/* I/O error */
#define	ENXIO		 errno_encode(6)	/* No such device or address */
#define	E2BIG		 errno_encode(7)	/* Argument list too long */
#define	ENOEXEC		 errno_encode(8)	/* Exec format error */
#define	EBADF		 errno_encode(9)	/* Bad file number */
#define	ECHILD		errno_encode(10)	/* No child processes */
#define	EAGAIN		errno_encode(11)	/* Try again */
#define	ENOMEM		errno_encode(12)	/* Out of memory */
#define	EACCES		errno_encode(13)	/* Permission denied */
#define	EFAULT		errno_encode(14)	/* Bad address */
#define	ENOTBLK		errno_encode(15)	/* Block device required */
#define	EBUSY		errno_encode(16)	/* Device or resource busy */
#define	EEXIST		errno_encode(17)	/* File exists */
#define	EXDEV		errno_encode(18)	/* Cross-device link */
#define	ENODEV		errno_encode(19)	/* No such device */
#define	ENOTDIR		errno_encode(20)	/* Not a directory */
#define	EISDIR		errno_encode(21)	/* Is a directory */
#define	EINVAL		errno_encode(22)	/* Invalid argument */
#define	ENFILE		errno_encode(23)	/* File table overflow */
#define	EMFILE		errno_encode(24)	/* Too many open files */
#define	ENOTTY		errno_encode(25)	/* Not a typewriter */
#define	ETXTBSY		errno_encode(26)	/* Text file busy */
#define	EFBIG		errno_encode(27)	/* File too large */
#define	ENOSPC		errno_encode(28)	/* No space left on device */
#define	ESPIPE		errno_encode(29)	/* Illegal seek */
#define	EROFS		errno_encode(30)	/* Read-only file system */
#define	EMLINK		errno_encode(31)	/* Too many links */
#define	EPIPE		errno_encode(32)	/* Broken pipe */
#define	EDOM		errno_encode(33)	/* Math argument out of domain of func */
#define	ERANGE		errno_encode(34)	/* Math result not representable */

#endif
