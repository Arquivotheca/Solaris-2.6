
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the prototypes for the object manager routines
 *	Functions are found in adm_find_method.c
 *
 *******************************************************************************
 */

#ifndef _adm_om_proto_h
#define _adm_om_proto_h

#pragma	ident	"@(#)adm_om_proto.h	1.9	93/05/18 SMI"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__

int adm_find_super_class(char *class_name, char super_class_list[], 
	int buf_len );
int adm_find_class (char *class_name, char *c_version, char path[], 
	int buf_len );
int adm_find_object_class(char object_path[], int buf_len);
int adm_find_method(char *class_name, char *c_version, char *method_name, 
        char path[], int buf_len);
int adm_find_domain(char *class_name, char domain_string[], int buf_len);
int adm_find_locale(char locale_path[], int buf_len);

int adm_search_dir(char *path, int buf_len, char *file_name);

#else 

int adm_find_super_class();
int adm_find_class(); 
int adm_find_object_class();
int adm_find_method(); 
int adm_find_domain();
int adm_find_locale();

int adm_search_dir();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* ifndef _adm_om_proto_h */
