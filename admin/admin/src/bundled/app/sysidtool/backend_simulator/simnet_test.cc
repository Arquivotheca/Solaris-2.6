/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Simnet_test.cc:
 * This file is the main program for the test of the simnet class methods
 */

#pragma ident   "@(#)simnet_test.cc 1.2     94/03/24 SMI"

#include <iostream.h>
#include <stdlib.h>
#include "simnet.h"

int vflag = 0;
static void ni_test();

int 
main(int argc, char * const *argv)
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	int c;

	while ((c = getopt(argc, argv, "v")) != EOF) {
		switch(c) {
		case 'v':
			vflag++;
			break;
		default:
			break; // Ignore unknown args
		}
	}
	ni_test();
	return(0);
}

static void ni_test()
{
	net_interface le1("le", 1);
	net_interface le2("le", 2);
	net_interface le0("le", 1);

	ni_list nil(&le1);

	cout << "Test 1: Insertion testing " << endl;
	int retval = nil.insert(&le2);
	cout << "insert of " << le2.get_name() << " returned " 
	    << retval << endl;

	retval = nil.insert(&le0);

	cout << "insert of " << le0.get_name() << " returned " 
	    << retval << endl;
	
	net_interface *index;

	nilist_iter nil_i(nil);

	const int my_io_options = ios::hex;

	cout.flags(my_io_options);

	while(index = nil_i()) {
		cout << "Interface " << index->get_name() <<
		" Flags " << index->get_flags() << endl;
	}

	// Now test setting of the interface flags

	cout << endl << "Test 2: Set broadcast address test " << endl;

	le2.set_broadcast("129.152.221.255");
	nil_i.reset();
	while(index = nil_i()) {
		cout << "Interface " << index->get_name() <<
		    " Flags " << index->get_flags() << endl;
		cout << "  ipaddr " << index->get_ipaddr() <<
		    " Broadcast addr "<< index->get_baddr() <<
		    " netmask " << index->get_netmask() << endl;
	}

	cout << endl << "Test 3: Set ip address test " << endl;
	le2.set_ipaddr("129.152.221.27");

	nil_i.reset();
	while(index = nil_i()) {
		cout << "Interface " << index->get_name() <<
		    " Flags " << index->get_flags() << endl;
		cout << "  ipaddr " << index->get_ipaddr() <<
		    " Broadcast addr "<< index->get_baddr() <<
		    " netmask " << index->get_netmask() << endl;
	}
}
