/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)invocation.c	1.7	94/11/16 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <launcher_p.h>


static const char	*nameservice_env = "_ADMIN_NAMESERVICE";
static const char	*transport_env = "_ADMIN_TRANSPORT";
static const char	*class_env = "_ADMIN_CLASS";
static const char	*method_env = "_ADMIN_METHOD";
static const char	*object_env = "_ADMIN_OBJECT";


static void
setup_env(
	const char	*a_nameservice,
	const char	*a_transport,
	const char	*a_class,
	const char	*a_method,
	const char	*a_object)
{

	char	env_string[1024];
	char	*e;


	sprintf(env_string, "%s=%s", nameservice_env, a_nameservice);
	e = strdup(env_string);
	putenv(e);

	sprintf(env_string, "%s=%s", transport_env, a_transport);
	e = strdup(env_string);
	putenv(e);

	sprintf(env_string, "%s=%s", class_env, a_class);
	e = strdup(env_string);
	putenv(e);

	sprintf(env_string, "%s=%s", method_env, a_method);
	e = strdup(env_string);
	putenv(e);

	sprintf(env_string, "%s=%s", object_env, a_object);
	e = strdup(env_string);
	putenv(e);
}


int
admin_execute_method(
	const char		*a_nameservice,
	const char		*a_transport,
	const char		*a_class,
	const char		*a_method,
	const char		*a_object,
	admin_geometry_t	display_coords,
	...)
{

	pid_t		cp;
	int		wstat;
	char		*executable;
	size_t		size = 1024;
	int		err;


	if (a_nameservice == NULL || a_transport == NULL ||
	    a_class == NULL || a_method == NULL) {
		return (E_ADMIN_BADARG);
	}

	/* Lookup executable in application database */

	if ((executable = (char *)malloc(size)) == NULL) {
		return (E_ADMIN_FAIL);
	}

	while (err = adma_get_executable(a_class, a_method,
	    executable, size) != E_ADMIN_OK) {
		if (err == E_ADMIN_TRUNC) {
			size *= 2;
			executable = (char *)realloc(executable, size);
			if (executable == NULL) {
				return (E_ADMIN_FAIL);
			}
		} else {
			return (E_ADMIN_FAIL);
		}
	}

	setup_env(a_nameservice, a_transport, a_class, a_method, a_object);

	if ((cp = fork()) == 0) {
		/* child */
		execlp(executable, executable,  NULL);
		/* if exec returns, it failed */
		return (E_ADMIN_FAIL);
	} else if (cp != -1) {
		/* parent */
		(void) waitpid(cp, &wstat, WNOHANG);
	} else {
		/* fork failed */
		return (E_ADMIN_FAIL);
	}

	return (E_ADMIN_OK);
}


admi_handle_t
admin_initialize(int *argc, char **argv)
{

	admin_p_handle_t	*hp;
	char			*e;
	char			buf[1024];


	hp = (admin_p_handle_t *)malloc((unsigned)sizeof (admin_p_handle_t));

	if (hp == NULL) {
		return (NULL);
	}

	memset((void *)hp, 0, sizeof (admin_p_handle_t));

	if ((e = getenv(nameservice_env)) != NULL) {
		if (strcmp(e, "ufs") == 0) {
			hp->the_nameservice = admin_ufs_nameservice;
		} else if (strcmp(e, "nis") == 0) {
			hp->the_nameservice = admin_nis_nameservice;
		} else if (strcmp(e, "nisplus") == 0) {
			hp->the_nameservice = admin_nisplus_nameservice;
		} else if (strcmp(e, "dns") == 0) {
			hp->the_nameservice = admin_dns_nameservice;
		} else if (strcmp(e, "any") == 0) {
			hp->the_nameservice = admin_any_nameservice;
		}
	} else {
		hp->the_transport = admin_err_transport;
	}

	if ((e = getenv(transport_env)) != NULL) {
		if (strcmp(e, "snag") == 0) {
			hp->the_transport = admin_snag_transport;
		}
	} else {
		hp->the_transport = admin_err_transport;
	}

	if ((e = getenv(class_env)) != NULL) {
		hp->the_class = strdup(e);
	} else {
		hp->the_class = NULL;
	}

	if ((e = getenv(method_env)) != NULL) {
		hp->the_method = strdup(e);
	} else {
		hp->the_method = NULL;
	}

	if ((e = getenv(object_env)) != NULL) {
		hp->the_object = strdup(e);
	} else {
		hp->the_object = NULL;
	}

	/* Determine and stash away the class of the object */

	switch (adma_get_method_type(hp->the_class, hp->the_method)) {
	case admin_class_method:
		if (admc_get_parentname(hp->the_class, buf, sizeof (buf)) ==
		    E_ADMIN_OK) {
			hp->the_object_class = strdup(buf);
		} else {
			hp->the_object_class = NULL;
		}
		break;
	case admin_instance_method:
		hp->the_object_class = strdup(hp->the_class);
		break;
	case admin_err_method:
	default:
		hp->the_object_class = NULL;
		break;
	}

	return ((void *)hp);
}


admin_nameservice_t
admi_get_nameservice(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp == NULL) {
		return (NULL);
	}

	return (hp->the_nameservice);
}


admin_transport_t
admi_get_transport(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp == NULL) {
		return (NULL);
	}

	return (hp->the_transport);
}


const char *
admi_get_class(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp == NULL) {
		return (NULL);
	}

	return (hp->the_class);
}


const char *
admi_get_method(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp == NULL) {
		return (NULL);
	}

	return (hp->the_method);
}


const char *
admi_get_object(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp == NULL) {
		return (NULL);
	}

	return (hp->the_object);
}


const char *
admi_get_class_of_object(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp == NULL) {
		return (NULL);
	}

	return (hp->the_object_class);
}


const char *
admi_get_display_coords(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp == NULL) {
		return (NULL);
	}

	return (hp->the_display_coords);
}


void
admi_free_handle(admi_handle_t handle)
{

	admin_p_handle_t	*hp = (admin_p_handle_t *)handle;


	if (hp != NULL) {
		if (hp->the_class != NULL)
			free((void *)hp->the_class);
		if (hp->the_method != NULL)
			free((void *)hp->the_method);
		if (hp->the_object != NULL)
			free((void *)hp->the_object);
		if (hp->the_display_coords != NULL)
			free((void *)hp->the_display_coords);

		free((void *)hp);
	}
}
