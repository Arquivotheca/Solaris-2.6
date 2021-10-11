/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)do_system.c	1.8	95/05/08 SMI"

/*
** do_system(). This subroutine executes the
** popen() syscall to execute a shell comand.
**
** If the shell returns an error during the execution of
** the command this routine will format the output from the 
** command and report it.
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifndef _B_TRUE
#define _B_TRUE		B_TRUE
#define _B_FALSE	B_FALSE
#endif

#include "printer_impl.h"

#define MAXSTRLEN 256

int
do_system(char *syscmd)
{

	char  *workbuf;
	char  msgbuffer[MAXSTRLEN];
	char *full_msg;
	char *c;
	int   msg_len;
	int   reported = _B_FALSE;
        int   status;
	FILE *cmd_results;


	/*
	 * the extra 32 bytes is to cover the additional stuff that
	 * we sprintf into this buffer.  If the sprintf command a
	 * couple of lines down changes, be sure to kick up the 32
	 * if necessary!
	 */

	if ((workbuf = (char *)malloc(strlen(syscmd) + 32)) == NULL) {
		return (PRINTER_ERR_MALLOC_FAILED);
	}

	(void) sprintf(workbuf, "(set -f ; %s) 2>&1", syscmd);
	if ((cmd_results = popen(workbuf, "r")) == NULL) { 
		free(workbuf);
		return (PRINTER_ERR_PIPE_FAILED);
	}

	free(workbuf);

	while (fgets(msgbuffer, sizeof(msgbuffer), cmd_results) != NULL) {
		if (!reported) {
			reported = _B_TRUE;
			/*
			 * Add leading CR to make output easier to read.
			 */
			full_msg = strdup("\n");
		}
		c = msgbuffer;
		while (*c == ' ') c++;	/* Strip off leading spaces */
		msg_len = strlen(full_msg);
		full_msg = (char *)realloc(full_msg,
					   msg_len +
					   strlen(c) + 5);
		strcat(full_msg, c);
	}

	status = pclose(cmd_results);

	/*
	**  Only report the error if status was not successful.  Note
	**  that status contains the exit status of the shell command.
	*/
	if (status != 0) {
	    if (status == -1) {
                /*
		**  errno is only set if -1 is returned otherwise
		**  the exit status of the shell command is returned.
		*/
/*
	        adm_err_setf(ADM_NOCATS,
			     PRT_CODE_AND_MSG(PRT_ERR_SYSTEM_CMD_FAILED),
			     method, syscmd, strerror(errno));
*/
	    }

	    if (reported) {
/*
	        adm_err_setf(ADM_NOCATS,
			     PRT_CODE_AND_MSG(PRT_ERR_SYSTEM_CMD_FAILED),
			     method, syscmd, full_msg);
*/
		free(full_msg);
	    }

            return (PRINTER_ERR_SYSTEM_CMD_FAILED);
	} else {
	    return (PRINTER_SUCCESS);
	}

}
