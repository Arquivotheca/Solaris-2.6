/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma ident   "@(#)admldb_impl.c 1.15     95/08/16 SMI"

#include <stdlib.h>
#include <stdarg.h>
#include <libintl.h>
#include <rpcsvc/nis.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <admldb_impl.h>
#include <admldb_msgs.h>
#include "nis_plus_ufs_policy.h"

#define	DB_ERR_MSG_MAX	2048

#define	CASE_INSENSITIVE(obj, col) \
	((obj)->TA_data.ta_cols.ta_cols_val[col].tc_flags & TA_CASE)

#define	COLUMN_NUMBER(arg) (ttp->match_args.at[arg].colnum[DB_NISPLUS_INDEX])

#define	COLUMN_NAME(obj, col) ((obj)->TA_data.ta_cols.ta_cols_val[col].tc_name)

struct cb_data {
        char *fn;
        Db_error **db_err;
        struct tbl_trans_data *ttp;
        char*  tbl_name;
	Row **rlp;
	int rl_size;
	Row *mr;
	int rows;
	int status;
	ulong_t flags;
};
        
void 
db_err_set(
	Db_error **db_err,
	ulong_t db_errno,
	ulong_t dirty_flag,
	...)
{
	va_list ap;
	char *fmt;
	
	if ((*db_err = (Db_error *) malloc(sizeof(Db_error))) == NULL)
		return;
	(*db_err)->errno = db_errno;
	(*db_err)->dirty = dirty_flag;
	if (((*db_err)->msg = (char *) malloc(DB_ERR_MSG_MAX + 1)) == NULL)
		return;
	fmt = ADMLDB_MSGS(db_errno);
	va_start(ap, dirty_flag);
	(void) vsprintf((*db_err)->msg, fmt, ap);
	va_end(ap);
}

static void
add_srch_attr(
	char *buf,
	char *colname,
	char *colval)
{
	int len;
	
	if ((len = strlen(buf)) == 0)
		strcpy(buf, "[");
	else
		buf[(len - 1)] = ',';
	strcat(buf, colname);
	strcat(buf, "=");
	strcat(buf, colval);
	strcat(buf, "]");
}
     
void 
ldb_build_nis_name(
	char *search,
	char *table,
	char *domain,
	char *buff)
{
	buff[0] = '\0';
	if ((search != NULL) && (strlen(search) != 0)) {
		strcat(buff, search);
		strcat(buff, ",");
	}
	strcat(buff, table);
	if ((domain != NULL) && (strlen(domain) != 0)) {
		strcat(buff, ".");
		if (strcmp(domain, "."))
			strcat(buff, domain);
	}
}

int
ldb_do_nis_lookup(
	char *name,
	nis_result **resp,
	Db_error **db_err)
{
	*resp = nis_lookup(name, EXPAND_NAME|FOLLOW_LINKS);
	if ((NIS_RES_STATUS(*resp) != NIS_SUCCESS) && 
	    (NIS_RES_STATUS(*resp) != NIS_S_SUCCESS)) {
		db_err_set(db_err, DB_ERR_NISPLUS, ADM_FAILCLEAN, "nis_lookup", 
		    name, nis_sperrno(NIS_RES_STATUS(*resp)));
		return (-1);
	} else
		return (0);
}

/*
 * Comparison function for qsort(), normal NIS+ table formats, where first
 * column is primary key.
 */
int
compare_nisplus_col0(
	nis_object *aobj,
	nis_object *bobj)
{
	return (strcmp(ENTRY_VAL(aobj, 0), ENTRY_VAL(bobj, 0)));
}

/*
 * Comparison function for qsort(), normal NIS+ table formats, where first
 * column is primary key, and case-insensitive.
 */
int
compare_nisplus_col0_ci(
	nis_object *aobj,
	nis_object *bobj)
{
	return (strcasecmp(ENTRY_VAL(aobj, 0), ENTRY_VAL(bobj, 0)));
}

/*
 * Comparison function for qsort(), where second column is primary key, 
 * and case-insensitive.
 */
int
compare_nisplus_col1_ci(
	nis_object *aobj,
	nis_object *bobj)
{
	return (strcasecmp(ENTRY_VAL(aobj, 1), ENTRY_VAL(bobj, 1)));
}


/*
 * Comparison function for qsort().  This function ensures that the primary 
 * entry (the entry whose name == cname) is always before alias entries, which 
 * allows optimization in the coalesce loop in list_table_impl.
 */

int
compare_nisplus_aliased(
	nis_object *aobj,
	nis_object *bobj)
{
	int s;

	if (ENTRY_VAL(aobj, 0) == 0) return(-1);
	if (ENTRY_VAL(bobj, 0) == 0) return(1);
	s = strcasecmp(ENTRY_VAL(aobj, 0), ENTRY_VAL(bobj, 0));
	if (s == 0) {
	        if (ENTRY_VAL(aobj, 1) == 0) return(1);
	        if (ENTRY_VAL(bobj, 1) == 0) return(-1);
		if (!strcasecmp(ENTRY_VAL(aobj, 0), ENTRY_VAL(aobj, 1)))
			return (-1);
		else if (!strcasecmp(ENTRY_VAL(bobj, 0), ENTRY_VAL(bobj, 1)))
			return (1);
	}
	return (s);
}

/*
 * Comparison function for qsort().  This function is a modified version of 
 * compare_nisplus_aliased to deal with the fact that the "key" for
 * the NIS+ services table is really a combination of 3 columns.
 */

int
compare_nisplus_services(
	nis_object *aobj,
	nis_object *bobj)
{
	int s;

	if ((s = strcasecmp(ENTRY_VAL(aobj, 0), ENTRY_VAL(bobj, 0))) != 0)
		return (s);
	if ((s = strcasecmp(ENTRY_VAL(aobj, 2), ENTRY_VAL(bobj, 2))) != 0)
		return (s);
	if ((s = strcasecmp(ENTRY_VAL(aobj, 3), ENTRY_VAL(bobj, 3))) != 0)
		return (s);
	if (!strcasecmp(ENTRY_VAL(aobj, 0), ENTRY_VAL(aobj, 1)))
		return (-1);
	else if (!strcasecmp(ENTRY_VAL(bobj, 0), ENTRY_VAL(bobj, 1)))
		return (1);
}

/*
 * Function to do the special formatting of a netgroup member list for
 * the NIS+ table.
 */
static char *
construct_netgroup_member(nis_object *eobj)
{
	static char buff[NIS_MAXATTRVAL];

	if ((ENTRY_VAL(eobj, 1) != NULL) && (ENTRY_LEN(eobj, 1) != 0))
		return (ENTRY_VAL(eobj, 1));
	else {
		strcpy(buff, "(");
		if (ENTRY_VAL(eobj, 2) != NULL)
			strcat(buff, ENTRY_VAL(eobj, 2));
		strcat(buff, ",");
		if (ENTRY_VAL(eobj, 3) != NULL)
			strcat(buff, ENTRY_VAL(eobj, 3));
		strcat(buff, ",");
		if (ENTRY_VAL(eobj, 4) != NULL)
			strcat(buff, ENTRY_VAL(eobj, 4));
		strcat(buff, ")");
		return (buff);
	}
}

/*
 * Function to handle the per-entry callbacks from yp_all().
 */
static int 
yp_cb(
	int instatus,
	char *inkey,
	int inkeylen,
	char *inval,
	int invallen,
	struct cb_data *cbdp)
{
        Row *rp;
        int i;
        char *sp;
	int found = 0;

	if (instatus != YP_TRUE) {
		if ((cbdp->status = ypprot_err(instatus)) != YPERR_NOMORE) {
		        if (cbdp->status == YPERR_MAP)
		                db_err_set(cbdp->db_err, DB_ERR_NO_TABLE, 
				    ADM_FAILCLEAN, cbdp->fn, cbdp->tbl_name);
			else
			        db_err_set(cbdp->db_err, DB_ERR_IN_LIB, 
				    ADM_FAILCLEAN, cbdp->fn, "yp_all", 
				    yperr_string(cbdp->status));
			for (i = 0; i < cbdp->rows; ++i)
			        free_row(cbdp->rlp[i]);
			free(cbdp->rlp);
			cbdp->status = -1;
			return (1);
		} else {
			cbdp->status = 0;
			return (1);
		}
	}
	
	if (cbdp->ttp->fmts[DB_NIS_INDEX].special) {
	        int numchars = 
		  inkeylen + invallen + strlen(cbdp->ttp->tn.column_sep) + 1;
		sp = (char *)calloc(numchars, sizeof(char));
	}
	else
		sp = (char *)calloc((invallen + 1), sizeof(char));
	if (sp == NULL) {
		db_err_set(cbdp->db_err, DB_ERR_NO_MEMORY,
		    ADM_FAILCLEAN, "yp_cb");
		for (i = 0; i < cbdp->rows; ++i)
			free_row(cbdp->rlp[i]);
		free(cbdp->rlp);
		return (1);
	}
	if (cbdp->ttp->fmts[DB_NIS_INDEX].special) {
		strncpy(sp, inkey, inkeylen);
		strcat(sp, cbdp->ttp->tn.column_sep);
		strncat(sp, inval, invallen);
	} else {
		strncpy(sp, inval, invallen);
	}
	if ((cbdp->status = _parse_db_buffer(sp, &rp, 
	    cbdp->ttp->tn.column_sep, cbdp->ttp->tn.comment_sep, 
	    &cbdp->ttp->fmts[DB_NIS_INDEX], cbdp->ttp->type)) > 0) {
	    	if ((cbdp->flags & DB_LIST_SINGLE)) {
	    		if (_match_entry(cbdp->mr, rp, 
			    &cbdp->ttp->fmts[DB_NIS_INDEX]) == EXACT_MATCH) {
	    			found = 1;
	    		} else {
	    			free_row(rp);
	    		}
	    	} else
			found = 1;
		if (found && 
		    ((cbdp->rlp == NULL) || (cbdp->rows == cbdp->rl_size))) {
			cbdp->rl_size = cbdp->rl_size * 2;
			if ((cbdp->rlp = (Row **) realloc(cbdp->rlp,
			    (cbdp->rl_size * sizeof(Row *)))) == NULL) {
				db_err_set(cbdp->db_err, DB_ERR_NO_MEMORY, 
				    ADM_FAILCLEAN, "yp_cb");
				free(cbdp->rlp);
				free(sp);
				return (1);
			}
		}
		if (found)
			cbdp->rlp[cbdp->rows++] = rp;
		cbdp->status = 0;			        
	} else if (cbdp->status < 0) {
		db_err_set(cbdp->db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN, 
		    "yp_cb");
		for (i = 0; i < cbdp->rows; ++i)
		        free_row(cbdp->rlp[i]);
		free(cbdp->rlp);
		free(sp);
		return (1);
	}
	free(sp);
	if ((cbdp->flags & DB_LIST_SINGLE) && found)
		return (1);
	else
		return (0);
}


/*
 * Function to list a table whose data appears as multiple NIS+ entries because 
 * of aliasing.
 */

int
list_table_impl(
	ulong_t ns_mask,
	char *host,
	char *domain,
	ulong_t flags,
	Db_error **db_err,
	Table *tbl,
	char **iargs,
	char ***oargs,
	struct tbl_trans_data *ttp,
	int action,
	uid_t *uidp)
{
	int status;
	Row *rp, *srp = NULL;
	Column *cp;
	u_int i, j, k;
	nis_object *eobj, *tobj, *nobj;
	nis_result *tres, *eres;
	char zname[NIS_MAXNAMELEN], zbuf[NIS_MAXNAMELEN];
	struct tbl_fmt *tfp;
	struct cb_data cbd;
	struct ypall_callback ypa_cb;
	char *sp = NULL;
	Table *stbl = NULL;
	extern struct tbl_trans_data shadow_trans;
        int (*null_fun)() = NULL;
	int cn, alias_search = 0;
	char *xargs[9];
	char* shadow_error_string;

	if (!(flags & DB_LIST_SINGLE))
		free_tdh(tbl);
	
	if ((ns_mask & DB_NS_UFS)) {
		if ((ttp->tn.ufs == NULL) && (ns_mask == DB_NS_UFS)) {
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    ADM_FAILCLEAN, "list_table_impl", "UFS");
			return (-1);
		}

	        /*
	         * Retrieve the data, then go through each row & change each 
		 * column from numbered to named.
	         */
		if ((tbl->type == DB_PASSWD_TBL) && (flags & DB_LIST_SHADOW))
			flags |= DB_SORT_LIST;
	        if (list_ufs_db(flags, db_err, tbl, iargs, oargs, ttp) != 0) {
			if (ns_mask == DB_NS_UFS)
		                return (-1);
			else {
				db_err_free(db_err);
				goto nis;
			}
		}
		if ((tbl->type == DB_PASSWD_TBL) && (flags & DB_LIST_SHADOW)) {
			stbl = table_of_type(DB_SHADOW_TBL);
			if ((flags & DB_LIST_SINGLE)) {
				if (lcl_list_table(DB_NS_UFS, host, domain,
				    flags, db_err, stbl, *oargs[0], &xargs[0],
				    &xargs[1], &xargs[2], &xargs[3], &xargs[4],
				    &xargs[5], &xargs[6], &xargs[7], &xargs[8])
				    < 0) {
					free_table(stbl);
					if (ns_mask == DB_NS_UFS)
						return (-1);
					else {
						db_err_free(db_err);
						goto nis;
					}
				}
			} else {
				if (lcl_list_table(DB_NS_UFS, host, domain,
				    flags, db_err, stbl) < 0) {
					free_table(stbl);
					return (-1);
				}
			}
			if ((flags & DB_LIST_SINGLE)) {
				if (oargs[1] != NULL)
					*oargs[1] = xargs[1];
				for (i = 2; i < 9; ++i)
					if (oargs[(i + 5)] != NULL)
						*oargs[(i + 5)] = xargs[i];
			} else
				srp = stbl->tdh->start;
		}
		if ((flags & DB_LIST_SINGLE))
			return (0);
		else if (tbl->tdh == NULL)
			return (0);
		for (rp = tbl->tdh->start; rp != NULL; rp = rp->next) {
			for (cp = rp->start; cp != NULL; cp = cp->next) {
			        if ((tbl->type == DB_SERVICES_TBL) &&
			            (cp->up->num == 1)) {
					if ((sp = strchr(cp->val, '/')) != NULL)
						*sp++ = '\0';
				}
				cp->name = ttp->fmts[DB_UFS_INDEX].data_cols[cp->up->num].param;
				free(cp->up);
				cp->up = NULL;
			}
			if (sp != NULL) {
				if (set_out_val(rp, DB_PROTOCOL_NAME_PAR, sp) 
				    != 0) {
					db_err_set(db_err, DB_ERR_NO_MEMORY, 
					    ADM_FAILCLEAN, "list_table_impl");
					free_tdh(tbl);
					return (-1);
				}
				sp = NULL;
			}
			if ((tbl->type == DB_PASSWD_TBL) && 
			    (flags & DB_LIST_SHADOW)) {
				while ((srp != NULL) && ((status = 
				    strcmp(rp->start->val, srp->start->val)) 
				    > 0))
					srp = srp->next;
				if ((srp != NULL) && (status == 0)) {
					free_column(srp, srp->start);
					rp->end->next = srp->start;
					srp->start->prev = rp->end;
					rp->end = srp->end;
					srp = srp->next;
				}
			}
		}
		if (stbl != NULL) {
			stbl->tdh->start = stbl->tdh->end = NULL;
			free_table(stbl);
		}
		
	}
nis:
	if ((ns_mask & DB_NS_NIS)) {
		if ((ttp->tn.nis == NULL) && (ns_mask == DB_NS_NIS)) {
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    ADM_FAILCLEAN, "list_table_impl", "NIS");
			return (-1);
		}

		  
	        /*
	         * Get the domain if not supplied.
	         */
		if (((domain == NULL) || !strlen(domain)) && 
		    ((status = yp_get_default_domain(&domain)) != 0)) {
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
			    "list_table_impl", "yp_get_default_domain", 
			    yperr_string(status));
			return (-1);
		}
		/*
		 * Set up & call yp_all to do the data retrieval
		 */
		cbd.fn = "list_table_impl";
		cbd.db_err = db_err;
		cbd.ttp = ttp;
		cbd.rlp = NULL;
		cbd.rl_size = 500;
		cbd.rows = cbd.status = 0;
		cbd.flags = flags;
		cbd.tbl_name = tbl->tn.nis;
		ypa_cb.foreach = &yp_cb;
		ypa_cb.data = (char *)&cbd;
		if ((flags & DB_LIST_SINGLE)) {
			if ((cbd.mr = new_row()) == NULL) {
				db_err_set(db_err, DB_ERR_NO_MEMORY,
				    ADM_FAILCLEAN, "list_table_impl");
				return (-1);
			}
			for (i = 0; i < ttp->match_args.cnt; ++i) {
				if ((iargs[i] == NULL) || !strlen(iargs[i]))
					continue;
				if ((tbl->type == DB_SERVICES_TBL) &&
				    (i == 1)) {
					if ((sp = (char *)malloc((strlen(iargs[1]) + strlen(iargs[2]) + 2))) == NULL) {
						db_err_set(db_err, 
						    DB_ERR_NO_MEMORY, 
						    ADM_FAILCLEAN,
						    "list_table_impl");
						free_row(cbd.mr);
						return(-1);
					}
					sprintf(sp, "%s/%s", iargs[1], 
					    iargs[2]);
					iargs[1] = sp;
					iargs[2] = NULL;
				}
				cn = ttp->match_args.at[i].colnum[DB_NIS_INDEX];
				if (new_numbered_column(cbd.mr, cn, NULL,
				    iargs[i], 
				    ttp->fmts[DB_NIS_INDEX].data_cols[cn].case_flag) 
				    == NULL) {
					db_err_set(db_err, DB_ERR_NO_MEMORY, 
					    ADM_FAILCLEAN, "list_table_impl");
					free_row(cbd.mr);
					return (-1);
				}
			}
		} else
			cbd.mr = NULL;
		if ((status = yp_all(domain, tbl->tn.nis, &ypa_cb)) != 0) {
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
			    "list_table_impl", "yp_all", yperr_string(status));
			return (-1);
		} else if (cbd.status != 0) {
			if (ns_mask == DB_NS_NIS)
			        return (cbd.status);
			else {
				db_err_free(db_err);
				goto nisplus;
			}
		}
		/*
		 * Sort the data if requested, then stuff rows into table 
		 * structure, changing columns from numbered to named 
		 * along the way.
		 */
		if ((flags & DB_LIST_SINGLE) && (cbd.rlp == NULL)) {
			if (ns_mask != DB_NS_NIS)
				goto nisplus;
			for (i = 0; i < ttp->match_args.cnt; ++i)
				if ((iargs[i] != NULL) && strlen(iargs[i]))
					break;
			db_err_set(db_err, DB_ERR_NO_ENTRY, ADM_FAILCLEAN, 
			    "list_table_impl", iargs[i], tbl->tn.nis);
			free_row(cbd.mr);
			return (-1);
		}

		if ((flags & DB_SORT_LIST))
		        qsort(cbd.rlp, cbd.rows, sizeof(Row *), 
		            ttp->fmts[DB_NIS_INDEX].sort_function);

		for (i = 0; i < cbd.rows; ++i) {
			if (!(flags & DB_LIST_SINGLE) && 
			    (append_row(tbl, cbd.rlp[i]) != 0)) {
				db_err_set(db_err, DB_ERR_NO_MEMORY, 
				    ADM_FAILCLEAN, "list_table_impl");
				for (j = 0; j < cbd.rows; ++j)
				        free_row(cbd.rlp[j]);
				free(cbd.rlp);
				free_tdh(tbl);
				return (-1);
			}
			for (cp = cbd.rlp[i]->start; cp != NULL; cp = cp->next) {
			        if ((tbl->type == DB_SERVICES_TBL) &&
			            (cp->up->num == 1)) {
					if ((sp = strchr(cp->val, '/')) != NULL)
						*sp++ = '\0';
				}
				cp->name = ttp->fmts[DB_NIS_INDEX].data_cols[cp->up->num].param;
				free(cp->up);
				cp->up = NULL;
			}
			if (sp != NULL) {
				if (set_out_val(cbd.rlp[i], 
				    DB_PROTOCOL_NAME_PAR, sp) != 0) {
					    db_err_set(db_err, DB_ERR_NO_MEMORY,
						ADM_FAILCLEAN, "list_table_impl");
					for (j = 0; j < cbd.rows; ++j)
						free_row(cbd.rlp[j]);
					free(cbd.rlp);
					free_tdh(tbl);
					return (-1);
				}
				sp = NULL;
			}
		}
		if ((flags & DB_LIST_SINGLE)) {
			for (cp = cbd.rlp[0]->start; cp != NULL; cp = cp->next)
				for (i = 0; i < ttp->io_args.cnt; ++i)
					if ((oargs[i] != NULL) && 
					    (cp->name != NULL) &&
					    !strcmp(cp->name, 
					    ttp->io_args.at[i].name)) 
						*oargs[i] = strdup(cp->val);
			free(cbd.rlp);
	
			/*
			   The following code was borrowed from the UFS db
			   code for the case when there is a NIS shadow map. 
			   Since dbmgr no longer refers to the passwd table 
			   we do not need to enumerate the shadow table, only
			   fetch single rows
			*/
			   

			if (tbl->type == DB_PASSWD_TBL &&
			    (flags & DB_LIST_SHADOW) &&
			    (flags & DB_LIST_SINGLE) &&
			    shadow_map_exists(&shadow_error_string, domain)) {
			    
			    stbl = table_of_type(DB_SHADOW_TBL);

			    if (lcl_list_table(DB_NS_NIS, host, domain, flags,
					   db_err, stbl, *oargs[0], &xargs[0],
					   &xargs[1], &xargs[2], &xargs[3],
					   &xargs[4], &xargs[5],
				   &xargs[6], &xargs[7], &xargs[8]) < 0) {
				free_table(stbl);
				if (ns_mask == DB_NS_NIS)
				  return (-1);
				else {
				    db_err_free(db_err);
				    goto nisplus;
				}
			    }

			    if (oargs[1] != NULL)
			      *oargs[1] = xargs[1];
			    for (i = 2; i < 9; ++i)
			      if (oargs[(i + 5)] != NULL)
				*oargs[(i + 5)] = xargs[i];
			} 

		    /*  end of borrowed UFS code */

		

			return (0);
		} else {
			free(cbd.rlp);
		}
	}
nisplus:
	if ((ns_mask & DB_NS_NISPLUS)) {
		if (ttp->tn.nisplus == NULL) {
			if (ns_mask != DB_NS_NISPLUS)
				goto end;
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    ADM_FAILCLEAN, "list_table_impl", "NIS+");
			return (-1);
		}

		/*
		 * If we are being run as a method, we need to set-uid to the
		 * client in order to allow the NIS+ security to do its stuff.
		 */
		if (uidp != NULL)
			setuid(*uidp);

	        tfp = &ttp->fmts[DB_NISPLUS_INDEX];
	        
		ldb_build_nis_name(NULL, tbl->tn.nisplus, domain, zname);
		/*
		 * Get the table object using expand name magic.
		 */
		if (ldb_do_nis_lookup(zname, &tres, db_err) != 0) {
		    if ((NIS_RES_STATUS(tres) == NIS_NOSUCHNAME) ||
			(NIS_RES_STATUS(tres) == NIS_NOSUCHTABLE) ||
			(NIS_RES_STATUS(tres) == NIS_NOTFOUND) )
		        return (DB_TABLE_NOTFND);
		    else
		        return (DB_TABLE_ERROR);
		}

		tobj = NIS_RES_OBJECT(tres);

		zbuf[0] = '\0';
		if ((flags & DB_LIST_SINGLE))
			for (i = 0; i < ttp->match_args.cnt; ++i) 
				if ((iargs[i] != NULL) &&
				    strlen(iargs[i])) {
					sp = iargs[i];
					cn = COLUMN_NUMBER(i);
					if (cn == tfp->alias_col)
						alias_search = 1;
					add_srch_attr(zbuf, 
					    COLUMN_NAME(tobj, cn), iargs[i]);
				}
		/*
		 * Construct the name for the table that we found.
		 */
		do_nis_list:
		ldb_build_nis_name(zbuf, tobj->zo_name, tobj->zo_domain, zname);
#ifdef PERF_DEBUG
timestamp("Started nis_list", "", TS_NOELAPSED);
#endif
		eres = nis_list(zname, 
				MASTER_ONLY|FOLLOW_LINKS|EXPAND_NAME, 
				null_fun, 
				(void *) NULL);

		if ((NIS_RES_STATUS(eres) != NIS_NOTFOUND) &&
		    (NIS_RES_STATUS(eres) != NIS_SUCCESS) &&
		    (NIS_RES_STATUS(eres) != NIS_S_SUCCESS)) {
			db_err_set(db_err, DB_ERR_NISPLUS, ADM_FAILCLEAN, 
			    "nis_list", zname, 
			    nis_sperrno(NIS_RES_STATUS(eres)));
			return (-1);
		} else if ((NIS_RES_STATUS(eres) == NIS_NOTFOUND) &&
		    (flags & DB_LIST_SINGLE)) {
			db_err_set(db_err, DB_ERR_NO_ENTRY, ADM_FAILCLEAN,
			    "list_table_impl", sp, tobj->zo_name);
			return (-1);
		}
#ifdef PERF_DEBUG
timestamp("Completed nis_list", "", TS_NOELAPSED);
#endif
		/*
		 * If we're doing a single list & the first search was on an alias,
		 * now search on the real thing.
		 */
		if ((flags & DB_LIST_SINGLE) && alias_search) {
			zbuf[0] = '\0';
			for (i = 0; i < tfp->cnt; ++i) {
				if (tfp->data_cols[i].key) {
					add_srch_attr(zbuf, 
					    COLUMN_NAME(tobj, i), 
					    ENTRY_VAL(NIS_RES_OBJECT(eres), i));
				}
			}
			nis_freeresult(eres);
			alias_search = 0;
			goto do_nis_list;
		}
			
		eobj = NIS_RES_OBJECT(eres);
		if (tfp->special || (flags & DB_SORT_LIST)) {
			qsort(eobj, NIS_RES_NUMOBJ(eres), sizeof(nis_object), 
			    ttp->fmts[DB_NISPLUS_INDEX].sort_function);
#ifdef PERF_DEBUG
timestamp("Completed quicksort", "", TS_NOELAPSED);
#endif
		}
		i = 0;
		while (i < NIS_RES_NUMOBJ(eres)) {
			if ((rp = new_row()) == NULL) {
				db_err_set(db_err, DB_ERR_NO_MEMORY, 
				    ADM_FAILCLEAN, "list_table_impl");
				nis_freeresult(eres);
				nis_freeresult(tres);
				free_tdh(tbl);
				return (-1);
			}

			rp->tri              = new_tri();
			rp->tri->domain      = strdup(eobj->zo_domain);
			rp->tri->owner       = strdup(eobj->zo_owner);
			rp->tri->group_owner = strdup(eobj->zo_group);
			rp->tri->permissions = eobj->zo_access;
			rp->tri->ttl         = eobj->zo_ttl;
			
			if (tbl->type == DB_NETGROUP_TBL) {
				if (set_out_val(rp, tfp->data_cols[0].param,
				    ENTRY_VAL(eobj, 0)) != 0) {
					db_err_set(db_err, DB_ERR_NO_MEMORY, 
					    ADM_FAILCLEAN, "list_table_impl");
					nis_freeresult(eres);
					nis_freeresult(tres);
					free_tdh(tbl);
					return (-1);
				}
				if (set_out_val(rp, tfp->data_cols[1].param,
				    construct_netgroup_member(eobj)) != 0) {
					db_err_set(db_err, DB_ERR_NO_MEMORY, 
					    ADM_FAILCLEAN, "list_table_impl");
					nis_freeresult(eres);
					nis_freeresult(tres);
					free_tdh(tbl);
					return (-1);
				}
				if (set_out_val(rp, 
				    tfp->data_cols[tfp->comment_col].param, 
				    ENTRY_VAL(eobj, tfp->comment_col)) != 0) {
					db_err_set(db_err, DB_ERR_NO_MEMORY, 
					    ADM_FAILCLEAN, "list_table_impl");
					nis_freeresult(eres);
					nis_freeresult(tres);
					free_tdh(tbl);
					return (-1);
				}
			} else for (k = 0; k < tfp->cnt; ++k) {
				if (k == tfp->alias_col)
					continue;
				if ((tbl->type == DB_PASSWD_TBL) && (k == 7)) {
					/* Handle shadow column specially */
					if ((flags & DB_LIST_SHADOW)) {
						if (ENTRY_VAL(eobj, k) == NULL)
							continue;
						if (_parse_db_buffer(
						    ENTRY_VAL(eobj, k),
						    &srp, ":", NULL, 
						    &shadow_trans.fmts[DB_NISPLUS_INDEX],
						    shadow_trans.type)
						    < 0) {
						    	db_err_set(db_err, 
						    	    DB_ERR_NO_MEMORY,
						    	    ADM_FAILCLEAN, 
						    	    "list_table_impl");
							nis_freeresult(eres);
							nis_freeresult(tres);
							free_tdh(tbl);
							return (-1);
						}
						for (cp = srp->start; 
						    cp != NULL;
						    cp = cp->next) {
							cp->name = shadow_trans.fmts[DB_NISPLUS_INDEX].data_cols[cp->up->num].param;
							free(cp->up);
							cp->up = NULL;
						}
						rp->end->next = srp->start;
						srp->start->prev = rp->end;
						rp->end = srp->end;
						free(srp);
						srp = NULL;
					} else
						continue;
				}
				if (set_out_val(rp, tfp->data_cols[k].param,
				    ENTRY_VAL(eobj, k)) != 0) {
					db_err_set(db_err, DB_ERR_NO_MEMORY, 
					    ADM_FAILCLEAN, "list_table_impl");
					nis_freeresult(eres);
					nis_freeresult(tres);
					free_tdh(tbl);
					return (-1);
				}
			}

			for ((nobj = eobj + 1), (j = i + 1);
			    (tfp->special && (j < NIS_RES_NUMOBJ(eres)));
			    ++j, ++nobj) {
				for (k = 0; k < tfp->cnt; ++k)
				        if (tfp->data_cols[k].key) {
						if ((ENTRY_VAL(eobj, k) == 0) ||
						    (ENTRY_VAL(nobj, k) == 0)) break;
						if (CASE_INSENSITIVE(tobj, k) &&
						    strcasecmp(
						      ENTRY_VAL(eobj, k), 
					              ENTRY_VAL(nobj, k)))
							break;
						else if (strcmp(
						    ENTRY_VAL(eobj, k), 
					            ENTRY_VAL(nobj, k)))
							break;
					}
				if (k != tfp->cnt)
				        break;
				if (tbl->type == DB_NETGROUP_TBL) {
					if (set_out_val(rp, 
					    tfp->data_cols[1].param,
					    construct_netgroup_member(nobj)) 
					    != 0) {
						db_err_set(db_err, 
						    DB_ERR_NO_MEMORY, 
						    ADM_FAILCLEAN, 
						    "list_table_impl");
						nis_freeresult(eres);
						nis_freeresult(tres);
						free_tdh(tbl);
						return (-1);
					}
				} else for (k = 0; k < tfp->cnt; ++k) {
					if (k == tfp->comment_col)
						continue;
					else if (tfp->data_cols[k].key)
						continue;
					else if (set_out_val(rp, 
					    tfp->data_cols[k].param,
					    ENTRY_VAL(nobj, k)) != 0) {
						db_err_set(db_err, 
						    DB_ERR_NO_MEMORY,
						    ADM_FAILCLEAN, 
						    "list_table_impl");
						nis_freeresult(eres);
						nis_freeresult(tres);
						free_tdh(tbl);
						return (-1);
					}
				}
			}
			if ((flags & DB_LIST_SINGLE)) {
				for (i = 0; i < ttp->io_args.cnt; ++i) {
					if (oargs[i] != NULL) {
						for (cp = rp->start; cp != NULL;
						    cp = cp->next) {
							if (!strcmp(cp->name, 
							    ttp->io_args.at[i].name)) {
								*oargs[i] = strdup(cp->val);
								break;
							}
						}
					}
				}
			}
			if (append_row(tbl, rp) != 0) {
				db_err_set(db_err, DB_ERR_NO_MEMORY, 
				    ADM_FAILCLEAN, "list_table_impl");
				nis_freeresult(eres);
				nis_freeresult(tres);
				free_tdh(tbl);
				return (-1);
			}
			i = j;
			eobj = nobj;
		}
		
		nis_freeresult(eres);
		nis_freeresult(tres);
#ifdef PERF_DEBUG
timestamp("Completed coalesce & arg return", "", TS_NOELAPSED);
#endif
	}
end:
	if ((flags & DB_LIST_SINGLE)) {
		return (0);
	} else if (tbl->tdh == NULL)
		return (0);
	else
		return (tbl->tdh->rows);
}

/*
 * Macro to set the value of an NIS+ table column.
 */
#define	set_col_val(e, cn, val) {						\
	e->EN_data.en_cols.en_cols_val[cn].ec_value.ec_value_val = val;		\
	e->EN_data.en_cols.en_cols_val[cn].ec_value.ec_value_len =		\
	    strlen(val) + 1;							\
}

static void
set_netgroup_val(
	nis_object *eobj,
	char *sp)
{
	char *cp;

	if (*sp != '(') {	/* recursive entry */
		set_col_val(eobj, 1, sp);
	} else {	/* regular host, user, domain triple */
		cp = strrchr(sp, ')');
		*cp = '\0';
		cp = strrchr(sp, ',');
		set_col_val(eobj, 4, (cp + 1));
		*cp = '\0';
		cp = strrchr(sp, ',');
		set_col_val(eobj, 3, (cp + 1));
		*cp = '\0';
		cp = sp + 1;
		set_col_val(eobj, 2, cp);
	}
}

/* 
 * Hack to avoid compiler warnings due to missing declaration in string.h
 * Remove when bug 1098475 is fixed.
 */
extern int strcasecmp(const char *, const char *);

/*
 * Function to do comparison of an entry to match criteria.
 */
static int 
same_keys(
	struct tbl_trans_data *ttp,
	char **iargs,
	char ***rargs)
{
	int i, j;
	int cn;
	int (*cmp)(const char *, const char *);
	int index = DB_UFS_INDEX;

	for (i = 0; i < ttp->match_args.cnt; ++i) {
		if (iargs[i] == NULL)
			continue;
		if ((ttp->type == DB_SERVICES_TBL) && (i == 2))
			index = DB_NISPLUS_INDEX;
		cn = ttp->match_args.at[i].colnum[index];
		for (j = 0; j < ttp->io_args.cnt; ++j) {
			if ((rargs[j] != NULL) && (*rargs[j] != NULL) &&
			    !strcmp(ttp->fmts[index].data_cols[cn].param, 
			    ttp->io_args.at[j].name)) {
			    	if (ttp->fmts[index].data_cols[cn].case_flag)
			    		cmp = strcasecmp;
			    	else
			    		cmp = strcmp;
			    	if ((*cmp)(iargs[i], *rargs[j]))
			    		return (0);
			}
		}
	}
	return (1);
}

/*
 * Function to compare a NIS+ entry object to match criteria.
 */
static int
same_nisplus_keys(
	struct tbl_trans_data *ttp,
	char **iargs,
	nis_result *tres,
	nis_result *eres)
{
	int i;
	int (*cmp)(const char *, const char *);
	
	for (i = 0; i < ttp->match_args.cnt; ++i) {
		if (iargs[i] == NULL)
			continue;
		if (CASE_INSENSITIVE(NIS_RES_OBJECT(tres), COLUMN_NUMBER(i)))
			cmp = strcasecmp;
		else
			cmp = strcmp;
			
		if ((*cmp)(ENTRY_VAL(NIS_RES_OBJECT(eres), COLUMN_NUMBER(i)), iargs[i]))
			return (0);
	}
	return (1);
}

#define	NIS_ALL_ACC (NIS_READ_ACC|NIS_MODIFY_ACC|NIS_CREATE_ACC|NIS_DESTROY_ACC)
static int
parse_rights_field(rights, shift, p)
	u_long *rights;
	int shift;
	char *p;
{
	int set;

	while (*p && (*p != ',')) {
		switch (*p) {
		case '=':
			*rights &= ~(NIS_ALL_ACC << shift);
		case '+':
			set = 1;
			break;
		case '-':
			set = 0;
			break;
		default:
			return (0);
		}
		for (p++; *p && (*p != ',') && (*p != '=') && (*p != '+') &&
							(*p != '-'); p++) {
			switch (*p) {
			case 'r':
				if (set)
					*rights |= (NIS_READ_ACC << shift);
				else
					*rights &= ~(NIS_READ_ACC << shift);
				break;
			case 'm':
				if (set)
					*rights |= (NIS_MODIFY_ACC << shift);
				else
					*rights &= ~(NIS_MODIFY_ACC << shift);
				break;
			case 'c':
				if (set)
					*rights |= (NIS_CREATE_ACC << shift);
				else
					*rights &= ~(NIS_CREATE_ACC << shift);
				break;
			case 'd':
				if (set)
					*rights |= (NIS_DESTROY_ACC << shift);
				else
					*rights &= ~(NIS_DESTROY_ACC << shift);
				break;
			default:
				return (0);
			}
		}
	}
	return (1);
}

#define	NIS_NOBODY_FLD 1
#define	NIS_OWNER_FLD 2
#define	NIS_GROUP_FLD 4
#define	NIS_WORLD_FLD 8
#define	NIS_ALL_FLD NIS_OWNER_FLD|NIS_GROUP_FLD|NIS_WORLD_FLD

static int
parse_rights(rights, p)
	u_long *rights;
	char *p;
{
	u_long f;

	if (p)
		while (*p) {
			for (f = 0; (*p != '=') && (*p != '+') && (*p != '-');
									p++)
				switch (*p) {
				case 'n':
					f |= NIS_NOBODY_FLD;
					break;
				case 'o':
					f |= NIS_OWNER_FLD;
					break;
				case 'g':
					f |= NIS_GROUP_FLD;
					break;
				case 'w':
					f |= NIS_WORLD_FLD;
					break;
				case 'a':
					f |= NIS_ALL_FLD;
					break;
				default:
					return (0);
				}
			if (f == 0)
				f = NIS_ALL_FLD;

			if ((f & NIS_NOBODY_FLD) &&
			    !parse_rights_field(rights, 24, p))
				return (0);

			if ((f & NIS_OWNER_FLD) &&
			    !parse_rights_field(rights, 16, p))
				return (0);

			if ((f & NIS_GROUP_FLD) &&
			    !parse_rights_field(rights, 8, p))
				return (0);

			if ((f & NIS_WORLD_FLD) &&
			    !parse_rights_field(rights, 0, p))
				return (0);

			while (*(++p))
				if (*p == ',') {
					p++;
					break;
				}
		}
	return (1);
}


static int
parse_flags(flags, p)
	u_long *flags;
	char *p;
{
	if (p) {
		while (*p) {
			switch (*(p++)) {
			case 'B':
				*flags |= TA_BINARY;
				break;
			case 'X':
				*flags |= TA_XDR;
				break;
			case 'S':
				*flags |= TA_SEARCHABLE;
				break;
			case 'I':
				*flags |= TA_CASE;
				break;
			case 'C':
				*flags |= TA_CRYPT;
				break;
			default:
				return (0);
			}
		}
		return (1);
	} else {
		fprintf(stderr,
	"Invalid table schema: At least one column must be searchable.\n");
		exit(1);
	}
}


static int
parse_time(time, p)
	u_long *time;
	char *p;
{
	char *s;
	u_long x;

	*time = 0;

	if (p)
		while (*p) {
			if (!isdigit(*p))
				return (0);
			x = strtol(p, &s, 10);
			switch (*s) {
			case '\0':
				(*time) += x;
				p = s;
				break;
			case 's':
			case 'S':
				(*time) += x;
				p = s+1;
				break;
			case 'm':
			case 'M':
				(*time) += x*60;
				p = s+1;
				break;
			case 'h':
			case 'H':
				(*time) += x*(60*60);
				p = s+1;
				break;
			case 'd':
			case 'D':
				(*time) += x*(24*60*60);
				p = s+1;
				break;
			default:
				return (0);
			}
		}

	return (1);
}


static int
nis_getsubopt(optionsp, tokens, sep, valuep)
	char **optionsp;
	char * const *tokens;
	const int sep; /* if this is a char we get an alignment error */
	char **valuep;
{
	register char *s = *optionsp, *p, *q;
	register int i, optlen;

	*valuep = NULL;
	if (*s == '\0')
		return (-1);
	q = strchr(s, (char)sep);	/* find next option */
	if (q == NULL) {
		q = s + strlen(s);
	} else {
		*q++ = '\0';		/* mark end and point to next */
	}
	p = strchr(s, '=');		/* find value */
	if (p == NULL) {
		optlen = strlen(s);
		*valuep = NULL;
	} else {
		optlen = p - s;
		*valuep = ++p;
	}
	for (i = 0; tokens[i] != NULL; i++) {
		if ((optlen == strlen(tokens[i])) &&
		    (strncmp(s, tokens[i], optlen) == 0)) {
			/* point to next option only if success */
			*optionsp = q;
			return (i);
		}
	}
	/* no match, point value at option and return error */
	*valuep = s;
	return (-1);
}


static nis_object nis_default_obj;

static char *nis_defaults_tokens[] = {
	"owner",
	"group",
	"access",
	"ttl",
	0
};

#define	T_OWNER 0
#define	T_GROUP 1
#define	T_ACCESS 2
#define	T_TTL 3

static int
nis_defaults_set(optstr)
	char *optstr;
{
	char str[1024], *p, *v;
	int i;

	strcpy(str, optstr);
	p = str;

	while ((i = nis_getsubopt(&p, nis_defaults_tokens, ':', &v)) != -1) {
		switch (i) {
		case T_OWNER:
			if (v == 0 || v[strlen(v)-1] != '.')
				return (0);
			nis_default_obj.zo_owner = strdup(v);
			break;
		case T_GROUP:
			if (v == 0 || v[strlen(v)-1] != '.')
				return (0);
			nis_default_obj.zo_group = strdup(v);
			break;
		case T_ACCESS:
			if ((v == 0) ||
			    (!parse_rights(&(nis_default_obj.zo_access), v)))
				return (0);
			break;
		case T_TTL:
			if ((v == 0) ||
			    !(parse_time(&(nis_default_obj.zo_ttl), v)))
				return (0);
			break;
		}
	}

	if (*p)
		return (0);

	return (1);
}


static int
nis_defaults_init(nisres, create_flag, optstr)
	nis_result *nisres;
	ulong_t      create_flag;
	char       *optstr;
{
	char       *envstr;
	nis_object *entry_obj;

	/* XXX calling this multiple times may leak memory */
	memset((char *)&nis_default_obj, 0, sizeof (nis_default_obj));

	if ((create_flag & DB_MODIFY) &&
	    (NIS_RES_STATUS(nisres) == NIS_SUCCESS) ||
	    (NIS_RES_STATUS(nisres) == NIS_S_SUCCESS)) {

	    /*
	    ** Copy/save the existing object information into the default
	    ** structure so that it can be used later.  
	    */
	    entry_obj = NIS_RES_OBJECT(nisres);

            /*
            **  Allocate space for the fields since entry_obj can be
            **  deallocated.
            */
	    nis_default_obj.zo_owner = strdup(entry_obj->zo_owner);
            nis_default_obj.zo_group = strdup(entry_obj->zo_group);
            nis_default_obj.zo_access = entry_obj->zo_access;
            nis_default_obj.zo_ttl = entry_obj->zo_ttl;
            nis_default_obj.zo_domain = strdup(entry_obj->zo_domain);
	    return (DB_MODIFY);
        } else { 

	    /*
	    **  New entry.  Fill in the default structure.
	    */
	    nis_default_obj.zo_owner = nis_local_principal();
	    nis_default_obj.zo_group = nis_local_group();
	    nis_default_obj.zo_access = DEFAULT_RIGHTS;
	    nis_default_obj.zo_ttl = 12 * 60 * 60;

	    if (envstr = getenv("NIS_DEFAULTS"))
		if (!nis_defaults_set(envstr)) {
			fprintf(stderr,
			"can't parse NIS_DEFAULTS environment variable.\n");
			return (0);
		}

	    if (optstr)
		if (!nis_defaults_set(optstr)) {
			fprintf(stderr, "can't parse nis_defaults argument.\n");
			return (0);
		}

	    return ( DB_ADD );
        }

}


int
set_table_impl(
	ulong_t ns_mask,
	char *host,
	char *domain,
	ulong_t flags,
	Db_error **db_err,
	Table *tbl,
	char **iargs,
	char ***oargs,
	struct tbl_trans_data *ttp,
	int action,
	uid_t *uidp)
{
	int status;
	Table *stbl;
	int dirty_bit;
	struct tbl_fmt *tfp;
	char zname[NIS_MAXNAMELEN], zbuf[NIS_MAXNAMELEN], dbuf[NIS_MAXNAMELEN];
	nis_result *tres, *eres, *dres;
	nis_object *eobj, *tobj;
	int i, j, cn;
	char *sp, *ap, *as;
	char group_passwd[16];
	int alias_search, alias_cnt;
	int (*null_fun)() = NULL;
        int    modifyop;
	char **margs = NULL;
	char ***rargs = NULL;
	char *targs[MAX_ARGS];

	group_passwd[0] = '\0';

	if (ns_mask == DB_NS_NIS) {
	    char* shadow_error_string;
	    char* save_args[14];
	    int do_shadow = 0;

	    if (tbl->type == DB_PASSWD_TBL &&
		shadow_map_exists(&shadow_error_string, domain)) {

		char**  save_arg_p = save_args;
		char*** oarg_p     = oargs;
		char**  end_p      = save_args + 14;

		while (save_arg_p < end_p) {
		    if (*oarg_p != NULL && **oarg_p != NULL) {
			*save_arg_p = strdup(**oarg_p);
		    } else {
			*save_arg_p = NULL;
		    } 

		    save_arg_p++;
		    oarg_p++;
		}

		do_shadow          = 1;
	    }

	    status = set_nis_db(host, domain, flags, db_err, tbl, iargs, oargs,
				ttp, action, uidp);
	    if (status < 0)
	      return (-1);
	    /*
	     * Because the passwd table is tied to the shadow table, we
	     * do a call which will end up coming back through here to handle
	     * the shadow table.  Any failures at this point are dirty.
	     */
	    if (do_shadow) {
		char**  save_arg_p = save_args;
		char**  end_p      = save_args + 14;

		stbl = table_of_type(DB_SHADOW_TBL);
		status = lcl_set_table_entry(DB_NS_NIS, host, domain, 
					 flags, db_err, stbl, iargs[0], 
					 &save_args[0], &save_args[1],
					 &save_args[7], &save_args[8],
					 &save_args[9], &save_args[10],
					 &save_args[11], &save_args[12],
					 &save_args[13]);

		while (save_arg_p < end_p) {
		    if (*save_arg_p) {
			free(*save_arg_p);
		    }

		    save_arg_p++;
		}

		free_table(stbl);
		
		if (status != 0) {
		    (*db_err)->dirty = ADM_FAILDIRTY;
		    return (-1);
		}
	    }
	}

	if ((ns_mask & DB_NS_UFS)) {
		if (ttp->tn.ufs == NULL) {
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    ADM_FAILCLEAN, "set_table_impl", "UFS");
			return (-1);
		}
		tfp = &ttp->fmts[DB_UFS_INDEX];
		/*
		 * First we have to verify that new data isn't a duplicate of a
		 * different entry in the table.  Walk table's list of columns,
		 * for each key column that must be unique, call list function to
		 * check for a match against the new data we want to place in that
		 * column.  If we get a match, and it doesn't completely match the
		 * match criteria we were passed, then error.
		 */
		margs = (char **) calloc(ttp->match_args.cnt, sizeof(char *));
		if (margs == NULL) {
			db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN, "set_table_impl");
			return (-1);
		}
	
		rargs = (char ***) calloc(ttp->io_args.cnt, sizeof(char **));
		if (rargs == NULL) {
			db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
			    "set_table_impl");
			free(margs);
			return (-1);
		}
		for (i = 0; i < ttp->io_args.cnt; ++i)
			rargs[i] = &targs[i];
			
		for (i = 0; i < ttp->match_args.cnt; ++i) {
			cn = ttp->match_args.at[i].colnum[DB_UFS_INDEX];
			if (tfp->data_cols[cn].key && tfp->data_cols[cn].unique) {
				for (j = 0; j < ttp->io_args.cnt; ++j) {
					if ((oargs[j] != NULL) && (*oargs[j] != NULL) &&
					    !strcmp(ttp->io_args.at[j].name, 
					    tfp->data_cols[cn].param)) {
						margs[i] = *oargs[j];
						status = list_ufs_db(DB_LIST_SINGLE, 
						    db_err, tbl, margs,
						    rargs, ttp);
						if (status != 0) {
							if (((*db_err)->errno == 
							    DB_ERR_NO_ENTRY) || 
							    ((*db_err)->errno == 
							    DB_ERR_NO_TABLE))
								break;
							else {
								free(margs);
								free(rargs);
								return (-1);
							}
						} else {
							if (same_keys(ttp, iargs, rargs))
								break;
							else {
								db_err_set(db_err, 
								    DB_ERR_ENTRY_EXISTS,
								    ADM_FAILCLEAN,
								    "set_table_impl",
								    margs[i],
								    tbl->tn.ufs);
								free(margs);
								free(rargs);
								return (-1);
							}
						}
					}
				}
			}
			margs[i] = NULL;
		}					    
		free(margs);
		free(rargs);
		/*
		 * Call function which actually does all the work for UFS.
		 */
		if (set_ufs_db(flags, db_err, tbl, iargs, oargs, action, ttp) != 0)
			return (-1);
		/*
		 * Because the passwd table is tied to the shadow table, we
		 * do a call which will end up coming back through here to
		 * handle the shadow table.  Any failures at this point are
		 * dirty.
		 */
		if (tbl->type == DB_PASSWD_TBL) {
			stbl = table_of_type(DB_SHADOW_TBL);
			status = lcl_set_table_entry(DB_NS_UFS, host, domain, 
			    flags, db_err, stbl, iargs[0], oargs[0], 
			    oargs[1], oargs[7], oargs[8], oargs[9], 
			    oargs[10], oargs[11], oargs[12], oargs[13]);
			free_table(stbl);
			if (status != 0) {
				(*db_err)->dirty = ADM_FAILDIRTY;
				return (-1);
			}
		}
	}
	if ((ns_mask & DB_NS_NISPLUS)) {
		dirty_bit = (ns_mask == DB_NS_NISPLUS) ? ADM_FAILCLEAN:ADM_FAILDIRTY;
		if (ttp->tn.nisplus == NULL) {
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    dirty_bit, "set_table_impl", "NIS+");
			return (-1);
		}
		
		/*
		 * If we are being run as a method, we need to set-uid to the
		 * client in order to allow the NIS+ security to do its stuff.
		 */
		if (uidp != NULL)
			setuid(*uidp);

	        tfp = &ttp->fmts[DB_NISPLUS_INDEX];
	        
		ldb_build_nis_name(NULL, tbl->tn.nisplus, domain, zname);
		/*
		 * Get the table object using expand name magic.
		 */
		if (ldb_do_nis_lookup(zname, &tres, db_err) != 0) {
		    if (tbl->type == DB_POLICY_TBL) {
			if (!create_new_policy_table(domain, db_err) ||
			    ldb_do_nis_lookup(zname, &tres, db_err) != 0) {
			    return (-1);
			}
		    } else {
			return (-1);
		    }
		}

		tobj = NIS_RES_OBJECT(tres);

		zbuf[0] = '\0';
		
		/*
		 * Now construct the NIS+ indexed name for the match
		 * arguments we were passed.
		 */
		for (i = 0; i < ttp->match_args.cnt; ++i) {
			if ((iargs[i] != NULL) &&
			    strlen(iargs[i])) {
				sp = iargs[i];
				cn = COLUMN_NUMBER(i);
				add_srch_attr(zbuf, 
				    COLUMN_NAME(tobj, cn), iargs[i]);
			}
		}
		ldb_build_nis_name(zbuf, tobj->zo_name, tobj->zo_domain, zname);
		/*
		 * If this is an "aliased" table (multiple entries equal one of
		 * our Rows), we have to do a lookup to find the canonical name
		 * which links these entries together and reconstruct the indexed
		 * name so we can do the right thing below.
		 */
		eres = nis_list(zname, 
				MASTER_ONLY|FOLLOW_LINKS, 
				null_fun, 
				(void *) NULL);

		if ((NIS_RES_STATUS(eres) != NIS_SUCCESS) &&
		    (NIS_RES_STATUS(eres) != NIS_S_SUCCESS)) {
		    if (((flags & DB_ADD_MODIFY) == DB_MODIFY) &&
			(NIS_RES_STATUS(eres) == NIS_NOTFOUND)) {
			db_err_set(db_err, DB_ERR_NO_ENTRY,
				   dirty_bit, "set_table_impl",
				   sp, tobj->zo_name);
			nis_freeresult(tres);
			nis_freeresult(eres);
			return (-1);
				}
		} else if ((flags & DB_ADD_MODIFY) == DB_ADD) {
		    db_err_set(db_err, DB_ERR_ENTRY_EXISTS, dirty_bit,
			       "set_table_impl", sp, tobj->zo_name);
		    nis_freeresult(tres);
		    nis_freeresult(eres);
		    return (-1);
		} else if (tfp->alias_col >= 0) {
		    zbuf[0] = '\0';
		    for (i = 0; i < ttp->match_args.cnt; ++i) {
			if ((iargs[i] != NULL) &&
			    strlen(iargs[i])) {
			    sp = iargs[i];
			    cn = COLUMN_NUMBER(i);
			    if (cn == tfp->alias_col)
			      add_srch_attr(zbuf, 
					    COLUMN_NAME(tobj, 0), 
					    ENTRY_VAL(
						      NIS_RES_OBJECT(eres), 0));
			    else
			      add_srch_attr(zbuf, 
					    COLUMN_NAME(tobj, 
							cn), iargs[i]);
			}
		    }
		}

                modifyop = nis_defaults_init (eres, flags, NULL);

		nis_freeresult(eres);
		
		/*
		 * Now check for duplicates of new data.
		 */
		for (i = 0; i < tfp->cnt; ++i) {
			dbuf[0] = '\0';
			if (tfp->data_cols[i].key && tfp->data_cols[i].unique) {
				for (j = 0; j < ttp->io_args.cnt; ++j) {
					if ((oargs[j] != NULL) && (*oargs[j] != NULL) &&
					    !strcmp(ttp->io_args.at[j].name, 
					    tfp->data_cols[i].param)) {
					    	add_srch_attr(dbuf, COLUMN_NAME(tobj, i),
					    	    *oargs[j]);
						ldb_build_nis_name(dbuf, tobj->zo_name,
						    tobj->zo_domain, zname);
						eres = nis_list(zname, 
								MASTER_ONLY|FOLLOW_LINKS,
								null_fun, 
								(void *)NULL);

						if (NIS_RES_STATUS(eres) == NIS_NOTFOUND) {
							nis_freeresult(eres);
							break;
						} else if ((NIS_RES_STATUS(eres) == 
						    NIS_SUCCESS) || 
						    (NIS_RES_STATUS(eres) == NIS_S_SUCCESS)) {
							if (same_nisplus_keys(ttp, iargs, 
							    tres, eres)) {
							    	break;
							} else {
								db_err_set(db_err, 
								    DB_ERR_ENTRY_EXISTS,
								    dirty_bit, 
								    "set_table_impl",
								    *oargs[j], tobj->zo_name);
								nis_freeresult(eres);
								nis_freeresult(tres);
								return (-1);
							}
						} else {
							db_err_set(db_err, DB_ERR_NISPLUS,
							   dirty_bit, "nis_list", zname,
							   nis_sperrno(NIS_RES_STATUS(eres)));
							nis_freeresult(eres);
							nis_freeresult(tres);
							return (-1);
						}
					}
				}
			}
		}
		/*
		 * If doing a modify, we remove the old data
		 * first.
		 */
		if ((flags & DB_MODIFY)) {

			ldb_build_nis_name(zbuf, tobj->zo_name, tobj->zo_domain,
			    zname);
                        dres = nis_list(zname, MASTER_ONLY, NULL, NULL);
 
			eres = nis_remove_entry(zname, NULL, REM_MULTIPLE);

			if (tbl->type == DB_GROUP_TBL &&
			    ((NIS_RES_STATUS(dres) == NIS_SUCCESS) || 
			     (NIS_RES_STATUS(dres) == NIS_S_SUCCESS))  &&
			    dres->objects.objects_len > 0) {
			    strcpy(group_passwd,
				   ENTRY_VAL(dres->objects.objects_val, 1));
			} 

			if ((NIS_RES_STATUS(eres) != NIS_SUCCESS) &&
			    (NIS_RES_STATUS(eres) != NIS_S_SUCCESS) &&
			    ((flags & DB_ADD_MODIFY) == DB_MODIFY)) {
				if (NIS_RES_STATUS(eres) == NIS_NOTFOUND)
					db_err_set(db_err, DB_ERR_NO_ENTRY,
					    dirty_bit, "set_table_impl", sp,
					    tobj->zo_name);
				else
					db_err_set(db_err, DB_ERR_NISPLUS,
					    dirty_bit, "nis_remove_entry",
					    zname, nis_sperrno(NIS_RES_STATUS(eres)));
				nis_freeresult(tres);
				nis_freeresult(eres);
				return (-1);
			}
			nis_freeresult(eres);
		}
		
		ldb_build_nis_name(NULL, tobj->zo_name, tobj->zo_domain, zname);
		/*
		 * If this table is "aliased" and we have aliases, then
		 * set flags so we can deal with this later.
		 */
		ap = NULL;
		alias_cnt = 0;
		alias_search = 0;
		if (tfp->alias_col >= 0) {
			alias_search = 1;
			for (i = 0; i < ttp->io_args.cnt; ++i) {
				if ((oargs[i] == NULL) || (*oargs[i] == NULL))
					continue;
				else if (!strcmp(ttp->io_args.at[i].name,
				    tfp->data_cols[tfp->alias_col].param)) {
					ap = *oargs[i];
					break;
				}
			}
		}

		/*
		 * Now add each entry to the table.  Only one will be done
		 * unless the table is aliased, in which case we have to 
		 * grab each alias out of the list they were passed in and
		 * add an entry for each, where the only variant part is the
		 * alias column.
		 */
		do {
			if (alias_search) {
				if ((alias_cnt == 0) && 
				    (tbl->type != DB_NETGROUP_TBL)) {
					if (tbl->type == DB_HOSTS_TBL) {
						as = *oargs[1];
					} else {
						as = *oargs[0];
					}
				} else {
					as = strtok(ap, " ");
					if (ap != NULL)
						ap = NULL;
				}
				if (as == NULL) {
					alias_search = 0;
					continue;
				}
				++alias_cnt;
			}
		        
			
			eobj = (nis_object *) malloc(sizeof(nis_object));

			eobj->zo_domain = nis_default_obj.zo_domain;
			eobj->zo_ttl    = nis_default_obj.zo_ttl;
			eobj->zo_group  = nis_default_obj.zo_group;
			eobj->zo_access = nis_default_obj.zo_access;

			if (tbl->tri != NULL) {
			    if (tbl->tri->domain != NULL) {
				eobj->zo_domain = tbl->tri->domain;
			    }
			    if (tbl->tri->ttl != NULL) {
				eobj->zo_ttl    = tbl->tri->ttl;
			    }
			    if (tbl->tri->group_owner != NULL) {
				eobj->zo_group  = tbl->tri->group_owner;
			    }
			    if (tbl->tri->permissions != NULL) {
				eobj->zo_access = tbl->tri->permissions;
			    }
			}

                        if (modifyop == DB_MODIFY) {
                            /*
                            **  Update an existing entry.
                            */
                            eobj->zo_owner = nis_default_obj.zo_owner;
                        } else { 
                            /* 
                            **  Creating a new table entry.
                            **
                            **  The passwd and cred tables get special 
                            **  ownership so that users can use nispasswd to
                            **  change thier own password and gcos information.
                            */
                            if (tbl->type == DB_PASSWD_TBL) {
		                eobj->zo_owner = strdup(*oargs[0]);
			        eobj->zo_owner = 
				      (char *)realloc(eobj->zo_owner,
						      (strlen(eobj->zo_owner) 
						       + strlen(domain) + 3));
	                        strcat(eobj->zo_owner, ".");
                                strcat(eobj->zo_owner, domain);
                                if (domain[strlen(domain)-1] != '.')
                                    strcat(eobj->zo_owner, ".");
		            } else if (tbl->type == DB_CRED_TBL) {
                                /*
                                ** cred(LOCAL).  Want root to own the entry
                                ** so user can't change their uid. (defaults)
                                **
                                ** cred(DES).  Want the user to own the entry
			        ** so they can change their password.
			        */
			        if (strcmp( *oargs[1], "DES" ) == 0) {
			            eobj->zo_owner = strdup(*oargs[0]);
			        } else {
                                    eobj->zo_owner = nis_default_obj.zo_owner;
                                }

			    } else if (tbl->tri != NULL && 
				       tbl->tri->owner != NULL) {
				eobj->zo_owner = tbl->tri->owner;

	            	    } else {
                                eobj->zo_owner = nis_default_obj.zo_owner;
                            }
                        }  

			eobj->zo_data.zo_type = ENTRY_OBJ;
			eobj->EN_data.en_type = tobj->TA_data.ta_type;
			eobj->EN_data.en_cols.en_cols_val = 
			    (entry_col *)calloc(tobj->TA_data.ta_maxcol, 
			    sizeof(entry_col));
			eobj->EN_data.en_cols.en_cols_len = tobj->TA_data.ta_maxcol;
			zbuf[0] = '\0';
			/*
			 * Walk argument list and stuff each into the
			 * appropriate column.  Passwd table gets special
			 * handling 'cause the shadow arguments all end up in
			 * one column.  Netgroup table gets special handling to
			 * stuff the right things in the right columns, as
			 * different columns get filled in depending on the
			 * format of the data.
			 */
			for (i = 0; i < ttp->io_args.cnt; ++i) {
				if ((tbl->type == DB_PASSWD_TBL) &&
				    (i >= 7)) {
					if ((oargs[i] != NULL) &&
					    (*oargs[i] != NULL))
						strcat(zbuf, *oargs[i]);
					if (i != (ttp->io_args.cnt - 1)) {
						strcat(zbuf, tbl->tn.column_sep);
					}
					continue;
				}
				for (j = 0; j < tfp->cnt; ++j) {
					if (!strcmp(ttp->io_args.at[i].name, 
					    tfp->data_cols[j].param)) {
						if (j == tfp->alias_col) {
							if (tbl->type ==
							    DB_NETGROUP_TBL) {
								    set_netgroup_val(
									eobj, as);
								break;
							}
							set_col_val(eobj, j, as);
						} else if ((oargs[i] != NULL) &&
						    (*oargs[i] != NULL)) {
							set_col_val(eobj, j, 
							    *oargs[i]);
						}
						if (tfp->data_cols[j].key && 
						    tfp->data_cols[j].unique)
							sp = *oargs[i];
						break;
					}
				}
			}
			if (tbl->type == DB_PASSWD_TBL) {
				set_col_val(eobj, 7, zbuf);
			}

			if (tbl->type == DB_GROUP_TBL) {
			    if (*group_passwd == '\0') {
				set_col_val(eobj, 1, "*");
			    } else {
				set_col_val(eobj, 1, group_passwd);
			    }
			}

			eres = nis_add_entry(zname, eobj, 0);
			if (NIS_RES_STATUS(eres) != NIS_SUCCESS) {
				if ((flags & DB_ADD_MODIFY) ==
				    DB_MODIFY)
					dirty_bit = ADM_FAILDIRTY;
				if (NIS_RES_STATUS(eres) == NIS_NAMEEXISTS)
					db_err_set(db_err, DB_ERR_ENTRY_EXISTS,
					    dirty_bit, "set_table_impl",
					    sp, tobj->zo_name);
				else
					db_err_set(db_err, DB_ERR_NISPLUS, dirty_bit,
					    "nis_add_entry", zname,
						nis_sperrno(NIS_RES_STATUS(eres)));
				nis_freeresult(eres);
				nis_freeresult(tres);
				free(eobj->EN_data.en_cols.en_cols_val);
				free(eobj);
				return (-1);
			}
			free(eobj->EN_data.en_cols.en_cols_val);
			free(eobj);
		} while (alias_search);

		nis_freeresult(eres);
		nis_freeresult(tres);
	}
	return (0);
}

int
remove_table_impl(
	ulong_t ns_mask,
	char *host,
	char *domain,
	ulong_t flags,
	Db_error **db_err,
	Table *tbl,
	char **iargs,
	char ***oargs,
	struct tbl_trans_data *ttp,
	int action,
	uid_t *uidp)
{
	int status;
	Table *stbl;
	int dirty_bit;
	struct tbl_fmt *tfp;
	char zname[NIS_MAXNAMELEN], zbuf[NIS_MAXNAMELEN];
	nis_result *tres, *eres;
	nis_object *eobj, *tobj;
	int i, j, cn;
	char *sp, *ap, *as;
	int alias_search, alias_cnt;
	int (*null_fun)() = NULL;
	
	if (ns_mask == DB_NS_NIS) {
		if (ttp->tn.nis == NULL) {
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    ADM_FAILCLEAN, "remove_table_impl", "NIS");
			return (-1);
		}
		tfp = &ttp->fmts[DB_NIS_INDEX];
		/*
		 * Call function which actually does all the work for NIS.
		 */
		if (set_nis_db(host, domain, flags, db_err, tbl, iargs, oargs,
		    ttp, action, uidp) != 0)
			return (-1);
		/*
		 * Because the passwd table is tied to the shadow table, we
		 * do a call which will end up coming back through here to
		 * handle the shadow table.  Any failures at this point are
		 * dirty.
		 */
		if (tbl->type == DB_PASSWD_TBL) {
		    char* shadow_error_string;

		    if (shadow_map_exists(&shadow_error_string, domain)) {
			stbl = table_of_type(DB_SHADOW_TBL);
			status = lcl_remove_table_entry(DB_NS_NIS, host, domain,
			    flags, db_err, stbl, iargs[0]);
			free_table(stbl);
			if (status != 0) {
				(*db_err)->dirty = ADM_FAILDIRTY;
				return (-1);
			}
		    }
		}
	}
	if ((ns_mask & DB_NS_UFS)) {
		if (ttp->tn.ufs == NULL) {
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    ADM_FAILCLEAN, "remove_table_impl", "UFS");
			return (-1);
		}
		tfp = &ttp->fmts[DB_UFS_INDEX];
		/*
		 * Call function which actually does all the work for UFS.
		 */
		if (set_ufs_db(flags, db_err, tbl, iargs, oargs, action, ttp) != 0)
			return (-1);
		/*
		 * Because the passwd table is tied to the shadow table, we
		 * do a call which will end up coming back through here to
		 * handle the shadow table.  Any failures at this point are
		 * dirty.
		 */
		if (tbl->type == DB_PASSWD_TBL) {
			stbl = table_of_type(DB_SHADOW_TBL);
			status = lcl_remove_table_entry(DB_NS_UFS, host, domain,
			    flags, db_err, stbl, iargs[0]);
			free_table(stbl);
			if (status != 0) {
				(*db_err)->dirty = ADM_FAILDIRTY;
				return (-1);
			}
		}
	}
	if ((ns_mask & DB_NS_NISPLUS)) {
		dirty_bit = (ns_mask == DB_NS_NISPLUS) ? ADM_FAILCLEAN:ADM_FAILDIRTY;
		if (ttp->tn.nisplus == NULL) {
			db_err_set(db_err, DB_ERR_BAD_NAMESERVICE,
			    dirty_bit, "remove_table_impl", "NIS+");
			return (-1);
		}
		
		/*
		 * If we are being run as a method, we need to set-uid to the
		 * client in order to allow the NIS+ security to do its stuff.
		 */
		if (uidp != NULL)
			setuid(*uidp);

	        tfp = &ttp->fmts[DB_NISPLUS_INDEX];
	        
		ldb_build_nis_name(NULL, tbl->tn.nisplus, domain, zname);
		/*
		 * Get the table object using expand name magic.
		 */
		if (ldb_do_nis_lookup(zname, &tres, db_err) != 0)
		        return (-1);

		tobj = NIS_RES_OBJECT(tres);

		zbuf[0] = '\0';
		/*
		 * Now construct the NIS+ indexed name for the match
		 * arguments we were passed.
		 */
		for (i = 0; i < ttp->match_args.cnt; ++i) 
			if ((iargs[i] != NULL) &&
			    strlen(iargs[i])) {
				sp = iargs[i];
				cn = COLUMN_NUMBER(i);
				if (cn == tfp->alias_col)
					alias_search = 1;
				add_srch_attr(zbuf, 
				    COLUMN_NAME(tobj, cn), iargs[i]);
			}
		ldb_build_nis_name(zbuf, tobj->zo_name, tobj->zo_domain, zname);
		/*
		 * If this is an "aliased" table (multiple entries equal one of
		 * our Rows), we have to do a lookup to find the canonical name
		 * which links these entries together and reconstruct the indexed
		 * name so we can do the right thing below.
		 */
		if (alias_search) {
			eres = nis_list(zname, 
					MASTER_ONLY|FOLLOW_LINKS|EXPAND_NAME, 
					null_fun, 
					(void *) NULL);
			if ((NIS_RES_STATUS(eres) != NIS_SUCCESS) &&
			    (NIS_RES_STATUS(eres) != NIS_S_SUCCESS)) {
				if (NIS_RES_STATUS(eres) == NIS_NOTFOUND)
					db_err_set(db_err, DB_ERR_NO_ENTRY, 
					    dirty_bit, "remove_table_impl",
					    sp, tobj->zo_name);
				else
					db_err_set(db_err, DB_ERR_NISPLUS,
					    dirty_bit, "nis_list", zname,
					    nis_sperrno(NIS_RES_STATUS(eres)));
				nis_freeresult(tres);
				nis_freeresult(eres);
				return (-1);
			} else {
				zbuf[0] = '\0';
				for (i = 0; i < ttp->match_args.cnt; ++i) {
					if ((iargs[i] != NULL) &&
					    strlen(iargs[i])) {
					    	sp = iargs[i];
						cn = COLUMN_NUMBER(i);
						if (cn == tfp->alias_col)
							add_srch_attr(zbuf, 
							    COLUMN_NAME(tobj, 0), 
							    ENTRY_VAL(
							    NIS_RES_OBJECT(eres), 0));
						else
							add_srch_attr(zbuf, 
							    COLUMN_NAME(tobj, 
							    cn), iargs[i]);
					}
				}
			}
			nis_freeresult(eres);
			ldb_build_nis_name(zbuf, tobj->zo_name, tobj->zo_domain, 
			    zname);
		}
		
		/*
		 * Now do the remove 
		 */
		eres = nis_remove_entry(zname, NULL, REM_MULTIPLE);
		if ((NIS_RES_STATUS(eres) != NIS_SUCCESS) &&
		    (NIS_RES_STATUS(eres) != NIS_S_SUCCESS) &&
		    ((action == DB_REMOVE) || ((action == DB_SET) && 
		    ((flags & DB_ADD_MODIFY) == DB_MODIFY)))) {
			if (NIS_RES_STATUS(eres) == NIS_NOTFOUND)
				db_err_set(db_err, DB_ERR_NO_ENTRY,
				    dirty_bit, "set_table_impl", sp,
				    tobj->zo_name);
			else
				db_err_set(db_err, DB_ERR_NISPLUS,
				    dirty_bit, "nis_remove_entry",
				    zname, nis_sperrno(NIS_RES_STATUS(eres)));
			nis_freeresult(tres);
			nis_freeresult(eres);
			return (-1);
		}
		nis_freeresult(eres);
		nis_freeresult(tres);
	}
	return (0);
}
