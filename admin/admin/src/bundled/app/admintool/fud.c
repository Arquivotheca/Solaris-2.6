/* Copyright 1995 Sun Microsystems, Inc. */

#pragma ident "@(#)fud.c	1.8 96/06/11 Sun Microsystems"

/* fud.c */

#include "software.h"

/* Public */
swFormUserData	* FocusData = NULL;

void free_fud_list();
void traverse_fud_list(void (*)(swFormUserData *, caddr_t), caddr_t);
void set_selected_fud(swFormUserData * fud, caddr_t m);

static swFormUserData	* head_fud = NULL, * tail_fud = NULL;

Boolean
same_fud_locales(swFormUserData * fud0, swFormUserData * fud1)
{
    if (fud0->f_locale == NULL && fud1->f_locale == NULL)
	return(TRUE);

    /* 
     * Since both are not NULL, then if either is NULL, they
     * can't be equal.
     */
    if (fud0->f_locale == NULL || fud1->f_locale == NULL)
	return(FALSE);

    /* Finally, they both are non-NULL, compare f_locale strings */
    return( strcmp(fud0->f_locale, fud1->f_locale) == 0 ? TRUE : FALSE); 
}

/*
 * Used to calculate total size of all selected pkgs.
 * Called by traverse_fud_list on successive elements
 * on fud list. 
 * Total size is accumulated in running_total, with is *int.
 */
void
selected_fud_size(swFormUserData * fud, caddr_t  running_total)
{
    if (fud->f_selected)
        *(int *)running_total += selectedModuleSize(fud->f_module, fud->f_locale);
}

/*
 * Recursive selection status of all fuds
 */
void
fud_selection_status(swFormUserData * fud, caddr_t on)
{
    if (fud->f_selected)
	*(int *)on = 1;
}

void
free_fud_list()
{
	swFormUserData * n;
	swFormUserData * fud = head_fud;

	if (fud == NULL)
		return;

	do {
		n = fud->f_next;
		free(fud);
		fud = n;
	} while (fud);

	head_fud = tail_fud = NULL;
}

swFormUserData *
fud_create(Widget toggle, Module * mtmp, Modinfo * mi)
{
	swFormUserData * fud = NULL;

	fud = (swFormUserData *) malloc(sizeof(swFormUserData));
	if (fud == NULL)
		fatal("fud_create: can not alloc\n");

        memset(fud, 0, sizeof(swFormUserData));
	fud->f_toggle = toggle;
	fud->f_module = mtmp;
	fud->f_mi = mi;
	fud->f_copyOfModule = NULL;
	fud->f_selected = False;
 	fud->f_locale = mi->m_locale;
	fud->f_swTree = NULL;
	fud->f_next = NULL;
	if (head_fud == NULL)
		head_fud = tail_fud = fud;
	else
		tail_fud->f_next = fud;
	tail_fud = fud;
	return(fud);
}

/*
 * Used to reset to selected status of a fud to 0.
 * Called by traverse_fud_list on each successive element
 * in fud list. 
 */
void
reset_fud(swFormUserData * fud, caddr_t dummy)
{
    fud->f_selected = 0;
}

void
set_selected_fud(swFormUserData * fud, caddr_t m)
{
	if (fud->f_module == (Module *)m && fud->f_selected)
		mark_cluster(fud->f_module, SELECTED, fud->f_locale);
}

/*
 * For each element in fud list apply the function passed as first
 * argument. 2nd argument is passed to this function for its own
 * use.
 */
void
traverse_fud_list(void (fun)(swFormUserData *, caddr_t), caddr_t arg)
{
	swFormUserData * f = head_fud;

	while (f) {
		fun(f, arg);
		f = f->f_next;
	}
}

/* 
 * Set 'selected' status of a fud's module/L10N list
 * as determined by status.
 */

void
mark_fud(swFormUserData * fud, ModStatus status)
{
	mark_cluster(fud->f_module, status, fud->f_locale);
}
