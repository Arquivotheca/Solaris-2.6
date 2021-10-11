
/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reeserved.
 */

#pragma	ident	"@(#)appreg.c 1.7     94/11/16 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <launcher_p.h>


#define APP_RECORD_P_FMT	"%s %s %s %s %s\n"
#define APP_RECORD_S_FMT	"%s %s %s %s %s"
#define DB_DIR			"/usr/snadm/etc"

typedef struct _s_appreg_rec {
	char			*class_name;
	char			*method_name;
	char			*executable;
	admin_method_t		method_type;
	admin_nameservice_t	*valid_nameservice;
	int			num_valid_nameservices;
} _appreg_rec;

static const char	*app_db_dir = DB_DIR;
static const char	*app_db = "/usr/snadm/etc/appreg.db";
static FILE		*app_fp = NULL;


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
method_to_string(admin_method_t the_method)
{
	switch (the_method) {
	case admin_class_method:
		return ("class");
	case admin_instance_method:
		return ("instance");
	default:
		return ("unknown");
	}
}


admin_method_t
string_to_method(const char *the_method)
{
	if (strcmp(the_method, "class") == 0) {
		return (admin_class_method);
	} else if (strcmp(the_method, "instance") == 0) {
		return (admin_instance_method);
	} else {
		return (admin_err_method);
	}
}


static const char *
nameservice_to_string(admin_nameservice_t the_nameservice)
{
	switch (the_nameservice) {
	case admin_ufs_nameservice:
		return ("ufs");
	case admin_nis_nameservice:
		return ("nis");
	case admin_nisplus_nameservice:
		return ("nisplus");
	case admin_dns_nameservice:
		return ("dns");
	case admin_any_nameservice:
		return ("any");
	default:
		return ("unknown");
	}
}


static admin_nameservice_t
string_to_nameservice(const char *the_nameservice)
{
	if (strcmp(the_nameservice, "ufs") == 0) {
		return (admin_ufs_nameservice);
	} else if (strcmp(the_nameservice, "nis") == 0) {
		return (admin_nis_nameservice);
	} else if (strcmp(the_nameservice, "nisplus") == 0) {
		return (admin_nisplus_nameservice);
	} else if (strcmp(the_nameservice, "dns") == 0) {
		return (admin_dns_nameservice);
	} else if (strcmp(the_nameservice, "any") == 0) {
		return (admin_any_nameservice);
	} else {
		return (admin_err_nameservice);
	}
}


static int
find_app_entry(
	const char	*class_name,
	const char	*method_name,
	_appreg_rec	*record)
{

	FILE	*app_fp;
	int	fd;
	int	retval;
	int	i;
	char	*cp;
	char	*start_p;
	char	c[256];
	char	m[256];
	char	e[256];
	char	mt[256];
	char	n[256];


	if (class_name == NULL || method_name == NULL || record == NULL) {
		return (E_ADMIN_BADARG);
	}

	/* Open and lock the file */

	app_fp = fopen(app_db, "r");

	if (app_fp == NULL) {
		return (E_ADMIN_FAIL);
	}

	fd = fileno(app_fp);

	if (lock_file(fd) == -1) {
		fclose(app_fp);
		return (E_ADMIN_FAIL);
	}

	/*
	 * Look through the file for the record named by the
	 * "class_name/method_name" composite key.  If found,
	 * fill in the return structure and break out of  the
	 * loop.  After the iteration ends, either by reading
	 * the entire file or breaking out, the file is closed,
	 * unlocked, and we return.
	 */

	retval = E_ADMIN_FAIL;

	while (fscanf(app_fp, APP_RECORD_S_FMT, c, m, e, mt, n) != EOF) {

		if (strcmp(class_name, c) == 0 &&
		    strcmp(method_name, m) == 0) {

			record->class_name = strdup(c);
			record->method_name = strdup(m);
			record->executable = strdup(e);
			record->method_type = string_to_method(mt);

			/*
			 * Count the commas so we know how many nameservices
			 * are supported (how many strings to allocate).
			 */

			record->num_valid_nameservices = 1;

			start_p = n;
			while ((cp = strchr(start_p, ',')) != NULL) {
				record->num_valid_nameservices++;
				start_p = cp + 1;
			}

			record->valid_nameservice =
			    (admin_nameservice_t *)malloc((size_t)(record->num_valid_nameservices * sizeof (admin_nameservice_t)));

			start_p = n;
			for (i = 0; i < record->num_valid_nameservices; i++) {
				if ((cp = strchr(start_p, ',')) != NULL) {
					*cp = '\0';
				}
				record->valid_nameservice[i] =
					string_to_nameservice(start_p);
				start_p = cp + 1;
			}

			retval = E_ADMIN_OK;

			break;
		}
	}

	/* close and unlock the file */

	fclose(app_fp);
	(void) unlock_file(fd);

	return (retval);
}


int
adma_register(
	const char		*class_name,
	const char		*method_name,
	const char		*executable,
	admin_method_t		method_type,
	admin_nameservice_t	*valid_nameservice,
	int			num_valid_nameservices)
{

	FILE		*app_fp;
	FILE		*tmpfp;
	char		*tmpname;
	int		i;
	int		err;
	int		fd;
	const char	**cp;
	char		c[256];
	char		m[256];
	char		e[256];
	char		mt[256];
	char		n[256];
	const char	*mt_string;
	char		n_string[1024];
	_appreg_rec	dummy;


	app_fp = fopen(app_db, "r");

	if (app_fp == NULL) {
		return E_ADMIN_FAIL;
	}

	fd = fileno(app_fp);

	/* attempt to lock the entire file, fail if unsuccessful */

	if (lock_file(fd) == -1) {
		return E_ADMIN_FAIL;
	}

	/*
	 * try to find this record in the application registry, return
	 * an error if it already exists as we don't allow duplicates.
	 */

	if (find_app_entry(class_name, method_name, &dummy) == E_ADMIN_OK) {
		fclose(app_fp);
		unlock_file(fd);
		return (E_ADMIN_EXISTS);
	}

	if ((tmpname = tempnam(app_db_dir, "admin")) == NULL) {
		fclose(app_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	if ((tmpfp = fopen(tmpname, "w")) == NULL) {
		fclose(app_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	mt_string = method_to_string(method_type);

	if (num_valid_nameservices > 0) {

		strcpy(n_string, nameservice_to_string(valid_nameservice[0]));

		for (i = 1; i < num_valid_nameservices; i++) {
			strcat(n_string, ",");
			strcat(n_string,
			    nameservice_to_string(valid_nameservice[i]));
		}
	} else {
		n_string[0] = '\0';
	}

	/*
	 * Read each record from the database, update it if
	 * necessary, write it to the temp file, write the
	 * new record to the end of the temp file, close the
	 * temp file, and move it back on top of the real
	 * file.
	 */

	while (fscanf(app_fp, APP_RECORD_S_FMT, c, m, e, mt, n) != EOF) {
		/* just copy straight across */
		fprintf(tmpfp, APP_RECORD_P_FMT, c, m, e, mt, n);
	}

	/* Write the new record */
	fprintf(tmpfp, APP_RECORD_P_FMT, class_name, method_name,
	    executable, mt_string, n_string);

	/* close the files */

	fclose(tmpfp);
	fclose(app_fp);

	/* Move the new, updated file over the old one */
	if (rename(tmpname, app_db) == -1) {
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
adma_unregister(const char *the_class, const char *the_method)
{

	FILE		*app_fp;
	FILE		*tmpfp;
	char		*tmpname;
	int		fd;
	char		c[256];
	char		m[256];
	char		e[256];
	char		mt[256];
	char		n[256];
	const char	*mt_string;
	const char	*n_string;
	int		retval = E_ADMIN_NOCLASS;


	if ((app_fp = fopen(app_db, "r")) == NULL) {
		return (E_ADMIN_FAIL);
	}

	fd = fileno(app_fp);

	/* attempt to lock the entire file, fail if unsuccessful */

	if (lock_file(fd) == -1) {
		fclose(app_fp);
		return (E_ADMIN_FAIL);
	}

	if ((tmpname = tempnam(app_db_dir, "admin")) == NULL) {
		fclose(app_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	if ((tmpfp = fopen(tmpname, "w")) == NULL) {
		fclose(app_fp);
		unlock_file(fd);
		return (E_ADMIN_FAIL);
	}

	/*
	 * Read each record from the database, copy it to the output
	 * file as long as it doesn't match the key that was passed
	 * in as the record to delete.
	 */

	while(fscanf(app_fp, APP_RECORD_S_FMT, c, m, e, mt, n) != EOF) {

		if (strcmp(the_class, c) == 0 &&
		    strcmp(the_method, m) == 0) {
			retval = E_ADMIN_OK;
			continue;
		}

		fprintf(tmpfp, APP_RECORD_P_FMT, c, m, e, mt, n);
	}

	/* close the files */

	fclose(tmpfp);
	fclose(app_fp);

	/* Move the new, updated file over the old one */
	if (rename(tmpname, app_db) == -1) {
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
adma_unregister_class(const char *the_class)
{
}


int
adma_set_executable(
	const char	*the_class,
	const char	*the_method,
	const char	*executable)
{
}


int
adma_get_executable(
	const char	*the_class,
	const char	*the_method,
	char		*buf,
	size_t		size)
{

	_appreg_rec	record;
	int		retval;


	if (find_app_entry(the_class, the_method, &record) == E_ADMIN_OK) {
		if (strlen(record.executable) < size) {
			strcpy(buf, record.executable);
			retval = E_ADMIN_OK;
		} else {
			strncpy(buf, record.executable, size - 1);
			buf[size - 1] = '\0';
			retval = E_ADMIN_TRUNC;
		}
	} else {
		retval = E_ADMIN_FAIL;
	}

	return (retval);
}


int
adma_set_method_type(
	const char	*the_class,
	const char	*the_method,
	admin_method_t	the_type)
{
}


admin_method_t
adma_get_method_type(const char *the_class, const char *the_method)
{

	_appreg_rec	record;


	if (find_app_entry(the_class, the_method, &record) == E_ADMIN_OK) {
		return (record.method_type);
	} else {
		return (admin_err_method);
	}
}


int
adma_set_valid_nameservices(
	const char		*the_class,
	const char		*the_method,
	admin_nameservice_t	*valid_nameservices,
	int			num_valid_nameservices)
{
}


admin_nameservice_t *
adma_get_valid_nameservices(
	const char	*the_class,
	const char	*the_method,
	int		*num_valid_nameservices)
{

	_appreg_rec	record;


	if (num_valid_nameservices == NULL) {
		return (NULL);
	}
	if (find_app_entry(the_class, the_method, &record) == E_ADMIN_OK) {
		*num_valid_nameservices = record.num_valid_nameservices;
		return (record.valid_nameservice);
	} else {
		return (NULL);
	}
}


static
char **
_get_methods(
	const char	*the_class,
	int		*num_methods,
	admin_method_t	m_type)
{

	FILE		*app_fp;
	int		fd;
	char		**retval;
	int		i;
	char		*cp;
	char		*start_p;
	char		c[256];
	char		m[256];
	char		e[256];
	char		mt[256];
	char		n[256];
	const char	*method_type;


	/* Open and lock the file */

	app_fp = fopen(app_db, "r");

	if (app_fp == NULL) {
		return (NULL);
	}

	fd = fileno(app_fp);

	if (lock_file(fd) == -1) {
		fclose(app_fp);
		return (NULL);
	}

	method_type = method_to_string(m_type);

	/*
	 * XXX First count them, then malloc and go again -- prototype,
	 * XXX inefficient, change to do one pass!
	 */

	*num_methods = 0;
	while (fscanf(app_fp, APP_RECORD_S_FMT, c, m, e, mt, n) != EOF) {
		if (strcmp(the_class, c) == 0 &&
		    strcmp(method_type, mt) == 0) {
			(*num_methods)++;
		}
	}

	rewind(app_fp);

	retval = (char **)malloc((size_t)(*num_methods * sizeof (char *)));

	if (retval == NULL) {
		fclose(app_fp);
		(void) unlock_file(fd);
		return (NULL);
	}

	i = 0;
	while (fscanf(app_fp, APP_RECORD_S_FMT, c, m, e, mt, n) != EOF) {
		if (strcmp(the_class, c) == 0 &&
		    strcmp(method_type, mt) == 0) {
			retval[i++] = strdup(m);
		}
	}
	/* close and unlock the file */

	fclose(app_fp);
	(void) unlock_file(fd);

	return (retval);
}


char **
adma_get_class_methods(
	const char	*the_class,
	int		*num_methods)
{

	if (the_class == NULL || num_methods == NULL) {
		return (NULL);
	}

	*num_methods = 0;
	return (_get_methods(the_class, num_methods, admin_class_method));
}


char **
adma_get_instance_methods(
	const char	*the_class,
	int		*num_methods)
{

	if (the_class == NULL || num_methods == NULL) {
		return (NULL);
	}

	*num_methods = 0;
	return (_get_methods(the_class, num_methods, admin_instance_method));
}
