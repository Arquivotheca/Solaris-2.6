%{
/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Simhost.cc:
 * This file contains the lexical definitions for the
 * sysidtool backend simulator
 */

#pragma ident   "@(#)sim_parser.l 1.4     94/08/10 SMI"
#include <string.h>
#undef lex_input
#define lex_input() (\
	(\
		(\
			yytchar=yysptr>yysbuf?U(*--yysptr):getc(siminputfp) \
		)==10? \
		(yylineno++,yytchar):yytchar \
	) ==EOF ? 0 : yytchar)

%}
%o 15000
%a 15000
%Start
%%
"machine"	return(MACHINE);
"name"		return(NAME);
"domainname"	return(DOMAINNAME);
"rootpass"	return(ROOTPASS);
"sysidtool"	return(SYSIDTOOL);
"configured"	return(CONFIGURED);
"bootparamed"	return(BOOTPARAMED);
"networked"	return(NETWORKED);
"autobound"	return(AUTOBOUND);
"subnetted"	return(SUBNETTED);
"termtype"	return(TERMTYPE);
"interfaces"	return(INTERFACES);
"nameservice"	return(NAMESERVICE);
"server"	return(SERVER);
"serverip"	return(SERVERIP);
"nisplus"	return(NISPLUS);
"nis"		return(NIS);
"ufs"		return(UFS);
"type"		return(TYPE);
"map"		return(MAP);
"hosts"		return(HOSTS);
"netmask"	return(NETMASKS);
"timezone"	return(TIMEZONE);
"locale"	return(LOCALE);
"bootparams"	return(BOOTPARAMS);
"ether"		return(ETHER);
"password"	return(PASSWD);
"NULL"		return(NULLSTR);
"up"		return(IFUP);
"broadcast"	return(IFBROADCAST);
"debug"		return(IFDEBUG);
"loopback"	return(IFLOOPBACK);
"pointopoint"	return(IFPOINTOPOINT);
"notrailers"	return(IFNOTRAILERS);
"running"	return(IFRUNNING);
"noarp"		return(IFNOARP);
"promisc"	return(IFPROMISC);
"allmulti"	return(IFALLMULTI);
"intelligent"	return(IFINTELLIGENT);
"multicast"	return(IFMULTICAST);
"multibcast"	return(IFMULTIBCAST);
"unnumbered"	return(IFUNNUMBERED);
"private"	return(IFPRIVATE);
"="		return(EQUAL);
"."		return(DOT);
","		return(COMMA);
"("		return(LPAREN);
")"		return(RPAREN); /* ignore parens */
"{"		return(LCURLY);
"}"		return(RCURLY);
"<"		return(LANGLE);
">"		return(RANGLE);
";"		return(TERMINATOR);
"/"		return(SLASH);
"+"		return(PLUS);
"-"		return(MINUS);
":"		return(COLON);

0x[0-9a-fA-F][0-9a-fA-F]*	{
			yylval = (caddr_t)strtol(yytext, (char **)0, 16);
			return(VAL);
			}

0[0-7][0-7]*	{
		yylval = (caddr_t)strtol(yytext, (char **)0, 8);
		return(VAL);
		}
[0-9][0-9]*	{
		yylval = (caddr_t)strtol(yytext, (char **)0, 10);
		return(VAL);
		}
[a-zA-Z_]+[a-zA-Z0-9_-]*	{
			char *buf;

			buf = (char *) malloc(strlen(yytext) + 1);
			yylval = (caddr_t) strcpy(buf,yytext);
			return(HNAME);
		}
\"[a-zA-Z0-9\./]+[a-zA-Z0-9\./]*\" {
			static char *buf;
			int len;

			len = strlen(yytext);
			buf = (char *) malloc(len);
			memset(buf, '\0', len);
			yylval = (caddr_t) strncpy(buf,&yytext[1],len-2);
			return(RPASS);
		}

\"[a-zA-Z0-9\./,; \t]*\" {
			static char *buf;
			int len;

			len = strlen(yytext);
			buf = (char *) malloc(len);
			memset(buf, '\0', len);
			yylval = (caddr_t) strncpy(buf,&yytext[1],len-2);
			return(STRING);
		}
[\t \n]+	;
.		{ 
			extern int verbose;

			printf("<ignoring char %c>\n",*yytext); 
		}
%%
