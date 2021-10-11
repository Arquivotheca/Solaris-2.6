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

#pragma	ident	"@(#)ui_reply.c 1.1 93/10/12"

#include <string.h>
#include "sysid_ui.h"

void
reply_error(Sysid_err errcode, char *str, int reply_to)
{
	MSG	*mp;

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_ERROR);
	(void) msg_add_arg(mp, ATTR_ERROR, VAL_INTEGER,
					(void *)&errcode, sizeof (errcode));
	(void) msg_add_arg(mp, ATTR_STRING, VAL_STRING,
					(void *)str, strlen(str) + 1);
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

void
reply_string(Sysid_attr attr, char *str, int reply_to)
{
	MSG	*mp;

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, attr, VAL_STRING,
					(void *)str, strlen(str) + 1);
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

void
reply_integer(Sysid_attr attr, int val, int reply_to)
{
	MSG	*mp;

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, attr, VAL_INTEGER,
					(void *)&val, sizeof (int));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

void
reply_void(int reply_to)
{
	MSG	*mp;

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

void
reply_fields(Field_desc *fields, int nfields, int reply_to)
{
	MSG	*mp;
	Menu	*menu;
	int	i;

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);

	for (i = 0; i < nfields; i++) {
		Field_desc *f = &fields[i];

		if ((f->flags & FF_RDONLY) == 0) {
			switch (f->type) {
			case FIELD_TEXT:
				(void) msg_add_arg(mp,
				    (Sysid_attr)f->user_data, VAL_STRING,
				    f->value, strlen((char *)f->value) + 1);
				break;
			case FIELD_EXCLUSIVE_CHOICE:
				menu = (Menu *)f->value;
				(void) msg_add_arg(mp,
				    (Sysid_attr)f->user_data, VAL_INTEGER,
				    (void *)&menu->selected, sizeof (int));
				break;
			case FIELD_CONFIRM:
				(void) msg_add_arg(mp,
				    (Sysid_attr)f->user_data, VAL_INTEGER,
				    (void *)&f->value, sizeof (int));
				break;
			default:
				break;
			}
		}
	}
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}
