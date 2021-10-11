
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the exported administrative argument definitions
 *	from the administrative framework.  The file contains definitions
 *	for:
 *
 *		o Argument constants.
 *		o Argument types.
 *		o Administrative data handle (Adm_data) and argument
 *	          (Adm_arg) structures.
 *
 *******************************************************************************
 */

#ifndef _adm_args_h
#define _adm_args_h

#pragma	ident	"@(#)adm_args.h	1.7	93/05/18 SMI"

#include <sys/types.h>

/*
 *----------------------------------------------------------------------
 * Administrative argument types 
 *----------------------------------------------------------------------
 */

#define ADM_ANYTYPE	((u_int)0)	/* Matches any argument type */
#define ADM_STRING	((u_int)9)	/* String type argument (same as SNM) */

/*
 *----------------------------------------------------------------------
 * Flags for indicating contents of an administrative data handle
 *----------------------------------------------------------------------
 */

#define ADM_INVALID	0x01		/* Unrecognized contents */
#define ADM_FORMATTED	0x02		/* Handle contains formatted data */
#define ADM_UNFORMATTED	0x04		/* Handle contains unformatted data */

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

/*
 *----------------------------------------------------------------------
 * Administrative argument handling interfaces
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	  adm_args_bor(Adm_data *);
extern	int	  adm_args_dela(Adm_data *, char *);
extern	int	  adm_args_freeh(Adm_data *);
extern	int	  adm_args_geta(Adm_data *, char *, u_int, Adm_arg **);
extern	int	  adm_args_geti(char *, u_int, Adm_arg **);
extern	int	  adm_args_getu(Adm_data *, caddr_t *, u_int *);
extern	int	  adm_args_htype(Adm_data *);
extern	int	  adm_args_markr();
extern	Adm_data *adm_args_newh();
extern	int	  adm_args_nexta(Adm_data *, u_int, Adm_arg **);
extern	int	  adm_args_nexti(u_int, Adm_arg **);
extern	int	  adm_args_nextr(Adm_data *);
extern	int	  adm_args_reset(Adm_data *);
extern	int	  adm_args_rsti();
extern	int	  adm_args_puta(Adm_data *, char *, u_int, u_int, caddr_t);
extern	int	  adm_args_putu(Adm_data *, caddr_t, u_int);
extern	int	  adm_args_set(char *, u_int, u_int, caddr_t);


#ifdef __cplusplus
}
#endif

#endif /* !_adm_args_h */

