
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *	adm_method.h
 *
 *	This file contains definitions methods will need
 */

#ifndef _adm_method_h
#define _adm_method_h

#pragma	ident	"@(#)adm_method.h	1.3	93/05/18 SMI"

#define OM_SUCCESS	0
#define OM_ERROR	-1
#define OM_ICON		".icon"

#define LIST_METHODS	"List Methods" 
#define FIND_SUPER_CLASS"Find Super Class"
#define FIND_CLASS	"Find Class"
#define FIND_METHOD	"Find method"
#define LIST_CLASS	"List class"
#define HIERARCHY	"hierarchy"

#define ADM_MS_MISSARG	250
#define ADM_MS_NOCLASS	251
#define ADM_MS_NOOBJ	252
#define ADM_MS_OPENDIR	253
#define ADM_MS_CLOSEDIR	254


#define ADM_M_FINDCLASS		"Method Findclass"
#define ADM_M_FINDCLASSICON	"Method Findclassicon"
#define ADM_M_LISTCLASSPATH	"Method Listclasspath"

/* 
 * Method error messages
 */

#define ADM_MT_MISSARG	"%s had missing Argument %s\n"
#define ADM_MT_NOCLASS	"%s did not find class %s\n"
#define ADM_MT_NOOBJ	"%s could not find object dir in path %s\n"
#define ADM_MT_OPENDIR	"%s could not open directory %s\n"
#define	ADM_MT_CLOSEDIR	"%s could not close directory %s\n"

#endif /* !_adm_method_h */
