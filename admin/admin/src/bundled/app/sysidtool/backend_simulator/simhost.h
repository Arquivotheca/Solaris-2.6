#ifndef SIMHOST_H
#define SIMHOST_H

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

#pragma	ident	"@(#)simhost.h	1.2 94/07/13"

#include <iostream.h>
#include "simnet.h"
#include "simns.h"

class simsysidstate {
private:
	int flags;
	char *termtype;
public:
	simsysidstate();
	~simsysidstate();
	void set_configured(int c);
	int get_configured();
	void set_bootparamed(int c);
	int get_bootparamed();
	void set_networked(int c);
	int get_networked();
	void set_autobound(int c);
	int get_autobound();
	void set_subnetted(int c);
	int get_subnetted();
	void set_rootpass(int c);
	int get_rootpass();
	void set_locale(int c);
	int get_locale();
	void set_termtype(char *type);
	const char *get_termtype();
	void print(ostream& output);
};

class simhost {
private:
	char *hostname;
	char *domainname;
	enum ns_type domaintype;
	char *timezone;
	char *rootpass;
	ni_list *nil_p;
	simsysidstate *simsysidstate_p;
public:
	simhost();
	~simhost();
	void set_hostname(char *hname);
	const char *get_hostname() { return hostname; }
	void set_domainname(char *dname);
	const char *get_domainname() { return domainname; }
	void set_domtype(enum ns_type dtyp);
	const enum ns_type get_domtype() { return domaintype; }
	void set_rootpassword(char *rp);
	const char *get_rootpassword() { return rootpass; }
	void set_timezone(char *tzone);
	const char *get_timezone() { return timezone; }
	void set_ni_list(ni_list *n) { nil_p = n; }
	ni_list *get_ni_list() { return nil_p; }
	void set_simsysidstate(simsysidstate *n) { simsysidstate_p = n; }
	simsysidstate *get_simsysidstate() { return simsysidstate_p; }
	void print(ostream& output);
};


#endif /* SIMHOST_H */
