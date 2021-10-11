/*
 *	nistbladm.c
 *
 *	Copyright (c) 1988-1992 Sun Microsystems Inc
 *	All Rights Reserved.
 */

#pragma ident	"@(#)nistbladm.c	1.27	96/07/25 SMI"

/*
 * nistbladm.c
 *
 * nis+ table admin utility
 */

#include <stdio.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpcsvc/nis.h>

extern int 	optind;
extern char	*optarg;

extern nis_object nis_default_obj;

extern char *nisname_index();


char *DEFAULT_PATH = "";
char *DEFAULT_SEP = " ";

#define	OP_CREATE 1
#define	OP_UPDATE 2
#define	OP_DESTROY 3
#define	OP_ADD 4
#define	OP_MODIFY 5
#define	OP_REMOVE 6

/* change defines */
#define	CHG_PATH	1
#define	CHG_SEP		2
#define	CHG_ACCESS	4
#define	CHG_TYPE	5

/* size of buffers for holding indexed names */
#define	MAX_INAME 8192

void
usage()
{
	fprintf(stderr,
	"usage:\tnistbladm [-D defaults] -c [-p path] [-s sep] type\n");
	fprintf(stderr, "\t\tcolname=[flags][,access] ... tablename\n");
	fprintf(stderr,
"\tnistbladm -u [-p path] [-s sep] [-t type] [colname=access ...] tablename\n");
	fprintf(stderr, "\tnistbladm -d tablename\n");
	fprintf(stderr,
		"\tnistbladm [-D defaults] -a|A colname=val ... tablename\n");
	fprintf(stderr, "\tnistbladm [-D defaults] -a|A indexedname\n");
	fprintf(stderr, "\tnistbladm -e|E colname=val ... indexedname\n");
	fprintf(stderr, "\tnistbladm -m colname=val ... indexedname\n");
	fprintf(stderr, "\tnistbladm -r|R [colname=val ...] tablename\n");
	fprintf(stderr, "\tnistbladm -r|R indexedname\n");
	exit(1);
}

/*
 *  This function is used to copy a "colname=value" string into another
 *  string that will be passed to nis_get_request.  Any unquoted ","
 *  or "]" characters are quoted so that they won't be interpreted as
 *  the end of the indexed name.
 */
static
char *
strcat_quote(char *dst, char *src)
{
	char *orig = dst;

	/* find end of dst */
	while (*dst)
		dst++;

	/*
	 *  Process each character in src.  If it is a quote, copy
	 *  characters until reaching the terminating quote.  If it
	 *  is a ',' or a ']', then quote it into dest.  All other
	 *  characters are copied "as is".
	 */
	while (*src) {
		if (*src == '"') {
			*dst++ = *src++;
			while (*src && *src != '"')
				*dst++ = *src++;
			if (*src)
				*dst++ = *src++;
		} else if (*src == ',' || *src == ']') {
			*dst++ = '"';
			*dst++ = *src++;
			*dst++ = '"';
		} else {
			*dst++ = *src++;
		}
	}

	/* terminate dst */
	*dst++ = 0;

	return (orig);
}

main(argc, argv)
	int argc;
	char *argv[];
{
	int c;
	char *defstr = 0;
	int op = 0;
	unsigned flags = 0;
	char *name;
	nis_result *res, *ares, *mres, *rres;
	nis_object *obj, tobj, eobj;
	char *ta_type = 0, *ta_path = DEFAULT_PATH, *ta_sep = DEFAULT_SEP;
	int ta_maxcol, i, j;
	char *p, *p1;
	table_col *tcol, *ocol;
	char mpred[MAX_INAME];
	char spred[MAX_INAME];
	char sname[MAX_INAME];
	char tname[NIS_MAXNAMELEN];
	nis_error s;
	ib_request ibr;
	entry_col *ecol;
	int nrights = 0;
	char **colname;
	char **rights;

	while ((c = getopt(argc, argv, "D:cudaAeEmrRp:s:t:")) != -1) {
		switch (c) {
		case 'D':
			defstr = optarg;
			break;
		case 'c':
			if (op)
				usage();
			op = OP_CREATE;
			break;
		case 'u':
			if (op)
				usage();
			op = OP_UPDATE;
			break;
		case 'd':
			if (op)
				usage();
			op = OP_DESTROY;
			break;
		case 'A':
			flags |= ADD_OVERWRITE;
		case 'a':
			if (op)
				usage();
			op = OP_ADD;
			break;
		case 'e':
			flags |= MOD_EXCLUSIVE;
		case 'm':
		case 'E':
			if (op)
				usage();
			op = OP_MODIFY;
			break;
		case 'R':
			flags |= REM_MULTIPLE;
		case 'r':
			if (op)
				usage();
			op = OP_REMOVE;
			break;
		case 'p':
			ta_path = optarg;
			break;
		case 's':
			ta_sep = optarg;
			if (strlen(optarg) != 1) {
				fprintf(stderr,
				    "separator must be a single character\n");
				exit(1);
			}
			break;
		case 't':
			ta_type = optarg;
			break;
		default:
			usage();
		}
	}

	if (!op)
		usage();

	if (!nis_defaults_init(defstr))
		exit(1);


	switch (op) {
	case OP_CREATE:
		if (argc - optind < 3)
			usage();
		ta_type = argv[optind++];
		ta_maxcol = argc - optind - 1;
		if ((tcol = (table_col*)malloc(ta_maxcol*sizeof (table_col)))
									== 0) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
		for (i = 0; i < ta_maxcol; i++) {
			p = tcol[i].tc_name = argv[optind++];
			if ((p = strchr(p, '=')) == 0)
				usage();
			*(p++) = 0;
			if (*p) {
				p1 = strchr(p, ',');
				if (p1)
					*(p1++) = 0;
			} else
				p1 = 0;
			tcol[i].tc_flags = 0;
			if (!parse_flags(&(tcol[i].tc_flags), p))
				usage();
			tcol[i].tc_rights = nis_default_obj.zo_access;
			if (!parse_rights(&(tcol[i].tc_rights), p1))
				usage();
		}

		name = argv[optind];
		if (name[strlen(name)-1] != '.') {
			fprintf(stderr, "tablename must be fully qualified.\n");
			exit(1);
		}

		tobj = nis_default_obj;
		tobj.zo_data.zo_type = NIS_TABLE_OBJ;
		tobj.TA_data.ta_type = ta_type;
		tobj.TA_data.ta_maxcol = ta_maxcol;
		tobj.TA_data.ta_sep = *ta_sep;
		tobj.TA_data.ta_path = ta_path;
		tobj.TA_data.ta_cols.ta_cols_len = ta_maxcol;
		tobj.TA_data.ta_cols.ta_cols_val = tcol;

		ares = nis_add(name, &tobj);
		if (ares->status != NIS_SUCCESS) {
			nis_perror(ares->status, "can't create table");
			exit(1);
		}
		exit(0);

	case OP_UPDATE:
		if (argc <= optind)
			usage();
		if (argc - optind > 1) {
			nrights = argc - optind - 1;
			colname = (char **)malloc(nrights * sizeof (char *));
			rights = (char **)malloc(nrights * sizeof (char *));
			if (colname == 0 || rights == 0) {
				fprintf(stderr, "out of memory\n");
				exit(1);
			}

			for (i = 0; i < nrights; i++) {
				p = strchr(argv[optind], '=');
				if (p == 0) {
					fprintf(stderr,
					    "Missing access rights for %s\n",
					    argv[optind]);
					exit(1);
				}
				*p++ = 0;
				colname[i] = strdup(argv[optind]);
				rights[i] = strdup(p);
				if (colname[i] == 0 || rights[i] == 0) {
					fprintf(stderr, "out of memory\n");
					exit(1);
				}

				optind++;
			}
		}

		name = argv[optind];
		if (*name == '[')
			usage();
		break;

	case OP_DESTROY:
		if (argc - optind < 1)
			usage();
		name = argv[optind];
		break;

	case OP_ADD:
	case OP_REMOVE:
		if (argc - optind < 1)
			usage();
		p = argv[optind++];
		if (*p == '[') {
			name = nisname_index(p, ']');
			if (name == 0)
				usage();
			name++;
			strncpy(spred, p, name-p);
			spred[name-p] = 0;
			if (*name == ',')
				name++;
		} else if (argc - optind < 1) {
			if (op == OP_ADD)
				usage();
			spred[0] = 0;
			name = p;
		} else {
			spred[0] = 0;
			strcat_quote(spred, p);
			while (argc - optind > 1) {
				strcat(spred, ",");
				strcat_quote(spred, argv[optind++]);
			}
			name = argv[optind++];
			if (*name == '[')
				usage();
		}
		break;

	case OP_MODIFY:
		if (argc - optind < 2)
			usage();
		mpred[0] = 0;
		strcat_quote(mpred, argv[optind++]);
		while (argc - optind > 1) {
			strcat(mpred, ",");
			strcat_quote(mpred, argv[optind++]);
		}
		p = argv[optind++];
		if (*p != '[')
			usage();
		name = nisname_index(p, ']');
		if (name == 0)
			usage();
		name++;
		strncpy(spred, p, name-p);
		spred[name-p] = 0;
		if (*name == ',')
			name++;
		break;
	}

	/*
	 * Get the table object.
	 */
	res = nis_lookup(name, MASTER_ONLY|FOLLOW_LINKS|EXPAND_NAME);
	if (res->status != NIS_SUCCESS) {
		nis_perror(res->status, name);
		exit(1);
	}

	obj = &(NIS_RES_OBJECT(res)[0]);
	sprintf(tname, "%s.", res->objects.objects_val[0].zo_name);
	if (*(res->objects.objects_val[0].zo_domain) != '.')
		strcat(tname, res->objects.objects_val[0].zo_domain);

	if (obj->zo_data.zo_type != NIS_TABLE_OBJ) {
		fprintf(stderr, "%s is not a table!\n", name);
		exit(1);
	}

	ta_maxcol = obj->TA_data.ta_maxcol;

	switch (op) {
	case OP_UPDATE:

		/* Now we morph the table object, first start with orig */
		tobj = *obj;

		/* Change its type if desired */
		if (ta_type)
			tobj.TA_data.ta_type = ta_type;

		/* Change the separator if desired */
		if (ta_sep != DEFAULT_SEP)
			tobj.TA_data.ta_sep = *ta_sep;

		/* Change the path if desired */
		if (ta_path != DEFAULT_PATH)
			tobj.TA_data.ta_path = ta_path;

		/* Allocate some column descriptors */
		tcol = (table_col *)calloc(ta_maxcol, sizeof (table_col));
		if (tcol == 0) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}

		/* Set the new table to have these malloc'd columns */
		tobj.TA_data.ta_cols.ta_cols_val = tcol;
		ocol = obj->TA_data.ta_cols.ta_cols_val;

		/* check the table schema */
		for (i = 0; i < nrights; ++i) {
		    for (j = 0; j < ta_maxcol; ++j)
			if (strcmp(ocol[j].tc_name, colname[i]) == 0)
				break;
		    if (j == ta_maxcol) {
			fprintf(stderr,
			    "Invalid table schema: %s is an invalid column.\n",
			    colname[i]);
			exit(1);
		    }
		}

		for (j = 0; j < ta_maxcol; j++) {

			/* Copy the old column into the new column */
			tcol[j] = ocol[j];

			/* Check to see if new access rights were specified */
			for (i = 0; i < nrights; i++) {

				/* If so change them appropriately */
				if (strcmp(ocol[j].tc_name, colname[i]) == 0) {
					if (! parse_rights(&(tcol[j].tc_rights),
							rights[i])) {
						fprintf(stderr,
		"Couldn't parse access rights (%s) for column %s\n",
							rights[i],
							colname[i]);
						exit(1);
					} else {
						tcol[j].tc_flags |= TA_MODIFIED;
						break;
					}	/* parsed correctly   */
				}		/* same name as index */
			}			/* for all indices    */
		}				/* for all columns    */

		mres = nis_modify(tname, &tobj);
		if (mres->status != NIS_SUCCESS) {
			nis_perror(mres->status, "can't modify table");
			exit(1);
		}
		exit(0);

	case OP_DESTROY:
		rres = nis_remove(tname, 0);
		if (rres->status != NIS_SUCCESS) {
			nis_perror(rres->status, "can't remove table");
			exit(1);
		}
		exit(0);

	case OP_ADD:
		if (*spred == '[')
			sprintf(sname, "%s%s", spred, tname);
		else
			sprintf(sname, "[%s]%s", spred, tname);

		/*
		 * Parse search criteria.
		 */
		s = nis_get_request(sname, 0, 0, &ibr);
		if (s != NIS_SUCCESS) {
			nis_perror(s, "can't parse column values");
			exit(1);
		}

		eobj = nis_default_obj;
		if (eobj.zo_ttl > obj->zo_ttl)
			eobj.zo_ttl = obj->zo_ttl;
		eobj.zo_data.zo_type = NIS_ENTRY_OBJ;
		eobj.EN_data.en_type = obj->TA_data.ta_type;

		if ((ecol = (entry_col*)malloc(ta_maxcol*sizeof (entry_col)))
								== 0) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
		memset((char *)ecol, 0, ta_maxcol*sizeof (entry_col));
		eobj.EN_data.en_cols.en_cols_len = ta_maxcol;
		eobj.EN_data.en_cols.en_cols_val = ecol;

		for (i = 0; i < ibr.ibr_srch.ibr_srch_len; i++) {
			for (j = 0; j < ta_maxcol; j++)
				if (strcmp(
				obj->TA_data.ta_cols.ta_cols_val[j].tc_name,
				ibr.ibr_srch.ibr_srch_val[i].zattr_ndx) == 0)
					break;
			if (j < ta_maxcol) {
				ecol[j].ec_value.ec_value_len =
			ibr.ibr_srch.ibr_srch_val[i].zattr_val.zattr_val_len;
				ecol[j].ec_value.ec_value_val =
			ibr.ibr_srch.ibr_srch_val[i].zattr_val.zattr_val_val;
				ecol[j].ec_flags =
				obj->TA_data.ta_cols.ta_cols_val[j].tc_flags &
				(TA_BINARY|TA_CRYPT|TA_XDR|TA_ASN1);
			} else {
				fprintf(stderr,
				    "table has no column named %s\n",
				    ibr.ibr_srch.ibr_srch_val[i].zattr_ndx);
				exit(1);
			}
		}

		ares = nis_add_entry(tname, &eobj, flags);
		if (ares->status != NIS_SUCCESS) {
			nis_perror(ares->status, "can't add entry");
			exit(1);
		}
		break;

	case OP_MODIFY:
		/*
		 * Parse modify criteria.
		 */
		sprintf(sname, "[%s]%s", mpred, tname);
		s = nis_get_request(sname, 0, 0, &ibr);
		if (s != NIS_SUCCESS) {
			nis_perror(s, "can't parse column values");
			exit(1);
		}

		memset((char *)&eobj, 0, sizeof (eobj));
		eobj.zo_data.zo_type = NIS_ENTRY_OBJ;
		eobj.EN_data.en_type = obj->TA_data.ta_type;

		if ((ecol = (entry_col*)malloc(ta_maxcol*sizeof (entry_col)))
								== 0) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
		memset((char *)ecol, 0, ta_maxcol*sizeof (entry_col));
		eobj.EN_data.en_cols.en_cols_len = ta_maxcol;
		eobj.EN_data.en_cols.en_cols_val = ecol;

		for (i = 0; i < ibr.ibr_srch.ibr_srch_len; i++) {
			for (j = 0; j < ta_maxcol; j++)
				if (strcmp(
				obj->TA_data.ta_cols.ta_cols_val[j].tc_name,
				ibr.ibr_srch.ibr_srch_val[i].zattr_ndx) == 0)
					break;
			if (j < ta_maxcol) {
				ecol[j].ec_value.ec_value_len =
			ibr.ibr_srch.ibr_srch_val[i].zattr_val.zattr_val_len;
				ecol[j].ec_value.ec_value_val =
			ibr.ibr_srch.ibr_srch_val[i].zattr_val.zattr_val_val;
				ecol[j].ec_flags =
				(obj->TA_data.ta_cols.ta_cols_val[j].tc_flags &
				(TA_BINARY|TA_CRYPT|TA_XDR|TA_ASN1)) |
				EN_MODIFIED;
			} else {
				fprintf(stderr,
				    "table has no column named %s\n",
				    ibr.ibr_srch.ibr_srch_val[i].zattr_ndx);
				exit(1);
			}
		}

		sprintf(sname, "%s%s", spred, tname);

		mres = nis_modify_entry(sname, &eobj, flags);
		if (mres->status != NIS_SUCCESS) {
			nis_perror(mres->status, "can't modify entry");
			exit(1);
		}
		break;

	case OP_REMOVE:
		if (*spred == '[')
			sprintf(sname, "%s%s", spred, tname);
		else
			sprintf(sname, "[%s]%s", spred, tname);

		rres = nis_remove_entry(sname, 0, flags);
		if (rres->status != NIS_SUCCESS) {
			nis_perror(rres->status, "can't remove entry");
			exit(1);
		}
		break;
	}

	exit(0);
}
