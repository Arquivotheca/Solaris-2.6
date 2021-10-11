/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.
 */

#ifndef	_adm_auth_impl_h
#define	_adm_auth_impl_h

#pragma ident	"@(#)adm_auth_impl.h	1.22	93/05/18 SMI"

/*
 * FILE:  adm_auth.h
 *
 *	Admin Framework class agent header file for security definitions.
 */

#include <sys/types.h>
#include <sys/param.h>
#include "adm_auth.h"

/*
 * Admin Access Control List structure definition
 *
 *	Structure is allocated to be size
 *		ADM_ACL_HDRSIZE + (number_entries * ADM_ACL_ENTRYSIZE)
 *	with entryp set to be
 *		entryp = &entryp + sizeof (struct Adm_acl_entry *)
 *
 *	The entryp pointer is set to NULL when the ACL is written to
 *	storage and reset to first entry address when the ACL is read.
 *
 *	The users_offset and groups_offset are set to zero if there are
 *	no specific user or group entries, respectively.
 *
 *	An ACL structure always has at least 5 entries unless the
 *	ADM_ACL_OFF flag is set, in which case there may be no entries and
 *	no ACL checking will be performed.
 */

/* Admin security information structure general definitions */
#define	ADM_AUTH_DENIED		1
#define	ADM_AUTH_WRONG_FLAVOR	2
#define	ADM_AUTH_RESET		1

/* Admin security information structure general definitions */
#define	ADM_AUTH_VERSION	1	/* Version of security info */
#define	ADM_AUTH_MAXSIZE	sizeof (struct Adm_auth)
#define	ADM_AUTH_MAXFLAVORS	8	/* Maximum flavors in list */

/* Admin Access Control List general definitions */
#define	ADM_ACL_MAXENTRIES	126	/* Maximum number ACL entries */
#define	ADM_ACL_ENTRYSIZE	sizeof (struct Adm_acl_entry)
#define	ADM_ACL_MAXSIZE		sizeof (struct Adm_acl)
#define	ADM_ACL_MAXENTRYSIZE	(ADM_ACL_MAXENTRIES * ADM_ACL_ENTRYSIZE)
#define	ADM_ACL_OWNER_OFFSET	0	/* Array pos of owner user entry */
#define	ADM_ACL_GROUP_OFFSET	1	/* Array pos of owner group entry */
#define	ADM_ACL_OTHER_OFFSET	2	/* Array pos of other entry */
#define	ADM_ACL_NOBODY_OFFSET	3	/* Array pos of nobody entry */
#define	ADM_ACL_MASK_OFFSET	4	/* Array pos of mask entry */
#define	ADM_ACL_USERS_OFFSET	5	/* Array pos of first user entry */

#define	ADM_AUTH_MAXHDR		80	/* Length of longest header */

/* Masks and test values used in cpn signature: hashed value of cpn */
#define	ADM_CPN_NAME_MASK	0x00FF0000
#define	ADM_CPN_ROLE_MASK	0x0000FF00
#define	ADM_CPN_HOST_MASK	0x0000FF00	/* Role and Host same */
#define	ADM_CPN_DOMAIN_MASK	0x000000FF
#define	ADM_CPN_WORLD_MASK	0x00FFFFFF
#define	ADM_CPN_LOCAL_USER	(ulong)0	/* Local uid/gid identity */
 
/* Admin security information ACL entry common principal name */
typedef	struct	Adm_auth_cpn {
	ulong	signature;		/* Hashed values */
	uid_t	ugid;			/* Local uid or gid, if any */
	ushort	name_len;		/* Length of names */
	ushort	role_off;		/* Offset to role or host name */
	ushort	domain_off;		/* Offset to domain name */
	ushort	context;		/* Context: user, group, other */
	char	name[ADM_AUTH_CPN_NAMESIZE]; /* Name buffer */
} Adm_auth_cpn;
 
/*
 * Admin security information cpn contexts 
 *
 *	!!!WARNING!!!	These definitions must match the cpn "type"
 *			definitions in adm_sec.h
 */

#define	ADM_CPN_NONE	0	/* Cpn has no context (empty) */
#define	ADM_CPN_USER	1	/* Cpn is in user context */
#define	ADM_CPN_GROUP	2	/* Cpn is in group context */
#define	ADM_CPN_OTHER	3	/* Cpn is in other context */
        
/* Admin security information runtime client identity structure */
typedef	struct	auth_client_id {
	uid_t	local_uid;		/* Local uid */
	gid_t	local_gid;		/* Local gid */
	ushort	num_groups;
	gid_t	groups[NGROUPS_UMAX];
	Adm_auth_cpn net_cpn;		/* Netid based cpn */
} auth_client_id;

/* Admin security cpn identity special values */
#define	ADM_CPN_ROOT_ID		0	/* Root identity uid */
#define	ADM_CPN_ROOT_NAME	"root"	/* Root identity name */
#define	ADM_CPN_NOBODY_NAME	"nobody" /* Nobody identity name */
 
/* Admin Access Control List Entry version definition */
typedef	struct	Adm_acl_version {
	long	cookie;			/* Magic number */
	long	version;		/* Version number */
	u_int	str_length;		/* Length of comment */
	char	comment[ADM_AUTH_MAXHDR];	/* For future use */
} Adm_acl_version;

/* Admin Access Control List Entry structure definition */
typedef	struct	Adm_acl_entry {
	uid_t	id;			/* Entry identifier: uid or gid */
	ushort	type;			/* Type of ACL entry identifier */
	ushort	permissions;		/* Unix permissions */
} Adm_acl_entry;

/* Admin Access Control List structure definition */
typedef	struct	Adm_acl {
	ushort	flags;			/* ACL flags */
	ushort	groups_offset;		/* Array pos of groups entries */
	u_int	number_entries;		/* Number entries in ACL */
	u_int	filler_1;
	u_int	filler_2;
	struct	Adm_acl_entry entry[ADM_ACL_MAXENTRIES]; /* ACL entries */
} Adm_acl;

/* Admin security information structure definition */
typedef	struct  Adm_auth {
	u_int  version;			/* Version of security info */
	u_int  auth_type;		/* Authentication type */
	u_int  number_flavors;		/* Number auth flavors in list */
	u_int  auth_flavor[ADM_AUTH_MAXFLAVORS]; /* List of auth flavors */
	u_int  set_flag;		/* Set identity flag */
	uid_t  set_uid;			/* Set identity user uid */
	gid_t  set_gid;			/* Set identity group gid */
	u_int  filler_1;		/* Filler space */
	u_int  filler_2;		/* Filler space */
	struct Adm_acl acl;		/* ACL structure */
} Adm_auth;

/* Admin security information entry defn (pointed to by Adm_auth_handle_t */
typedef	struct	Adm_auth_entry {
	u_int  errcode;			/* Error status code */
	char  *errmsgp;			/* Pointer to error message */
	u_int  name_length;		/* Length of method name */
	u_int  auth_length;		/* Length of Adm_auth info */
	struct Adm_auth auth_info;	/* Adm_auth info */
	char   name[ADM_AUTH_NAMESIZE];	/* Entry name */
} Adm_auth_entry;

/* Admin authentication information flag definitions */
#define	ADM_AUTH_ANYACL		0	/* Ordinary ACL for getnext, putnext */
#define	ADM_AUTH_FIRSTACL	1	/* First ACL for getnext or putnext */
#define	ADM_AUTH_LASTACL	2	/* Last ACL for getnext or putnext */

#define	ADM_AUTH_TRUST_ALL	0	/* UNIX auth: trust all clients */
#define	ADM_AUTH_TRUST_MYDOM	1	/* UNIX auth: trust local domain */
#define	ADM_AUTH_TRUST_MYSYS	2	/* UNIX auth: trust local system */
 
/* Admin security information structure authentitication types */
#define	ADM_AUTH_UNSPECIFIED	255	/* Unspecified authentication */

/* Admin security information structure set identity flag values */
#define	ADM_SID_CLIENT_UID	1	/* Use client user identity */
#define	ADM_SID_CLIENT_GID	2	/* Use client group identity */
#define	ADM_SID_AGENT_UID	4	/* Use class agent user identity */
#define	ADM_SID_AGENT_GID	8	/* Use class agent group identity */

/* Admin Access Control List flag definitions */
#define	ADM_ACL_OFF 	1		/* ACL's not in effect */

/* Admin Access Control List Entry type definitions */
#define	ADM_ACL_NONE	(u_int)0	/* Null entry */
#define	ADM_ACL_OWNER	(u_int)1	/* Owning user entry */
#define	ADM_ACL_GROUP	(u_int)2	/* Owning group entry */
#define	ADM_ACL_OTHER	(u_int)3	/* Authenticated users entry */
#define	ADM_ACL_NOBODY	(u_int)4	/* Unauthenticated users entry */
#define	ADM_ACL_MASK	(u_int)5	/* Highest non-owner permissions */
#define	ADM_ACL_USERS	(u_int)6	/* Specific user entry */
#define	ADM_ACL_GROUPS	(u_int)7	/* Specific group entry */
#define	ADM_ACL_DEAD	(u_int)10	/* Type >= 10 is dead entry */

/* Admin Acess Control List Entry permissions definitions */
#define	ADM_ACL_READ	0x0004		/* Read permission */
#define	ADM_ACL_WRITE	0x0002		/* Write permission */
#define	ADM_ACL_EXECUTE	0x0001		/* Execute permission */
#define	ADM_ACL_RWX	0x0007		/* RWX permission */

/*
 * Admin Access Control List Entry type names for char format ACLs
 *
 * !!!WARNING!!!  The first character of each name must be unique!
 */

#define	ADM_ACL_OWNER_NAME  "USER"	/* Owning user entry */
#define	ADM_ACL_GROUP_NAME  "GROUP"	/* Owning group entry */
#define	ADM_ACL_OTHER_NAME  "other"	/* Authenticated users entry */
#define	ADM_ACL_NOBODY_NAME "nobody"	/* Unauthenticated users entry */
#define	ADM_ACL_MASK_NAME   "mask"	/* Highest non-owner permissions */
#define	ADM_ACL_USERS_NAME  "user"	/* Specific user entry */
#define	ADM_ACL_GROUPS_NAME "group"	/* Specific group entry */

/* Admin Access Control List Entry separator chars for char format ACLs */
#define	ADM_ACL_ENTRY_SEP	' '	/* Separator char between entries */
#define	ADM_ACL_TYPE_SEP	':'	/* ACL entry type:id separator */
#define	ADM_ACL_PERM_SEP	':'	/* ACL entry id:perms separator */

/* Admin security library read/write function special method arg values */
#define	ADM_AUTH_CLASS_INFO	".class"	/* Class info itself */
#define	ADM_AUTH_DEFAULT_INFO	".default"	/* Class default info */

/* Admin security library function options definitions */
#define	ADM_AUTH_OPT_SETACL	1	/* Set or verify full ACL struct */
#define	ADM_AUTH_OPT_DELACL	2	/* Verify ACL struct for delete */

/* Admin security library runtime function prototype definitions */

#ifdef __cplusplus
extern "C" {
#endif

extern int  adm_auth_read(char *class_name, char *class_version,
	    char *method_name, Adm_auth_entry *auth_entry_p);
extern int  adm_auth_getnext(char *class_name, char *class_version, u_int opt,
	    Adm_auth_entry *auth_entry_p);
extern int  adm_auth_putnext(char *class_name, char *class_version, u_int opt,
	    Adm_auth_entry *entry_p);
extern int  adm_auth_write(char *class_name, char *class_version,
	    char *method_name, Adm_auth_entry *auth_entry_p);
extern int  adm_auth_delete(char *class_name, char *class_version,
	    char *method_name, Adm_auth_entry *auth_entry_p);
extern int  adm_auth_chkacl(Adm_auth *auth_info_p, u_short mode,
	    uid_t user_uid, gid_t group_gid, int number_groups,
	    gid_t group_list[]);
extern int  adm_auth_chkauth(Adm_auth *auth_info_p, u_int sys_auth_type,
	    u_int sys_auth_flavor, u_int user_auth_flavor, u_int *auth_type,
	    u_int *number_flavors, u_int auth_flavor[]);
extern int  adm_auth_chksid(Adm_auth *auth_info_p, uid_t *user_uid,
	    gid_t *group_gid);
extern int  adm_auth_chkother(Adm_auth *auth_info_p, u_short mode);
extern int  adm_auth_chknobody(Adm_auth *auth_info_p, u_short mode);
extern void adm_auth_impl_set_debug_level(int level);

/* Admin security library maintenance function prototype definitions */
extern int  adm_auth_valacl(Adm_acl *acl_info_p, u_int option,
	    char **error_message_pp);
extern int  adm_auth_valauth(u_int auth_type, u_int auth_flavor,
	    char **error_message_pp);
extern int  adm_auth_valsid(u_int setid_flag, uid_t user_uid, gid_t group_gid,
	    char **error_message_pp);
extern int  adm_auth_sortacl(Adm_acl *acl_info_p);
extern int  adm_auth_acl2str(u_int option, Adm_acl *acl_info_p,
	    char *acl_buffer, u_int *acl_length_p, char **error_message_pp);
extern int  adm_auth_str2acl(u_int option, Adm_acl *acl_info_p,
	    char *acl_buffer, u_int acl_length, char **error_message_pp);
extern int  adm_auth_ace2str(u_int option, Adm_acl_entry *acl_info_p,
	    char *acl_entry_buffer, u_int *acl_entry_size_p);
extern int  adm_auth_str2ace(Adm_acl_entry *acl_info_p, char *acl_entry_buffer);
extern int  adm_auth_loc2cpn(uid_t uid, Adm_auth_cpn *cpn_p);
extern int  adm_auth_net2cpn(u_int flavor, char *netname, Adm_auth_cpn *cpn_p);
extern int  adm_auth_cpn2str(u_int option, Adm_auth_cpn *cpn_p, char *buff,
	    u_int bufflen);
extern void adm_auth_clear_cpn(Adm_auth_cpn *cpn_p);
extern int  adm_auth_uid2str(char *name_buffer, u_int buffer_size,
	    uid_t user_uid);
extern int  adm_auth_str2uid(char *user_name, uid_t *user_uid_p);
extern int  adm_auth_gid2str(char *name_buffer, u_int buffer_size,
	    gid_t group_gid);
extern int  adm_auth_str2gid(char *group_name, gid_t *group_gid_p);

/* Admin security library error handling function prototype definitions */
extern int  adm_auth_fmterr(Adm_auth_entry *auth_entry_p, u_int error_code,
	    ...);
extern int  adm_auth_seterr(Adm_auth_entry *auth_entry_p, u_int error_code,
	    char *error_message);
extern int  adm_auth_err2str(char **error_message_pp, u_int error_code, ...);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_auth_impl_h */
