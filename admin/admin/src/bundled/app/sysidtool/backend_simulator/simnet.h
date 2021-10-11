#ifndef SIMNET_H
#define SIMNET_H

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

#pragma	ident	"@(#)simnet.h	1.1 94/04/05"

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <iostream.h>

class net_interface {
private:
	net_interface *next;
	char *type;
	char *qname;
	int instance_no;
	int flags;
	int private_flags;
	char *ipaddr;
	char *broadcast_addr;
	char *ether_addr;
	char *netmask;
public:
	net_interface(char *type, int instno);
	net_interface(char *fullname);
	~net_interface();
	const char *get_type() { return type; }
	const char *get_name() { return qname; }
	void set_flags(int nflags) { flags = nflags; }
	int get_flags() { return flags; }
	void set_next(net_interface *p) { next = p; }
	net_interface *get_next()  { return next; }
	int interface_cmp(net_interface *p);
	// Set interfaces
	void up();
	void down();
	void notrailers();
	void trailers();
	void run();
	void stop();
	void multicast();
	void no_multicast();
	void set_broadcast(char *baddr);
	void set_broadcast_no_flag(char *baddr);
	void set_ether(char *eaddr);
	void set_netmask(char *nmaddr);
	void set_ipaddr(char *addr);
	// Get interfaces
	int is_up();
	int is_loopback();
	int using_trailers();
	int using_multicast();
	const char *get_baddr()
	    { return (broadcast_addr ? broadcast_addr : (char *)0); }
	const char *get_eaddr()
	    { return (ether_addr ? ether_addr : (char *)0); }
	const char *get_netmask() 
	    { return (netmask ? netmask : (char *)0); }
	const char *get_ipaddr() 
	    { return (ipaddr ? ipaddr : (char *)0); }
	void print(ostream& output);
	char *flagstr();
};

class ni_list {
	net_interface* first;	// first member of list (next is null)
	net_interface* last;	// last member of list 
	char *name;
public:
	ni_list() { last = first = 0; }
	ni_list(net_interface* a) { 
	    last = first = a; a->set_next(0);  
	}
	int insert(net_interface *);	// Add at head of list
	int append(net_interface *);	// append to tail of list
	void remove(net_interface *); // Remove from the list
	net_interface *get();		// remove and return head of list

	void clear() { last = first = 0; }

	void print(ostream& output);
	friend class nilist_iter;
};

// Iterator class
class nilist_iter {
	net_interface*	ce; 	// current element
	ni_list*	cs;	// current map
public:
	inline nilist_iter(ni_list& s) {
		cs = &s;
		ce = cs->first;
	}
	inline net_interface* operator() () {
		// return 0 to indicate end of iteration

		net_interface* ret = ce;
		if(ce)
			ce=ce->get_next();
		return ret;
	}
	inline net_interface* find_by_interface(net_interface *p) {
		// Search rest of map for entry with specified
		// key. If found, return the entry, else
		// return 0
		net_interface* entry_to_cmp;

		while(entry_to_cmp = this->operator()()) {
			if(entry_to_cmp->interface_cmp(p) == 0)
				return(entry_to_cmp);
		}
		return((net_interface *)0);
	}
	inline void reset() {
		ce = cs->first;
	}
};

#endif /* SIMNET_H */
