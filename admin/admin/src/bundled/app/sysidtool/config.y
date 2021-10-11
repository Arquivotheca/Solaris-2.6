%{ 
/*
 *  Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident   "@(#)config.y 1.4 96/06/14"

/*
 * config.y - functions for parsing the sysid preconfiguration file
 *
 * These functions are called by the read_config_file function
 * to parse the configuration file, check for valid syntax, and
 * build the internal configuration data structure
 *
 */


#include "sysidtool.h"
#include <string.h>
#include <ctype.h>
#include "sysid_preconfig.h"

int yyparse(void);

/* static prototypes */
static void yyerror( char* ch );
static int yylex(void);
static int next_char(FILE *f);
static void push_char(int ch);

/* STATICS */
static char cur_value[MAXPATHLEN];
static char err_string[256];
static int cur_tok_beg_pos;
static int cur_tok_beg_line;

%}

 %union {
	char	str[MAXPATHLEN];
}


%token INSTALL_LOCALE SYSTEM_LOCALE TERMINAL TIMEZONE
%token ROOT_PASSWORD MONITOR 
%token NETWORK_INTERFACE NAME_SERVICE KEYBOARD DISPLAY POINTER
%token HOSTNAME IP_ADDRESS NETMASK DOMAIN_NAME NAME_SERVER
%token LAYOUT SIZE DEPTH RESOLUTION NBUTTONS IRQ
%token NIS_TOK NISPLUS_TOK NONE OTHER
%token <str> IP_ADDR PARTIAL_IPADDR
%token <str> RESOLUTION_VAL
%token <str> NUMBER
%token <str> STRING
%token <str> IDENT
%type <str> ip_addr 
%type <str> ns_value zone_info
%%

config_file
		:	config_entry config_file
		|	/* EMPTY */
		;

config_entry
		:	uni_spec
		|	multi_spec
		;

uni_spec
		:	INSTALL_LOCALE '=' IDENT
		{
			if (!sysid_valid_install_locale($3)) {
				sprintf(err_string,
					"%s is not a valid install locale",$3);
				yyerror(err_string);
				YYERROR ;
				/* NOTREACHED */
			}
			else {
				create_config_entry( CFG_INSTALL_LOCALE, $3); 
			}
		}
		|	SYSTEM_LOCALE '=' IDENT
		{
			if (!sysid_valid_system_locale($3)) {
				sprintf(err_string,
					"%s is not a valid system locale",$3);
				yyerror(err_string);
				YYERROR ;
				/* NOTREACHED */
			}
			else {
				create_config_entry( CFG_SYSTEM_LOCALE, $3); 
			}
		}
		|	TERMINAL  '=' IDENT
		{
			if (!sysid_valid_terminal($3)) {
				sprintf(err_string,
					"%s is not a valid terminal type",$3);
				yyerror(err_string);
				YYERROR ;
				/* NOTREACHED */
			}
			else {
				create_config_entry( CFG_TERMINAL, $3); 
			}
		}
		|	TIMEZONE  '=' zone_info
		{
			if (!sysid_valid_timezone($3)) {
				sprintf(err_string,"%s is not a valid timezone",$3);
				yyerror(err_string);
				YYERROR ;
				/* NOTREACHED */
			}
			else {
				create_config_entry( CFG_TIMEZONE, $3);
			}
		}
		|	ROOT_PASSWORD '=' IDENT
		{
			create_config_entry( CFG_ROOT_PASSWORD, $3); 
		}
		|	IDENT
		{
			sprintf(err_string,"%s is not a valid keyword",$1);
			yyerror(err_string);
			YYERROR ;
			/* NOTREACHED */
		}
		;

zone_info
		:	IDENT
		|	IDENT '/' IDENT
		{
			sprintf($$,"%s/%s",$1,$3);
		}
		;

multi_spec
		:	NETWORK_INTERFACE '=' IDENT {
				if (!sysid_valid_network_interface($3)) {
					sprintf(err_string,
						"%s is not a valid network interface",$3);
					yyerror(err_string);
					YYERROR ;
					/* NOTREACHED */
				}
				else {
					create_config_entry(CFG_NETWORK_INTERFACE, $3); 
					strcpy(cur_value, $3);
				}
			} ni_block 
		|	NAME_SERVICE '=' ns_value {
			create_config_entry(CFG_NAME_SERVICE, $3); 
			strcpy(cur_value, $3);
			} ns_block
		|	IDENT '=' wildcard_entry ident_block

wildcard_entry
		:	NUMBER
		|	STRING
		|	IDENT
		|	RESOLUTION_VAL
		;

ni_block
		:	'{' ni_mods '}'
		|	/* EMPTY */
		;

ni_mods
		:	ni_modifier ni_mods
		|	/* EMPTY */
		;

ni_modifier
		:	HOSTNAME '=' IDENT
		{
			if (!sysid_valid_hostname($3)) {
				sprintf(err_string, "%s is not a valid hostname",$3);
				yyerror(err_string);
				YYERROR ;
				/* NOTREACHED */
			}
			else {
				create_config_attribute( CFG_NETWORK_INTERFACE, 
					cur_value, CFG_HOSTNAME, $3); 
			}
		}
		|	IP_ADDRESS '=' ip_addr
		{
			if (!sysid_valid_host_ip_addr($3)) {
				sprintf(err_string,
					"%s is not a valid host ip address",$3);
				yyerror(err_string);
				YYERROR ;
				/* NOTREACHED */
			}
			else {
				create_config_attribute( CFG_NETWORK_INTERFACE, 
					cur_value, CFG_IP_ADDRESS, $3); 
			}
		}
		|	NETMASK '=' ip_addr
		{
			if ( !sysid_valid_ip_netmask($3) ) {
				sprintf(err_string,"%s is not a valid netmask",$3);
				yyerror(err_string);
				YYERROR;
				/* NOTREACHED */
			}
			else {
				create_config_attribute( CFG_NETWORK_INTERFACE, 
					cur_value, CFG_NETMASK, $3); 
			}
		}
		|	IDENT 
		{
			sprintf(err_string,"%s is not a valid keyword",$1);
			yyerror(err_string);
			YYERROR;
			/* NOTREACHED */
		}
		;

ip_addr
		:	IP_ADDR
		|	PARTIAL_IPADDR 
		|	NUMBER 
		;


ns_value
		:	NIS_TOK
		{
			strcpy($$,"NIS");
		}
		|	NISPLUS_TOK
		{
			strcpy($$,"NIS+");
		}
		|	OTHER
		{
			strcpy($$,"OTHER");
		}
		|	NONE
		{
			strcpy($$,"NONE");
		}
		|	IDENT 
		{
			sprintf(err_string,
				"%s is not a valid name service value",$1);
			yyerror(err_string);
			YYERROR;
			/* NOTREACHED */
		}
		;

ns_block
		:	'{' ns_mods '}'
		|	/* EMPTY */
		;

ns_mods
		:	ns_modifier ns_mods
		|	/* EMPTY */
		;

ns_modifier
		:	DOMAIN_NAME '=' IDENT
		{
			if ( ! sysid_valid_domainname($3)) {
				sprintf(err_string,"%s is not a valid domain name",$3);
				yyerror(err_string);
				YYERROR;
				/* NOTREACHED */
			}
			else {
				create_config_attribute( CFG_NAME_SERVICE, 
					cur_value, CFG_DOMAIN_NAME, $3); 
			}
		}
		|	NAME_SERVER '=' IDENT '(' ip_addr ')'
		{
			if (sysid_valid_hostname($3) && sysid_valid_host_ip_addr($5)) {
				create_config_attribute( CFG_NAME_SERVICE, 
					cur_value, CFG_NAME_SERVER_NAME, $3); 
				create_config_attribute( CFG_NAME_SERVICE, 
					cur_value, CFG_NAME_SERVER_ADDR, $5); 
			}
			else {
				if (!sysid_valid_hostname($3)) {
					sprintf(err_string,
						"%s is not a valid hostname",$3);
					yyerror(err_string);
				}
				if (!sysid_valid_host_ip_addr($5)) {
					sprintf(err_string,
						"%s is not a valid id address",$3);
					yyerror(err_string);
				}
				YYERROR;
				/* NOTREACHED */
			}
		}
		|	IDENT 
		{
			sprintf(err_string,"%s is not a valid keyword",$1);
			yyerror(err_string);
			YYERROR;
			/* NOTREACHED */
		}
		;

ident_block
		:	'{' ident_mods '}'
		|	/* EMPTY */
		;

ident_mods
		:	ident_modifier ident_mods
		|	/* EMPTY */
		;

ident_modifier
		:	IDENT '=' wildcard_entry
		|	IDENT 
		{
			sprintf(err_string,"%s is not a valid keyword",$1);
			yyerror(err_string);
			YYERROR;
			/* NOTREACHED */
		}
		;


%%

/*
** Keyword table:
**   keytab: text format of keyword
**   keyval: token value for each keyword
*/

char *keytab[] = { "install_locale", "system_locale", "terminal", 
		"timezone", "root_password", "network_interface",
		"name_service", "hostname",
		"ip_address", "netmask", "domain_name", "name_server", 
		"nis", "nisplus", "none", "other"
} ;

int keyval[] = {
		INSTALL_LOCALE, SYSTEM_LOCALE, TERMINAL,
		TIMEZONE, ROOT_PASSWORD, NETWORK_INTERFACE,
		NAME_SERVICE, HOSTNAME,
		IP_ADDRESS, NETMASK, DOMAIN_NAME, NAME_SERVER, 
		NIS_TOK, NISPLUS_TOK, NONE, OTHER
} ;

#define KEYSIZE (sizeof(keyval)/sizeof(int))

/*
**  File interface for parser
*/

static FILE *input_file;
#define nextch (ch = next_char( input_file ))
static int yylineno=0;
char	yytext[1024];

static char    current_line[1024];
static int	line_len = 0;
static int  cur_pos = -1;    

extern int yydebug;

/*
 * Function:	next_char
 * Description:	Return the next character from the input
 *		file.
 * Scope:	Private
 * Parameters:	f [RO] (File *)
 *		The file pointer to the opened input file
 * Return:	the next character or EOF if all characters have
 *		been read.
 */
static int 
next_char( FILE *f ) {

	if (cur_pos + 1 >= line_len) {
		if (fgets(current_line, 1023, f) == NULL)
			return EOF;

		line_len = strlen(current_line);
		current_line[line_len-1] = '\n';
		current_line[line_len] = '\0';
		cur_pos = 0;
		yylineno++;
		line_len++;
	}
	return current_line[cur_pos++];
}

/*
 * Function:	push_char
 * Description:	Return the current character to the unread
 *		input list.  This is guaranteed to work for
 *		a single character.  If ability to put back
 *		multiple characters is needed then this function
 *		will need to be expanded.
 * Scope:	Private
 * Parameters:	ch [RO] (int)
 *		The character to return to the unread input list.
 * Return:	None
 */
static void 
push_char( int ch) {
	cur_pos--;
}


/*
 * Function:	yyerror
 * Description:	Print a parser error message.  It will print an
 *		error message and then the line number and location
 *		on the line at which the error occurred.
 * Scope:	Private
 * Parameters:	s [RO] (char *)
 *		A character string containing the error message.
 * Return:	None
 */
void
yyerror(char *s)
{
static last_lineno = -1;

	if (last_lineno != yylineno) {
		(void)fprintf(stderr," %s",current_line);
		last_lineno = yylineno;
	}
	(void)fprintf(stderr,"%s", s);
	(void)fprintf(stderr,"  line %d position %d\n", 
			cur_tok_beg_line,cur_tok_beg_pos);
}


/*
 * Function:	yylex
 * Description:	Lexical analyzer for the parser.  Returns tokens
 *		for each lexical element in the input file.
 *		The recognized lexical tokens are:
 *		IDENT		[a-zA-Z_][a-zA-Z0-9_.-]*
 *		keywords	see keytab array definition
 *		NUMBER		[0-9]*
 *		STRING		"IDENT"
 *		IP_ADDR		NUMBER.NUMBER.NUMBER.NUMBER
 *		PARTIAL_IPADDR	NUMBER.[NUMBER[.[NUMBER[.]]]]
 *		RESOLUTION_VAL	NUMBER[xX]NUMBER
 * Scope:	Private
 * Parameters:	None
 * Return:	None
 */
static int yylex( void )
{
	int	ch;
	int string_delimiter;
	int	index;
	int	i;

	/*
	** Skip spaces/tabs/newlines
	**   keep track of line number while skipping
	*/
	
	for( nextch; isspace(ch) || ch == '\r'; nextch )
		;
	/*
	** check for EOF on file. If so, return $EOT token for parser
	*/
		
	cur_tok_beg_pos = cur_pos;  
	cur_tok_beg_line = yylineno;

	if( ch == EOF ) {
		return 0;
	}
	
	/*
	** suck up a comment
	*/

	if ( ch == '#') {
		for (nextch; ch != '\n' && ch != EOF; nextch) ;

		return(yylex());
	}

	/*
	** suck up the line continuation character
	*/
	if (ch == '\\') {
		nextch;
		if (ch == '\n') {
			return(yylex());
		} else {
			push_char(ch);
			return '\\';
		}

	}

	/*
	** parse an IDENT/keyword
	*/
	
	if( isalpha(ch) || ch == '_' ) {
		
		/*
		** get token string
		*/
		
		index=0;
		while( isalpha(ch) || isdigit(ch) || 
			  ch == '_' || ch == '-' || ch == '.') {
			yytext[index++] = (char) ch;
			nextch;
		}
		push_char( ch );
		yytext[index] = 0;
		
		/*
		** check for keyword
		*/
		
		for( i=0; i<KEYSIZE; i++ )
			if( strcasecmp(keytab[i],yytext) == 0 ) {
				/*
				** The following block is to allow the user
				** to specify NIS+ as a keyword value without
				** opening up the IDENT specifier to include them
				*/
				if (keyval[i] == NIS_TOK) {
					nextch ;
					if (ch == '+') {
						yytext[index++] = (char)ch;
						yytext[index] = '\0';
						return(NISPLUS_TOK);
					}
					else {
						push_char(ch);
					}
				}
				return keyval[i];
			}
		
		/*
		** not a keyword, return IDENT value
		*/
			
		strcpy(yylval.str,yytext);
		return IDENT;
	}
	
	/*
	** parse a number
	*/
	
	if( isdigit(ch) ) {
		
		/*
		** get digit stream, accumulating the numeric value
		*/
		
		index=0;
		while( isdigit(ch) ) {
			yytext[index++] = (char) ch;
			nextch;
		}

		/*
		** check to see if it is an ip address
		*/
		if (ch == '.') {
			yytext[index++] = (char) ch;
			nextch;
			if (isdigit(ch)) {
				while( isdigit(ch) ) {
					yytext[index++] = (char) ch;
					nextch;
				}
			}
			else {
				push_char(ch);
				yytext[index] = '\0';
				strcpy(yylval.str,yytext);
				return PARTIAL_IPADDR;
			}

			if (ch == '.') {
				yytext[index++] = (char) ch;
				nextch;
				if (isdigit(ch)) {
					while( isdigit(ch) ) {
						yytext[index++] = (char) ch;
						nextch;
					}
				}
				else {
					push_char(ch);
					yytext[index] = '\0';
					strcpy(yylval.str,yytext);
					return PARTIAL_IPADDR;
				}
			}
			else {
				push_char(ch);
				yytext[index] = '\0';
				strcpy(yylval.str,yytext);
				return PARTIAL_IPADDR;
			}

			if (ch == '.') {
				yytext[index++] = (char) ch;
				nextch;
				if (isdigit(ch)) {
					while( isdigit(ch) ) {
						yytext[index++] = (char) ch;
						nextch;
					}
				}
				else {
					push_char(ch);
					yytext[index] = '\0';
					strcpy(yylval.str,yytext);
					return PARTIAL_IPADDR;
				}
			}
			else {
				push_char(ch);
				yytext[index] = '\0';
				strcpy(yylval.str,yytext);
				return PARTIAL_IPADDR;
			}
			push_char( ch );
			yytext[index] = 0;
			strcpy(yylval.str,yytext);
			return IP_ADDR;
		}


		/*
		** check to see if it is a resolution value
		*/
		if ( ch == 'x' || ch == 'X') {
			yytext[index++] = (char) ch;
			nextch;
			if (isdigit(ch)) {
				while( isdigit(ch) ) {
					yytext[index++] = (char) ch;
					nextch;
				}
			}
			else {
				push_char(ch);
				push_char(ch);
				yytext[--index] = '\0';
				strcpy(yylval.str,yytext);
				return NUMBER;
			}
			push_char( ch );
			yytext[index] = '\0';
			
			/*
			** return the RESOLUTION_VAL
			*/
			
			strcpy(yylval.str,yytext);
			return RESOLUTION_VAL;
			
		}
		push_char( ch );
		yytext[index] = 0;
		
		/*
		** return the NUMBER
		*/
		
		strcpy(yylval.str,yytext);
		return NUMBER;
	}

	/*
	** parse a string
	*/
	
	if( ch == '\'' || ch == '"' ) {
		
		/*
		** build string value
		*/
		
		string_delimiter = ch;
		index = 0;
		nextch;
		while( ch != string_delimiter && ch != '\n' && ch != EOF ) {
			yytext[index++] = (char) ch;
			nextch;
		}
		yytext[index] = 0;
		
		/*
		** if failed to find terminator, return error
		*/
		
		if( ch == '\n' || ch == EOF )
			return -1;
		
		/*
		** return string value
		*/
		
		strcpy(yylval.str,yytext);
		return IDENT;
	}
	

	return ch;
}

#ifdef TEST_PROG

FILE *debugfp;

int
main(int argc, char *argv[])
{

	if (argc <= 1) {
		(void)fprintf(stderr,"Error: No sysidtool configuration file specified\n");
		exit(1);
	}

	if ( (input_file = fopen(argv[1],"r")) == NULL) {
		(void)fprintf(stderr,"Error: Cannot access file %s\n",argv[1]);
		exit(1);
	}

	debugfp = open_log("preconfig");

	yydebug = 1;
	if (yyparse() == 0)
		dump_preconfig();
}
#endif



/*
 * read_config_file()
 * 
 * Attempt to find the sysid configuration file,
 * open it, and read its contents into an internal
 * data structure.  Access is given to that information
 * is provided by the function, get_preconfig_value.
 *
 * Parameters:
 *  none
 *
 * Return:
 *  SUCCESS - the file exists, was readable, and contained at
 *            least one keyword/value pair.
 *  FAILURE - No keyword/value data structure was built
 *
 */
int 
read_config_file(void)
{
	int ret_code;
 
	if ((input_file = fopen(PRECONFIG_FILE, "r")) == NULL) {
		return(FAILURE);
	}

	ret_code = SUCCESS;
	if (yyparse() != 0 || !config_entries_exist())
		ret_code = FAILURE;

	fclose(input_file);
	return(ret_code);
 }

