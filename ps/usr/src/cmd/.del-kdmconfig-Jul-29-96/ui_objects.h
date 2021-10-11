/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 * ui_objects.h: User Interface object interface
 *
 * Description:
 *	This file contains the User Interface object interface
 *	User interface objects are completely encapsulated;
 *	only the routines in ui_objects.c can directly view
 *	and manipulate their internals.
 *
 * The following exported routines are found in this file
 *
 *	extern UIobj		*ui_get_object(const char *);
 *	extern char		*get_object_title(const UIobj *);
 *	extern char		*get_object_label(const UIobj *);
 *	extern char		*get_object_confirm(const UIobj *);
 *	extern char		*get_object_text(const UIobj *);
 *	extern Field_help	*get_object_help(const UIobj *);
 *
 * This file also provides interface definitions for these routines.
 *
 */

#ifndef _INC_UI_OBJECTS_H_
#define	_INC_UI_OBJECTS_H_

#pragma ident "@(#)ui_objects.h 1.2 94/02/17 SMI"

#include "tty_form.h"

typedef	void	*UIobj;		/* opaque object handle */

/*
 * Names of ui-defined objects
 */
#define	INTRO_CAT	"__intro__"
#define	CONFIRM_CAT	"__confirm__"
#define	ERROR_CAT	"__error__"

extern UIobj		*ui_get_object(const char *);

extern char		*get_object_title(const UIobj *);
extern char		*get_object_label(const UIobj *);
extern char		*get_object_confirm(const UIobj *);
extern char		*get_object_text(const UIobj *);
extern Field_help	*get_object_help(const UIobj *); 

#endif /* _INC_UI_OBJECTS_H_ */
