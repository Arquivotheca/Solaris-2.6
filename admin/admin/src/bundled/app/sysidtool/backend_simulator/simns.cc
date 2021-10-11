/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Simhost.cc:
 * This file contains the methods that implement the simns
 * (simulated nameservice) object
 */

#pragma ident   "@(#)simns.cc 1.4     94/11/29 SMI"

#include <iostream.h>
#include <unistd.h>
#include <stdio.h>
#include "simns.h"

static void default_nsmap_handler(const char *);

static PFV nsmap_handler = &default_nsmap_handler;

void nsmap::insert(nsentry* a) // add to head of list
{
	if (first) {
		a->set_next(first);
		first = a;
	} else {
		last = first = a;
		first->set_next(0);
	}
}

void nsmap::append(nsentry* a) // append to tail of list
{
	if(last) {
		last->set_next(a);
		a->set_next(0);
		last = a;
	} else {
		last = first = a;
		first->set_next(0);
	}
}

void nsmap::remove(nsentry* a) // append to tail of list
{
	nsentry *pindex, *index = first;

	if(a == index)
		if(first == last)
			last = first = (nsentry *)0;
		else
			first = a->get_next();
	else {
		pindex = index;
		index = pindex->get_next();
		while(index != (nsentry *)0) {
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

nsentry* nsmap::get()	// return and remove head of list
{
	if (first == 0) nsmap_handler("get from empty slist");
	nsentry * f = first->get_next();
	if (f == first)
		first = 0;
	else 
		first->set_next(f->get_next());
	return f;
}

static void
default_nsmap_handler(const char *a)
{
	cout << a << endl;
	exit(1);

}

PFV set_nsmap_handler(PFV a)
{
	PFV old = nsmap_handler;
	nsmap_handler = a;
	return old;

}

/*
 * Host and hostmap specific methods
 */

int 
host_entry::host_cmp(host_entry *p)
{ 
	// The name matches if either

	// Name to name comparison
	if (nsentry::key_cmp(p->get_hname()) == 0)
		return(0);

	// Name to alias
	if (p->alias && p->alias[0] != '\0' &&
	    nsentry::key_cmp(p->alias) == 0 )
		return(0);
	// Alias to name
	if (alias && alias[0] != '\0' && p->nsentry::key_cmp(alias) == 0)
		return(0);

	// Alias to alias
	if (alias && alias[0] != '\0' && p->alias && 
	    strcmp(alias, p->alias) == 0)
		return(0);
	return(1);
}

void
host_entry::print(ostream& output)
{
	output << "\t\t{" << nsentry::get_key() << ", " << ipaddr;
	if (alias && alias[0])
		output << ", " << alias;
	output << "}\n";
}

int hostmap::insert(host_entry *p) 
{ 
	// First, look to see if this entry is already in the map
	hostmap_iter find_iter(*this);
	host_entry *index;

	if((index = find_iter.find_by_hentry(p)) != 0)
		return(-1);

	nsmap::insert((nsentry *)p); 
	return(0);
}

int hostmap::append(host_entry *p) 
{ 
	// First, look to see if this entry is already in the map
	hostmap_iter find_iter(*this);
	host_entry *index;

	if((index = find_iter.find_by_hentry(p)) != 0)
		return(-1);
	nsmap::append((nsentry *)p); 
	return(0);
}

void hostmap::print(ostream &output)
{
	// First, look to see if this entry is already in the map
	hostmap_iter find_iter(*this);
	host_entry *index;

	output << "\tmap {\n\t\tname = hosts;\n";
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}\n";
}

host_entry *hostmap_iter::find_by_hentry(host_entry* p)
{
	host_entry *index;
	while(index = (host_entry *)nsmap_iter::operator()()) {
		if(index->host_cmp(p) == 0)
			return(index);
	}
	return((host_entry *)0);
}

/*
 * Bootparam entries and bootparam map specific methodS
 */
void
bootparam_entry::print(ostream &output)
{
	output << "\t\t{" << nsentry::get_key() << ", " << entry << "}\n";
}

int bpmap::insert(bootparam_entry *p)
{
	// First, look to see if this entry is already in the map
	bpmap_iter find_iter(*this);
	bootparam_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::insert((nsentry *)p); 
	return(0);
}

int bpmap::append(bootparam_entry *p)
{
	// First, look to see if this entry is already in the map
	bpmap_iter find_iter(*this);
	bootparam_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::append((nsentry *)p); 
	return(0);
}

void bpmap::print(ostream& output)
{
	// First, look to see if this entry is already in the map
	bpmap_iter find_iter(*this);
	bootparam_entry *index;

	output << "\tmap {\n\t\tname = bootparams;\n";
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}\n";
}

/*
 * Ether entry and ether map specific entries
 */
void
ether_entry::print(ostream& output)
{
	output << "\t\t{" << nsentry::get_key() << ", " << etheraddr << "}\n";
}

int ethermap::insert(ether_entry *p)
{
	// First, look to see if this entry is already in the map
	ethermap_iter find_iter(*this);
	ether_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::insert((nsentry *)p); 
	return(1);
}
int ethermap::append(ether_entry *p)
{
	// First, look to see if this entry is already in the map
	ethermap_iter find_iter(*this);
	ether_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::append((nsentry *)p); 
	return(1);
}

void ethermap::print(ostream& output)
{
	// First, look to see if this entry is already in the map
	ethermap_iter find_iter(*this);
	ether_entry *index;

	output << "\tmap {\n\t\tname = ether;\n";
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}\n";
}

/*
 * Timezone entry and timezone map specific methods
 */
void
timezone_entry::print(ostream& output)
{
	output << "\t\t{" << nsentry::get_key() << ", " << timezone << "}\n";
}

int timezonemap::insert(timezone_entry *p)
{
	// First, look to see if this entry is already in the map
	timezonemap_iter find_iter(*this);
	timezone_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::insert((nsentry *)p); 
	return(1);
}
int timezonemap::append(timezone_entry *p)
{
	// First, look to see if this entry is already in the map
	timezonemap_iter find_iter(*this);
	timezone_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::append((nsentry *)p); 
	return(1);
}

void timezonemap::print(ostream& output)
{
	// First, look to see if this entry is already in the map
	timezonemap_iter find_iter(*this);
	timezone_entry *index;

	output << "\tmap {\n\t\tname = timezone;\n";
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}\n";
}

/*
 * Locale entry and locale map specific methods
 */

void
locale_entry::print(ostream& output)
{
	output << "\t\t{" << nsentry::get_key() << ", " << locale << "}\n";
}

int localemap::insert(locale_entry *p)
{
	// First, look to see if this entry is already in the map
	localemap_iter find_iter(*this);
	locale_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::insert((nsentry *)p); 
	return(1);
}
int localemap::append(locale_entry *p)
{
	// First, look to see if this entry is already in the map
	localemap_iter find_iter(*this);
	locale_entry *index;

	if((index = find_iter.find_by_key(p->get_hname())) != 0)
		return(-1);

	nsmap::append((nsentry *)p); 
	return(1);
}
void localemap::print(ostream& output)
{
	// First, look to see if this entry is already in the map
	localemap_iter find_iter(*this);
	locale_entry *index;

	output << "\tmap {\n\t\tname = locale;\n";
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}\n";
}

/*
 * Netmask entry and netmask map specific methods
 */

void
netmask_entry::print(ostream& output)
{
	output << "\t\t{" << nsentry::get_key() << ", " << netmask << 
	    ", \"" << comment << "\"}\n";
}

int netmaskmap::insert(netmask_entry *p)
{
	// First, look to see if this entry is already in the map
	netmaskmap_iter find_iter(*this);
	netmask_entry *index;

	if((index = find_iter.find_by_key(p->get_netnum())) != 0)
		return(-1);

	nsmap::insert((nsentry *)p); 
	return(1);
}
int netmaskmap::append(netmask_entry *p)
{
	// First, look to see if this entry is already in the map
	netmaskmap_iter find_iter(*this);
	netmask_entry *index;

	if((index = find_iter.find_by_key(p->get_netnum())) != 0)
		return(-1);

	nsmap::append((nsentry *)p); 
	return(1);
}

void netmaskmap::print(ostream& output)
{
	// First, look to see if this entry is already in the map
	netmaskmap_iter find_iter(*this);
	netmask_entry *index;

	output << "\tmap {\n\t\tname = netmask;\n";
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}\n";
}

/*
 * Password entry and password specific methods
 */
void
password_entry::print(ostream& output)
{
	output << "\t\t{" << nsentry::get_key() << " , " 
	    << "\"" << password << "\"  , "
	    << uid << " , "
	    << gid << " , "
	    << "\"" << gcos << "\" , "
	    << homedir << " , "
	    << shell <<  " }\n";
}

char *
password_entry::get_uidstring()
{
	char *cp;

	cp = (char *)malloc(10);
	sprintf(cp, "%d", uid);
	return(cp);
}

char *
password_entry::get_gidstring()
{
	char *cp;

	cp = (char *)malloc(10);
	sprintf(cp, "%d", gid);
	return(cp);
}

int passwordmap::insert(password_entry *p)
{
	// First, look to see if this entry is already in the map
	passwordmap_iter find_iter(*this);
	password_entry *index;

	if((index = find_iter.find_by_login(p->get_login())) != 0)
		return(-1);

	nsmap::insert((nsentry *)p); 
	return(1);
}
int passwordmap::append(password_entry *p)
{
	// First, look to see if this entry is already in the map
	passwordmap_iter find_iter(*this);
	password_entry *index;

	if((index = find_iter.find_by_login(p->get_login())) != 0)
		return(-1);

	nsmap::append((nsentry *)p); 
	return(1);
}

void passwordmap::print(ostream& output)
{
	// First, look to see if this entry is already in the map
	passwordmap_iter find_iter(*this);
	password_entry *index;

	output << "\tmap {\n\t\tname = password;\n";
	while(index = find_iter()) {
		index->print(output);
	}
	output << "\t}\n";
}

/*
 * ns_server methods
 */

static char nullstring[] = "";
ns_server::ns_server()
{
	broadcast = 1;
	serverhname = strdup(nullstring);
	serveripaddr = strdup(nullstring);
}
ns_server::ns_server(char *hname, char *ipaddr)
{
	broadcast = 0;
	serverhname = strdup(hname);
	serveripaddr = strdup(ipaddr);
}
void ns_server::set_broadcast(int bcast_or_not)
{
	broadcast = (bcast_or_not) ? 1 : 0;
}
int ns_server::set_serverhname(char *hname)
{
	if (broadcast)
		return (-1);
	if(serverhname)
		free(serverhname);
	serverhname = strdup(hname);
	return(0);
}
int ns_server::set_serveripaddr(char *ipaddr)
{
	if (broadcast)
		return (-1);
	if(serveripaddr)
		free(serveripaddr);
	serveripaddr = strdup(ipaddr);
	return(0);
}

void
ns_server::print(ostream& output)
{
	if(broadcast)
		output << "\tserver = broadcast;" << endl;
	else
		output << "\tserver = " << serverhname << ";" << endl
		    << "\tserverip = " << serveripaddr << ";" << endl;
}

nameservice::nameservice(enum ns_type ns_type, char *dname)
{
	type = ns_type;
	domainname = strdup(dname);
	nssp = (ns_server *)0;
	hostmap_p = (hostmap *)0;
	bpmap_p = (bpmap *)0;
	ethermap_p = (ethermap *)0;
	timezonemap_p = (timezonemap *)0;
	localemap_p = (localemap *)0;
	netmaskmap_p = (netmaskmap *)0;
	passwordmap_p = (passwordmap *)0;
}
nameservice::nameservice()
{
	type = NS_UNSPECIFIED;
	domainname = strdup(nullstring);
	nssp = (ns_server *)0;
	hostmap_p = (hostmap *)0;
	bpmap_p = (bpmap *)0;
	ethermap_p = (ethermap *)0;
	timezonemap_p = (timezonemap *)0;
	localemap_p = (localemap *)0;
	netmaskmap_p = (netmaskmap *)0;
	passwordmap_p = (passwordmap *)0;
}
nameservice::~nameservice()
{
	if (nssp != (ns_server *)0)
		delete nssp;
	free(domainname);
	// XXX: delete maps?
}
int nameservice::set_type(enum ns_type ntype)
{
	if(type != NS_UNSPECIFIED) {
		return(-1);
	}
	if(ntype == NS_UNSPECIFIED)
		return(-2);
	type = ntype;
	return(0);
}
int nameservice::set_domainname(char *dname)
{
	if(type == NS_UNSPECIFIED)
		return(-1);
	if(strcmp(domainname, nullstring) != 0)
		return(-2);
	if(dname == (char *)0 || strcmp(dname, nullstring) == 0)
		return(-3);
	free(domainname);
	domainname = strdup(dname);
	return(0);
}
void nameservice::set_broadcast(int bcast_or_not)
{
	nssp->set_broadcast(bcast_or_not);
}
int nameservice::set_serverhname(char *hname)
{
	return(nssp->set_serverhname(hname));
}
int nameservice::set_serveripaddr(char *ipaddr)
{
	return(nssp->set_serveripaddr(ipaddr));
}

void nameservice::print(ostream& output)
{
	output << "nameservice {" << endl;
	
	if (domainname != (char *)0 && domainname[0] != '\0')
		output<< "\tdomainname = " << domainname << ";" << endl;

	output << "\ttype = " << nstype_to_string(type) << ";" << endl;

	if(nssp != (ns_server *)0)
		nssp->print(output);

	if (hostmap_p != (hostmap *)0)
		hostmap_p->print(output);

	if (bpmap_p != (bpmap *) 0)
		bpmap_p->print(output);

	if (ethermap_p != (ethermap *) 0)
		ethermap_p->print(output);
	
	if (timezonemap_p != (timezonemap *)0)
		timezonemap_p->print(output);
	
	if (localemap_p != (localemap *)0)
		localemap_p->print(output);
	
	if (netmaskmap_p != (netmaskmap *)0)
		netmaskmap_p->print(output);
	
	if (passwordmap_p != (passwordmap *)0)
		passwordmap_p->print(output);
	
	output << "}" << endl;
}


int nameservicelist::insert(nameservice* a) // add to head of list
{
	// First, look to see if this entry is already in the map
	nameservicelist_iter find_iter(*this);
	nameservice *index;
	enum ns_type type = a->get_type();

	if((index = find_iter.find_by_type(type)) != 0)
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

int nameservicelist::append(nameservice* a) // append to tail of list
{
	// First, look to see if this entry is already in the map
	nameservicelist_iter find_iter(*this);
	nameservice *index;
	enum ns_type type = a->get_type();

	if((index = find_iter.find_by_type(type)) != 0)
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

void nameservicelist::remove(nameservice* a) // append to tail of list
{
	nameservice *pindex, *index = first;

	if(a == index)
		if(first == last)
			last = first = (nameservice *)0;
		else
			first = a->get_next();
	else {
		pindex = index;
		index = pindex->get_next();
		while(index != (nameservice *)0) {
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

nameservice* nameservicelist::get()	// return and remove head of list
{
	if (first == 0) nsmap_handler("get from empty nameservicelist");
	nameservice * f = first->get_next();
	if (f == first)
		first = 0;
	else 
		first->set_next(f->get_next());
	return f;
}

void nameservicelist::print(ostream& output) {
	// First, look to see if this entry is already in the map
	nameservicelist_iter find_iter(*this);
	nameservice *index;

	while(index = find_iter()) {
		index->print(output);
	}
}

/*
 * Routines for translating enums defined in simns.h to
 * human readable string
 */

const char *
nstype_to_string(enum ns_type type)
{
	char *str;

	switch(type)
	{
	case NS_NISPLUS:
		str = "nisplus";
		break;
	case NS_NIS:
		str = "nis";
		break;
	case NS_NONE:
		str = "ufs";
		break;
	case NS_UNSPECIFIED:
		str = "unspecified";
		break;
	}
	return(str);
}

const char *
nsmaptype_to_string(enum nsmaptype type)
{
	char *str;

	switch(type)
	{
	case MAP_HOST:
		str = "host";
		break;
	case MAP_NETMASK:
		str = "netmask";
		break;
	case MAP_TIMEZONE:
		str = "timezone";
		break;
	case MAP_LOCALE:
		str = "locale";
		break;
	case MAP_BOOTPARAM:
		str = "bootparams";
		break;
	case MAP_ETHER:
		str = "ether";
		break;
	case MAP_PASSWD:
		str = "password";
		break;
	case MAP_UNSPECIFIED:
		str = "password";
		break;
	}
	return(str);
}
