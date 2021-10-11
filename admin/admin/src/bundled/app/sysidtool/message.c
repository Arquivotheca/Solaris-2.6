/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#pragma	ident	"@(#)message.c 1.17 94/12/12"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include "message.h"

#define	MAXMSGLEN	8192

/*
 * The actual message, composed of a header
 * and one or more arguments.  Both are to
 * be treated as opaque data types by user
 * functions and thus are not published.
 */
struct msg_hdr {
	Sysid_op	msg_op;		/* message operand (type) */
	u_long		msg_len;	/* total message length, in bytes */
	u_long		msg_nargs;	/* number of args */
};

struct msg_arg {
	u_long		arg_size;	/* size of this struct in bytes */
	Sysid_attr	arg_attr;	/* attribute type/name */
	Val_t		arg_type;	/* value type (string, int, bitmap) */
	char		arg_val[4];	/* the value, length actually >= 4 */
};

struct message {
	struct msg_hdr	msg_hdr;
	struct msg_arg	msg_args[1];	/* arguments, actually >= 1 */
};

extern	void	*xmalloc(size_t);
extern	char	*xstrdup(char *);

MSG *
msg_new(void)
{
	struct message *msg;
	MSG	*mp;

	msg = (struct message *)xmalloc(MAXMSGLEN);
	if (msg == NULL)
		return (NULL);
	(void) memset((void *)msg, 0, MAXMSGLEN);

	msg->msg_hdr.msg_len = sizeof (struct msg_hdr);

	mp = (MSG *)xmalloc(sizeof (MSG));
	if (mp == NULL)
		return (NULL);

	mp->m_index = 0;
	mp->m_arg = (void *)msg->msg_args;
	mp->m_data = (void *)msg;

	return (mp);
}

void
msg_delete(MSG *mp)
{
	if (mp != (MSG *)0) {
		if (mp->m_data)
			free(mp->m_data);
		free(mp);
	}
}

int
msg_set_type(MSG *mp, Sysid_op op)
{
	struct message	*msg = (struct message *)mp->m_data;

	msg->msg_hdr.msg_op = op;
	return (0);
}

/*
 * XXX Needs range checking
 */
int
msg_add_arg(MSG *mp, Sysid_attr attr, Val_t type, void *buf, size_t len)
{
	struct message	*msg = (struct message *)mp->m_data;
	struct msg_arg	*arg;

	arg = (struct msg_arg *)mp->m_arg;	/* get current argument */
	arg->arg_attr = attr;
	arg->arg_type = type;
	arg->arg_size = sizeof (struct msg_arg) - sizeof (arg->arg_val);
	switch (type) {
	case VAL_STRING:
		(void) strncpy(arg->arg_val, (char *)buf, len);
		arg->arg_size += roundup(len, sizeof (u_long));
		break;
	case VAL_INTEGER:
	case VAL_BITMAP:
	case VAL_BOOLEAN:
		(void) memcpy((void *)arg->arg_val, buf, len);
		arg->arg_size += roundup(len, sizeof (u_long));	/* XXX */
		break;
	default:
		return (-1);
	}

	msg->msg_hdr.msg_nargs++;
	msg->msg_hdr.msg_len += arg->arg_size;

	mp->m_arg = (void *)((char *)mp->m_arg + arg->arg_size);
	mp->m_index++;

	return (0);
}

Sysid_op
msg_get_type(MSG *mp)
{
	Sysid_op	op;

	if (mp == (MSG *)0)
		op = ERROR;
	else {
		struct message	*msg = (struct message *)mp->m_data;

		if (msg == (struct message *)0)
			op = ERROR;
		else
			op = msg->msg_hdr.msg_op;
	}

	return (op);
}

u_long
msg_get_nargs(MSG *mp)
{
	u_long		nargs;

	if (mp == (MSG *)0)
		nargs = 0L;
	else {
		struct message	*msg = (struct message *)mp->m_data;

		if (msg == (struct message *)0)
			nargs = 0L;
		else
			nargs = msg->msg_hdr.msg_nargs;
	}

	return (nargs);
}

void
msg_reset_args(MSG *mp)
{
	struct message	*msg = (struct message *)mp->m_data;

	mp->m_arg = (void *)msg->msg_args;
	mp->m_index = 0;
}

/*
 * XXX Needs range checking
 */
int
msg_get_arg(MSG *mp, Sysid_attr *attr, Val_t *type, void *buf, size_t len)
{
	struct message	*msg = (struct message *)mp->m_data;
	struct msg_arg	*arg;

	if (msg == (struct message *)0 ||
	    msg->msg_hdr.msg_nargs == 0 ||
	    mp->m_index == msg->msg_hdr.msg_nargs)
		return (EOF);

	arg = (struct msg_arg *)mp->m_arg;	/* get current argument */
	if (attr != (Sysid_attr *)0)
		*attr = arg->arg_attr;
	if (type != (Val_t *)0)
		*type = arg->arg_type;
	switch (arg->arg_type) {
	case VAL_STRING:
		(void) strncpy((char *)buf, arg->arg_val, len);
		break;
	case VAL_INTEGER:
	case VAL_BITMAP:
	case VAL_BOOLEAN:
		(void) memcpy(buf, (void *)arg->arg_val, len);
		break;
	default:
		return (-1);
	}

	mp->m_arg = (void *)((char *)mp->m_arg + arg->arg_size);
	mp->m_index++;

	return (0);
}

int
msg_send(MSG *mp, int to)
{
	struct message	*msg = (struct message *)mp->m_data;
	ssize_t	n;

	n = write(to, mp->m_data, msg->msg_hdr.msg_len);
	if (n != msg->msg_hdr.msg_len) {
		(void) fprintf(debugfp,
			"Failed to write outgoing reply (sent %d)\n", n);
		exit(1);
	}
	return (0);
}

MSG *
msg_receive(int from)
{
	MSG	*mp;
	struct message	*msg;
	ssize_t	want, got;

	mp = msg_new();
	if (mp == NULL)
		return (mp);

	msg = (struct message *)mp->m_data;

	got = read(from, mp->m_data, sizeof (struct msg_hdr));
	if (got == sizeof (struct msg_hdr)) {
		want = msg->msg_hdr.msg_len - sizeof (struct msg_hdr);
		got = read(from, (void *)msg->msg_args, want);
		if (want != got) {
			if (got < 0 && errno == EINTR) {
				(void) msg_delete(mp);
				mp = (MSG *)0;
			}
			(void) fprintf(debugfp,
				"Failed to read msg (got %d)\n", got);
		}
	} else {
		(void) msg_delete(mp);
		mp = (MSG *)0;
	}

	return (mp);
}

#ifdef MSGDEBUG
void
msg_dump(MSG *mp)
{
	struct message	*msg = (struct message *)mp->m_data;
	struct msg_arg	*arg;
	char	*str;
	int	i;

	switch (msg->msg_hdr.msg_op) {
	case ERROR:
		str = "Post Error";
		break;
	case CLEANUP:
		str = "Cleanup";
		break;
	case REPLY_OK:
		str = "Reply OK";
		break;
	case REPLY_ERROR:
		str = "Reply Error";
		break;
	default:
		str = "Unknown type";
		break;
	}

	(void) fprintf(debugfp, "Message:\n");
	(void) fprintf(debugfp, "\tType: %s\n", str);
	(void) fprintf(debugfp, "\tLength: %d\n", msg->msg_hdr.msg_len);
	(void) fprintf(debugfp, "\tNargs: %d\n", msg->msg_hdr.msg_nargs);
	arg = msg->msg_args;
	for (i = 0; i < msg->msg_hdr.msg_nargs; i++) {
		(void) fprintf(debugfp, "\tArg[%d]: %s (%d bytes)\n",
			i, arg->arg_val, arg->arg_size);
		arg = (struct msg_arg *)((char *)arg + arg->arg_size);
	}
}
#endif /* MSGDEBUG */


char **
msg_get_array(MSG *mp, Sysid_attr attr, int *nitems)
{
	char	buf[MAX_FIELDVALLEN];
	char	**ptrs;
	Sysid_attr	arg_attr;
	Val_t	arg_val;
	int	nptrs;
	int	i;

	(void) msg_get_arg(mp, &arg_attr, &arg_val,
				(void *)&nptrs, sizeof (int));
	if (arg_attr != ATTR_SIZE && arg_val != VAL_INTEGER)
		return ((char **)0);
	ptrs = (char **)xmalloc((nptrs) * sizeof (char *));
	if (ptrs != (char **)0) {
		for (i = 0; i < nptrs; i++) {
			(void) msg_get_arg(mp, &arg_attr, &arg_val,
					(void *)buf, sizeof (buf));
			if (attr != arg_attr || arg_val != VAL_STRING)
				break;
			ptrs[i] = xstrdup(buf);
			if (ptrs[i] == (char *)0)
				break;
		}
		if (i != nptrs) {
			while (--i >= 0)
				free(ptrs[i]);
			free(ptrs);
			ptrs = (char **)0;
		}
	}
	if (nitems != (int *)0)
		*nitems = i;
	return (ptrs);
}

void
msg_free_array(char **ptrs, int nptrs)
{
	int	i;

	for (i = 0; i < nptrs; i++)
		free(ptrs[i]);
	free(ptrs);
}
