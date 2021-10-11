#ifndef SIMSTATE_H
#define SIMSTATE_H

/*
 * Copyright (c) 1991,1992,1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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

#pragma	ident	"@(#)simstate.h	1.1 94/04/05"

#include "simns.h"
#include "simhost.h"
#include "simns.h"
#include <iostream.h>

class Simstate {
private:
	simhost *simhost_p;
	nameservicelist *nl_p;
public:
	Simstate();
	~Simstate();
	simhost *get_simhost() { return simhost_p; }
	void set_simhost(simhost *n) { simhost_p = n; }
	nameservicelist *get_nslist() { return nl_p; }
	void set_nslist(nameservicelist *n) { nl_p = n; }
	void print(ostream& output);
};

#endif /* SIMSTATE_H */
