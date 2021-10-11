
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the definitions of global, initialized
 *	variables used by the log handling rouutines.  It includes
 *	the definitions of:
 *
 *	    adm_first_logIDp	Pointer to first log information block
 *				in linked list of blocks used to keep
 *				track of all active logs.
 *
 ****************************************************************************
 */

#ifndef _adm_log_glob_c
#define _adm_log_glob_c

#pragma	ident	"@(#)adm_log_glob.c	1.3	92/01/28 SMI"

#include "adm_fw.h"
#include "adm_fw_impl.h"

Adm_logID *adm_first_logIDp = NULL;	/* Ptr. to first log info block */

#endif /* !_adm_log_glob_c */

