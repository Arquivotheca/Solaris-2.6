#ifndef lint
#pragma ident   "@(#)svc_sp_print_results.c 1.7 96/07/10 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include "spmistore_api.h"
#include "spmicommon_api.h"
#include "spmisoft_lib.h"
#include "spmisvc_lib.h"
#include <fcntl.h>
#include <libintl.h>
#include <stdlib.h>
#include <sys/stat.h>

/*	Public Function Prototypes	*/

void	print_final_results(FSspace **, char *);
SW_space_results	*gen_final_space_report(FSspace **);

/*	Static function prototyptes 	*/
static SW_space_results	*_final_results(FSspace **, char *, int);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * print_final_results()
 *	To be run after final_space_chk(). Print results to file.
 * Parameters:
 *	outfile	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_final_results(FSspace **fs_list, char * outfile)
{
	(void) _final_results(fs_list, outfile, 0);
}

/*
 * gen_final_space_report - produce a data structure containing the
 *	final space report.
 * Parameters:
 *	none
 * Return:
 *      SW_space_results *
 * Status:
 *	public
 */
SW_space_results *
gen_final_space_report(FSspace **fs_list)
{
	return (_final_results(fs_list, NULL, 1));
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

static SW_space_results *
_final_results(FSspace **sp, char * outfile, int output_struct)
{
	int		i, inode_err;
	FILE		*fp = NULL;
	char		inode_flag;
	SW_space_results	*sp_head = NULL;
	SW_space_results	*sp_cur, *sp_tmp;

	if (outfile) {
		fp = fopen(outfile, "w");
		if (fp == (FILE *) NULL) {
			return (NULL);
		}
		(void) chmod(outfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}

	if (fp) {
		/*
		 * Print Current slice size and suggested slice size.
		 */
		(void) fprintf(fp, "%-20s%-20s%-20s%-20s\n",
			"", "",
			dgettext("SUNW_INSTALL_LIBSVC", "Current Size"),
			dgettext("SUNW_INSTALL_LIBSVC", "Minimum Suggested"));

		(void) fprintf(fp, "%-20s%-20s%-20s%-20s\n",
			dgettext("SUNW_INSTALL_LIBSVC", "Mount Point"),
			dgettext("SUNW_INSTALL_LIBSVC", "Slice"),
			dgettext("SUNW_INSTALL_LIBSVC", "1 Kilobyte Blocks"),
			dgettext("SUNW_INSTALL_LIBSVC", "1 Kilobyte Blocks"));

		(void) fprintf(fp,
"-------------------------------------------------------------------------------\n");
	}

	inode_err = 0;

	/*
	 * Run through the list and print out all of the file systems that have failed
	 * due to space limitations.
	 */

	
	(void) fprintf(fp, "%s\n",
	    dgettext("SUNW_INSTALL_LIBSVC",
		"File systems with insufficient space."));
	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;

		if (sp[i]->fsp_fsi == NULL) {
			continue;
		}

		if (!(sp[i]->fsp_flags & FS_INSUFFICIENT_SPACE))
			continue;

		inode_flag = ' ';

		if (sp[i]->fsp_cts.contents_inodes_used >
		    sp[i]->fsp_fsi->f_files) {
			inode_flag = '*';
			inode_err++;
		}
		if (fp) {
			if ((int)strlen(sp[i]->fsp_mntpnt) > (int) 19) {
				(void) fprintf(fp, "%s\n", sp[i]->fsp_mntpnt);
				(void) fprintf(fp,
				    "%-20s%-20s%-20ld%-18ld%c\n", "",
				    sp[i]->fsp_fsi->fsi_device,
				    sp[i]->fsp_cur_slice_size,
				    sp[i]->fsp_reqd_slice_size, inode_flag);
	
			} else {
				(void) fprintf(fp, "%-20s%-20s%-20ld%-18ld%c\n",
				    sp[i]->fsp_mntpnt,
				    sp[i]->fsp_fsi->fsi_device,
				    sp[i]->fsp_cur_slice_size,
				    sp[i]->fsp_reqd_slice_size, inode_flag);
			}
		}
		if (output_struct) {
			sp_tmp = (SW_space_results *) xmalloc((size_t) sizeof
			    (SW_space_results));
			sp_tmp->next = NULL;
			if (sp_head == NULL) {
				sp_head = sp_tmp;
				sp_cur = sp_tmp;
			} else {
				sp_cur->next = sp_tmp;
				sp_cur = sp_tmp;
			}
			sp_cur->sw_mountpnt =
			    xstrdup(sp[i]->fsp_mntpnt);
			sp_cur->sw_devname =
			    xstrdup(sp[i]->fsp_fsi->fsi_device);
			sp_cur->sw_cursiz = sp[i]->fsp_cur_slice_size;
			sp_cur->sw_newsiz = sp[i]->fsp_reqd_slice_size;
			if (inode_flag == ' ')
				sp_cur->sw_toofew_inodes = 0;
			else
				sp_cur->sw_toofew_inodes = 1;
		}
	}

	/*
	 * Now run through the list and print out all of the remaining file systems
	 */

	(void) fprintf(fp, "\n");
	(void) fprintf(fp, "%s\n",
	    dgettext("SUNW_INSTALL_LIBSVC",
		"Remaining file systems."));
	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;

		if (sp[i]->fsp_fsi == NULL) {
			continue;
		}

		if (sp[i]->fsp_flags & FS_INSUFFICIENT_SPACE)
			continue;

/*
		if (!(sp[i]->fsp_flags & FS_HAS_PACKAGED_DATA))
			continue;
*/

		inode_flag = ' ';

		if (sp[i]->fsp_cts.contents_inodes_used >
		    sp[i]->fsp_fsi->f_files) {
			inode_flag = '*';
			inode_err++;
		}
		if (fp) {
			if ((int)strlen(sp[i]->fsp_mntpnt) > (int) 19) {
				(void) fprintf(fp, "%s\n", sp[i]->fsp_mntpnt);
				(void) fprintf(fp,
				    "%-20s%-20s%-20ld%-18ld%c\n", "",
				    sp[i]->fsp_fsi->fsi_device,
				    sp[i]->fsp_cur_slice_size,
				    sp[i]->fsp_reqd_slice_size, inode_flag);
	
			} else {
				(void) fprintf(fp, "%-20s%-20s%-20ld%-18ld%c\n",
				    sp[i]->fsp_mntpnt,
				    sp[i]->fsp_fsi->fsi_device,
				    sp[i]->fsp_cur_slice_size,
				    sp[i]->fsp_reqd_slice_size, inode_flag);
			}
		}
		if (output_struct) {
			sp_tmp = (SW_space_results *) xmalloc((size_t) sizeof
			    (SW_space_results));
			sp_tmp->next = NULL;
			if (sp_head == NULL) {
				sp_head = sp_tmp;
				sp_cur = sp_tmp;
			} else {
				sp_cur->next = sp_tmp;
				sp_cur = sp_tmp;
			}
			sp_cur->sw_mountpnt =
			    xstrdup(sp[i]->fsp_mntpnt);
			sp_cur->sw_devname =
			    xstrdup(sp[i]->fsp_fsi->fsi_device);
			sp_cur->sw_cursiz = sp[i]->fsp_cur_slice_size;
			sp_cur->sw_newsiz = sp[i]->fsp_reqd_slice_size;
			if (inode_flag == ' ')
				sp_cur->sw_toofew_inodes = 0;
			else
				sp_cur->sw_toofew_inodes = 1;
		}
	}

	if (fp && inode_err) {
		(void) fprintf(fp, "\n%s\n", dgettext("SUNW_INSTALL_LIBSVC",
"Slices marked with a '*' have an insufficient number of inodes available."));
		(void) fprintf(fp, "%s\n",
			dgettext("SUNW_INSTALL_LIBSVC",
"See newfs(1M) for details on how to increase the number of inodes per slice."));
	}
	if (fp)
		(void) fclose(fp);
	return (sp_head);
}
