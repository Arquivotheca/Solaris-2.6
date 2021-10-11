/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)admdb_tbl.c	1.9	95/02/19 SMI"

#include <admldb_impl.h>

/*
 * Function to allocate a column.
 */
static Column *
new_column(void) {
	Column *cp;

	if ((cp = (Column *)malloc(sizeof(Column))) == NULL)
		return (NULL);

	cp->name = cp->val = (char *)NULL;
	cp->up = (void *)NULL;
	cp->next = cp->prev = (Column *)NULL;

	return(cp);
}

/*
 * Function to free a column.
 */
void
free_column(Row *rp, Column *cp) {
	if (cp == NULL) 
		return;
	if (cp->prev == NULL)
		rp->start = cp->next;
	else
		cp->prev->next = cp->next;
	if (cp->next == NULL)
		rp->end = cp->prev;
	else
		cp->next->prev = cp->prev;
	if (cp->up != NULL) {
		free(cp->up->match_val);
		free(cp->up);
	}
	free(cp->val);
	free(cp);
}


/*
 * Function to allocate a row structure.
 */
Row *
new_row(void) {
	Row *rp;

	if ((rp = (Row *)malloc(sizeof(Row))) == NULL)
		return (NULL);
	rp->start = rp->end = (Column *)NULL;
	rp->next = (Row *)NULL;
	rp->tri = NULL;
	return (rp);
}

/*
 * Function to free a row structure.
 */
void
free_row(Row *rp) {
	Column *cp, *ncp;

	if (rp == NULL)
		return;
	for (cp = rp->start; cp != NULL; cp = ncp) {
		ncp = cp->next;
		free_column(rp, cp);
	}

	free_tri(rp->tri);
}


/*
 * Function to allocate a new table data handle.
 */
Table_data *
new_tdh(void) {
	Table_data *tdh;

	if ((tdh = (Table_data *)malloc(sizeof(Table_data))) == NULL)
		return (NULL);

	tdh->start = tdh->end = tdh->current = (Row *)NULL;
	tdh->rows = 0;
	return (tdh);
}

/*
 * Function to free a table data handle.
 */
void
free_tdh(Table *tbl) {
	Row *rp, *nrp;

	if (tbl->tdh == NULL)
		return;

	for (rp = tbl->tdh->start; rp != NULL; rp = nrp) {
		nrp = rp->next;
		free_row(rp);
	}
	free(tbl->tdh);
	tbl->tdh = (Table_data *)NULL;
}


/*
 * Function to allocate a new row information structure.
 */

Table_row_info *
new_tri(void) {
        Table_row_info *tri;

	if ((tri = (Table_row_info *)malloc(sizeof(Table_row_info))) == NULL)
		return (NULL);

	tri->domain      = NULL;
	tri->owner       = NULL;
	tri->group_owner = NULL;
	tri->permissions = 0;
	tri->ttl         = 0;
	return (tri);
}


Table_row_info *
copy_tri(Table_row_info *tri) {
        Table_row_info *tri_copy;

        if (tri == NULL) {
	    return NULL;
	}

	if ((tri_copy = new_tri()) == NULL) {
	    return NULL;
	}

	if (tri->domain != NULL) {
	    tri_copy->domain = strdup(tri->domain);
	}

	if (tri->owner != NULL) {
	    tri_copy->owner = strdup(tri->owner);
	}

	if (tri->group_owner != NULL) {
	    tri_copy->group_owner = strdup(tri->group_owner);
	}
	
	tri_copy->permissions = tri->permissions;
	tri_copy->ttl         = tri->ttl;

	return tri_copy;
}



/* 
 * Function to free table row setting information.
 */

void
free_tri(Table_row_info *tri) {
        if (tri != NULL) {
	    if (tri->domain != NULL) {
	        free(tri->domain);
	    }
	    if (tri->owner != NULL) {
	        free(tri->owner);
	    }
	    if (tri->group_owner != NULL) {
	        free(tri->group_owner);
	    }

	    free(tri);
	}
}

/*
 * Function to allocate a new table structure.
 */
Table *
new_table(void) {
	Table *tbl;

	if ((tbl = (Table *)malloc(sizeof(Table))) == NULL)
		return (NULL);
	tbl->tdh  = NULL;
	tbl->tri  = NULL;
	tbl->type = 0;
	return (tbl);
}

/*
 * Function to free a table structure.
 */
void
free_table(Table *tbl) {

	if (tbl == NULL)
		return;
	free_tdh(tbl);
	free_tri(tbl->tri);
	free(tbl);
}

/*
 * Function to allocate & initialize a table.
 */
Table *
table_of_type(int tt) {
	Table *tbl;

	if ((tbl = new_table()) == NULL)
		return (NULL);

	tbl->type = tt;
	memcpy(&tbl->tn, &adm_tbl_trans[tt]->tn, sizeof(Table_names));
	return (tbl);
}

/*
 * Function to append a row to a table.
 */
int
append_row(
	Table *tbl,
	Row *rp)
{
	if (tbl->tdh == NULL)
		if ((tbl->tdh = new_tdh()) == NULL)
			return (-1);
	
	if (tbl->tdh->end == NULL)
		tbl->tdh->start = tbl->tdh->current = tbl->tdh->end = rp;
	else {
		tbl->tdh->end->next = rp;
		tbl->tdh->end = rp;
	}
	++tbl->tdh->rows;
	return (0);
}

int
set_out_val(
	Row *rp,
	char *cn, 
	char *cv)
{
	Column *cp;
	char *sp;
	
	if (cv == NULL)
	        return (0);

	for (cp = rp->start; cp != NULL; cp = cp->next) {
		if ((cp->name != NULL) && !strcmp(cp->name, cn)) {
		        if ((cp->val = (char *)realloc(cp->val, 
		            (strlen(cp->val) + strlen(cv) + 2))) 
		            == NULL)
				return (-1);
			strcat(cp->val, " ");
			strcat(cp->val, cv);
			return (0);
		}
	}
	if ((cp = new_column()) == NULL)
	        return (-1);
	cp->name = cn;
	cp->val = strdup(cv);
	if (rp->end != NULL)
		rp->end->next = cp;
	else
		rp->start = cp;
	cp->prev = rp->end;
	rp->end = cp;
	return (0);
}

/*
 * Function to locate a particular numbered column in a row.
 */
Column *
column_num_in_row(
	Row *rp,
	int cn)
{
	Column *cp;

	for (cp = rp->start; cp != NULL; cp = cp->next)
		if ((cp->up != NULL) && (cp->up->num == cn))
			break;
	return (cp);
}
	
/*
 * Function to allocate a numbered column for UFS/NIS functions.
 */
Column *
new_numbered_column(
	Row *rp,
	ushort_t num,
	char *val,
	char *match_val,
	ushort_t case_flag)
{
	Column *cp, *tp;
	char *sp;
	
	if ((cp = column_num_in_row(rp, num)) != NULL) {
		if ((sp = (char *)malloc((strlen(cp->val) + strlen(val) + 2))) 
		    == NULL)
			return (NULL);
		sprintf(sp, "%s %s", cp->val, val);
		free(cp->val);
		cp->val = sp;
		return (cp);
	}
	        
	if ((cp = new_column()) == NULL)
		return (NULL);
	if ((cp->up = (struct ufs_column *) malloc(sizeof(struct ufs_column))) == NULL) {
		free(cp);
		return (NULL);
	}
	cp->up->num = num;
	cp->up->case_flag = case_flag;
	cp->up->match_flag = 0;
	if (((cp->up->match_val = match_val) != NULL) &&
	    ((cp->up->match_val = strdup(match_val)) == NULL)) {
		free(cp->up);
		free(cp);
		return (NULL);
	}

	if (((cp->val = val) != NULL) &&
	    ((cp->val = strdup(val)) == NULL)) {
		free(cp->up->match_val);
		free(cp->up);
		free(cp);
		return(NULL);
	}
	
	for (tp = rp->end; 
	    ((tp != NULL) && (tp->up->num > cp->up->num)); 
	    tp = tp->prev);

	if (tp == NULL) {
		cp->next = rp->start;
		rp->start = cp;
		if (rp->end == NULL)
			rp->end = cp;
		else
			cp->next->prev = cp;
	} else {
		cp->next = tp->next;
		tp->next = cp;
		cp->prev = tp;
		if (cp->next != NULL)
			cp->next->prev = cp;
		else
			rp->end = cp;
	}

	return (cp);
}

