#ifndef lint
#pragma ident "@(#)pf_parse.c 2.42 96/08/15"
#endif
/*
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mntent.h>
#include <sys/socket.h>
#include "spmicommon_api.h"
#include "spmisvc_api.h"
#include "spmistore_api.h"
#include "spmiapp_api.h"
#include "admutil.h"
#include "profile.h"
#include "pf_strings.h"

/* public prototypes */

int		parse_profile(Profile *, char *);

/* private prototypes */

static void	parse_fdisk(Fdisk **, char *, char *, char *, char *);
static void	parse_filesys(Profile *, char *, char *, char *, char *);
static char *	_strip_quotes(char *);
static void	_pf_parse(char *, char **, char **, char **, char **, char **);
static void	parse_error(char *, char *, ...);
static void	_pf_validate_profile(Profile *);
static void	_pf_add_locale(Profile *, char *);

/* Local Statics */

static int	line_num;
static char	buf[BUFSIZ + 1];
static Storage	**fsp;

/* ----------------------- public functions ---------------------------- */

/*
 * Function:	parse_profile
 * Description: This is the main profile parsing routine. Values extracted
 *		from the parser are store in the Profile structure. Once all
 *		input is parsed, the resulting profile structure is then
 *		validated to ensure that the profile was consistent.
 * Scope:	public
 * Parameters:	prop	- pointer to profile structure
 *		locale	- simplex or composite locale specifier
 * Return:	D_OK	 - parsing successful (ready for processing)
 *		D_FAILED - parsing failed (terminate operation)
 */
int
parse_profile(Profile *prop, char *locale)
{
	char		*s1, *s2, *s3, *s4, *s5;
	char		*loc;
	char		*last;
	Sw_unit		**mp = &UNITS(prop);
	Fdisk		**fdp = &DISKFDISK(prop);
	Namelist	*tmp;
	char		*cp;
	MFILE *		fp;
	int		upd;
	int		pid;
	int		slice;
	Disk_t *	bdp;
	LayoutConstraint	*lc;

	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 0)
		write_status(LOGSCR, LEVEL0, MSG0_PROFILE_PARSE);

	/*
	 * open the specified profile file for processing
	 */
	if ((fp = mopen(PROFILE(prop))) == NULL) {
		write_notice(ERRMSG,
			MSG1_PROFILE_OPEN_FAILED,
			PROFILE(prop));
		return (D_FAILED);
	}

	/*
	 * Set the profile version ID
	 */
	PROVERSION(prop) = PROFILE_VER_0;

	fsp = &DISKFILESYS(prop);

	for (line_num = 0; mgets(buf, sizeof (buf), fp) != NULL; line_num++) {
		if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 0)
			write_status(LOGSCR, LEVEL1|FMTPARTIAL, "%2d: %s",
				line_num, buf);

		/*
		 * strip off all trailing comment lines, newlines, and
		 * any preceding white space; this will strip off #
		 * and subsequent characters even if they appear
		 * in single quotes (consistent with check routine)
		 */
		if ((cp = strchr(buf, '#')) != NULL)
			*cp = '\0';

		cp = &buf[strlen(buf)];
		cp--;
		for (; cp >= buf && (isspace(*cp) || *cp == '\n'); cp--);
		*++cp = '\0';

		/* ignore NULL lines */
		if (buf[0] == '\0')
			continue;

		_pf_parse(&buf[0], &s1, &s2, &s3, &s4, &s5);

		if (s1 == NULL)
			parse_error(MSG0_FIELD1, MSG0_KEYWORD_NONE);

		if (s2 == NULL && !ci_streq(s1, "noreboot"))
			parse_error(MSG0_FIELD2, MSG0_VALUE_NONE);

/* install_type */
		if (ci_streq(s1, "install_type")) {
			if (s3)
				parse_error(MSG0_FIELD1,
					MSG0_SYNTAX_TOOMANY_FIELDS);
			if (OPTYPE(prop) != SI_UNDEFINED)
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_DUPLICATED, s1);
			if (ci_streq(s2, "initial_install"))
				OPTYPE(prop) = SI_INITIAL_INSTALL;
			else if (ci_streq(s2, "upgrade"))
				OPTYPE(prop) = SI_UPGRADE;
			else
				parse_error(MSG0_FIELD2,
					MSG1_INSTALLTYPE_INVALID, s2);
		}
/* system_type */
		else if (ci_streq(s1, "system_type")) {
			if (s3) {
				parse_error(MSG0_FIELD1,
					MSG0_SYNTAX_TOOMANY_FIELDS);
			} else if (SYSTYPE(prop) != MT_UNDEFINED) {
				write_notice(WARNMSG,
					MSG0_SYSTYPE_DUPLICATE_IGNORED);
			} else if (!ISOPTYPE(prop, SI_INITIAL_INSTALL)) {
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE, s1);
			} else {
				if (ci_streq(s2, "standalone"))
					SYSTYPE(prop) = MT_STANDALONE;
				else if (ci_streq(s2, "cacheos"))
					SYSTYPE(prop) = MT_CCLIENT;
				else if (ci_streq(s2, "autoclient"))
					SYSTYPE(prop) = MT_CCLIENT;
				else if (ci_streq(s2, "server"))
					SYSTYPE(prop) = MT_SERVER;
				else
					parse_error(MSG0_FIELD2,
						MSG1_SYSTEMTYPE_INVALID, s2);

				set_machinetype(SYSTYPE(prop));
			}
		}
/* fdisk */
		else if (ci_streq(s1, "fdisk")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL)) {
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE, s1);
			} else {
				/*
				 * check for fdisk keywords on systems which
				 * don't support fdisk
				 */
				if (disk_no_fdisk_req(first_disk())) {
					parse_error(MSG0_FIELD1,
					    MSG0_KEYWORD_FDISK_INVALID);
				}

				parse_fdisk(fdp, s2, s3, s4, s5);
			}
		}
/* partitioning */
		else if (ci_streq(s1, "partitioning")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL)) {
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);
			} else if (ci_streq(s2, "existing")) {
				DISKPARTITIONING(prop) = LAYOUT_EXIST;
			} else if (ci_streq(s2, "default")) {
				DISKPARTITIONING(prop) = LAYOUT_DEFAULT;
			} else if (ci_streq(s2, "explicit")) {
				DISKPARTITIONING(prop) = LAYOUT_RESET;
			} else {
				parse_error(MSG0_FIELD2,
					MSG1_PARTITIONING_INVALID, s2);
			}
		}
/* filesys */
		else if (ci_streq(s1, "filesys")) {
			if (ISOPTYPE(prop, SI_INITIAL_INSTALL)) {
				parse_filesys(prop, s2, s3, s4, s5);
			} else {
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);
			}
		}

/* cluster and package */
		else if ((ci_streq(s1, "cluster")) ||
				ci_streq(s1, "package")) {

			if ((*mp = (Sw_unit *)
					xcalloc(sizeof (Sw_unit))) == NULL)
				parse_error(NULL, MSG_STD_OUTOFMEM);

			(*mp)->name = s2;
			if (ci_streq(s1, "cluster"))
				(*mp)->unit_type = CLUSTER;
			else
				(*mp)->unit_type = PACKAGE;

			if (!s3 || (!ci_streq(s3, "delete")))
				(*mp)->delta = SELECTED;
			else
				(*mp)->delta = UNSELECTED;

			(*mp)->next = NULL;
			mp = &((*mp)->next);
		}

/* usedisk */
		else if (ci_streq(s1, "usedisk")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);

			if (DISKDONTUSE(prop) != NULL)
				parse_error(MSG0_FIELD1,
					MSG0_USE_DONTUSE_EXCLUSIVE);

			if (find_disk(s2) == NULL)
				parse_error(MSG0_FIELD2,
					MSG1_DISK_INVALID, s2);

			if (is_disk_name(s2) == 0)
				parse_error(MSG0_FIELD2,
					MSG1_DISKNAME_INVALID, s2);

			if ((tmp = (Namelist *)
				xcalloc(sizeof (Namelist))) == NULL)
				parse_error(NULL, MSG_STD_OUTOFMEM);

			tmp->next = DISKUSE(prop);
			DISKUSE(prop) = tmp;
			tmp->name = s2;
		}
/* dontuse */
		else if (ci_streq(s1, "dontuse")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);

			if (DISKUSE(prop) != NULL)
				parse_error(NULL,
				    MSG0_USE_DONTUSE_EXCLUSIVE);

			if (is_disk_name(s2) == 0)
				parse_error(MSG0_FIELD2,
					MSG1_DISKNAME_INVALID, s2);

			if (find_disk(s2) == NULL)
				parse_error(MSG0_FIELD2,
					MSG1_DISK_INVALID, s2);

			if ((tmp = (Namelist *)
				xcalloc(sizeof (Namelist))) == NULL)
				parse_error(NULL, MSG_STD_OUTOFMEM);

			tmp->next = DISKDONTUSE(prop);
			DISKDONTUSE(prop) = tmp;
			tmp->name = s2;
		}
/* num_clients */
		else if (ci_streq(s1, "num_clients")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);

			if (is_allnums(s2) == 0)
				parse_error(MSG0_FIELD2,
					MSG1_CLIENTNUM_INVALID,
					s2);

			CLIENTCNT(prop) = atoi(s2);
		}
/* client_swap */
		else if (ci_streq(s1, "client_swap")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);

			if (is_allnums(s2) == 0)
				parse_error(MSG0_FIELD2,
					MSG1_CLIENTSWAP_INVALID,
					s2);

			CLIENTSWAP(prop) = mb_to_sectors(atoi(s2));
		}
/* client_root */
		else if (ci_streq(s1, "client_root")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);

			if (is_allnums(s2) == 0)
				parse_error(MSG0_FIELD2,
					MSG1_CLIENTROOT_INVALID,
					s2);

			CLIENTROOT(prop) = mb_to_sectors(atoi(s2));
		}
/* client_arch */
		else if (ci_streq(s1, "client_arch")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);

			if (valid_arch(NULL, s2) != SUCCESS)
				parse_error(MSG0_FIELD2,
					MSG1_CLIENTARCH_INVALID, s2);

			if ((tmp = (Namelist *)
				xcalloc(sizeof (Namelist))) == NULL)
				parse_error(NULL, MSG_STD_OUTOFMEM);

			tmp->next = CLIENTPLATFORM(prop);
			CLIENTPLATFORM(prop) = tmp;
			tmp->name = s2;
		}
/* locale */
		else if (ci_streq(s1, "locale")) {
			/* syntax check on specified locale */
			if (valid_locale(SWPRODUCT(prop), s2) == 0)
				parse_error(MSG0_FIELD2,
					MSG1_LOCALE_INVALID, s2);

			if (ci_streq(s2, "c")) {
				write_notice(WARNMSG,
					MSG1_LOCALE_DUPLICATE,
					s2);
			} else {
				/* check to see if already selected */
				WALK_LIST(tmp, LOCALES(prop)) {
					if (ci_streq(s2, tmp->name)) {
						write_notice(WARNMSG,
							MSG1_LOCALE_DUPLICATE,
							s2);
					break;
					}
				}

				if (tmp == NULL)
					_pf_add_locale(prop, s2);
			}
		}
/* noreboot  (private) */
		else if (ci_streq(s1, "noreboot"))
			NOREBOOT(prop) = 1;

/* swap_size (private) */
		else if (ci_streq(s1, "swap_size")) {
			if (ISOPTYPE(prop, SI_INITIAL_INSTALL)) {
				if (is_allnums(s2) == 0)
					parse_error(MSG0_FIELD2,
					MSG1_SIZE_INVALID, s2);
				TOTALSWAP(prop) = atoi(s2);
			}
		}

/* root_device */
		else if (ci_streq(s1, "root_device")) {
			if (s3)
				parse_error(MSG0_FIELD1,
					MSG0_SYNTAX_TOOMANY_FIELDS);

			if (ROOTDEVICE(prop) != NULL)
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_DUPLICATED, s1);

			if (!is_slice_name(basename(s2)))
				parse_error(MSG1_FIELD2,
					MSG1_INVALID_DEVICE_NAME, s2);

			/* PowerPC can only have a slice of '0' */
			if (IsIsa("ppc") &&
					(cp = strchr(s2, 's')) != NULL &&
						atoi(++cp) != 0) {
				parse_error(MSG1_FIELD2, MSG0_ONLY_SLICE_ZERO);
			}

			/* make sure disk exists */
			if (find_disk(s2) == NULL)
				fatal_exit(MSG1_ROOTDEVICE_INVALID, s2);

			ROOTDEVICE(prop) = basename(xstrdup(s2));
		}

/* boot_device */
		else if (ci_streq(s1, "boot_device")) {
			if (!ISOPTYPE(prop, SI_INITIAL_INSTALL))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);
			/* check for missing second parameter */
			if (s3 == NULL)
				parse_error(MSG0_FIELD3, MSG0_VALUE_NONE);

			if (s4)
				parse_error(MSG0_FIELD1,
					MSG0_SYNTAX_TOOMANY_FIELDS);

			if (BOOTDEVICE(prop) != NULL)
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_DUPLICATED, s1);

			/* process the device field of the specifier */
			if (ci_streq(s2, "existing")) {
				if (DiskobjFindBoot(CFG_EXIST, &bdp) != D_OK ||
						bdp == NULL)
					fatal_exit(MSG0_EXISTING_NO_ROOT);

				if (IsIsa("sparc")) {
					if (BootobjGetAttribute(CFG_EXIST,
						    BOOTOBJ_DEVICE, &slice,
						    NULL) != D_OK ||
						!valid_sdisk_slice(slice)) {
						    fatal_exit(
							MSG0_EXISTING_NO_ROOT);
					}
					BOOTDEVICE(prop) = xstrdup(
						make_slice_name(disk_name(bdp),
							slice));
				} else if (IsIsa("ppc")) {
					WALK_PARTITIONS(pid) {
					    if (part_id(bdp, pid) == DOSOS12 ||
							part_id(bdp,
								pid) == DOSOS16)
						break;
					}
					if (!valid_fdisk_part(pid)) {
						fatal_exit(
						    MSG0_EXISTING_NO_ROOT);
					}
					BOOTDEVICE(prop) =
						xstrdup(disk_name(bdp));
				} else if (IsIsa("i386")) {
					pid = get_solaris_part(bdp, CFG_EXIST);
					if (!valid_fdisk_part(pid)) {
						fatal_exit(
						    MSG0_EXISTING_NO_ROOT);
					}
					BOOTDEVICE(prop) =
						xstrdup(disk_name(bdp));
				} else
					fatal_exit(MSG0_EXISTING_NO_ROOT);
			} else if (!ci_streq(s2, "any")) {
				if ((IsIsa("i386") || IsIsa("ppc")) &&
						!is_disk_name(s2)) {
					parse_error(MSG0_FIELD2,
						MSG1_DISKNAME_INVALID, s2);
				} else if (!is_slice_name(s2)) {
					parse_error(MSG0_FIELD2,
						MSG1_INVALID_DEVICE_NAME,
						s2);
				}
				BOOTDEVICE(prop) = xstrdup(s2);
			}

			/* make sure the disk exists */
			if (BOOTDEVICE(prop) != NULL &&
					find_disk(BOOTDEVICE(prop)) == NULL) {
				fatal_exit(MSG1_BOOTDEVICE_INVALID,
					BOOTDEVICE(prop));
			}

			/* process the prom update field specifier */
			if (ci_streq(s3, "update")) {
				(void) BootobjGetAttribute(CFG_EXIST,
					BOOTOBJ_PROM_UPDATEABLE, &upd,
					NULL);
				if (upd == 0) {
					write_notice(WARNMSG,
						MSG0_NOT_UPDATABLE);
					(void) BootobjSetAttribute(CFG_CURRENT,
						BOOTOBJ_PROM_UPDATE, 0,
						NULL);
				} else {
					(void) BootobjSetAttribute(CFG_CURRENT,
						BOOTOBJ_PROM_UPDATE, 1,
						NULL);
				}
			} else if (ci_streq(s3, "preserve")) {
				(void) BootobjSetAttribute(CFG_CURRENT,
					BOOTOBJ_PROM_UPDATE, 0,
					NULL);
			} else
				parse_error(MSG1_FIELD3,
					MSG1_SYNTAX_ERROR, s3);
		}

/* backup_media */

		else if (ci_streq(s1, "backup_media")) {
			if (!ISOPTYPE(prop, SI_UPGRADE))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);
			if (s4)
				parse_error(MSG0_FIELD1,
					MSG0_SYNTAX_TOOMANY_FIELDS);

			if (!s2 || !s3)
				parse_error(MSG0_FIELD1,
					MSG1_MISSING_PARAMS, s1);

			if (ci_streq(s2, "local_tape"))
				BACKUPMEDIA(prop) = DSRALTape;
			else if (ci_streq(s2, "local_diskette"))
				BACKUPMEDIA(prop) = DSRALFloppy;
			else if (ci_streq(s2, "local_filesystem"))
				BACKUPMEDIA(prop) = DSRALDisk;
			else if (ci_streq(s2, "remote_system"))
				BACKUPMEDIA(prop) = DSRALRsh;
			else if (ci_streq(s2, "remote_filesystem"))
				BACKUPMEDIA(prop) = DSRALNFS;
			else
				parse_error(MSG0_FIELD2,
					MSG1_INVALID_BAKDEV_TYPE, s2);

			if (MEDIAPATH(prop) != NULL)
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_DUPLICATED, s1);

			MEDIAPATH(prop) = xstrdup(s3);

		}

/* layout_constraint */

		else if (ci_streq(s1, "layout_constraint")) {
			if (!ISOPTYPE(prop, SI_UPGRADE))
				parse_error(MSG0_FIELD1,
					MSG1_KEYWORD_INVALID_OPTYPE,
					s1);

			if (!s2 || !s3)
				parse_error(MSG0_FIELD1,
					MSG1_MISSING_PARAMS, s1);

			/*
			 * If the third field has a value of changeable then
			 * a fourth field being provided is valid.  So check to
			 * see if a fifth field was given, if so then we have an
			 * error
			 */

			if (ci_streq(s3, "changeable") && s5) {
				parse_error(MSG0_FIELD1,
				    MSG0_SYNTAX_TOOMANY_FIELDS);
			}

			/*
			 * For all other values of the third field, only
			 * three fields are allowed.
			 */

			else if (!(ci_streq(s3, "changeable")) && s4) {
				parse_error(MSG0_FIELD1,
				    MSG0_SYNTAX_TOOMANY_FIELDS);
			}

			if ((lc = (LayoutConstraint *)xcalloc(
					sizeof (LayoutConstraint))) == NULL)
				parse_error(NULL, MSG_STD_OUTOFMEM);

			LCDEVNAME(lc) = xstrdup(s2);

			if (ci_streq(s3, "changeable")) {
				LCSTATE(lc) = SLChangeable;
				if (s4) {

					if (atoi(s4) == 0) {
						parse_error(MSG0_FIELD4,
						    MSG1_SIZE_INVALID,
						    s4);
					}

					/*
					 * Convert the size provided in MB to KB
					 */

					LCSIZE(lc) = atoi(s4) * 1024;
				}
			} else if (ci_streq(s3, "movable")) {
				LCSTATE(lc) = SLMoveable;
			} else if (ci_streq(s3, "available")) {
				LCSTATE(lc) = SLAvailable;
			} else if (ci_streq(s3, "collapse")) {
				LCSTATE(lc) = SLCollapse;
			} else {
				parse_error(MSG1_FIELD3,
					MSG1_INVALID_SLICE_MODE, s3);
			}

			LCNEXT(lc) = LAYOUTCONSTRAINT(prop);
			LAYOUTCONSTRAINT(prop) = lc;
		}
/* error */
		else {
			parse_error(MSG0_FIELD1, MSG1_KEYWORD_INVALID, s1);
		}

		if (ISOPTYPE(prop, SI_UNDEFINED))
			parse_error(MSG0_FIELD1, MSG0_INSTALL_TYPE_FIRST);
	}

	_pf_validate_profile(prop);

	/*
	 * add the default locales if necessary
	 */
	if (locale != NULL && locale[0] != '\0') {
		write_status(LOGSCR, LEVEL0, MSG0_DEFAULT_LOCALES);
		while (locale[0] != '\0') {
			last = strrchr(locale, '/');
			if (last == NULL)
				loc = locale;
			else {
				*last++ = '\0';
				loc = last;
			}

			/* check to see if already selected */
			WALK_LIST(tmp, LOCALES(prop)) {
				if (ci_streq(loc, tmp->name))
					break;
			}

			if (tmp == NULL && !streq(loc, "C")) {
				if (valid_locale(SWPRODUCT(prop), loc) == 0) {
					write_notice(ERRMSG,
						MSG1_LOCALE_DEFAULT_INVALID,
						loc);
					(void) mclose(fp);
					return (D_FAILED);
				} else {
					write_status(LOGSCR, LEVEL1|LISTITEM,
						MSG1_LOCALE_DEFAULT,
						loc);
					_pf_add_locale(prop, loc);
				}
			}

			if (strchr(locale, '/') == NULL)
				break;
		}
	}

	(void) mclose(fp);
	return (D_OK);
}

/* ----------------------- private functions --------------------------- */

/*
 * Function:	_pf_parse
 * Description:	Parse out value from string passed in.
 * Scope:	private
 * Parameters:	buf
 *		s1
 *		s2
 *		s3
 *		s4
 *		s5
 * Return:	none
 */
static void
_pf_parse(char *buf, char **s1, char **s2, char **s3, char **s4, char **s5)
{
	char	*cp;
	char	*ep;
	char	*sp;
	char	*str;

	if (buf == NULL)
		return;

	*s1 = *s2 = *s3 = *s4 = *s5 = NULL;
	str = (char *)xstrdup(buf);

	for (cp = str; *cp != '\0';) {
		if (isspace(*cp)) {
			cp++;
			continue;
		}
		ep = cp;
		if (*cp == '\'') {
			++ep;
			for (; *ep && *ep != '\''; ep++);
			ep++;
		} else
			for (; *ep && !isspace(*ep); ep++);

		if (ep == cp)
			break;

		if (!*s4) {
			if (*ep != '\0')
				*ep++ = '\0';
			if ((sp = _strip_quotes(cp)) == NULL)
				break;
		}
		if (!*s1)
			*s1 = xstrdup(sp);
		else if (!*s2)
			*s2 = xstrdup(sp);
		else if (!*s3)
			*s3 = xstrdup(sp);
		else if (!*s4)
			*s4 = xstrdup(sp);
		else if (!*s5) {
			*s5 = xstrdup(cp);
			break;
		}
		cp = ep;
	}
	free(str);
}

/*
 * Function:	_strip_quotes
 * Description:	Remove single quotes from around a string, if there are any.
 *		Only paired quotes are removed, and pairing occurs in order.
 * Scope:	private
 * Parameters:	str	- pointer to string to be processed
 * Return:	NULL	- bogus parse
 * 		char *	- pointer to stripped string
 */
static char *
_strip_quotes(char *str)
{
	char	*cp;
	char	*bp;

	if (str == NULL)
		return (NULL);

	for (cp = str; *cp && isspace(*cp); cp++);

	if (*cp == '\'') {
		bp = xstrdup(++cp);
		if ((cp = strchr(bp, '\'')) != NULL) {
			*cp = '\0';
			(void) strcpy(str, bp);
		}
		free(bp);
	}

	return (str);
}

/*
 * Function:	parse_error
 * Description:	Print out message to standard error and exit with an error
 *		status.
 * Scope:	private
 * Parameters:	format	- format string (required)
 *		...	- variable arguments (optional)
 * Return:	none
 */
static void
parse_error(char *field, char *format, ...)
{
	va_list		ap;
	char		str[BUFSIZ+256] = "";
	char		msg[BUFSIZ+256] = "";

	va_start(ap, format);
	if (format && *format)
		(void) vsprintf(msg, format, ap);
	va_end(ap);

	if (field != NULL)
		(void) sprintf(str, "%s - ", field);

	(void) strcat(str, msg);
	fatal_exit(str);
	/*NOTREACHED*/
}

/*
 * Function:	parse_filesys
 * Description:	Parse the 'filesys' keyword line, doing syntactic checking
 *		where possible.
 * Scope:	private
 * Parameters:	prop	- pointer to profile structure
 *		s2	- pointer to field 2
 *		s3	- pointer to field 3
 *		s4	- pointer to field 4
 *		s5	- pointer to remiander of input buffer string
 * Return:	none
 */
static void
parse_filesys(Profile *prop, char *s2, char *s3, char *s4, char *s5)
{
	Remote_FS	*p_cfs;
	char		*cp;
	char		*p, *o1, *o2, *o3, *o4, *o5;

	o1 = o2 = NULL;
	if (s5) {
		_pf_parse(s5, &o1, &o2, &o3, &o4, &o5);
		if (o3)
			parse_error(NULL, MSG0_SYNTAX_TOOMANY_FIELDS);
	}
	/*
	 * Parse the second field. If it's of the form <string>:<string>
	 * it is assumed to be a remote file system specification, otherwise,
	 * it's assumed to be a local file system specification. s2 is
	 * assumed to be non-NULL at this point.
	 */
	if ((p = strchr(s2, ':')) != NULL) {

		/* create a new remote file system data structure */
		if ((p_cfs = (Remote_FS *)xcalloc(sizeof (Remote_FS))) == NULL)
			parse_error(NULL, MSG_STD_OUTOFMEM);

		p_cfs->c_next = REMOTEFS(prop);
		REMOTEFS(prop) = p_cfs;
		*p++ = '\0';
		/*
		 * field 2 - remote mount name
		 * separate the hostname and remote mount point path name
		 */
		if (s2 == NULL || *s2 == NULL)
			parse_error(MSG0_FIELD2, MSG0_HOST_NAME_NONE);

		p_cfs->c_hostname = s2;

		if (p == NULL)
			parse_error(MSG0_FIELD2, MSG0_MOUNTPNT_NONE);
		else if (valid_mountp(p) == 0)
			parse_error(MSG0_FIELD2,
				MSG1_MOUNTPNT_REMOTE_INVALID, p);
		else
			p_cfs->c_export_path = p;

		/*
		 * field 3 - IP address for remote host
		 * if the IP address specified is '-', try to find the address.
		 * If this fails, and the file system being mount is /usr, if
		 * the run is live, quit; if the run is dryrun, print a warning;
		 * otherwise, set the c_ip_addr field to "". Valid values are:
		 *	-
		 *	<#>.<#>.<#>.<#>
		 */
		if (s3 == NULL)
			parse_error(MSG0_FIELD3, MSG0_IPADDR_NONE);
		else if (streq(s3, "-")) {
			if ((cp = name2ipaddr(s2)) == NULL || *cp == '\0') {
				if (streq(s4, USR) &&
					!GetSimulation(SIM_EXECUTE))
					parse_error(MSG0_FIELD3,
						MSG1_IPADDR_UNKNOWN, s2);
				else {
					write_notice(WARNMSG,
						MSG1_IPADDR_UNKNOWN, s2);
					p_cfs->c_ip_addr = "";
				}
			} else
				p_cfs->c_ip_addr = xstrdup(cp);
		} else {
			if (is_ipaddr(s3) == 0)
				parse_error(MSG0_FIELD3,
					MSG1_IPADDR_INVALID, s3);
			p_cfs->c_ip_addr = s3;
		}
		/*
		 * field 4 - local mount point (required).
		 */
		if (s4 == NULL)
			parse_error(MSG0_FIELD4, MSG0_MOUNTPNT_NONE);
		else if (valid_mountp(s4) == 0)
			parse_error(MSG0_FIELD4, MSG1_MOUNTPNT_INVALID, s4);
		else
			p_cfs->c_mnt_pt = s4;

		/*
		 * field 5 - (optional) mount options. Valid values are:
		 *	<string containing mount options>
		 */
		p_cfs->c_mount_opts = "-";
		if (o1 != NULL)
			p_cfs->c_mount_opts = o1;
	} else {
		/*
		 * field 2 - slice name. Valid values are:
		 *	any
		 *	rootdisk.s<#>
		 *	<cannonical slice name - cXtXdXsX>
		 */
		if ((*fsp = (Storage *) xcalloc(sizeof (Storage))) == NULL)
			parse_error(NULL, MSG_STD_OUTOFMEM);

		if (ci_streq(s2, "any")) {
			(*fsp)->dev = s2;
		} else if (strncasecmp(s2, "rootdisk", 8) == 0) {
			cp = &s2[strlen("rootdisk")];

			if (strncasecmp(cp, ".s", 2) != 0)
				parse_error(MSG0_FIELD2,
					MSG0_ROOTDEVICE_SLICE_NONE);

			++cp; ++cp;
			if (is_allnums(cp) == 0 ||
					invalid_sdisk_slice(atoi(cp)))
				parse_error(MSG0_FIELD2,
					MSG1_ROOTDEVICE_SLICE_INVALID, cp);

			if (slice_locked(first_disk(), atoi(cp)))
				parse_error(MSG0_FIELD2, MSG1_SLICE_FIXED, cp);

			(*fsp)->dev = xstrdup(s2);
		} else if (is_slice_name(s2)) {
			/*
			 * an explicit disk was specified; the disk must exist
			 * on the system
			 */
			if (find_disk(s2) == NULL)
				parse_error(MSG0_FIELD2, MSG1_DISK_INVALID, s2);

			(*fsp)->dev = s2;
			cp = strchr(s2, 's');
			++cp;
			if (!valid_sdisk_slice(atoi(cp)))
				parse_error(MSG0_FIELD2, MSG1_DEVICE_INVALID,
					s2);

			if (slice_locked(first_disk(), atoi(cp)))
				parse_error(MSG0_FIELD2, MSG1_SLICE_FIXED, s2);

			if (slice_access(s2, 1) == 1)
				parse_error(MSG0_FIELD2,
					MSG1_SLICE_DUPLICATE, s2);
		} else
			parse_error(MSG0_FIELD2, MSG1_DISKNAME_INVALID, s2);

		/*
		 * field 3 - size field of the filesys line. Valid values are:
		 *	existing
		 *	auto		(requires dfltmnt mount point name)
		 *	all
		 *	free
		 *	<#*>	 - number of MB
		 *	<#>:<#>	 - starting cylinder and size in cylinders
		 */
		if (s3 == NULL) {
			parse_error(MSG0_FIELD3, MSG0_SIZE_NONE);
		} else if (DISKPARTITIONING(prop) == LAYOUT_EXIST &&
					!ci_streq(s3, "existing")) {
			parse_error(MSG0_FIELD3,
				MSG0_PARTITIONING_EXISTING_SIZE);
		} else if (ci_streq(s3, "existing")) {
			if (ci_streq(s2, "any"))
				parse_error(MSG0_FIELD3,
					MSG1_SIZE_INVALID_ANY, s3);

			if ((o1 && ci_streq(o1, "preserve")) ||
					(o2 && ci_streq(o2, "preserve")))
				(*fsp)->preserve = 1;

			(*fsp)->size = s3;
		} else if ((cp = strchr(s3, ':')) != NULL) {
			*cp = '\0';
			if (is_allnums(s3) == 0 || is_allnums(++cp) == 0) {
				*cp = ':';
				parse_error(MSG0_FIELD3, MSG1_SIZE_INVALID, s3);
			}

			*--cp = ':';
			if (ci_streq(s2, "any"))
				parse_error(MSG0_FIELD3,
					MSG1_SIZE_INVALID_ANY, s3);

			(*fsp)->size = s3;
		} else if (ci_streq(s3, "auto") ||
				ci_streq(s3, "all") ||
				ci_streq(s3, "free") ||
				is_allnums(s3)) {
			(*fsp)->size = s3;
		} else {
			parse_error(MSG0_FIELD3, MSG1_SIZE_INVALID, s3);
		}

		/*
		 * field 4 - (optional) file system field valid mount points.
		 * Valid values are:
		 *	swap
		 *	unnamed
		 *	overlap
		 *	ignore
		 *	<absolute mount point pathname>
		 *	""
		 */
		if ((s4 == NULL || get_dfltmnt_ent(NULL, s4) != D_OK) &&
					ci_streq(s3, "auto")) {
			parse_error(MSG0_FIELD4,
				MSG1_MOUNTPNT_INVALID_AUTO,
				(s4 == NULL ? "<NULL>" : s4));
		} else if (s4 == NULL || ci_streq(s4, "unnamed")) {
			(*fsp)->name = "";
		} else if (ci_streq(s4, "overlap")) {
			if (ci_streq(s2, "any")) {
				parse_error(MSG0_FIELD4,
					MSG1_MOUNTPNT_INVALID_ANY,
					(s4 == NULL ? "<NULL>" : s4));
			}

			if (ci_streq(s3, "free")) {
				parse_error(MSG0_FIELD4,
					MSG1_MOUNTPNT_INVALID_FREE,
					s4);
			}

			(*fsp)->name = s4;
		} else if (ci_streq(s4, "ignore")) {
			if (!ci_streq(s3, "existing")) {
				parse_error(MSG0_FIELD4,
					MSG0_MOUNTPNT_INVALID_IGNORE);
			}

			(*fsp)->name = s4;
		} else if (valid_mountp(s4) == 0 && !ci_streq(s4, "swap")) {
			parse_error(MSG0_FIELD4, MSG1_MOUNTPNT_INVALID, s4);
		} else if (is_allnums(s3) && atoi(s3) < 10 && atoi(s3) != 0 &&
		    is_pathname(s4)) {
			/*
			 * file systems must be at least 10 MB in size; a
			 * '0' in this field indicates that the file system
			 * has been explicitly deselected in the layout
			 * routine
			 */
			parse_error(MSG0_FIELD4, MSG0_SLICE_TOOSMALL);
		} else {
			(*fsp)->name = s4;
		}

		/*
		 * fields 5 and 6 - (optional) optional parameter. Valid
		 * values are:
		 *
		 *	preserve
		 *	<string containing mount options - first char '-'>
		 *
		 * The default mount options is "-".
		 *
		 * NOTE:	if more than two optional parameters are
		 *		supported the logic in the Field 3
		 *		validation code above for "existing"
		 *		will have to have the third field added
		 */
		(*fsp)->mntopts = "-";

		/*
		 * NOTE: preserve is only being checked. It's acutal effect
		 * on the parsing logic occurred when the size field was
		 * geing set above
		 */
		if (o1 != NULL) {
			if (ci_streq(o1, "preserve")) {
				if (!ci_streq(s3, "existing"))
					parse_error(MSG0_FIELD5,
						MSG0_PRES_EXISTING);
				if (ci_streq(s4, "ignore"))
					parse_error(
						MSG0_FIELD5, MSG0_PRES_IGNORE);
			} else {
				for (cp = o1;
					*cp && *cp != '\'' && *cp != '\"';
					cp++);
				if (*cp == '\0') {
					if (ci_streq(s4, "ignore"))
						parse_error(MSG0_FIELD5,
						    MSG0_MOUNTOPT_IGNORE);
					(*fsp)->mntopts = o1;
				} else
					parse_error(MSG0_FIELD5,
						MSG1_MOUNTOPT_INVALID, o1);
			}
		}
		if (o2 != NULL) {
			if (ci_streq(o2, "preserve")) {
				if (!ci_streq(s3, "existing"))
					parse_error(MSG0_FIELD6,
						MSG0_PRES_EXISTING);

				if (ci_streq(s4, "ignore"))
					parse_error(
						MSG0_FIELD6, MSG0_PRES_IGNORE);
			} else {
				for (cp = o2;
					*cp && *cp != '\'' && *cp != '\"';
					cp++);
				if (*cp == '\0') {
					if (ci_streq(s4, "ignore"))
						parse_error(MSG0_FIELD6,
						    MSG0_MOUNTOPT_IGNORE);

					(*fsp)->mntopts = o2;
				} else
					parse_error(MSG0_FIELD6,
						MSG1_MOUNTOPT_INVALID, o2);
			}
		}

		(*fsp)->next = NULL;
		fsp = &((*fsp)->next);
	}
}

/*
 * Function:	parse_fdisk
 * Description:	Parse the "fdisk" keyword line. The syntax is:
 *
 *		fdisk <disk> <type> <size>
 *
 *		disk	- disk specifier:
 *			- cX[tX]dX
 *			- rootdisk
 *			- all
 *		type	- partition type specifier:
 *			- solaris
 *			- dosprimary
 *			- 0xHH	(0x01 -> 0xFF) hex type value
 *			- NNN	(1 -> 255) decimal type value
 *		size	- size of the partition:
 *			- all	  (entire disk - Solaris partitions only)
 *			- maxfree (largest contiguous unused area)
 *			- NNN	  (size in MB)
 *			- delete  (set the type to UNUSED and the size to 0)
 *		extra - should be NULL
 *
 *		Parsing rules enforced are:
 *		(1) size of '0' implies "size = 0 and type = UNUSED"
 *		(2) cannot have two SOLARIS creation entries for the same drive
 *		(3) cannot have an "all" entry and any other creation entry for
 *		    the same drive
 * Scope:	private
 * Parameters:	fdisk	- pointer to the head of the fdisk record chain
 *		disk	- disk name (rootdisk or cX[tX]dX)
 *		type	- non-NULL partition type specifier
 *		size	- non-NULL partition size
 *		extra	- parameters above and beyond those expected
 * Return:	none
 */
static void
parse_fdisk(Fdisk **fdisk, char *disk, char *type, char *size, char *extra)
{
	int	  t;
	Fdisk	  *fdp;
	Fdisk	  *tmp;

	if (extra != NULL)
		parse_error(NULL, MSG0_SYNTAX_TOOMANY_FIELDS);

	/* allocate a new fdisk structure */
	if ((fdp = (Fdisk *) xcalloc(sizeof (Fdisk))) == NULL)
		parse_error(NULL, MSG_STD_OUTOFMEM);

	/*
	 * field 2 - disk name
	 * Valid values are:
	 *	cX[tX]dX
	 *	rootdisk
	 *	all
	 */
	if (disk == NULL)
		parse_error(MSG0_FIELD2, MSG0_DISKNAME_NONE);
	else if (ci_streq(disk, "rootdisk"))
		fdp->disk = xstrdup(disk);
	else if (ci_streq(disk, "all"))
		fdp->disk = "all";
	else if (is_disk_name(disk) == 0)
		parse_error(MSG0_FIELD2, MSG1_DISKNAME_INVALID, disk);
	else if (find_disk(disk) == NULL)
		parse_error(MSG0_FIELD2, MSG1_DISK_INVALID, disk);
	else
		fdp->disk = xstrdup(disk);

	/*
	 * field 3 - type specifier
	 * Valid values are:
	 *	solaris
	 *	dosprimary
	 *	0xHH	(0x01 -> 0xFF)
	 *	NNN	(1 -> 255)
	 */
	if (type == NULL)
		parse_error(MSG0_FIELD3, MSG0_PARTTYPE_NONE);
	else if (ci_streq(type, "solaris"))
		fdp->id = SUNIXOS;
	else if (ci_streq(type, "dosprimary"))
		fdp->id = DOSPRIMARY;
	else if (is_hex_numeric(type)) {
		t = axtoi(type);
		if (t <= 0 || t > 255)
			parse_error(MSG0_FIELD3, MSG1_PARTTYPE_INVALID, type);
		fdp->id = t;
	} else if (is_allnums(type)) {
		t = atoi(type);
		if (t > 0 && t <= 255)
			fdp->id = t;
		else
			parse_error(MSG0_FIELD3, MSG1_PARTTYPE_INVALID, type);
	} else
		parse_error(MSG0_FIELD3, MSG1_PARTTYPE_INVALID, type);

	/*
	 * field 4 - size specifier
	 * Valid values are:
	 *	all		FD_SIZE_ALL
	 *	maxfree 	FD_SIZE_MAXFREE
	 *	delete		0
	 *	NNN		(# >= 0 MB where 0 implies 'delete')
	 */
	if (size == NULL)
		parse_error(MSG0_FIELD4, MSG0_SIZE_NONE);
	if (pf_convert_id(fdp->id) != DOSHUGE &&
			pf_convert_id(fdp->id) != SUNIXOS &&
			!ci_streq(size, "delete") &&
			!ci_streq(size, "0")) {
		parse_error(MSG0_FIELD4, MSG1_PART_CREATE_PROHIBITED, type);
	} else if (ci_streq(size, "all")) {
		if (fdp->id != SUNIXOS)
			parse_error(MSG0_FIELD4, MSG0_SIZE_ALL_INVALID);

		fdp->size = FD_SIZE_ALL;
	} else if (ci_streq(size, "maxfree"))
		fdp->size = FD_SIZE_MAXFREE;
	else if (ci_streq(size, "delete"))
		fdp->size = FD_SIZE_DELETE;
	else if (is_allnums(size)) {
		t = atoi(size);
		if (t == 0)
			fdp->size = FD_SIZE_DELETE;
		else if (fdp->id == SUNIXOS && t < 20)
			parse_error(MSG0_FIELD4, MSG0_SIZE_SOLARIS_TOOSMALL);
		else
			fdp->size = t;
	} else
		parse_error(MSG0_FIELD4, MSG1_SIZE_INVALID, size);

	fdp->next = NULL;

	/* add the record to the end of the chain */
	for (tmp = *fdisk; tmp && tmp->next; tmp = tmp->next);
	if (tmp == NULL)
		(*fdisk) = fdp;
	else
		tmp->next = fdp;
}

/*
 * Function:	_pf_validate_profile
 * Description:	Validate the parsed profile to fill in defaults that were
 *		not explicitly set, and to flag errors where the profile
 *		was inconsistent in its specification.
 * Scope:	private
 * Parameters:	prop	- pointer to profile structure
 * Return:	none
 */
static void
_pf_validate_profile(Profile *prop)
{
	/*
	 * error if "intall_type" was not specified
	 */
	if (OPTYPE(prop) == NULL)
		fatal_exit(MSG0_NO_OPTYPE);

	/*
	 * Set the default system type if one wasn't defined
	 */
	if (SYSTYPE(prop) == MT_UNDEFINED) {
		SYSTYPE(prop) = MT_STANDALONE;
		set_machinetype(SYSTYPE(prop));
	}

	/*
	 * Set the default partitioning if one wasn't defined
	 */
	if (DISKPARTITIONING(prop) == LAYOUT_UNDEFINED)
		DISKPARTITIONING(prop) = LAYOUT_DEFAULT;

	/*
	 * make sure server specific keywords weren't used on
	 * a non-server configuration
	 */
	if (SYSTYPE(prop) != MT_SERVER) {
		if (CLIENTROOT(prop) >= 0 ||
				CLIENTSWAP(prop) >= 0 ||
				CLIENTCNT(prop) >= 0 ||
				CLIENTPLATFORM(prop) != NULL)
			fatal_exit(MSG0_VERIFY_NOTSERVER);
	} else {
		/*
		 * set the defaults if the user did not supply them
		 */
		if (CLIENTROOT(prop) < 0)
			CLIENTROOT(prop) = mb_to_sectors(
				DEFAULT_ROOT_PER_CLIENT);

		if (CLIENTSWAP(prop) < 0)
			CLIENTSWAP(prop) = mb_to_sectors(
				DEFAULT_SWAP_PER_CLIENT);

		if (CLIENTCNT(prop) < 0)
			CLIENTCNT(prop) = DEFAULT_NUMBER_OF_CLIENTS;
	}

	/*
	 * make sure invalid keywords are not being used
	 * for CacheOS clients
	 */
	if (SYSTYPE(prop) == MT_CCLIENT) {
		/*
		** cacheos not allowed
		*/
		if ((OPTYPE(prop) == SI_UPGRADE) ||
		    (OPTYPE(prop) == SI_ADAPTIVE))
			fatal_exit(MSG0_AUTOCLIENT_UPGRADE_INVALID);

		/* make sure there are no software keywords */
		if (METACLUSTER(prop) != NULL ||
				UNITS(prop) != NULL ||
				LOCALES(prop) != NULL)
			fatal_exit(MSG0_VERIFY_CACHEOS_SW);

		/* check for partitioning value */
		if (DISKPARTITIONING(prop) == LAYOUT_EXIST)
			fatal_exit(MSG0_VERIFY_CACHEOS_PARTITIONING);
	}

	/*
	 * swap_size key currently only supported for COC profiles
	 */
	if (TOTALSWAP(prop) >= 0) {
		if (SYSTYPE(prop) != MT_CCLIENT)
			fatal_exit(MSG1_KEYWORD_INVALID_OPTYPE, "swap_size");
		else if (getenv("SYS_SWAPSIZE") == NULL) {
			/*
			 * swap_size is valid; override env variable
			 * which is used by the install library for
			 * sizing calculations
			 */
			static char buff[128];
			(void) sprintf(buff,
				"SYS_SWAPSIZE=%d", TOTALSWAP(prop));
			(void) putenv(buff);
		}
	}
	/*
	 * If the "intall_type" is set to upgrade, then check to see if
	 * the "backup_media" was provided.
	 */

	if (OPTYPE(prop) == SI_UPGRADE) {
		switch (BACKUPMEDIA(prop)) {
		case DSRALTape:
		case DSRALFloppy:
		case DSRALDisk:
		case DSRALRsh:
		case DSRALNFS:
			break;
		default:
			write_notice(WARNMSG,
				MSG0_NO_BACKUP_DEVICE);
			break;
		}
	}
}

/*
 * Function:	_pf_add_locale
 * Description:
 * Scope:	private
 * Parameters:	prop	- pointer to profile structure
 *		locale	- simplex or composite locale specifier
 * Return:	none
 */
static void
_pf_add_locale(Profile *prop, char *locale)
{
	Namelist	*tmp;

	if ((tmp = (Namelist *)xcalloc(sizeof (Namelist))) == NULL)
		parse_error(NULL, MSG_STD_OUTOFMEM);

	tmp->name = locale;
	tmp->next = LOCALES(prop);
	LOCALES(prop) = tmp;
}
