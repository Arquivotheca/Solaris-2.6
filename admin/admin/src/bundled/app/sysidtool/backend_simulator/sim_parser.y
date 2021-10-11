%{
/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * sim_parser.y
 * This file contains the definition of the grammer for the
 * sysidtool backend simulator
 */

#pragma ident   "@(#)sim_parser.y 1.7     94/11/30 SMI"

#include <iostream.h>
#include <fstream.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "simstate.h"
#define YYSTYPE caddr_t
#define	UN(string) ((string) ? (string) : "")
static char Broadcast[] = "broadcast";
static int parse_verbose = 0;
static Simstate *ssp;

extern int ni_up;
extern int ni_broadcast;
extern int ni_debug;
extern int ni_loopback;
extern int ni_pointopoint;
extern int ni_notrailers;
extern int ni_running;
extern int ni_noarp;
extern int ni_promisc;
extern int ni_allmulti;
extern int ni_intelligent;
extern int ni_multicast;
extern int ni_multi_bcast;
extern int ni_unnumbered;
extern int ni_private;
extern int ni_hasnetmask;
extern int ni_hasipaddr;


ofstream fout;


FILE *siminputfp;
%}

%start simstate
%token MACHINE NAME DOMAINNAME ROOTPASS
%token SYSIDTOOL CONFIGURED BOOTPARAMED NETWORKED
%token AUTOBOUND SUBNETTED TERMTYPE
%token INTERFACES NAMESERVICE
%token TYPE NISPLUS NIS UFS SERVER SERVERIP
%token MAP HOSTS NETMASKS TIMEZONE LOCALE BOOTPARAMS ETHER PASSWD
%token LETTER ALNUM PERIOD
%token EQUAL
%token HNAME RPASS STRING
%token VAL DOT COMMA
%token LPAREN RPAREN LCURLY RCURLY LANGLE RANGLE
%token TERMINATOR
%token SLASH
%token PLUS MINUS
%token COLON NULLSTR
%token IFUP IFBROADCAST IFDEBUG IFLOOPBACK IFPOINTOPOINT IFNOTRAILERS
%token IFRUNNING IFNOARP IFPROMISC IFALLMULTI IFINTELLIGENT IFMULTICAST
%token IFMULTIBCAST IFUNNUMBERED IFPRIVATE
%%

simstate: machspec nameservicespec
	{ 
		ssp = new Simstate;
		ssp->set_simhost((simhost *)$1);
		ssp->set_nslist((nameservicelist *)$2);
	}

machspec: MACHINE LCURLY name_stmnt dom_stmnt domtype_stmnt rp_stmnt tz_stmnt interfacespec sysidspec RCURLY
	{
		simhost *simhostp;

		simhostp = new simhost;
		$$ = (caddr_t) simhostp;
		if ($3 != (caddr_t)0)
			simhostp->set_hostname((char *)$3);
		if ($4 != (caddr_t)0)
			simhostp->set_domainname((char *)$4);
		if ($5 != (caddr_t)0)
			simhostp->set_domtype((enum ns_type)$5);
		if ($6 != (caddr_t)0)
			simhostp->set_rootpassword((char *)$6);
		if ($7 != (caddr_t)0)
			simhostp->set_timezone((char *)$7);
		if ($8 != (caddr_t)0)
			simhostp->set_ni_list((ni_list *)$8);
		simhostp->set_simsysidstate((simsysidstate *)$9);
		$$ = (caddr_t)simhostp;
	}
	;

name_stmnt: NAME EQUAL domainname TERMINATOR
	{ 
		if(parse_verbose) {
			fout << "Hostname is " << (char *)$3  << endl; 
		}
		$$ = $3; 
	}
	| /* */
	{
		$$ = (caddr_t)0;
	}
	;

dom_stmnt: DOMAINNAME EQUAL domainname TERMINATOR
	{
		if (parse_verbose) {
			fout << "domainname is " << (char *)$3 << endl; 
		}
		$$ = $3;
	}
	| /* */
	{
		$$ = (caddr_t)0;
	}
	;

domtype_stmnt: /* */
	{
		$$ = (caddr_t)0;
	}
	|TYPE EQUAL nstype TERMINATOR
	{
		$$ = $3;
	}
	;

rp_stmnt: ROOTPASS EQUAL rootpass TERMINATOR
	{
		if (parse_verbose) {
			fout << "rootpassword is " << (char *)$3 << endl; 
		}
		$$ = $3;
	}
	| /* */
	{
		$$ = (caddr_t)0;
	}
	;
tz_stmnt: TIMEZONE EQUAL timezone TERMINATOR
	{
		if (parse_verbose) {
			fout << "timezone is " << (char *)$3 << endl; 
		}
		$$ = $3;
	}
	| /* */
	{
		$$ = (caddr_t)0;
	}
	;
interfacespec:  /* */
	{
		$$ = (caddr_t) 0;
	}
	| INTERFACES LCURLY intspecs RCURLY
	{
		$$ = $3;
	}
intspecs: /* */
	{
		$$ = (caddr_t)0;
	}
	| intspecs intspec
	{
		ni_list *nil_p = (ni_list *)$1;
		net_interface *nep = (net_interface *)$2;
		if (nil_p == (ni_list *)0) {
			if (parse_verbose)
				fout << "Creating net interface_list " << endl;
			nil_p = new ni_list;
		}

		// Add intspec to list
		if (parse_verbose) {
			fout << "Appending ";
			nep->print(fout);
			fout << endl;
		}
		nil_p->append(nep);
		$$ = (caddr_t)nil_p;
	}

intspec: LCURLY HNAME COMMA VAL COMMA ipaddr COMMA ipaddr COMMA ipaddr COMMA netflags RCURLY
	{
		net_interface *nep;

		nep = new net_interface((char *)$2, (int)$4);
		nep->set_flags((int)$12);
		nep->set_ipaddr((char *)$6);
		nep->set_netmask((char *)$8);
		nep->set_broadcast_no_flag((char *)$10);
		if (parse_verbose > 2) 
			nep->print(fout);
		$$ = (caddr_t)nep;
	}

ipaddr: VAL DOT VAL DOT VAL DOT VAL
	{
		if((int)$1 > 255 || (int)$3 > 255 || (int)$5 > 255 || 
		    (int)$7 > 255) {
			cerr << "A value in an ip address on line " << 
			    yylineno << "is out of bounds" << endl;
			YYERROR;
		}
		$$ = (caddr_t)malloc(21);
		sprintf($$, "%d.%d.%d.%d", $1, $3, $5, $7);
	}

netflags: VAL
	{
		$$ = $1;
	}
	| LANGLE ipflags RANGLE
	{
		$$ = $2;
	}

ipflags : /* */
	{
		$$ = 0;
	}
	| ipflag
	{
		$$ = (caddr_t)$1;
	}
	| ipflags COMMA ipflag
	{
		$$ = (caddr_t)( (ulong_t)$1 | (ulong_t)$3);
	}

ipflag:	IFUP
	{
		$$ = (caddr_t)ni_up;
	}
	| IFBROADCAST
	{
		$$ = (caddr_t)ni_broadcast;
	}
	| IFDEBUG
	{
		$$ = (caddr_t)ni_debug;
	}
	| IFLOOPBACK
	{
		$$ = (caddr_t)ni_loopback;
	}
	| IFPOINTOPOINT
	{
		$$ = (caddr_t)ni_pointopoint;
	}
	| IFNOTRAILERS
	{
		$$ = (caddr_t)ni_notrailers;
	}
	| IFRUNNING
	{
		$$ = (caddr_t)ni_running;
	}
	| IFNOARP
	{
		$$ = (caddr_t)ni_noarp;
	}
	| IFPROMISC
	{
		$$ = (caddr_t)ni_promisc;
	}
	| IFALLMULTI
	{
		$$ = (caddr_t)ni_allmulti;
	}
	| IFINTELLIGENT
	{
		$$ = (caddr_t)ni_intelligent;
	}
	| IFMULTICAST
	{
		$$ = (caddr_t)ni_multicast;
	}
	| IFMULTIBCAST
	{
		$$ = (caddr_t)ni_multi_bcast;
	}
	| IFUNNUMBERED
	{
		$$ = (caddr_t)ni_unnumbered;
	}
	| IFPRIVATE
	{
		$$ = (caddr_t)ni_private;
	}

sysidspec: SYSIDTOOL LCURLY conf_stmnt bp_stmnt net_stmnt ab_stmnt sub_stmnt rootp_stmnt locale_stmnt term_stmnt RCURLY 
	{
		simsysidstate *sssp = new simsysidstate;

		sssp->set_configured((int)$3);
		sssp->set_bootparamed((int)$4);
		sssp->set_networked((int)$5);
		sssp->set_autobound((int)$6);
		sssp->set_subnetted((int)$7);
		sssp->set_rootpass((int)$8);
		sssp->set_locale((int)$9);
		sssp->set_termtype((char *)$10);
		if (parse_verbose) {
			sssp->print(fout);
		}
		$$ = (caddr_t)sssp;
	}
	;

conf_stmnt: CONFIGURED EQUAL VAL TERMINATOR
	{
		$$ = $3;
	}
	;

bp_stmnt: BOOTPARAMED EQUAL VAL TERMINATOR
	{
		$$ = $3;
	}
	;

net_stmnt: NETWORKED EQUAL VAL TERMINATOR
	{
		$$ = $3;
	}
	;

ab_stmnt: AUTOBOUND EQUAL VAL TERMINATOR
	{
		$$ = $3;
	}
	;

sub_stmnt: SUBNETTED EQUAL VAL TERMINATOR
	{
		$$ = $3;
	}
	;

rootp_stmnt: ROOTPASS EQUAL VAL TERMINATOR
	{
		$$ = $3;
	}
	;

locale_stmnt: LOCALE EQUAL VAL TERMINATOR
	{
		$$ = $3;
	}
	;

term_stmnt: TERMTYPE EQUAL HNAME TERMINATOR
	{
		$$ = $3;
	}
	;

nameservicespec: /* */
	{
		$$ = (caddr_t)0;
	}
	| nameservicespec ns_spec
	{
		nameservicelist *nsl_p = (nameservicelist *)$1;
		nameservice *nsp = (nameservice *)$2;

		if (nsl_p == (nameservicelist *)0)
			nsl_p = new nameservicelist;

		// Add intspec to list
		if (nsl_p->append(nsp) < 0) {
			cerr << "There is already a nameservice with type "
			    << nstype_to_string(nsp->get_type())
			    << "( line " << yylineno << " )" << endl;
			YYERROR;
		}
		$$ = (caddr_t)nsl_p;
	}
	;

ns_spec: NAMESERVICE LCURLY dom_stmnt type_stmnt serv_spec mapspecs RCURLY
	{
		// mapspecs returns a nameservice without a type 
		// or domainname
		nameservice *nsp = (nameservice *)$6;
		int stat;

		if ((stat = nsp->set_type((enum ns_type)$4)) < 0) {
			cerr << "Unable to set nameservice type to " 
			    << (int) $4 << "( " << stat << " )"
			    << "( line " << yylineno << " )" << endl;
			YYERROR;
		}
		if ($3 != (caddr_t)0) {
			if((stat = nsp->set_domainname((char *)$3)) < 0) {
				cerr << 
				    "Unable to set nameservice domain name to " 
				    << (char *) $3 << "( " << stat << " )" 
				    << "( line " << yylineno << " )" << endl;
				YYERROR;
			}
		}
		nsp->set_ns_server((ns_server *)$5);
		$$ = (caddr_t)nsp;
	}
	;

type_stmnt: TYPE EQUAL nstype TERMINATOR
	{
		$$ = $3;
	}
	;

nstype: NISPLUS
	{
		$$ = (caddr_t)NS_NISPLUS;
	}
	| NIS
	{
		$$ = (caddr_t)NS_NIS;
	}
	| UFS
	{
		$$ = (caddr_t)NS_NONE;
	}
	;

serv_spec: serv_stmnt
	{
		if (strcmp((char *)$1, Broadcast) != 0) {
			cerr << 
	"Specified a nameservice host without a nameservice ip address "
			    << "( line " << yylineno << " )" << endl;
			YYERROR;
		}
		// Create server object
		ns_server *nsp = new ns_server;
		$$ = (caddr_t)nsp;
	}
	| serv_stmnt servip_stmnt
	{
		if (strcmp((char *)$1, Broadcast) == 0) {
			cerr << 
			    "Specified broadcast as well as server ip address" 
			    << "( line " << yylineno << " )" << endl;
			YYERROR;
		}
		// Create server object
		ns_server *nsp = new ns_server((char *)$1, (char *)$2);
		$$ = (caddr_t)nsp;
	}
	;

serv_stmnt: SERVER EQUAL domainname TERMINATOR
	{
		if (parse_verbose) {
			fout << "Server is " <<  (char *)$3 << endl;
		}
		$$ = $3;
	}
	| SERVER EQUAL IFBROADCAST TERMINATOR
	{
		if (parse_verbose) {
			fout << "Server is " <<  Broadcast << endl;
		}
		$$ = (caddr_t)strdup(Broadcast);
	}
	;

servip_stmnt: SERVERIP EQUAL ipaddr TERMINATOR
	{
		if (parse_verbose) {
			fout << "Server ip is " << (char *)$3 << endl;
		}
		$$ = $3;
	}
	;

mapspecs: /* */
	{
		$$ = (caddr_t)0;
	}
	| mapspecs mapspec
	{
		nameservice *nsp = (nameservice *)$1;
		nsmap *mapp = (nsmap *)$2;

		if (nsp == (nameservice *)0)
			nsp = new nameservice;

		// Add intspec to list
		switch(mapp->get_maptype()) {
		case MAP_HOST:
			nsp->set_hostmap((hostmap *)mapp);
			break;
		case MAP_NETMASK:
			nsp->set_netmaskmap((netmaskmap *)mapp);
			break;
		case MAP_TIMEZONE:
			nsp->set_timezonemap((timezonemap *)mapp);
			break;
		case MAP_LOCALE:
			nsp->set_localemap((localemap *)mapp);
			break;
		case MAP_BOOTPARAM:
			nsp->set_bpmap((bpmap *)mapp);
			break;
		case MAP_ETHER:
			nsp->set_ethermap((ethermap *)mapp);
			break;
		case MAP_PASSWD:
			nsp->set_passwordmap((passwordmap *)mapp);
			break;
		case MAP_UNSPECIFIED:
			cerr << "Got an unexpected unspecified map" 
			    << "( line " << yylineno << " )" << endl;
			YYERROR;
		}
		if(parse_verbose)
			fout << "Adding map of type " << 
			    nsmaptype_to_string(mapp->get_maptype()) << endl;
		$$ = (caddr_t)nsp;
	}
	;

mapspec: MAP LCURLY NAME EQUAL HOSTS TERMINATOR host_entries RCURLY
	{
		$$ = $7;
	}
	| MAP LCURLY NAME EQUAL NETMASKS TERMINATOR netmask_entries RCURLY
	{
		$$ = $7;
	}
	| MAP LCURLY NAME EQUAL TIMEZONE TERMINATOR timezone_entries RCURLY
	{
		$$ = $7;
	}
	| MAP LCURLY NAME EQUAL LOCALE TERMINATOR locale_entries RCURLY
	{
		$$ = $7;
	}
	| MAP LCURLY NAME EQUAL BOOTPARAMS TERMINATOR bootparam_entries RCURLY
	{
		$$ = $7;
	}
	| MAP LCURLY NAME EQUAL ETHER TERMINATOR ether_entries RCURLY
	{
		$$ = $7;
	}
	| MAP LCURLY NAME EQUAL PASSWD TERMINATOR passwd_entries RCURLY
	{
		$$ = $7;
	}
	;

host_entries: /* */
	{
		$$ = (caddr_t)0;
	}
	| host_entries host_entry
	{
		hostmap *hmp = (hostmap *)$1;
		host_entry *hep = (host_entry *)$2;
		int stat;

		// if $1 is 0, create a map
		if (hmp == (hostmap *) 0) {
			if (parse_verbose)
				fout << "Creating hostmap" << endl;
			hmp = new hostmap;
		}
		if (parse_verbose) {
			fout << "Appending " << hep->get_hname()
			    << " to hostmap" << endl;
		}
		if((stat = hmp->append(hep)) < 0) {
			cerr << "Can't add (" << hep->get_hname() << ") ("
			    << stat << " )" << endl;
		}
		$$ = (caddr_t)hmp;
	}
	;

host_entry: LCURLY random_string COMMA ipaddr host_alias RCURLY
	{
		host_entry *hep;
		// Make host entry

		hep = new host_entry((char *)$2, (char *)$4, (char *)$5);

		if (parse_verbose > 1) {
			fout << "Host entry " << 
			    (char *)hep->get_hname() << endl;
		}
		$$ = (caddr_t)hep;
	}
	;

host_alias: /* */
	{
		$$ = "";
	}
	| COMMA random_string
	{
		$$ = $2;
	}
	;

netmask_entries: /* */
	{
		$$ = (caddr_t)0;
	}
	| netmask_entries netmask_entry
	{
		netmaskmap *nmp = (netmaskmap *)$1;
		netmask_entry *nep = (netmask_entry *)$2;

		// if $1 is 0, create a map
		if (nmp == (netmaskmap *) 0) {
			if (parse_verbose)
				fout << "Creating netmaskmap" << endl;
			nmp = new netmaskmap;
		}
		if (parse_verbose) {
			fout << "Appending " << nep->get_netnum()
			    << " to netmap" << endl;
		}
		if(nmp->append(nep) < 0) {
			cerr << "Can't add (" << nep->get_netnum() << ")"
			    << endl;
		}
		$$ = (caddr_t)nmp;
	}
	;

netmask_entry: LCURLY random_string COMMA random_string COMMA string RCURLY
	{
		netmask_entry *nep;
		// Make netmask entry

		nep = new netmask_entry((char *)$2, (char *)$4, (char *)$6);

		if (parse_verbose > 1) {
			fout << "Netmask entry " << 
			    (char *)nep->get_netnum() << endl;
		}
		$$ = (caddr_t)nep;
	}
	;

timezone_entries: /* */
	{
		$$ = (caddr_t)0;
	}
	| timezone_entries timezone_entry
	{
		timezonemap *tmp = (timezonemap *)$1;
		timezone_entry *tep = (timezone_entry *)$2;

		// if $1 is 0, create a timezonemap
		if (tmp == (timezonemap *) 0) {
			if (parse_verbose)
				fout << "Creating timezonemap" << endl;
			tmp = new timezonemap;
		}
		if (parse_verbose) {
			fout << "Appending " << tep->get_hname()
			    << " to timezonemap" << endl;
		}
		if(tmp->append(tep) < 0) {
			cerr << "Can't add (" << tep->get_hname() << ")"
			    << endl;
		}
		$$ = (caddr_t)tmp;
	}
	;

timezone_entry: LCURLY random_string COMMA random_string RCURLY
	{
		timezone_entry *tep;
		// Make timezone entry

		tep = new timezone_entry((char *)$2, (char *)$4);

		if (parse_verbose > 1) {
			fout << "Timezone entry " << tep->get_hname() << endl;
		}
		$$ = (caddr_t)tep;
	}
	;

locale_entries: /* */
	{
		$$ = (caddr_t)0;
	}
	| locale_entries locale_entry
	{
		localemap *lmp = (localemap *)$1;
		locale_entry *lep = (locale_entry *)$2;

		// if $1 is 0, create a map
		if (lmp == (localemap *) 0) {
			if (parse_verbose)
				fout << "Creating localemap" << endl;
			lmp = new localemap;
		}
		if (parse_verbose) {
			fout << "Appending " << lep->get_hname()
			    << " to localemap" << endl;
		}
		if(lmp->append(lep) < 0) {
			cerr << "Can't add (" << lep->get_hname() << ")"
			    << endl;
		}
		$$ = (caddr_t)lmp;
	}
	;

locale_entry: LCURLY random_string COMMA random_string RCURLY
	{
		locale_entry *lep;
		// Make locale entry

		lep = new locale_entry((char *)$2, (char *)$4);

		if (parse_verbose > 1) {
			fout << "Locale entry " << 
			    (char *)lep->get_hname() << endl;
		}
		$$ = (caddr_t)lep;
	}
	;

bootparam_entries: /* */
	{
		$$ = (caddr_t)0;
	}
	| bootparam_entries bootparam_entry
	{
		bpmap *bmp = (bpmap *)$1;
		bootparam_entry *bep = (bootparam_entry *)$2;

		// if $1 is 0, create a map
		if (bmp == (bpmap *) 0) {
			if (parse_verbose)
				fout << "Creating bpmap" << endl;
			bmp = new bpmap;
		}
		if (parse_verbose) {
			fout << "Appending " << bep->get_hname()
			    << " to bpmap" << endl;
		}
		if(bmp->append(bep) < 0) {
			cerr << "Can't add (" << bep->get_hname() << ")"
			    << endl;
		}
		$$ = (caddr_t)bmp;
	}
	;

bootparam_entry: LCURLY HNAME COMMA bp_body RCURLY
	{
		bootparam_entry *bep;
		// Make bootparam entry

		bep = new bootparam_entry((char *)$2, (char *)$4);

		if (parse_verbose > 1) {
			fout << "Bootparam entry " << 
			    (char *)bep->get_hname() << endl;
		}
		$$ = (caddr_t)bep;
	}
	;

bp_body: bp_keyval
	{
		$$ = $1;
	}
	| bp_body COMMA bp_keyval
	{
		$$ = $3;
	}
	;

bp_keyval: HNAME EQUAL random_string
	{
		int hlen, slen;

		hlen = strlen($1);
		slen = strlen($3);

		$$ = (caddr_t)malloc(hlen+slen+2);
		sprintf($$, "%s=%s", $1, $3);
	}
	;

ether_entries: /* */
	{
		$$ = (caddr_t) 0;
	}
	| ether_entries ether_entry
	{
		ethermap *emp = (ethermap *)$1;
		ether_entry *eep = (ether_entry *)$2;

		// if $1 is 0, create a map
		if (emp == (ethermap *) 0) {
			if (parse_verbose)
				fout << "Creating ethermap" << endl;
			emp = new ethermap;
		}
		if (parse_verbose) {
			fout << "Appending " << eep->get_hname()
			    << " to ethermap" << endl;
		}
		if(emp->append(eep) < 0) {
			cerr << "Can't add (" << eep->get_hname() << ")"
			    << endl;
		}
		$$ = (caddr_t)emp;
	}
	;

ether_entry: LCURLY random_string COMMA random_string RCURLY
	{
		ether_entry *eep;
		// Make ether entry

		eep = new ether_entry((char *)$2, (char *)$4);

		if (parse_verbose > 1) {
			fout << "Ether entry " << 
			    (char *)eep->get_hname() << endl;
		}
		$$ = (caddr_t)eep;
	}
	;

passwd_entries: /* */
	{
		$$ = (caddr_t)0;
	}
	| passwd_entries passwd_entry
	{
		passwordmap *pwmp = (passwordmap *)$1;
		password_entry *pwep = (password_entry *)$2;

		// if $1 is 0, create a map
		if (pwmp == (passwordmap *) 0) {
			if (parse_verbose)
				fout << "Creating passwordmap" << endl;
			pwmp = new passwordmap;
		}
		if (parse_verbose) {
			fout << "Appending " << pwep->get_login()
			    << " to passwordmap" << endl;
		}
		if(pwmp->append(pwep) < 0) {
			cerr << "Can't add (" << pwep->get_login() << ")"
			    << endl;
		}
		$$ = (caddr_t)pwmp;
	}
	;

passwd_entry: LCURLY HNAME COMMA rootpass COMMA VAL COMMA VAL COMMA string COMMA path COMMA path RCURLY
	{
		password_entry *pwep;
		// Make password entry

		pwep = new password_entry((char *)$2, (char *)$4,
		    (int)$6, (int)$8, (char *)$10, (char *)$12, (char *)$14);

		if (parse_verbose > 1) {
			fout << "Password entry " << 
			    (char *)pwep->get_login() << endl;
		}
		$$ = (caddr_t)pwep;
	}
	;

hostname: NULLSTR
	{
		$$ = "";
	}
	|
	HNAME
	{
		$$ = $1;
	}
	;

domainname: 	hostname 
		| domainname DOT hostname
		{
			$$ = (caddr_t) malloc(strlen($1) + strlen($3) + 2);
			sprintf($$, "%s.%s", $1, $3);
		}
		;

rootpass: NULLSTR 
	{
		$$ = (caddr_t)"";
	}
	| HNAME
	{
		$$ = $1;
	}
	| RPASS
	{
		$$ = $1;
	}
	;

string: HNAME
	{
		$$ = $1;
	}
	| RPASS
	{
		$$ = $1;
	}
	| STRING
	{
		$$ = $1;
	}
	;

random_string:  /* */
        {
                $$ = "";
        }
	| random_string rscomp
	{
		int len1, len2;

		len1 = strlen($1);
		len2 = strlen($2);
		$$ = (caddr_t) malloc(len1+len2+1);
		sprintf($$, "%s%s", $1, $2);
	}
	;

rscomp: VAL
	{
		$$ = (caddr_t)malloc(20);
		sprintf($$, "%d", $1);
	}
	| DOT
	{
		$$ = ".";
	}
	| SLASH
	{
		$$ = "/";
	}
	| COLON
	{
		$$ = ":";
	}
	| LPAREN
	{
		$$ = "(";
	}
	| RPAREN
	{
		$$ = ")";
	}
	| PLUS
	{
		$$ = "+";
	}
	| MINUS
	{
		$$ = "-";
	}
	| NIS
	{
		$$ = "nis";
	}
	| NISPLUS
	{
		$$ = "nisplus";
	}
	| HNAME
	{
		$$ = $1;
	}
	;

path: HNAME
	{
		$$ = $1;
	}
	| SLASH HNAME
	{
		int hlen;

		hlen = strlen($2);
		$$ = (caddr_t) malloc(hlen+3);
		sprintf($$, "/%s", $2);
	}
	| path SLASH HNAME
	{
		int hlen, hlen2;

		hlen = strlen($1);
		hlen2 = strlen($3);
		$$ = (caddr_t) malloc(hlen+hlen2+2);
		sprintf($$, "%s/%s", $1, $3);
	}
	;

timezone: NULLSTR
	{
		$$ = (caddr_t)"";
	}
	| HNAME
	{
		$$ = $1;
	}
	| path
	{
		$$ = $1;
	}
	| HNAME PLUS VAL
	{
		int hlen;

		if ( (int)$3 > 12) {
			cerr << "Error: timezone offset out of bounds "
			    << "( line " << yylineno << " )" << endl;
			YYERROR;
		}
		hlen = strlen($1);
		$$ = (caddr_t) malloc(hlen+4);
		sprintf($$, "%s+%d\n", $1, $3);
	}
	| HNAME MINUS VAL
	{
		int hlen;

		if ((int)$3 > 12) {
			cerr << "Error: timezone offset out of bounds "
			    << "( line " << yylineno << " )" << endl;
			YYERROR;
		}
		hlen = strlen($1);
		$$ = (caddr_t) malloc(hlen+4);
		sprintf($$, "%s-%d\n", $1, $3);
	}
	;
%%
#include "lex.yy.c"

yywrap()
{
	return(1);
}

static hityyerror = 0;
void yyerror(char *str)
{
	hityyerror = 1;
	fout << endl << str << ": at '" << yytext << "'" << endl;
	return;
}

Simstate *simparse(char *fname)
{
	ssp = (Simstate *)0;

	if ((siminputfp = fopen(fname, "r")) == NULL)
		return (ssp);

	// Because of iostream stupidity, we need to
	// reopen stdout (/dev/fd/1) for printing
	fout.open("/dev/fd/1");
	yyparse();
	if (hityyerror)
		return((Simstate *)0);
	else
		return(ssp);
}

void simparse_debug()
{
	extern int yydebug;
	yydebug++;
}

void simparse_verbose()
{
	parse_verbose++;
}
