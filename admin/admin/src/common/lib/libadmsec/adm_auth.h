/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.
 */

#ifndef	_adm_auth_h
#define	_adm_auth_h

#pragma ident	"@(#)adm_auth.h	1.10	93/05/18 SMI"


/*
 * FILE:  adm_auth.h
 *
 *	Admin Framework high level security library header file for
 *	security definitions.
 */

/* Admin security routines completion codes */
#define	ADM_AUTH_OK		0	/* Success status code */

/* Admin security type definitions */
typedef void Adm_auth_handle_t;

/* Admin security information general definitions */
#define	ADM_AUTH_NAMESIZE	128	/* Max size of name identifier */
#define	ADM_AUTH_CPN_NAMESIZE	256	/* Max size of cpn identifier */
#define	ADM_AUTH_ACLENTRY_SIZE	(ADM_AUTH_NAMESIZE + 12)
					/* Max char format ACL entry size */
#define	ADM_AUTH_ERRMSG_SIZE	256	/* Max error message length */

/* Admin security function option definitions: getinfo, putinfo, delinfo */
#define	ADM_AUTH_DEFAULT	1	/* Access class default info */
#define	ADM_AUTH_CLASS		2	/* Access class info */
#define	ADM_AUTH_FIRST		4	/* Access first entry info */
#define	ADM_AUTH_NEXT		8	/* Access next entry info */

/* Admin security function special entry names: getname & read errors */
#define	ADM_AUTH_DEFAULT_NAME	"*DEFAULT*"	/* Class default name */
#define	ADM_AUTH_CLASS_NAME	"*CLASS*"	/* Class entry name */
#define	ADM_AUTH_FIRST_NAME	"*FIRST*"	/* First entry (error) */
#define	ADM_AUTH_NEXT_NAME	"*NEXT*"	/* Next entry (error) */

/* Admin security function option definitions: getacl, setacl */
#define	ADM_AUTH_ACLOFF		1	/* Turn off ACL checking */
#define	ADM_AUTH_FULLIDS	2	/* Use full cpn name in char ACL */
#define	ADM_AUTH_LONGTYPE	4	/* Use long type name in char ACL */
#define	ADM_AUTH_NOIDS		8	/* Do not use uid/gid in char ACL */

/* Admin security function option definitions: getsid, setsid */
#define	ADM_AUTH_CLIENT		1	/* Use client uid/gid in set id's */
#define	ADM_AUTH_AGENT		2	/* Use agent uid/gid in set id's */

/* Admin security information structure authentitication types */
#define	ADM_AUTH_NONE		0	/* No authentication */
#define	ADM_AUTH_WEAK		1	/* Weak authentication */
#define	ADM_AUTH_STRONG		2	/* Strong authentication */

/* Admin authentication type name definitions: getauth, setauth */
#define	ADM_AUTH_NONE_NAME	"none"	/* No authentication */
#define	ADM_AUTH_WEAK_NAME	"weak"	/* Weak authentication */
#define	ADM_AUTH_STRONG_NAME	"strong" /* Strong authentication */

/* Admin authentication flavor name definitions: getauth, setauth */
#define	ADM_AUTH_NONE_NAME	"none"	/* No authentication */
#define	ADM_AUTH_UNIX_NAME	"unix"	/* Unix weak authentication */
#define	ADM_AUTH_DES_NAME	"des"	/* DES strong authentication */

/* Admin security library API function prototype definitions */

#ifdef __cplusplus
extern "C" {
#endif

extern Adm_auth_handle_t *adm_auth_newh();
extern void adm_auth_clearh(Adm_auth_handle_t *auth_hp);
extern void adm_auth_freeh(Adm_auth_handle_t *auth_hp);
extern int  adm_auth_geterr(Adm_auth_handle_t *auth_hp, char **err_msg_pp);
extern int  adm_auth_getinfo(Adm_auth_handle_t *auth_hp, char *class_name,
	    char *class_version, char *method_name, u_int option);
extern int  adm_auth_putinfo(Adm_auth_handle_t *auth_hp, char *class_name,
	    char *class_version, char *method_name, u_int option);
extern int  adm_auth_delinfo(Adm_auth_handle_t *auth_hp, char *class_name,
	    char *class_version, char *method_name, u_int option);
extern int  adm_auth_verinfo(Adm_auth_handle_t *auth_hp);
extern int  adm_auth_getacl(Adm_auth_handle_t *auth_hp, char *acl_buffer,
	    u_int *acl_length_p, u_int *number_entries_p, u_int *acl_flag_p,
	    u_int option);
extern int  adm_auth_setacl(Adm_auth_handle_t *auth_hp, char *acl_buffer,
	    u_int acl_length, u_int *number_entries_p, u_int option);
extern int  adm_auth_addacl(Adm_auth_handle_t *auth_hp, char *acl_buffer);
extern int  adm_auth_delacl(Adm_auth_handle_t *auth_hp, char *acl_buffer);
extern int  adm_auth_modacl(Adm_auth_handle_t *auth_hp, char *acl_buffer);
extern int  adm_auth_clracl(Adm_auth_handle_t *auth_hp);
extern int  adm_auth_rstacl(Adm_auth_handle_t *auth_hp, u_int option);
extern int  adm_auth_getauth(Adm_auth_handle_t *auth_hp, char *auth_type,
	    u_int type_length, char *auth_flavor, u_int flavor_length);
extern int  adm_auth_setauth(Adm_auth_handle_t *auth_hp, char *auth_type,
	    char *auth_flavor);
extern int  adm_auth_getsid(Adm_auth_handle_t *auth_hp, u_int *setid_flag_p,
	    char *user_name_buffer, u_int user_buffer_length,
	    char *group_name_buffer, u_int group_buffer_length, u_int option);
extern int  adm_auth_setsid(Adm_auth_handle_t *auth_hp, char *user_name,
	    char *group_name, u_int option);
extern char *adm_auth_getname(Adm_auth_handle_t *auth_hp);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_auth_h */
