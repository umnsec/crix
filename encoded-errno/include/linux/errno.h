#ifndef _LINUX_ERRNO_H
#define _LINUX_ERRNO_H

#include <uapi/linux/errno.h>


/*
 * These should never be seen by user programs.  To return one of ERESTART*
 * codes, signal_pending() MUST be set.  Note that ptrace can observe these
 * at syscall exit tracing, but they will never be left for the debugged user
 * process to see.
 */
#define ERESTARTSYS	errno_encode(512)
#define ERESTARTNOINTR	errno_encode(513)
#define ERESTARTNOHAND	errno_encode(514)	/* restart if no handler.. */
#define ENOIOCTLCMD	errno_encode(515)	/* No ioctl command */
#define ERESTART_RESTARTBLOCK errno_encode(516) /* restart by calling sys_restart_syscall */
#define EPROBE_DEFER	errno_encode(517)	/* Driver requests probe retry */
#define EOPENSTALE	errno_encode(518)	/* open found a stale dentry */
#define ENOPARAM        errno_encode(519)     /* Parameter not supported */

/* Defined for the NFSv3 protocol */
#define EBADHANDLE	errno_encode(521)	/* Illegal NFS file handle */
#define ENOTSYNC	errno_encode(522)	/* Update synchronization mismatch */
#define EBADCOOKIE	errno_encode(523)	/* Cookie is stale */
#define ENOTSUPP	errno_encode(524)	/* Operation is not supported */
#define ETOOSMALL	errno_encode(525)	/* Buffer or request is too small */
#define ESERVERFAULT	errno_encode(526)	/* An untranslatable error occurred */
#define EBADTYPE	errno_encode(527)	/* Type not supported by server */
#define EJUKEBOX	errno_encode(528)	/* Request initiated, but will not complete before timeout */
#define EIOCBQUEUED	errno_encode(529)	/* iocb queued, will get completion event */
#define ERECALLCONFLICT	errno_encode(530)	/* conflict with recalled state */

#endif
