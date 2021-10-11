#ifndef SIMNS_H
#define SIMNS_H

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

#pragma	ident	"@(#)simns.h	1.1 94/04/05"

#include <string.h>
#include <stdlib.h>
#include <iostream.h>

typedef void (*PFV) (const char *);

PFV set_nsmap_handler(PFV a);

enum nsmaptype { MAP_UNSPECIFIED, MAP_HOST, MAP_NETMASK, MAP_TIMEZONE, 
    MAP_LOCALE, MAP_BOOTPARAM, MAP_ETHER, MAP_PASSWD };
extern const char *nsmaptype_to_string(enum nsmaptype type);

class nsentry {
private:
	nsentry *next;
	char *key;
public:
	nsentry(char *ikey)	{ key = strdup(ikey); next = 0; }
	nsentry(char *ikey, nsentry *p) { key = strdup(ikey); next = p; }
	void set_next(nsentry *p) { next = p; }
	nsentry *get_next()  { return next; }
	int key_cmp(char *ikey) { return(strcmp(ikey, key)); }
	char *get_key()		{ return key; }
};

class nsmap {
	nsentry* first;	// first member of list (next is null)
	nsentry* last; // last member of list (next is next member of list)
	enum nsmaptype type;
public:
	void insert(nsentry *);	// Add at head of list
	void append(nsentry *);	// append to tail of list
	void remove(nsentry *); // Remove from the list
	nsentry *get();		// remove and return head of list

	void clear() { last = first = 0; }
	nsmap(enum nsmaptype ntype) { type = ntype; last = first = 0; }
	nsmap(enum nsmaptype ntype, nsentry* a) { 
	    type = ntype;
	    last = first = a; a->set_next(0);  
	}
	enum nsmaptype get_maptype() { return type; }

	friend class nsmap_iter;
};

// Iterator class
class nsmap_iter {
	nsentry*	ce; 	// current element
	nsmap*		cs;	// current map
public:
	inline nsmap_iter(nsmap& s) {
		cs = &s;
		ce = cs->first;
	}
	inline nsentry* operator() () {
		// return 0 to indicate end of iteration

		nsentry* ret = ce;
		if(ce)
			ce=ce->get_next();
		return ret;
	}
	inline nsentry* find_by_key(char *ikey) {
		// Search rest of map for entry with specified
		// key. If found, return the entry, else
		// return 0
		nsentry* entry_to_cmp;

		while(entry_to_cmp = this->operator()()) {
			if(entry_to_cmp->key_cmp(ikey) == 0)
				return(entry_to_cmp);
		}
		return((nsentry *)0);
	}
	inline void reset() {
		ce = cs->first;
	}
};

// And now for the host maps classes

class host_entry: private nsentry {
private:
	char *ipaddr;
	char *alias;
public:
	host_entry(char *hname, char *nipaddr, char *nalias) : nsentry(hname) {
		ipaddr = (nipaddr != NULL) ? strdup(nipaddr) : NULL;
		alias = (nalias != NULL ) ? strdup(nalias) : NULL;
	}
	host_entry(char *hname, char *nipaddr, char *nalias, host_entry *p) : 
	    nsentry(hname, p) {
		ipaddr = (nipaddr != NULL) ? strdup(nipaddr) : NULL;
		alias = (nalias != NULL ) ? strdup(nalias) : NULL;
	}
	void set_next(host_entry *p)  { nsentry::set_next((nsentry *)p); }
	host_entry *get_next()  { return( (host_entry *)nsentry::get_next()); }
	int host_cmp(host_entry* p);
	char *get_hname()		{ return nsentry::get_key(); }
	void set_ipaddr(char *addr) { 
	    if (ipaddr) 
		free(ipaddr); 
	    ipaddr = (addr != NULL) ? strdup(addr) : NULL;
	}
	char *get_ipaddr() { return ipaddr; }
	void set_alias(char *nalias) {
	    if (alias)
		free(alias);
	    alias = (nalias != NULL) ? strdup(nalias) : NULL;
	}
	char *get_alias() { return alias; }
	void print(ostream& output);
};

class hostmap: private nsmap {
public:
	int insert(host_entry *p);
	int append(host_entry *p);
	void remove(host_entry *p) { nsmap::remove((nsentry *)p); }
	host_entry *get() { return ((host_entry *)nsmap::get()); }

	void clear() { nsmap::clear(); }
	hostmap():nsmap(MAP_HOST) {}
	hostmap(host_entry *a):nsmap(MAP_HOST, (nsentry *)a) {}
	void print(ostream& output);

	friend class hostmap_iter;
};


// Iterator class
class hostmap_iter:private nsmap_iter {
public:
	inline hostmap_iter(hostmap& s):nsmap_iter((nsmap&)s) {}
	inline host_entry* operator() () {
		return((host_entry *)nsmap_iter::operator()());
	}
	inline host_entry* find_by_hname(char *hname) {
		return((host_entry *)nsmap_iter::find_by_key(hname));
	}
	host_entry* find_by_hentry(host_entry* p);
	inline void reset() {
		nsmap_iter::reset();
	}
};

class bootparam_entry: private nsentry {
private:
	char *entry;
public:
	bootparam_entry(char *hname, char *nentry) : nsentry(hname) {
		entry = (nentry != NULL) ? strdup(nentry) : NULL;
	}
	bootparam_entry(char *hname, char *nentry, bootparam_entry *p) : 
	    nsentry(hname, p) {
		entry = (nentry != NULL) ? strdup(nentry) : NULL;
	}
	void set_next(bootparam_entry *p)  { nsentry::set_next((nsentry *)p); }
	bootparam_entry *get_next()  { 
	    return( (bootparam_entry *)nsentry::get_next()); }
	int bootparam_cmp(char *hname) { return( nsentry::key_cmp(hname)); }
	char *get_hname()		{ return nsentry::get_key(); }
	void set_entry(char *nentry) { 
	    if (entry) 
		free(entry); 
	    entry = (nentry != NULL) ? strdup(nentry) : NULL;
	}
	char *get_entry() { return entry; }
	void print(ostream& output);
};

class bpmap: private nsmap {
public:
	int insert(bootparam_entry *p);
	int append(bootparam_entry *p);
	void remove(bootparam_entry *p) { nsmap::remove((nsentry *)p); }
	bootparam_entry *get() { return ((bootparam_entry *)nsmap::get()); }

	void clear() { nsmap::clear(); }
	bpmap():nsmap(MAP_BOOTPARAM) { }
	bpmap(bootparam_entry *a):nsmap(MAP_BOOTPARAM, (nsentry *)a) { }
	void print(ostream& output);

	friend class bpmap_iter;
};


// Iterator class
class bpmap_iter:private nsmap_iter {
public:
	inline bpmap_iter(bpmap& s):nsmap_iter((nsmap&)s) {}
	inline bootparam_entry* operator() () {
		return((bootparam_entry *)nsmap_iter::operator()());
	}
	inline bootparam_entry* find_by_key(char *ikey) {
		return((bootparam_entry *)nsmap_iter::find_by_key(ikey));
	}
	/*  bootparam_entry* find_in_entry(char *str); */
	inline void reset() {
		nsmap_iter::reset();
	}
};

class ether_entry: private nsentry {
private:
	char *etheraddr;
public:
	ether_entry(char *hname, char *eaddr) : nsentry(hname) {
		etheraddr = (eaddr != NULL) ? strdup(eaddr) : NULL;
	}
	ether_entry(char *hname, char *eaddr, ether_entry *p) : 
	    nsentry(hname, p) {
		etheraddr = (eaddr != NULL) ? strdup(eaddr) : NULL;
	}
	void set_next(ether_entry *p)  { nsentry::set_next((nsentry *)p); }
	ether_entry *get_next()  { 
	    return( (ether_entry *)nsentry::get_next()); }
	int ether_cmp(char *hname) { return( nsentry::key_cmp(hname)); }
	char *get_hname()		{ return nsentry::get_key(); }
	void set_eaddr(char *eaddr) { 
	    if (etheraddr) 
		free(etheraddr); 
	    etheraddr = (eaddr != NULL) ? strdup(eaddr) : NULL;
	}
	char *get_eaddr() { return etheraddr; }
	void print(ostream& output);
};

class ethermap: private nsmap {
public:
	int insert(ether_entry *p);
	int append(ether_entry *p);
	void remove(ether_entry *p) { nsmap::remove((nsentry *)p); }
	ether_entry *get() { return ((ether_entry *)nsmap::get()); }

	void clear() { nsmap::clear(); }
	ethermap():nsmap(MAP_ETHER) {}
	ethermap(ether_entry *a):nsmap(MAP_ETHER, (nsentry *)a) {}
	void print(ostream& output);

	friend class ethermap_iter;
};


// Iterator class
class ethermap_iter:private nsmap_iter {
public:
	inline ethermap_iter(ethermap& s):nsmap_iter((nsmap&)s) {}
	inline ether_entry* operator() () {
		return((ether_entry *)nsmap_iter::operator()());
	}
	inline ether_entry* find_by_key(char *ikey) {
		return((ether_entry *)nsmap_iter::find_by_key(ikey));
	}
	inline void reset() {
		nsmap_iter::reset();
	}
};

class timezone_entry: private nsentry {
private:
	char *timezone;
public:
	timezone_entry(char *hname, char *tzone) : nsentry(hname) {
		timezone = (tzone != NULL) ? strdup(tzone) : NULL;
	}
	timezone_entry(char *hname, char *tzone, timezone_entry *p) : 
	    nsentry(hname, p) {
		timezone = (tzone != NULL) ? strdup(tzone) : NULL;
	}
	void set_next(timezone_entry *p)  { nsentry::set_next((nsentry *)p); }
	timezone_entry *get_next()  { 
	    return( (timezone_entry *)nsentry::get_next()); }
	int timezone_cmp(char *hname) { return( nsentry::key_cmp(hname)); }
	char *get_hname()		{ return nsentry::get_key(); }
	void set_tzone(char *tzone) { 
	    if (timezone) 
		free(timezone); 
	    timezone = (tzone != NULL) ? strdup(tzone) : NULL;
	}
	char *get_tzone() { return timezone; }
	void print(ostream& output);
};

class timezonemap: private nsmap {
public:
	int insert(timezone_entry *p);
	int append(timezone_entry *p);
	void remove(timezone_entry *p) { nsmap::remove((nsentry *)p); }
	timezone_entry *get() { return ((timezone_entry *)nsmap::get()); }

	void clear() { nsmap::clear(); }
	timezonemap():nsmap(MAP_TIMEZONE) {}
	timezonemap(timezone_entry *a):nsmap(MAP_TIMEZONE, (nsentry *)a) {}
	void print(ostream& output);

	friend class timezonemap_iter;
};


// Iterator class
class timezonemap_iter:private nsmap_iter {
public:
	inline timezonemap_iter(timezonemap& s):nsmap_iter((nsmap&)s) {}
	inline timezone_entry* operator() () {
		return((timezone_entry *)nsmap_iter::operator()());
	}
	inline timezone_entry* find_by_key(char *ikey) {
		return((timezone_entry *)nsmap_iter::find_by_key(ikey));
	}
	inline void reset() {
		nsmap_iter::reset();
	}
};

class locale_entry: private nsentry {
private:
	char *locale;
public:
	locale_entry(char *hname, char *nlocale) : nsentry(hname) {
		locale = (nlocale != NULL) ? strdup(nlocale) : NULL;
	}
	locale_entry(char *hname, char *nlocale, locale_entry *p) : 
	    nsentry(hname, p) {
		locale = (nlocale != NULL) ? strdup(nlocale) : NULL;
	}
	void set_next(locale_entry *p)  { nsentry::set_next((nsentry *)p); }
	locale_entry *get_next()  { 
	    return( (locale_entry *)nsentry::get_next()); }
	int locale_cmp(char *hname) { return( nsentry::key_cmp(hname)); }
	char *get_hname()		{ return nsentry::get_key(); }
	void set_locale(char *nlocale) { 
	    if (locale) 
		free(locale); 
	    locale = (nlocale != NULL) ? strdup(nlocale) : NULL;
	}
	char *get_locale() { return locale; }
	void print(ostream& output);
};

class localemap: private nsmap {
public:
	int insert(locale_entry *p);
	int append(locale_entry *p);
	void remove(locale_entry *p) { nsmap::remove((nsentry *)p); }
	locale_entry *get() { return ((locale_entry *)nsmap::get()); }

	void clear() { nsmap::clear(); }
	localemap():nsmap(MAP_LOCALE) {}
	localemap(locale_entry *a):nsmap(MAP_LOCALE, (nsentry *)a) {}
	void print(ostream& output);

	friend class localemap_iter;
};


// Iterator class
class localemap_iter:private nsmap_iter {
public:
	inline localemap_iter(localemap& s):nsmap_iter((nsmap&)s) {}
	inline locale_entry* operator() () {
		return((locale_entry *)nsmap_iter::operator()());
	}
	inline locale_entry* find_by_key(char *ikey) {
		return((locale_entry *)nsmap_iter::find_by_key(ikey));
	}
	inline void reset() {
		nsmap_iter::reset();
	}
};

class netmask_entry: private nsentry {
private:
	char *netmask;
	char *comment;
public:
	netmask_entry(char *netnum, char *nmask) : nsentry(netnum) {
		netmask = (nmask != NULL) ? strdup(nmask) : NULL;
		comment = NULL;
	}
	netmask_entry(char *netnum, char *nmask, char *cmnt) 
	    : nsentry(netnum) {
		netmask = (nmask != NULL) ? strdup(nmask) : NULL;
		comment = (cmnt != NULL) ? strdup(cmnt) : NULL;
	}
	netmask_entry(char *netnum, char *nmask, netmask_entry *p) : 
	    nsentry(netnum, (nsentry *)p) {
		netmask = (nmask != NULL) ? strdup(nmask) : NULL;
	}
	void set_next(netmask_entry *p)  { nsentry::set_next((nsentry *)p); }
	netmask_entry *get_next()  { 
	    return( (netmask_entry *)nsentry::get_next()); }
	int ether_cmp(char *netnum) { return( nsentry::key_cmp(netnum)); }
	char *get_netnum()		{ return nsentry::get_key(); }
	void set_netmask(char *nmask) { 
	    if (netmask) 
		free(netmask); 
	    netmask = (nmask != NULL) ? strdup(nmask) : NULL;
	}
	char *get_netmask() { return netmask; }
	void set_comment(char *cmt) { 
	    if (comment) 
		free(comment); 
	    comment = (cmt != NULL) ? strdup(cmt) : NULL;
	}
	char *get_comment() { return comment; }
	void print(ostream& output);
};

class netmaskmap: private nsmap {
public:
	int insert(netmask_entry *p);
	int append(netmask_entry *p);
	void remove(netmask_entry *p) { nsmap::remove((nsentry *)p); }
	netmask_entry *get() { return ((netmask_entry *)nsmap::get()); }

	void clear() { nsmap::clear(); }
	netmaskmap():nsmap(MAP_NETMASK) {}
	netmaskmap(netmask_entry *a):nsmap(MAP_NETMASK, (nsentry *)a) {}
	void print(ostream& output);

	friend class netmaskmap_iter;
};


// Iterator class
class netmaskmap_iter:private nsmap_iter {
public:
	inline netmaskmap_iter(netmaskmap& s):nsmap_iter((nsmap&)s) {}
	inline netmask_entry* operator() () {
		return((netmask_entry *)nsmap_iter::operator()());
	}
	inline netmask_entry* find_by_key(char *ikey) {
		return((netmask_entry *)nsmap_iter::find_by_key(ikey));
	}
	inline void reset() {
		nsmap_iter::reset();
	}
};

class password_entry: private nsentry {
private:
	char *password;
	int uid, gid;
	char *gcos;
	char *homedir;
	char *shell;
public:
	password_entry(char *login) : nsentry(login) {
	}
	password_entry(char *login, password_entry *p) : 
	    nsentry(login, (nsentry *)p) {
	}
	password_entry(char *login, char *pw, int nuid, int ngid,
	    char *ngcos, char *nhomedir, char *nshell) : nsentry(login) {
		password = strdup(pw);
		uid = nuid;
		gid = ngid;
		gcos = strdup(ngcos);
		homedir = strdup(nhomedir);
		shell = strdup(nshell);
	}
	password_entry(char *login, char *pw, int nuid, int ngid,
	    char *ngcos, char *nhomedir, char *nshell, password_entry *p) : 
	    nsentry(login, (nsentry *)p) {
		password = strdup(pw);
		uid = nuid;
		gid = ngid;
		gcos = strdup(ngcos);
		homedir = strdup(nhomedir);
		shell = strdup(nshell);
	}
	void set_next(password_entry *p)  { nsentry::set_next((nsentry *)p); }
	password_entry *get_next()  { 
	    return( (password_entry *)nsentry::get_next()); }
	int login_cmp(char *login) { return( nsentry::key_cmp(login)); }
	char *get_login()		{ return nsentry::get_key(); }
	void set_password(char *rp) {
	    if (password) 
		free(password); 
	    password = (rp != NULL) ? strdup(rp) : NULL;
	}
	char *get_password() { return password; }
	void set_uid(int nuid)	{ uid = nuid; }
	int get_uid()		{ return uid; }
	char *get_uidstring();
	void set_gid(int ngid)	{ gid = ngid; }
	int get_gid()		{ return gid; }
	char *get_gidstring();
	void set_gcos(char *ngcos) {
	    if (gcos) 
		free(gcos); 
	    gcos = (ngcos != NULL) ? strdup(ngcos) : NULL;
	}
	char *get_gcos() { return gcos; }
	void set_homedir(char *nhomedir) {
	    if (homedir) 
		free(homedir); 
	    homedir = (nhomedir != NULL) ? strdup(nhomedir) : NULL;
	}
	char *get_homedir() { return homedir; }
	void set_shell(char *nshell) {
	    if (shell) 
		free(shell); 
	    shell = (nshell != NULL) ? strdup(nshell) : NULL;
	}
	char *get_shell() { return shell; }
	void print(ostream& output);
};

class passwordmap: private nsmap {
public:
	int insert(password_entry *p);
	int append(password_entry *p);
	void remove(password_entry *p) { nsmap::remove((nsentry *)p); }
	password_entry *get() { return ((password_entry *)nsmap::get()); }

	void clear() { nsmap::clear(); }
	passwordmap():nsmap(MAP_PASSWD) {}
	passwordmap(password_entry *a):nsmap(MAP_PASSWD, (nsentry *)a) {}
	void print(ostream& output);

	friend class passwordmap_iter;
};


// Iterator class
class passwordmap_iter:private nsmap_iter {
public:
	inline passwordmap_iter(passwordmap& s):nsmap_iter((nsmap&)s) {}
	inline password_entry* operator() () {
		return((password_entry *)nsmap_iter::operator()());
	}
	inline password_entry* find_by_login(char *login) {
		return((password_entry *)nsmap_iter::find_by_key(login));
	}
	inline void reset() {
		nsmap_iter::reset();
	}
};

enum ns_type { NS_UNSPECIFIED, NS_NISPLUS, NS_NIS, NS_NONE };
extern const char *nstype_to_string(enum ns_type type);

class ns_server {
private:
	int broadcast;
	char *serverhname;
	char *serveripaddr;
public:
	ns_server();		// Assumes broadcast
	ns_server(char *sname, char *sip);
	int is_broadcast() { return broadcast; }
	void set_broadcast(int bcast_or_not);
	char *get_serverhname() { return serverhname; }
	int set_serverhname(char *hname);
	char *get_serveripaddr() { return serveripaddr; }
	int set_serveripaddr(char *ipaddr);
	void print(ostream& output);
};

class nameservice {
private:
	enum ns_type type;
	ns_server *nssp;
	char *domainname;
	nameservice *next;
	hostmap *hostmap_p;
	bpmap *bpmap_p;
	ethermap *ethermap_p;
	timezonemap *timezonemap_p;
	localemap *localemap_p;
	netmaskmap *netmaskmap_p;
	passwordmap *passwordmap_p;
public:
	nameservice(enum ns_type type, char *dname);
	nameservice();
	~nameservice();
	void set_broadcast(int bcast); // <> 0 for bcast, 0 otherwise
	int is_broadcast() { return (nssp->is_broadcast()); }
	void set_ns_server(ns_server *n) { nssp = n; }
	int set_serverhname(char *hname); 	// 1 success, 0 failure
	const char *get_serverhname() { return (nssp->get_serverhname()); }
	int set_serveripaddr(char *ipaddr);	// 1 success, 0 failure
	const char *get_serveripaddr() { return (nssp->get_serveripaddr()); }
	int set_domainname(char *dname);	// 0 success, <0 failure
	const char *get_domainname() { return domainname; }
	int set_type(enum ns_type type);	// 0 success, <0 failure
	enum ns_type get_type() { return type; }
	void set_next(nameservice *p) { next = p; }
	nameservice *get_next()  { return next; }
	void set_hostmap(hostmap *n)	{ hostmap_p = n; }
	hostmap *get_hostmap()		{ return hostmap_p; }
	void set_bpmap(bpmap *n)	{ bpmap_p = n; }
	bpmap *get_bpmap()		{ return bpmap_p; }
	void set_ethermap(ethermap *n)	{ ethermap_p = n; }
	ethermap *get_ethermap()	{ return ethermap_p; }
	void set_timezonemap(timezonemap *n)	{ timezonemap_p = n; }
	timezonemap *get_timezonemap()		{ return timezonemap_p; }
	void set_localemap(localemap *n)	{ localemap_p = n; }
	localemap *get_localemap()		{ return localemap_p; }
	void set_netmaskmap(netmaskmap *n)	{ netmaskmap_p = n; }
	netmaskmap *get_netmaskmap()		{ return netmaskmap_p; }
	void set_passwordmap(passwordmap *n)	{ passwordmap_p = n; }
	passwordmap *get_passwordmap()		{ return passwordmap_p; }
	void print(ostream& output);
};

class nameservicelist {
	nameservice *first;	// first member of list (next is null)
	nameservice *last; // last member of list (next is next member of list)
public:
	int insert(nameservice *);	// Add at head of list
	int append(nameservice *);	// append to tail of list
	void remove(nameservice *); // Remove from the list
	nameservice *get();		// remove and return head of list

	void clear() { last = first = 0; }
	nameservicelist() { last = first = 0; }
	nameservicelist(nameservice* a) { 
	    last = first = a; a->set_next(0);  
	}
	void print(ostream& output);
	friend class nameservicelist_iter;
};

// Iterator class
class nameservicelist_iter {
	nameservice	*ce; 	// current element
	nameservicelist	*cs;	// current list
public:
	inline nameservicelist_iter(nameservicelist& s) {
		cs = &s;
		ce = cs->first;
	}
	inline nameservice* operator() () {
		// return 0 to indicate end of iteration

		nameservice* ret = ce;
		if(ce)
			ce=ce->get_next();
		return ret;
	}
	inline nameservice* find_by_type(enum ns_type type) {
		// Search rest of map for entry with specified
		// key. If found, return the entry, else
		// return 0
		nameservice* entry_to_cmp;

		while(entry_to_cmp = this->operator()()) {
			if(entry_to_cmp->get_type() == type)
				return(entry_to_cmp);
		}
		return((nameservice *)0);
	}
	inline void reset() {
		ce = cs->first;
	}
};

#endif /* SIMNS_H */
