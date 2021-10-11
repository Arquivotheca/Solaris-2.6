/*
 * except.h: Exception handler and i18n interface
 *
 * Copyright (c) 1983-1993 Sun Microsystems, Inc.
 *
 */

#ifndef __INC_EXCEPT_H__
#define	__INC_EXCEPT_H__

#ident "@(#)except.h 1.4 93/12/16 SMI"

#include "kdmconfig_msgs.h"

typedef enum {
SILENT_MODE,
INTERACTIVE_MODE
} ExceptionMode;

void ui_error_exit(char *text);
void ui_warning(char * text);
void set_except_mode(ExceptionMode emode);

#endif /* __INC_EXCEPT_H__ */
