#ifndef lint
#pragma ident "@(#)soft_platform.c 1.2 96/06/06 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc. All Rights Reserved. Sun
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
#include "spmisoft_lib.h"

#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "soft_templates.h"

/* Public Function Prototypes */
int swi_write_platform_file(char *, Module *);

/* Library FUnction Prototypes */
int load_platforms(Module *);
void load_installed_platforms(Module *);
void upg_write_platform_file(FILE *, char *, Product *, Product *);
void upg_write_plat_softinfo(FILE *, Product *, Product *);

/* Internal FUnctions */
static void process_error(void);

static char *keywords[] = {
	"CONFIGURATION_ID",		/*   0 */
	"CFG_MEMBER",			/*   1 */
	"AUTOCONFIG",			/*   2 */
	"PLATFORM_GROUP",		/*   3 */
	"PLATFORM_SWCFG",		/*   4 */
	"PLATFORM_SWCFG_ALL",		/*   5 */
	"INST_ARCH",			/*   6 */
	"EXPORT",			/*   7 */
	"PLATFORM_NAME",		/*   8 */
	"PLATFORM_ID",			/*   9 */
	"MACHINE_TYPE",			/*  10 */
	"IN_PLATFORM_GROUP",		/*  11 */
	"HW_NODENAME",			/*  12 */
	"HW_TESTPROG",			/*  13 */
	"HW_SUPPORT_PKG",		/*  14 */
	"HW_TESTARG",			/*  15 */
	"EOF",				/*  16 */
	""
};

enum parse_state {
	NOT_IN_STANZA,
	IN_SW_CONFIG,
	IN_PLATGRP,
	IN_PLATDEF,
	IN_HW_NODE,
	IN_HW_TEST,
	DONE
};

enum st_action {
	SYNTAX_ERROR = 0,
	NONE,
	ALLOC_SWCFG,
	SAVE_SWCFG,
	CFG_MEMBER,
	DO_AUTOCONFIG,
	ALLOC_PLTGRP,
	SAVE_PLTGRP,
	PLTGRP_CFG,
	PLTGRP_CFG_ALL,
	PLTGRP_ISA,
	PLTGRP_EXPORT,
	ALLOC_PLAT,
	SAVE_PLAT,
	PLAT_UNAME_ID,
	PLAT_MACHINE,
	GROUP_MEMBER,
	PLAT_CFG,
	PLAT_CFG_ALL,
	PLAT_ISA,
	ALLOC_HWNODE,
	ALLOC_HWTEST,
	SAVE_HW,
	SET_HW_PKG,
	SET_HWTEST_ARG,
	STOP
};

struct state_table_entry {
	enum st_action		action1;
	enum st_action		action2;
	enum parse_state	next_state;
};

static struct state_table_entry ste[][6] =
	{			/* ACTION1,	ACTION2,	NEXT_STATE */
/*CONFIG_ID     */	
	/*NOT_IN_STANZA*/	ALLOC_SWCFG,	NONE,		IN_SW_CONFIG,
	/*IN_SW_CONFIG */	SAVE_SWCFG,	ALLOC_SWCFG,	IN_SW_CONFIG,
	/* others      */	0,0,0,  0,0,0,  0,0,0, 0,0,0,
/*CFG_MEMBER    */
	/*NOT_IN_STANZA*/	0,0,0,
	/*IN_SW_CONFIG */	CFG_MEMBER,	NONE,		IN_SW_CONFIG,
	/* others      */	0,0,0,  0,0,0,  0,0,0, 0,0,0,
/*AUTOCONFIG    */
	/*NOT_IN_STANZA*/	0,0,0,
	/*IN_SW_CONFIG */	DO_AUTOCONFIG,	NONE,		IN_SW_CONFIG,
	/* others      */	0,0,0,  0,0,0,  0,0,0, 0,0,0,
/*PLAT_GROUP    */
	/*NOT_IN_STANZA*/	ALLOC_PLTGRP,	NONE,		IN_PLATGRP,
	/*IN_SW_CONFIG */	SAVE_SWCFG,	ALLOC_PLTGRP,	IN_PLATGRP,
	/*IN_PLATGRP   */	SAVE_PLTGRP,	ALLOC_PLTGRP,	IN_PLATGRP,
	/*IN_PLATDEF   */	SAVE_PLAT,	ALLOC_PLTGRP,	IN_PLATGRP,
	/*IN_HW_NODE   */	SAVE_HW,	ALLOC_PLTGRP,	IN_PLATGRP,
	/*IN_HW_TEST   */	SAVE_HW,	ALLOC_PLTGRP,	IN_PLATGRP,
/*PLAT_SWCFG    */
	/*NOT_IN_STANZA, IN_SW_CONFIG */	0,0,0, 0,0,0,
	/*IN_PLATGRP   */	PLTGRP_CFG,	NONE,		IN_PLATGRP,
	/*IN_PLATDEF   */	PLAT_CFG,	NONE,		IN_PLATDEF,
	/* others      */	0,0,0,  0,0,0,
/*PLAT_SWCFG_ALL*/
	/*NOT_IN_STANZA, IN_SW_CONFIG */	0,0,0, 0,0,0,
	/*IN_PLATGRP   */	PLTGRP_CFG_ALL,	NONE,		IN_PLATGRP,
	/*IN_PLATDEF   */	PLAT_CFG_ALL,	NONE,		IN_PLATDEF,
	/* others      */	0,0,0,  0,0,0,
/*INST_ARCH     */
	/*NOT_IN_STANZA, IN_SW_CONFIG */	0,0,0, 0,0,0,
	/*IN_PLATGRP   */	PLTGRP_ISA,	NONE,		IN_PLATGRP,
	/*IN_PLATDEF   */	PLAT_ISA,	NONE,		IN_PLATDEF,
	/* others      */	0,0,0,  0,0,0,
/*EXPORT        */
	/*NOT_IN_STANZA, IN_SW_CONFIG */	0,0,0, 0,0,0,
	/*IN_PLATGRP   */	PLTGRP_EXPORT,	NONE,		IN_PLATGRP,
	/* others      */	0,0,0,  0,0,0,  0,0,0,
/*PLATFORM_NAME */
	/*NOT_IN_STANZA*/	ALLOC_PLAT,	NONE,		IN_PLATDEF,
	/*IN_SW_CONFIG */	SAVE_SWCFG,	ALLOC_PLAT,	IN_PLATDEF,
	/*IN_PLATGRP   */	SAVE_PLTGRP,	ALLOC_PLAT,	IN_PLATDEF,
	/*IN_PLATDEF   */	SAVE_PLAT,	ALLOC_PLAT,	IN_PLATDEF,
	/*IN_HW_NODE   */	SAVE_HW,	ALLOC_PLAT,	IN_PLATDEF,
	/*IN_HW_TEST   */	SAVE_HW,	ALLOC_PLAT,	IN_PLATDEF,
/*PLATFORM_ID   */
	/* others      */	0,0,0,  0,0,0,  0,0,0,
	/*IN_PLATDEF   */	PLAT_UNAME_ID,	NONE,		IN_PLATDEF,
	/* others      */	0,0,0,  0,0,0,
/*MACHINE_TYPE  */
	/* others      */	0,0,0,  0,0,0,  0,0,0,
	/*IN_PLATDEF   */	PLAT_MACHINE,	NONE,		IN_PLATDEF,
	/* others      */	0,0,0,  0,0,0,
/*IN_PLAT_GROUP */
	/* others      */	0,0,0,  0,0,0,  0,0,0,
	/*IN_PLATDEF   */	GROUP_MEMBER,	NONE,		IN_PLATDEF,
	/* others      */	0,0,0,  0,0,0,
/*HW_NODENAME   */
	/*NOT_IN_STANZA*/	ALLOC_HWNODE,	NONE,		IN_HW_NODE,
	/*IN_SW_CONFIG */	SAVE_SWCFG,	ALLOC_HWNODE,	IN_HW_NODE,
	/*IN_PLATGRP   */	SAVE_PLTGRP,	ALLOC_HWNODE,	IN_HW_NODE,
	/*IN_PLATDEF   */	SAVE_PLAT,	ALLOC_HWNODE,	IN_HW_NODE,
	/*IN_HW_NODE   */	SAVE_HW,	ALLOC_HWNODE,	IN_HW_NODE,
	/*IN_HW_TEST   */	SAVE_HW,	ALLOC_HWNODE,	IN_HW_NODE,
/*HW_TESTPROG   */
	/*NOT_IN_STANZA*/	ALLOC_HWTEST,	NONE,		IN_HW_TEST,
	/*IN_SW_CONFIG */	SAVE_SWCFG,	ALLOC_HWTEST,	IN_HW_TEST,
	/*IN_PLATGRP   */	SAVE_PLTGRP,	ALLOC_HWTEST,	IN_HW_TEST,
	/*IN_PLATDEF   */	SAVE_PLAT,	ALLOC_HWTEST,	IN_HW_TEST,
	/*IN_HW_NODE   */	SAVE_HW,	ALLOC_HWTEST,	IN_HW_TEST,
	/*IN_HW_TEST   */	SAVE_HW,	ALLOC_HWTEST,	IN_HW_TEST,
/*HW_SUPPORT_PKG*/
	/* others      */	0,0,0,  0,0,0,  0,0,0,  0,0,0,
	/*IN_HW_NODE   */	SET_HW_PKG,	NONE,		IN_HW_NODE,
	/*IN_HW_TEST   */	SET_HW_PKG,	NONE,		IN_HW_TEST,
/*HW_TESTARG    */
	/* others      */	0,0,0,  0,0,0,  0,0,0,  0,0,0,  0,0,0,
	/*IN_HW_TEST   */	SET_HWTEST_ARG,	NONE,		IN_HW_TEST,
/*EOF */
	/*NOT_IN_STANZA*/	NONE,		STOP,		DONE,
	/*IN_SW_CONFIG */	SAVE_SWCFG,	STOP,		DONE,
	/*IN_PLATGRP   */	SAVE_PLTGRP,	STOP,		DONE,
	/*IN_PLATDEF   */	SAVE_PLAT,	STOP,		DONE,
	/*IN_HW_NODE   */	SAVE_HW,	STOP,		DONE,
	/*IN_HW_TEST   */	SAVE_HW,	STOP,		DONE
};

static SW_config	*g_swcfg = NULL;
static Platform		*g_plat = NULL;
static PlatGroup	*g_platgrp = NULL;
static HW_config	*g_hwcfg = NULL;

static SW_config	*temp_swcfg_head = NULL;
static Platform		*temp_plat_head = NULL;
static PlatGroup	*temp_platgrp_head = NULL;
static HW_config	*temp_hwcfg_head = NULL;

static SW_config	*swcfg_head = NULL;
static PlatGroup	*platgrp_head = NULL;
static HW_config	*hwcfg_head = NULL;

/* Internal Function Prototypes */
static int do_action(enum st_action, char *, int);
static int read_platform_file(char *, char *, int);
static void build_mergeplatlist(Product *);
static void build_mergeplatlist_selected(Product *);
static void _mergeplatlist(PlatGroup *);

/* ******************************************************************** */
/*			PUBLIC FUNCTIONS				*/
/* ******************************************************************** */

/*
 * load_platforms()
 * Parameters:
 *	prod	-
 * Return:
 * Status:
 *	private
 */
int
load_platforms(Module * prod)
{
	struct dirent	*dp;
	DIR		*dirp;
	char		buf[MAXPATHLEN];
	PlatGroup	*pgp, **pgpp;

	if (prod == NULL ||
			(prod->type != PRODUCT && prod->type != NULLPRODUCT))
		return (ERR_INVALIDTYPE);

	if (prod->info.prod->p_pkgdir == NULL)
		return (ERR_INVALID);

	(void) strcpy(buf, prod->info.prod->p_pkgdir);
	(void) strcat(buf, "/.platform");
	dirp = opendir(buf);
	if (dirp == (DIR *)0)
		return (ERR_NODIR);

	while ((dp = readdir(dirp)) != (struct dirent *)0)
		if (streq(dp->d_name, "Solaris"))
			break;
	if (dp == NULL)
		return (ERR_NOPROD);
	if (read_platform_file(buf, dp->d_name, TRUE) != 0)
		return (ERR_NOPROD);
	(void) closedir(dirp);
	dirp = opendir(buf);
	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (streq(dp->d_name, ".") || streq(dp->d_name, "..") ||
		    streq(dp->d_name, "Solaris"))
			continue;
		if (read_platform_file(buf, dp->d_name, FALSE) != 0) {
			process_error();
			(void) closedir(dirp);
			return (ERR_INVALID);
		}
	}
	(void) closedir(dirp);
	prod->info.prod->p_swcfg = swcfg_head;
	swcfg_head = NULL;
	/*
	 * Verify that all platform groups have at least one member.
	 * Delete any that don't.
	 */
	pgpp = &platgrp_head;
	while (*pgpp != (PlatGroup *)NULL) {
		pgp = *pgpp;
		if (pgp->pltgrp_members == NULL) {
			*pgpp = pgp->next;
			pgp->next = NULL;	/* Do this to avoid freeing */
						/* the whole chain */
			free_platgroup(pgp);
		} else
			pgpp = &(pgp->next);
	}
	prod->info.prod->p_platgrp = platgrp_head;
	platgrp_head = NULL;

	prod->info.prod->p_hwcfg = hwcfg_head;
	hwcfg_head = NULL;
	return (0);
}

/*
 * load_installed_platforms()
 * Parameters:
 *	prod	-
 * Return:
 * Status:
 *	private
 */
void
load_installed_platforms(Module * prod)
{
	char		buf[MAXPATHLEN];
	PlatGroup	*pgp, **pgpp;

	if (prod == NULL ||
			(prod->type != PRODUCT && prod->type != NULLPRODUCT))
		return;

	if (prod->info.prod->p_rootdir == NULL)
		return;

	(void) strcpy(buf, prod->info.prod->p_rootdir);
	(void) strcat(buf, "/var/sadm/system/admin");
	if (read_platform_file(buf, ".platform", TRUE) != 0)
		return;
	prod->info.prod->p_swcfg = swcfg_head;
	swcfg_head = NULL;
	/*
	 * Verify that all platform groups have at least one member.
	 * Delete any that don't.
	 */
	pgpp = &platgrp_head;
	while (*pgpp != (PlatGroup *)NULL) {
		pgp = *pgpp;
		if (pgp->pltgrp_members == NULL) {
			*pgpp = pgp->next;
			pgp->next = NULL;	/* Do this to avoid freeing */
						/* the whole chain */
			free_platgroup(pgp);
		} else
			pgpp = &(pgp->next);
	}
	prod->info.prod->p_platgrp = platgrp_head;
	platgrp_head = NULL;

	prod->info.prod->p_hwcfg = hwcfg_head;
	hwcfg_head = NULL;
	return;
}

/*
 * write_platform_file()
 * Parameters:
 *	rootdir	-
 */
int
swi_write_platform_file(char *rootdir, Module *prod)
{
	Arch	*ap;
	PlatGroup *pgp;
	Platform *pp;
	char	*cp;
	int	len;
	FILE	*fp = (FILE *) NULL;
	char	path[MAXPATHLEN];

	if (!prod->info.prod->p_platgrp)
		return (SUCCESS);
	(void) sprintf(path, "%s/var/sadm/system/admin/.platform", rootdir);
	for (ap = prod->info.prod->p_arches; ap; ap = ap->a_next) {
		/* is arch not selected, continue */
		if (!ap->a_selected)
			continue;
		cp = strchr(ap->a_arch, '.');
		len = cp - ap->a_arch;
		cp++;
		for (pgp = prod->info.prod->p_platgrp; pgp; pgp = pgp->next) {
			/* if platgroup doesn't match, continue */
			if (!strneq(pgp->pltgrp_isa, ap->a_arch, len) ||
			    !streq(pgp->pltgrp_name, cp))
				continue;

			/* platgroup matches selected architecture */

			/* open file, if not already open */
			if (!fp) {
				if ((fp = fopen(path, "a")) == (FILE *) NULL)
					(void) chmod(path,
					    S_IRUSR | S_IWUSR | S_IRGRP |
					    S_IROTH);
			}

			(void) fprintf(fp, "PLATFORM_GROUP=%s\n",
			    pgp->pltgrp_name);
			(void) fprintf(fp, "INST_ARCH=%s\n", pgp->pltgrp_isa);

			/* write out individual platform entries */
			for (pp = pgp->pltgrp_members; pp; pp = pp->next) {
				(void) fprintf(fp, "PLATFORM_NAME=%s\n",
				    pp->plat_name);
				if (pp->plat_uname_id)
					(void) fprintf(fp, "PLATFORM_ID=%s\n",
					    pp->plat_uname_id);
				if (pp->plat_machine)
					(void) fprintf(fp, "MACHINE_TYPE=%s\n",
					    pp->plat_machine);
				(void) fprintf(fp, "IN_PLATFORM_GROUP=%s\n",
				    pgp->pltgrp_name);
			}
		}
	}
	if (fp)
		fclose(fp);
	return (SUCCESS);
}

static PlatGroup *merge_pltgrp = NULL;

/*
 * upg_write_platform_file()
 * Parameters:
 *	rootdir	-
 */
void
upg_write_platform_file(FILE *fp, char *rootdir, Product *prod1,
    Product *prod2)
{
	PlatGroup *pgp;
	Platform *pp;
	char	buf[BUFSIZ + 1];
	int	started = 0;

	build_mergeplatlist(prod1);
	build_mergeplatlist(prod2);

	for (pgp = merge_pltgrp; pgp; pgp = pgp->next) {

		if (!started) {
			scriptwrite(fp, LEVEL0, start_platform,
			    "ROOT", rootdir, (char *)0);
			started = 1;
		}
		(void) sprintf(buf, "PLATFORM_GROUP=%s", pgp->pltgrp_name);
		scriptwrite(fp, LEVEL1, generic, "LINE", buf, (char *)0);
		(void) sprintf(buf, "INST_ARCH=%s", pgp->pltgrp_isa);
		scriptwrite(fp, LEVEL1, generic, "LINE", buf, (char *)0);

		/* write out individual platform entries */
		for (pp = pgp->pltgrp_members; pp; pp = pp->next) {
			(void) sprintf(buf, "PLATFORM_NAME=%s", pp->plat_name);
			scriptwrite(fp, LEVEL1, generic,
			    "LINE", buf, (char *)0);
			if (pp->plat_uname_id) {
				(void) sprintf(buf, "PLATFORM_ID=%s",
				    pp->plat_uname_id);
				scriptwrite(fp, LEVEL1, generic, "LINE", buf,
				    (char *)0);
			}
			if (pp->plat_machine) {
				(void) sprintf(buf, "MACHINE_TYPE=%s",
				    pp->plat_machine);
				scriptwrite(fp, LEVEL1, generic, "LINE", buf,
				    (char *)0);
			}
			(void) sprintf(buf, "IN_PLATFORM_GROUP=%s",
			    pgp->pltgrp_name);
			scriptwrite(fp, LEVEL1, generic,
			    "LINE", buf, (char *)0);
		}
	}
	if (started)
		scriptwrite(fp, LEVEL0, end_platform_file, (char *)0);
	free_platgroup(merge_pltgrp);
	merge_pltgrp = NULL;
}

/*
 * upg_write_plat_softinfo()
 * Parameters:
 *	rootdir	-
 */
void
upg_write_plat_softinfo(FILE *fp, Product *prod1, Product *prod2)
{
	PlatGroup *pgp;
	Platform *pp;

	build_mergeplatlist_selected(prod1);
	build_mergeplatlist_selected(prod2);

	for (pgp = merge_pltgrp; pgp; pgp = pgp->next) {

		scriptwrite(fp, LEVEL1, platgrp_softinfo,
		    "ISA", pgp->pltgrp_isa,
		    "PLATGRP", pgp->pltgrp_name, (char *)0);

		for (pp = pgp->pltgrp_members; pp; pp = pp->next) {
			scriptwrite(fp, LEVEL1, platmember_softinfo,
			    "PLAT", pp->plat_name, (char *)0);
		}
	}
	free_platgroup(merge_pltgrp);
	merge_pltgrp = NULL;
}

static void
build_mergeplatlist(Product *prod)
{
	PlatGroup	*pgp;

	if (!prod || !prod->p_platgrp)
		return;
	for (pgp = prod->p_platgrp; pgp; pgp = pgp->next)
		_mergeplatlist(pgp);
}

static void
build_mergeplatlist_selected(Product *prod)
{
	Arch	*ap;
	char	*cp;
	int	len;
	PlatGroup	*pgp;

	if (!prod || !prod->p_platgrp)
		return;
	for (ap = prod->p_arches; ap; ap = ap->a_next) {
		/* is arch not selected, continue */
		if (!ap->a_selected)
			continue;
		cp = strchr(ap->a_arch, '.');
		len = cp - ap->a_arch;
		cp++;
		for (pgp = prod->p_platgrp; pgp; pgp = pgp->next) {
			/* if platgroup doesn't match, continue */
			if (!strneq(pgp->pltgrp_isa, ap->a_arch, len) ||
			    !streq(pgp->pltgrp_name, cp))
				continue;

			/* platgroup matches selected architecture */

			_mergeplatlist(pgp);
		}
	}
}

static void
_mergeplatlist(PlatGroup *pgp)
{
	PlatGroup	*tpgp;
	Platform	*pp, *tpp;

	for (tpgp = merge_pltgrp; tpgp; tpgp = tpgp->next)
		if streq(pgp->pltgrp_name, tpgp->pltgrp_name)
			break;

	if (!tpgp) {
		tpgp = (PlatGroup *)xcalloc((size_t) sizeof (PlatGroup));
		tpgp->pltgrp_name = xstrdup(pgp->pltgrp_name);
		tpgp->pltgrp_isa = xstrdup(pgp->pltgrp_isa);
		link_to((Item **)&merge_pltgrp, (Item *)tpgp);
	}

	for (pp = pgp->pltgrp_members; pp; pp = pp->next) {
		for (tpp = tpgp->pltgrp_members; tpp; tpp = tpp->next)
			if (streq(pp->plat_name, tpp->plat_name))
				break;
		if (!tpp) {
			tpp = (Platform *)xcalloc((size_t) sizeof (Platform));
			tpp->plat_name = xstrdup(pp->plat_name);
			if (pp->plat_uname_id)
				tpp->plat_uname_id = xstrdup(
				    pp->plat_uname_id);
			if (pp->plat_machine)
				tpp->plat_machine = xstrdup (pp->plat_machine);
			link_to((Item **)&tpgp->pltgrp_members, (Item *)tpp);
		}
	}
}
/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * read_platform_file()
 * Parameters:
 *	mod	-
 * Return:
 *
 * Status:
 *	private
 */
static int
read_platform_file(char *dir, char *filename, int is_core)
{

	FILE		*fp;
	char		buf[BUFSIZ + 1];
	char		key[BUFSIZ];
	char		path[MAXPATHLEN];
	char		*cp;
	int		 i, len, status;
	enum parse_state scanstate = NOT_IN_STANZA;
	struct state_table_entry *ste_p;
	Platform	*plat;
	PlatGroup	*platgrp;

	(void) strcpy(path, dir);
	(void) strcat(path, "/");
	(void) strcat(path, filename);

	if ((fp = fopen(path, "r")) == (FILE *) NULL)
		return (ERR_NOFILE);

	while (scanstate != DONE) {
		if (fgets(buf, BUFSIZ, fp)) {
			buf[strlen(buf) - 1] = '\0';

			if (buf[0] == '#' || buf[0] == '\n')
				continue;

			/* copy the keyword to key */
			if ((cp = strchr(buf, '=')) == NULL) {
				process_error();
				fclose(fp);
				return (ERR_INVALID);
			}
			len = cp - buf;
			(void) strncpy(key, buf, len);
			key[len] = '\0';
		} else
			(void) strcpy(key, "EOF");

		/* find the keyword in the keyword array */
		for (i = 0; *keywords[i]; i++)
			if (streq(key, keywords[i]))
				break;

		if (*keywords[i] == '\0') {
			process_error();
			fclose(fp);
			return (ERR_INVALID);
		}

		/* get the action/new_state struct */
		ste_p = &ste[i][scanstate];

		/* execute the first action */
		if ((status = do_action(ste_p->action1, buf, is_core)) != 0) {
			process_error();
			fclose(fp);
			return (status);
		}

		/* execute the second action */
		if ((status = do_action(ste_p->action2, buf, is_core)) != 0) {
			process_error();
			fclose(fp);
			return (status);
		}
		/* go to the new state */
		scanstate = ste_p->next_state;
	}
	fclose(fp);
	/*
	 *  For each platform on the temp_plat_head queue, dequeue
	 *  and add it to the correct platform group's member list.
	 *  If the platform isn't a member of any group, give it its
	 *  own platform group.
	 */
	while (temp_plat_head) {
		/* detach platform from head of queue */
		plat = temp_plat_head;
		temp_plat_head = plat->next;
		plat->next = NULL;

		if (plat->plat_group) {
			platgrp = platgrp_head;
			while (platgrp) {
				if (platgrp->pltgrp_export &&
				    streq(plat->plat_group,
				    platgrp->pltgrp_name)) {
					free(plat->plat_group);
					plat->plat_group = NULL;
					link_to((Item **)
					    &platgrp->pltgrp_members,
					    (Item *) plat);
					plat = NULL;
					break;
				}
				platgrp = platgrp->next;
			}
			if (platgrp)
				break;
			free(plat);
			process_error();
			/* platform group isn't defined */
			return (ERR_INVALID);
		} else {
			if (!plat->plat_isa) {
				free(plat);
				process_error();
				return (ERR_INVALID);
			}
			platgrp = (PlatGroup *)xcalloc((size_t)
			    sizeof (PlatGroup));
			platgrp->pltgrp_name = xstrdup(plat->plat_name);
			platgrp->pltgrp_config = plat->plat_config;
			platgrp->pltgrp_all_config = plat->plat_all_config;
			platgrp->pltgrp_isa = xstrdup(plat->plat_isa);
			platgrp->pltgrp_members = plat;
			link_to((Item **)&temp_platgrp_head, (Item *)platgrp);
			platgrp = NULL;
		}
	}

	link_to((Item **)&swcfg_head, (Item *)temp_swcfg_head);
	temp_swcfg_head = NULL;
	link_to((Item **)&platgrp_head, (Item *)temp_platgrp_head);
	temp_platgrp_head = NULL;
	link_to((Item **)&hwcfg_head, (Item *)temp_hwcfg_head);
	temp_hwcfg_head = NULL;
	return (0);
}

static int
do_action(enum st_action action, char *buf, int is_core)
{
	StringList	*strlist;
	char		*cp;
	SW_config	*swp;
	PlatGroup	*pgrp;
	static int	dup_pgrp = 0;

	switch (action) {

	case SYNTAX_ERROR:
		return (ERR_INVALID);

	case NONE:
		return (0);

	case ALLOC_SWCFG:
		g_swcfg = (SW_config *)xcalloc((size_t) sizeof (SW_config));
		g_swcfg->sw_cfg_name = xstrdup(get_value(buf, '='));
		return (0);

	case SAVE_SWCFG:
		link_to((Item **)&temp_swcfg_head, (Item *)g_swcfg);
		g_swcfg = NULL;
		return (0);

	case CFG_MEMBER:
		strlist = (StringList *)xcalloc((size_t) sizeof (StringList));
		strlist->string_ptr = xstrdup(get_value(buf, '='));
		link_to((Item **)&g_swcfg->sw_cfg_members, (Item *)strlist);
		strlist = NULL;
		return (0);

	case DO_AUTOCONFIG:
		cp = strchr(buf, '=') + 1;
		if (*cp == 'y' || *cp == 'Y')
			g_swcfg->sw_cfg_auto = TRUE;
		else
			g_swcfg->sw_cfg_auto = FALSE;
		return (0);

	case ALLOC_PLTGRP:
		/*
		 * Check for multiple platform groups, if we find one
		 * set the dup flag and return.
		 */
		cp = get_value(buf, '=');
		dup_pgrp = 0;

		for (pgrp = platgrp_head; pgrp; pgrp = pgrp->next)
			if (streq(cp, pgrp->pltgrp_name)) {
				dup_pgrp = 1;
				return (0);
			}

		for (pgrp = temp_platgrp_head; pgrp; pgrp = pgrp->next)
			if (streq(cp, pgrp->pltgrp_name)) {
				dup_pgrp = 1;
				return (0);
			}

		g_platgrp = (PlatGroup *)xcalloc((size_t) sizeof (PlatGroup));
		g_platgrp->pltgrp_name = xstrdup(get_value(buf, '='));
		if (is_core)
			g_platgrp->pltgrp_export = 1;
		return (0);

	case SAVE_PLTGRP:
		if (dup_pgrp)
			return (0);
		if (!g_platgrp->pltgrp_isa)
			return (ERR_INVALID);
		link_to((Item **)&temp_platgrp_head, (Item *)g_platgrp);
		g_platgrp = NULL;
		return (0);

	case PLTGRP_CFG:
		if (dup_pgrp)
			return (0);
		if (g_platgrp->pltgrp_config)
			return (ERR_INVALID);
		cp = strchr(buf, '=') + 1;
		for (swp = temp_swcfg_head; swp; swp = swp->next)
			if (streq(cp, swp->sw_cfg_name)) {
				g_platgrp->pltgrp_config = swp;
				break;
			}
		if (swp == NULL)
			return (ERR_INVALID);
		return (0);

	case PLTGRP_CFG_ALL:
		if (dup_pgrp)
			return (0);
		if (g_platgrp->pltgrp_all_config)
			return (ERR_INVALID);
		cp = strchr(buf, '=') + 1;
		for (swp = temp_swcfg_head; swp; swp = swp->next)
			if (streq(cp, swp->sw_cfg_name)) {
				g_platgrp->pltgrp_all_config = swp;
				break;
			}
		if (swp == NULL)
			return (ERR_INVALID);
		return (0);

	case PLTGRP_ISA:
		if (dup_pgrp)
			return (0);
		if (g_platgrp->pltgrp_isa)
			return (ERR_INVALID);
		cp = strchr(buf, '=') + 1;
		g_platgrp->pltgrp_isa = xstrdup(cp);
		return (0);

	case PLTGRP_EXPORT:
		if (dup_pgrp)
			return (0);
		cp = strchr(buf, '=') + 1;
		if (*cp == 'y' || *cp == 'Y') {
			if (!is_core)
				return (ERR_INVALID);
			g_platgrp->pltgrp_export = 1;
		}
		return (0);

	case ALLOC_PLAT:
		g_plat = (Platform *)xcalloc((size_t) sizeof (Platform));
		g_plat->plat_name = xstrdup(get_value(buf, '='));
		return (0);

	case SAVE_PLAT:
		if (g_plat->plat_uname_id == NULL &&
		    g_plat->plat_machine == NULL)
			return (ERR_INVALID);
		if (g_plat->plat_group) {
			for (pgrp = temp_platgrp_head; pgrp; pgrp = pgrp->next)
				if (streq(g_plat->plat_group,
				    pgrp->pltgrp_name)) {
					free(g_plat->plat_group);
					g_plat->plat_group = NULL;
					link_to((Item **)&pgrp->pltgrp_members,
					    (Item *)g_plat);
					g_plat = NULL;
					break;
				}
		}
		if (g_plat) {
			link_to((Item **)&temp_plat_head, (Item *)g_plat);
			g_plat = NULL;
		}
		return (0);

	case PLAT_UNAME_ID:
		if (g_plat->plat_uname_id)
			return (ERR_INVALID);
		g_plat->plat_uname_id = xstrdup(get_value(buf, '='));
		return (0);

	case PLAT_MACHINE:
		if (g_plat->plat_machine)
			return (ERR_INVALID);
		g_plat->plat_machine = xstrdup(get_value(buf, '='));
		return (0);

	case GROUP_MEMBER:
		if (g_plat->plat_group)
			return (ERR_INVALID);
		g_plat->plat_group = xstrdup(get_value(buf, '='));
		return (0);

	case PLAT_CFG:
		if (g_plat->plat_config)
			return (ERR_INVALID);
		cp = strchr(buf, '=') + 1;
		for (swp = temp_swcfg_head; swp; swp = swp->next)
			if (streq(cp, swp->sw_cfg_name)) {
				g_plat->plat_config = swp;
				break;
			}
		if (swp == NULL)
			return (ERR_INVALID);
		return (0);

	case PLAT_CFG_ALL:
		if (g_plat->plat_all_config)
			return (ERR_INVALID);
		cp = strchr(buf, '=') + 1;
		for (swp = temp_swcfg_head; swp; swp = swp->next)
			if (streq(cp, swp->sw_cfg_name)) {
				g_plat->plat_all_config = swp;
				break;
			}
		if (swp == NULL)
			return (ERR_INVALID);
		return (0);

	case PLAT_ISA:
		if (g_plat->plat_isa)
			return (ERR_INVALID);
		g_plat->plat_isa = xstrdup(get_value(buf, '='));
		return (0);

	case ALLOC_HWNODE:
		g_hwcfg = (HW_config *)xcalloc((size_t) sizeof (HW_config));
		g_hwcfg->hw_node = xstrdup(get_value(buf, '='));
		return (0);

	case ALLOC_HWTEST:
		g_hwcfg = (HW_config *)xcalloc((size_t) sizeof (HW_config));
		g_hwcfg->hw_testprog = xstrdup(get_value(buf, '='));
		return (0);

	case SAVE_HW:
		if (!g_hwcfg->hw_support_pkgs)
			return (ERR_INVALID);
		link_to((Item **)&temp_hwcfg_head, (Item *)g_hwcfg);
		g_hwcfg = NULL;
		return (0);

	case SET_HW_PKG:
		strlist = (StringList *)xcalloc((size_t) sizeof (StringList));
		strlist->string_ptr = xstrdup(get_value(buf, '='));
		link_to((Item **)&g_hwcfg->hw_support_pkgs, (Item *)strlist);
		strlist = NULL;
		return (0);

	case SET_HWTEST_ARG:
		if (g_hwcfg->hw_testarg)
			return (ERR_INVALID);
		g_hwcfg->hw_testarg = xstrdup(get_value(buf, '='));
		return (0);

	case STOP:
		return (0);
	}
	return (0);
}

static void
process_error()
{
	if (g_swcfg) {
		free_sw_config_list(g_swcfg);
		g_swcfg = NULL;
	}
	if (g_plat) {
		free_platform(g_plat);
		g_plat = NULL;
	}
	if (g_platgrp) {
		free_platgroup(g_platgrp);
		g_platgrp = NULL;
	}
	if (g_hwcfg) {
		free_hw_config(g_hwcfg);
		g_hwcfg = NULL;
	}
	if (temp_swcfg_head) {
		free_sw_config_list(temp_swcfg_head);
		temp_swcfg_head = NULL;
	}
	if (temp_plat_head) {
		free_platform(temp_plat_head);
		temp_plat_head = NULL;
	}
	if (temp_platgrp_head) {
		free_platgroup(temp_platgrp_head);
		temp_platgrp_head = NULL;
	}
	if (temp_hwcfg_head) {
		free_hw_config(temp_hwcfg_head);
		temp_hwcfg_head = NULL;
	}
}
