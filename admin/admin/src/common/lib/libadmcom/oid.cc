#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)oid.cc	1.40 5/5/91 SMI";
#endif
 
/**************************************************************************
 *  File:	lib/libnetmgt/oid.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.	
 *
 *  SCCSID:     @(#)oid.cc  1.40  91/05/05
 *
 *  Copyright (c) 1989 Sun Microsystems, Inc.  All Rights Reserved.
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
 *  Comments:	SNMP object identifier handling routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

#include <sys/param.h>

/* ----------------------------------------------------------------------
 *  netmgt_oid2string - converts OBJECT IDENTIFIER from 
 *	from array of unsigned long integers to dot notation string.     
 *      Returns a pointer to dot notation string if successful; 
 *	otherwise returns (char *) NULL
 * ----------------------------------------------------------------------
 */
char *
netmgt_oid2string (u_int length, caddr_t value)
                  		// object identifier length 
                   		// object identifier value 
{
  static NetmgtBuffer buf;	// string buffer 
  register char *bufp;		// string buffer pointer 
  register u_long i;		// loop counter 
  register elements;		// number of array elements 
  register dots;		// number of dots 

  NETMGT_PRN (("netmgt_oid2string\n"));

  // get number of array elements 
  elements = length / sizeof (u_long);

  // get number of dots to print 
  dots = elements - 1;

  // initialize string buffer 
  if (!buf.alloc (length, NETMGT_NAMESIZ))
    return (char *) NULL;
  buf.resetPtr ();
  bufp = buf.getBase () + buf.getOffset ();

  // iterate through long integer array printing elements 
  // in dot notation 
  for (i = 0; i < elements; i++)
    {
      (void) sprintf (bufp, "%u", ((u_long *) value)[i]);
      bufp += strlen (bufp);

      if (i < dots) 
	{ 
	  (void) sprintf (bufp, ".");
	  bufp += strlen (bufp);
	}
    }

  return buf.getBase ();
}

/* ----------------------------------------------------------------------
 *  netmgt_string2oid - converts an OBJECT IDENTIFIER from 
 *	a dot notation string to an array of unsigned long integers.
 *      Returns a pointer to array if successful; otherwise returns NULL
 * ----------------------------------------------------------------------
 */
u_long *
netmgt_string2oid (char *string)
{

  NETMGT_PRN (("netmgt_string2oid: OID string == %s\n", string));

  // get number of array elements 
  u_int dots = 0; 			// number of dots in OID string
  register char *cp = string;		// OID string pointer
  while (*cp)
    {
      if (*cp == '.')
	dots++;
      cp++;
    }

  u_int elements = dots + 1;            // # OID elements

  // allocate OID array
  static NetmgtBuffer buf;              // string buffer 
  register char *bufp;		        // string buffer pointer
 
  if (!buf.alloc (sizeof (u_long) * strlen (string), NETMGT_NAMESIZ))
    return (u_long *) NULL;
  buf.resetPtr ();
  bufp = buf.getBase () + buf.getOffset ();

  // convert and copy string to u_long array
  char substring [NETMGT_NAMESIZ]; 	// OID substring
  char *cp1;				// OID substring pointer
  u_long subidentifier ;		// OID subidentifier

  cp = string;
  NETMGT_PRN (("netmgt_string2oid: OID array == "));
  for (int i = 0; i < elements; i++)
    {
      // skip whitespace
      while (*cp && isspace (*cp))
	cp++;

      // copy OID element to substring 
      cp1 = substring;
      while (*cp && *cp != '.')
	{
	  *cp1 = *cp;
	  cp++;
	  cp1++;
	}
      *cp1 = '\0';

      // insert OID element in oid array
      subidentifier = atoi (substring);
      (void) memcpy ((caddr_t) bufp,
		     (caddr_t) & subidentifier,
		     sizeof (u_long));
      NETMGT_PRN (("%lu ", *(u_long *) bufp));
      bufp += sizeof (u_long);

      // skip dot
      cp++;
    }
  NETMGT_PRN (("\n"));

  return (u_long *)buf.getBase ();
}
	  
static int
short match_game(char *ptr1, char *ptr2)
{
  int count = 0;

  while (*ptr1++ == *ptr2++) {
    count++;
  }
  return(count);
}


/* 
 * Routines to build up a "dot notation" character string from an 
 * Object Identifier
 */

/*
netmgt_oid2decodedstring

     This routine is  called  to  convert  an  Object  Identifier
     library construct into a dot notation character string, usu-
     ally for us in a human interface.  The  dot-notation  output
     is the usual form (1.2.3.4.1.2.1.1) with the a MIB name sub-
     situted for the most possible sub-identifiers starting  from
     the  left  (1.3.6.1.2.1.1.1.0  becomes sysDescr.0).  The MIB
     names are found in the oid_table parameter.
*/

char *
netmgt_oid2decodedstring(u_int length, caddr_t value, caddr_t oid_map)
                  		// object identifier length 
                   		// object identifier value 
{
    char *string_buffer;
    static char out_buffer[256];
    short i, oidcount;
    short best_i, best_len;
    char *id, *name;
    struct oid_ptr{
	int nameoff;
	int idoff;
        int idlen;
    } *oid_ptrs;
    char *oid_strings;
    int len;
    
  // build test string ... put dots between each entry but not after last 
  string_buffer = netmgt_oid2string(length, value);
  out_buffer[0] = '\0';

  // if no map, just return current string 
  if (oid_map == NULL) 
	return string_buffer;

  oid_ptrs = (struct oid_ptr *)oid_map;
  oid_strings = (char *)oid_map;

  best_i = -1; best_len = -1;
  /* OK, now find the best fit to this string */
  oidcount = (oid_ptrs[0].idoff) / sizeof(struct oid_ptr);
  for(i=0; i < oidcount; i++) {
	id = oid_strings + oid_ptrs[i].idoff;
	len = oid_ptrs[i].idlen;
	if ((strncmp(id, string_buffer, len) == 0) &&
	    ((string_buffer[len] == '.') || (string_buffer[len] == 0))) {
		if (best_i != -1) 
			strcat(out_buffer, ".");
		name = oid_strings + oid_ptrs[i].nameoff;
		if ((strlen(out_buffer) + strlen(name)) >
						(sizeof(out_buffer) - 2))
			return (string_buffer);
		strcat(out_buffer, name);
		best_len = len;
		best_i = i;
	} else if (best_len >= len) {
		break;
	}
  }

  if (best_i == -1)
	return (string_buffer);

  strcat(out_buffer, string_buffer + best_len);
  return (out_buffer);
}
  
caddr_t
netmgt_get_oid_map()
{
	NetmgtObject object;
	char *database;
	char filename[MAXPATHLEN];
	int fd;
	struct stat buf;
        caddr_t oid_map;

	database = object.getConfig("database");
	if (database == NULL) {
		NETMGT_PRN (("oid: no 'database' entry in config file\n"));
     		 return (caddr_t) NULL; 
	}
	strcpy(filename, database);
	strcat(filename, "/");
	strcat(filename, "oid.dbase");

	fd = open(filename, O_RDONLY, 0);
	if (fd == -1) {
		NETMGT_PRN (("oid: can't open oid database.\n"));
	        return (caddr_t) NULL; 
	}

	if (fstat(fd, &buf)) {
		NETMGT_PRN (("oid: can't stat oid database.\n"));
		close(fd);
	        return (caddr_t) NULL; 
	}

#ifdef _SVR4_
	oid_map = (caddr_t) malloc ((u_int) buf.st_size);
#else
	oid_map = (caddr_t) malloc (buf.st_size);
#endif _SVR4_
	if (oid_map == NULL) {
		NETMGT_PRN (("oid: not enough memory for oid database.\n"));
		close(fd);
	        return (caddr_t) NULL; 
	}

#ifdef _SVR4_
	if (read(fd, oid_map, (u_int) buf.st_size) != (u_int) buf.st_size) {
#else
	if (read(fd, oid_map, buf.st_size) != buf.st_size) {
#endif _SVR4_
		NETMGT_PRN (("oid: failure reading oid database.\n"));
		close(fd);
		free(oid_map);
	        return (caddr_t) NULL; 
	}
	return oid_map;
}


 
	




		
