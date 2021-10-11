/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)launcher_p.h	1.3	94/11/16 SMI"

#include <launcher.h>

typedef struct {
	admin_nameservice_t	the_nameservice;
	admin_transport_t	the_transport;
	const char		*the_class;
	const char		*the_method;
	const char		*the_object;
	const char		*the_object_class;
	const char		*the_display_coords;
} admin_p_handle_t;
