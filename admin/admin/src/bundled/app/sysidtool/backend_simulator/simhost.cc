
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Simhost.cc:
 * This file contains the methods that implement the simhost
 * object
 */

#pragma ident   "@(#)simhost.cc 1.3     94/07/13 SMI"

#include "simhost.h"
#include <string.h>
#include <stdlib.h>

static const char nullstring[] = "";

simhost::simhost()
{
	hostname = strdup(nullstring);
	domainname = strdup(nullstring);
	rootpass = strdup(nullstring);
	domaintype = NS_UNSPECIFIED;
}
simhost::~simhost()
{
	free(hostname);
	free(domainname);
	free(rootpass);
}

void
simhost::set_hostname(char *hname)
{
	if (hostname)
		free(hostname);
	hostname = strdup(hname);
}

void
simhost::set_domainname(char *dname)
{

	if (domainname)
		free(domainname);
	domainname = strdup(dname);
}

void
simhost::set_domtype(enum ns_type type)
{
	domaintype = type;
}

void
simhost::set_timezone(char *tzone)
{
	if (timezone)
		free(timezone);
	timezone = strdup(tzone);
}

void
simhost::set_rootpassword(char *rp)
{
	if (rootpass)
		free(rootpass);
	rootpass = strdup(rp);
}

void
simhost::print(ostream& output)
{
	output << "machine {\n" << "\tname = " << hostname << ";" << endl;
	if (domainname != NULL && domainname[0] != '\0')
		output << "\tdomainname = " << domainname << ";" << endl;
	if (domaintype != NS_UNSPECIFIED)
		output << "\ttype = " << (char *) nstype_to_string(domaintype) 
		    << ";" << endl;
	if (rootpass != NULL && rootpass[0] != '\0')
		output << "\trootpass = \"" << rootpass << "\";" << endl;
	if (timezone != NULL && timezone[0] != '\0')
		output << "\ttimezone = " << timezone << ";" << endl;
	if (nil_p != (ni_list *)0) 
		nil_p->print(output);

	if (simsysidstate_p != (simsysidstate *)0)
		simsysidstate_p->print(output);
	output << "}" << endl;
}

static int configured = 0x1;
static int bootparamed = 0x2;
static int networked = 0x4;
static int autobound = 0x8;
static int subnetted = 0x10;
static int rootpass = 0x20;
static int locale = 0x40;

simsysidstate::simsysidstate()
{
	termtype = strdup(nullstring);
	flags = 0;
}
simsysidstate::~simsysidstate()
{
	free(termtype);
}
void simsysidstate::set_configured(int c)
{
	if (c)
		flags |= configured;
	else
		flags &= (~configured);
}
int simsysidstate::get_configured()
{
	return((flags & configured) ? 1 : 0);
}
void simsysidstate::set_bootparamed(int c)
{
	if (c)
		flags |= bootparamed;
	else
		flags &= (~bootparamed);
}
int simsysidstate::get_bootparamed()
{
	return((flags & bootparamed) ? 1 : 0);
}
void simsysidstate::set_networked(int c)
{
	if (c)
		flags |= networked;
	else
		flags &= (~networked);
}
int simsysidstate::get_networked()
{
	return((flags & networked) ? 1 : 0);
}
void simsysidstate::set_autobound(int c)
{
	if (c)
		flags |= autobound;
	else
		flags &= (~autobound);
}
int simsysidstate::get_autobound()
{
	return((flags & autobound) ? 1 : 0);
}
void simsysidstate::set_subnetted(int c)
{
	if (c)
		flags |= subnetted;
	else
		flags &= (~subnetted);
}
int simsysidstate::get_subnetted()
{
	return((flags & subnetted) ? 1 : 0);
}
void simsysidstate::set_rootpass(int c)
{
	if (c)
		flags |= rootpass;
	else
		flags &= (~rootpass);
}
int simsysidstate::get_rootpass()
{
	return((flags & rootpass) ? 1 : 0);
}
void simsysidstate::set_locale(int c)
{
	if (c)
		flags |= locale;
	else
		flags &= (~locale);
}
int simsysidstate::get_locale()
{
	return((flags & locale) ? 1 : 0);
}
void simsysidstate::set_termtype(char *term)
{
	if (termtype)
		free(termtype);
	termtype = strdup(term);
}
const char *simsysidstate::get_termtype()
{
	return(termtype);
}
void simsysidstate::print(ostream& output)
{
	output << "\tsysidtool {" << endl
	    << "\t\tconfigured = " << ((flags & configured) ? 1 : 0) << ";\n"
	    << "\t\tbootparamed = " << ((flags & bootparamed) ? 1 : 0) << ";\n" 
	    << "\t\tnetworked = " << ((flags & networked) ? 1 : 0) << ";\n"
	    << "\t\tautobound = " << ((flags & autobound) ? 1 : 0) << ";\n"
	    << "\t\tsubnetted = " << ((flags & subnetted) ? 1 : 0) << ";\n"
	    << "\t\trootpass = " << ((flags & rootpass) ? 1 : 0) << ";\n"
	    << "\t\tlocale = " << ((flags & locale) ? 1 : 0) << ";\n"
	    << "\t\ttermtype = " << termtype << ";\n"
	    << "\t}\n";
}
