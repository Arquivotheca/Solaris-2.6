
/* Copyright 1993 Sun Microsystems, Inc. */

#ifndef	UTIL_H
#define	UTIL_H

#pragma ident "@(#)util.h	1.21 95/05/04 Sun Microsystems"

/*	util.h	*/


#include <sys/types.h>
#include <Xm/Xm.h>
#include "sysman_iface.h"
#include "spmisoft_api.h"

#define OK_STRING	catgets(_catd, 8, 20, "OK")
#define APPLY_STRING	catgets(_catd, 8, 21, "Apply")
#define RESET_STRING	catgets(_catd, 8, 22, "Reset")
#define CANCEL_STRING	catgets(_catd, 8, 23, "Cancel")
#define HELP_STRING	catgets(_catd, 8, 24, "Help")

#define	RSC_CVT( rsc_name, rsc_value ) \
	XtVaTypedArg, (rsc_name), XmRString, (rsc_value), strlen(rsc_value) + 1

#define	OTHERLABEL	catgets(_catd, 8, 466, "Other...")
	
#define YES_STRING 	catgets(_catd, 8, 182, "Yes") 
#define NO_STRING	catgets(_catd, 8, 183, "No")
#define CANT_ALLOC_MSG  catgets(_catd, 8, 470, "Unable to malloc")

#define MISSING_REQ_ARGS  \
catgets(_catd, 8, 467, "You did not specify a value for the following required parameters: \
\n\n%s\n\n Please enter values for these parameters and retry the operation.")

#define ERRBUF_SIZE 1024

#define FORM_OFFSET 15
#define DEFAULT_PBTN_OFFSET FORM_OFFSET-6

#define MAXPRINTERNAMELEN	14
#define MAXSERVERNAMELEN	64
#define MAXCOMMENTLEN		256
#define MAXUSERNAMELEN		8

#define YES	1
#define	NO	0

#define	ALL_USERS	"all"

#define MANAGEDCDPATH		"/cdrom/cdrom0/s0"
#define UNMANAGEDCDDEVICE	"/dev/dsk/c0t6d0s0"
#define CDMOUNTPOINT		"/export/install"

/* Browser Context */

typedef enum {	ctxt_user,
		ctxt_group,
		ctxt_host,
		ctxt_printer,
		ctxt_serial,
		ctxt_sw
} context_t;


typedef char numstr     [8];
typedef char shortstr  [14];
typedef char labelstr  [80];
typedef char medstr   [258];
typedef char longstr [1026];

typedef struct install_size_reqs {
	char 	* mountp;
	int	size;
} InstallSizeReqs;

typedef struct _sw_struct {
	ModType		sw_type;
	const char	*sw_name;
	const char	*sw_id;
	const char	*version;
	const char	*desc;
	const char	*category;
	const char	*arch;
	const char	*date;
	const char	*vendor;
	const char	*prodname;
	const char	*prodvers;
	const char	*basedir;
	const char	*locale;
        const char	*instance;
	int		size;
	InstallSizeReqs	* install_reqs;
	int		level;
} SWStruct;

typedef	struct {
	char* mail;
	char* instance;
	char* partial;
	char* runlevel;
	char* idepend;
	char* rdepend;
	char* space;
	char* setuid;
	char* conflict;
	char* action;
	char* basedir;
	char* showcr;
	char* interactive;
} PkgAdminProps;

typedef	struct {
	Widget	mainWindow;
	Widget	fileMenu;
	Widget	exitMenuItem;
	Widget	editMenu;
	Widget	addMenuItem;
	Widget	addMenu;
	Widget	localPrinterMenuItem;
	Widget	remotePrinterMenuItem;
	Widget	modifyMenuItem;
	Widget	deleteMenuItem;
	Widget	manageMenu;
	Widget	usersMenuItem;
	Widget	groupsMenuItem;
	Widget	hostsMenuItem;
	Widget	printersMenuItem;
	Widget	portsMenuItem;
	Widget	softwareMenuItem;
	Widget	propMenu;
	Widget	adminMenuItem;
	Widget	helpMenu;
	Widget	aboutMenuItem;
	Widget	aboutContextMenuItem;
	Widget	workForm;
	Widget	listForm;
	Widget	userHeader;
	Widget	userLabel;
	Widget	uidLabel;
	Widget	userCommentLabel;
	Widget	groupHeader;
	Widget	groupLabel;
	Widget	gidLabel;
	Widget	groupMembersLabel;
	Widget	hostHeader;
	Widget	hostLabel;
	Widget	hostIpLabel;
	Widget	printerHeader;
	Widget	printerLabel;
	Widget	serverLabel;
	Widget	printerCommentLabel;
	Widget	serialHeader;
	Widget	serialLabel;
	Widget	serviceLabel;
	Widget	tagLabel;
	Widget	serialCommentLabel;
	Widget	softwareHeader;
	Widget	softwareLabel;
	Widget	swViewOptionMenu;
	Widget	swIdLabel;
	Widget	swSizeLabel;
	Widget	listScrollWin;
	Widget	objectList;
	Widget	defPrinterLabel;
	Widget	detailsButton;
	Widget	currHostLabel;
	Widget	currDialog;
} sysMgrMainCtxt;

extern XtAppContext	GappContext;
extern Widget		GtopLevel;
extern Display		*Gdisplay;
extern int		Gscreen;
extern char*		localhost;
extern char		errbuf[];

extern char * installFileSystems[];

void	addListEntryCB(Widget wgt, XtPointer cd, XtPointer cbs);
void	copy_user(SysmanUserArg* d, SysmanUserArg* s);
void	copy_group(SysmanGroupArg* d, SysmanGroupArg* s);
void	copy_host(SysmanHostArg* d, SysmanHostArg* s);
void	copy_printer(SysmanPrinterArg* d, SysmanPrinterArg* s);
void	copy_serial(SysmanSerialArg* d, SysmanSerialArg* s);
void	copy_software(SysmanSWArg* d, SysmanSWArg* s);
void	deleteListEntryCB(Widget wgt, XtPointer cd, XtPointer cbs);
void	free_mem(void* ptr);
void	free_user(SysmanUserArg* u_ptr);
void	free_group(SysmanGroupArg* g_ptr);
void	free_host(SysmanHostArg* h_ptr);
void	free_printer(SysmanPrinterArg* p_ptr);
void	free_serial(SysmanSerialArg* s_ptr);
/* XXXXXXX Does this stay this way? */
void	free_software(SWStruct* sw_ptr);
int	Confirm(Widget parent, char* msg, int* del_homedir, char* verb);
XmFontList ConvertFontList(char* fontlist_str);
Widget	create_button_box(Widget parent, Widget top_widget, void* context,
	Widget* ok, Widget* apply, Widget* reset, Widget* cancel, Widget* help);
void	get_admin_file_values(PkgAdminProps*);
unsigned long get_fs_space(FSspace **sp, char *fs);
unsigned long get_pkg_fs_space(Modinfo* mi, char* fs);
FSspace* get_fs(FSspace**, char*);
int     load_admin_file(char*);
char*	GetPromptInput(Widget parent, char* title, char * msg);
void	display_error(Widget parent, char* msg);
Widget	display_warning(Widget parent, char* msg);
Widget	find_parent(Widget);
void	format_the_date(int day, int month, int year, char* result_p);
int	get_selected_modules(Module *m, struct modinfo *** list);
Widget	get_shell_ancestor(Widget w);
char    * get_mod_name(Module *);
void	helpCB(Widget, char* helpfile, XtPointer);
int	min2(int a, int b);
void	MakePosVisible(Widget list, int item);
void	SetBusyPointer(Boolean	busystate);
void	stringcopy(char* dest, char* src, int maxlen);
int	wildmatch(char* s, char* p);
char*	write_admin_file(PkgAdminProps* admin);
char*	mungeSolarisPath(char *path);


#endif	/* UTIL_H */

