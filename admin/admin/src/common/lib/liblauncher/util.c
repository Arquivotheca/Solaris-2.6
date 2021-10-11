/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)util.c	1.7	94/11/16 SMI"

#include <stdarg.h>
#include <string.h>
#include <launcher_p.h>


int
get_value_from_object(
	char		*buf,
	size_t		size,
	const char	*the_class,
	const char	*the_object,
	const char	*the_subclass)
{

	char		*cp;
	char		class_parent[1024];
	const char	*start_p;
	const char	*end_p;
	const char	*cur_class;
	int		i;
	int		length;
	int		retval;


	if (the_class == NULL || the_object == NULL || the_subclass == NULL ||
	    buf == NULL || size <= 0) {
		return (E_ADMIN_BADARG);
	}

	/*
	 * If the class name and the subclass part name are the same,
	 * that means that you're looking for the last component of
	 * the object name.  For example, if the object is of class
	 * Host, and Host is a subclass of Domain, and Domain is the
	 * ROOT class, then you have an object name that looks like
	 * dom_value:hostname, and you're looking for hostname.  A
	 * simple strrchr on ":" takes care of this.
	 * NOTE that if the class/subclass IS the root class, there
	 * is not going to be a colon, so just strdup and return.
	 */

	if (strcmp(the_class, the_subclass) == 0) {

		/*
		 * NOTE -- this needs to be smarter, loop on get_parent
		 * if necessary.  DON'T allow this to fail on a TRUNC
		 * error from admc_get_parentname().
		 */

		if (admc_get_parentname(the_class, class_parent,
		    sizeof (class_parent)) != E_ADMIN_OK) {
			return (E_ADMIN_FAIL);
		}

		if (strcmp(class_parent, "ROOT") == 0) {
			if (strlen(the_object) < size) {
				strcpy(buf, the_object);
				return (E_ADMIN_OK);
			} else {
				strncpy(buf, the_object, size - 1);
				buf[size - 1] = '\0';
				return (E_ADMIN_TRUNC);
			}
		}

		if ((cp = strrchr(the_object, ':')) != NULL) {
			if (strlen(cp + 1) < size) {
				strcpy(buf, cp + 1);
				return (E_ADMIN_OK);
			} else {
				strncpy(buf, cp + 1, size - 1);
				buf[size - 1] = '\0';
				return (E_ADMIN_TRUNC);
			}
		} else {
			return (E_ADMIN_FAIL);
		}
	}

	/*
	 * Go up the containment tree, count how many we go up until
	 * we hit the desired class; this will tell us how many times
	 * we have to strrchr on ':' to find the value we want.
	 */

	start_p = strrchr(the_object, ':') + 1;
	end_p = the_object + strlen(the_object);

	cur_class = the_class;

	while (strcmp(cur_class, the_subclass) != 0) {
		end_p = start_p - 1;
		start_p = end_p - 1;
		while (*start_p != ':' && start_p > the_object) {
			--start_p;
		}
		if (*start_p == ':') {
			start_p++;
		}

		if (admc_get_parentname(cur_class, class_parent,
		    sizeof (class_parent)) != E_ADMIN_OK) {
			return (E_ADMIN_FAIL);
		}

		cur_class = class_parent;
	}

	/* found it! */

	length = end_p - start_p;

	if (length < size) {
		strncpy(buf, start_p, length);
		buf[length] = '\0';
		retval = E_ADMIN_OK;
	} else {
		strncpy(buf, start_p, size - 1);
		buf[size - 1] = '\0';
		retval = E_ADMIN_TRUNC;
	}

	return (retval);
}


int
make_object_from_values(
	char		*buf,
	size_t		size,
	const char	*the_class,
	const char	*the_value,
	const char	**ancestor_classes,
	const char	**ancestor_values,
	int		num_ancestors)
{

	int		i;
	int		j;
	int		num;
	const int	max_class_path = 64;
	const char	*full_class[64];
	const char	*full_class_val[64];
	char		b[1024];
	const char	*c;


	if (buf == NULL || the_class == NULL || size <= 0) {
		return (E_ADMIN_BADARG);
	}

	full_class[0] = the_class;
	full_class_val[0] = the_value;

	c = the_class;
	for (num = 1; num < max_class_path; num++) {
		if (admc_get_parentname(c, b, sizeof (b)) !=
		    E_ADMIN_OK) {
			/* MEMORY LEAK HERE! -- clean later */
			return (E_ADMIN_FAIL);
		}

		if (strcmp(b, "ROOT") == 0) {
			break;
		}

		full_class[num] = strdup(b);
		full_class_val[num] = NULL;
		c = b;
	}

	if (num == max_class_path) {
		/* classes nested too deep */
		for (i = 1; i < max_class_path; i++) {
			free(full_class[i]);
		}
		return (E_ADMIN_FAIL);
	}

	for (i = 0; i < num_ancestors; i++) {

		for (j = 1; j < num; j++) {
			if (strcmp(ancestor_classes[i], full_class[j]) == 0) {
				full_class_val[j] = ancestor_values[i];
				break;
			}
		}
	}

	/*
	 * Work backwards, the [0] -> [num] indices go from children
	 * to ancestors, while we want to construct the name with
	 * the ancestors first.
	 */

	buf[0] = '\0';

	for (i = num - 1; i >= 0; i--) {

		if (full_class_val[i] != NULL) {
			strcat(buf, full_class_val[i]);
		}

		strcat(buf, ":");
	}

	/* NULL out final ':' */
	buf[strlen(buf) - 1] = '\0';

	return (E_ADMIN_OK);
}


int
va_make_object_from_values(
	char		*buf,
	size_t		size,
	const char	*the_class,
	const char	*the_value,
	...)
{

	int		i;
	int		cnt;
	int		retval;
	char		*p;
	const char	*classes[64];
	const char	*values[64];
	va_list		ap;


	va_start(ap, the_value);

	cnt = 0;
	while ((p = va_arg(ap, char *)) != NULL) {
		classes[cnt] = strdup(p);
		p = va_arg(ap, char *);
		values[cnt] = strdup(p);
		cnt++;
	}

	va_end(ap);

	retval = make_object_from_values(buf, size, the_class, the_value,
	    classes, values, cnt);

	for (i = 0; i < cnt; i++) {
		free(classes[i]);
		free(values[i]);
	}

	return (retval);
}
