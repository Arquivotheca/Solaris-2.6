/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Simnet_test.cc:
 * This file is the main program for the test of the simns class methods
 */

#pragma ident   "@(#)simns_test.cc 1.2     94/03/24 SMI"

#include <iostream.h>
#include <stdlib.h>
#include "simns.h"

static void nsmap_test(char *key), hostmap_test(), bootparam_test();
static void ether_test(), timezone_test(), netmask_test();
static void nameservice_test();

main(int argc, char * const *argv)
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	int c;
	char *key = "foo";

	while ((c = getopt(argc, argv, "k:")) != EOF) {
		switch(c) {
		case 'k':
			key = optarg;
			break;
		default:
			break; // Ignore unknown args
		}
	}


	nsmap_test(key);
	hostmap_test();
	bootparam_test();
	ether_test();
	timezone_test();
	netmask_test();
	nameservice_test();
	return(0);
}

static void 
nsmap_test(char *key_to_remove)
{
	nsmap test_map(MAP_UNSPECIFIED);
	nsentry foo_entry("foo");
	nsentry bar_entry("bar");
	nsentry dwe_entry("dwe");

	test_map.insert(&foo_entry);
	test_map.insert(&bar_entry);
	test_map.insert(&dwe_entry);

	nsmap_iter test_iter(test_map);
	nsentry *ns_index;

	while(ns_index = test_iter()) {
		cout << "test iterator returned entry " << 
		   ns_index->get_key() << endl;
	}

	test_iter.reset();
	if(ns_index = test_iter.find_by_key("foo"))
		cout << "Test interator find returned entry " <<
		    ns_index->get_key() << endl;

	test_iter.reset();
	if(ns_index = test_iter.find_by_key(key_to_remove))
		cout << "Test interator find returned entry " <<
		    ns_index->get_key() << endl;
	test_map.remove(ns_index);
	test_iter.reset();
	while(ns_index = test_iter()) {
		cout << "test iterator returned entry " << 
		   ns_index->get_key() << endl;
	}
}

static void 
hostmap_test()
{

	//Now try out the host maps
	host_entry boo("boo", "129.152.221.7", NULL);
	hostmap test_hmap(&boo);
	host_entry booboo("booboo", "129.152.221.8", "bbooo");
	host_entry fubar ("fubar", "129.152.221.5", "rabuf");
	test_hmap.insert(&booboo);
	test_hmap.insert(&fubar);

	// Host entries that should be unaddable 
	host_entry b1("rabuf", "129.152.221.9", "abcd");
	host_entry b2("abcd", "129.152.221.9", "rabuf");
	host_entry b3("bbooo", "129.152.221.9", "abcd");

	int retval = test_hmap.insert(&b1);	// Should return -1
	cout << "Insert of b1 (" <<
	    b1.get_hname() << ") returned " << retval << endl;
	retval = test_hmap.insert(&b2);	// Should return -1
	cout << "Insert of b2 (" <<
	    b2.get_hname() << ") returned " << retval << endl;
	retval = test_hmap.insert(&b3);	// Should return -1
	cout << "Insert of b3 (" <<
	    b3.get_hname() << ") returned " << retval << endl;

	// nsmap_iter t1_iter(test_hmap); // Should be error

	hostmap_iter t1_iter(test_hmap);
	host_entry *hs_index;

	while(hs_index = t1_iter()) { // Should be error
		cout << "Test interator find returned entry " <<
		    hs_index->get_hname()  << " (" << hs_index->get_ipaddr()
		    << ") (" << hs_index->get_alias() << ")" << endl;
	}
}

static void
bootparam_test()
{
	//Create a bootparam map object

	bpmap test_bmap;

	bootparam_entry bp_entry("foo", "root=/export/root,swap=/export/swap");

	test_bmap.insert(&bp_entry);

	bpmap_iter bp_iter(test_bmap);

	bootparam_entry *index;

	while(index = bp_iter()) {
		cout << "Bootparam iterator returned entry " <<
		    index->get_hname() << " ( " << index->get_entry()
		    << " ) " << endl;
	}

	bootparam_entry bp1("foo", "root=/export/bar,swap=/export/bar");
	
	int retval = test_bmap.insert(&bp1);

	cout << "Insert of bp1 record (" << bp1.get_hname() << 
	    ") returned " << retval << endl;
}

static void
ether_test()
{
	//Create a bootparam map object

	ethermap test_emap;

	ether_entry e_entry("foo", "8:0:20:c:d:e");

	test_emap.insert(&e_entry);

	ethermap_iter ether_iter(test_emap);

	ether_entry *index;

	while(index = ether_iter()) {
		cout << "Ether iterator returned entry " <<
		    index->get_hname() << " ( " << index->get_eaddr()
		    << " ) " << endl;
	}

	ether_entry e1("foo", "8:0:20:1:2:3");
	
	int retval = test_emap.insert(&e1);

	cout << "Insert of e1 record (" << e1.get_hname() << 
	    "( returned " << retval << endl;
}


static void
timezone_test()
{
	//Create a bootparam map object

	timezonemap test_tzmap;

	timezone_entry tz_entry("foo", "US/Mountain");

	test_tzmap.insert(&tz_entry);

	timezonemap_iter timezone_iter(test_tzmap);

	timezone_entry *index;

	while(index = timezone_iter()) {
		cout << "Timezone iterator returned entry " <<
		    index->get_hname() << " ( " << index->get_tzone()
		    << " ) " << endl;
	}

	timezone_entry tz1("foo", "US/Mountain");
	
	int retval = test_tzmap.insert(&tz1);

	cout << "Insert of tz1 record (" << tz1.get_hname() << 
	    ") returned " << retval << endl;
}

static void
netmask_test()
{
	//Create a bootparam map object

	netmaskmap test_netmap;

	netmask_entry nm_entry("129.152.221", "255.255.255.0");

	test_netmap.insert(&nm_entry);

	netmaskmap_iter netmask_iter(test_netmap);

	netmask_entry *index;

	while(index = netmask_iter()) {
		cout << "Netmask iterator returned entry " <<
		    index->get_netnum() << " ( " << index->get_netmask()
		    << " ) " << endl;
	}

	netmask_entry nm1("129.152.221", "255.255.255.0");
	
	int retval = test_netmap.insert(&nm1);

	cout << "Insert of nm1 record (" << nm1.get_netnum() << 
	    ") returned " << retval << endl;
}
static void
nameservice_test()
{
	nameservice test_ns(NS_NIS, "foo.com");
	ns_server  test_server;

	test_ns.set_ns_server(&test_server);

	cout << "Initial ns values type " << test_ns.get_type() 
	    << " domainname " << test_ns.get_domainname()
	    << " bcast " << test_ns.is_broadcast()
	    << " server " << test_ns.get_serverhname()
	    << " serverip " << test_ns.get_serveripaddr() 
	    << endl;

	cout << "Setting broadcast" << endl;

	test_ns.set_broadcast(1);

	int retval = test_ns.set_serverhname("bar");

	cout << "Set of serverhname returned " << retval << endl;

	retval = test_ns.set_serveripaddr("129.152.221.1");

	cout << "Set of serveripaddr returned " << retval << endl;

	cout << "Clearing broadcast" << endl;

	test_ns.set_broadcast(0);

	retval = test_ns.set_serverhname("bar");

	cout << "Set of serverhname returned " << retval << endl;

	retval = test_ns.set_serveripaddr("129.152.221.1");

	cout << "Set of serveripaddr returned " << retval << endl;
}
