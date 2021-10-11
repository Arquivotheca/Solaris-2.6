/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 * kdmconfig.h: Misc routines and definitions for kdmconfig
 *
 * Description:
 *
 *
 */

#ifndef _INC_KDMCONFIG_H
#define	_INC_KDMCONFIG_H
#pragma ident "@(#)kdmconfig.h 1.2 93/12/16 SMI"

extern int	force_prompt;

/* PRINTFLIKE1 */
extern void	verb_msg(const char *, ...);

extern void	do_config(void);
extern void	do_unconfig(void);

#endif /* _INC_KDMCONFIG_H_ */
