/*
 * $Source: /mit/kerberos/src/lib/krb/RCS/k_gethostname.c,v $
 * $Author: jtkohl $
 *
 * Copyright 1987, 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

#ifndef lint
static char rcsid_k_gethostname_c[] =
"$Header: k_gethostname.c,v 4.1 88/12/01 14:04:42 jtkohl Exp $";
#endif /* lint */

#include <kerberos/mit-copyright.h>

#ifndef PC
#ifndef BSD42
#ifndef SYSV
teach me how to k_gethostname for your system here
#endif
#endif
#endif

#ifdef PC
#include <stdio.h>
typedef long in_name;
#include "custom.h"		/* where is this file? */
extern get_custom();
#define LEN 64			/* just a guess */
#endif /* PC */

#ifdef SYSV
#include <string.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#endif

/*
 * Return the local host's name in "name", up to "namelen" characters.
 * "name" will be null-terminated if "namelen" is big enough.
 * The return code is 0 on success, -1 on failure.  (The calling
 * interface is identical to gethostname(2).)
 *
 * Currently defined for BSD 4.2 and PC.  The BSD version just calls
 * gethostname(); the PC code was taken from "kinit.c", and may or may
 * not work.
 */

k_gethostname(name, namelen)
    char *name;
{
#ifdef SYSV
	if (sysinfo(SI_HOSTNAME, name, (long)namelen) < 0) {
		return -1;
	}
	return 0;
#endif

#ifdef BSD42
    return gethostname(name, namelen);
#endif

#ifdef PC
    char buf[LEN];
    char b1, b2, b3, b4;
    register char *ptr;

    get_custom();		/* should check for errors,
				 * return -1 on failure */
    ptr = (char *) &(custom.c_me);
    b1 = *ptr++;
    b2 = *ptr++;
    b3 = *ptr++;
    b4 = *ptr;
    (void) sprintf(buf,"PC address %d.%d.%d.%d",b1,b2,b3,b4);
    if (strlen(buf) > namelen)
        fprintf(stderr, "gethostname: namelen too small; truncating");
    strnpcy(name, buf, namelen);
    return 0;
#endif
}
