/**************************************************************************
 *  File:	netmgt_db.h
 *
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary trade 
 *  secret, and it is available only under strict license provisions.  
 *  This copyright notice is placed here only to protect Sun in the event
 *  the source is deemed a published work.  Dissassembly, decompilation, 
 *  or other means of reducing the object code to human readable form is 
 *  prohibited by the license agreement under which this code is provided
 *  to the user or company in possession of this copy.
 * 
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
 *  Government is subject to restrictions as set forth in subparagraph 
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software 
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
 *  NASA FAR Supplement.
 *
 *  SCCSID:	@(#)netmgt_db.h 1.3 91/04/04 
 *
 *  Comments:	SunNet Manager Runtime Database definitions
 *
 **************************************************************************
 */


#define SNMBUFFERSIZE	8192

typedef struct {
    unsigned char data[SNMBUFFERSIZE];
} snmdb_buffer;

typedef union {
    int db_int;			/* Type = int, enum */
    unsigned int db_uint;	/* Type = unsigned int, counter, gauge */
    long db_long;		/* Type = long */
    unsigned long db_ulong;	/* Type = unsigned long, unixtime, timeticks */
    float db_float;		/* Type = float */
    double db_double;		/* Type = double */
    char *db_string;		/* Type = string, octet */
    short db_short;		/* Type = short */
    unsigned short db_ushort;	/* Type = unsigned short */
    unsigned char db_octets[4];	/* Type = netaddress, ipaddress */
}snmdb_data;

typedef enum {
	undefined,
	short_integer,
	unsigned_short_integer,
	integer,
	unsigned_integer,
	long_integer,
	unsigned_long_integer,
	floating_point,
	double_floating_point,
	octet,
	string,
	counter,
	gauge,
	timeticks,
	netaddress,
	ipaddress,
	unixtime,
	enumeration
} snmdb_type;

typedef char *snmdb_handle;

/* Errors */

extern int snm_error;

#define SNMDB_BAD_ARGUMENT			1
#define SNMDB_NOT_INITIALIZED			2
#define SNMDB_UNKNOWN_AGENT			3
#define SNMDB_NO_MEMORY				4
#define SNMDB_INVALID_TYPE			5
#define SNMDB_TOO_LONG				6
#define SNMDB_ELEMENT_NOT_FOUND			7
#define SNMDB_NO_KEY_FIELD			8
#define SNMDB_DUPLICATE_ID			9
#define SNMDB_DUPLICATE_NAME			10
#define SNMDB_CONNECTED_TO_ELEMENT_NOT_FOUND	11
#define SNMDB_CONNECTION_EXISTS		        12
#define SNMDB_CONNECTION_NOT_FOUND		13
#define SNMDB_CANNOT_CONNECT_TO_SELF		14
#define SNMDB_ELEMENT_ALREADY_IN_VIEW		15
#define SNMDB_ELEMENT_NOT_IN_VIEW		16
#define SNMDB_SUBVIEW_IS_NOT_EMPTY		17
#define SNMDB_UNABLE_TO_DELETE_ELEMENT		18
#define SNMDB_UNKNOWN_PROPERTY		        19
#define SNMDB_INVALID_DATA_IN_BUFFER	        20
#define SNMDB_NOT_CONNECTED_TO_ANY              21
#define SNMDB_NOT_EXISTS_IN_ANY_VIEW	        22
#define SNMDB_NO_AGENT_APPLY     	        23
#define SNMDB_AGENT_NOT_FOUND     	        24

snmdb_data snmdb_get_property();
snmdb_handle *snmdb_enumerate_elements();
char *snmdb_get_next_element();
char **snmdb_enumerate_agents();
char **snmdb_enumerate_views();
char **snmdb_enumerate_connections();
