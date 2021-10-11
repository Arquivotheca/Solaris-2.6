/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)simhooks.c	1.3	94/08/17 SMI"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <dlfcn.h>

#include "sysidtool.h"

int 	testing = 0;

int	test_shutdown = 0;
static int	(*simulatep)(Sim_type type, ...);
static int	(*sim_initp)(char *source_state, char *dest_state);
static void	(*sim_shutdownp)();
static char	simulate_name[] = "simulate";
static char	simulate_init_name[] = "sim_init";
static char	simulate_shutdown_name[] = "sim_shutdown";
static void sim_shutdown();

void
test_enable()
{
	testing++;
}

void
test_disable()
{
	test_shutdown++;
}

int (*
sim_handle())(Sim_type type, ...)
{
	return (simulatep);
}

int
sim_load(char *dlname)
{
	void *dl_handle;

	/* Load the library and find the right symbols */
	/* Need RTLD_GLOBAL? */
	dl_handle = dlopen(dlname, RTLD_LAZY);
	if (dl_handle == (void *)0) {
		perror(dlerror());
		return (-1);
	}
	simulatep = (int (*)(Sim_type type, ...)) dlsym(dl_handle,
	    simulate_name);

	if (simulatep == (int (*)(Sim_type type, ...)) 0) {
		perror(dlerror());
		fprintf(stderr, "Cannot locate sym %s in %s\n",
		    simulate_name, dlname);
		return (-2);
	}

	sim_initp = (int (*)(char *source, char *dest)) dlsym(dl_handle,
	    simulate_init_name);

	if (sim_initp == (int (*)(char *source, char *dest)) 0) {
		perror(dlerror());
		fprintf(stderr, "Cannot locate sym %s in %s\n",
		    simulate_init_name, dlname);
		return (-3);
	}

	sim_shutdownp = (void (*)()) dlsym(dl_handle,  simulate_shutdown_name);

	if (sim_shutdownp == (void (*)()) 0) {
		perror(dlerror());
		fprintf(stderr, "Cannot locate sym %s in %s\n",
		    simulate_shutdown_name, dlname);
		return (-3);
	}
	return (0);
}

int
sim_init(char *n1, char *n2)
{
	int status;
	status = (*sim_initp)(n1, n2);

	if (status == 0) {
		/* Load sim_shutdown into the at_exit processing? */
		if (atexit(sim_shutdown) != 0) {
			fprintf(stderr, "Unable to register shutdown proc");
			return (-4);
		}
	}
	return (status);
}

static void
sim_shutdown()
{
	if (test_shutdown) {
		fprintf(stderr, "Log disabled\n");
		return;
	}
	fprintf(stderr, "Shutting down simulator\n");
	(*sim_shutdownp)();
	test_shutdown++;
}
