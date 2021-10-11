#ifndef lint
#pragma ident "@(#)spmicommon_api.h 1.27 96/10/11 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	spmicommon_api.h
 * Group:	libspmicommon
 * Description:	This module contains the libspmicommon API data structures,
 *		constants, and function prototypes.
 */

#ifndef _SPMICOMMON_API_H
#define	_SPMICOMMON_API_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>

/* constants */

/*
 * Exit status constants
 */
#define	EXIT_INSTALL_SUCCESS_REBOOT	0
#define	EXIT_INSTALL_SUCCESS_NOREBOOT	1
#define	EXIT_INSTALL_FAILURE		2

/*
 * Return status constants
 */
#define	NOERR           0
#define	ERROR           1

/* posting message types */

#define	STATMSG         0
#define	ERRMSG          1
#define	WARNMSG         2

/* format values */

#define	LOG             0x1     /* write message to log file */
#define	SCR             0x2     /* write message to the screen */
#define	LOGSCR          LOG|SCR /* write message to the log and screen */
#define	LEVEL0          0x0001  /* message level 0 */
#define	LEVEL1          0x0002  /* message level 1 */
#define	LEVEL2          0x0004  /* message level 2 */
#define	LEVEL3          0x0010  /* message level 3 */
#define	LISTITEM        0x0100  /* list item */
#define	CONTINUE        0x0200  /* continuation line */
#define	FMTPARTIAL	0x0400  /* no newline at end of line */

#define	IP_ADDR		35

/*
 *
 */
#define	SUCCESS		0
#define	FAILURE		1

/*
 * Privilege flags
 */
#define	NOPRIVILEGE	0
#define	PRIVILEGE	1

/*
 * Library standard definitions for true and false
 */
#if	!defined(TRUE) || ((TRUE) != 1)
#define	TRUE    (1)
#endif
#if	!defined(FALSE) || ((FALSE) != 0)
#define	FALSE   (0)
#endif

/* Error return codes */
#define	ERR_NOMEDIA		1
#define	ERR_NODIR		2
#define	ERR_INVALIDTYPE		3
#define	ERR_UMOUNTED		4
#define	ERR_NOPROD		5
#define	ERR_MOUNTED		6
#define	ERR_INVALID		7
#define	ERR_NOPRODUCT		8
#define	ERR_NOLOAD		9
#define	ERR_NOCLSTR		10
#define	ERR_LOADFAIL		11
#define	ERR_UNDEF		12
#define	ERR_NOMATCH		13
#define	ERR_NOFILE		14
#define	ERR_BADENTRY		15
#define	ERR_NOPKG		16
#define	ERR_BADPKG		17
#define	ERR_UNMOUNT		18
#define	ERR_NODEVICE		19
#define	ERR_PREVLOAD		20
#define	ERR_BADARCH		21
#define	ERR_INVSERVER		22
#define	ERR_NOMOUNT		23
#define	ERR_FSTYPE		24
#define	ERR_SHARE		25
#define	ERR_LOCKFILE		26
#define	ERR_VOLUME		27
#define	ERR_MOUNTPT		28
#define	ERR_SAVE		29
#define	ERR_PIPECREATE		30
#define	ERR_ULIMIT		31
#define	ERR_FORKFAIL		32
#define	ERR_MNTTAB		33
#define	ERR_HOSTDOWN		34
#define	ERR_NOPORT		35
#define	ERR_NOSTREAM		36
#define	ERR_NOPASSWD		37
#define	ERR_INVPASSWD		38
#define	ERR_BADCOMM		39
#define	ERR_HOSTINFO		40
#define	ERR_NOACCESS		41
#define	ERR_DIFFREV		42
#define	ERR_INVARCH		43
#define	ERR_BADLOCALE		44
#define	ERR_NULLPKG		45
#define	ERR_OPENING_VFSTAB 	46
#define	ERR_ADD_SWAP		47
#define	ERR_MOUNT_FAIL		48
#define	ERR_MUST_MANUAL_FSCK	49
#define	ERR_FSCK_FAILURE	50
#define	ERR_OPEN_VFSTAB		51
#define	ERR_DELETE_SWAP		52
#define	ERR_UMOUNT_FAIL		53
#define	ERR_SVC_ALREADY_EXISTS	54
#define	ERR_NONNATIVE_MEDIA	55
#define	ERR_NOTHING_TO_UPGRADE	56
#define ERR_LOAD_INSTALLED	57

/* FATAL ERROR CODES */
#define	ERR_MALLOC_FAIL	 	-50
#define	ERR_IBE			-51
#define	ERR_STR_TOO_LONG	-101

/* LENGTH CONSTANTS */
#define	ARCH_LENGTH	MAXNAMELEN
#define	PLATFORM_LENGTH	MAXNAMELEN

/* for use with debugging calls to write_debug */
#define	DEBUG_LOC       __FILE__, __LINE__
#define	DEBUG_LOC_NOHD  NULL, __FILE__, __LINE__

/*
 * Define the type for the generic callback used throughout the
 * code base for passing progress information from the software
 * library to the calling application.  The first argument is
 * always a pointer provided by the calling application to
 * application specific data.  The second pointer is a pointer
 * to the information that is specific to the registered callback.
 * (e.g. In the case of the DSR code the second pointer is a
 * pointer to a structure that contains a state variable and
 * a union.  Based on the value of the state, the application can
 * read the union to retrieve the state specific information)
 */

typedef int TCallback(void *, void *);

/*
 * data structures and enumerated types
 */
typedef enum {
	SIM_ANY	    = 0,	/* "on" if any simulation flag is "on" */
	SIM_SYSDISK = 1,	/* system disk/prom configuration */
	SIM_EXECUTE = 2,	/* system updating */
	SIM_SYSSOFT = 3,	/* system software configuraiton */
	SIM_MEDIA   = 4		/* media configuration */
} SimType;

typedef enum machine_type {
	MT_UNDEFINED = -1,
	MT_STANDALONE = 0,
	MT_SERVER = 1,
	MT_DATALESS = 2,
	MT_DISKLESS = 3,
	MT_SERVICE = 4,
	MT_CCLIENT = 5
} MachineType;

typedef enum test_mount {
	NOT_TESTED = 0,
	TEST_FAILURE = 1,
	TEST_SUCCESS = 2
} TestMount;

typedef struct remote_fs {
	TestMount	 c_test_mounted;
	char		*c_mnt_pt;
	char		*c_hostname;
	char		*c_ip_addr;
	char		*c_export_path;
	char		*c_mount_opts;
	struct remote_fs *c_next;
} Remote_FS;

/*
 *  The following data structure is a linked list of strings.  It is
 *  used in numerous places.
 */
typedef struct string_list {
	struct string_list	*next;
	char			*string_ptr;
} StringList;

typedef struct item {
	struct item *next;
} Item;

typedef struct {
	size_t	m_size;		/* size of file in bytes */
	caddr_t	m_base;		/* base [mapped] address */
	caddr_t	m_ptr;		/* currently addressed offset in file */
} MFILE;

/*
 * The following section is the Type definitions for the common_linklist.c
 * package.
 */

typedef unsigned char  	   TUOneByte;
typedef          char  	   TOneByte;
typedef unsigned short 	   TUTwoBytes;
typedef          short 	   TTwoBytes;
typedef unsigned long  	   TUFourBytes;
typedef          long  	   TFourBytes;
typedef          long long TEightBytes;
typedef unsigned long long TUEightBytes;

typedef TOneByte      TBoolean;
#ifndef True
#define	True          1
#endif
#ifndef False
#define	False         0
#endif

/*
 * Define the error codes returned by the link list package
 */

typedef enum
{
  LLSuccess = 0,
  LLMemoryAllocationError,
  LLInvalidList,
  LLInvalidLink,
  LLInvalidOperation,
  LLLinkNotInUse,
  LLLinkInUse,
  LLListInUse,
  LLBeginningOfList,
  LLEndOfList,
  LLListEmpty,
  LLCallbackError,
  LLMemoryLeak
} TLLError;

/*
 * Define the operations that can be applied to a link list
 */

typedef enum
{
  LLPrev,
  LLCurrent,
  LLNext,
  LLHead,
  LLTail
} TLLOperation;

/*
 * Define the return types for the comparison callback used by the
 * LLSortList function.
 */

typedef enum
{
	LLCompareError,
	LLCompareLess,
	LLCompareEqual,
	LLCompareGreater
} TLLCompare;

/*
 * Define the opaque pointers to the three data types manipulated by
 * the link list package.
 */

typedef void *TLLData;
typedef void *TLink;
typedef void *TList;

/*
 * Define the MACRO for walking a list
 */

#define	LL_WALK(list, current, data, err) \
	for (\
		(err) = LLGetLinkData( \
			(TList)(list), \
			LLHead, \
			(TLink *)&(current), \
			(TLLData *)&(data));\
		(err) != LLEndOfList; \
		(err) = LLGetLinkData( \
			(TList)(list), \
			LLNext, \
			(TLink *)&(current), \
			(TLLData *)&(data)))

/*
 * The following section is the Type definitions for the
 * common_process_control.c package.
 */

typedef void *TPCHandle;

typedef enum
{
  PCSuccess = 0,
  PCInvalidHandle,
  PCProcessNotRunning,
  PCProcessRunning,
  PCSystemCallFailed,
  PCMemoryAllocationFailure,
  PCFailure
} TPCError;

typedef enum
{
  PCNotInitialized,
  PCInitialized,
  PCRunning,
  PCExited
} TPCState;

typedef struct
{
  int  StdIn;
  int  StdOut;
  int  StdErr;
  int  PTYMaster;
} TPCFD;

typedef struct
{
  FILE *StdIn;
  FILE *StdOut;
  FILE *StdErr;
  FILE *PTYMaster;
} TPCFILE;

/*
 * Regular expression routine matching
 */
typedef enum {
	REMatch,
	RENoMatch,
	RECompFailure,
	REBadArg
} REError;

/*
 * macros
 */

#define	WALK_LIST(x, y)			for ((x) = (y); \
						(x) != NULL; \
						(x) = (x)->next)
#define	WALK_LIST_INDIRECT(x, y)        for ((x) = &(y); \
						*(x) != NULL; \
                                                (x) = &((*x)->next))
#define	     is_pathname(x)		((x) != NULL && *(x) == '/')

/*
 * unit converstion, rounding must always be 'up' or errors will
 * occur when trying to fit software in slices which appear to
 * be large enough
 */

/* unit rounding up to nearest unit without cylinder rounding */

#define	sectors_to_mb(s)	(((s) + 2047) / 2048)
#define	sectors_to_kb(s)	(((s) + 1) / 2)
#define	bytes_to_sectors(b) 	(((b) + 511) / 512)
#define	kb_to_mb(k)		(((k) + 1023) / 1024)
#define	bytes_to_mb(b)          (sectors_to_mb(bytes_to_sectors(b)))


/* unit truncating to nearest unit without cylinder rounding */

#define	sectors_to_mb_trunc(s)    ((s) / 2048)
#define	sectors_to_kb_trunc(s)    ((s) / 2)
#define	bytes_to_sectors_trunc(b) ((b) / 512)
#define	kb_to_mb_trunc(k)	  ((k) / 1024)

/*
 * conversion macros without cylinder rounding which are not impacted by
 * unit mismatching
 */
#define	sectors_to_bytes(s)  	((s) * 512)
#define	kb_to_sectors(k)	((k) * 2)
#define	mb_to_kb(m)		((m) * 1024)
#define	mb_to_sectors(m)	((m) * 2048)


/* numeric comparisons */
#if !defined(MIN)
#define	MIN(a, b)	((int) (a) > (int) (b) ? (b) : (a))
#endif

#if !defined(MAX)
#define	MAX(a, b)	((int) (a) > (int) (b) ? (a) : (b))
#endif

/* string comparitor abbreviators */

#define	ci_streq(a, b)		(strcasecmp((a), (b)) == 0)
#define	ci_strneq(a, b, c)	(strncasecmp((a), (b), (c)) == 0)
#define	streq(a, b)		(strcmp((a), (b)) == 0)
#define	strneq(a, b, c)		(strncmp((a), (b), (c)) == 0)

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* common_boolean.c */
int		IsIsa(char *);
int		is_allnums(char *);
int		is_disk_name(char *);
int		is_hex_numeric(char *);
int		is_hostname(char *);
int		is_ipaddr(char *);
int		is_numeric(char *);
int		_is_openprom(int);
int		is_slice_name(char *);
int		is_part_name(char *);

/* common_post.c */
void 		(*register_func(u_int, void (*)(u_int, char *)))();
void 		write_message(u_char, u_int, u_int, char *, ...);
void		write_notice(u_int, char *, ...);
void 		write_status(u_char, u_int, char *, ...);
void		write_message_nofmt(
			u_char dest, u_int type, u_int format, char *string);
void		write_notice_nofmt(u_int type, char *string);
void		write_status_nofmt(u_char dest, u_int format, char *string);
int 		write_status_register_log(char *);
int 		write_error_register_log(char *);
int 		write_warning_register_log(char *);
void		write_debug(
			u_char, int, char *, char *, int, u_int, char *, ...);
void		write_debug_test(void);

/* common_util.c */
int		axtoi(char *);
int		_copy_file(char *, char *);
int		_create_dir(char *);
void		_filesys_fiodio(char *, int);
int		get_trace_level(void);
int		_lock_prog(char *);
char *		make_block_device(char *, int);
char *		make_char_device(char *, int);
char *		make_slice_name(char *, int);
char *		make_device_name(char *, int);
int		_map_to_effective_dev(char *, char *);
int		ParseBuffer(char *, char ***);
int		reset_system_state(void);
int		slice_access(char *, int);
int		set_trace_level(int);
int 		simplify_disk_name(char *, char *);
int		_system_fs_ancestor(char *);
int		GetSimulation(SimType);
int		SetSimulation(SimType, int);
void		CatFile(char *, u_char, u_int, u_int);


int
CMNWiteBuffer (int        FileDes,
	       const void *Buffer,
	       size_t     BytesToWrite);
int
CMNModifyFileDesFlag(int Set,
		     int FileDes,
		     int FlagsToSet);
int
CMNPTYMasterOpen(char *PTSName);

int
CMNPTYSlaveOpen(int FDMaster, char *PTSName);

pid_t
CMNPTYFork(int *FDMaster,
	   char *PTSName,
	   const struct termios *SlaveTermios,
	   const struct winsize *SlaveWinSize);

/* common_client.c */
TestMount	get_rfs_test_status(Remote_FS *);
char		*name2ipaddr(char *);
int		set_rfs_test_status(Remote_FS *, TestMount);
int		test_mount(Remote_FS *, int);

/* common_misc.c */
char		*get_err_str(int);
MachineType	get_machinetype(void);
char		*get_rootdir(void);
void		link_to(Item **, Item *);
int		path_is_readable(char *);
void     	set_machinetype(MachineType);
void		set_memalloc_failure_func(void(*)(int));
void     	set_rootdir(char *);
void		*xcalloc(size_t);
void		*xmalloc(size_t);
void		*xrealloc(void *, size_t);
char		*xstrdup(char *);
void		strip_whitespace(char *str);
REError		re_match(char *search_str, char *pattern, int shell_re_flag);


/* common_mmap.c */
char 		*mgets(char *, int, MFILE *);
void 		mclose(MFILE *);
MFILE		*mopen(char *);

/* common_mount.c */
int      	FsMount(char *, char *, char *, char *);
int		UfsRestoreName(char *, char *);
int		UfsMount(char *, char *, char *);
int		UfsUmount(char *, char *, char *);
int		DirUmount(char *);
int		DirUmountAll(char *);

/* common_canon_path.c */
void    	canoninplace(char *);

/* common_proc.c */
int     	ProcKill(int, char *);
int     	ProcIsRunning(int, char *);
int     	ProcWalk(int (*)(int, char *), char *);

/* common_strlist.c */
void		StringListFree(StringList *);
StringList *	StringListFind(StringList *, char *);
int		StringListCount(StringList *);
int		StringListAdd(StringList **, char *);
StringList *	StringListBuild(char *, char);
StringList *	StringListDup(StringList *);

/* common_arch.c */
char		*get_actual_platform(void);
char		*get_default_inst(void);
char		*get_default_machine(void);
char		*get_default_platform(void);

/*
 * common_linklist.c
 */

TLLError
LLCreateList(TList *List,
	     TLLData Data);

TLLError
LLCreateLink(TLink *Link,
	     TLLData Data);

TLLError
LLAddLink(TList List,
	  TLink Link,
	  TLLOperation Operation);

TLLError
LLRemoveLink(TList List,
	     TLink Link);

TLLError
LLDestroyLink(TLink   *Link,
	      TLLData *Data);

TLLError
LLDestroyList(TList   *List,
	      TLLData *Data);

TLLError
LLUpdateCurrent(TList List,
		TLLOperation Operation);

TLLError
LLGetLinkData(TList List,
	      TLLOperation Operation,
	      TLink *Link,
	      TLLData *Data);

TLLError
LLGetCurrentLinkData(TList List,
		     TLink *Link,
		     TLLData *Data);

TLLError
LLGetSuppliedLinkData(TLink Link,
		      TLLData *Data);

TLLError
LLGetSuppliedListData(TList    List,
		      int      *NumberLinks,
		      TLLData  *Data);

TLLError
LLSortList(TList ListToSort,
	   TLLCompare (*Compare)(void *,TLLData,TLLData),
	   void *UserPtr);

TLLError
LLClearList (TList List,
	     TLLError (*CleanUp)(TLLData Data));

char *
LLErrorString(TLLError Error);

/*
 * common_process_control.c
 */

TPCError
PCCreate(TPCHandle *Handle,
	 char      *Image,
	 char      *argv0,
	 ...);

TPCError
PCStart(TPCHandle Handle);

TPCError
PCWait(TPCHandle Handle,
       int       *ExitStatus,
       int       *ExitSignal);

TPCError
PCGetPID(TPCHandle Handle,
	 pid_t     *PID);

TPCError
PCGetFD(TPCHandle Handle,
	TPCFD     *FD);

TPCError
PCGetFILE(TPCHandle      Handle,
	  TPCFILE *FILE);

TPCError
PCKill(TPCHandle Handle,int Signal);

TPCError
PCDestroy(TPCHandle *Handle);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMICOMMON_API_H */
