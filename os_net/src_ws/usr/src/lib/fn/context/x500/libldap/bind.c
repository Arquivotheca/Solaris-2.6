/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)bind.c	1.1	96/03/31 SMI"


/*
 *  Copyright (c) 1990 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  bind.c
 */

#ifndef lint
static char copyright[] = "@(#) Copyright (c) 1990 Regents of the University "
			    "of Michigan.\nAll rights reserved.\n";
#endif

#include <stdio.h>
#include <string.h>
#ifdef MACOS
#include <stdlib.h>
#include "macos.h"
#else /* MACOS */
#ifdef DOS
#include "msdos.h"
#ifdef NCSA
#include "externs.h"
#endif /* NCSA */
#else /* DOS */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#endif /* DOS */
#endif /* MACOS */

#include "lber.h"
#include "ldap.h"


/*
 * ldap_bind - bind to the ldap server (and X.500).  The dn and password
 * of the entry to which to bind are supplied, along with the authentication
 * method to use.  The msgid of the bind request is returned on success,
 * -1 if there's trouble.  Note, the kerberos support assumes the user already
 * has a valid tgt for now.  ldap_result() should be called to find out the
 * outcome of the bind request.
 *
 * Example:
 *	ldap_bind(ld, "cn=manager, o=university of michigan, c=us", "secret",
 *	    LDAP_AUTH_SIMPLE)
 */

int
ldap_bind(LDAP *ld, char *dn, char *passwd, int authmethod)
{
	/*
	 * The bind request looks like this:
	 *	BindRequest ::= SEQUENCE {
	 *		version		INTEGER,
	 *		name		DistinguishedName,	 -- who
	 *		authentication	CHOICE {
	 *			simple		[0] OCTET STRING -- passwd
#ifdef KERBEROS
	 *			krbv42ldap	[1] OCTET STRING
	 *			krbv42dsa	[2] OCTET STRING
#endif
	 *		}
	 *	}
	 * all wrapped up in an LDAPMessage sequence.
	 */

	Debug(LDAP_DEBUG_TRACE, "ldap_bind\n", 0, 0, 0);

	switch (authmethod) {
	case LDAP_AUTH_SIMPLE:
		return (ldap_simple_bind(ld, dn, passwd));

#ifdef KERBEROS
	case LDAP_AUTH_KRBV41:
		return (ldap_kerberos_bind1(ld, dn));

	case LDAP_AUTH_KRBV42:
		return (ldap_kerberos_bind2(ld, dn));
#endif

	default:
		ld->ld_errno = LDAP_AUTH_UNKNOWN;
		return (-1);
	}
}

/*
 * ldap_bind_s - bind to the ldap server (and X.500).  The dn and password
 * of the entry to which to bind are supplied, along with the authentication
 * method to use.  This routine just calls whichever bind routine is
 * appropriate and returns the result of the bind (e.g. LDAP_SUCCESS or
 * some other error indication).  Note, the kerberos support assumes the
 * user already has a valid tgt for now.
 *
 * Examples:
 *	ldap_bind_s(ld, "cn=manager, o=university of michigan, c=us",
 *	    "secret", LDAP_AUTH_SIMPLE)
 *	ldap_bind_s(ld, "cn=manager, o=university of michigan, c=us",
 *	    NULL, LDAP_AUTH_KRBV4)
 */
int
ldap_bind_s(LDAP *ld, char *dn, char *passwd, int authmethod)
{
	Debug(LDAP_DEBUG_TRACE, "ldap_bind_s\n", 0, 0, 0);

	switch (authmethod) {
	case LDAP_AUTH_SIMPLE:
		return (ldap_simple_bind_s(ld, dn, passwd));

#ifdef KERBEROS
	case LDAP_AUTH_KRBV4:
		return (ldap_kerberos_bind_s(ld, dn));

	case LDAP_AUTH_KRBV41:
		return (ldap_kerberos_bind1_s(ld, dn));

	case LDAP_AUTH_KRBV42:
		return (ldap_kerberos_bind2_s(ld, dn));
#endif

	default:
		return (ld->ld_errno = LDAP_AUTH_UNKNOWN);
	}
}
