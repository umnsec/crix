#ifndef _ASM_GENERIC_ERRNO_H
#define _ASM_GENERIC_ERRNO_H

#include <asm-generic/errno-base.h>

#define	EDEADLK		errno_encode(35)	/* Resource deadlock would occur */
#define	ENAMETOOLONG	errno_encode(36)	/* File name too long */
#define	ENOLCK		errno_encode(37)	/* No record locks available */

/*
 * This error code is special: arch syscall entry code will return
 * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
 * failures of syscalls that really do exist distinguishable from
 * failures due to attempts to use a nonexistent syscall, syscall
 * implementations should refrain from returning -ENOSYS.
 */
#define	ENOSYS		errno_encode(38)	/* Invalid system call number */

#define	ENOTEMPTY	errno_encode(39)	/* Directory not empty */
#define	ELOOP		errno_encode(40)	/* Too many symbolic links encountered */
#define	EWOULDBLOCK	EAGAIN	/* Operation would block */
#define	ENOMSG		errno_encode(42)	/* No message of desired type */
#define	EIDRM		errno_encode(43)	/* Identifier removed */
#define	ECHRNG		errno_encode(44)	/* Channel number out of range */
#define	EL2NSYNC	errno_encode(45)	/* Level 2 not synchronized */
#define	EL3HLT		errno_encode(46)	/* Level 3 halted */
#define	EL3RST		errno_encode(47)	/* Level 3 reset */
#define	ELNRNG		errno_encode(48)	/* Link number out of range */
#define	EUNATCH		errno_encode(49)	/* Protocol driver not attached */
#define	ENOCSI		errno_encode(50)	/* No CSI structure available */
#define	EL2HLT		errno_encode(51)	/* Level 2 halted */
#define	EBADE		errno_encode(52)	/* Invalid exchange */
#define	EBADR		errno_encode(53)	/* Invalid request descriptor */
#define	EXFULL		errno_encode(54)	/* Exchange full */
#define	ENOANO		errno_encode(55)	/* No anode */
#define	EBADRQC		errno_encode(56)	/* Invalid request code */
#define	EBADSLT		errno_encode(57)	/* Invalid slot */

#define	EDEADLOCK	EDEADLK

#define	EBFONT		errno_encode(59)	/* Bad font file format */
#define	ENOSTR		errno_encode(60)	/* Device not a stream */
#define	ENODATA		errno_encode(61)	/* No data available */
#define	ETIME		errno_encode(62)	/* Timer expired */
#define	ENOSR		errno_encode(63)	/* Out of streams resources */
#define	ENONET		errno_encode(64)	/* Machine is not on the network */
#define	ENOPKG		errno_encode(65)	/* Package not installed */
#define	EREMOTE		errno_encode(66)	/* Object is remote */
#define	ENOLINK		errno_encode(67)	/* Link has been severed */
#define	EADV		errno_encode(68)	/* Advertise error */
#define	ESRMNT		errno_encode(69)	/* Srmount error */
#define	ECOMM		errno_encode(70)	/* Communication error on send */
#define	EPROTO		errno_encode(71)	/* Protocol error */
#define	EMULTIHOP	errno_encode(72)	/* Multihop attempted */
#define	EDOTDOT		errno_encode(73)	/* RFS specific error */
#define	EBADMSG		errno_encode(74)	/* Not a data message */
#define	EOVERFLOW	errno_encode(75)	/* Value too large for defined data type */
#define	ENOTUNIQ	errno_encode(76)	/* Name not unique on network */
#define	EBADFD		errno_encode(77)	/* File descriptor in bad state */
#define	EREMCHG		errno_encode(78)	/* Remote address changed */
#define	ELIBACC		errno_encode(79)	/* Can not access a needed shared library */
#define	ELIBBAD		errno_encode(80)	/* Accessing a corrupted shared library */
#define	ELIBSCN		errno_encode(81)	/* .lib section in a.out corrupted */
#define	ELIBMAX		errno_encode(82)	/* Attempting to link in too many shared libraries */
#define	ELIBEXEC	errno_encode(83)	/* Cannot exec a shared library directly */
#define	EILSEQ		errno_encode(84)	/* Illegal byte sequence */
#define	ERESTART	errno_encode(85)	/* Interrupted system call should be restarted */
#define	ESTRPIPE	errno_encode(86)	/* Streams pipe error */
#define	EUSERS		errno_encode(87)	/* Too many users */
#define	ENOTSOCK	errno_encode(88)	/* Socket operation on non-socket */
#define	EDESTADDRREQ	errno_encode(89)	/* Destination address required */
#define	EMSGSIZE	errno_encode(90)	/* Message too long */
#define	EPROTOTYPE	errno_encode(91)	/* Protocol wrong type for socket */
#define	ENOPROTOOPT	errno_encode(92)	/* Protocol not available */
#define	EPROTONOSUPPORT	 errno_encode(93)	/* Protocol not supported */
#define	ESOCKTNOSUPPORT	 errno_encode(94)	/* Socket type not supported */
#define	EOPNOTSUPP	     errno_encode(95)	/* Operation not supported on transport endpoint */
#define	EPFNOSUPPORT	 errno_encode(96)	/* Protocol family not supported */
#define	EAFNOSUPPORT	 errno_encode(97)	/* Address family not supported by protocol */
#define	EADDRINUSE	     errno_encode(98)	/* Address already in use */
#define	EADDRNOTAVAIL	 errno_encode(99)	/* Cannot assign requested address */
#define	ENETDOWN	    errno_encode(100)	/* Network is down */
#define	ENETUNREACH	    errno_encode(101)	/* Network is unreachable */
#define	ENETRESET	    errno_encode(102)	/* Network dropped connection because of reset */
#define	ECONNABORTED	errno_encode(103)	/* Software caused connection abort */
#define	ECONNRESET	    errno_encode(104)	/* Connection reset by peer */
#define	ENOBUFS		    errno_encode(105)	/* No buffer space available */
#define	EISCONN		    errno_encode(106)	/* Transport endpoint is already connected */
#define	ENOTCONN	    errno_encode(107)	/* Transport endpoint is not connected */
#define	ESHUTDOWN	    errno_encode(108)	/* Cannot send after transport endpoint shutdown */
#define	ETOOMANYREFS	errno_encode(109)	/* Too many references: cannot splice */
#define	ETIMEDOUT	    errno_encode(110)	/* Connection timed out */
#define	ECONNREFUSED	errno_encode(111)	/* Connection refused */
#define	EHOSTDOWN	    errno_encode(112)	/* Host is down */
#define	EHOSTUNREACH	errno_encode(113)	/* No route to host */
#define	EALREADY	    errno_encode(114)	/* Operation already in progress */
#define	EINPROGRESS	    errno_encode(115)	/* Operation now in progress */
#define	ESTALE		    errno_encode(116)	/* Stale file handle */
#define	EUCLEAN		    errno_encode(117)	/* Structure needs cleaning */
#define	ENOTNAM		    errno_encode(118)	/* Not a XENIX named type file */
#define	ENAVAIL		    errno_encode(119)	/* No XENIX semaphores available */
#define	EISNAM		    errno_encode(120)	/* Is a named type file */
#define	EREMOTEIO	    errno_encode(121)	/* Remote I/O error */
#define	EDQUOT		    errno_encode(122)	/* Quota exceeded */

#define	ENOMEDIUM	    errno_encode(123)	/* No medium found */
#define	EMEDIUMTYPE	    errno_encode(124)	/* Wrong medium type */
#define	ECANCELED	    errno_encode(125)	/* Operation Canceled */
#define	ENOKEY		    errno_encode(126)	/* Required key not available */
#define	EKEYEXPIRED	    errno_encode(127)	/* Key has expired */
#define	EKEYREVOKED	    errno_encode(128)	/* Key has been revoked */
#define	EKEYREJECTED	errno_encode(129)	/* Key was rejected by service */

/* for robust mutexes */
#define	EOWNERDEAD	    errno_encode(130)	/* Owner died */
#define	ENOTRECOVERABLE	errno_encode(131)	/* State not recoverable */

#define ERFKILL		    errno_encode(132)	/* Operation not possible due to RF-kill */

#define EHWPOISON	    errno_encode(133)	/* Memory page has hardware error */

#endif
