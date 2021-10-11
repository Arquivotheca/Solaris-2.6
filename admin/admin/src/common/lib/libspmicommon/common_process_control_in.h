/*
 * *********************************************************************
 * The Legal Stuff:						       *
 * *********************************************************************
 * Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved. Sun *
 * considers its source code as an unpublished, proprietary trade      *
 * secret, and it is available only under strict license provisions.   *
 * This copyright notice is placed here only to protect Sun in the     *
 * event the source is deemed a published work.	 Dissassembly,	       *
 * decompilation, or other means of reducing the object code to human  *
 * readable form is prohibited by the license agreement under which    *
 * this code is provided to the user or company in possession of this  *
 * copy.							       *
 *								       *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the    *
 * Government is subject to restrictions as set forth in subparagraph  *
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software    *
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and   *
 *  NASA FAR Supplement.					       *
 * *********************************************************************
 */
#ifndef lint
#pragma ident "@(#)common_process_control_in.h 1.4 96/05/23 SMI"
#endif

#ifndef	__PROCESS_CONTROL_IN_H
#define	__PROCESS_CONTROL_IN_H

#include<sys/types.h>
#include<termios.h>
#include<sys/ioctl.h>
#include<limits.h>

#include"spmicommon_api.h"

/*
 * Define the structure to hold the data for the process being
 * controlled.
 */

#define	PROCESS_INITIALIZED 0xDEADBEEF
typedef struct {
	unsigned int	Initialized;
	char		Image[PATH_MAX];
	char		**argv;
	TPCState	State;
	pid_t		PID;
	TPCFD		FD;
	TPCFILE		FILE;
} TPCB;

/*
 * *********************************************************************
 * Function Name: PCValidateHandle				       *
 *								       *
 * Description:							       *
 *   This function takes in a process handle and determines if it is   *
 *   valid.  Upon Success, the function returns Zero and on failure    *
 *   returns non-zero.						       *
 *								       *
 * Return:							       *
 *  Type			     Description		       *
 *  TPCError			     Upon successful completion the    *
 *				     PCSuccess flag is returned.  Upon *
 *				     failure the appropriate error     *
 *				     code is returned.		       *
 * Parameters:							       *
 *  Type			     Description		       *
 *  TPCHandle			     The handle that is to be	       *
 *				     validated.			       *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

static	TPCError
PCValidateHandle(TPCHandle Handle);

#endif
