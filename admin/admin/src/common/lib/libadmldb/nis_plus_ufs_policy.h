/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#pragma ident "@(#)nis_plus_ufs_policy.h	1.4 95/03/02 Sun Microsystems"

/* define the operations for creating a NIS+ or UFS policy table */


#ifndef _NIS_PLUS_UFS_POLICY_H
#define _NIS_PLUS_UFS_POLICY_H

#define POLICY_FILE_NAME "/opt/SUNWadm/2.1/etc/policy.defaults"

int
create_new_policy_table(
    const char* domain,
    Db_error**  db_err
);

void
install_fallback_file(
    const char* db_file_name
);		     

#endif _NIS_PLUS_UFS_POLICY_H
