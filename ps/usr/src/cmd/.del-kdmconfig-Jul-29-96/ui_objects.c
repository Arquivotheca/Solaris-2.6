/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 *
 * ui_objects.c: User interface object internals
 *
 * Description:
 *  This file contains the public interface to user interface objects
 *  as well as the code required to initialize these objects.
 *
 * The following exported routines are found in this file
 *
 *  char *obj_get_title(char *)
 *  char *obj_get_label(char *)
 *  char *obj_get_text(char *)
 *  Field_help *obj_get_help(char *)
 *
 */

#pragma ident "@(#)ui_objects.c 1.9 95/05/24 SMI"

#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include "windvc.h"
#include "ui.h"
#include "ui_objects.h"
#include "kdmconfig_msgs.h"

/*
 * Every entity the UI knows about has a
 * corresponding object, with an entry in
 * an object table.  Each object has a
 * reference to its name, title, label,
 * descriptive text and on-line help.
 */
#define	OBJ_DISPLAY		0
#define	OBJ_POINTER		1
#define	OBJ_KEYBOARD	2
#define	OBJ_MONITOR		3
#define	OBJ_SCREEN		4
#define	OBJ_BUTTONS		5
#define	OBJ_LAYOUT		6
#define	OBJ_DEVICE		7
#define	OBJ_IRQ			8
#define	OBJ_IOADDR		9
#define	OBJ_POSITION		10
#define	OBJ_RESOLUTION		11
#define	OBJ_DESKTOP		12
#define	OBJ_CONFIRM		13
#define	OBJ_INTRO		14
#define	OBJ_ERROR		15
#define	OBJ_DEPTH		16
#define	OBJ_BMEMADDR		17
#define	OBJ_BUSTYPE		18

typedef struct ui_object_private UIobjP;
struct ui_object_private {
	char		*name;		/* internal name of object */
	char		*title;		/* title for screen */
	char		*label;		/* label for input field */
	char		*confirm;	/* label for output (confirm) field */
	char		*text;		/* on-screen "help" text */
	Field_help	*help;		/* on-line help pointer */
};

static UIobjP objects[] = {
	{  DISPLAY_CAT,		NULL, NULL, NULL, NULL	},
	{  POINTER_CAT,		NULL, NULL, NULL, NULL	},
	{  KEYBOARD_CAT,	NULL, NULL, NULL, NULL	},
	{  MONITOR_CAT,		NULL, NULL, NULL, NULL	},
	{  "__size__",		NULL, NULL, NULL, NULL	},
	{  "__nbuttons__",	NULL, NULL, NULL, NULL	},
	{  "__layout__",	NULL, NULL, NULL, NULL	},
	{  "__device__",	NULL, NULL, NULL, NULL	},
	{  "__irq__",		NULL, NULL, NULL, NULL	},
	{  "__ioa__",		NULL, NULL, NULL, NULL	},
	{  "__position__",		NULL, NULL, NULL, NULL	},
	{  "__resolution__",	NULL, NULL, NULL, NULL	},
	{  "__desktop__",	NULL, NULL, NULL, NULL  },
	{  CONFIRM_CAT,		NULL, NULL, NULL, NULL	},
	{  INTRO_CAT,		NULL, NULL, NULL, NULL	},
	{  ERROR_CAT,		NULL, NULL, NULL, NULL	},
	{  "__depth__",		NULL, NULL, NULL, NULL  },
	{  "__bmemaddr__",	NULL, NULL, NULL, NULL  },
	{  "__btype__",		NULL, NULL, NULL, NULL  },
	{  NULL,		NULL, NULL, NULL, NULL	}
};

static int	obj_init(void);
static int	obj_get_index(const char *name);

/*
 * Public interface
 */
UIobj *
ui_get_object(const char *name)
{
	UIobj	*obj;
	int	i;

	(void) obj_init();

	obj = (UIobj *)0;

	i = obj_get_index(name);
	if (i >= 0)
		obj = (UIobj *)&objects[i];

	return (obj);
}

char *
get_object_title(const UIobj *obj)
{
	UIobjP	*objP;

	objP = (UIobjP *)obj;

	return (objP ? objP->title : (char *)0);
}

char *
get_object_label(const UIobj *obj)
{
	UIobjP	*objP;

	objP = (UIobjP *)obj;

	return (objP ? objP->label : (char *)0);
}

char *
get_object_confirm(const UIobj *obj)
{
	UIobjP	*objP;

	objP = (UIobjP *)obj;

	return (objP ? objP->confirm : (char *)0);
}

char *
get_object_text(const UIobj *obj)
{
	UIobjP	*objP;

	objP = (UIobjP *)obj;

	return (objP ? objP->text : (char *)0);
}

Field_help *
get_object_help(const UIobj *obj)
{
	UIobjP	*objP;

	objP = (UIobjP *)obj;

	return (objP ? objP->help : (Field_help *)0);
}

/*
 * Private interface
 */

static int
obj_init(void)
{
	static int	been_here = 0;
	Field_help	*help;

	if (been_here)
		return (0);
	/*
	 * Graphics adapter device
	 */
	objects[OBJ_DISPLAY].title = KDMCONFIG_MSGS(KDMCONFIG_DISPLAY_TITLE);
	objects[OBJ_DISPLAY].label = KDMCONFIG_MSGS(KDMCONFIG_DISPLAY_LABEL);
	objects[OBJ_DISPLAY].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_DISPLAY_CONFIRM);
	objects[OBJ_DISPLAY].text = KDMCONFIG_MSGS(KDMCONFIG_DISPLAY_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_DISPLAY_TOPICS);

	objects[OBJ_DISPLAY].help = help;
	/*
	 * Pointing device (mouse)
	 */
	objects[OBJ_POINTER].title = KDMCONFIG_MSGS(KDMCONFIG_POINTER_TITLE);
	objects[OBJ_POINTER].label = KDMCONFIG_MSGS(KDMCONFIG_POINTER_LABEL);
	objects[OBJ_POINTER].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_POINTER_CONFIRM);
	objects[OBJ_POINTER].text = KDMCONFIG_MSGS(KDMCONFIG_POINTER_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_POINTER_TOPICS);

	objects[OBJ_POINTER].help = help;
	/*
	 * Keyboard device
	 */
	objects[OBJ_KEYBOARD].title = KDMCONFIG_MSGS(KDMCONFIG_KEYBOARD_TITLE);
	objects[OBJ_KEYBOARD].label = KDMCONFIG_MSGS(KDMCONFIG_KEYBOARD_LABEL);
	objects[OBJ_KEYBOARD].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_KEYBOARD_CONFIRM);
	objects[OBJ_KEYBOARD].text = KDMCONFIG_MSGS(KDMCONFIG_KEYBOARD_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_KEYBOARD_TOPICS);

	objects[OBJ_KEYBOARD].help = help;
	/*
	 * Monitor
	 */
	objects[OBJ_MONITOR].title = KDMCONFIG_MSGS(KDMCONFIG_MONITOR_TITLE);
	objects[OBJ_MONITOR].label = KDMCONFIG_MSGS(KDMCONFIG_MONITOR_LABEL);
	objects[OBJ_MONITOR].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_MONITOR_CONFIRM);
	objects[OBJ_MONITOR].text = KDMCONFIG_MSGS(KDMCONFIG_MONITOR_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_MONITOR_TOPICS);

	objects[OBJ_MONITOR].help = help;

	/*
	 * Screen size
	 */
	objects[OBJ_SCREEN].title = KDMCONFIG_MSGS(KDMCONFIG_SCREEN_TITLE);
	objects[OBJ_SCREEN].label = KDMCONFIG_MSGS(KDMCONFIG_SCREEN_LABEL);
	objects[OBJ_SCREEN].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_SCREEN_CONFIRM);
	objects[OBJ_SCREEN].text = KDMCONFIG_MSGS(KDMCONFIG_SCREEN_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_SCREEN_TOPICS);

	objects[OBJ_SCREEN].help = help;
	/*
	 * Number of buttons on pointing device
	 */
	objects[OBJ_BUTTONS].title = KDMCONFIG_MSGS(KDMCONFIG_BUTTONS_TITLE);
	objects[OBJ_BUTTONS].label = KDMCONFIG_MSGS(KDMCONFIG_BUTTONS_LABEL);
	objects[OBJ_BUTTONS].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_BUTTONS_CONFIRM);
	objects[OBJ_BUTTONS].text = KDMCONFIG_MSGS(KDMCONFIG_BUTTONS_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_BUTTONS_TOPICS);

	objects[OBJ_BUTTONS].help = help;
	/*
	 * Layout of keyboard device
	 */
	objects[OBJ_LAYOUT].title = KDMCONFIG_MSGS(KDMCONFIG_LAYOUT_TITLE);
	objects[OBJ_LAYOUT].label = KDMCONFIG_MSGS(KDMCONFIG_LAYOUT_LABEL);
	objects[OBJ_LAYOUT].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_LAYOUT_CONFIRM);
	objects[OBJ_LAYOUT].text = KDMCONFIG_MSGS(KDMCONFIG_LAYOUT_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_LAYOUT_TOPICS);

	objects[OBJ_LAYOUT].help = help;
	/*
	 * Serial port of pointing device
	 */
	objects[OBJ_DEVICE].title = KDMCONFIG_MSGS(KDMCONFIG_DEVICE_TITLE);
	objects[OBJ_DEVICE].label = KDMCONFIG_MSGS(KDMCONFIG_DEVICE_LABEL);
	objects[OBJ_DEVICE].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_DEVICE_CONFIRM);
	objects[OBJ_DEVICE].text = KDMCONFIG_MSGS(KDMCONFIG_DEVICE_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_DEVICE_TOPICS);

	objects[OBJ_DEVICE].help = help;
	/*
	 * Interrupt vector
	 */
	objects[OBJ_IRQ].title = KDMCONFIG_MSGS(KDMCONFIG_IRQ_TITLE);
	objects[OBJ_IRQ].label = KDMCONFIG_MSGS(KDMCONFIG_IRQ_LABEL);
	objects[OBJ_IRQ].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_IRQ_CONFIRM);
	objects[OBJ_IRQ].text = KDMCONFIG_MSGS(KDMCONFIG_IRQ_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_IRQ_TOPICS);

	objects[OBJ_IRQ].help = help;
	/*
	 * IO address
	 */
	objects[OBJ_IOADDR].title = KDMCONFIG_MSGS(KDMCONFIG_IOADDR_TITLE);
	objects[OBJ_IOADDR].label = KDMCONFIG_MSGS(KDMCONFIG_IOADDR_LABEL);
	objects[OBJ_IOADDR].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_IOADDR_CONFIRM);
	objects[OBJ_IOADDR].text = KDMCONFIG_MSGS(KDMCONFIG_IOADDR_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_IOADDR_TOPICS);

	objects[OBJ_IOADDR].help = help;

	/*
	 * Screen Depth
	 */
	objects[OBJ_DEPTH].title = KDMCONFIG_MSGS(KDMCONFIG_DEPTH_TITLE);
	objects[OBJ_DEPTH].label = KDMCONFIG_MSGS(KDMCONFIG_DEPTH_LABEL);
	objects[OBJ_DEPTH].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_DEPTH_CONFIRM);
	objects[OBJ_DEPTH].text = KDMCONFIG_MSGS(KDMCONFIG_DEPTH_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_DEPTH_TOPICS);

	objects[OBJ_DEPTH].help = help;

	/*
	 * Screen Position
	 */
	objects[OBJ_POSITION].title = KDMCONFIG_MSGS(KDMCONFIG_POSITION_TITLE);
	objects[OBJ_POSITION].label = KDMCONFIG_MSGS(KDMCONFIG_POSITION_LABEL);
	objects[OBJ_POSITION].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_POSITION_CONFIRM);
	objects[OBJ_POSITION].text = KDMCONFIG_MSGS(KDMCONFIG_POSITION_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_POSITION_TOPICS);

	objects[OBJ_POSITION].help = help;
	/*
	 * Display Resolution
	 */
	objects[OBJ_RESOLUTION].title =
			    KDMCONFIG_MSGS(KDMCONFIG_RESOLUTION_TITLE);
	objects[OBJ_RESOLUTION].label =
			    KDMCONFIG_MSGS(KDMCONFIG_RESOLUTION_LABEL);
	objects[OBJ_RESOLUTION].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_RESOLUTION_CONFIRM);
	objects[OBJ_RESOLUTION].text =
			    KDMCONFIG_MSGS(KDMCONFIG_RESOLUTION_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_RESOLUTION_TOPICS);

	objects[OBJ_RESOLUTION].help = help;

	/*
	 * Virtual Display Resolution
	 */
	objects[OBJ_DESKTOP].title =
			    KDMCONFIG_MSGS(KDMCONFIG_DESKTOP_TITLE);
	objects[OBJ_DESKTOP].label =
			    KDMCONFIG_MSGS(KDMCONFIG_DESKTOP_LABEL);
	objects[OBJ_DESKTOP].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_DESKTOP_CONFIRM);
	objects[OBJ_DESKTOP].text =
			    KDMCONFIG_MSGS(KDMCONFIG_DESKTOP_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_DESKTOP_TOPICS);

	objects[OBJ_DESKTOP].help = help;
	/*
	 * Confirmation screen
	 */
	objects[OBJ_CONFIRM].title = KDMCONFIG_MSGS(KDMCONFIG_CONFIRM_TITLE);
	objects[OBJ_CONFIRM].label = (char *)0;
	objects[OBJ_CONFIRM].confirm = (char *)0;
	objects[OBJ_CONFIRM].text = KDMCONFIG_MSGS(KDMCONFIG_CONFIRM_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_CONFIRM_TOPICS);

	objects[OBJ_CONFIRM].help = help;
	/*
	 * Introductory screen
	 */
	objects[OBJ_INTRO].title = KDMCONFIG_MSGS(KDMCONFIG_INTRO_TITLE);
	objects[OBJ_INTRO].label = (char *)0;
	objects[OBJ_INTRO].confirm = (char *)0;
	objects[OBJ_INTRO].text = KDMCONFIG_MSGS(KDMCONFIG_INTRO_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_NAVIGATE_TOPICS);

	objects[OBJ_INTRO].help = help;

	/*
	 * Memory Address
	 */
	objects[OBJ_BMEMADDR].title = KDMCONFIG_MSGS(KDMCONFIG_BMEMADDR_TITLE);
	objects[OBJ_BMEMADDR].label =  KDMCONFIG_MSGS(KDMCONFIG_BMEMADDR_LABEL);
	objects[OBJ_BMEMADDR].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_BMEMADDR_CONFIRM);
	objects[OBJ_BMEMADDR].text = KDMCONFIG_MSGS(KDMCONFIG_BMEMADDR_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));
	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_BMEMADDR_TOPICS);

	objects[OBJ_BMEMADDR].help = help;
        /*
	 * Bus Type
	 */
	objects[OBJ_BUSTYPE].title = KDMCONFIG_MSGS(KDMCONFIG_BUSTYPE_TITLE);
	objects[OBJ_BUSTYPE].label =  KDMCONFIG_MSGS(KDMCONFIG_BUSTYPE_LABEL);
	objects[OBJ_BUSTYPE].confirm =
			    KDMCONFIG_MSGS(KDMCONFIG_BUSTYPE_CONFIRM);
	objects[OBJ_BUSTYPE].text = KDMCONFIG_MSGS(KDMCONFIG_BUSTYPE_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_BUSTYPE_TOPICS);

	objects[OBJ_BUSTYPE].help = help;
	/*
	 * Error screen(s)
	 */
	objects[OBJ_ERROR].title = KDMCONFIG_MSGS(KDMCONFIG_ERROR_TITLE);
	objects[OBJ_ERROR].label = (char *)0;
	objects[OBJ_ERROR].confirm = (char *)0;
	objects[OBJ_ERROR].text = KDMCONFIG_MSGS(KDMCONFIG_ERROR_TEXT);

	help = (Field_help *)malloc(sizeof (Field_help));

	help->howto = (char *)0;
	help->reference = (char *)0;
	help->topics = KDMCONFIG_MSGS(KDMCONFIG_NAVIGATE_TOPICS);

	objects[OBJ_ERROR].help = help;

	been_here++;

	return (0);
}

static int
obj_get_index(const char *name)
{
	UIobjP	*objP;

	for (objP = objects; objP->name != (char *)0; objP++)
		if (strcmp(name, objP->name) == 0)
			break;

	return (objP->name != (char *)0 ? objP - objects : -1);
}
