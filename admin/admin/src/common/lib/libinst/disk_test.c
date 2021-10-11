#ifndef lint
#pragma ident   "@(#)disk_test.c 1.56 95/04/20 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#define	DEBUG

#include "disk_api.h"
#include "ibe_api.h"

typedef struct drive {
	char		type[32];
	int		index;
	struct drive	*next;
} Drive;

extern int	optind;
extern char	*optarg;

int	get_drive_selection(Disk_t *, Drive **);

/* standalone for testing without screens.  */
main(int ac, char **av, char **env)
{
	Drive		*dpp, *tp;
	Defmnt_t	ment;
	Disk_t		*dp;
	Disk_t		*first;
	int		x, n, i, j, cnt;
	int		*ap;
	char		c;
	char		*cp;
	int		count = -1;
	int		validate = 0;
	Errmsg_t	*emp;

	while ((c = getopt(ac, av, "ubd:v")) != EOF) {
		switch (c) {
			case 'b' :
				(void) printf(
				    "Building disk list from system configuration\n");
				count = build_disk_list();
				first = first_disk();
				break;
			case 'd' :
				(void) printf("Loading disk list from input file '%s'\n",
					optarg);
				count = load_disk_list(optarg);
				first = first_disk(); 
				break;
			case 'u' :
				(void) printf("Building upgradeable disks\n");
				(void) build_disk_list();
				first = upgradeable_disks();
				count = -1;
				break;
			case 'v' :
				validate++;
				break;
			default  :
				(void) printf("Invalid arguments\n");
				exit(1);
		}
	}

	/*
	for (dp = first_disk(); dp; dp = next_disk(dp)) {
		(void) select_disk(dp, NULL);
		(void) fdisk_config(dp, NULL, CFG_DEFAULT);
		(void) sdisk_config(dp, NULL, CFG_NONE);
		(void) sdisk_config(dp, NULL, CFG_DEFAULT);
		(void) sdisk_use_free_space(dp);
		print_disk(dp, NULL);
		get_dfltmnt_ent(&ment, OPT);
		ment.expansion = 409600;
		set_dfltmnt_ent(&ment, OPT);
		(void) sdisk_config(dp, NULL, CFG_NONE);
		(void) sdisk_config(dp, NULL, CFG_DEFAULT);
		(void) sdisk_use_free_space(dp);
		if ((n = get_drive_selection(dp, &dpp)) > 0)
			for (tp = dpp; tp; tp = tp->next)
				(void) printf("(%d) %s\n", tp->index, tp->type);
		print_disk(dp, NULL);
	}
	*/

	if (first == NULL) {
		(void) printf("No disks found\n");
		exit (0);
	} else if (count > 0)
		(void) printf("%d disk(s) found\n\n", count);

	WALK_DISK_LIST(dp)
		print_disk(dp, NULL);

	/*
	for (dp = first; dp; dp = next_disk(dp)) {
		if (validate) {
			(void) printf("Disk %s\n", disk_name(dp));
			(void) printf("\tReturn from validate_disk: %d\n",
				validate_disk(dp));
			(void) printf("\tReturn from check_disk: %d\n",
				check_disk(dp));
			
			for (emp = get_error_list();
					emp != NULL; emp = emp->next)
				(void) printf("Message %d: %s\n",
					emp->code, emp->msg);
		} else
			print_disk(dp, NULL);
	}
	*/

	/*
		if ((cp = spec_dflt_bootdisk()) == NULL)
			(void) printf("NULL DEFAULT");
		else
			(void) printf("DEFAULT DRIVE: %s\n", cp);
	(void) label_disks(first_disk());
	*/

	/*
	for (dp = first_disk(); dp; dp = next_disk(dp)) {
		print_disk(dp, NULL);
		(void) printf("Unused slice space: %dMB\n",
				sectors_to_mb(sdisk_space_avail(dp)));
		x = validate_disk(dp);
		if (x == D_OK)
			printf("DISK OK\n");
		else if (x < 0)
			printf("ERROR: %s\n", err_text);
		else
			printf("WARNING: %s\n", err_text);
		(void) sdisk_use_free_space(dp);
		print_disk(dp, NULL);
		(void) printf("Unused slice space: %dMB\n",
				sectors_to_mb(sdisk_space_avail(dp)));
		x = validate_disk(dp);
		if (x == D_OK)
			printf("DISK OK\n");
		else if (x < 0)
			printf("ERROR: %s\n", err_text);
		else
			printf("WARNING: %s\n", err_text);
	}
*/
	exit(0);
}
