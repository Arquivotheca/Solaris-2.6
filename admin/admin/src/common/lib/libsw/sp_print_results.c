#ifndef lint
#pragma ident   "@(#)sp_print_results.c 1.29 96/02/08 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.
 * All rights reserved.
 */
#include "fcntl.h"
#include "disk_lib.h"
#include "sw_lib.h"
#include "sw_space.h"

/*	Public Function Prototypes	*/

void	swi_print_final_results(char *);
SW_space_results	*swi_gen_final_space_report();

/*	Static function prototyptes 	*/
SW_space_results	*_final_results(char *, int);

extern	Space		**upg_stab;

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
swi_print_final_results(char * outfile)
{
	(void) _final_results(outfile, 0);
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
swi_gen_final_space_report()
{
	return (_final_results(NULL, 1));
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

SW_space_results *
_final_results(char * outfile, int output_struct)
{
	int		i, slice, inode_err;
	FILE		*fp = NULL;
	Disk_t		*dp;
	char		*device;
	daddr_t		cur_size, new_size;
	char		inode_flag;
	SW_space_results	*sp_head = NULL;
	SW_space_results	*sp_cur, *sp_tmp;

	if (upg_stab == NULL) {
		return (NULL);
	}

	if (outfile) {
		fp = fopen(outfile, "w");
		if (fp == (FILE *) NULL) {
			return (NULL);
		}
		chmod(outfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}

	if (fp) {
		/*
		 * Print Current slice size and suggested slice size.
		 */
		(void) fprintf(fp, "%-20s%-20s%-20s%-20s\n",
			"", "",
			dgettext("SUNW_INSTALL_SWLIB", "Current Size"),
			dgettext("SUNW_INSTALL_SWLIB", "Minimum Suggested"));

		(void) fprintf(fp, "%-20s%-20s%-20s%-20s\n",
			dgettext("SUNW_INSTALL_SWLIB", "Mount Point"),
			dgettext("SUNW_INSTALL_SWLIB", "Slice"),
			dgettext("SUNW_INSTALL_SWLIB", "1 Kilobyte Blocks"),
			dgettext("SUNW_INSTALL_SWLIB", "1 Kilobyte Blocks"));

		(void) fprintf(fp,
"-------------------------------------------------------------------------------\n");
	}

	inode_err = 0;
	set_units(D_KBYTE);

	for (i = 0; upg_stab[i]; i++) {
		if (upg_stab[i]->fsi == NULL) {
			continue;
		}

		if (upg_stab[i]->touched == 0)
			continue;

		WALK_DISK_LIST(dp) {
			if (disk_not_okay(dp))
				continue;

			inode_flag = ' ';
			device = upg_stab[i]->fsi->device;

			if (strstr(device, disk_name(dp)) == 0)
				continue;

			slice = (int) atoi(device + (strlen(device) - 1));
			cur_size = blocks2size(dp, orig_slice_size(dp, slice), 1);

			new_size = new_slice_size((u_long)upg_stab[i]->bused,
					upg_stab[i]->fsi->su_only);
			new_size = size2blocks(dp, new_size);
			new_size = blocks2size(dp, new_size, 1);

			/*
			 * If this partitions is one for which there was
			 * insufficient space, make sure new_size is always
			 * greater than cur_size.
			 */
			if (upg_stab[i]->bused > tot_bavail(upg_stab, i) &&
			    new_size <= cur_size) {
				while (new_size <= cur_size) {
					new_size++;
					new_size = size2blocks(dp, new_size);
					new_size = blocks2size(dp, new_size, 1);
				}
			}

			if (upg_stab[i]->fused > upg_stab[i]->fsi->f_files) {
				inode_flag = '*';
				inode_err++;
			}
			if (fp) {
				if ((int)strlen(upg_stab[i]->mountp) > (int) 19) {
					(void) fprintf(fp, "%s\n",
					    upg_stab[i]->mountp);
					(void) fprintf(fp,
					    "%-20s%-20s%-20ld%-18ld%c\n",
					    "", upg_stab[i]->fsi->device,
					    cur_size, new_size, inode_flag);
	
				} else {
					(void) fprintf(fp,
					    "%-20s%-20s%-20ld%-18ld%c\n",
					    upg_stab[i]->mountp,
					    upg_stab[i]->fsi->device,
					    cur_size, new_size, inode_flag);
				}
			}
			if (output_struct) {
				sp_tmp = (SW_space_results *)
				    xmalloc((size_t) sizeof
				    (SW_space_results));
				sp_tmp->next = NULL;
				if (sp_head == NULL) {
					sp_head = sp_tmp;
					sp_cur = sp_tmp;
				} else {
					sp_cur->next = sp_tmp;
					sp_cur = sp_tmp;
				}
				sp_cur->sw_mountpnt = xstrdup(upg_stab[i]->mountp);
				sp_cur->sw_devname =
				    xstrdup(upg_stab[i]->fsi->device);
				sp_cur->sw_cursiz = cur_size;
				sp_cur->sw_newsiz = new_size;
				if (inode_flag == ' ')
					sp_cur->sw_toofew_inodes = 0;
				else
					sp_cur->sw_toofew_inodes = 1;
			}
		}
	}

	if (fp && inode_err) {
		(void) fprintf(fp, "\n%s\n", dgettext("SUNW_INSTALL_SWLIB",
"Slices marked with a '*' have an insufficient number of inodes available."));
		(void) fprintf(fp, "%s\n", dgettext("SUNW_INSTALL_SWLIB",
"See newfs(1M) for details on how to increase the number of inodes per slice."));
	}
	if (fp)
		(void) fclose(fp);
	return (sp_head);
}
