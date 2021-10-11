#ifndef MESSAGE_H
#define	MESSAGE_H

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

#pragma	ident	"@(#)message.h 1.3 93/10/14"

#include <sys/types.h>
#include "sysidtool.h"
#include "prompt.h"

#define	MAX_FIELDNAMELEN	MAXPATHLEN
#define	MAX_FIELDVALLEN		MAXPATHLEN

typedef enum value_type {
	VAL_STRING = 0,
	VAL_INTEGER = 1,
	VAL_BITMAP = 2,
	VAL_BOOLEAN = 3
} Val_t;

typedef struct message_handle {
	u_long	m_index;		/* current argument index */
	void	*m_arg;			/* current argument */
	void	*m_data;		/* the message itself: opaque data */
} MSG;

#define	NO_INTEGER_VALUE	-1
#define	NO_STRING_VALUE		NULL
#define	NO_ARRAY_VALUE		NULL

extern MSG	*msg_new(void);
extern void	msg_delete(MSG *);

extern int	msg_set_type(MSG *, Sysid_op);
extern int	msg_add_arg(MSG *, Sysid_attr, Val_t, void *, size_t);

extern Sysid_op	msg_get_type(MSG *);
extern u_long	msg_get_nargs(MSG *);
extern void	msg_reset_args(MSG *);
extern int	msg_get_arg(MSG *, Sysid_attr *, Val_t *, void *, size_t);

extern int	msg_send(MSG *, int);
extern MSG	*msg_receive(int);

extern char	**msg_get_array(MSG *, Sysid_attr, int *);
extern void	msg_free_array(char **, int);

#endif /* !MESSAGE_H */
