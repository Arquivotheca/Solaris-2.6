/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)launcher_p.h	1.1	95/01/13 SMI"


#include "launcher_api.h"


#ifndef _LAUNCHER_P_H
#define _LAUNCHER_P_H


#ifdef __cplusplus
extern "C" {
#endif

extern const char	*get_global_reg(void);
extern const char	*get_default_icon_path(void);

extern int	lock_registry(const char *tag);

extern int	unlock_registry(const char *tag);

extern int	parse_entry(
			const char	*entry,
			char		*name,
			size_t		name_size,
			char		*icon_path,
			size_t		icon_path_size,
			char		*app_path,
			size_t		app_path_size,
			char		*app_args,
			size_t		app_args_size);

#ifdef __cplusplus
}
#endif


#endif /* _LAUNCHER_P_H */
