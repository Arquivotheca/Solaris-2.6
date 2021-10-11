#ifndef lint
#pragma ident   "@(#)pkghist.c 1.17 95/02/08 SMI"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "sw_lib.h"
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>


struct s_histid {
	char *verlo;
	char *verhi;
	char *arch;
};

/* Local Statics and Constants */

#define	VER_LOW		1
#define	VER_HIGH	2

#define	ENTRY_BUF_SIZE	8192

static struct pkg_hist *pkg_history = NULL;
static struct pkg_hist *cls_history = NULL;
static char *		gtoksbuf;
static unsigned int	gtoksbuf_size;
static char *		glinebuf;

/* Public Function Prototypes */

void		read_pkg_history_file(char *);
void		read_cls_history_file(char *);

/* Library Function Prototynes */

/* Local Function Prototypes */

static int 	map_hist_to_pkg(char *, char *, char *, char *);
static void 	attach_pkg_hist(char *, char *, char *, char *, struct
    pkg_hist *);
static int 	map_hist_to_cls(char *, char *, char *);
static void 	attach_cls_hist(char *, char *, char *, struct pkg_hist *);
static int 	is_pkg_installed(Modinfo *, struct s_histid *);
static int 	is_cls_installed(Modinfo *, struct s_histid *);
static void	parse_pkg_entry(char *);
static void	parse_cls_entry(char *);
static char * 	set_token_value(char *, char *);
static char * 	tok_value(char *);
static char *	split_ver(char *, int);
static void	free_hist_ent(struct pkg_hist *);


/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */


/*
 * read_pkg_history_file()
 *
 * Parameters:
 *	path	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
read_pkg_history_file(char * path)
{
	FILE	*fp;
	char	*line, *entry;
	unsigned int	entry_used, entry_size, n;

	if ((fp = fopen(path, "r")) == (FILE*)NULL) {
		return;  /* assume no history, which is OK */
	}

	gtoksbuf = (char *) xmalloc(ENTRY_BUF_SIZE);
	gtoksbuf_size = ENTRY_BUF_SIZE;

	glinebuf = (char *) xcalloc(BUFSIZ);

	line = (char *) xcalloc(BUFSIZ);

	entry = (char *) xcalloc(ENTRY_BUF_SIZE);
	entry_size = ENTRY_BUF_SIZE;
	entry_used = 0;

	while (fgets(line, BUFSIZ, fp)) {

		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;

		if (strstr(line, "PKG=")) {
			parse_pkg_entry(entry);
			(void) memset (entry, '\0', sizeof (entry));
			(void) strcpy(entry, line);
			entry_used = strlen(line);
		} else {
			n = strlen(line);
			if (entry_used + n >= entry_size) {
				entry = xrealloc(entry, entry_size +
				    ENTRY_BUF_SIZE);
				entry_size += ENTRY_BUF_SIZE;
			}
			(void) strcat(entry, line);
			entry_used += n;
		}
	}
	/*
	 * Last one
	 */
	parse_pkg_entry(entry);

	(void) free(line);
	(void) free(entry);
	(void) free(gtoksbuf);
	(void) free(glinebuf);

	fclose(fp);
}

/*
 * read_cls_history_file()
 * Parameters:
 *	path
 * Return:
 *	none
 * Status:
 *	public
 */
void
read_cls_history_file(char *path)
{
	FILE	*fp;
	char	*line, *entry;
	unsigned int	entry_used, entry_size, n;

	if ((fp = fopen(path, "r")) == (FILE*)NULL) {
		return;  /* assume no history, which is OK */
	}

	gtoksbuf = (char *) xmalloc(ENTRY_BUF_SIZE);
	gtoksbuf_size = ENTRY_BUF_SIZE;

	glinebuf = (char *) xcalloc(BUFSIZ);

	line = (char *) xcalloc(BUFSIZ);

	entry = (char *) xcalloc(ENTRY_BUF_SIZE);
	entry_size = ENTRY_BUF_SIZE;
	entry_used = 0;

	while (fgets(line, BUFSIZ, fp)) {

		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;

		if (strstr(line, "CLUSTER=")) {
			parse_cls_entry(entry);
			(void) memset (entry, '\0', sizeof (entry));
			(void) strcpy(entry, line);
			entry_used = strlen(line);
		} else {
			n = strlen(line);
			if (entry_used + n >= entry_size) {
				entry = xrealloc(entry, entry_size +
				    ENTRY_BUF_SIZE);
				entry_size += ENTRY_BUF_SIZE;
			}
			(void) strcat(entry, line);
		}
	}
	/*
	 * Last one
	 */
	parse_cls_entry(entry);

	(void) free (line);
	(void) free (entry);
	(void) free (gtoksbuf);
	(void) free (glinebuf);

	fclose(fp);
}

/*
 * free_history()
 * Parameters:
 *	histp
 * Return:
 *	none
 * Status:
 *	public
 */
void
free_history(struct pkg_hist *histp)
{
	struct pkg_hist **last, *cur;

	if (histp == NULL || --(histp->ref_count) > 0)
		return;

	last = &pkg_history;
	cur = pkg_history;
	while (cur) {
		if (cur == histp) {
			*last = cur->hist_next;
			free_hist_ent(cur);
			return;
		}
		last = &(cur->hist_next);
		cur = cur->hist_next;
	}
	last = &cls_history;
	cur = cls_history;
	while (cur) {
		if (cur == histp) {
			*last = cur->hist_next;
			free_hist_ent(cur);
			return;
		}
		last = &(cur->hist_next);
		cur = cur->hist_next;
	}
	return;
}


/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * parse_cls_entry()
 * Parameters:
 *	entry	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
parse_cls_entry(char *entry)
{
	struct pkg_hist	*ph;
	char	cls_abbr[80], ver_high[80], ver_low[80];
	char	*tok_value;

	if (*entry == '\0')
		return;

	(void) memset(cls_abbr, '\0', sizeof (cls_abbr));
	(void) memset(ver_high, '\0', sizeof (ver_high));
	(void) memset(ver_low, '\0', sizeof (ver_low));

	tok_value = set_token_value("CLUSTER=", entry);
	if (tok_value != NULL)
		(void) strcpy(cls_abbr, tok_value);

	tok_value = set_token_value("VERSION=", entry);
	if (tok_value != NULL) {
		(void) strcpy(ver_low, split_ver(tok_value, VER_LOW));
		(void) strcpy(ver_high, split_ver(tok_value, VER_HIGH));
	}

	if (!map_hist_to_cls(cls_abbr, ver_low, ver_high))
		return;

	ph = (struct pkg_hist *) calloc(sizeof (struct pkg_hist), 1);

	tok_value = set_token_value("REPLACED_BY=", entry);
	if (tok_value != NULL)
		ph->replaced_by = strdup(tok_value);

	if (ph->replaced_by != NULL) {
		if (!strstr(ph->replaced_by, cls_abbr))
			ph->to_be_removed = 1;
	}

	attach_cls_hist(cls_abbr, ver_low, ver_high, ph);

	return;
}


/*
 * set_token_value()
 * Parameters:
 *	tok	-
 *	entry	-
 * Return:
 * Status:
 *	private
 */
static char *
set_token_value(char *tok, char *entry)
{
	char	*cp, *tmp;
	int	tok_len;
	unsigned int	gtoksbuf_used;

	*gtoksbuf = '\0';
	gtoksbuf_used = 0;
	tok_len = strlen(tok);

	for (cp = strstr(entry, tok); cp != NULL;
	    cp = strstr((cp + tok_len), tok)) {
		tmp = tok_value(cp + tok_len);
		if (gtoksbuf_used + strlen(tmp) + 1 >= gtoksbuf_size) {
			gtoksbuf = xrealloc(gtoksbuf, gtoksbuf_size +
			    ENTRY_BUF_SIZE);
			gtoksbuf_size += ENTRY_BUF_SIZE;
		}
		if (*gtoksbuf != '\0') {
			strcat(gtoksbuf, " ");
			gtoksbuf_used++;
		}
		(void) strcat(gtoksbuf, tmp);
		gtoksbuf_used += strlen(tmp);
	}
	if (*gtoksbuf == '\0')
		return (NULL);
	return (gtoksbuf);
}

/*
 * tok_value()
 * Parameters:
 *	cp	-
 * Return:
 * Status:
 *	private
 */
static char *
tok_value(char *cp)
{
	char	*lp;

	lp = glinebuf;

	while (*cp != '\n')
		*lp++ = *cp++;
	*lp = '\0';

	return (glinebuf);
}

/*
 * split_ver()
 * Parameters:
 *	ver	-
 *	flag	-
 * Return:
 *	none
 * Status:
 *	private
 */
static char *
split_ver(char *ver, int flag)
{
	static	char version[80];
	char *zero = "0";
	char *cp;

	strcpy(version, ver);
	cp = strchr(version, ':');

	if (flag == VER_LOW) {
		if (cp == NULL)
			strcpy(version, zero);
		else {
			cp--;
			while (isspace((u_char)*cp)) cp--;
			*++cp = '\0';
		}
	}

	if (flag == VER_HIGH) {
		if (cp != NULL) {
			cp++;
			while (isspace((u_char)*cp)) cp++;
			return (cp);
		}
	}
	return (version);
}


/*
 * parse_pkg_entry()
 * Parameters:
 *	entry	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
parse_pkg_entry(char *entry)
{
	struct pkg_hist	*ph;
	char	pkg_abbr[80], ver_high[80], ver_low[80], arch[80];
	char	*tok_value;

	if (*entry == '\0')
		return;

	(void) memset(pkg_abbr, '\0', sizeof (pkg_abbr));
	(void) memset(ver_high, '\0', sizeof (ver_high));
	(void) memset(ver_low, '\0', sizeof (ver_low));
	(void) memset(arch, '\0', sizeof (arch));

	tok_value = set_token_value("PKG=", entry);
	if (tok_value != NULL)
		(void) strcpy(pkg_abbr, tok_value);

	tok_value = set_token_value("ARCH=", entry);
	if (tok_value != NULL)
		(void) strcpy(arch, tok_value);

	tok_value = set_token_value("VERSION=", entry);
	if (tok_value != NULL) {
		(void) strcpy(ver_low, split_ver(tok_value, VER_LOW));
		(void) strcpy(ver_high, split_ver(tok_value, VER_HIGH));
	}

	if (!map_hist_to_pkg(pkg_abbr, ver_low, ver_high, arch))
		return;

	ph = (struct pkg_hist *) calloc(sizeof (struct pkg_hist), 1);

	tok_value = set_token_value("REPLACED_BY=", entry);
	if (tok_value != NULL)
		ph->replaced_by = strdup(tok_value);

	tok_value = set_token_value("REMOVED_FILES=", entry);
	if (tok_value != NULL)
		ph->deleted_files = strdup(tok_value);

	tok_value = set_token_value("REMOVE_FROM_CLUSTER=", entry);
	if (tok_value != NULL)
		ph->cluster_rm_list = strdup(tok_value);

	tok_value = set_token_value("IGNORE_VALIDATION_ERROR=", entry);
	if (tok_value != NULL)
		ph->ignore_list = strdup(tok_value);

	tok_value = set_token_value("PKGRM=", entry);
	if (tok_value != NULL) {
		if ((*tok_value == 'y') || (*tok_value == 'Y'))
			ph->needs_pkgrm = 1;
	}
	if (ph->replaced_by != NULL) {
		if (!strstr(ph->replaced_by, pkg_abbr))
			ph->to_be_removed = 1;
	}

	attach_pkg_hist(pkg_abbr, ver_low, ver_high, arch, ph);

	return;
}

/*
 * map_hist_to_pkg()
 * Parameters:
 *	pkg	-
 *	verlo	-
 *	verhi	-
 *	arch	-
 * Return:
 * Status:
 *	private
 */
static int
map_hist_to_pkg(char *pkg, char *verlo, char *verhi, char *arch)
{
	Module *mod;
	struct s_histid histid;
	int is_installed = 0;
	Node *node;
	Modinfo *mi;

	histid.verlo = verlo;
	histid.verhi = verhi;
	histid.arch = arch;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC) {
			node = findnode(mod->sub->info.prod->p_packages,
			    pkg);
			if (node) {
				mi = (Modinfo *)(node->data);
				if (is_pkg_installed(mi, &histid))
					is_installed = 1;
				else
					while ((mi = next_inst(mi)) != NULL)
						if (is_pkg_installed(mi,
						    &histid)) {
							is_installed = 1;
							break;
						}
			}
		}
		if (is_installed)
			break;
	}
	return (is_installed);
}

/*
 * is_pkg_installed()
 * Parameters:
 *	mi	-
 *	histid	-
 * Return:
 * Status:
 *	private
 */
static int
is_pkg_installed(Modinfo * mi, struct s_histid * histid)
{
	if ((mi->m_shared == NOTDUPLICATE ||
	    mi->m_shared == SPOOLED_NOTDUP) &&
	    strcmp(mi->m_arch, histid->arch) == 0 &&
	    pkg_vcmp(mi->m_version, histid->verlo) >= 0 &&
	    pkg_vcmp(mi->m_version, histid->verhi) < 0)
		return (1);
	else
		return (0);
}

/*
 * map_hist_to_cls()
 * Parameters:
 *	cls	-
 *	verlo	-
 *	verhi	-
 * Return:
 * Status:
 *	private
 */
static int
map_hist_to_cls(char *cls, char *verlo, char *verhi)
{
	Module *mod;
	struct s_histid histid;
	int is_installed = 0;
	Node *node;
	Modinfo *mi;

	histid.verlo = verlo;
	histid.verhi = verhi;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC) {
			node = findnode(mod->sub->info.prod->p_clusters,
			    cls);
			if (node) {
				mi = (Modinfo *)(node->data);
				if (is_cls_installed(mi, &histid))
					is_installed = 1;
				else
					while ((mi = next_inst(mi)) != NULL)
						if (is_cls_installed(mi,
						    &histid)) {
							is_installed = 1;
							break;
						}
			}
		}
		if (is_installed)
			break;
	}
	return (is_installed);
}

/*
 * is_cls_installed()
 * Parameters:
 *	mi	-
 *	histid	-
 * Return:
 * Status:
 *	private
 */
static int
is_cls_installed(Modinfo * mi, struct s_histid * histid)
{
	if (prod_vcmp(mi->m_version, histid->verlo) >= 0 &&
	    prod_vcmp(mi->m_version, histid->verhi) < 0)
		return (1);
	else
		return (0);
}

/*
 * attach_pkg_hist()
 * Parameters:
 *	pkg	-
 *	verlo	-
 *	verhi	-
 *	arch	-
 *	histp	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
attach_pkg_hist(char *pkg, char *verlo, char *verhi, char *arch,
					struct pkg_hist *histp)
{
	Module *mod;
	struct s_histid histid;
	Node *node;
	Modinfo *mi;

	histid.verlo = verlo;
	histid.verhi = verhi;
	histid.arch = arch;

	histp->hist_next = pkg_history;
	pkg_history = histp;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC) {
			node = findnode(mod->sub->info.prod->p_packages,
			    pkg);
			if (node) {
				mi = (Modinfo *)(node->data);
				if (is_pkg_installed(mi, &histid)) {
					mi->m_pkg_hist = histp;
					histp->ref_count++;
				}
				while ((mi = next_inst(mi)) != NULL)
					if (is_pkg_installed(mi, &histid)) {
						mi->m_pkg_hist = histp;
						histp->ref_count++;
					}
			}
		}
	}
}

/*
 * attach_cls_hist()
 * Parameters:
 *	pkg	-
 *	verlo	-
 *	verhi	-
 *	arch	-
 *	histp	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
attach_cls_hist(char *cls, char *verlo, char *verhi, struct pkg_hist *histp)
{
	Module *mod;
	struct s_histid histid;
	Node *node;
	Modinfo *mi;

	histid.verlo = verlo;
	histid.verhi = verhi;

	histp->hist_next = cls_history;
	cls_history = histp;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC) {
			node = findnode(mod->sub->info.prod->p_clusters,
			    cls);
			if (node) {
				mi = (Modinfo *)(node->data);
				if (is_cls_installed(mi, &histid)) {
					mi->m_pkg_hist = histp;
					histp->ref_count++;
				}
				while ((mi = next_inst(mi)) != NULL)
					if (is_cls_installed(mi, &histid)) {
						mi->m_pkg_hist = histp;
						histp->ref_count++;
					}
			}
		}
	}
	return;
}

/*
 * free_hist_ent()
 * Parameters:
 *	histp
 * Return:
 *	none
 * Status:
 *	private
 */
static void
free_hist_ent(struct pkg_hist *histp)
{
	if (histp->replaced_by)
		free(histp->replaced_by);
	if (histp->deleted_files)
		free(histp->deleted_files);
	if (histp->cluster_rm_list)
		free(histp->cluster_rm_list);
	if (histp->ignore_list)
		free(histp->ignore_list);
	free(histp);
}
