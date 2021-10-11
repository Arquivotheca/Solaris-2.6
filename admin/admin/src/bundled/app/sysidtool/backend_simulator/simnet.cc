/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Simhost.cc:
 * This file contains the methods that implement the simnet
 * object
 */

#pragma ident   "@(#)simnet.cc 1.3     94/07/13 SMI"

#include <iostream.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "simnet.h"

int ni_up = 0x1;
int ni_broadcast = 0x2;
int ni_debug = 0x4;
int ni_loopback = 0x8;
int ni_pointopoint = 0x10;
int ni_notrailers = 0x20;
int ni_running = 0x40;
int ni_noarp = 0x80;
int ni_promisc = 0x100;
int ni_allmulti = 0x200;
int ni_intelligent = 0x400;
int ni_multicast = 0x800;
int ni_multi_bcast = 0x1000;
int ni_unnumbered = 0x2000;
int ni_private = 0x8000;
int ni_hasnetmask = 0x10000;
int ni_hasipaddr = 0x20000;

net_interface::net_interface(char *ntype, int instno)
{
	instance_no = instno;
	flags = 0;
	type = strdup(ntype);
	if(strcmp(ntype, "lo") == 0)
		flags |= ni_loopback;
	// Should there be special code here to automatically bring
	// up loopback interfaces (i.e. set netmask, up, multicast, and 
	// running).
	qname = (char *)malloc(strlen(type)+2);
	sprintf(qname, "%s%d", type, instance_no);
	ipaddr = broadcast_addr = netmask = 0;
}

net_interface::net_interface(char *fullname)
{
	char *cp, *namep;
	int base = 1;

	namep = (char *)malloc(strlen(fullname)+2);
	strcpy(namep, fullname);
	cp = namep + strlen(namep);

	cp--;
	
	while(*cp >= '0' && *cp <= '9') {
		instance_no = (base * (*cp - '0'));
		base *= 10;
		cp--;
	}
	cp++; /* go back to first number */
	*cp = '\0';
	flags = 0;
	type = strdup(namep);
	free(namep);

	if(strcmp(type, "lo") == 0)
		flags |= ni_loopback;
	// Should there be special code here to automatically bring
	// up loopback interfaces (i.e. set netmask, up, multicast, and 
	// running).
	qname = (char *)malloc(strlen(type)+2);
	sprintf(qname, "%s%d", type, instance_no);
	ipaddr = broadcast_addr = netmask = 0;
}

net_interface::~net_interface()
{
	free(type);
	free(qname);
}

int net_interface::interface_cmp(net_interface *p)
{
	if(strcmp(p->type, type) == 0 && instance_no == p->instance_no)
		return(0);
	return(1);
}

void net_interface::set_broadcast(char *baddr)
{
	broadcast_addr = strdup(baddr);
	flags |= ni_broadcast;
}

void net_interface::set_broadcast_no_flag(char *baddr)
{
	broadcast_addr = strdup(baddr);
}

void net_interface::set_ether(char *eaddr)
{
	ether_addr = strdup(eaddr);
}

void net_interface::set_netmask(char *nmaddr)
{
	netmask = strdup(nmaddr);
	private_flags |= ni_hasnetmask;
}

void net_interface::set_ipaddr(char *addr)
{
	ipaddr = strdup(addr);
	private_flags |= ni_hasipaddr;
}

int ni_list::insert(net_interface* a) // add to head of list
{
	// First, look to see if this entry is already in the map
	nilist_iter find_iter(*this);
	net_interface *index;

	if((index = find_iter.find_by_interface(a)) != 0)
		return(-1);

	if (first) {
		a->set_next(first);
		first = a;
	} else {
		last = first = a;
		first->set_next(0);
	}
	return(0);
}

int ni_list::append(net_interface* a) // append to tail of list
{
	// First, look to see if this entry is already in the map
	nilist_iter find_iter(*this);
	net_interface *index;

	if((index = find_iter.find_by_interface(a)) != 0)
		return(-1);

	if(last) {
		last->set_next(a);
		a->set_next(0);
		last = a;
	} else {
		last = first = a;
		first->set_next(0);
	}
	return(0);
}

void ni_list::remove(net_interface* a) // append to tail of list
{
	net_interface *pindex, *index = first;

	if(a == index)
		if(first == last)
			last = first = (net_interface *)0;
		else
			first = a->get_next();
	else {
		pindex = index;
		index = pindex->get_next();
		while(index != (net_interface *)0) {
			if(a == index)
				break;
			pindex = index;
			index = pindex->get_next();
		}
		if (a == index) { // Not null
			pindex->set_next(a->get_next());
			if(a == last)
				last = pindex;
		}
	}
}

net_interface* ni_list::get()	// return and remove head of list
{
	if (first == 0)  {
		cout << "get from empty slist" << endl;
		exit(1);
	}
	net_interface * f = first->get_next();
	if (f == first)
		first = 0;
	else 
		first->set_next(f->get_next());
	return f;
}
void net_interface::up() 
{ 
	flags |= ni_up; 
}
void net_interface::down() 
{ 
	flags &= ~(ni_up); 
}
void net_interface::notrailers()
{
	flags |= ni_notrailers;
}
void net_interface::trailers()
{
	flags &= ~(ni_notrailers);
}
void net_interface::run()
{
	flags |= ni_running;
}
void net_interface::stop()
{
	flags &= ~(ni_running);
}
void net_interface::multicast()
{
    flags |= ni_multicast;
}
void net_interface::no_multicast()
{
	flags &= ~(ni_multicast);
}
int net_interface::is_up()
{
	return (flags & ni_up);
}
int net_interface::is_loopback()
{
	return (flags & ni_loopback);
}
int net_interface::using_trailers()
{
	return (!(flags & ni_notrailers));
}
int net_interface::using_multicast()
{
	return (flags & ni_multicast);
}
void net_interface::print(ostream &output)
{
	output << "\t{ " << type << " , "
	    << instance_no << " , "
	    << ipaddr << " , "
	    << netmask << " , "
	    << broadcast_addr << " , "
	    << flags << " }" << endl;
}

char *net_interface::flagstr()
{
	return("Not yet implemented");
}

void ni_list::print(ostream& output)
{
	nilist_iter find_iter(*this);
	net_interface *index;

	output << "\tinterfaces { " << endl;
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}" << endl;
}
