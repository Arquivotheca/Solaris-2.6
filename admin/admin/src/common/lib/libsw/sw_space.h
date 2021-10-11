/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#ifndef lint
#pragma ident "@(#)sw_space.h 1.4 96/02/08"
#endif

#ifndef _SW_SPACE_H
#define	_SW_SPACE_H

/* sp_mmap.c */

extern MFILE	*mopen(char *);
extern void	mclose(MFILE *);
extern char	*mgets(char *, int, MFILE *);

/* sp_calc.c */

extern void	begin_space_chk(Space **);
extern void	begin_qspace_chk(Space **);
extern void	end_space_chk(void);
extern void	add_contents_entry(char *, daddr_t, Modinfo *);
extern void	add_file(char *, daddr_t, daddr_t, int);
extern void	add_file_blks(char *, daddr_t, daddr_t, int);
extern int	do_stat_file(char *fname);
extern void	chk_sp_init(void);
extern void	add_fs_overhead(Space **, int);
extern long	get_spooled_size(char *);
extern void	add_upg_fs_overhead(Space **);

/* sp_load.c */

extern int	sp_read_pkg_map(char *, char *, char *, char *, int);
extern int	sp_read_space_file(char *, char *, char *);
extern void	set_sp_err(int, int, char *);

/* sp_spacetab.c */

extern Space **	load_spacetab(Space **, char **);
extern Space **	load_def_spacetab(Space **);
extern Space **	load_defined_spacetab(char **);
extern	void	sort_spacetab(Space **);
extern	void	zero_spacetab(Space **);

/* sp_util.c */

extern int	valid_mountp_list(char **);
extern int	do_chroot(char *);
extern int	check_path_for_vars(char *);
extern void	set_path(char [], char *, char *, char *);
extern void	reset_stab(Space **);
extern int	reset_swm_stab(Space **);
extern int	meets_reqs(Modinfo *);
extern daddr_t	tot_bavail(Space **, int);
extern u_long	new_slice_size(u_long, int);

#ifdef __cplusplus
}
#endif

#endif	/* _SW_SPACE_H */
