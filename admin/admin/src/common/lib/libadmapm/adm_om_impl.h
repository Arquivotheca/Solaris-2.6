
/*
 *  Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 *  All rights reserved.
 */

/*
 *	adm_om_impl.h 
 *	This file contains the exported definitions from the Object Manager
 *	These are used in adm_find_method.c
 *
 *	Nov 20, 1990
 *	Jan 12, 1995	Ken Jones
 */

#ifndef _adm_om_impl_h
#define _adm_om_impl_h	

#pragma	ident	"@(#)adm_om_impl.h	1.16	95/01/13 SMI"

#define	FOUND 		0
#define NOTFOUND	1	
#define	ERROR		-1	
#define OBJ		"classes"	/* This will change to class */
#define OBJECT		"object"	/* Used to see if class=root class */
#define SEPARATOR	'/'	/* Separates components of path name 	*/
#define	SLASH		"/"	/* Used in to build pathnames		*/
#define	EXTENSION	"."	/* Used in to build version names	*/
#define BUFSIZE		250	/* Size of entry in .class_name file 	*/
#define MAXSUPERCLASS	250	/* Size of entry in .class_name file 	*/
#define MAXPATHLIST     4096	/* Buffer to hold list of search paths  */
#define MAXOBJPATHS	100
#define READONLY	"r"	/* Used in fopen			*/
#define SUPERCLASSFILE	".superclass"	/* used for inheiritance */
#define CLASS_2_DOMAIN	".textdomains"	/* Used for i18n */
#define PATHENVV	"ADMINPATH"
#define LOCALE		"locale"

#define LOCATION     	"location"
#define LISTCLASS	"List Class"
#define FINDCLASS	"Find Class"
#define FINDMETHOD	"Find Method"
#define FINDLOCALE	"Find Locale"
#define FINDDOMAIN	"Find Domain"
#define GETUOID		"Get UOID"
#define FINDSUPERCLASS	"Find Super Class"
#define LISTMETHODS	"List Methods" 
#define FINDOBJECT	"Find Object Class"
#define SEARCHDIR	"Search Directory"
#define FINDFIRST	"Find First"
#define FINDBEST	"Find Best"
#define PPREFIX		"Prefix"
#define CLASS		"Class"
#define METHOD		"Method"

#define	OM_CACHE_SIZE	25
#define	OM_PATH_TTL	1000
#define OM_SUPER_TTL	1000
#define OM_ACL_TTL	500
#define OM_DOMAIN_TTL	1000

#endif/* ! _adm_om_impl_h */

