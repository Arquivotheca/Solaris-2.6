/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident   "@(#)showrev.c 1.24     95/08/22 SMI"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <fcntl.h>
#include <libelf.h>
#include <dirent.h>
#include <pkglocs.h>
#include <pwd.h>
#include <grp.h>

#define	NO_ERR		0
#define	MTHD_ERR	1
#define	ARG_ERR		2
#define	LIB_ERR		3

#define	MTHD_ERR_MSG	"%1$s: %3$s\n"
#define	ARG_ERR_MSG	"%1$s: Error retrieving parameter %2$s: %3$s\n"
#define	ENV_VAR_MSG	"%1$s: Unable to retrieve value for environment variable %2$s\n"
#define	NOT_SET_MSG	"\n%1$s is not set in the current environment\n"
#define	NOT_IN_MSG	"\n%1$s not found in current %2$s\n"
#define	VAR_IS_MSG	"\n%1$s is:\n%2$s\n"
#define	ITEM_MSG	"%1$s: %2$s\n"
#define	NO_PATCHES	"No patches are installed\n"
#define	PATCH_FMT	"Patch: %s  Obsoletes: %s  Packages: %s\n"

#define	PATH_VAR	"PATH"
#define	LD_LIBRARY_PATH_VAR	"LD_LIBRARY_PATH"
#define	PWD_VAR		"PWD"
#define	HOSTNAME_STR	"Hostname"
#define	HOSTID_STR	"Hostid"
#define	RELEASE_STR	"Release"
#define	KERNARCH_STR	"Kernel architecture"
#define	APPARCH_STR	"Application architecture"
#define	HWPROV_STR	"Hardware provider"
#define	DOMAIN_STR	"Domain"
#define	KERNVERS_STR	"Kernel version"
#define	OWVERS_STR	"OpenWindows version"
#define	FILETYPE_STR	"File type"
#define	CMDVERS_STR	"Command version"
#define	MODE_STR	"File mode"
#define	OWNER_STR	"User owning file"
#define	GROUP_STR	"Group owning file"
#define	LIB_STR		"Library information"
#define	SUM_STR		"Sum"
#define	FILE_STR	"File"
#define	ELF_STR		"ELF"
#ifndef	FALSE
#define	FALSE	(0)
#endif

#ifndef	TRUE
#define	TRUE	(1)
#endif

#ifndef	NULL
#define	NULL	0
#endif

#define	SYS_CLASS	"system"
#define ADM_STRING	((u_int)9)	/* String type argument (same as SNM) */
#define ADM_SUCCESS	0
/*
 *----------------------------------------------------------------------
 * Framework control options.
 *----------------------------------------------------------------------
 */

#define ADM_ENDOPTS	   0	/* End of framework control options */
#define ADM_CLASS	   1	/* Name of class for the invoked method */
#define ADM_CLASS_VERS	   2	/* Class version number */
#define ADM_HOST	   3	/* Host on which to perform method */
#define ADM_DOMAIN	   4	/* Domain in which to perform request */
#define ADM_ACK_TIMEOUT	   5	/* Timeout for waiting for initial request ack */
#define ADM_REP_TIMEOUT	   6	/* Timeout for waiting for agent report */
#define ADM_AGENT	   7	/* Class agent program and version # */
#define ADM_CATEGORIES	   8	/* Additional categories for tracing message */
#define ADM_PINGS	   9	/* # of ping retries before giving up on agent */
#define ADM_PING_TIMEOUT  10	/* Timeout to wait for ping acknowledgement */
#define ADM_PING_DELAY	  11	/* Delay before beginning to ping request */
#define ADM_AUTH_TYPE	  12	/* Request authentication type */
#define ADM_AUTH_FLAVOR	  13	/* Request authentication flavor */
#define ADM_CLIENT_GROUP  14	/* Client's preferred group */
#define ADM_ALLOW_AUTH_NEGO 15  /* Allow authentication negotiation? */
#define ADM_LOCAL_DISPATCH  16  /* Invoke method locally w/o using RPC? */


#define SYS_HOSTNAME_PAR		"hostname"
#define SYS_HW_PROVIDER_PAR		"hw_provider"
#define SYS_HW_SERIAL_PAR		"hw_serial"
#define SYS_RELEASE_PAR			"release"
#define SYS_MACHINE_PAR			"machine"
#define SYS_ARCHITECTURE_PAR		"architecture"
#define SYS_DOMAIN_PAR			"domain"
#define SYS_OPENWIN_REV_PAR		"openwin_rev"
#define	SYS_CURR_PATH_PAR		"curr_path"
#define SYS_LD_LIBRARY_PATH_PAR		"ld_library_path"
#define SYS_KERNEL_REV_PAR		"kernel_rev"
#define SYS_OWNER_PAR			"owner"
#define SYS_GROUP_PAR			"group"
#define SYS_MODE_PAR			"mode"
#define SYS_ELF_LIB_PAR			"elf_lib"
#define SYS_CHECKSUM_PAR		"checksum"
#define SYS_FILE_PAR			"file"
#define SYS_FILE_TYPE_PAR		"file_type"
#define SYS_COMMAND_REV_PAR		"command_rev"
#define SYS_PATH_PAR			"path"
#define	SYS_PATCHID_PAR			"patchid"
#define	SYS_OBSOLETES_PAR		"obsoletes"
#define	SYS_PKGS_PAR			"pkgs"
#define SYS_FULL_PATH_PAR		"full_path"
#define SYS_GET_FILE_TYPE_MTHD		"get_file_type"
#define SYS_GET_COMMAND_REV_MTHD	"get_command_rev"
#define SYS_GET_FILE_PERMISSIONS_MTHD	"get_file_permissions"
#define SYS_GET_ELF_LIBS_MTHD		"get_elf_libs"
#define SYS_GET_FILE_CHECKSUM_MTHD	"get_file_checksum"
#define SYS_GET_KERNEL_REV_MTHD		"get_kernel_rev"
#define	SYS_LIST_PATCHES_MTHD		"list_patches"
#define SYS_GET_SYSINFO_MTHD		"get_sysinfo"
#define SYS_GET_OPENWIN_REV_MTHD	"get_openwin_rev"
#define SYS_FIND_FILE_IN_PATH_MTHD	"find_file_in_path"

#define UNIX_LOCATION_1 	"/kernel/unix"
#define UNIX_LOCATION_2 	"/kernel/genunix"
#define	SCCS_STR	"@(#)"
#define	BLANK_STR	"    "

#define	OPENWINHOME_VAR	"OPENWINHOME"
#define OPENWINHOME_DEFAULT "/usr/openwin"
#define	XNEWS_REL_LOC	"bin/Xsun"

#define	NAME_VAR	"NAME"
#define	VERSION_VAR	"VERSION"
#define	PATCHID_VAR	"SUNW_PATCHID"
#define	OBSOLETES_VAR	"SUNW_OBSOLETES"
#define PATCHLIST_VAR	"PATCHLIST"
#define PATCH_INFO_VAR	"PATCH_INFO_%s"

/*
 * pia will be the list of progressive instance patches, dipia will be a list
 * of direct instance patches.
 */
struct patchinfo {
	char *patchid;
	char *obsoletes;
	char *pkg_name_vers;
} **pia = NULL, **dipia = NULL;
int cnt = 0;	/* counts progressive instance patches. */
int dicnt = 0;	/* counts direct instance patches. */
char *errdata;

typedef struct column {
	ushort_t num;		/* Number of this column */
	ushort_t first_match;	/* First column to match against */
	ushort_t last_match;	/* Last column to match against */
	char *match_val;	/* Value to match */
	char *replace_val;	/* New value when replacing */
	ushort_t case_flag;	/* Handle column as case-insensitive? */
	ushort_t match_flag;	/* Was match successful on this column? */
	struct column *next;	/* Next list member */
	struct column *prev;	/* Previous list member */
} Column;

typedef struct col_list {
	Column *start;		/* First column in list */
	Column *end;		/* Last column in list */
	char *column_sep;	/* Separator string for columns */
	char *comment_sep;	/* Separator string for comment */
	char *comment;		/* Comment for this entry */
} Col_list;


/*
 *----------------------------------------------------------------------
 * Administrative handle and argument structures
 *----------------------------------------------------------------------
 */

#define ADM_MAX_NAMESIZE	127	/* Max. length of argument names */

typedef struct Adm_arg Adm_arg;		/* Administrative argument */
struct Adm_arg {
	char	name[ADM_MAX_NAMESIZE + 1];	/* Null-terminated arg name */
	u_int	type;				/* Argument type */
	u_int	length;				/* Length of arg value */
	caddr_t	valuep;				/* Ptr. to arg value */
};

typedef struct Adm_arglink Adm_arglink;	/* Link to an administrative argument */
struct Adm_arglink {
	Adm_arg		*argp;			/* Ptr to admin arg */
	Adm_arglink	*next_alinkp;		/* Ptr to next arg link in row */
};

typedef struct Adm_rowlink Adm_rowlink;	/* Link to row of admin args */
struct Adm_rowlink{
	Adm_arglink	*first_alinkp;		/* Ptr to first arg link in row */
	Adm_rowlink	*next_rowp;		/* Ptr to next row in table */
};

typedef struct Adm_data Adm_data; /* Administrative data handle */
struct Adm_data {

	/****** SPACE FOR MUTEX LOCK FOR MT ******/

	u_int		unformatted_len;	/* Len of unformatted data blk */
	caddr_t		unformattedp;		/* Ptr to unformatted data blk */
	Adm_rowlink	*first_rowp;		/* Ptr to first row in table */
	Adm_rowlink	*last_rowp;		/* Ptr to last row in table */
	Adm_rowlink	*current_rowp;		/* Ptr to current row in table */
	Adm_arglink	*current_alinkp;	/* Ptr to cur arg link in table */
};

typedef struct Adm_error Adm_error;	/* Administrative error */
struct Adm_error {
    int     code;	    /* Error code (ADM_SUCCESS if successful) */
    u_int   type;	    /* Error type (ADM_ERR_SYSTEM or ADM_ERR_CLASS) */
    u_int   cleanup;	    /* Cleanliness (ADM_FAILCLEAN or ADM_FAILDIRTY) */
    char   *message;	    /* Optional error message */
    u_int   unfmt_len;	    /* Length of unformatted error block */
    caddr_t unfmt_txt;	    /* Unformatted error block */
};

Adm_data *in_handle_p;
Adm_error *errbuf;

int elf=FALSE;
char *hostname;
char *myname;

/*
 * do_sys - call the get_sysinfo method to get most of the system config.
 */
int
do_sys()
{
	Adm_data *out_handle_p = NULL;
	Adm_arg *arg_p;
	char buf[SYS_NMLN];	/* Buffer for sysinfo() return values */
	long serial;		/* Serial number for conversion to hex */
	
	if (hostname) {
	        if (adm_perf_method(SYS_GET_SYSINFO_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {
			fprintf(stderr, MTHD_ERR_MSG, myname, 
				SYS_GET_SYSINFO_MTHD,
				errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
	
		if (adm_args_geta(out_handle_p, SYS_HOSTNAME_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_HOSTNAME_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, HOSTNAME_STR, arg_p->valuep);
		
		if (adm_args_geta(out_handle_p, SYS_HW_SERIAL_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_HW_SERIAL_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, HOSTID_STR, arg_p->valuep);
	
	
		if (adm_args_geta(out_handle_p, SYS_RELEASE_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_RELEASE_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, RELEASE_STR, arg_p->valuep);
	
		if (adm_args_geta(out_handle_p, SYS_MACHINE_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_MACHINE_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, KERNARCH_STR, arg_p->valuep);
		
		if (adm_args_geta(out_handle_p, SYS_ARCHITECTURE_PAR, 
				  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, 
			    SYS_ARCHITECTURE_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, APPARCH_STR, arg_p->valuep);
		
		if (adm_args_geta(out_handle_p, SYS_HW_PROVIDER_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, 
			    SYS_HW_PROVIDER_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, HWPROV_STR, arg_p->valuep);
		
		if (adm_args_geta(out_handle_p, SYS_DOMAIN_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_DOMAIN_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, DOMAIN_STR, arg_p->valuep);
		adm_args_freeh(out_handle_p);
	} else {
		if (sysinfo(SI_HOSTNAME, buf, sizeof(buf)) == -1) {
			fprintf(stderr, MTHD_ERR_MSG, myname, SYS_HOSTNAME_PAR,
			    strerror(errno));	
			return (MTHD_ERR);
		}
		printf(ITEM_MSG, HOSTNAME_STR, buf);
	
		if (sysinfo(SI_HW_SERIAL, buf, sizeof(buf)) == -1) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_HW_SERIAL_PAR,
			    strerror(errno));
			return (MTHD_ERR);
		}
		sscanf(buf, "%d", &serial);
		sprintf(buf, "%x", serial);
		printf(ITEM_MSG, HOSTID_STR, buf);
	
		if (sysinfo(SI_RELEASE, buf, sizeof(buf)) == -1) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_RELEASE_PAR,
			    strerror(errno));
			return (MTHD_ERR);
		}
		printf(ITEM_MSG, RELEASE_STR, buf);
	
		if (sysinfo(SI_MACHINE, buf, sizeof(buf)) == -1) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_MACHINE_PAR,
			    strerror(errno));
			return (MTHD_ERR);
		}
		printf(ITEM_MSG, KERNARCH_STR, buf);
		
		if (sysinfo(SI_ARCHITECTURE, buf, sizeof(buf)) == -1) {
			fprintf(stderr, ARG_ERR_MSG, myname,
			    SYS_ARCHITECTURE_PAR,
			    strerror(errno));
			return (MTHD_ERR);
		}
		printf(ITEM_MSG, APPARCH_STR, buf);
		
		if (sysinfo(SI_HW_PROVIDER, buf, sizeof(buf)) == -1) {
			fprintf(stderr, ARG_ERR_MSG, myname, 
			    SYS_HW_PROVIDER_PAR,
			    strerror(errno));
			return (MTHD_ERR);
		}
		printf(ITEM_MSG, HWPROV_STR, buf);
		
		if (sysinfo(SI_SRPC_DOMAIN, buf, sizeof(buf)) == -1) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_DOMAIN_PAR,
			    strerror(errno));
			return (MTHD_ERR);
		}
		printf(ITEM_MSG, DOMAIN_STR, buf);
	}
	return (NO_ERR);
}

int
do_kernel()
{
	Adm_data *out_handle_p = NULL;
	Adm_arg *arg_p;
	int fd;
	Elf *elf;
	char kernel_rev[4000];
	char *name;
	Elf_Scn *scn;
	Elf32_Shdr *shdr;
	Elf32_Ehdr *ehdr;
	char *buf;
	char *cp;

	if (hostname) {
	        if (adm_perf_method(SYS_GET_KERNEL_REV_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {	
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_KERNEL_REV_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
	
		if (adm_args_geta(out_handle_p, SYS_KERNEL_REV_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_KERNEL_REV_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		printf(ITEM_MSG, KERNVERS_STR, arg_p->valuep);
	} else {
		if ((fd = open(UNIX_LOCATION_1, O_RDONLY)) == -1) {
			if ((fd = open(UNIX_LOCATION_2, O_RDONLY)) == -1) {
				fprintf(stderr, MTHD_ERR_MSG, myname, 
				SYS_GET_KERNEL_REV_MTHD,
			    	strerror(errno));	
				return (MTHD_ERR);
			}
		}
		if (elf_version(EV_CURRENT) == EV_NONE)	{
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_KERNEL_REV_MTHD,
			    strerror(errno));	
			close(fd);
			return (MTHD_ERR);
		}
		elf = elf_begin(fd, ELF_C_READ, NULL);
		ehdr = elf32_getehdr(elf);
		scn = 0;
	
		kernel_rev[0]='\0';
		while ((scn = elf_nextscn(elf, scn)) != 0) {
			name = NULL;
			if ((shdr = elf32_getshdr(scn)) != 0)
				name = elf_strptr(elf, ehdr->e_shstrndx, 
						  shdr->sh_name);
	
			if (!strcmp(name, ".comment")) {
				lseek(fd, (long)shdr->sh_offset, 0);
				buf = (char *)malloc(shdr->sh_size + 2);
	
				if (read(fd, buf, shdr->sh_size) != 
							shdr->sh_size) {
					fprintf(stderr, MTHD_ERR_MSG, myname,
						SYS_GET_KERNEL_REV_MTHD,
						strerror(errno));
					close(fd);
					elf_end(elf);
					return (MTHD_ERR);
			        } else {
	                                buf[shdr->sh_size] = '\0';
					for (cp = buf; 
					     cp < (buf + shdr->sh_size); 
					     cp++)
						if (*cp == '\0')
							*cp = '\n';
					cp = buf;
					while ((cp = strstr(cp, SCCS_STR)) 
					    != NULL)
						strncpy(cp, BLANK_STR, 
						    strlen(BLANK_STR));
				}
				break;
			}
		}
		*(buf+(shdr->sh_size - 1)) = '\0';  
		if (!strncmp(buf, SCCS_STR, strlen(SCCS_STR)))
			cp = buf + strlen(SCCS_STR);
		else
			cp = buf;
		while (isspace(*cp))
			++cp;
	
		elf_end(elf); 
		close(fd); 
	
		printf(ITEM_MSG, KERNVERS_STR, cp);
	}
	return (NO_ERR);
}

int
do_window()
{
	Adm_data *out_handle_p = NULL;
	Adm_arg *arg_p;
	char *owhome;
	char xnews[MAXPATHLEN];
	char *openwin_rev;
	
	if (hostname) {
	        if (adm_perf_method(SYS_GET_OPENWIN_REV_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {	
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_OPENWIN_REV_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
	
		if (adm_args_geta(out_handle_p, SYS_OPENWIN_REV_PAR, ADM_STRING,
				  &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, 
			    SYS_OPENWIN_REV_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		openwin_rev=arg_p->valuep;
	
		printf("\n");
		printf(ITEM_MSG, OWVERS_STR, "");
		printf("%s\n",openwin_rev);
		adm_args_freeh(out_handle_p);
	} else {
		if ((owhome = getenv(OPENWINHOME_VAR)) == NULL) {
			owhome=OPENWINHOME_DEFAULT;
		}
		sprintf(xnews, "%s/%s", owhome, XNEWS_REL_LOC);
		
		if (do_command_rev(xnews, 1) != NO_ERR) {
			fprintf(stderr, ARG_ERR_MSG, myname, 
				SYS_OPENWIN_REV_PAR, 
				errbuf->message);
			return (MTHD_ERR);
		}
	}
	return (NO_ERR);	
}

char *
do_file_type(char *path)
{
	char file_type[1024], buff[1024];
	char temp[(MAXPATHLEN + 10)];
	int fd;
	FILE *fp;
	char *cp;

	if ((fd = open(path, O_RDONLY)) == -1) {
		errdata=strerror(errno);
		close(fd);
		return NULL;
	} else
		close(fd);
		
	temp[0]='\0';
	sprintf(temp,"unset LANG LC_MESSAGES LC_ALL LC_CTYPE ; set -f ; file %s",path);

	if ((fp = popen(temp,"r")) == NULL ) {
		errdata=strerror(errno);
		return NULL;
	}

	file_type[0] = '\0';
	while (fgets(buff, sizeof(buff), fp) != NULL) {
	        buff[strlen(buff) - 1] = ' ';
		cp = strncmp(buff, path, strlen(path)) ? 
		    buff : (buff + strlen(path) + 2);
		while (isspace(*cp))
		        ++cp;
		strcat(file_type, cp);
	}
	pclose(fp);

	return (strdup(file_type));
}

int
do_command_rev(char *filename, int window)
{
	char *filetype;
	Adm_data *out_handle_p = NULL;
	Adm_arg *arg_p;
	char *com_rev;
	int i;
	FILE *fp;
	Elf *elfh;
	int fd;
	char *name;
	Elf_Scn *scn;
	Elf32_Shdr *shdr;
	Elf32_Ehdr *ehdr;
	char *buf;
	char *cp;
	int buflen;
	char temp[1024];

	if (hostname) {
	        if (adm_perf_method(SYS_GET_COMMAND_REV_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {	
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_COMMAND_REV_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
		
		if (adm_args_geta(out_handle_p, SYS_FILE_TYPE_PAR, 
		    ADM_STRING, &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_FILE_TYPE_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		
		filetype=arg_p->valuep;
		if (strstr(filetype, "ELF")!=NULL)
			elf=TRUE;
		else
			elf=FALSE;
	
		printf(ITEM_MSG, FILETYPE_STR, filetype);
	
		if (adm_args_geta(out_handle_p, SYS_COMMAND_REV_PAR, 
					  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, 
			    SYS_COMMAND_REV_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		com_rev=arg_p->valuep;
		printf(ITEM_MSG, CMDVERS_STR, com_rev);
		adm_args_freeh(out_handle_p);
		return (NO_ERR);
	} else {
		filetype = do_file_type(filename);
		if (!filetype) {
			fprintf(stderr, MTHD_ERR_MSG, SYS_GET_FILE_TYPE_MTHD, 
				myname,
				errdata);
			return(MTHD_ERR);
		}
	
		if (strstr(filetype, ELF_STR)!=NULL)
			elf=TRUE;
		else
			elf=FALSE;
	
		if (!window)
			printf(ITEM_MSG, FILETYPE_STR, filetype);
	
		if ( (strstr(filetype,"ELF") != NULL) && 
		     ( (strstr(filetype,"executable SPARC") != NULL) ||
		       (strstr(filetype,"executable 80") != NULL) ) ) {
			if ((fd = open(filename, O_RDONLY)) == -1) {
				close(fd);
				fprintf(stderr, MTHD_ERR_MSG, myname, 
					SYS_GET_COMMAND_REV_MTHD,
					strerror(errno));
				return(MTHD_ERR);
			}
	
			if (elf_version(EV_CURRENT) == EV_NONE) {
				close(fd);
				fprintf(stderr, MTHD_ERR_MSG, myname, 
					SYS_GET_COMMAND_REV_MTHD,
					strerror(errno));
				return(MTHD_ERR);
			}
	
			elfh = elf_begin(fd, ELF_C_READ, NULL);
			ehdr = elf32_getehdr(elfh);
			scn = 0;
	
			while ((scn = elf_nextscn(elfh, scn)) != 0) {
				name = NULL;
				if ((shdr = elf32_getshdr(scn)) != 0)
					name = elf_strptr(elfh, 
							  ehdr->e_shstrndx, 
							  shdr->sh_name);
				if (!strcmp(name, ".comment")) {
					lseek(fd, (long)shdr->sh_offset, 0);
					buflen = 
					 (shdr->sh_size > (sizeof(temp) - 2)) ? 
					  sizeof(temp) - 2 : shdr->sh_size;
					if (read(fd, temp, buflen) != buflen) {
						fprintf(stderr, MTHD_ERR_MSG, 
						myname, 
						SYS_GET_COMMAND_REV_MTHD,
						strerror(errno));
						return(MTHD_ERR);
					} else {
						temp[buflen] = '\0';
						for (cp = temp; 
						     cp < (temp + buflen); 
						     cp++)
							if (*cp == '\0')
								*cp='\n';
						cp = temp;
						while ((cp = strstr(cp, 
								    SCCS_STR)) 
						    != NULL)
							strncpy(cp, BLANK_STR, 
							    strlen(BLANK_STR));
						    
					}
					break;
				}
			}
			temp[buflen - 1] = '\0';
			if (!strncmp(temp, SCCS_STR, strlen(SCCS_STR)))
			        cp = temp + strlen(SCCS_STR);
			else
			        cp = temp;
			while (isspace(*cp))
			        ++cp;
			com_rev = cp;
			elf_end(elfh); 
			close(fd); 
			if (window) {
				printf("\n");
				printf(ITEM_MSG, OWVERS_STR, "");
				printf("%s\n",com_rev);
			} else
				printf(ITEM_MSG, CMDVERS_STR, com_rev);
			return (NO_ERR);
		} else if ((strstr(filetype,"commands text") != NULL) ||
	            (strstr(filetype, "script") != NULL)) {
	
	  	  	/* parse first few lines for SCCS string or such */
	
			if ((fp = fopen(filename,"r")) == NULL) {
				fclose(fp);
				fprintf(stderr, MTHD_ERR_MSG, myname, 
					SYS_GET_COMMAND_REV_MTHD,
					strerror(errno));
				return(MTHD_ERR);
			}
	
			while ((fgets(temp, sizeof(temp), fp) != NULL) && 
			    (temp[0] == '#')) {
				if ((buf = strstr(temp, SCCS_STR)) != NULL) {
				        buf += strlen(SCCS_STR);
				        if ((cp = strchr(buf, '"')) != NULL)
				                *cp = '\0';
					com_rev = buf;
					printf(ITEM_MSG, CMDVERS_STR, com_rev);
					return (NO_ERR);
				}
		      	}
		      	(void) fclose(fp);
		} else if (strstr(filetype, "executable") != NULL) {
			sprintf(temp, "set -f ; /usr/ccs/bin/what %s", 
				filename);
			if ((fp = popen(temp, "r")) == NULL) {
				fprintf(stderr, MTHD_ERR_MSG, myname, 
					SYS_GET_COMMAND_REV_MTHD,
					strerror(errno));
				return(MTHD_ERR);
			}
			temp[0] = '\0';
			i = 0;
			buf = (char *) malloc(1024);
			while ((i < sizeof(temp)) &&
			       (fgets(buf, 1024, fp) != NULL)) {
			        if (strstr(buf, filename) != NULL)
			                continue;
				strcat(temp, buf);
				i += strlen(buf);
			}
			pclose(fp);
			printf(ITEM_MSG, CMDVERS_STR, com_rev);
			return (NO_ERR);
		}
	}
}

static void select_bit(
	int *pairp, 
	mode_t flags,
	char *cp)
{
	register int n;

	n = *pairp++;
	while (n-->0) {
		if((flags & *pairp) == *pairp) {
			pairp++;
			break;
		} else {
			pairp += 2;
		}
	}
	*cp = *pairp;
}

int
do_file_perms(char *filename)
{
	char *mode, *owner, *group;
	Adm_data *out_handle_p = NULL;
	Adm_arg *arg_p;
	char owner_s[8];
	char group_s[80];
	char perm[20];
	struct stat stbuf;
	struct passwd *pw;
	struct group *grp;
        /* these arrays are declared static to allow initializations */
	static int	m0[] = { 1, S_IRUSR, 'r', '-' };
	static int	m1[] = { 1, S_IWUSR, 'w', '-' };
	static int	m2[] = { 3, S_ISUID|S_IXUSR, 's', S_IXUSR, 'x', S_ISUID, 'S', '-' };
	static int	m3[] = { 1, S_IRGRP, 'r', '-' };
	static int	m4[] = { 1, S_IWGRP, 'w', '-' };
	static int	m5[] = { 3, S_ISGID|S_IXGRP, 's', S_IXGRP, 'x', S_ISGID, 'l', '-'};
	static int	m6[] = { 1, S_IROTH, 'r', '-' };
	static int	m7[] = { 1, S_IWOTH, 'w', '-' };
	static int	m8[] = { 3, S_ISVTX|S_IXOTH, 't', S_IXOTH, 'x', S_ISVTX, 'T', '-'};

        static int  *m[] = { m0, m1, m2, m3, m4, m5, m6, m7, m8};
	int **mp;
	int i;

	if (hostname) {
	        if (adm_perf_method(SYS_GET_FILE_PERMISSIONS_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {	
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_FILE_PERMISSIONS_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
		
		if (adm_args_geta(out_handle_p, SYS_OWNER_PAR, 
					  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_OWNER_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		owner=arg_p->valuep;
	
		if (adm_args_geta(out_handle_p, SYS_GROUP_PAR, 
					  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_GROUP_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		group=arg_p->valuep;
	
		if (adm_args_geta(out_handle_p, SYS_MODE_PAR, 
					  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_MODE_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		mode=arg_p->valuep;
	
		adm_args_freeh(out_handle_p);
	} else {
		if (stat(filename, &stbuf) == -1) {
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_FILE_PERMISSIONS_MTHD,
			    strerror(errno));	
			return (MTHD_ERR);
		}
		
		if ((pw = getpwuid(stbuf.st_uid)) != NULL)
		        strcpy(owner_s, pw->pw_name);
		else
			sprintf(owner_s, "%d", stbuf.st_uid);
		
		if ((grp = getgrgid(stbuf.st_gid)) != NULL)
		        strcpy(group_s, grp->gr_name);
		else
			sprintf(group_s,"%d",  stbuf.st_gid);
		
		for (mp = &m[0], i = 0; 
		     mp < &m[sizeof(m)/sizeof(m[0])];
		     ++mp, ++i)
			select_bit(*mp, stbuf.st_mode, &perm[i]);
		perm[i] = '\0';
		mode=perm;
		owner=owner_s;
		group=group_s;
	
	}
	printf(ITEM_MSG, MODE_STR, mode);
	printf(ITEM_MSG, OWNER_STR, owner);
	printf(ITEM_MSG, GROUP_STR, group);
	return (NO_ERR);
}

int
do_elf(char *filename, char *libs)
{
	char *elf_libs;
	Adm_data *out_handle_p = NULL;	
	Adm_arg *arg_p;
	char temp[80], env_var[80];
	FILE *fp;

	printf(ITEM_MSG, LIB_STR, "");
	
	if (hostname) {
	        if (adm_perf_method(SYS_GET_ELF_LIBS_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_ELF_LIBS_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
	
		if (adm_args_geta(out_handle_p, SYS_ELF_LIB_PAR, 
				  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			printf("No ELF dynamic libraries\n");
			adm_args_freeh(out_handle_p);
			return (NO_ERR);
		}
		
		for (;;) {
			elf_libs=arg_p->valuep;
			printf("%s", elf_libs);
	
			if ((adm_args_nextr(out_handle_p) != ADM_SUCCESS) ||
			    (adm_args_geta(out_handle_p, SYS_ELF_LIB_PAR, 
					  ADM_STRING, &arg_p) != ADM_SUCCESS)) {
				adm_args_freeh(out_handle_p);
				return (NO_ERR);
			}
	
		}
	} else {
		if (libs) {
			strcpy(env_var,"LD_LIBRARY_PATH=");
			strcat(env_var,libs);
			if (putenv(env_var) !=0) 
			{
				fprintf(stderr, MTHD_ERR_MSG, myname, 
					SYS_GET_ELF_LIBS_MTHD,
					"putenv failed");
				return (MTHD_ERR);
			}		
		}
		temp[0]='\0';
		sprintf(temp,"set -f ; /bin/ldd %s",filename);
	
		if ((fp = popen(temp,"r")) == NULL )
		  {
			fprintf(stderr, MTHD_ERR_MSG, myname, 
				SYS_GET_ELF_LIBS_MTHD,
				"ldd failed");
			return (MTHD_ERR);
	          }
	
		while (fgets(temp, 80, fp) != NULL)
		  {
			  printf("%s", temp);
		  }
	}

	return (NO_ERR);


}

int
do_checksum(char *filename)
{
	char temp[40];
	FILE *fp;
	char checksum[20], blocks[20];
	int cond;
	char errmsg[80];
	Adm_data *out_handle_p = NULL;	
	Adm_arg *arg_p;

	if (hostname) {
	        if (adm_perf_method(SYS_GET_FILE_CHECKSUM_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_GET_FILE_CHECKSUM_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
		
		if (adm_args_geta(out_handle_p, SYS_CHECKSUM_PAR, 
					  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			fprintf(stderr, ARG_ERR_MSG, myname, SYS_CHECKSUM_PAR,
			    errbuf->message);
			adm_args_freeh(out_handle_p);
			return (ARG_ERR);
		}
		strcpy(checksum,arg_p->valuep);
		adm_args_freeh(out_handle_p);
	} else {
		temp[0]='\0';
		sprintf(temp,"set -f ; sum %s",filename);
		
		if ((fp = popen(temp,"r")) == NULL )
		{
			fprintf(stderr, MTHD_ERR_MSG, myname, 
				SYS_GET_FILE_CHECKSUM_MTHD,
				strerror(errno));
			return (MTHD_ERR);
		}
	
		fscanf(fp, "%s %s",checksum, blocks);
	}
	printf(ITEM_MSG, SUM_STR, checksum);
	return (NO_ERR);
}

int
do_c(char *filename)
{
	Adm_data *out_handle_p = NULL;
	Adm_arg *arg_p;
	char *cur_dir;
	struct stat stbuf;            
	Col_list *listp;
	Column *colp;
	int found=0;
	char *command_rev;
	char *filepath;
	char *path;
	char *ld_library_path;
	char *cwd;
	char scrub[50];
	int status;
	int i;
	
	if (hostname) {
		adm_args_puta(in_handle_p, SYS_FILE_PAR, ADM_STRING,
			      strlen(filename), filename);
	}

	if ((path = getenv(PATH_VAR)) == NULL) {
		fprintf(stderr, ENV_VAR_MSG, myname, PATH_VAR);
		return (LIB_ERR);
	} else {
		printf(VAR_IS_MSG, PATH_VAR, path);
		if (hostname) {
			adm_args_puta(in_handle_p, SYS_PATH_PAR, ADM_STRING,
			      strlen(path), path);
		}
	}

	if ((cwd = getcwd(NULL, (MAXPATHLEN + 1))) == NULL) {
		sprintf(scrub, "%s: %s", myname, "getcwd");
		perror(scrub);
		return (LIB_ERR);
	} else {
		printf(VAR_IS_MSG, PWD_VAR, cwd);
		if (hostname) {
			adm_args_puta(in_handle_p, SYS_CURR_PATH_PAR,
				      ADM_STRING, strlen(cwd), cwd);
		}
	}
	
	if ((ld_library_path = getenv(LD_LIBRARY_PATH_VAR)) != NULL) {
		printf(VAR_IS_MSG, LD_LIBRARY_PATH_VAR, ld_library_path);
		if (hostname) {
			adm_args_puta(in_handle_p, SYS_LD_LIBRARY_PATH_PAR, 
				      ADM_STRING,
				      strlen(ld_library_path), ld_library_path);
		}
	} else 
		printf(NOT_SET_MSG, LD_LIBRARY_PATH_VAR);
	
	if (hostname) {
	        if (adm_perf_method(SYS_FIND_FILE_IN_PATH_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS,SYS_CLASS,ADM_HOST,hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {	
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_FIND_FILE_IN_PATH_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
		
		if (adm_args_geta(out_handle_p, SYS_FULL_PATH_PAR, 
				  ADM_STRING, &arg_p) != ADM_SUCCESS) {
			printf(NOT_IN_MSG, filename, PATH_VAR);
			adm_args_freeh(out_handle_p);
			return (NO_ERR);
		}
	
		for(;;)	{
			filepath=arg_p->valuep;
			for (i = 0; i < 72; ++i)
			        putchar('_');
			putchar('\n');
			putchar('\n');
			printf(ITEM_MSG, FILE_STR, filepath);
			for (i = 0; 
			     i < (int)(strlen(filepath) + strlen(FILE_STR) + 2);
			     ++i)
			        putchar('=');
			putchar('\n');
	
			adm_args_puta(in_handle_p, SYS_FILE_PAR, ADM_STRING,
				      strlen(filepath), filepath);
	
			if ((status = do_command_rev(filepath, NULL)) != NO_ERR)
			        return (status);
			if ((status = do_file_perms(filepath)) != NO_ERR)
			        return (status);
			if (elf && ((status = do_elf(filepath, NULL)) 
			    != NO_ERR))
			        return (status);
						
			if ((status = do_checksum(filepath)) != NO_ERR)
			        return (status);
	
			if ((adm_args_nextr(out_handle_p) != ADM_SUCCESS) ||
			    (adm_args_geta(out_handle_p, SYS_FULL_PATH_PAR, 
			    ADM_STRING, &arg_p) != ADM_SUCCESS)) {
				for (i = 0; i < 72; ++i)
					putchar('_');
				putchar('\n');
				adm_args_freeh(out_handle_p);
				return (NO_ERR);
			}
		}
	} else {
		if (*filename == '/') {
			if (stat(filename, &stbuf) == 0) {
				for (i = 0; i < 72; ++i)
					putchar('_');
				putchar('\n');
				putchar('\n');
				printf(ITEM_MSG, FILE_STR, filename);
				for (i = 0; 
				     i < (int)(strlen(filename) + 
							strlen(FILE_STR) + 2);
				     ++i)
					putchar('=');
				putchar('\n');
				if ((status = do_command_rev(filename, 0)) 
				     != NO_ERR)
				        return (status);
				if ((status = do_file_perms(filename)) 
				     != NO_ERR)
				        return (status);
				if (elf && ((status = do_elf(filename, 
							     ld_library_path)) 
				     != NO_ERR))
				        return (status);
						
				if ((status = do_checksum(filename)) != NO_ERR)
				        return (status);
			}
			for (i = 0; i < 72; ++i)
				putchar('_');
			putchar('\n');
			return ;
		}
	
		if (parse_db_buffer(path, ":", NULL, &listp) != 0) {
			for (colp = listp->start; colp != NULL; 
						  colp = colp->next) {
				cur_dir = colp->match_val;
				if (strlen(cur_dir) == 0)
					cur_dir = cwd;
				filepath=strcat(cur_dir, "/");
				filepath=strcat(filepath, filename);
				if (stat(filepath, &stbuf) == 0) {
					found = 1;
					for (i = 0; i < 72; ++i)
					        putchar('_');
					putchar('\n');
					putchar('\n');
					printf(ITEM_MSG, FILE_STR, filepath);
					for (i = 0; 
					     i < (int)(strlen(filepath) + 
						       strlen(FILE_STR) + 2); 
					     ++i)
					        putchar('=');
					putchar('\n');
					if ((status = do_command_rev(filepath, 
								     NULL)) 
					     != NO_ERR)
			        		return (status);
					if ((status = do_file_perms(filepath)) 
					     != NO_ERR)
			        		return (status);
					if (elf && ((status = do_elf(filepath, 
							      ld_library_path)) 
					     != NO_ERR))
			        		return (status);
						
					if ((status = do_checksum(filepath)) 
					     != NO_ERR)
			        		return (status);
	
				}
				if (!colp->next) {
					for (i = 0; i < 72; ++i)
					        putchar('_');
					putchar('\n');
				}
			}
		}
		if (!found)
			printf(NOT_IN_MSG, filename, PATH_VAR);
	}
}

/*
 * This function lists the patches applied to the local machine. It uses
 * interfaces private to pkgadd which may or may not be available in the
 * future. All references to the pkginfo directory need to be replaced with
 * calls to pkginfo() and pkgparam(); but that will need to wait 'til 2.6.
 */
int
list_patches(void)
{
	DIR *dirp;
	struct dirent *dentp;
	char val[1024], val2[1024], path[MAXPATHLEN], patch_info[2048];
	char patch_info_var[25], obsoletes[2046];
	int status = 0;
	int i;

	if ((dirp = opendir(PKGLOC)) == NULL) {
		errdata = "opendir";
		return (-1);
	}

	/* Look at each package directory in the /var/sadm/pkg. */
	while ((dentp = readdir(dirp)) != NULL) {
		if (dentp->d_name[0] == '.')
			continue;

		/*
		 * Read the SUNW_PATCHID parameter from the pkginfo file and
		 * update pia[] appropriately.
		 */
		sprintf(path, "%s/%s/pkginfo", PKGLOC, dentp->d_name);
		if ((errno = get_env_var(path, PATCHID_VAR, val)) == 0 &&
		    *val != '\000' ) {
			/*
			 * While it is not preferred, we are bound to have
			 * the occasional package which contains old-style
			 * and new-style patches. If this is a remnant of a
			 * direct instance patch then we need to skip to the
			 * new style stuff. We determine this by looking for
			 * the direct instance audit entry - the PATCH_INFO
			 * entry.
			 */
			sprintf(patch_info_var, PATCH_INFO_VAR, val);
			if ((errno = get_env_var(path, patch_info_var,
			    patch_info)) != 0 &&
			    strcmp(patch_info, "backed out") != 0) {
				/*
				 * Enter this patch ID into the list without
				 * repetition.
				 */
				for (i = 0; i < cnt; ++i) {
					if (strcmp(val, pia[i]->patchid) == 0) {
						/*
						 * Since it's a match, add this
						 * version to the list.
						 */
						if (get_env_var(path, VERSION_VAR, 
						    val2) != 0)
							val2[0] = '\0';
						pia[i]->pkg_name_vers = (char *) 
						    realloc(pia[i]->pkg_name_vers,
						    (strlen(pia[i]->pkg_name_vers) +
						    strlen(dentp->d_name) + 
						    strlen(val2) + 4));
						strcat(pia[i]->pkg_name_vers, ", ");
						strcat(pia[i]->pkg_name_vers, 
						    dentp->d_name);
						strcat(pia[i]->pkg_name_vers, " ");
						strcat(pia[i]->pkg_name_vers, val2);
						break;
					}
				}

				/* If that was a duplicate, next SUNW_PATCHID entry */
				if (i != cnt)
					continue;

				/* ... otherwise get the obsoletions. */
				pia = (struct patchinfo **) realloc(pia, 
					    (++cnt * sizeof(struct patchinfo *)));
				pia[i] = (struct patchinfo *)
				    malloc(sizeof(struct patchinfo));
				pia[i]->patchid = strdup(val);
				pia[i]->pkg_name_vers = strdup(dentp->d_name);
				if (get_env_var(path, VERSION_VAR, val) == 0) {
					pia[i]->pkg_name_vers = (char *) realloc(
					    pia[i]->pkg_name_vers,
					    (strlen(pia[i]->pkg_name_vers) + 
					    strlen(val) + 2));
					strcat(pia[i]->pkg_name_vers, " ");
					strcat(pia[i]->pkg_name_vers, val);
				}
				if (get_env_var(path, OBSOLETES_VAR, val) == 0)
					pia[i]->obsoletes = strdup(val);
				else
					pia[i]->obsoletes = NULL;
			}
		} else if (errno == 2) {
			errdata = (char *)malloc(strlen(strerror(errno))
					+ strlen(path) + 16);
			sprintf(errdata,"%s - %s", path,strerror(errno));
			fprintf(stderr, MTHD_ERR_MSG, myname,
					SYS_LIST_PATCHES_MTHD, errdata);
		} else if (errno > 0) {
			errdata = (char *)malloc(strlen(dentp->d_name)
					+ strlen(PATCHID_VAR) + 16);
			sprintf(errdata, "get_env_var(%s, %s)",
					dentp->d_name, PATCHID_VAR);
			status = -1;
			break;
		}

		/* direct instance now */
		if ((errno = get_env_var(path, PATCHLIST_VAR, val)) == 0) {
			char *patch, *token_ptr;

			/*
			 * Enter this patch ID into the list without
			 * repetition.
			 */
			token_ptr = val;
			while ((patch = strtok(token_ptr, " ")) != NULL) {
				token_ptr = NULL;

				for (i = 0; i < dicnt; ++i) {
					/*
					 * If this isn't already in the
					 * list...
					 */
					if (strcmp(patch,
					    dipia[i]->patchid) == 0) {
						dipia[i]->pkg_name_vers =
						    (char *) realloc(dipia[i]->pkg_name_vers,
						    (strlen(dipia[i]->pkg_name_vers) +
						    strlen(dentp->d_name) + 4));
						strcat(dipia[i]->pkg_name_vers, ", ");
						strcat(dipia[i]->pkg_name_vers, 
						    dentp->d_name);
						break;
					}
				}

				/* If that was a duplicate, next patch */
				if (i != dicnt)
					continue;

				/*
				 * Populate a new patchinfo structure.
				 */
				dipia = (struct patchinfo **) realloc(dipia, 
				    (++dicnt * sizeof(struct patchinfo *)));
				dipia[i] = (struct patchinfo *)
				    malloc(sizeof(struct patchinfo));
				dipia[i]->patchid = strdup(patch);
				dipia[i]->pkg_name_vers = strdup(dentp->d_name);

				/*
				 * Now get the obsoletes list from the
				 * PATCH_INFO entry.
				 */
				sprintf(patch_info_var, PATCH_INFO_VAR, patch);
				if ((errno = get_env_var(path, patch_info_var,
				    patch_info)) == 0) {
					char *obs_ptr, *this_obs;

					obs_ptr = strstr(patch_info, "Obsoletes:") + 11;
					/*
					 * If the list is empty, the next
					 * entry will be NULL or
					 * alphabetical.
					 */
					if (*obs_ptr == '\000') {
						dipia[i]->obsoletes = NULL;
					} else {
						int last_entry=0;
						int pass = 0;

						/*
						 * This has to be a comma
						 * separated list.
						 */
						do {
							/* point at this token */
							this_obs = obs_ptr;

							while (*obs_ptr != ' ' &&
							    *obs_ptr != '\000' )
								obs_ptr++;
							if ( *obs_ptr == '\000') {
								last_entry=1;
							} else {
								*obs_ptr = '\000';
								obs_ptr++;
							}

							if (pass > 0)
								strcat(obsoletes, ", ");
							strcat(obsoletes, this_obs);
							pass++;
						} while ( !last_entry );

						dipia[i]->obsoletes = strdup(obsoletes);
						*obsoletes = '\000';	/* clear the array */
					}
				} else {
					dipia[i]->obsoletes = NULL;
				}
			}
		}
	}

	(void) closedir(dirp);
	return (status);
}

int
do_patch()
{
	Adm_data *out_handle_p = NULL;
	Adm_arg *arg_p;
	int status;
	int i;
	char *id, *ob, *pkgs;

	if (hostname) {
	        if (adm_perf_method(SYS_LIST_PATCHES_MTHD,
				    in_handle_p, &out_handle_p, &errbuf,
				    ADM_CLASS, SYS_CLASS, ADM_HOST, hostname,
				    ADM_ENDOPTS) != ADM_SUCCESS) {	
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_LIST_PATCHES_MTHD,
			    errbuf->message);	
			adm_args_freeh(out_handle_p);
			return (MTHD_ERR);
		}
		for (cnt = 0, status = ADM_SUCCESS; status == ADM_SUCCESS; 
		    status = adm_args_nextr(out_handle_p)) {
			if (adm_args_geta(out_handle_p, SYS_PATCHID_PAR, 
			    ADM_STRING, &arg_p) == ADM_SUCCESS) {
				id = arg_p->valuep;
				++cnt;
			} else
				continue;
			if (adm_args_geta(out_handle_p, SYS_OBSOLETES_PAR, 
			    ADM_STRING, &arg_p) == ADM_SUCCESS)
				ob = arg_p->valuep;
			else
				ob = "";
			if (adm_args_geta(out_handle_p, SYS_PKGS_PAR, 
			    ADM_STRING, &arg_p) == ADM_SUCCESS)
				pkgs = arg_p->valuep;
			else
				pkgs = "";		
			printf(PATCH_FMT, id, ob, pkgs);
		}
		adm_args_freeh(out_handle_p);
	} else {
		if (list_patches() != 0) {
			fprintf(stderr, MTHD_ERR_MSG, myname, 
			    SYS_LIST_PATCHES_MTHD,
			    errdata);
			return (MTHD_ERR);
		}

		/* Display the progressive instance patches first. */
		for (i = 0; i < cnt; ++i) {
			id = pia[i]->patchid;
			if (pia[i]->obsoletes != NULL)
				ob = pia[i]->obsoletes;
			else
				ob = "";
			pkgs = pia[i]->pkg_name_vers;
			printf(PATCH_FMT, id, ob, pkgs);
		}

		/* And now the direct instance patches. */
		for (i = 0; i < dicnt; ++i) {
			id = dipia[i]->patchid;
			if (dipia[i]->obsoletes != NULL)
				ob = dipia[i]->obsoletes;
			else
				ob = "";
			pkgs = dipia[i]->pkg_name_vers;
			printf(PATCH_FMT, id, ob, pkgs);
		}

	}
	if (cnt == 0 && dicnt == 0)
		printf(NO_PATCHES);
	return (NO_ERR);
}

main(int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int optind;
	int status = 0;
	int aflg = 0;
	int wflg = 0;
	int cflg = 0;
	int pflg = 0;
	int errflg = 0;
	char *command = NULL;
	static char thishost[SYS_NMLN];

	in_handle_p=(Adm_data *)adm_args_newh();
	
	hostname=NULL;
	myname = argv[0];

	while ((c = getopt(argc, argv, "ac:ps:w")) != EOF)
		switch (c) {
		case 's':	
			hostname = optarg;
			if (sysinfo(SI_HOSTNAME, thishost, sizeof(thishost)) <= 0)
				break;
			if (strcmp (hostname, thishost) == 0)
				hostname=NULL;
			break;
		case 'a':
		        ++aflg;
		        ++wflg;
		        ++pflg;
			break;
		case 'w':	
		        ++wflg;
			break;
		case 'c':	
			command = optarg;
			++cflg;
			break;
		case 'p':
		        ++pflg;
		        break;
		case '?':
			errflg++;
		}
	if (errflg) {
		fprintf(stderr,
		    "usage: showrev [-s <system>] [-c <command>] [-w] [-p] [-a]\n");
		exit (1);
        }
	
	if (aflg || (!cflg && !wflg && !pflg))
	        if (!(status = do_sys()))
	        	status = do_kernel();
	if (!status && wflg)
	        status = do_window();
	if (!status && pflg)
	        status = do_patch();
	if (!status && cflg)
		status = do_c(command);
	fflush(stdout);
	exit(status);
	
}


