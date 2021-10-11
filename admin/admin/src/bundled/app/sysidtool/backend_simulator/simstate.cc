/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Simstate.cc:
 * This file contains the methods that implement the simstate
 * object
 */

#pragma ident   "@(#)simstate.cc 1.2     94/03/24 SMI"

#include <iostream.h>
#include <stdlib.h>
#include <string.h>
#include "simstate.h"

Simstate::Simstate ()
{
	// Nothing for now
}
Simstate::~Simstate()
{
	if (simhost_p != (simhost *)0)
		delete simhost_p;
	if (nl_p != (nameservicelist *)0)
		delete nl_p;
}
void Simstate::print(ostream& output)
{
	if (simhost_p != (simhost *)0)
		simhost_p->print(output);
	if (nl_p != (nameservicelist *)0)
		nl_p->print(output);
}
