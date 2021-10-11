#ifndef lint
#pragma ident "@(#)summary_util.c 1.35 96/07/30 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	summary_util.c
 * Group:	ttinstall
 * Description:
 */

#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <stdlib.h>

#include "pf.h"
#include "inst_msgs.h"
#include "summary_util.h"
#include "v_types.h"
#include "v_rfs.h"
#include "v_lfs.h"
#include "v_sw.h"
#include "v_disk.h"
#include "v_misc.h"

static _Summary_Row_t *_blank_line(_Summary_Row_t *, int *, int *);

_Summary_Row_t *
load_init_summary(_Summary_Row_t * table, int *row, int *last)
{
	char *str;
	char buf[128];
	char name[32];
	char part_char;
	int index;

	/*
	 * install option
	 */
	if (pfgState & AppState_UPGRADE) {
		if (pfgState & AppState_UPGRADE_DSR)
			str = INSTALL_TYPE_UPGRADE_DSR_STR;
		else
			str = INSTALL_TYPE_UPGRADE_STR;
	} else {
		str = INSTALL_TYPE_INITIAL_STR;
	}
	(void) sprintf(buf, "%*.*s:", SUMMARY_LABEL_LEN, SUMMARY_LABEL_LEN,
	    INSTALL_TYPE);
	table[*row].fld[0].label = xstrdup(buf);
	table[*row].fld[0].loc.c = 0;
	table[*row].fld[0].prompt = (char *) 0;
	table[*row].fld[0].sel_proc = NULL;

	table[*row].fld[1].label = xstrdup(str);
	table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
	table[*row].fld[1].prompt = (char *) 0;
	++(*row);
	table = grow_summary_table(table, row, last);

	/*
	 * Boot device
	 */
	(void) sprintf(buf, "%*.*s:", SUMMARY_LABEL_LEN, SUMMARY_LABEL_LEN,
	    INSTALL_BOOT_DEVICE);
	table[*row].fld[0].label = xstrdup(buf);
	table[*row].fld[0].loc.c = 0;
	table[*row].fld[0].prompt = (char *) 0;

	(void) BootobjGetAttribute(CFG_CURRENT,
		BOOTOBJ_DISK, name,
		BOOTOBJ_DEVICE_TYPE, &part_char,
		BOOTOBJ_DEVICE, &index,
		NULL);
	if (!IsIsa("sparc") || index == -1)
		(void) sprintf(buf, "%s", name);
	else
		(void) sprintf(buf, "%s%c%d", name, part_char, index);
	table[*row].fld[1].label = xstrdup(buf);
	table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
	table[*row].fld[1].prompt = (char *) 0;
	++(*row);
	table = grow_summary_table(table, row, last);

	return (table);
}

_Summary_Row_t *
load_upg_summary(_Summary_Row_t * table, int *row, int *last)
{
	char buf[128];
	UpgOs_t *slice;

	slice = SliceGetSelected(UpgradeSlices, NULL);
	(void) sprintf(buf, "%*.*s:",
		SUMMARY_LABEL_LEN,
		SUMMARY_LABEL_LEN,
		INSTALL_UPG_TARGET);
	table[*row].fld[0].label = xstrdup(buf);
	table[*row].fld[0].loc.c = 0;
	table[*row].fld[0].prompt = (char *) 0;

	(void) sprintf(buf, "%s %s", slice->release, slice->slice);
	table[*row].fld[1].label = xstrdup(buf);
	table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
	table[*row].fld[1].prompt = (char *) 0;

	(*row)++;
	table = grow_summary_table(table, row, last);

	table = load_dsr_summary(table, row, last);

	return (table);
}

_Summary_Row_t *
load_dsr_summary(_Summary_Row_t * table, int *row, int *last)
{
	char buf[128];
	DsrSLListExtraData *LLextra;


	/*
	 * DSR upgrade media
	 */
	if (pfgState & AppState_UPGRADE_DSR) {
		(void) sprintf(buf, "%*.*s:",
			SUMMARY_LABEL_LEN,
			SUMMARY_LABEL_LEN,
			INSTALL_UPG_DSR_BACKUP_MEDIA);
		table[*row].fld[0].label =
			xstrdup(buf);
		table[*row].fld[0].loc.c = 0;
		table[*row].fld[0].prompt = (char *) 0;

		(void) LLGetSuppliedListData(DsrSLHandle, NULL,
			(TLLData *)&LLextra);
		(void) sprintf(buf, "%s: %s",
			DsrALMediaTypeStr(LLextra->history.media_type),
			LLextra->history.media_device);
		table[*row].fld[1].label = xstrdup(buf);
		table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
		table[*row].fld[1].prompt = (char *) 0;

		(*row)++;
		table = grow_summary_table(table, row, last);

	}

	return (table);
}

_Summary_Row_t *
load_rfs_summary(_Summary_Row_t * table, int *row, int *last)
{
	int i;
	int first = 1;
	char *mp;
	char buf[128];

	for (i = 0; (i < v_get_n_rfs()); i++) {

		if (v_rfs_configed(i) == 1) {

			if (mp = v_get_rfs_mnt_pt(i)) {

				if (first) {

					table = _blank_line(table, row, last);

					(void) sprintf(buf, "%*.*s:",
						SUMMARY_LABEL_LEN,
						SUMMARY_LABEL_LEN,
						REMOTE_TITLE);
					table[*row].fld[0].label = xstrdup(buf);
					table[*row].fld[0].loc.c = 0;
					table[*row].fld[0].prompt = (char *) 0;
					table[*row].fld[0].sel_proc =
						(parIntFunc) do_rfs;

					first = 0;
				} else {
					table[*row].fld[0].label = xstrdup(" ");
					table[*row].fld[0].loc.c = 0;
					table[*row].fld[0].prompt = (char *) 0;
				}

				(void) sprintf(buf, "%.*s",
					SUMMARY_LABEL_LEN, mp);
				table[*row].fld[1].label = xstrdup(buf);
				table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
				table[*row].fld[1].prompt = (char *) 0;

				/*
				 * increase table size if necessary
				 */
				(*row)++;
				table = grow_summary_table(table, row, last);

			}
		}
	}

	return (table);
}

_Summary_Row_t *
load_lfs_summary(_Summary_Row_t * table, int *row, int *last)
{
	int i;
	int j;
	int first = 1;
	char *mount;
	char buf[128];
	V_Units_t units = v_get_disp_units();

	v_set_disp_units(V_MBYTES);

	/* for each disk */
	for (i = 0; i < v_get_n_disks(); i++) {

		if (v_get_disk_usable(i) == 1) {

			(void) v_set_current_disk(i);

			/* for each possible slice */
			for (j = 0; j < N_Slices; j++) {

				mount = v_get_cur_mount_pt(j);

				/* skip alts and overlap mount points */
				if ((strcmp(mount, Alts) == 0) ||
				    (strcmp(mount, Overlap) == 0))
					continue;

				/* skip empty mount points */
				if (mount == (char *) 0 || *mount == '\0')
					continue;

				if (first) {

					table = _blank_line(table, row, last);

					(void) sprintf(buf, "%*.*s:",
						SUMMARY_LABEL_LEN,
						SUMMARY_LABEL_LEN,
					gettext("File System and Disk Layout"));

					table[*row].fld[0].label = xstrdup(buf);
					table[*row].fld[0].loc.c = 0;
					table[*row].fld[0].prompt = (char *) 0;
					table[*row].fld[0].sel_proc =
					    (parIntFunc) do_disk_edit;

					first = 0;
				} else {
					table[*row].fld[0].label = xstrdup(" ");
					table[*row].fld[0].loc.c = 0;
					table[*row].fld[0].prompt = (char *) 0;
				}

				/* show the file system information */
				(void) sprintf(buf, "%-*.*s%s %ss%d %4d %s",
				    ((int) strlen(mount) > 18 ? 13 : 18),
				    ((int) strlen(mount) > 18 ? 13 : 18),
				    mount,
				    ((int) strlen(mount) > 18 ? " ...>" : " "),
				    v_get_disk_name(i),
				    j,	/* slice # */
				    v_get_cur_size(j),
				    v_get_disp_units_str());

				table[*row].fld[1].label = xstrdup(buf);
				table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
				table[*row].fld[1].prompt = (char *) 0;

				/* increase table size if necessary */
				(*row)++;
				table = grow_summary_table(table, row, last);

			}
		}
	}

	v_set_disp_units(units);

	return (table);
}

_Summary_Row_t *
load_sw_summary(_Summary_Row_t * table, int *row, int *last)
{
	char buf[128];
	int i;
	int n;
	Module *module;

	/*
	 * first row of sw summary shows product, version and metacluster
	 */
	(void) sprintf(buf, "%*.*s:",
		SUMMARY_LABEL_LEN,
		SUMMARY_LABEL_LEN,
		gettext("Software"));
	table[*row].fld[0].label = xstrdup(buf);
	table[*row].fld[0].loc.c = 0;
	table[*row].fld[0].prompt = (char *) 0;
	table[*row].fld[0].sel_proc =
		(parIntFunc) do_sw_edit;


	if (pfgState & AppState_UPGRADE) {
		module = (Module *)get_local_metacluster();
		if (module) {
			(void) sprintf(buf, "%s %s, %s", v_get_product_name(),
				v_get_product_version(),
				module->info.mod->m_name);
		} else {
			(void) sprintf(buf, "");
		}

	} else { /* initial install */
		(void) sprintf(buf, "%s %s, %s", v_get_product_name(),
			v_get_product_version(),
			v_get_metaclst_name(v_get_current_metaclst()));
	}

	table[*row].fld[1].label = xstrdup(buf);
	table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
	table[*row].fld[1].prompt = (char *) 0;

	/*
	 * increase table size if necessary
	 */
	(*row)++;
	table = grow_summary_table(table, row, last);

	/*
	 * additional rows show any modifications to default sw config
	 */

	/* get # of modules added, non-zero argument retrieves additions */
	if ((n = v_get_n_metaclst_deltas(1)) > 0) {

		table = _blank_line(table, row, last);

		table[*row].fld[0].label = xstrdup(" ");
		table[*row].fld[0].loc.c = 0;
		table[*row].fld[0].prompt = (char *) 0;

		(void) sprintf(buf, "%.*s", SUMMARY_LABEL_LEN,
		    gettext("Including:"));

		table[*row].fld[1].label = xstrdup(buf);
		table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
		table[*row].fld[1].prompt = (char *) 0;

		(*row)++;
		table = grow_summary_table(table, row, last);

		for (i = 0; i < n; i++) {

			table[*row].fld[0].label = xstrdup(" ");
			table[*row].fld[0].loc.c = 0;
			table[*row].fld[0].prompt = (char *) 0;

			(void) sprintf(buf, "  %.*s",
			    SUMMARY_LABEL_LEN, v_get_delta_package_name(i));

			table[*row].fld[1].label = xstrdup(buf);
			table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
			table[*row].fld[1].prompt = (char *) 0;

			(*row)++;
			table = grow_summary_table(table, row, last);

		}

	}
	/* get # of modules removed, zero argument retrieves removals */
	if ((n = v_get_n_metaclst_deltas(0)) > 0) {

		table = _blank_line(table, row, last);

		table[*row].fld[0].label = xstrdup(" ");
		table[*row].fld[0].loc.c = 0;
		table[*row].fld[0].prompt = (char *) 0;

		(void) sprintf(buf, "%.*s", SUMMARY_LABEL_LEN,
		    gettext("Excluding:"));

		table[*row].fld[1].label = xstrdup(buf);
		table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
		table[*row].fld[1].prompt = (char *) 0;

		(*row)++;
		table = grow_summary_table(table, row, last);

		for (i = 0; i < n; i++) {

			table[*row].fld[0].label = xstrdup(" ");
			table[*row].fld[0].loc.c = 0;
			table[*row].fld[0].prompt = (char *) 0;

			(void) sprintf(buf, "  %.*s",
			    SUMMARY_LABEL_LEN, v_get_delta_package_name(i));

			table[*row].fld[1].label = xstrdup(buf);
			table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
			table[*row].fld[1].prompt = (char *) 0;

			(*row)++;
			table = grow_summary_table(table, row, last);

		}
	}
	return (table);

}

_Summary_Row_t *
load_client_arch_summary(_Summary_Row_t * table, int *row, int *last)
{
	char buf[128];
	int i;
	int first = 1;

	for (i = 0; (i < v_get_n_arches()); i++) {

		if (v_get_arch_status(i) == SELECTED) {

			if (first) {
				(void) sprintf(buf, "%*.*s:",
					SUMMARY_LABEL_LEN,
					SUMMARY_LABEL_LEN,
					INSTALL_SUMMARY_CLIENT_ARCH_TITLE);
				table[*row].fld[0].label = xstrdup(buf);
				table[*row].fld[0].loc.c = 0;
				table[*row].fld[0].prompt = (char *) 0;
				table[*row].fld[0].sel_proc =
					(parIntFunc) do_client_arches;
				first = 0;
			} else {
				table[*row].fld[0].label = xstrdup(" ");
				table[*row].fld[0].loc.c = 0;
				table[*row].fld[0].prompt = (char *) 0;
			}

			(void) sprintf(buf, "%-.*s",
				SUMMARY_LABEL_LEN, v_get_arch_name(i));
			table[*row].fld[1].label = xstrdup(buf);
			table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
			table[*row].fld[1].prompt = (char *) 0;

			/*
			 * increase table size if necessary
			 */
			(*row)++;
			table = grow_summary_table(table, row, last);

		}
	}

	return (table);
}

_Summary_Row_t *
load_locale_summary(_Summary_Row_t * table, int *row, int *last)
{
	char buf[128];
	int i;
	int first = 1;

	for (i = 0; (i < v_get_n_locales()); i++) {

		if (v_get_locale_status(i) == SELECTED) {

			if (first) {
				table = _blank_line(table, row, last);

				(void) sprintf(buf, "%*.*s:",
					SUMMARY_LABEL_LEN,
					SUMMARY_LABEL_LEN,
					INSTALL_SUMMARY_LOCALE_TITLE);
				table[*row].fld[0].label = xstrdup(buf);
				table[*row].fld[0].loc.c = 0;
				table[*row].fld[0].prompt = (char *) 0;
				table[*row].fld[0].sel_proc =
					(parIntFunc) do_alt_lang;

				first = 0;
			} else {
				table[*row].fld[0].label = xstrdup(" ");
				table[*row].fld[0].loc.c = 0;
				table[*row].fld[0].prompt = (char *) 0;
			}

			(void) sprintf(buf, "%-.*s",
				SUMMARY_LABEL_LEN, v_get_locale_language(i));
			table[*row].fld[1].label = xstrdup(buf);
			table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
			table[*row].fld[1].prompt = (char *) 0;

			/*
			 * increase table size if necessary
			 */
			(*row)++;
			table = grow_summary_table(table, row, last);

		}
	}

	return (table);
}
void
show_summary_table(WINDOW * w, int max, int npp, int row,
	_Summary_Row_t * table, int first)
{
	int i;			/* counts lines displayed	*/
	int j;			/* index of current summary line */
	int r;			/* counts row positions		*/

	j = first;

	for (i = 0, r = row; (i < npp) && (j < max); i++, r++) {

		(void) mvwprintw(w, r, 0, "%*s", COLS, " ");
		(void) mvwprintw(w, r, table[j].fld[0].loc.c, "%s",
		    table[j].fld[0].label);
		(void) mvwprintw(w, r, table[j].fld[1].loc.c, "%s",
		    table[j].fld[1].label);

		table[j].fld[0].loc.r = table[j].fld[1].loc.r = r;
		j++;
	}

	for (; i < npp; i++, r++) {
		(void) mvwprintw(w, r, 0, "%*s", COLS, " ");
	}

	/*
	 * X86 hardware has some refresh problems with this
	 * scrolling/repainting wnoutrefresh()/doupdate() don't seem to fix.
	 * Force the lines to be redrawn completly.
	 */
	(void) wredrawln(w, row, row + npp);

}

void
free_summary_table(_Summary_Row_t * table, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		if (table[i].fld[0].label != (char *) 0)
			free((void *) table[i].fld[0].label);
		if (table[i].fld[1].label != (char *) 0)
			free((void *) table[i].fld[1].label);
	}

	free((void *) table);
}


_Summary_Row_t *
grow_summary_table(_Summary_Row_t * table, int *row, int *last)
{
	_Summary_Row_t *tmp = table;

	if (*row == *last) {
		*last += *last;

		tmp = (_Summary_Row_t *) xrealloc((void *) table, (*last) *
		    sizeof (_Summary_Row_t));
	}
	return (tmp);
}

static _Summary_Row_t *
_blank_line(_Summary_Row_t * table, int *row, int *last)
{
	table[*row].fld[0].label = xstrdup(" ");
	table[*row].fld[0].loc.c = 0;
	table[*row].fld[0].prompt = (char *) 0;
	table[*row].fld[1].label = xstrdup(" ");
	table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
	table[*row].fld[1].prompt = (char *) 0;
	(*row)++;
	table = grow_summary_table(table, row, last);

	return (table);
}
