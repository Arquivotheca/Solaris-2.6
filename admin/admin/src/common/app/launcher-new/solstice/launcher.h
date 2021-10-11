#include <X11/Intrinsic.h>

#include <nl_types.h>
#include "launcher_api.h"

nl_catd		catd;	/* for catgets() */


typedef enum { LEFT, CENTER, RIGHT } title_pos_t;
typedef enum { LOCAL=0, GLOBAL } registry_loc_t;
typedef enum { HIDE=0, SHOW, SHOW_PENDING, HIDE_PENDING } visibility_t;

typedef struct app_info {
	Boolean		a_obsolete;
	Boolean		a_errReported;
	unsigned int	a_displayOrdinal;
	visibility_t	a_show;
  	registry_loc_t	a_site;	
	Widget		a_iconW;
	Widget		a_toggle;
	Widget		a_label;
	char 		* a_appName;
	char		* a_appPath;
	char		* a_appArgs;
	char		* a_iconPath;
	char		* a_scriptName;
	pid_t		a_pid;
	XtIntervalId	a_timer;
} AppInfo;

typedef struct field_s {
	short	f_ncols;
	int	f_maxlen;
	Boolean f_fileSelection;
	char * f_label;
	Widget f_form;
	Widget f_fieldWidget;
	Widget f_ellipsisWidget;
} FieldItem;


typedef struct property_data_s {
	Widget p_form;
	Widget p_appNameField;
	Widget p_appPathField;
	Widget p_appArgsField;
	Widget p_iconPathField;
	Widget p_scriptNameField;
} propertyContext_t;

typedef struct config_context_s {
	int	c_currSelection;
	Widget	c_mainWindow;
	Widget	c_workForm;	
	Widget	c_title;
	Widget	c_scrollWin;	
	Widget	c_appList;
	Widget	c_addAppButton;
 	Widget	c_rmAppButton;
	Widget	c_propertyButton;
	Widget	c_upButton;
	Widget	c_downButton;	
	Widget 	c_buttonBox;
	Widget	c_closeButton;
	Widget	c_helpButton;
	Widget  c_listButtonForm;
	Widget	c_hidetitle;
	Widget  c_hidescrollWin;
	Widget  c_hideappList;
	Widget  c_showButton;
	Widget  c_hideButton;
	Widget 	c_numColumns;
} configContext_t;

typedef struct launcher_context_s {
	Widget	l_mainWindow;
	Widget	l_workForm;
	Widget	l_menuBar;
	Widget  l_fileMenu;
	Widget	l_drawingArea;
	Widget  l_addMenuItem;
	Widget  l_configMenuItem;
	Widget  l_exitMenuItem;
	Widget	l_helpMenu;
	Widget	l_aboutMenuItem;
	int	l_appTableMax; 	/* size of appTable	    */
	int	l_appCount;	/* # of entries in appTable */
	int	l_showCount;	/* tracks # of SHOW entries */
	AppInfo *l_appTable;
} launcherContext_t;

extern Widget launchermain;
extern Widget configDialog;
extern Widget propertyDialog;
extern Widget fileSelectionDialog;
	
extern launcherContext_t * launcherContext;
extern configContext_t * configContext;
extern propertyContext_t * propertyContext;

extern char * localRegistry;
extern char * sysRegistry;

#define ADD_NONAME_MSG 	\
    catgets(catd, 1, 88, "An application name must be provided in order to register an application")
#define ADD_NOPATH_MSG 	\
    catgets(catd, 1, 89, "A path must be provided in order to register an application")


#define LAUNCH_TIMEOUT  10000 /* millisecs */

