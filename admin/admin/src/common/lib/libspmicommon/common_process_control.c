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
#pragma ident "@(#)common_process_control.c 1.2 96/05/15 SMI"
#endif

#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<stropts.h>
#include<unistd.h>
#include<stdarg.h>
#include<signal.h>
#include<malloc.h>
#include<stdlib.h>

#include"common_process_control_in.h"

/*
 * *********************************************************************
 * Function Name: PCValidateHandle                                     *
 *								       *
 * Description:							       *
 *   This function takes in a process handle and determines if it is   *
 *   valid.  Upon Success, the function returns Zero and on failure    *
 *   returns non-zero.                                                 *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle that is to be          *
 *                                   validated.                        *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

static TPCError
PCValidateHandle(TPCHandle Handle)
{
	TPCB *PCB;
	if (Handle == NULL) {
		return (PCInvalidHandle);
	}
	PCB = (TPCB *) Handle;

	if (PCB->Initialized != PROCESS_INITIALIZED) {
		return (PCInvalidHandle);
	}
	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCCreate                                             *
 *								       *
 * Description:							       *
 *   This function taskes in the parameters for the process to be      *
 *   managed.  This function ONLY takes in the parameters to be passed *
 *   to the child and stores them in the proces block.  It does not    *
 *   start the child process.                                          *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle *                      A Pointer to a process control    *
 *                                   handle.  This handle will be      *
 *                                   associated with the new process   *
 *                                   control block being created.      *
 *  char *                           The name of the image to be       *
 *                                   exec'd in the PCStart() function. *
 *  char *                           The first argument to the child   *
 *                                   process.                          *
 *  ... (char *)                     The following arguments to the    *
 *                                   child process.  The last argument *
 *                                   MUST be a NULL to signify the end *
 *                                   of the argument list.             *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCCreate(TPCHandle * Handle, char *Image, char *argv0, ...)
{
	int 		i;
	va_list		ap;
	TPCB		*PCB;
	char		*TmpStr;
	TBoolean	Done;


	/*
	 * Get some space for the new Process Control Block (PCB)
	 */

	if (!(PCB = (TPCB *) calloc(1, sizeof (TPCB)))) {
		return (PCMemoryAllocationFailure);
	}

	/*
	 * Initialize the PCB.
	 */

	PCB->Initialized = PROCESS_INITIALIZED;
	PCB->State = PCInitialized;
	(void) strcpy(PCB->Image, Image);

	/*
	 * Allocate space for the argv pointer.
	 */

	if (!(PCB->argv = (char **) malloc(sizeof (char *)))) {
		return (PCMemoryAllocationFailure);
	}

	/*
	 * Allocate out space for the first argv entry
	 */

	if (!(PCB->argv[0] = (char *) malloc(strlen(argv0)))) {
		return (PCMemoryAllocationFailure);
	}

	/*
	 * Copy in the argv[0] entry
	 */

	(void) strcpy(PCB->argv[0], argv0);


	/*
	 * Walk the remaining variable argument list allocating memory for
	 * each new entry and assigning the argument into the argv list.
	 */

	i = 1;
	va_start(ap, argv0);
	Done = False;
	while (!Done) {
		if (!(PCB->argv = (char **) realloc(PCB->argv,
			    sizeof (char *) * (i + 1)))) {
			return (PCMemoryAllocationFailure);
		}
		TmpStr = va_arg(ap, char *);

		if (TmpStr == NULL) {
			PCB->argv[i] = NULL;
			Done = True;
			break;
		}
		if (!(PCB->argv[i] = (char *) malloc(strlen(TmpStr)))) {
			return (PCMemoryAllocationFailure);
		}
		(void) strcpy(PCB->argv[i], TmpStr);
		i++;
	}
	va_end(ap);

	/*
	 * Set the handle to point to the new PCB.
	 */

	*Handle = PCB;
	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCStart                                              *
 *								       *
 * Description:							       *
 *  This function is called to start the process associated with the   *
 *  given process control handle.  The call MUST be preceeded by a     *
 *  call to PCCreate().                                                *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle returned by PCCreate().*
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCStart(TPCHandle Handle)
{
	int		StdIn[2];
	int		StdOut[2];
	int		StdErr[2];
	char		PTYSlave[20];

	TPCB		*PCB;
	TPCError	PCError;

	/*
	 * Validate the given handle
	 */

	if ((PCError = PCValidateHandle(Handle))) {
		return (PCError);
	}
	PCB = (TPCB *) Handle;

	/*
	 * Create the pipe's to the child for STDIN, STDOUT, STDERR
	 */

	if (pipe(StdIn)) {
		return (PCSystemCallFailed);
	}
	if (pipe(StdOut)) {
		return (PCSystemCallFailed);
	}
	if (pipe(StdErr)) {
		return (PCSystemCallFailed);
	}

	/*
	 * If this is the child process.
	 */

	if ((PCB->PID = CMNPTYFork(&PCB->FD.PTYMaster,
		    PTYSlave,
		    NULL,
		    NULL)) == 0) {

		/*
		 * Assign STDIN to the parent's pipe.
		 */

		if (dup2(StdIn[0], STDIN_FILENO) == -1) {

			/*
			 * Since this is the child the best that I can do is
			 * exit with a non-zero code to signify that an error
			 * occured.
			 */

			exit(1);
		}
		(void) close(StdIn[0]);
		(void) close(StdIn[1]);

		/*
		 * Assign STDOUT to the parent's pipe.
		 */

		if (dup2(StdOut[1], STDOUT_FILENO) == -1) {

			/*
			 * Since this is the child the best that I can do is
			 * exit with a non-zero code to signify that an error
			 * occured.
			 */

			exit(1);
		}
		(void) close(StdOut[0]);
		(void) close(StdOut[1]);

		/*
		 * Assign STDERR to the parent's pipe.
		 */

		if (dup2(StdErr[1], STDERR_FILENO) == -1) {

			/*
			 * Since this is the child the best that I can do is
			 * exit with a non-zero code to signify that an error
			 * occured.
			 */

			exit(1);
		}
		(void) close(StdErr[0]);
		(void) close(StdErr[1]);

		/*
		 * Finally, lets start up that specified process.
		 */

		if (execvp(PCB->Image,
			PCB->argv) == -1) {

			/*
			 * Finally, lets start up that specified process.
			 */

			exit(1);
		}
	} else {

		/*
		 * Close the ends of the pipe that the parent will not be
		 * using
		 */

		(void) close(StdIn[0]);
		(void) close(StdOut[1]);
		(void) close(StdErr[1]);

		/*
		 * Set the pipes to the child into the PCB
		 */

		PCB->FD.StdIn = StdIn[1];
		PCB->FD.StdOut = StdOut[0];
		PCB->FD.StdErr = StdErr[0];

		/*
		 * Open the FILE version of all of the pipes.
		 */

		if (!(PCB->FILE.StdIn =
			fdopen(PCB->FD.StdIn, "w"))) {
			return (PCSystemCallFailed);
		}
		if (!(PCB->FILE.StdOut =
			fdopen(PCB->FD.StdOut, "r"))) {
			return (PCSystemCallFailed);
		}
		if (!(PCB->FILE.StdErr =
			fdopen(PCB->FD.StdErr, "r"))) {
			return (PCSystemCallFailed);
		}
		if (!(PCB->FILE.PTYMaster =
			fdopen(PCB->FD.PTYMaster, "rw"))) {
			return (PCSystemCallFailed);
		}

		/*
		 * Set the state of the child process to running
		 */

		PCB->State = PCRunning;
	}
	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCWait()                                             *
 *                                                                     *
 * Description:                                                        *
 *  This function waits for the child process to exit.  Upon exit,     *
 *  the child's exit status and exit signal (if any) are retrieved     *
 *  and set.                                                           *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle returned by PCCreate().*
 *  int *                            If non NULL then the exit status  *
 *                                   of the child is returned.         *
 *                                   If an exit status is not present  *
 *                                   then -1 is returned.              *
 *  int *                            If non NULL then the exit signal  *
 *                                   (if any) of the child is returned.*
 *                                   If an exit signal is not present  *
 *                                   then -1 is returned.              *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCWait(TPCHandle Handle, int *ExitStatus, int *ExitSignal)
{
	int 		Status;
	TPCB		*PCB;
	TPCError	PCError;

	/*
	 * Validate the given handle
	 */

	if ((PCError = PCValidateHandle(Handle))) {
		return (PCError);
	}
	PCB = (TPCB *) Handle;

	/*
	 * Wait for the child.
	 */

	if (waitpid(PCB->PID, &Status, 0) < 0) {
		return (PCSystemCallFailed);
	}
	PCB->State = PCExited;

	/*
	 * Close the pipes and FILE pointers to the child process.
	 */

	(void) close(PCB->FD.StdIn);
	(void) close(PCB->FD.StdOut);
	(void) close(PCB->FD.StdErr);
	(void) close(PCB->FD.PTYMaster);

	(void) fclose(PCB->FILE.StdIn);
	(void) fclose(PCB->FILE.StdOut);
	(void) fclose(PCB->FILE.StdErr);
	(void) fclose(PCB->FILE.PTYMaster);

	/*
	 * Check for an exit status and signal
	 */

	if (WIFEXITED(Status)) {
		if (ExitStatus != NULL) {
			*ExitStatus = (WEXITSTATUS(Status));
		}
		if (ExitSignal != NULL) {
			*ExitSignal = -1;
		}
	} else if (WIFSIGNALED(Status)) {
		if (ExitStatus != NULL) {
			*ExitStatus = -1;
		}
		if (ExitSignal != NULL) {
			*ExitSignal = WTERMSIG(Status);
		}
	} else if (WIFSTOPPED(Status)) {
		if (ExitStatus != NULL) {
			*ExitStatus = -1;
		}
		if (ExitSignal != NULL) {
			*ExitSignal = WSTOPSIG(Status);
		}
	}
	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCGetPID                                             *
 *								       *
 * Description:							       *
 *  This function returnes the PID of the child process if running.    *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle returned by PCCreate().*
 *  pid_t                            The PID of the child process.     *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCGetPID(TPCHandle Handle, pid_t * PID)
{
	TPCB 		*PCB;
	TPCError	PCError;

	/*
	 * Validate the given handle
	 */

	if ((PCError = PCValidateHandle(Handle))) {
		return (PCError);
	}
	PCB = (TPCB *) Handle;

	/*
	 * Make sure that the process is running
	 */

	if (PCB->State != PCRunning) {
		return (PCProcessNotRunning);
	}
	*PID = PCB->PID;

	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCGetFD                                              *
 *								       *
 * Description:							       *
 *  This function returns a pointer to a structure containing the      *
 *  file descriptors for STDIN, STDOUT, STDERR and the Pseudo Terminal *
 *  Master side.                                                       *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle returned by PCCreate().*
 *  TPCFD *                          Pointer to the process control    *
 *                                   file descriptor structure.        *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCGetFD(TPCHandle Handle, TPCFD * FD)
{
	TPCB 		*PCB;
	TPCError	PCError;

	/*
	 * Validate the given handle
	 */

	if ((PCError = PCValidateHandle(Handle))) {
		return (PCError);
	}
	PCB = (TPCB *) Handle;

	/*
	 * Make sure that the process is running
	 */

	if (PCB->State != PCRunning) {
		return (PCProcessNotRunning);
	}
	(void) memcpy(FD,
	    &PCB->FD,
	    sizeof (TPCFD));

	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCGetFILE                                            *
 *								       *
 * Description:							       *
 *  This function returns a pointer to a structure containing the      *
 *  FILE pointers for STDIN, STDOUT, STDERR and the Pseudo Terminal    *
 *  Master side.                                                       *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle returned by PCCreate().*
 *  TPCFILE *                        Pointer to the process control    *
 *                                   FILE pointer structure.           *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCGetFILE(TPCHandle Handle, TPCFILE * FILE)
{
	TPCB		*PCB;
	TPCError	PCError;

	/*
	 * Validate the given handle
	 */

	if ((PCError = PCValidateHandle(Handle))) {
		return (PCError);
	}
	PCB = (TPCB *) Handle;

	/*
	 * Make sure that the process is running
	 */

	if (PCB->State != PCRunning) {
		return (PCProcessNotRunning);
	}
	(void) memcpy(FILE,
	    &PCB->FILE,
	    sizeof (TPCFILE));

	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCKill                                               *
 *								       *
 * Description:							       *
 *  Sends the specified signal to the child process.                   *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle returned by PCCreate().*
 *  int                              The signal to send to the child   *
 *                                   process.                          *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCKill(TPCHandle Handle, int Signal)
{
	TPCB		*PCB;
	TPCError	PCError;

	/*
	 * Validate the given handle
	 */

	if ((PCError = PCValidateHandle(Handle))) {
		return (PCError);
	}
	PCB = (TPCB *) Handle;

	/*
	 * Make sure that the process is running
	 */

	if (PCB->State != PCRunning) {
		return (PCProcessNotRunning);
	}
	if (kill(PCB->PID, Signal)) {
		return (PCSystemCallFailed);
	}
	return (PCSuccess);
}

/*
 * *********************************************************************
 * Function Name: PCDestroy                                            *
 *								       *
 * Description:							       *
 *  Destroys the process control block associated with the give handle.*
 *  If the child process is running, then an error is returned.        *
 *								       *
 * Return:							       *
 *  Type                             Description                       *
 *  TPCError                         Upon successful completion the    *
 *                                   PCSuccess flag is returned.  Upon *
 *                                   failure the appropriate error     *
 *                                   code is returned.                 *
 * Parameters:							       *
 *  Type                             Description                       *
 *  TPCHandle                        The handle returned by PCCreate().*
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

TPCError
PCDestroy(TPCHandle * Handle)
{
	TPCB		*PCB;
	int		i;
	TPCError	PCError;

	/*
	 * Validate the given handle
	 */

	if ((PCError = PCValidateHandle(*Handle))) {
		return (PCError);
	}
	PCB = (TPCB *) * Handle;

	/*
	 * If the child process is running then return an error
	 */

	if (PCB->State == PCRunning) {
		return (PCProcessRunning);
	}

	/*
	 * Free up the space associated with the PCB
	 */

	for (i = 0; PCB->argv[i] != NULL; i++) {
		free(PCB->argv[i]);
	}

	free(PCB->argv);
	free(PCB);

	/*
	 * Invalidate the handle.
	 */

	*Handle = NULL;
	return (PCSuccess);
}
