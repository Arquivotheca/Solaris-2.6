/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights resverved.
 */

#pragma	ident	"@(#)classreg.c	1.9	94/11/16 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <launcher_p.h>


#define CLASS_RECORD_P_FMT	"%s %s %s %s %s %s %s\n"
#define CLASS_RECORD_S_FMT	"%s %s %s %s %s %s %s"
#define DB_DIR			"/usr/snadm/etc"

typedef struct _s_classreg_rec {
	char		*class_name;
	char		*display_name;
	char		*parent_name;
	char		*class_icon;
	admin_icon_t	class_icon_type;
	char		*instance_icon;
	admin_icon_t	instance_icon_type;
} _classreg_rec;

static const char	*rootname = "ROOT";
static const char	*class_db_dir = DB_DIR;
static const char	*class_db = "/usr/snadm/etc/classreg.db";
static FILE		*class_fp = NULL;


static int
lock_file(int fd)
{
	lseek(fd, 0L, SEEK_SET);
	/* TMP */
	return (1);
	return (lockf(fd, F_TLOCK, 0L));
}


static int
unlock_file(int fd)
{
	lseek(fd, 0L, SEEK_SET);
	/* TMP */
	return (1);
	return (lockf(fd, F_ULOCK, 0L));
}


static const char *
icon_to_string(admin_icon_t the_icon)
{
	switch (the_icon) {
	case admin_xbm_icon:
		return ("xbm");
	case admin_cde_icon:
		return ("cde");
	default:
		return ("unknown");
	}
}


admin_icon_t
string_to_icon(const char *the_icon)
{
	if (strcmp(the_icon, "xbm") == 0) {
		return (admin_xbm_icon);
	} else if (strcmp(the_icon, "cde") == 0) {
		return (admin_cde_icon);
	} else {
		return (admin_err_icon);
	}
}


static
int
find_class_entry(const char *key_name, _classreg_rec *record)
{

	FILE	*class_fp;
	int	fd;
	int	retval;
	char	c[256];
	char	d[256];
	char	p[256];
	char	ci[256];
	char	cit[256];
	char	ii[256];
	char	iit[256];


	if (key_name == NULL || record == NULL) {
		return (E_ADMIN_BADARG);
	}

	/* Open and lock the file */

	class_fp = fopen(class_db, "r");

	if (class_fp == NULL) {
		return (E_ADMIN_FAIL);
	}

	fd = fileno(class_fp);

	if (lock_file(fd) == -1) {
		fclose(class_fp);
		return (E_ADMIN_FAIL);
	}

	/*
	 * Look through the file for the record named "class_name".
	 * If found, fill in the return structure and break out of
	 * the loop.  After the iteration ends, either by reading
	 * the entire file or breaking out, the file is closed,
	 * unlocked, and we return.
	 */

	retval = E_ADMIN_FAIL;

	while (fscanf(class_fp, CLASS_RECORD_S_FMT, c, d, p,
	    ci, cit, ii, iit) != EOF) {

		if (strcmp(key_name, c) == 0) {

			record->class_name = strdup(c);
			record->display_name = strdup(d);
			record->parent_name = strdup(p);
			record->class_icon = strdup(ci);
			record->class_icon_type = string_to_icon(cit);
			record->instance_icon = strdup(ii);
			record->instance_icon_type = string_to_icon(iit);

			retval = E_ADMIN_OK;

			break;
		}
	}

	/* close and unlock the file */

	fclose(class_fp);
	(void) unlock_file(fd);

	return (retval);
}


int
admc_register(
	const char	*class_name,
	const char	*display_name,
	const char	*parent_name,
	const char	**children,
	const char	*class_icon,
	admin_icon_t	class_icon_type,
	const char	*instance_icon,
	admin_icon_t	instance_icon_type)
{

	FILE		*class_fp;
	FILE		*tmpfp;
	char		*tmpname;
	int		err;
	int		fd;
	const char	**cp;
	char		c[256];
	char		d[256];
	char		p[256];
	char		ci[256];
	char		cit[256];
	char		ii[256];
	char		iit[256];
	const char	*ci_string;
	const char	*ii_string;
	_classreg_rec	dummy;


	class_fp = fopen(class_db, "r");

	if (class_fp == NULL) {
		return E_ADMIN_FAIL;
	}

	fd = fileno(class_fp);

	/* attempt to lock the entire file, fail if unsuccessful */

	if (lock_file(fd) == -1) {
		return E_ADMIN_FAIL;
	}

	/*
	 * try to find this class_name (the "key" for the class database),
	 * return an error if it already exists as we don't allow duplicates.
	 */

	if (find_class_entry(class_name, &dummy) == E_ADMIN_OK) {
		fclose(class_fp);
		unlock_file(fd);
		return (E_ADMIN_EXISTS);
	}

	if ((tmpname = tempnam(class_db_dir, "admin")) == NULL) {
		fclose(class_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	if ((tmpfp = fopen(tmpname, "w")) == NULL) {
		fclose(class_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	ci_string = icon_to_string(class_icon_type);
	ii_string = icon_to_string(instance_icon_type);

	/*
	 * Read each record from the database, update it if
	 * necessary, write it to the temp file, write the
	 * new record to the end of the temp file, close the
	 * temp file, and move it back on top of the real
	 * file.
	 */

	while (fscanf(class_fp, CLASS_RECORD_S_FMT, c, d, p,
	    ci, cit, ii, iit) != EOF) {

		for (cp = children; cp != NULL && *cp != NULL; cp++) {
			if (strcmp(*cp, c) == 0) {
				/*
				 * break out; this guy needs to be made a child
				 * of the new class.
				 */
				break;
			}
		}
	
		if (cp != NULL) {
			/* found a guy that needs to be reparented */
			fprintf(tmpfp, CLASS_RECORD_P_FMT, c, d,
			    parent_name, ci, cit, ii, iit);
		} else {
			/* just copy straight across */
			fprintf(tmpfp, CLASS_RECORD_P_FMT, c, d,
			    p, ci, cit, ii, iit);
		}
	}

	/* Write the new record */
	if (parent_name == NULL) {
		parent_name = rootname;
	}
	fprintf(tmpfp, CLASS_RECORD_P_FMT, class_name, display_name,
	    parent_name, class_icon, ci_string, instance_icon, ii_string);

	/* close the files */

	fclose(tmpfp);
	fclose(class_fp);

	/* Move the new, updated file over the old one */
	if (rename(tmpname, class_db) == -1) {
		/* failure */
		free(tmpname);
		(void) unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	free(tmpname);

	/* Unlock the file */

	(void) unlock_file(fd);

	return (E_ADMIN_OK);
}


int
admc_unregister(const char *class_name)
{

	FILE		*class_fp;
	FILE		*tmpfp;
	char		*tmpname;
	int		fd;
	char		c[256];
	char		d[256];
	char		p[256];
	char		ci[256];
	char		cit[256];
	char		ii[256];
	char		iit[256];
	const char	*ci_string;
	const char	*ii_string;
	int		retval = E_ADMIN_NOCLASS;


	if ((class_fp = fopen(class_db, "r")) == NULL) {
		return (E_ADMIN_FAIL);
	}

	fd = fileno(class_fp);

	/* attempt to lock the entire file, fail if unsuccessful */

	if (lock_file(fd) == -1) {
		fclose(class_fp);
		return (E_ADMIN_FAIL);
	}

	if ((tmpname = tempnam(class_db_dir, "admin")) == NULL) {
		fclose(class_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	if ((tmpfp = fopen(tmpname, "w")) == NULL) {
		fclose(class_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	/*
	 * Read each record from the database, copy it to the output
	 * file as long as it doesn't match the key that was passed
	 * in as the record to delete.
	 */

	while(fscanf(class_fp, CLASS_RECORD_S_FMT, c, d, p,
	    ci, cit, ii, iit) != EOF) {

		if (strcmp(class_name, c) == 0) {
			retval = E_ADMIN_OK;
			continue;
		}

		fprintf(tmpfp, CLASS_RECORD_P_FMT, c, d, p, ci, cit, ii, iit);
	}

	/* close the files */

	fclose(tmpfp);
	fclose(class_fp);

	/* Move the new, updated file over the old one */
	if (rename(tmpname, class_db) == -1) {
		/* failure */
		free(tmpname);
		(void) unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	free(tmpname);
	(void) unlock_file(fd);

	return (retval);
}


int
admc_delete_registry()
{
	if (open(class_db, O_TRUNC) == -1) {
		return (E_ADMIN_FAIL);
	}

	return (E_ADMIN_OK);
}


int
admc_find_rootclass(char *buf, size_t size)
{

	FILE		*class_fp;
	int		fd;
	char		c[256];
	char		d[256];
	char		p[256];
	char		ci[256];
	char		cit[256];
	char		ii[256];
	char		iit[256];


	class_fp = fopen(class_db, "r");

	if (class_fp == NULL) {
		return (E_ADMIN_BADARG);
	}

	fd = fileno(class_fp);

	/* attempt to lock the entire file, fail if unsuccessful */

	if (lock_file(fd) == -1) {
		return (E_ADMIN_FAIL);
	}

	while (fscanf(class_fp, CLASS_RECORD_S_FMT, c, d, p,
	    ci, cit, ii, iit) != EOF) {

		if (strcmp(p, rootname) == 0) {
			if (strlen(c) < size) {
				strcpy(buf, c);
				return (E_ADMIN_OK);
			} else {
				strncpy(buf, c, size - 1);
				buf[size - 1] = '\0';
				return (E_ADMIN_TRUNC);
			}
		}
	}

	return (E_ADMIN_FAIL);
}


int
admc_set_displayname(const char *key, const char *newdisplay)
{
}


int
admc_get_displayname(const char *key, char *buf, size_t size)
{

	_classreg_rec	record;
	int		retval;


	if (find_class_entry(key, &record) == E_ADMIN_OK) {
		if (strlen(record.display_name) < size) {
			strcpy(buf, record.display_name);
			retval = E_ADMIN_OK;
		} else {
			strncpy(buf, record.display_name, size - 1);
			buf[size - 1] = '\0';
			retval = E_ADMIN_TRUNC;
		}
	} else {
		retval = E_ADMIN_FAIL;
	}

	return (retval);
}


int
admc_reparent(const char *key, const char *newparent)
{
}


int
admc_get_parentname(const char *key, char *buf, size_t size)
{

	_classreg_rec	record;
	int		retval;


	if (find_class_entry(key, &record) == E_ADMIN_OK) {
		if (strlen(record.parent_name) < size) {
			strcpy(buf, record.parent_name);
			retval = E_ADMIN_OK;
		} else {
			strncpy(buf, record.parent_name, size - 1);
			buf[size - 1] = '\0';
			retval = E_ADMIN_TRUNC;
		}
	} else {
		retval = E_ADMIN_FAIL;
	}

	return (retval);
}


char **
admc_get_children(const char *key)
{

	FILE		*class_fp;
	int		fd;
	_classreg_rec	dummy;
	int		cnt;
	char		**retval;
	char		c[256];
	char		d[256];
	char		p[256];
	char		ci[256];
	char		cit[256];
	char		ii[256];
	char		iit[256];


	if (key == NULL) {
		return (NULL);
	}

	if (find_class_entry(key, &dummy) != E_ADMIN_OK) {
		return (NULL);
	}

	/* Open and lock the file */

	class_fp = fopen(class_db, "r");

	if (class_fp == NULL) {
		return (NULL);
	}

	fd = fileno(class_fp);

	if (lock_file(fd) == -1) {
		fclose(class_fp);
		return (NULL);
	}

	/*
	 * Go through the file, count children, malloc space for
	 * returned children, go through again, fill in return
	 * value, then return.  Pretty ineffecient, but this
	 * is just for the prototype.
	 */

	cnt = 0;

	while (fscanf(class_fp, CLASS_RECORD_S_FMT, c, d, p,
	    ci, cit, ii, iit) != EOF) {

		if (strcmp(key, p) == 0) {
			cnt++;
		}
	}

	/* Extra one for a NULL terminator */
	cnt++;

	rewind(class_fp);

	retval = (char **)malloc((unsigned)(cnt * sizeof (char *)));
	if (retval == NULL) {
		(void) unlock_file(fd);
		fclose(class_fp);
		return (NULL);
	}

	cnt = 0;

	while (fscanf(class_fp, CLASS_RECORD_S_FMT, c, d, p,
	    ci, cit, ii, iit) != EOF) {

		if (strcmp(key, p) == 0) {
			retval[cnt] = strdup(c);
			cnt++;
		}
	}

	retval[cnt] = NULL;

	(void) unlock_file(fd);
	fclose(class_fp);

	return (retval);
}


int
admc_set_classicon(const char *key, const char *newclassicon)
{
}


int
admc_get_classicon(const char *key, char *buf, size_t size)
{

	_classreg_rec	record;
	int		retval;


	if (find_class_entry(key, &record) == E_ADMIN_OK) {
		if (strlen(record.class_icon) < size) {
			strcpy(buf, record.class_icon);
			retval = E_ADMIN_OK;
		} else {
			strncpy(buf, record.class_icon, size - 1);
			buf[size - 1] = '\0';
			retval = E_ADMIN_TRUNC;
		}
	} else {
		retval = E_ADMIN_FAIL;
	}

	return (retval);
}


int
admc_set_classicontype(const char *key, admin_icon_t newtype)
{
}


admin_icon_t
admc_get_classicontype(const char *key)
{

	_classreg_rec	record;


	if (find_class_entry(key, &record) == E_ADMIN_OK) {
		return (record.class_icon_type);
	} else {
		return (admin_err_icon);
	}
}


int
admc_set_instanceicon(const char *key, const char *newinstanceicon)
{
}


int
admc_get_instanceicon(const char *key, char *buf, size_t size)
{

	_classreg_rec	record;
	int		retval;


	if (find_class_entry(key, &record) == E_ADMIN_OK) {
		if (strlen(record.instance_icon) < size) {
			strcpy(buf, record.instance_icon);
			retval = E_ADMIN_OK;
		} else {
			strncpy(buf, record.instance_icon, size - 1);
			buf[size - 1] = '\0';
			retval = E_ADMIN_TRUNC;
		}
	} else {
		retval = E_ADMIN_FAIL;
	}

	return (retval);
}


int
admc_set_instanceicontype(const char *key, admin_icon_t newtype)
{
}


admin_icon_t
admc_get_instanceicontype(const char *key)
{

	_classreg_rec	record;


	if (find_class_entry(key, &record) == E_ADMIN_OK) {
		return (record.instance_icon_type);
	} else {
		return (admin_err_icon);
	}
}
