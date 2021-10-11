#ifndef lint
#ident "@(#)buildtoc.c 1.20 96/05/15"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "spmicommon_api.h"
#include "spmisoft_api.h"

/* private prototypes */

static int	make_exclude_list(StringList **, char *);
static void	print_usage(void);
static int	print_field(FILE *, char *, char *, int);
static int	print_strlist(FILE *, char *, StringList *, int);
static int	print_depend(FILE *, char *, Depend *);
static int	print_size(FILE *, char *, int);
static int	print_ptype(FILE *, char *, char);
static int	update_size_data(Modinfo *);
static void	fatal_exit(char *, ...);
static void	post_error(char *, ...);

/*
 * table continaing list of known ptypes (also defined in the software library)
 */
static struct ptype known_ptypes[] = {
	{ "root",	4,	PTYPE_ROOT },
	{ "kvm",	3,	PTYPE_KVM  },
	{ "usr",	3,	PTYPE_USR  },
	{ "ow",		2,	PTYPE_OW   },
	{ "",		0,	'\0'	   }
};

/* local prototypes */
static int	check_pkg_order(Module *);
static int	create_orderfile(Module *, char *);
static int	create_packagetoc(Module *, char *);
static int	print_package(FILE *, Modinfo *);
static int	set_spooled(Node *, caddr_t);
static int	found(char *, Node *, Node *);

/* globals */
StringList	*expkgs = NULL;

/*
 * Function:	main
 * Description:
 * Exit:	0	- operation successful
 *		1	- operation failed
 */
int
main(int argc, char **argv)
{
	int	 c;
	int	 usetoc = 0;
	char *	 mediadir = NULL;
	char *	 orderfile = "order.out";
	char *	 tocfile = "packagetoc.out";
	Module * media;
	Module * prod;
	int	 results;

	/* parse arguments */
	while ((c = getopt(argc, argv, "d:eo:t:x:")) != EOF) {
		switch (c) {
		    case 'd':	/* media directory */
			mediadir = xstrdup(optarg);
			break;
		    case 'e':
			usetoc = 1;
			break;
		    case 'o':	/* explicit file name for order file */
			orderfile = xstrdup(optarg);
			break;
		    case 't':	/* explicit file name for packagetoc file */
			tocfile = xstrdup(optarg);
			break;
		    case 'x':	/* use exclude list */
			if (make_exclude_list(&expkgs, optarg) < 0)
				fatal_exit(
					"Cannot load excluded packages file %s",
					optarg);
			break;
		    default:
			print_usage();
			fatal_exit("");
		}
	}

	/*
	 * the user must specify a media directory name
	 */
	if (mediadir == NULL) {
		print_usage();
		fatal_exit("");
	}

	/*
	 *  Use add-service-style space checking.  This causes
	 *  the space numbers to include the space required for any
	 *  package overhead in /var/sadm.
	 */
	set_add_service_mode(1);

	/*
	 *  load the software tree from the user specified media directory
	 */
	if ((media = add_media(mediadir)) != NULL) {
		results = load_media(media, usetoc);
		if ((results != SUCCESS) && (results != ERR_NOFILE))
			fatal_exit("Cannot load media from %s", mediadir);
	}

	/*
	 * select all packages and set their status to be spooled;
	 * then force the space meter to update space calculation tables,
	 * and thus the m_spooled value for each package
	 */
	if ((prod = get_current_product()) == NULL)
		fatal_exit("Cannot determine current product");

	/*
	 * update the spooled sizes for all packages in the software library
	 */
	(void) walklist(prod->info.prod->p_packages, set_spooled, NULL);
	(void) gen_dflt_fs_spaceinfo();

	/*
	 * print out error messages if any of the packages are in an
	 * order which is inconsistent with the package dependencies
	 */
	if (check_pkg_order(prod) < 0)
		fatal_exit("Execution failed to due package order errors");

	/*
	 * create the order and packagetoc files
	 */
	if (create_orderfile(prod, orderfile) < 0)
		fatal_exit("Unable to create the order output file");

	if (create_packagetoc(prod, tocfile) < 0)
		fatal_exit("Unable to create the packagetoc output file");

	/* print a successful execution status message */
	(void) printf("\nExecution completed successfully\n");
	exit(0);
}

/*
 * Function:	check_pkg_order
 * Description:	Print a message to stderr for any packages which are in the
 *		package list in an order which is incorrect based on their
 *		package pre-dependencies.
 * Parameters:	prod	[RO, *RO]
 *			Non-NULL pointer to a product module.
 * Return:	 0	all packages are in correct order
 *		-1	at least one package is not in the correct order
 */
static int
check_pkg_order(Module *prod)
{
	Node *	    tmp;
	Modinfo *   info;
	Depend *    dpnd;
	int	    status = 0;

	/*
	 * for each package, do all of the packages it depends on preceed it?
	 * Make sure you ignore any packages which were listed in the
	 * exclusion list
	 */
	for (tmp = (Node *)prod->info.prod->p_packages->list->next;
			tmp != (Node *)prod->info.prod->p_packages->list;
			tmp = (Node *)tmp->next) {
		info = (Modinfo *)tmp->data;
		if (StringListFind(expkgs, info->m_pkgid) != NULL)
			continue;

		for (dpnd = (Depend *)info->m_pdepends; dpnd;
				dpnd = dpnd->d_next) {
			if (found(dpnd->d_pkgid, tmp->prev,
				    prod->info.prod->p_packages->list) == 0) {
				post_error(
					"Package %s depends on %s which does not preceed it",
					info->m_pkgid, dpnd->d_pkgid);
				status = -1;
			}
		}
	}

	return (status);
}

/*
 * Function:	found
 * Description:	Determine if the pkgid is found in the provided package
 *		list.
 * Parameters:	pkgid	[RO, *RO]
 *			String specifying the package id name.
 *		ante	[RO, *RO]
 *			Pointer to head of package module list.
 *		end	[RO, *RO]
 *			Pointer to end pf package module list.
 * Return:	0	the package was not found
 *		1	the package was found
 */
static int
found(char *pkgid, Node *ante, Node *end)
{
	Node *	  tmp;

	for (tmp = ante; tmp != end; tmp = tmp->prev)
		if (streq(pkgid, ((Modinfo *) tmp->data)->m_pkgid))
			return (1);

	return (0);
}

/*
 * Function:	create_orderfile
 * Description:	Create the order file and print entries (one per line)
 *		in the order that they appear in the package list.
 * Parameters:	prod	[RO, *RO]
 *			Pointer to the head of the product module list.
 *		file	[RO, *RO]
 *			Name of file (relative or absolute) to use to
 *			store the order file output.
 * Return:	 0	order file created successfully
 *		-1	errors in order data
 */
static int
create_orderfile(Module *prod, char *file)
{
	Node *		pkg;
	FILE *		fp;
	Modinfo	*	info;

	/*
	 * create or truncate the order output file and prepare for writing
	 */
	if ((fp = fopen(file, "w")) == NULL) {
		post_error("Cannot write to order output file %s", file);
		return (-1);
	}

	/*
	 * for each package associated with the media (except those
	 * explicitly excluded, print the package name in the order it
	 * appears in the software tree; for architecture specific packages,
	 * also dump all instances
	 */
	for (pkg = prod->info.prod->p_packages->list->next;
			pkg != prod->info.prod->p_packages->list;
			pkg = pkg->next) {
		info = (Modinfo *)pkg->data;
		if (StringListFind(expkgs, info->m_pkgid) == NULL)
			(void) fprintf(fp, "%s\n", info->m_pkgid);
	}

	(void) fclose(fp);
	return (0);
}

/*
 * Function:	create_packagetoc
 * Description:	Create the packagetoc file and print package data
 *		in the order that they appear in the package list.
 * Parameters:	prod	[RO, *RO]
 *			Pointer to the head of the product module list.
 *		file	[RO, *RO]
 *			Name of file (relative or absolute) to use to
 *			store the order file output.
 * Return:	 0	packagetoc file created successfully
 *		-1	errors in packagetoc data
 */
static int
create_packagetoc(Module *prod, char *file)
{
	Modinfo *	info;
	Node *		pkg;
	Node *		instp;
	FILE *		fp;
	int		status = 0;

	/*
	 * create or truncate the packagetoc output file and prepare for writing
	 */
	if ((fp = fopen(file, "w")) == NULL) {
		post_error("Cannot write to packagetoc output file %s", file);
		return (-1);
	}

	/*
	 * for each package associated with the media (except those
	 * explicitly excluded, print the packagetoc information for
	 * that package in the order it appears in the software tree;
	 * for architecture specific packages, also dump all instances
	 */
	for (pkg = prod->info.prod->p_packages->list->next;
			pkg != prod->info.prod->p_packages->list;
			pkg = pkg->next) {
		info = (Modinfo *) pkg->data;
		if (StringListFind(expkgs, info->m_pkgid) != NULL)
			continue;

		if (print_package(fp, info) < 0)
			status = -1;

		for (instp = info->m_instances; instp;
			    instp = ((Modinfo *) instp->data)->m_instances) {
			info = (Modinfo *) instp->data;
			(void) printf(
				"\tinstance: %s\n", (char *) info->m_pkg_dir);
			if (print_package(fp, info) < 0)
				status = -1;
		}
	}

	(void) fclose(fp);
	return (status);
}

/*
 * Function:	print_package
 * Description:	Print the .packagetoc information to output. All
 *		packagetoc data is printed to the packagetoc file, and all
 *		ommissions are reported to stdout.
 * Parameters:	fp	[RO, *RO]
 *			Open file pointer to output file for packagetoc.
 *		info	[RO, *RO]
 *			Pointer to a package module.
 * Return:	 0	all required fields were printed successfully
 *		-1	at least one required field failed to print
 */
static int
print_package(FILE *fp, Modinfo *info)
{
	int	status = 0;

	if (info->m_shared == NULLPKG)
		return (0);

	(void) printf("Writing information for %s...", info->m_pkgid);

	if (print_field(fp, "PKG", info->m_pkgid, 1) < 0 ||
	    print_field(fp, "PKGDIR", info->m_pkg_dir, 1) < 0 ||
	    print_field(fp, "DESC", info->m_desc, 1) < 0 ||
	    print_field(fp, "NAME", info->m_name, 1) < 0 ||
	    print_field(fp, "VENDOR", info->m_vendor, 1) < 0 ||
	    print_field(fp, "VERSION", info->m_version, 1) < 0 ||
	    print_field(fp, "PRODNAME", info->m_prodname, 1) < 0 ||
	    print_field(fp, "PRODVERS", info->m_prodvers, 1) < 0 ||
	    print_field(fp, "BASEDIR", info->m_basedir, 1) < 0 ||
	    print_field(fp, "CATEGORY", info->m_category, 1) < 0 ||
	    print_ptype(fp, "SUNW_PKGTYPE", info->m_sunw_ptype) < 0 ||
	    print_field(fp, "ARCH", info->m_arch, 1) < 0 ||
	    print_strlist(fp, "SUNW_LOC", info->m_loc_strlist, 0) < 0 ||
	    print_field(fp, "SUNW_PKGLIST", info->m_l10n_pkglist, 0) < 0 ||
	    print_depend(fp, "SUNW_PDEPEND", info->m_pdepends) < 0 ||
	    print_depend(fp, "SUNW_RDEPEND", info->m_rdepends) < 0 ||
	    print_depend(fp, "SUNW_IDEPEND", info->m_idepends))
		status = -1;

	/* print out size data fields */
	if (status == 0) {
		if (update_size_data(info) < 0 ||
		    print_size(fp, "ROOTSIZE", info->m_deflt_fs[ROOT_FS]) < 0 ||
		    print_size(fp, "VARSIZE", info->m_deflt_fs[VAR_FS]) < 0 ||
		    print_size(fp, "OPTSIZE", info->m_deflt_fs[OPT_FS]) < 0 ||
		    print_size(fp, "EXPORTSIZE",
			info->m_deflt_fs[EXPORT_FS]) < 0 ||
		    print_size(fp, "USRSIZE", info->m_deflt_fs[USR_FS]) < 0 ||
		    print_size(fp, "USROWNSIZE",
			info->m_deflt_fs[USR_OWN_FS]) < 0 ||
		    print_size(fp, "SPOOLEDSIZE", info->m_spooled_size) < 0)
			status = -1;
	}

	(void) printf("done.\n");
	return (status);
}

/*
 * Function:	set_spooled
 * Description:	Function used by the walklist() function to calcuate the
 *		spooled size of each package in the product.
 * Parameters:	np	[RO, *RO]
 *		data	[RO]
 * Return:	0	always returns this value; required by walklist()
 */
/*ARGSUSED1*/
static int
set_spooled(Node *np, caddr_t data)
{
	Modinfo	*	info;

	if (np == NULL)
		return (0);

	for (info = (Modinfo*)np->data; info != NULL; info = next_inst(info)) {
		info->m_status = SELECTED;
		info->m_action = TO_BE_SPOOLED;
	}

	return (0);
}

/*
 * Function:	make_exclude_list
 * Description:	Create a linked list of excluded packages from the user
 *		specified package exclusion list.
 * Parameters:	listp	[RO, *RO, **RW]
 *			Address of the pointer to the head of the excluded
 *			package linked list.
 *		file	[RO, *RO]
 *			String containing one or more file names, separated by
 *			commas, for excluded packages to be listed.
 * Return:	 0	package list loaded successfully
 *		-1	package list load failed
 */
static int
make_exclude_list(StringList **listp, char *file)
{
	FILE *	fp;
	char	buf[MAXPATHLEN];
	char *	ep;
	char *  sp;
	int	status = 0;

	/* validate parameters */
	if (listp == NULL || file == NULL || *file == NULL)
		return (-1);

	/* open the exclude list file */
	if ((fp = fopen(file, "r")) == NULL) {
		post_error("Cannot read excluded packages input file %s", file);
		return (-1);
	}

	/*
	 * for each line in the exclude file, pull out the package name (first
	 * alphanumeric string) and add it to the list of excluded packages;
	 * make sure to skip comment lines
	 */
	while (fgets(buf, MAXPATHLEN, fp) != NULL) {
		/* extract the desired package name and skip comment lines */
		for (sp = buf; *sp && isspace(*sp); sp++);
		if (*sp == '#')
			continue;

		for (ep = sp; *ep && isalnum(*ep); ep++);
		if (ep > sp) {
			*ep = '\0';
			if (StringListAdd(listp, sp) < 0) {
				post_error(
				    "Internal error processing excluded packages input list for %s",
				    sp);
				status = -1;
				break;
			}
		}
	}

	(void) fclose(fp);
	return (status);
}

/*
 * Function:	print_usage
 * Description:	Print the usage line for this function to stdout.
 * Parameters:	none
 * Return:	none
 */
static void
print_usage(void)
{
	(void) printf(
	    "Usage: buildtoc -d <directory> [-e] [-o <order_file>] [-t <toc_file>] [-x <exclude_file>]\n");
}

/*
 * Function:	print_field
 * Description: Printout a generic attribute/value string pair.
 * Parameters:	fp	    [RO, *RO]
 *			    File pointer for output file
 *		attribute   [RO, *RO]
 *			    String containing attribute name.
 *		value	    [RO, *RO]
 *			    String containing value name.
 *		required    [RO]
 *			    Indicates if the attribute is required for the
 *			    output.
 * Return:	 0	the field was printed, or it was not printed
 *			but was not required
 *		-1	the field was not printed and was required
 */
static int
print_field(FILE *fp, char *attribute, char *value, int required)
{
	int	status = 0;

	if (value != NULL)
		(void) fprintf(fp, "%s=%s\n", attribute, value);
	else if (required == 1) {
		post_error("Missing %s macro", attribute);
		status = -1;
	}

	return (status);
}

/*
 * Function:	print_strlist
 * Description: Printout a generic attribute/value string pair,
 * 		where the "value" is a list of strings to be separated
 *		by commas in the output.
 * Parameters:	fp	    [RO, *RO]
 *			    File pointer for output file
 *		attribute   [RO, *RO]
 *			    String containing attribute name.
 *		valstr	    [RO, *RO]
 *			    String-list containing values.
 *		required    [RO]
 *			    Indicates if the attribute is required for the
 *			    output.
 * Return:	 0	the field was printed, or it was not printed
 *			but was not required
 *		-1	the field was not printed and was required
 */
static int
print_strlist(FILE *fp, char *attribute, StringList *valstr, int required)
{
	int	status = 0;
	int	first = 1;

	if (valstr != NULL)
		for ( ; valstr != NULL; valstr = valstr->next) {
			if (valstr->string_ptr == NULL)
				continue;
			if (first) {
				(void) fprintf(fp, "%s=%s", attribute,
				    valstr->string_ptr);
				first = 0;
			} else
				(void) fprintf(fp, ",%s",
				    valstr->string_ptr);
		}
	if (first == 0)
		(void) fprintf(fp, "\n");
	if (first = 1 && required == 1) {
		post_error("Missing %s macro", attribute);
		status = -1;
	}

	return (status);
}

/*
 * Function:	print_depend
 * Description: Print out dependency info, if any
 *		SUNW_{P|R|I}DEPEND=pkgid[:[(arch)][version]]
 * Parameters:	fp	[RO, *RO]
 *			Open file pointer to output file for packagetoc.
 *		attribute [RO, *RO]
 *			Pointer to ptype attribute name to put in file.
 *		dlist	[RO, *RO]
 *			Pointer to head of dependency list.
 * Return:	 0	the field was printed, or it was not printed
 *			but was not required
 *		-1	the field was not printed and was required
 */
static int
print_depend(FILE *fp, char *attribute, Depend *dlist)
{
	Depend *    dpnd;
	char	    buf[MAXPATHLEN];
	int	    status = 0;

	for (dpnd = (Depend *) dlist; dpnd != NULL; dpnd = dpnd->d_next) {
		(void) sprintf(buf, "%s%s%s%s%s%s",
			dpnd->d_pkgid,
			(dpnd->d_version || dpnd->d_arch) ? ":" : "",
			(dpnd->d_arch) ? "(" : "",
			(dpnd->d_arch) ? dpnd->d_arch : "",
			(dpnd->d_arch) ? ")" : "",
			(dpnd->d_version) ? dpnd->d_version : "");
		if (print_field(fp, attribute, buf, 0) < 0)
			status = -1;
	}

	return (status);
}

/*
 * Function:	print_size
 * Description:	Print out one of the size attribute fields.
 * Parameters:	fp	[RO, *RO]
 *			Open file pointer to output file for packagetoc.
 *		attribute [RO, *RO]
 *			Pointer to ptype attribute name to put in file.
 *		size	[RO]
 *			Integer size value to print for field.
 * Return:	 0	the field was printed successfully
 *		-1	the field failed to print
 */
static int
print_size(FILE *fp, char *attribute, int size)
{
	char 	buf[MAXPATHLEN];
	int	status;

	(void) sprintf(buf, "%d", size * 1024);
	status = print_field(fp, attribute, buf, 0);
	return (status);
}

/*
 * Function:	print_ptype
 * Description: Print the ptype field in pacakgetoc-ese.
 * Parameters:	fp	[RO, *RO]
 *			Open file pointer to output file for packagetoc.
 *		attribute [RO, *RO]
 *			Pointer to ptype attribute name to put in file.
 *		flag	[RO]
 *			Flag indicating package type.
 * Return:	 0	the package type field wrote out correctly
 *		-1	the package type field failed to write out correctly
 */
static int
print_ptype(FILE *fp, char *attribute, char flag)
{
	char 	buf[MAXPATHLEN];
	int	status;
	int	i;

	buf[0] = '\0';
	for (i = 0; known_ptypes[i].namelen; i++) {
		if (flag == known_ptypes[i].flag) {
			(void) strcpy(buf, known_ptypes[i].name);
			break;
		}
	}

	status = print_field(fp, attribute, buf, 1);
	return (status);
}

/*
 * Function:	update_size_data
 * Description:	update the software library data structures in
 * 		preparation for size estimation printing
 * Parameters:	info	[RO, *RO]
 *			Pointer to modinfo structure for product.
 * Return:	 0	successfully updated size data
 *		-1	size data failed to update successfully
 */
static int
update_size_data(Modinfo *info)
{
	Module	    tmp;
	int	    status = 0;

	tmp.info.mod = info;
	tmp.type = PACKAGE;
	tmp.next = tmp.prev = tmp.sub = tmp.head = tmp.parent = NULL;
	if (calc_cluster_space(&tmp, UNSELECTED) == NULL) {
		post_error("Cannot successfully calculate cluster space");
		status = -1;
	}

	return (status);
}

/*
 * Function:	fatal_exit
 * Description:	Exit routine called when an irreconcilable error has occurred.
 *		If the caller provides a message, it is printed as an error
 *		notification. Stdout and stderr are flushed, and the process
 *		is terminated.
 * Parameters:	msg	- message string format
 *		...	- message string parameters
 * Return:	none
 */
static void
fatal_exit(char *msg, ...)
{
	va_list		ap;
	char		buf[256];

	if (msg != NULL && *msg != NULL) {
		buf[0] = '\0';
		va_start(ap, msg);
		(void) vsprintf(buf, msg, ap);
		va_end(ap);
		(void) fprintf(stderr, "ERROR: %s\n", buf);
	}

	/* print a failure status message */
	(void) printf("Execution failed.\n");

	(void) fflush(stderr);
	(void) fflush(stdout);
	exit(1);
}

/*
 * Function:	post_error
 * Description:	Write an error message to stderr. Append a newline to all
 *		output, and prefix the message with "ERROR: ".
 * Parameters:	msg	- message string format
 *		...	- message string parameters
 * Return:	none
 */
static void
post_error(char *msg, ...)
{
	va_list		ap;
	char		buf[256];

	if (msg != NULL && *msg != NULL) {
		buf[0] = '\0';
		va_start(ap, msg);
		(void) vsprintf(buf, msg, ap);
		va_end(ap);
		(void) fprintf(stderr, "ERROR: %s\n", buf);
	}

	(void) fflush(stderr);
}
