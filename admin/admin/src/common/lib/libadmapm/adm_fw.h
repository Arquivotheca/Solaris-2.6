
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the general definitions used within
 *	the administrative framework.
 *
 *	NOTE: This file defines constants for the maximum length of
 *	      and administrative class name, method name, and domain.
 *	      These may need to be changed to be more realistic.
 *
 *******************************************************************************
 */

#ifndef _adm_fw_h
#define _adm_fw_h

#pragma	ident	"@(#)adm_fw.h	1.16	94/08/23 SMI"

#include <string.h>
#include <sys/types.h>

/*
 *--------------------------------------------------------------------
 * General constants
 *--------------------------------------------------------------------
 */

#define ADM_MAXINTLEN	  10	/* Max. number of digits in an int */
#define ADM_MAXLONGLEN	  10	/* Max. number of digits in a long */

#define ADM_MAXCLASSLEN	  20	/* Max. length of an admin class name */
#define ADM_MAXDOMLEN	  30	/* Max. length of an admin domain name */
#define ADM_MAXMETHLEN	  20	/* Max. length of an admin method name */

#define ADM_MAXTIMELEN	  200	/* Max. length of a localized date & time */

#define ADM_BLANK	 "??"	/* Blank string */
#define adm_isempty(a) (strcmp(a, ADM_BLANK) == 0)

/*
 *--------------------------------------------------------------------
 * Include files from framework components.
 *--------------------------------------------------------------------
 */

#include <rpc/types.h>
#include <rpc/auth.h>
#include "adm_args.h"
#include "adm_reqID.h"
#include "adm_err_msgs.h"
#include "adm_err.h"
#include "adm_amcl.h"
#include "adm_diag.h"
#include "adm_log.h"
#include "adm_amsl.h"
#include "adm_om_proto.h"
#include "adm_sec.h"
#include "adm_local.h"

/*
 *--------------------------------------------------------------------
 * Exported interfaces.
 *--------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	adm_init();
extern	int	adm_msg_path(char **);
extern	int	adm_set_local_dispatch_info(boolean_t);
extern	int	adm_get_local_dispatch_info(boolean_t *);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_fw_h */

