#ifndef lint
#pragma ident "@(#)summary_util.h 1.5 96/07/09 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	summary_util.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _SUMMARY_UTIL_H
#define	_SUMMARY_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define	SUMMARY_LABEL_LEN	35
#define	SUMMARY_VALUE_COLUMN	37

typedef struct {
	NRowCol loc;
	char *label;
	char *prompt;
	int (*sel_proc) ();
} _Summary_Item_t;

typedef struct {

	/*
	 * a row in the summary table consists of 2 items: field for
	 * category field for selection/value
	 */
	_Summary_Item_t fld[2];

} _Summary_Row_t;

extern _Summary_Row_t * load_install_summary(int *row);

extern _Summary_Row_t *grow_summary_table(
	_Summary_Row_t *, int *, int *);
extern _Summary_Row_t * load_init_summary(
	_Summary_Row_t * table, int *row, int *last);
extern _Summary_Row_t * load_upg_summary(
	_Summary_Row_t * table, int *row, int *last);
extern	_Summary_Row_t * load_dsr_summary(
	_Summary_Row_t * table, int *row, int *last);
extern _Summary_Row_t *load_rfs_summary(_Summary_Row_t *, int *, int *);
extern _Summary_Row_t *load_lfs_summary(_Summary_Row_t *, int *, int *);
extern _Summary_Row_t *load_sw_summary(_Summary_Row_t *, int *, int *);
extern _Summary_Row_t *load_client_arch_summary(_Summary_Row_t *,
	int *, int *);
extern _Summary_Row_t *load_locale_summary(_Summary_Row_t *, int *,
	int *);
extern void free_summary_table(_Summary_Row_t *, int);
extern void show_summary_table(WINDOW *, int, int, int,
	_Summary_Row_t *, int);

#ifdef __cplusplus
}

#endif

#endif	/* _SUMMARY_UTIL_H */
