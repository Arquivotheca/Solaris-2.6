/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#ifndef lint
#pragma ident "@(#)sw_space.h 1.8 96/06/10 SMI"
#endif

#ifndef _SW_SPACE_H
#define	_SW_SPACE_H

/*
 *  component type flags, passed in the type_flags argument for
 *  add_file() and add_file_blks();
 */
#define	SP_DIRECTORY	0x0001
#define SP_MOUNTP	0x0002

#ifdef __cplusplus
extern "C" {
#endif

/* sp_calc.c */

void	begin_global_space_sum(FSspace **);
void	begin_global_qspace_sum(FSspace **);
FSspace	**end_global_space_sum(void);
void	begin_specific_space_sum(FSspace **);
void	begin_specific_qspace_sum(FSspace **);
void	end_specific_space_sum(FSspace **);
void	add_file(char *, daddr_t, daddr_t, int, FSspace **);
void	add_file_blks(char *, daddr_t, daddr_t, int, FSspace **);
int	record_save_file(char *fname, FSspace **);
void	do_spacecheck_init(void);
void	add_contents_record(ContentsRecord *, FSspace **);
void	add_spacetab(FSspace **, FSspace **);

/* sp_load.c */

int	sp_read_pkg_map(char *, char *, char *, char *, int, FSspace **);
int	sp_read_space_file(char *, char *, char *, FSspace **);
void	set_sp_err(int, int, char *);

/* sp_spacetab.c */

FSspace **	load_defined_spacetab(char **);
void	sort_spacetab(FSspace **);
FSspace	**load_def_spacetab(FSspace **);

/* sp_util.c */

int	valid_mountp_list(char **);
int	do_chroot(char *);
int	check_path_for_vars(char *);
void	set_path(char [], char *, char *, char *);
void	reset_stab(FSspace **);
int	meets_reqs(Modinfo *);
ContentsRecord *contents_record_from_stab(FSspace **, ContentsRecord *);
void	stab_from_contents_record(FSspace **, ContentsRecord *);

#ifdef __cplusplus
}
#endif

#endif	/* _SW_SPACE_H */
