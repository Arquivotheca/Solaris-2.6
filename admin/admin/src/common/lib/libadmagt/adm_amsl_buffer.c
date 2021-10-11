/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_buffer.c	1.6	92/02/28 SMI"

/*
 * FILE:  buffer.c
 *
 *	Admin Framework class agent STDIN, STDOUT, STDERR buffer handling
 *	routines.
 */

#include <unistd.h>
#include <malloc.h>
#include <sys/poll.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/*
 * -------------------------------------------------------------------
 *  init_buffs - Allocate buffer control structures for STDIN, STDOUT,
 *	and STDERR unformatted data, and for STDFMT formatted data buffer.
 *	Accepts pointer to amsl request structure and pollfd file descriptor
 *	structure with information about STDIN, STDOUT, STDERR, STDFMT (this
 *	is primarily to support error processing when buffer allocation
 *	fails; that is, the corresponding file descriptor is closed).
 *	Allocates bufctl structures for each pipe unformatted data pipe
 *	end and initializes these structures to empty buffers.
 *	Allocates bufctl structure for the error stack of formatted errors.
 *	Returns zero on success, or Admin Framework error status code if
 *	buffers cannot be allocated, with error message at top of the
 *	error stack anchored in amsl request structure.
 * -------------------------------------------------------------------
 */

int
init_buffs(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct pollfd Pfd[])	/* pollfd structs for STDIN, STDOUT, STDERR */
{
	struct bufctl *bp;	/* Local pointer to buffer control struct */
	size_t buffsize;	/* Local buffer size variable */

	buffsize = sizeof (struct bufctl);

	/* If no unformatted input, close down STDIN pipe */
	if (reqp->inp->unformatted_len == 0) {
		(void) close(Pfd[AMSL_STDIN_WRITE].fd);
		Pfd[AMSL_STDIN_WRITE].fd = -1;
		reqp->inbuff = (struct bufctl *)NULL;
	}
	/* Else have unformatted input, set up buffer control */
	else {
		if ((bp = (struct bufctl *)malloc(buffsize)) == NULL) {
			(void) amsl_err(reqp, ADM_ERR_REQNOBUFFER, AMSL_STDIN_FD);
			(void) close(Pfd[AMSL_STDIN_WRITE].fd);
			Pfd[AMSL_STDIN_WRITE].fd = -1;
		} else {
			bp->size = reqp->inp->unformatted_len;
			bp->startp = reqp->inp->unformattedp;
			bp->left = reqp->inp->unformatted_len;
			bp->currp = reqp->inp->unformattedp;
			reqp->inbuff = bp;
		}
	}

	/* Initialize buffer for unformatted output on STDOUT */
	if ((bp = (struct bufctl *)malloc(buffsize)) == NULL) {
		(void) amsl_err(reqp, ADM_ERR_REQNOBUFFER, AMSL_STDOUT_FD);
		(void) close(Pfd[AMSL_STDOUT_READ].fd);
		Pfd[AMSL_STDOUT_READ].fd = -1;
	} else {
		bp->size = 0;
		bp->startp = (char *)NULL;
		bp->left = 0;
		bp->currp = (char *)NULL;
		reqp->outbuff = bp;
	}

	/* Initialize buffer for unformatted output on STDERR */
	if ((bp = (struct bufctl *)malloc(buffsize)) == NULL) {
		(void) amsl_err(reqp, ADM_ERR_REQNOBUFFER, AMSL_STDERR_FD);
		(void) close(Pfd[AMSL_STDERR_READ].fd);
		Pfd[AMSL_STDERR_READ].fd = -1;
	} else {
		bp->size = 0;
		bp->startp = (char *)NULL;
		bp->left = 0;
		bp->currp = (char *)NULL;
		reqp->errbuff = bp;
	}

	/* Initialize buffer for formatted data on STDFMT */
	if ((bp = (struct bufctl *)malloc(buffsize)) == NULL) {
		(void) amsl_err(reqp, ADM_ERR_REQNOBUFFER, 3);
		(void) close(Pfd[AMSL_STDFMT_READ].fd);
		Pfd[AMSL_STDFMT_READ].fd = -1;
	} else {
		bp->size = 0;
		bp->startp = (char *)NULL;
		bp->left = 0;
		bp->currp = (char *)NULL;
		reqp->fmtbuff = bp;
	}

	/* Return success */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  free_buffs - Free up buffers for STDOUT, STDERR, STDFMT, and free up
 *	buffer control structures for STDIN, STDOUT, STDERR, and STDFMT.
 *	Accepts pointer to amsl request structure.
 *	Note the unformatted input buffer controlled by the STDIN bufctl
 *	structure is released when the input argument io handle is freed,
 *	so we don't free the buffer itself here.
 *	Returns no status.
 * -------------------------------------------------------------------
 */

void
free_buffs(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	struct bufctl *bp;	/* Local ptr to buffer control structure */

	/* Free up buffer control structure for STDIN */
	if ((bp = reqp->inbuff) != (struct bufctl *)NULL) {
		free(bp);
		reqp->inbuff = (struct bufctl *)NULL;
	}

	/* Free up buffer structures for STDOUT */
	if ((bp = reqp->outbuff) != (struct bufctl *)NULL) {
		if (bp->startp != (char *)NULL)
			free(bp->startp);
		free(bp);
		reqp->outbuff = (struct bufctl *)NULL;
	}

	/* Free up buffer structures for STDERR */
	if ((bp = reqp->errbuff) != (struct bufctl *)NULL) {
		if (bp->startp != (char *)NULL)
			free(bp->startp);
		free(bp);
		reqp->errbuff = (struct bufctl *)NULL;
	}

	/* Free up buffer structures for STDFMT */
	if ((bp = reqp->fmtbuff) != (struct bufctl *)NULL) {
		if (bp->startp != (char *)NULL)
			free(bp->startp);
		free(bp);
		reqp->fmtbuff = (struct bufctl *)NULL;
	}

	/* Return without status */
	return;
}

/*
 * -------------------------------------------------------------------
 *  grow_buff - Increment the size of the current buffer.
 *	Accepts pointer to the buffer control structure whose buffer
 *	is to be enlarged.
 *	Returns zero on success, or -1 on failure (no more memory).
 * -------------------------------------------------------------------
 */

int
grow_buff(
	struct bufctl *bp)	/* Pointer to buffer control structure */
{
	char *buffp;		/* Local buffer pointer */
	size_t newsize;		/* New buffer size */

	/* Reallocate the buffer, incrementing by AMSL_BUFF_SIZE */
	newsize = (size_t)(bp->size + AMSL_BUFF_SIZE);
	if (bp->size == 0)
		buffp = (char *)malloc(newsize);
	else
		buffp = (char *)realloc(bp->startp, newsize);
	if (buffp == NULL)
		return (-1);

	/* Reset buffer control structure */
	bp->startp = buffp;
	bp->currp = buffp + (bp->size - bp->left);
	bp->left = bp->left + ((u_int)newsize - bp->size);
	bp->size = (u_int)newsize;

	/* Return success */
	return (0);
}
