/*
 *  Copyright (c) 1993-1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident   "@(#)simhost_test.cc 1.2     94/03/24 SMI"

/*
 * simhost_test.cc
 * A file that contains tests for the simhost object
 */

#include <iostream.h>
#include <stdlib.h>
#include "simhost.h"


int verbose = 0;

main(int argc, char * const *argv)
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	int c;
	char *hname = "none";
	char *domain = "sun.com";
	char *rootpass = "x";
	char *term = "xterms";

	while ((c = getopt(argc, argv, "vh:d:p:t:")) != EOF) {
		switch(c) {
		case 'v':
			verbose++;
			break;
		case 'h':
			hname = optarg;
			break;
		case 'd':
			domain = optarg;
			break;
		case 'p':
			rootpass = optarg;
			break;
		case 't':
			term = optarg;
			break;
		default:
			break; // Ignore unknown args
		}
	}

	simhost sh;

	cout << "Initial values  (" << sh.get_hostname() << ") (" <<
	    sh.get_domainname() << ") (" << 
	    sh.get_rootpassword() << ")" << endl;

	sh.set_hostname(hname);
	sh.set_domainname(domain);
	sh.set_rootpassword(rootpass);
	cout << "Final values  (" << sh.get_hostname() << ") (" <<
	    sh.get_domainname() << ") (" << sh.get_rootpassword() << ")" << endl;

	simsysidstate state;

	cout << "Initial values  " << state.get_configured() << " "  <<
	    state.get_bootparamed() << " " << 
	    state.get_networked() << " " << 
	    state.get_autobound() << " " << 
	    state.get_subnetted() << " " << 
	    state.get_rootpass() << " (" << 
	    state.get_termtype() << ")" << endl;

	state.set_configured(1);
	state.set_bootparamed(1);
	state.set_networked(1);
	state.set_autobound(1);
	state.set_rootpass(1);
	state.set_subnetted(1);
	state.set_termtype(term);

	cout << "Final values  " << state.get_configured() << " "  <<
	    state.get_bootparamed() << " " << 
	    state.get_networked() << " " << 
	    state.get_autobound() << " " << 
	    state.get_subnetted() << " " << 
	    state.get_rootpass() << " (" << 
	    state.get_termtype() << ")" << endl;
	return(0);
}
