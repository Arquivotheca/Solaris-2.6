#ifndef lint
#pragma ident "@(#)c_progress.h 1.15 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1992-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	c_progress.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _C_PROGRESS_H
#define	_C_PROGRESS_H

#ifdef __cplusplus
extern "C" {
#endif

	extern void	progress_init(void);
	extern void	progress_error(int);
	extern void	progress_cleanup(void);
	extern void	progress_done(void);
	extern int	start_pkgadd(char *pkgdir);
	extern int	end_pkgadd(char *pkgdir);

#ifdef __cplusplus
}
#endif

#endif
