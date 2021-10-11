/*  @(#)string_op.h 1.4 93/10/22 SMI  */

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef _string_op_h
#define _string_op_h

/*
 * Miscellaneous string handling utility routines
 * that aren't available from the OS
 */
int modify_line_in_file(const char *filename, const char *lineidentifier,
    const int modtype);
/* modtypes for modify_line_in_file */
#define MLIF_COMMENT 1
#define MLIF_UNCOMMENT 2
char *strrstr(register char *s1, register char *s2);
char *strlwr(char *stringin);
char *strupr(char *stringin);
void itoa(const int inputnumber, char resultstringbuffer[]);
char *strcomment(const char *stringin);
char *struncomment(const char *stringin);
char *ip_addr_2_netnum(const char *ip_addr);

#endif /* !_string_op_h */
