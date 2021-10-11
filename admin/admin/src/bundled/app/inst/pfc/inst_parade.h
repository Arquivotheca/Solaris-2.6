#ifndef lint
#pragma ident "@(#)inst_parade.h 1.18 96/08/08 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_parade.h
 * Group:	ttinstall
 * Description:
 */

#ifndef	_INST_PARADE_H
#define	_INST_PARADE_H

#include <curses.h>

typedef parAction_t (*parFunc) ();
typedef int (*parIntFunc) ();

extern void pfgParade(parWin_t win);
extern int parade_prev_win(void);
extern parAction_t pfgConfirmExit(void);
extern void pfcSetConfirmExit(int confirmed);
extern int pfcGetConfirmExit(void);

extern parAction_t do_install_intro(parWin_t win);
extern parAction_t do_systype(void);		/* inst_systype.c	*/
extern parAction_t do_sw(void);			/* inst_sw_choice.c	*/
extern parAction_t do_choose_disks(void);	/* inst_disk.c		*/
extern parAction_t do_choose_bootdisk(void);	/* inst_bootdisk.c	*/
extern parAction_t do_rfs(void);		/* inst_rfs.c		*/
extern parAction_t do_initial_summary(void);	/* inst_summary.c	*/
extern parAction_t do_show_filesystems(void);	/* inst_client_arch.c	*/
extern parAction_t do_client_arches(void);	/* inst_client_arch.c	*/
extern parAction_t do_server_params(void);	/* inst_srv_params.c	*/
extern parAction_t do_fs_autoconfig(void);
extern parAction_t do_preserve_fs(void);
extern parAction_t do_fs_space_warning(void);
extern parAction_t do_upgrade_summary(void);
extern parAction_t do_upgrade_progress(void);
extern parAction_t do_upgrade_or_install(void);
extern parAction_t upgrade_sw_edit(void);
extern parAction_t do_os(void);

/* DSR functions */
extern parAction_t do_dsr_analyze(void);
extern parAction_t do_dsr_al_progress(void);
extern parAction_t do_dsr_fsredist(void);
extern parAction_t do_dsr_fssummary(void);
extern parAction_t do_dsr_media(void);
extern parAction_t do_dsr_space_req(int main_parade);


extern void do_disk_edit(void);
extern parAction_t do_sw_edit(void);
extern parAction_t do_alt_lang(void);		/* inst_alt_lang.c	*/

extern int _confirm_reboot(void);
extern void pfcExit(void);
extern int confirm_exit(WINDOW * parent);
extern int customize_disks(WINDOW *, int);	/* inst_disk_edit.c */
extern int try_preserve(WINDOW * win, int disk, int slice, char *mntpt);
extern int prepare_disk(WINDOW * parent, int disk);
extern int edit_disk_parts(WINDOW *, int, int);
extern int do_autolayout(WINDOW *);
extern void edit_disk(WINDOW *, int, int);
extern int check_hostname(WINDOW *, char *);
extern int check_ipaddr(WINDOW *, char *);
extern int check_mountpt(WINDOW *, char *);

#endif	/* _INST_PARADE_H */
