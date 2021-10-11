/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)launcher_api.h	1.1	95/01/13 SMI"

#ifndef _LAUNCHER_API_H
#define _LAUNCHER_API_H


#define	LAUNCH_OK		0
#define	LAUNCH_ERROR		-1
#define	LAUNCH_BAD_INPUT	-10
#define	LAUNCH_LOCKED		-11
#define	LAUNCH_DUP		-12
#define	LAUNCH_NO_ENTRY		-13
#define	LAUNCH_NO_REGISTRY	-14


typedef struct s_solstice_app {
	const char *name;
	const char *icon_path;
	const char *app_path;
	const char *app_args;
} SolsticeApp;


#ifdef __cplusplus
extern "C" {
#endif

extern int	solstice_add_app(
		    const SolsticeApp	*app,
		    const char		*registry_tag);

extern int	solstice_del_app(
		    const char		*name,
		    const char		*registry_tag);

extern int	solstice_get_apps(
		    SolsticeApp		**apps,
		    const char		*registry_tag);

void		solstice_free_app_list(
		    SolsticeApp		*app_list,
		    int			cnt);

#ifdef __cplusplus
}
#endif


#endif /* _LAUNCHER_API_H */
