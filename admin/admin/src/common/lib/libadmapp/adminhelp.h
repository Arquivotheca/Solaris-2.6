
/* Copyright 1993 Sun Microsystems, Inc. */

#ifndef	ADMINHELP_H
#define	ADMINHELP_H

#pragma ident "@(#)adminhelp.h	1.3 93/11/29 Sun Microsystems"

/*	adminhelp.h	*/


/* the following three defines are used for second argument to adminhelp */
#define TOPIC	'C'
#define HOWTO	'P'
#define REFER	'R'

#ifdef __cplusplus
extern "C" {
#endif

int adminhelp(Widget, char, char*);

#ifdef __cplusplus
}
#endif


#endif	/* ADMINHELP_H */
