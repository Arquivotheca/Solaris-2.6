#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)config.cc	1.38 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/config.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)config.cc  1.38  91/05/05
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
 *  Comments:	configuration handling routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------
 *  _netmgt_get_config - C callable version of NetmgtObject::getConfig
 * --------------------------------------------------------------------
 */

char *
_netmgt_get_config (char *name)
{
	NetmgtObject obj;

	return obj.getConfig (name);
}

/* ------------------------------------------------------------------
 *  NetmgtObject::getConfig - get product configuration value
 *	returns pointer to value if successful; otherwise NULL
 * ------------------------------------------------------------------
 */
char *
NetmgtObject::getConfig (char *name)
                 		// configuration name 
{
  FILE *file ;			// configuration database stream 
  register char *cp, *cp1 ;	// utility pointers 
  char line [10*NETMGT_NAMESIZ] ;	// configuration database entry 
  char attrib [10*NETMGT_NAMESIZ] ;	// attribute buffer 
  static char value [10*NETMGT_NAMESIZ] ;	// value buffer 

  NETMGT_PRN (("config: NetmgtObject::getConfig: name == %s\n", name)) ;

  if (!name)
    {
      NETMGT_PRN (("config: no configuration name\n"));
      return (char *) NULL;
    }

  // open agent security database 
  file = fopen (NETMGT_CONFIG_FILE, "r");
  if (!file)
    {
      NETMGT_PRN (("config: can't open %s\n", NETMGT_CONFIG_FILE));
      return (char *) NULL;
    }

  // find match for agent name 
next_line:
  while (fgets (line, NETMGT_NAMESIZ, file) != NULL)
    {
      cp = line;

      // skip leading whitespace 
      while (*cp && isspace (*cp))
	cp++;

      // get configuration name 
      cp1 = attrib;
      while (*cp && !isspace (*cp))
	{
	  if (*cp == '#')
	    goto next_line;
	  *cp1 = *cp;
	  cp++;
	  cp1++;
	}
      *cp1 = 0;

      // look for configuration name match 
      NETMGT_PRN (("config: configuration name: %s\n", attrib));
      if (strcmp (name, attrib) != 0)
	continue;

      // skip separating whitespace 
      while (*cp && isspace (*cp))
	cp++;

      // get configuration value 
      cp1 = value;
      while (*cp && *cp != '#' && *cp != '\n')
	{
	  *cp1 = *cp;
	  cp++;
	  cp1++;
	}
      *cp1 = 0;
      NETMGT_PRN (("config: configuration value: %s\n", value));

      (void) fclose (file);
      return value;
    }

  // no match 
  (void) fclose (file);
  return (char *) NULL;
}
