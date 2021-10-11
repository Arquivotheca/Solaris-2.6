
/* Copyright 1995 Sun Microsystems, Inc. */
/* All rights reserved. */

#pragma ident "@(#)main_win.c	1.58 96/10/11 Sun Microsystems"

/*	main_win.c	*/

#include <stdlib.h>
#include <stdio.h>
#include <macros.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MenuShell.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/ToggleBG.h>
#include <X11/Shell.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#include <X11/IntrinsicP.h>

#include "sysman_iface.h"
#include "sysman_codes.h"
#include "spmisoft_api.h"
#include "software.h"
#include "util.h"
#include "add_user.h"

#define USER		0
#define GROUP		1
#define HOST		2
#define PRINTER		3
#define SERIAL		4
#define SOFTWARE	5
 
struct {
	char *uc_name;
	char *lc_name;
} object_names[] = {
	{ NULL, "user" },
	{ NULL, "group" },
	{ NULL, "host"},
	{ NULL, "printer"}, 
	{ NULL, "serial"},
	{ NULL, "software" }
};


typedef struct {
	int	type;
	union {
		SysmanUserArg		user;
		SysmanGroupArg		group;
		SysmanHostArg		host;
		SysmanPrinterArg	printer;
		SysmanSerialArg		serial;
		/*SysmanSWArg		software;*/
		SWStruct		software;
	} obj;
} obj_data;

typedef enum {sw_view_all, sw_view_sys, sw_view_app } sw_view_t;

#define BAD_SW_LIB_MSG \
catgets(_catd, 8, 502, "You are possibly running admintool with an \n\
incompatible version of Solaris OS.\n\
The software add and remove capability will be disabled.")

#define LABEL_BOTTOM_OFFSET 10


/* external functions */
extern void show_addgroupdialog(Widget parent, sysMgrMainCtxt * ctxt);
extern void show_addhostdialog(Widget parent, sysMgrMainCtxt * ctxt);
extern void show_addlocaldialog(Widget parent, sysMgrMainCtxt * ctxt);
extern void show_addremotedialog(Widget parent, sysMgrMainCtxt * ctxt);
extern void show_addsoftwaredialog(Widget parent, sysMgrMainCtxt * ctxt, char* path);
extern void show_adduserdialog(Widget parent, sysMgrMainCtxt * ctxt);
extern void show_modifygroupdialog(Widget parent, SysmanGroupArg* group, sysMgrMainCtxt * ctxt);
extern void show_modifyhostdialog(Widget parent, SysmanHostArg* host, sysMgrMainCtxt * ctxt);
extern void show_modifyprinterdialog(Widget parent, SysmanPrinterArg* printer, sysMgrMainCtxt * ctxt);
extern void show_modifyserialdialog(Widget parent, SysmanSerialArg* serial, sysMgrMainCtxt * ctxt);
extern void show_modifyswdialog(Widget parent, SWStruct* sw, sysMgrMainCtxt * ctxt);
extern void show_modifyuserdialog(Widget parent, SysmanUserArg* group, sysMgrMainCtxt * ctxt);
extern show_pkgadmindialog(Widget parent, sysMgrMainCtxt * ctxt);

extern Module* sysman_list_sw(char* src);

/* internal functions */
static int	build_sw_list(Module *m, int lev, sw_view_t view, int *stpt);
static void	delete_list_entry(int index);
void		display_host_list(void);
void		free_objlist(void);
static char*	get_display_string(obj_data* od);
void		init_mainwin();
int		load_list(int type);
void		set_default_printer_msg(char* printername);
static void	set_menus_active(Boolean active);
static void	set_edit_functions_active(Boolean active);

Widget		sysmgrmain;

int		current_type;
obj_data**	objlist = NULL;
int		entry_count;

static Module*	installed_sw_tree;

#define 	SW_LIST_PGSZ	200
SWStruct	* sw_list = NULL;
int		sw_list_nentries = 0;
int		sw_list_cnt;

extern context_t	initial_ctxt;
extern boolean_t	show_browse_menu;

/* scrolling list entry formats */
char	user_fmt[64];
char	group_fmt[64];
char	host_fmt[64];
char	printer_fmt[64];
char	serial_fmt[64];
char	dis_serial_fmt[64];
char	sw_fmt[74];

/* scrolling list column widths (in characters) */
int	username_cw	= 10;
int	userid_cw	= 10;
int	usercmnt_cw	= 80;
int	groupname_cw	= 10;
int	groupid_cw	= 10;
int	groupmembers_cw	= 80;
int	hostname_cw	= 14;
int	hostip_cw	= 15;
int	printername_cw	= 14;
int	printserver_cw	= 14;
int	printercmnt_cw	= 80;
int	serial_cw	= 8;
int	service_cw	= 12;
int	tag_cw		= 12;
int	serialcmnt_cw	= 80;
int	swpkg_cw	= 70;
int	swsize_cw	= 5;

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

/*
 *  Exit
 */
static void
exitCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	exit(0);
}

/*
 *  Add
 */
static void
addCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	sysMgrMainCtxt *ctxt = (sysMgrMainCtxt*)cd;

	switch (current_type) {
	    case USER:
		show_adduserdialog(sysmgrmain, ctxt);
		break;

	    case GROUP:
		show_addgroupdialog(sysmgrmain, ctxt);
		break;

	    case HOST:
		show_addhostdialog(sysmgrmain, ctxt);
		break;

	    case PRINTER:
		if (w == ctxt->localPrinterMenuItem) {
			show_addlocaldialog(sysmgrmain, ctxt);
		}
		else if (w == ctxt->remotePrinterMenuItem) {
			show_addremotedialog(sysmgrmain, ctxt);
		}
		break;

	    case SERIAL:
		break;

	    case SOFTWARE:
		show_addsoftwaredialog(sysmgrmain, ctxt, NULL);
		break;
	}
}

/*
 *  Modify
 */
static void
modifyCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	sysMgrMainCtxt *ctxt = (sysMgrMainCtxt*)cd;
	int*		pos_list;
	int		pos_count;
	int		index;

	if (XmListGetSelectedPos(ctxt->objectList, &pos_list, &pos_count)) {
		index = pos_list[0]-1;
		XtFree((char*)pos_list);

		switch (objlist[index]->type) {
		    case USER:
			show_modifyuserdialog(sysmgrmain,
				&objlist[index]->obj.user, ctxt);
			break;
	
		    case GROUP:
			show_modifygroupdialog(sysmgrmain,
				&objlist[index]->obj.group, ctxt);
			break;
	
		    case HOST:
			show_modifyhostdialog(sysmgrmain,
				&objlist[index]->obj.host, ctxt);
			break;
	
		    case PRINTER:
			show_modifyprinterdialog(sysmgrmain,
				&objlist[index]->obj.printer, ctxt);
			break;
	
		    case SERIAL:
			show_modifyserialdialog(sysmgrmain,
				&objlist[index]->obj.serial, ctxt);
			break;
	
		    case SOFTWARE:
			break;
		}
	}
}

/*
 *  Delete
 */
static void
deleteCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	sysMgrMainCtxt*		ctxt = (sysMgrMainCtxt*)cd;
	SysmanUserArg*		user;
	SysmanGroupArg*		group;
	SysmanHostArg*		host;
	SysmanPrinterArg*	printer;
	SysmanSerialArg*	serial;
	SWStruct*		sw;
	SysmanSWArg		pkg;
	PkgAdminProps		pkgprops;
	int*			pos_list;
	int			pos_count;
	int			index;
	int			sts = 0;
	int			ii;
	char*			def_printer;
	char*			list = NULL;
	char*			buf = NULL;
	char*			name;
	int			del_homedir = 0;
	int*			del_homedir_p = NULL;
	int			strsize = 0;
	int			bufsize = 256;
	char			msg[1024];
	char			tmpbuf[128];
	char 			** del_list;
	int 			di = 0;
	int			toppos;
	int			idx;
	XmString		xstr;


#define DEL_USER_CONF catgets(_catd, 8, 263, "Do you really want to Delete user `%s'?")
#define DEL_GROUP_CONF catgets(_catd, 8, 248, "Do you really want to Delete group `%s'?")
#define DEL_HOST_CONF catgets(_catd, 8, 249, "Do you really want to Delete host `%s'?")
#define DEL_PRINTER_CONF catgets(_catd, 8, 250, "Do you really want to Delete printer `%s'?")
#define DEL_SERIAL_CONF catgets(_catd, 8, 251, "Do you really want to Delete service `%s'?")
#define DEL_SW_CONF catgets(_catd, 8, 252, "Do you really want to Delete the following software?\n\n")

#define ROOT_DELETE_CAUTION_MSG \
        catgets(_catd, 8, 651, "You are deleting a root account!\n" \
        "Admintool can not be used to delete a root account.\n" \
        "To delete user `%s` run userdel from a shell.")


	if (XmListGetSelectedPos(ctxt->objectList, &pos_list, &pos_count)) {
		index = pos_list[0]-1;

		switch (objlist[index]->type) {
		    case USER:
			user = &objlist[index]->obj.user;
        		if (USR_IS_ROOT(user->username, user->uid)) {
				/* Notify the user a root id can not be deleted. */
				sprintf(msg, ROOT_DELETE_CAUTION_MSG, (char*)user->username);
				display_error(sysmgrmain, msg);
			        free_mem(buf);
			        return;
				}
			else
				sprintf(msg, DEL_USER_CONF, (char*)user->username);
			del_homedir_p = &del_homedir;
			break;
	
		    case GROUP:
			group = &objlist[index]->obj.group;
			sprintf(msg, DEL_GROUP_CONF, (char*)group->groupname);
			break;
	
		    case HOST:
			host = &objlist[index]->obj.host;
			sprintf(msg, DEL_HOST_CONF, (char*)host->hostname);
			break;
	
		    case PRINTER:
			printer = &objlist[index]->obj.printer;
			sprintf(msg, DEL_PRINTER_CONF, (char*)printer->printername);
			break;
	
		    case SERIAL:
			serial = &objlist[index]->obj.serial;
			sprintf(msg, DEL_SERIAL_CONF, (char*)serial->svctag);
			break;
	
		    case SOFTWARE:
			sprintf(msg, DEL_SW_CONF);
			del_list = (char **) malloc(pos_count * sizeof(char *));
			memset(del_list, 0, pos_count * sizeof(char *));
			for (ii = 0; ii < pos_count; ii++) {
				index = pos_list[ii] - 1;
				sw = &objlist[index]->obj.software;
				if (sw->sw_type == PACKAGE) {
					del_list[di] = (char *) 
						(sw->instance ? 
 						   sw->instance : sw->sw_id);
				        sprintf(tmpbuf, "%s\n", 
						(sw->instance ? 
						   sw->instance : sw->sw_name));
				        strcat(msg, tmpbuf);
					di++;
				}
			}
			break;
		}
		XtFree((char*)pos_list);

		if (!Confirm(sysmgrmain, msg, del_homedir_p, catgets(_catd, 8, 253, "Delete"))) {
			free_mem(buf);
			return;
		}

		SetBusyPointer(True);
		switch (objlist[index]->type) {
		    case USER:
			user->home_dir_flag = del_homedir;
			sts = sysman_delete_user(user, errbuf, ERRBUF_SIZE);
			break;
	
		    case GROUP:
			sts = sysman_delete_group(group, errbuf, ERRBUF_SIZE);
			break;
	
		    case HOST:
			sts = sysman_delete_host(host, errbuf, ERRBUF_SIZE);
			break;
	
		    case PRINTER:
			sts = sysman_delete_printer(printer, errbuf, ERRBUF_SIZE);
			break;
	
		    case SERIAL:
			sts = sysman_delete_serial(serial, errbuf, ERRBUF_SIZE);
			break;
	
		    case SOFTWARE:
			sysman_sw_do_gui(B_TRUE, DisplayString(Gdisplay));

			memset((void *)&pkg, 0, sizeof (pkg));

			get_admin_file_values(&pkgprops);
			pkg.admin = write_admin_file(&pkgprops);

			pkg.show_copyrights =
			    (strcmp(pkgprops.showcr, "yes") == 0) ? 1 : 0;
			pkg.non_interactive =
			    (strcmp(pkgprops.interactive, "no") ==0) ? 1 : 0;

			pkg.num_pkgs = di;
			pkg.pkg_names = (const char**)del_list;

			sts = sysman_delete_sw(&pkg, errbuf, ERRBUF_SIZE);

			free(del_list);
			free_mem(buf);
			if (pkg.admin)
				unlink(pkg.admin);
			break;
		}
		SetBusyPointer(False);

		if (sts < 0) {
			display_error(sysmgrmain, errbuf);
		}
		else {
			if (sts == SYSMAN_INFO) {
				display_infomsg(sysmgrmain, errbuf);
			}
			if (objlist[index]->type == PRINTER) {
				sysman_get_default_printer_name(&def_printer,
						errbuf, ERRBUF_SIZE);
				set_default_printer_msg(def_printer);
				if (def_printer)
					free(def_printer);
			}

			if (objlist[index]->type == SOFTWARE) {
				/* re-build entire tree for now... */	
				XtVaGetValues(ctxt->objectList, 
					XmNtopItemPosition, &toppos, NULL);
				load_list(SOFTWARE);
				XtVaSetValues(ctxt->objectList, 
					XmNtopItemPosition, toppos, NULL);
			}
			else if (objlist[index]->type == SERIAL) {
				serial->service_enabled = s_inactive;
				if (serial->svctag != NULL) {
					free((char *)serial->svctag);
					serial->svctag = NULL;
				}

				idx = find_entry(serial);
				if (idx < 0) {
					/* should never happen */
					printf(
					"Couldn't find object in main list!\n");
				}
				else {
					xstr = XmStringCreateLocalized(
					    get_display_string(objlist[idx]));

					XmListReplaceItemsPos(ctxt->objectList,
					    &xstr, 1, idx+1);
					MakePosVisible(ctxt->objectList, idx+1);
					XmListDeselectAllItems(ctxt->objectList);
					XmStringFree(xstr);
				}
			}
			else {
			 	delete_list_entry(index);
			}

			set_edit_functions_active(False);
		}
	}
}

void
set_default_printer_msg(char* printername)
{
	XmString	xstr;
	char		buf[128];
	sysMgrMainCtxt* ctxt;


	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	if (printername == NULL) {
		printername = catgets(_catd, 8, 254, "None");
	}

	sprintf(buf, catgets(_catd, 8, 255, "Default Printer: %s"), printername);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->defPrinterLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

}

static void
manageCB(
	Widget wgt, 
	XtPointer cd, 
	XmRowColumnCallbackStruct* cbs)
{
	sysMgrMainCtxt *ctxt = (sysMgrMainCtxt*)cd;
	XtCallbackProc cbf = NULL;
	XmToggleButtonCallbackStruct* tb_cbs =
		(XmToggleButtonCallbackStruct*)cbs->callbackstruct;
	int		objtype;
	char*		printer;
	Widget		wa[8];
	XmString	mtmp;
	static char	tmp[64];
	static int	first = TRUE;

	if (first) {
	object_names[0].uc_name = malloc(strlen(catgets(_catd, 8, 242, "Users"))+1);
	strcpy(object_names[0].uc_name, catgets(_catd, 8, 242, "Users"));

	object_names[1].uc_name = malloc(strlen(catgets(_catd, 8, 243, "Groups"))+1);
	strcpy(object_names[1].uc_name, catgets(_catd, 8, 243, "Groups"));

	object_names[2].uc_name = malloc(strlen(catgets(_catd, 8, 244, "Hosts"))+1);
	strcpy(object_names[2].uc_name, catgets(_catd, 8, 244, "Hosts"));

	object_names[3].uc_name = malloc(strlen(catgets(_catd, 8, 245, "Printers"))+1);
	strcpy(object_names[3].uc_name, catgets(_catd, 8, 245, "Printers"));

	object_names[4].uc_name = malloc(strlen(catgets(_catd, 8, 246, "Serial Ports"))+1);
	strcpy(object_names[4].uc_name, catgets(_catd, 8, 246, "Serial Ports"));

	object_names[5].uc_name = malloc(strlen(catgets(_catd, 8, 247, "Software"))+1);
	strcpy(object_names[5].uc_name, catgets(_catd, 8, 247, "Software"));

	first = FALSE;
	}


	if ((cbs->reason != XmCR_ACTIVATE) || (!tb_cbs->set))
		return;

	objtype = (int)cbs->data;

	switch (objtype) {
	    case USER:
		if (load_list(USER)) {
			XtManageChild(ctxt->userHeader);
			wa[0] = ctxt->groupHeader;
			wa[1] = ctxt->hostHeader;
			wa[2] = ctxt->printerHeader;
			wa[3] = ctxt->serialHeader;
			wa[4] = ctxt->softwareHeader;
			XtUnmanageChildren(wa, 5);
	
			XtUnmanageChild(ctxt->addMenu);
			XtManageChild(ctxt->addMenuItem);
			XtUnmanageChild(ctxt->defPrinterLabel);
			XtUnmanageChild(ctxt->detailsButton);
			XtUnmanageChild(ctxt->propMenu);

	
			XtVaSetValues(XtParent(sysmgrmain),
				XmNtitle, catgets(_catd, 8, 256, "Admintool: Users"),
				NULL );
		}
		break;

	    case GROUP:
		if (load_list(GROUP)) {
			XtManageChild(ctxt->groupHeader);
			wa[0] = ctxt->userHeader;
			wa[1] = ctxt->hostHeader;
			wa[2] = ctxt->printerHeader;
			wa[3] = ctxt->serialHeader;
			wa[4] = ctxt->softwareHeader;
			XtUnmanageChildren(wa, 5);
	
			XtUnmanageChild(ctxt->addMenu);
			XtManageChild(ctxt->addMenuItem);
			XtUnmanageChild(ctxt->defPrinterLabel);
			XtUnmanageChild(ctxt->detailsButton);
			XtUnmanageChild(ctxt->propMenu);
	
			XtVaSetValues(XtParent(sysmgrmain),
				XmNtitle, catgets(_catd, 8, 257, "Admintool: Groups"),
				NULL );
		}
		break;

	    case HOST:
		if (load_list(HOST)) {
			XtManageChild(ctxt->hostHeader);
			wa[0] = ctxt->userHeader;
			wa[1] = ctxt->groupHeader;
			wa[2] = ctxt->printerHeader;
			wa[3] = ctxt->serialHeader;
			wa[4] = ctxt->softwareHeader;
			XtUnmanageChildren(wa, 5);
	
			XtUnmanageChild(ctxt->addMenu);
			XtManageChild(ctxt->addMenuItem);
			XtUnmanageChild(ctxt->defPrinterLabel);
			XtUnmanageChild(ctxt->detailsButton);
			XtUnmanageChild(ctxt->propMenu);
	
			XtVaSetValues(XtParent(sysmgrmain),
				XmNtitle, catgets(_catd, 8, 258, "Admintool: Hosts"),
				NULL );
		}
		break;

	    case PRINTER:
		if (load_list(PRINTER)) {
			XtManageChild(ctxt->printerHeader);
			wa[0] = ctxt->userHeader;
			wa[1] = ctxt->groupHeader;
			wa[2] = ctxt->hostHeader;
			wa[3] = ctxt->serialHeader;
			wa[4] = ctxt->softwareHeader;
			XtUnmanageChildren(wa, 5);
	
			XtUnmanageChild(ctxt->addMenuItem);
			XtManageChild(ctxt->addMenu);
			XtManageChild(ctxt->defPrinterLabel);
			XtUnmanageChild(ctxt->detailsButton);
			XtUnmanageChild(ctxt->propMenu);
	
			sysman_get_default_printer_name(&printer,
				errbuf, ERRBUF_SIZE);
			set_default_printer_msg(printer);
			if (printer)
				free(printer);
	
			XtVaSetValues(XtParent(sysmgrmain),
				XmNtitle, catgets(_catd, 8, 259, "Admintool: Printers"),
				NULL );
		}
		break;

	    case SERIAL:
		if (load_list(SERIAL)) {
			XtManageChild(ctxt->serialHeader);
			wa[0] = ctxt->userHeader;
			wa[1] = ctxt->groupHeader;
			wa[2] = ctxt->hostHeader;
			wa[3] = ctxt->printerHeader;
			wa[4] = ctxt->softwareHeader;
			XtUnmanageChildren(wa, 5);
	
			XtUnmanageChild(ctxt->addMenu);
			XtManageChild(ctxt->addMenuItem);
			XtUnmanageChild(ctxt->defPrinterLabel);
			XtUnmanageChild(ctxt->detailsButton);
			XtUnmanageChild(ctxt->propMenu);
	
			XtVaSetValues(XtParent(sysmgrmain),
				XmNtitle, catgets(_catd, 8, 260, "Admintool: Serial Ports"),
				NULL );
		}
		break;

	    case SOFTWARE:
		if (load_list(SOFTWARE)) {
			XtManageChild(ctxt->softwareHeader);
			wa[0] = ctxt->userHeader;
			wa[1] = ctxt->groupHeader;
			wa[2] = ctxt->hostHeader;
			wa[3] = ctxt->printerHeader;
			wa[4] = ctxt->serialHeader;
			XtUnmanageChildren(wa, 5);
	
			XtUnmanageChild(ctxt->addMenu);
			XtManageChild(ctxt->addMenuItem);
			XtUnmanageChild(ctxt->defPrinterLabel);
			XtManageChild(ctxt->detailsButton);
			XtManageChild(ctxt->propMenu);

			XtVaSetValues(ctxt->listScrollWin,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, ctxt->detailsButton,
				XmNbottomOffset, 5,
				NULL );
	
			XtVaSetValues(XtParent(sysmgrmain),
				XmNtitle, catgets(_catd, 8, 261, "Admintool: Software"),
				NULL );
		}
		break;
	}
	XtVaSetValues(ctxt->listScrollWin,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, ctxt->currHostLabel,
			XmNbottomOffset, LABEL_BOTTOM_OFFSET,
			NULL );

	if (objtype <= SOFTWARE) {
		/*change label in aboutContextMenuItem to relfect new context */
		sprintf(tmp, catgets(_catd, 8, 262, "About Managing %s..."), 
				object_names[objtype].uc_name);
		mtmp = XmStringCreateLocalized(tmp);
		XtVaSetValues(ctxt->aboutContextMenuItem,
			XmNlabelString, mtmp,
			NULL);
		XmStringFree(mtmp);	

		XtVaGetValues(ctxt->aboutContextMenuItem, 
                                XmNactivateCallback, &cbf, 
				NULL);
                if (cbf)
			XtRemoveAllCallbacks(ctxt->aboutContextMenuItem, 
				XmNactivateCallback);
		sprintf(tmp, "%s.t.hlp", object_names[objtype].lc_name);
		XtAddCallback(ctxt->aboutContextMenuItem, 
			XmNactivateCallback,
			(XtCallbackProc) helpCB, 
			tmp);
	}
	if (ctxt->currDialog)
		UxPopdownInterface(ctxt->currDialog);

}

/*
 *  Properties
 */
static void
adminCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	sysMgrMainCtxt *ctxt = (sysMgrMainCtxt*)cd;

	show_pkgadmindialog(sysmgrmain, ctxt);
}

static void
swViewCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	sw_view_t	view = (sw_view_t)cd;
	int		i;
	int		jj;

	free_objlist();

	if ((jj = build_sw_list(installed_sw_tree, 0, view, 0)) > 0) {
		entry_count = jj;
		objlist = (obj_data**)
			malloc(entry_count * sizeof(obj_data*));
		for (i=0; i<entry_count; i++) {
			objlist[i] = (obj_data*)malloc(sizeof(obj_data));
			objlist[i]->type = SOFTWARE;

			objlist[i]->obj.software.sw_type = sw_list[i].sw_type;
			objlist[i]->obj.software.sw_name = sw_list[i].sw_name;
			objlist[i]->obj.software.sw_id = sw_list[i].sw_id;
			objlist[i]->obj.software.version = sw_list[i].version;
			objlist[i]->obj.software.category = sw_list[i].category;
			objlist[i]->obj.software.vendor = sw_list[i].vendor;
			objlist[i]->obj.software.arch = sw_list[i].arch;
			objlist[i]->obj.software.date = sw_list[i].date;
			objlist[i]->obj.software.prodname = sw_list[i].prodname;
			objlist[i]->obj.software.prodvers = sw_list[i].prodvers;
			objlist[i]->obj.software.desc = sw_list[i].desc;
			objlist[i]->obj.software.basedir = sw_list[i].basedir;
			objlist[i]->obj.software.locale = sw_list[i].locale;
			objlist[i]->obj.software.size = sw_list[i].size;
			objlist[i]->obj.software.level = sw_list[i].level;
			objlist[i]->obj.software.install_reqs = 
						sw_list[i].install_reqs;
			objlist[i]->obj.software.instance = 
						sw_list[i].instance;
		}

		/* display list */
		display_host_list();

		set_menus_active(True);
		set_edit_functions_active(False);

	}
}

static void
ext_selectionCB(Widget w, XtPointer cd, XmListCallbackStruct *cbs)
{
	sysMgrMainCtxt *ctxt = (sysMgrMainCtxt*)cd;
	int index, ii;
	static int ip, mp;
	Boolean do_multiple = False;

	if (cbs->selected_item_count) {
		set_edit_functions_active(True);
	}
	else {
		set_edit_functions_active(False);
	}

	index = cbs->item_position-1;
	if (objlist[index]->type == SOFTWARE) {
		if (cbs->selection_type == XmINITIAL) {
			ii = ip = cbs->item_position;
			do_multiple = True;
		} else if (cbs->selection_type == XmMODIFICATION) {
			mp = cbs->item_position;
			ii = max(ip, mp);
			do_multiple = True;
		}
		if (do_multiple) {
			/* select all pkg's in a container */
			XtVaSetValues(ctxt->objectList,
				XmNselectionPolicy, XmMULTIPLE_SELECT,
				NULL );
			while (ii < entry_count && 
			    (objlist[ii++]->obj.software.level >
		             objlist[index]->obj.software.level)) {
				XmListSelectPos(ctxt->objectList, ii, False);
			}
			XtVaSetValues(ctxt->objectList,
				XmNselectionPolicy, XmEXTENDED_SELECT,
				NULL );
		}
	}
}

static void
selectionCB(
	Widget w, 
	XtPointer cd, 
	XmListCallbackStruct* cbs)
{
	sysMgrMainCtxt *ctxt = (sysMgrMainCtxt*)cd;
	int	index, ii;


	if (cbs->selected_item_count) {
		set_edit_functions_active(True);
	}
	else {
		set_edit_functions_active(False);
	}

	index = cbs->item_position-1;
	if (objlist[index]->type == SOFTWARE) {
		/* select all pkg's in a container */
		ii = index + 1;
		XtVaSetValues(ctxt->objectList,
			XmNselectionPolicy, XmMULTIPLE_SELECT,
			NULL );
		while (ii < entry_count &&
		       (objlist[ii++]->obj.software.level >
		        objlist[index]->obj.software.level)) {
			XmListSelectPos(ctxt->objectList, ii, True);
		}
		XtVaSetValues(ctxt->objectList,
			XmNselectionPolicy, XmEXTENDED_SELECT,
			NULL );
	}
	else if ((objlist[index]->type == SERIAL) &&
	         (objlist[index]->obj.serial.service_enabled == s_inactive)) {
		XtSetSensitive(ctxt->deleteMenuItem, False);
	}
	else if (((objlist[index]->type == USER) &&
		  (strcmp(objlist[index]->obj.user.username, "+") == 0)) ||
		 ((objlist[index]->type == GROUP) &&
		  (strcmp(objlist[index]->obj.group.groupname, "+") == 0)) ||
		 ((objlist[index]->type == HOST) &&
		  (strcmp(objlist[index]->obj.host.ipaddr, "+") == 0))) {

		/* Can't do anything with "+" entry */
		XtSetSensitive(ctxt->modifyMenuItem, False);
		XtSetSensitive(ctxt->deleteMenuItem, False);
	}
}

static void
detailsCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	sysMgrMainCtxt *ctxt = (sysMgrMainCtxt*)cd;
	int*		pos_list;
	int		pos_count;
	int		index;

	if (XmListGetSelectedPos(ctxt->objectList, &pos_list, &pos_count)) {
		index = pos_list[0]-1;
		XtFree((char*)pos_list);

		show_modifyswdialog(sysmgrmain, &objlist[index]->obj.software, ctxt);
	}
}

static void
doubleclickCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cb)
{
	sysMgrMainCtxt		* ctxt = (sysMgrMainCtxt *)cd;
	XmListCallbackStruct	*cbs = (XmListCallbackStruct *) cb;
	int		index;


	index = cbs->item_position-1;
	switch (objlist[index]->type) {
	    case USER:
		if (strcmp(objlist[index]->obj.user.username, "+") != 0) {
			show_modifyuserdialog(sysmgrmain,
				&objlist[index]->obj.user, ctxt);
		}
		else {
			XBell(XtDisplay(w), 0);
		}
		break;

	    case GROUP:
		if (strcmp(objlist[index]->obj.group.groupname, "+") != 0) {
			show_modifygroupdialog(sysmgrmain,
				&objlist[index]->obj.group, ctxt);
		}
		else {
			XBell(XtDisplay(w), 0);
		}
		break;

	    case HOST:
		if (strcmp(objlist[index]->obj.host.ipaddr, "+") != 0) {
			show_modifyhostdialog(sysmgrmain,
				&objlist[index]->obj.host, ctxt);
		}
		else {
			XBell(XtDisplay(w), 0);
		}
		break;

	    case PRINTER:
		show_modifyprinterdialog(sysmgrmain,
			&objlist[index]->obj.printer, ctxt);
		break;

	    case SERIAL:
		show_modifyserialdialog(sysmgrmain,
			&objlist[index]->obj.serial, ctxt);
		break;

	    case SOFTWARE:
		detailsCB(NULL, cd, NULL);
		break;
	}
}

int
compare_name (
	const void *	e1,
	const void *	e2)
{
	const char* s1;
	const char* s2;
	int	cmp;

	switch ((*((obj_data**)e1))->type) {
	    case USER:
		s1 = (const char*)(*((obj_data**)e1))->obj.user.username;
		s2 = (const char*)(*((obj_data**)e2))->obj.user.username;
		cmp = strcmp(s1, s2);
		break;

	    case GROUP:
		s1 = (const char*)(*((obj_data**)e1))->obj.group.groupname;
		s2 = (const char*)(*((obj_data**)e2))->obj.group.groupname;
		cmp = strcmp(s1, s2);
		break;

	    case HOST:
		s1 = (const char*)(*((obj_data**)e1))->obj.host.hostname;
		if (s1 == NULL) {
			s1 = (const char*)(*((obj_data**)e1))->obj.host.ipaddr;
		}

		s2 = (const char*)(*((obj_data**)e2))->obj.host.hostname;
		if (s2 == NULL) {
			s2 = (const char*)(*((obj_data**)e2))->obj.host.ipaddr;
		}

		cmp = strcmp(s1, s2);
		break;

	    case PRINTER:
		s1 = (const char*)(*((obj_data**)e1))->obj.printer.printername;
		s2 = (const char*)(*((obj_data**)e2))->obj.printer.printername;
		cmp = strcmp(s1, s2);
		break;

	    case SERIAL:
		s1 = (const char*)(*((obj_data**)e1))->obj.serial.port;
		s2 = (const char*)(*((obj_data**)e2))->obj.serial.port;
		cmp = strcmp(s1, s2);
		if (cmp == 0) {
			s1 = (const char*)(*((obj_data**)e1))->obj.serial.pmtag;
			s2 = (const char*)(*((obj_data**)e2))->obj.serial.pmtag;
			cmp = strcmp(s1, s2);
			if (cmp == 0) {
				s1 = (const char*)
				    (*((obj_data**)e1))->obj.serial.svctag;
				s2 = (const char*)
				    (*((obj_data**)e2))->obj.serial.svctag;
				cmp = strcmp(s1, s2);
			}
		}
		break;

	    case SOFTWARE:
		break;
	}

	return cmp;
}

static void
neverCB(void)
{
	/* This function is never called */
}

static char*
rstrtok(char* str, char sep)
{
	int len;
	char* pp;

	len = strlen(str);

	if (len < 1)
		return NULL;

	pp = &str[len - 1];

	while ((*pp != sep) && (pp > str))
		pp--;
	if (*pp == sep) {
		*pp = '\0';
		return str;
	}
	else {
		return NULL;
	}
}

/*
 * Helper function for set_column_widths()
 */
static void
set_col_width(
	int*		col_width,
	XmFontList	scrolling_list_fl,
	int		char_width,
	Widget		label)
{
	XmString	xstr;
	Dimension	field_width;
	Dimension	label_width;
	XmFontList	label_fontlist;
	char		buf[256];

	/*
	 *  If a column label is wider than its column,
	 *  make the column wider to match.
	 */

	sprintf(buf, "%*.*s", *col_width, *col_width, "x");
	xstr = XmStringCreateLocalized(buf);
	field_width = XmStringWidth(scrolling_list_fl, xstr);
	XmStringFree(xstr);

	XtVaGetValues(label,
		XmNlabelString, &xstr,
		XmNfontList, &label_fontlist,
		NULL);
	label_width = XmStringWidth(label_fontlist, xstr);

	if (label_width > field_width) {
		*col_width = (int)label_width / char_width;
	}
}

static void
set_column_widths(sysMgrMainCtxt* ctxt)
{
	XmFontList	fl;
	Dimension	chwid;
	XmString	xstr;

	XtVaGetValues(ctxt->objectList,
		XmNfontList, &fl,
		NULL);

	/* Assume scrolling list font is fixed-width */
	xstr = XmStringCreateLocalized("x");
	chwid = XmStringWidth(fl, xstr);
	XmStringFree(xstr);

	/* User */
	set_col_width(&username_cw, fl, chwid, ctxt->userLabel);
	set_col_width(&userid_cw, fl, chwid, ctxt->uidLabel);
	set_col_width(&usercmnt_cw, fl, chwid, ctxt->userCommentLabel);
	sprintf(user_fmt, "%%-%d.%ds    %%%d.%ds    %%-.%ds",
		username_cw, username_cw,
		userid_cw, userid_cw,
		usercmnt_cw);

	/* Group */
	set_col_width(&groupname_cw, fl, chwid, ctxt->groupLabel);
	set_col_width(&groupid_cw, fl, chwid, ctxt->gidLabel);
	set_col_width(&groupmembers_cw, fl, chwid, ctxt->groupMembersLabel);
	sprintf(group_fmt, "%%-%d.%ds    %%%d.%ds    %%-.%ds",
		groupname_cw, groupname_cw,
		groupid_cw, groupid_cw,
		groupmembers_cw);

	/* Host */
	set_col_width(&hostname_cw, fl, chwid, ctxt->hostLabel);
	set_col_width(&hostip_cw, fl, chwid, ctxt->hostIpLabel);
	sprintf(host_fmt, "%%-%d.%ds    %%-%d.%ds",
		hostname_cw, hostname_cw,
		hostip_cw, hostip_cw);

	/* Printer */
	set_col_width(&printername_cw, fl, chwid, ctxt->printerLabel);
	set_col_width(&printserver_cw, fl, chwid, ctxt->serverLabel);
	set_col_width(&printercmnt_cw, fl, chwid, ctxt->printerCommentLabel);
	sprintf(printer_fmt, "%%-%d.%ds    %%-%d.%ds    %%-.%ds",
		printername_cw, printername_cw,
		printserver_cw, printserver_cw,
		printercmnt_cw);

	/* Serial Port */
	set_col_width(&serial_cw, fl, chwid, ctxt->serialLabel);
	set_col_width(&service_cw, fl, chwid, ctxt->serviceLabel);
	set_col_width(&tag_cw, fl, chwid, ctxt->tagLabel);
	set_col_width(&serialcmnt_cw, fl, chwid, ctxt->serialCommentLabel);
	sprintf(serial_fmt, "%%-%d.%ds    %%-%d.%ds    %%-%d.%ds    %%-.%ds",
		serial_cw, serial_cw,
		service_cw, service_cw,
		tag_cw, tag_cw,
		serialcmnt_cw);
	sprintf(dis_serial_fmt, "%%-%d.%ds    %%-%d.%ds    %%s",
		serial_cw, serial_cw,
		service_cw, service_cw);

	/* Software */
	strcpy(sw_fmt, "%-70.70s  %5.5s");
}

static void
align_list_headers(sysMgrMainCtxt* ctxt)
{
	XmFontList	sl_fontlist, label_fontlist;
	Dimension	border, shadow, margin, indent;
	Dimension	col;
	Dimension	wid;
	XmString	xstr;
	char*		str;
	int		ii;
	Widget		lab[5];
	char		entry[256];

	XtVaGetValues(ctxt->objectList,
		XmNfontList, &sl_fontlist,
		XmNborderWidth, &border,
		XmNshadowThickness, &shadow,
		XmNlistMarginWidth, &margin,
		NULL);
	indent = border + shadow + margin;

	lab[0] = NULL;
	lab[1] = ctxt->userLabel;
	lab[2] = ctxt->uidLabel;
	lab[3] = ctxt->userCommentLabel;
	sprintf(entry, user_fmt, "x", "x", "x");
	for (ii=3; ii>=1; ii--) {
		if (rstrtok(entry, 'x') != NULL) {
			if (ii == 2) {
				/* right-align uid label */
				strcat(entry, "  ");
				xstr = XmStringCreateLocalized(entry);
				col = XmStringWidth(sl_fontlist, xstr);
				XmStringFree(xstr);
				XtVaGetValues(lab[ii],
					XmNwidth, &wid,
					NULL );
				XtVaSetValues(lab[ii],
					XmNleftOffset, indent + (col - wid),
					XmNalignment, XmALIGNMENT_END,
					NULL );
			}
			else {
				xstr = XmStringCreateLocalized(entry);
				col = XmStringWidth(sl_fontlist, xstr);
				XmStringFree(xstr);
				XtVaSetValues(lab[ii],
					XmNleftOffset, indent + col,
					NULL );
			}
		}
	}

	lab[0] = NULL;
	lab[1] = ctxt->groupLabel;
	lab[2] = ctxt->gidLabel;
	lab[3] = ctxt->groupMembersLabel;
	sprintf(entry, group_fmt, "x", "x", "x");
	for (ii=3; ii>=1; ii--) {
		if (rstrtok(entry, 'x') != NULL) {
			if (ii == 2) {
				/* right-align gid label */
				strcat(entry, "  ");
				xstr = XmStringCreateLocalized(entry);
				col = XmStringWidth(sl_fontlist, xstr);
				XmStringFree(xstr);
				XtVaGetValues(lab[ii],
					XmNwidth, &wid,
					NULL );
				XtVaSetValues(lab[ii],
					XmNleftOffset, indent + (col - wid),
					XmNalignment, XmALIGNMENT_END,
					NULL );
			}
			else {
				xstr = XmStringCreateLocalized(entry);
				col = XmStringWidth(sl_fontlist, xstr);
				XmStringFree(xstr);
				XtVaSetValues(lab[ii],
					XmNleftOffset, indent + col,
					NULL );
			}
		}
	}

	lab[0] = NULL;
	lab[1] = ctxt->hostLabel;
	lab[2] = ctxt->hostIpLabel;
	sprintf(entry, host_fmt, "x", "x");
	for (ii=2; ii>=1; ii--) {
		if (rstrtok(entry, 'x') != NULL) {
			xstr = XmStringCreateLocalized(entry);
			col = XmStringWidth(sl_fontlist, xstr);
			XmStringFree(xstr);
			XtVaSetValues(lab[ii],
				XmNleftOffset, indent + col,
				NULL );
		}
	}

	lab[0] = NULL;
	lab[1] = ctxt->printerLabel;
	lab[2] = ctxt->serverLabel;
	lab[3] = ctxt->printerCommentLabel;
	sprintf(entry, printer_fmt, "x", "x", "x");
	for (ii=3; ii>=1; ii--) {
		if (rstrtok(entry, 'x') != NULL) {
			xstr = XmStringCreateLocalized(entry);
			col = XmStringWidth(sl_fontlist, xstr);
			XmStringFree(xstr);
			XtVaSetValues(lab[ii],
				XmNleftOffset, indent + col,
				NULL );
		}
	}

	lab[0] = NULL;
	lab[1] = ctxt->serialLabel;
	lab[2] = ctxt->serviceLabel;
	lab[3] = ctxt->tagLabel;
	lab[4] = ctxt->serialCommentLabel;
	sprintf(entry, serial_fmt, "x", "x", "x", "x");
	for (ii=4; ii>=1; ii--) {
		if (rstrtok(entry, 'x') != NULL) {
			xstr = XmStringCreateLocalized(entry);
			col = XmStringWidth(sl_fontlist, xstr);
			XmStringFree(xstr);
			XtVaSetValues(lab[ii],
				XmNleftOffset, indent + col,
				NULL );
		}
	}

	lab[0] = NULL;
	lab[1] = NULL;
	lab[2] = ctxt->swSizeLabel;
	sprintf(entry, sw_fmt, "x", "x");
	for (ii=2; ii>=2; ii--) {
		if (rstrtok(entry, 'x') != NULL) {
			if (ii == 2) {
				/* right-align size label */
				strcat(entry, "  ");
				xstr = XmStringCreateLocalized(entry);
				col = XmStringWidth(sl_fontlist, xstr);
				XmStringFree(xstr);
				XtVaGetValues(lab[ii],
					XmNwidth, &wid,
					NULL );
				XtVaSetValues(lab[ii],
					XmNleftOffset, indent + 10 + (col - wid),
					XmNalignment, XmALIGNMENT_END,
					NULL );
			}
			else {
				xstr = XmStringCreateLocalized(entry);
				col = XmStringWidth(sl_fontlist, xstr);
				XmStringFree(xstr);
				XtVaSetValues(lab[ii],
					XmNleftOffset, indent + col + 10,
					NULL );
			}
		}
	}
}

Widget	build_mainwin()
{
	sysMgrMainCtxt	*ctxt;
	Widget		menubar;
	Widget		pulldown;
	Widget		menushell;
	Widget		w, rtn;
	Widget		headerForm;
	Widget		addPulldown;
	Atom		wmdelete;
	Arg		args[3];
	XtCallbackRec	cb[2];
	Dimension	height;


	ctxt = (sysMgrMainCtxt *) malloc(sizeof(sysMgrMainCtxt));
	memset((void *)ctxt, 0, sizeof (sysMgrMainCtxt));

	rtn = XtVaCreatePopupShell( "SysMgrMain",
			topLevelShellWidgetClass, GtopLevel,
			XmNtitle, "Admintool",
			XmNdeleteResponse, XmDO_NOTHING,
			/* XmNallowShellResize, False, */
			XmNx, 300,
			XmNy, 200,
			NULL );

	/* add protocol callback for window manager quitting the shell */
	wmdelete = XmInternAtom(XtDisplay(rtn), "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(rtn, wmdelete, (XtCallbackProc)exitCB, NULL);

	ctxt->mainWindow = XtVaCreateManagedWidget( "mainWindow",
			xmMainWindowWidgetClass, rtn,
			XmNunitType, XmPIXELS,
			NULL );

	XtVaSetValues(ctxt->mainWindow,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	/* Menu Bar */
	XtSetArg(args[0], XmNmenuAccelerator, "<KeyUp>F10");
	menubar = XmCreateMenuBar(ctxt->mainWindow, "menubar", args, 1);

	/* File menu */
	pulldown = XmCreatePulldownMenu(menubar, "_pulldown", NULL, 0);
	ctxt->fileMenu = XtVaCreateManagedWidget("File",
		xmCascadeButtonWidgetClass, menubar,
		XmNsubMenuId, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 600, "File")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 601, "F")),
		NULL);
	ctxt->exitMenuItem = XtVaCreateManagedWidget("Exit",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 602, "Exit")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 603, "x")),
		NULL);
	XtAddCallback( ctxt->exitMenuItem, XmNactivateCallback,
		(XtCallbackProc) exitCB,
		(XtPointer) ctxt );

	/* Edit menu */
	pulldown = XmCreatePulldownMenu(menubar, "_pulldown", NULL, 0);
	ctxt->editMenu = XtVaCreateManagedWidget("Edit",
		xmCascadeButtonWidgetClass, menubar,
		XmNsubMenuId, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 604, "Edit")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 605, "E")),
		NULL);
	ctxt->addMenuItem = XtVaCreateManagedWidget("Add",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 606, "Add...")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 607, "A")),
		NULL);

	addPulldown = XmCreatePulldownMenu(pulldown, "", NULL, 0);
	ctxt->addMenu = XtVaCreateWidget("add",
		xmCascadeButtonWidgetClass, pulldown,
		XmNsubMenuId, addPulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 608, "Add")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 609, "A")),
		NULL);
	ctxt->localPrinterMenuItem = XtVaCreateManagedWidget( "local",
		xmPushButtonGadgetClass, addPulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 610, "Local Printer...")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 611, "L")),
		NULL);
	XtAddCallback( ctxt->localPrinterMenuItem, XmNactivateCallback,
		(XtCallbackProc) addCB,
		(XtPointer) ctxt );
	ctxt->remotePrinterMenuItem = XtVaCreateManagedWidget( "remote",
		xmPushButtonGadgetClass, addPulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 612, "Access to Printer...")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 613, "A")),
		NULL);
	XtAddCallback( ctxt->remotePrinterMenuItem, XmNactivateCallback,
		(XtCallbackProc) addCB,
		(XtPointer) ctxt );

	ctxt->modifyMenuItem = XtVaCreateManagedWidget("Modify",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 614, "Modify...")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 615, "M")),
		NULL);
	w = XtVaCreateManagedWidget("",
		xmSeparatorGadgetClass, pulldown,
		NULL);
	ctxt->deleteMenuItem = XtVaCreateManagedWidget("Delete",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 616, "Delete")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 617, "D")),
		NULL);
	XtAddCallback( ctxt->addMenuItem, XmNactivateCallback,
		(XtCallbackProc) addCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->modifyMenuItem, XmNactivateCallback,
		(XtCallbackProc) modifyCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->deleteMenuItem, XmNactivateCallback,
		(XtCallbackProc) deleteCB,
		(XtPointer) ctxt );

	/* Browse menu */
	cb[0].callback = (XtCallbackProc)manageCB;
	cb[0].closure = (XtPointer)ctxt;
	cb[1].callback = NULL;
	XtSetArg(args[0], XmNentryCallback, cb);
	XtSetArg(args[1], XmNradioBehavior, True);
	XtSetArg(args[2], XmNradioAlwaysOne, True);
	pulldown = XmCreatePulldownMenu(menubar, "_pulldown", args, 3);
	ctxt->manageMenu = XtVaCreateManagedWidget("Manage",
		xmCascadeButtonWidgetClass, menubar,
		XmNsubMenuId, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 618, "Browse")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 619, "B")),
		NULL);
	ctxt->usersMenuItem = XtVaCreateManagedWidget( "Users",
		xmToggleButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 620, "Users")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 621, "U")),
		NULL);
	XtAddCallback(ctxt->usersMenuItem, XmNvalueChangedCallback,
		(XtCallbackProc) neverCB,
		(XtPointer) USER );
	ctxt->groupsMenuItem = XtVaCreateManagedWidget( "Groups",
		xmToggleButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 622, "Groups")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 623, "G")),
		NULL);
	XtAddCallback(ctxt->groupsMenuItem, XmNvalueChangedCallback,
		(XtCallbackProc) neverCB,
		(XtPointer) GROUP );
	ctxt->hostsMenuItem = XtVaCreateManagedWidget( "Hosts",
		xmToggleButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 624, "Hosts")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 625, "H")),
		NULL);
	XtAddCallback(ctxt->hostsMenuItem, XmNvalueChangedCallback,
		(XtCallbackProc) neverCB,
		(XtPointer) HOST );
	ctxt->printersMenuItem = XtVaCreateManagedWidget( "Printers",
		xmToggleButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 626, "Printers")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 627, "P")),
		NULL);
	XtAddCallback(ctxt->printersMenuItem, XmNvalueChangedCallback,
		(XtCallbackProc) neverCB,
		(XtPointer) PRINTER );
	ctxt->portsMenuItem = XtVaCreateManagedWidget( "Serial Ports",
		xmToggleButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 628, "Serial Ports")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 629, "e")),
		NULL);
	XtAddCallback(ctxt->portsMenuItem, XmNvalueChangedCallback,
		(XtCallbackProc) neverCB,
		(XtPointer) SERIAL );
	ctxt->softwareMenuItem = XtVaCreateManagedWidget( "Software",
		xmToggleButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 630, "Software")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 631, "S")),
		NULL);
	XtAddCallback(ctxt->softwareMenuItem, XmNvalueChangedCallback,
		(XtCallbackProc) neverCB,
		(XtPointer) SOFTWARE );

	/* Properties menu */
	pulldown = XmCreatePulldownMenu(menubar, "_pulldown", NULL, 0);
	ctxt->propMenu = XtVaCreateManagedWidget("Properties",
		xmCascadeButtonWidgetClass, menubar,
		XmNsubMenuId, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 700, "Properties")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 701, "P")),
		NULL);
	ctxt->adminMenuItem = XtVaCreateManagedWidget("AdminFile",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 702, "Package Administration")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 703, "A")),
		NULL);
	XtAddCallback( ctxt->adminMenuItem, XmNactivateCallback,
		(XtCallbackProc) adminCB,
		(XtPointer) ctxt );

	/* Help menu */
	pulldown = XmCreatePulldownMenu(menubar, "_pulldown", NULL, 0);
	ctxt->helpMenu = XtVaCreateManagedWidget("Help",
		xmCascadeButtonWidgetClass, menubar,
		XmNsubMenuId, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 632, "Help")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 633, "H")),
		NULL);
	ctxt->aboutMenuItem = XtVaCreateManagedWidget( "About",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 634, "About Admintool...")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 635, "A")),
		NULL);

	ctxt->aboutContextMenuItem = XtVaCreateManagedWidget( "About",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 636, "About Managing Users...")),
		RSC_CVT(XmNmnemonic, catgets(_catd, 8, 637, "M")),
		NULL);

	XtAddCallback(ctxt->aboutMenuItem, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"admintool.t.hlp" );

	XtAddCallback(ctxt->aboutContextMenuItem, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"user.t.hlp" );

	XtVaSetValues(menubar,
		XmNmenuHelpWidget, ctxt->helpMenu,
		NULL );

	XtManageChild(menubar);


	ctxt->workForm = XtVaCreateWidget( "workForm",
		xmFormWidgetClass,
		ctxt->mainWindow,
		NULL );

	ctxt->listForm = XtVaCreateWidget( "listForm",
		xmFormWidgetClass,
		ctxt->workForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
/*	
		XmNresizable, False,
*/
		NULL );

	headerForm = XtVaCreateManagedWidget( "headerForm",
		xmFormWidgetClass,
		ctxt->listForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL );

	ctxt->userHeader = XtVaCreateManagedWidget( "userHeader",
		xmFormWidgetClass,
		headerForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftOffset, 10,
		NULL );

	ctxt->userLabel = XtVaCreateManagedWidget( "user",
		xmLabelGadgetClass,
		ctxt->userHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 283, "User Name")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL );

	ctxt->uidLabel = XtVaCreateManagedWidget( "uid",
		xmLabelGadgetClass,
		ctxt->userHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 284, "User ID")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 100,
		NULL );

	ctxt->userCommentLabel = XtVaCreateManagedWidget( "comment",
		xmLabelGadgetClass,
		ctxt->userHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 285, "Comment")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 200,
		NULL );

	ctxt->groupHeader = XtVaCreateManagedWidget( "groupHeader",
		xmFormWidgetClass,
		headerForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftOffset, 10,
		NULL );

	ctxt->groupLabel = XtVaCreateManagedWidget( "group",
		xmLabelGadgetClass,
		ctxt->groupHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 286, "Group Name")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL );

	ctxt->gidLabel = XtVaCreateManagedWidget( "gid",
		xmLabelGadgetClass,
		ctxt->groupHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 287, "Group ID")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 100,
		NULL );

	ctxt->groupMembersLabel = XtVaCreateManagedWidget( "members",
		xmLabelGadgetClass,
		ctxt->groupHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 288, "Members")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 200,
		NULL );

	ctxt->hostHeader = XtVaCreateManagedWidget( "hostHeader",
		xmFormWidgetClass,
		headerForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftOffset, 10,
		NULL );

	ctxt->hostLabel = XtVaCreateManagedWidget( "host",
		xmLabelGadgetClass,
		ctxt->hostHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 289, "Host Name")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL );

	ctxt->hostIpLabel = XtVaCreateManagedWidget( "ip",
		xmLabelGadgetClass,
		ctxt->hostHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 290, "IP Address")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 100,
		NULL );

	ctxt->printerHeader = XtVaCreateWidget( "printerHeader",
		xmFormWidgetClass,
		headerForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftOffset, 10,
		NULL );

	ctxt->printerLabel = XtVaCreateManagedWidget( "printer",
		xmLabelGadgetClass,
		ctxt->printerHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 291, "Printer Name")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL );

	ctxt->serverLabel = XtVaCreateManagedWidget( "server",
		xmLabelGadgetClass,
		ctxt->printerHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 292, "Server")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 100,
		NULL );

	ctxt->printerCommentLabel = XtVaCreateManagedWidget( "comment",
		xmLabelGadgetClass,
		ctxt->printerHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 293, "Description")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 200,
		NULL );

	ctxt->serialHeader = XtVaCreateWidget( "serialHeader",
		xmFormWidgetClass,
		headerForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftOffset, 10,
		NULL );

	ctxt->serialLabel = XtVaCreateManagedWidget( "serial",
		xmLabelGadgetClass,
		ctxt->serialHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 294, "Port")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL );

	ctxt->serviceLabel = XtVaCreateManagedWidget( "pmtag",
		xmLabelGadgetClass,
		ctxt->serialHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 295, "Port Monitor")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 100,
		NULL );

	ctxt->tagLabel = XtVaCreateManagedWidget( "svctag",
		xmLabelGadgetClass,
		ctxt->serialHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 296, "Service Tag")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 200,
		NULL );

	ctxt->serialCommentLabel = XtVaCreateManagedWidget( "comment",
		xmLabelGadgetClass,
		ctxt->serialHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 297, "Comment")),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 300,
		NULL );

	ctxt->softwareHeader = XtVaCreateWidget( "softwareHeader",
		xmFormWidgetClass,
		headerForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopOffset, 5,
		NULL );

	menushell = XtVaCreatePopupShell("menushell",
		xmMenuShellWidgetClass, ctxt->softwareHeader,
		XmNwidth, 1,
		XmNheight, 1,
		XmNallowShellResize, TRUE,
		XmNoverrideRedirect, TRUE,
		NULL );

	pulldown = XtVaCreateWidget( "pulldown",
		xmRowColumnWidgetClass,
		menushell,
		XmNrowColumnType, XmMENU_PULLDOWN,
		NULL );

	w = XtVaCreateManagedWidget( "all",
		xmPushButtonGadgetClass,
		pulldown,
		RSC_CVT( XmNlabelString, catgets(_catd, 8, 298, "All Software") ),
		NULL );
	XtAddCallback( w, XmNactivateCallback,
		(XtCallbackProc) swViewCB,
		(XtPointer)sw_view_all);

	w = XtVaCreateManagedWidget( "system",
		xmPushButtonGadgetClass,
		pulldown,
		RSC_CVT( XmNlabelString, catgets(_catd, 8, 299, "System Software") ),
		NULL );
	XtAddCallback( w, XmNactivateCallback,
		(XtCallbackProc) swViewCB,
		(XtPointer)sw_view_sys);

	w = XtVaCreateManagedWidget( "app",
		xmPushButtonGadgetClass,
		pulldown,
		RSC_CVT( XmNlabelString, catgets(_catd, 8, 300, "Application Software") ),
		NULL );
	XtAddCallback( w, XmNactivateCallback,
		(XtCallbackProc) swViewCB,
		(XtPointer)sw_view_app);

	ctxt->swViewOptionMenu = XtVaCreateManagedWidget( "sw_view",
		xmRowColumnWidgetClass,
		ctxt->softwareHeader,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RSC_CVT( XmNlabelString, "" ),
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL );

	ctxt->swSizeLabel = XtVaCreateManagedWidget( "swsize",
		xmLabelGadgetClass,
		ctxt->softwareHeader,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 301, "Size (MB)")),
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 500,
		NULL );

	ctxt->listScrollWin = XtVaCreateManagedWidget( "listScrollWin",
		xmScrolledWindowWidgetClass,
		ctxt->listForm,
		XmNwidth, 525,
		XmNresizable, False,
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNshadowThickness, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, headerForm,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 10,
		NULL );
	ctxt->objectList = XtVaCreateManagedWidget( "objectList",
		xmListWidgetClass,
		ctxt->listScrollWin,
		XmNvisibleItemCount, 12,
		XmNlistSizePolicy, XmCONSTANT,
		XmNresizable, False,
		XmNselectionPolicy, XmEXTENDED_SELECT,
		NULL );
	XtAddCallback( ctxt->objectList, XmNextendedSelectionCallback,
		(XtCallbackProc) ext_selectionCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->objectList, XmNbrowseSelectionCallback,
		(XtCallbackProc) selectionCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->objectList, XmNdefaultActionCallback,
		(XtCallbackProc) doubleclickCB,
		(XtPointer) ctxt );

	ctxt->defPrinterLabel = XtVaCreateWidget( "default",
		xmLabelGadgetClass,
		ctxt->listForm,
		RSC_CVT(XmNlabelString, " "),
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, LABEL_BOTTOM_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 50,
		NULL );

	ctxt->detailsButton = XtVaCreateWidget( "details",
		xmPushButtonGadgetClass,
		ctxt->listForm,
		RSC_CVT(XmNlabelString, catgets(_catd, 8, 302, "Show Details...")),
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		XmNsensitive, False,
		NULL);
	XtAddCallback(ctxt->detailsButton, XmNactivateCallback,
		(XtCallbackProc) detailsCB,
		(XtPointer) ctxt );

	ctxt->currHostLabel = XtVaCreateManagedWidget( "host",
		xmLabelGadgetClass,
		ctxt->listForm,
		RSC_CVT(XmNlabelString, " "),
		XmNalignment, XmALIGNMENT_END,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 10,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 50,
		NULL );

	XtVaSetValues(ctxt->listScrollWin,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, ctxt->currHostLabel,
		XmNbottomOffset, LABEL_BOTTOM_OFFSET,
		NULL );

	set_column_widths(ctxt);
	align_list_headers(ctxt);

	XtManageChild(ctxt->listForm);
	XtManageChild(ctxt->workForm);

	/* shouldn't have to, but set height of header */
	XtVaGetValues(ctxt->userLabel,
		XmNheight, &height,
		NULL );

	XtAddCallback( rtn, XmNdestroyCallback,
		(XtCallbackProc) exitCB,
		(XtPointer) ctxt);

	XmMainWindowSetAreas(ctxt->mainWindow, menubar, (Widget) NULL,
			(Widget) NULL, (Widget) NULL, ctxt->workForm);


	return (ctxt->mainWindow);
}

static char*
get_display_string(obj_data* od)
{
	static char	displayString[1024];
	char		tmp[1024];
	char* 		vers;
	char		size[16];
	int		i, len;

	switch (od->type) {
	    case USER:
		sprintf(displayString, user_fmt,
			od->obj.user.username ? od->obj.user.username : "",
			od->obj.user.uid ? od->obj.user.uid : "",
			od->obj.user.comment ? od->obj.user.comment : "");
		break;

	    case GROUP:
		sprintf(displayString, group_fmt,
			od->obj.group.groupname ? od->obj.group.groupname : "",
			od->obj.group.gid ? od->obj.group.gid : "",
			od->obj.group.members ? od->obj.group.members : "");
		break;

	    case HOST:
		sprintf(displayString, host_fmt,
			od->obj.host.hostname ? od->obj.host.hostname : "",
			od->obj.host.ipaddr ? od->obj.host.ipaddr : "");
		break;

	    case PRINTER:
		sprintf(displayString, printer_fmt,
			od->obj.printer.printername ? od->obj.printer.printername : "",
			od->obj.printer.printserver ? od->obj.printer.printserver : "",
			od->obj.printer.comment ? od->obj.printer.comment : "");
		break;

	    case SERIAL:
		switch (od->obj.serial.service_enabled) {
		    case s_inactive:
			sprintf(displayString, dis_serial_fmt,
				od->obj.serial.port ? od->obj.serial.port : "",
				"",
				catgets(_catd, 8, 303, "< no service >"));
			break;
		    case s_disabled:
			sprintf(displayString, serial_fmt,
				od->obj.serial.port ? od->obj.serial.port : "",
				od->obj.serial.pmtag ? od->obj.serial.pmtag : "",
				od->obj.serial.svctag ? od->obj.serial.svctag : "",
				catgets(_catd, 8, 304, "< port service disabled >"));
			break;
		    case s_enabled:
			sprintf(displayString, serial_fmt,
				od->obj.serial.port ? od->obj.serial.port : "",
				od->obj.serial.pmtag ? od->obj.serial.pmtag : "",
				od->obj.serial.svctag ? od->obj.serial.svctag : "",
				od->obj.serial.comment ? od->obj.serial.comment : "");
			break;
		}
		break;

	    case SOFTWARE:
    		tmp[0] = '\0';
		for (i=0; i < od->obj.software.level; i++)
			strcat(tmp, "    ");
		len = strlen(tmp);
		if (od->obj.software.sw_name != NULL) {
		    strcat(tmp, od->obj.software.sw_name);
		    strcat(tmp, " ");
  	            vers = (char*) (od->obj.software.sw_type == PACKAGE ? 
			od->obj.software.prodvers : od->obj.software.version);
	            strcat(tmp, vers);
		    tmp[len + strlen(od->obj.software.sw_name) + strlen(vers) + 2] = '\0';
		    if (od->obj.software.sw_type == PACKAGE &&
	                od->obj.software.instance &&
			  strcmp(od->obj.software.sw_id, 
				od->obj.software.instance)) {
			strcat(tmp, " [");
			strcat(tmp, od->obj.software.instance);
			strcat(tmp, "]");
		    }
		}
		if (*od->obj.software.locale != NULL) {
		    len = strlen(tmp);
		    sprintf(&tmp[len], " (%s %s)",
			    od->obj.software.locale, catgets(_catd, 8, 652, "Localization"));
		}

		if (od->obj.software.size > 0) {
			sprintf(size, "%d", od->obj.software.size);
		}
		else {
			sprintf(size, catgets(_catd, 8, 305, "<1"));
		}

		sprintf(displayString, sw_fmt,
			tmp,
			size);
		break;
	}

	return(displayString);
}

void
display_host_list(void)
{
	XmString* 	xstrtable;
	char*		s;
	int		i;
	sysMgrMainCtxt* ctxt;


	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	SetBusyPointer(True);

	/* Build list of XmStrings from objlist */
	xstrtable = (XmString*)malloc(entry_count * sizeof(XmString));
	for (i=0; i<entry_count; i++) {
		s = get_display_string(objlist[i]);
		xstrtable[i] = XmStringCreateLocalized(s);
	}

	XmListDeleteAllItems(ctxt->objectList);

	XmListAddItems(ctxt->objectList, xstrtable, entry_count, 1);
	for (i=0; i<entry_count; i++) {
		XmStringFree(xstrtable[i]);
	}
	free_mem(xstrtable);

	SetBusyPointer(False);
}

void
free_objlist(void)
{
	int i;

	if (objlist != NULL) {
		for (i=0; i<entry_count; i++) {
			switch (objlist[i]->type) {
			    case USER:
				free_user(&objlist[i]->obj.user);
				break;
		
			    case GROUP:
				free_group(&objlist[i]->obj.group);
				break;
		
			    case HOST:
				free_host(&objlist[i]->obj.host);
				break;
		
			    case PRINTER:
				free_printer(&objlist[i]->obj.printer);
				break;
		
			    case SERIAL:
				free_serial(&objlist[i]->obj.serial);
				break;
		
			    case SOFTWARE:
				free_software(&objlist[i]->obj.software);
				break;
			}
			free_mem(objlist[i]);
		}
		free_mem(objlist);
		objlist = NULL;
	}
}

void
init_mainwin(void)
{
	sysMgrMainCtxt*		ctxt;
	XmString	xstr;
	char		buf[128];
	Widget		wgt;

	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	switch (initial_ctxt) {
	  case ctxt_user:
	    wgt = ctxt->usersMenuItem;
	    break;
	  case ctxt_group:
	    wgt = ctxt->groupsMenuItem;
	    break;
	  case ctxt_host:
	    wgt = ctxt->hostsMenuItem;
	    break;
	  case ctxt_printer:
	    wgt = ctxt->printersMenuItem;
	    break;
	  case ctxt_serial:
	    wgt = ctxt->portsMenuItem;
	    break;
	  case ctxt_sw:
	    wgt = ctxt->softwareMenuItem;
	    break;
	  default:
	    wgt = ctxt->usersMenuItem;
	}
	XmToggleButtonGadgetSetState(wgt, True, True);

	if (!show_browse_menu) {
		XtUnmanageChild(ctxt->manageMenu);
	}

	sprintf(buf, catgets(_catd, 8, 306, "Host: %s"), localhost);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->currHostLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

}

static void
init_sw_list_table() {

	sw_list = (SWStruct *)malloc(sizeof(SWStruct) * SW_LIST_PGSZ);
	sw_list_cnt = 0;
	sw_list_nentries = SW_LIST_PGSZ;
	if (sw_list == NULL)
		fatal("init_sw_list_table: can't malloc");
}

static void
adjust_sw_list_table() {
	SWStruct * s;

	s = (SWStruct *)realloc((char *)sw_list, 
			  sizeof(SWStruct) * (sw_list_nentries+SW_LIST_PGSZ));
	if (s == NULL)
		fatal("adjust_sw_list_table: can't malloc");
   	sw_list_nentries += SW_LIST_PGSZ;
	sw_list = s;
}

static void
init_sw_list_entry(SWStruct* s, int lev, ModType type, Modinfo *mi)
{

    s->sw_type = type;
    s->sw_name = strdup(mi->m_name);
    s->sw_id = strdup(mi->m_pkgid);
    s->version = strdup(
	mi->m_version ? mi->m_version : "");
    s->desc = strdup(
	mi->m_desc ? mi->m_desc : "");
    s->category = strdup(
	mi->m_category ? mi->m_category : "");
    s->arch = strdup(
	mi->m_arch ? mi->m_arch : "");
    s->date = strdup(
	mi->m_instdate ? mi->m_instdate : "");
    s->vendor = strdup(
	mi->m_vendor ? mi->m_vendor : "");
    s->prodname = strdup(
	mi->m_prodname ? mi->m_prodname : "");
    s->prodvers = strdup(
	mi->m_prodvers ? mi->m_prodvers : "");
    s->basedir = strdup(
	mi->m_basedir ? mi->m_basedir : "");
    s->locale = strdup(
	mi->m_locale ? mi->m_locale : "");
    s->instance = strdup(mi->m_pkginst ? mi->m_pkginst : ""); 
    /* s->instance = NULL; */
    s->size = 0;
    s->level = lev;

}

/*
 * Compute size for an SWStruct entry in software list.
 * Only one of mtmp or mi should be non-NULL. Non-null
 * value determines which technique to use for computation
 * of size. 
 */
static void
calc_sw_list_entry_size(SWStruct* s, Module* mtmp, Modinfo* mi)
{
    int i;
    FSspace **space = NULL;
    int   size = 0;
    int	  sp_cnt = 0;

    if (mi) {
	s->size = pkg_size(mi) / 1024;
    } else {
        space = calc_cluster_space(mtmp, UNSELECTED);
        size = 0;
        sp_cnt = 0;
        for (i = 0; space[i] != NULL && i < MAX_SPACE_FS; i++) {
	    size += space[i]->fsp_reqd_contents_space;
        }
        s->size = size / 1024;
    }

/* allocate an extra struct to provide 0 size for termination */
    s->install_reqs = 
	(InstallSizeReqs *) malloc(sizeof(InstallSizeReqs) *
		(N_INSTALL_FS+1));

    if (s->install_reqs) {
	memset(s->install_reqs, 0, 
		sizeof(InstallSizeReqs) * (N_INSTALL_FS+1));
	i = 0;
	while (installFileSystems[i]) {
	    s->install_reqs[i].mountp = 
			strdup(installFileSystems[i]);
	    s->install_reqs[i].size = 
			mi ? get_pkg_fs_space(mi, installFileSystems[i]) :
			     get_fs_space(space, installFileSystems[i]) / 1024;
	    i++;
	}
    }
}

static int
add_global_localization_to_sw_list(Node * n, caddr_t ix)
{
    int index;
    Modinfo * mi = (Modinfo *)n->data;

    index = *(int*)ix;

    if (mi->m_locale && ((mi->m_l10n_pkglist == NULL) ||
		         (mi->m_l10n_pkglist[0] == '\0'))) {
        init_sw_list_entry(&sw_list[index], 0, PACKAGE, mi);
        calc_sw_list_entry_size(&sw_list[index], NULL, mi);

        index++;
        if (index == sw_list_nentries)	
	    adjust_sw_list_table();
    }
    *(int*)ix = index;
}


static int
build_sw_list(Module *m, int lev, sw_view_t view, int *stpt)
{
  extern L10N * getL10Ns(Module *);
  Module * mtmp = m;
  Node* n;
  Modinfo *mi;
  ulong size;
  int count;
  int i, cnt;
  char* cat;
  int   viewable;
  int  jj = 0;

  if (m == NULL)
	return(sw_list_cnt = 0);

  if (sw_list == NULL)
	init_sw_list_table();

  if (stpt)
     jj = *stpt;

  /* List those pkgs which localize entire installation first... */
  if (mtmp->type == PRODUCT || mtmp->type == NULLPRODUCT) {
      /* This path will only be traversed first time build_sw_list is called */
      walklist(mtmp->info.prod->p_packages, 
		add_global_localization_to_sw_list, (caddr_t)&jj);

      /* First meta/cluster/pkg hangs off of {NULL}PRODUCT module in 'sub' field */
      mtmp = get_sub(mtmp);
  }

  do {
    count = jj;

    if ((mtmp->type == NULLPRODUCT) ||
    	    (mtmp->type == NULLPRODUCT)) {
        /* This branch will NEVER be taken */
        /*  
            sw_list[jj].sw_type = mtmp->type;
	    sw_list[jj].sw_name = strdup(mtmp->info.prod->p_name);
	    sw_list[jj].sw_id = strdup("");
	    sw_list[jj].size = 0;
	    sw_list[jj].level = lev;

	    size = calc_tot_space((Product *)(mtmp->info.prod));
	    sw_list[jj].size = size / 1024;

	    jj++;
	    if (jj == sw_list_nentries)	
		adjust_sw_list_table();
         */

    }
    else {
	if (mtmp->info.mod->m_shared == NULLPKG)
		continue;

	viewable = 0;
	cat = mtmp->info.mod->m_category;
	switch (view) {
	    case sw_view_all:
		viewable = 1;
		break;

	    case sw_view_sys:
		if (cat == NULL || (cat && strncmp(cat, "sys", 3) == 0))
			viewable = 1;
		break;

	    case sw_view_app:
		if (cat == NULL || (cat && strncmp(cat, "app", 3) == 0))
			viewable = 1;
		break;
	}

	if (viewable) {
 	    init_sw_list_entry(&sw_list[jj], lev, mtmp->type, mtmp->info.mod);
            /* This is a bit of an optimization to avoid resursive calc of space */
            if (mtmp->type == PACKAGE)
	        calc_sw_list_entry_size(&sw_list[jj], NULL, mtmp->info.mod);
  	    else
	        calc_sw_list_entry_size(&sw_list[jj], mtmp, NULL);

            jj++;
            if (jj == sw_list_nentries)
	        adjust_sw_list_table();

	    /* Display any pkg instances */
            mi = mtmp->info.mod;
            while (n = mi->m_instances) {
                 mi = (Modinfo*)(n->data);

 	         init_sw_list_entry(&sw_list[jj], lev, mtmp->type, mi);
	         calc_sw_list_entry_size(&sw_list[jj], NULL, mi);
		 jj++;
                 if (jj == sw_list_nentries)
	             adjust_sw_list_table();
	    }
          
            if (mtmp->type == PACKAGE && is_localized(mtmp)) {
		char* last_loc = "";
		L10N* l10n = getL10Ns(mtmp);
		while (l10n) {
		    Modinfo* mi = l10n->l10n_package;
		    if (strcmp(mi->m_locale, last_loc) != 0) {
                        init_sw_list_entry(&sw_list[jj], lev, PACKAGE, mi);
	    		calc_sw_list_entry_size(&sw_list[jj], NULL, mi);
		    }

 		    l10n = l10n->l10n_next;
               	    jj++;
            	    if (jj == sw_list_nentries)
	         	adjust_sw_list_table();
		} 
	    }
        }
    }

    if (mtmp->type == CLUSTER || mtmp->type == METACLUSTER 
            /* We'll never see a PRODUCT or NULLPRODUCT at this point
            || mtmp->type == NULLPRODUCT || 
            mtmp->type == PRODUCT 
           */ ) {

	if (jj == count + 1) {
	    build_sw_list(get_sub(mtmp), lev+1, view, &jj);

	    if (jj == (count + 1))
		jj--;
	}

    }

  } while ((mtmp = get_next(mtmp)));

  if (stpt)
     *stpt = jj;
  return(sw_list_cnt = jj);
}

int
load_list(int type)
{
	sysMgrMainCtxt*		ctxt;
	SysmanUserArg*		userlist;
	SysmanGroupArg*		grouplist;
	SysmanHostArg*		hostlist;
	SysmanPrinterArg*	printerlist;
	SysmanSerialArg*	seriallist;
	int			listnum = 0;
	XmString		xstr;
	int			i;
	int			rtn;


	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	SetBusyPointer(True);

	free_objlist();

	switch (type) {
	    case USER:
	 	listnum = sysman_list_user(&userlist, errbuf, ERRBUF_SIZE);
		break;

	    case GROUP:
		listnum = sysman_list_group(&grouplist, errbuf, ERRBUF_SIZE);
		break;

	    case HOST:
	 	listnum = sysman_list_host(&hostlist, errbuf, ERRBUF_SIZE);
		break;

	    case PRINTER:
	 	listnum = sysman_list_printer(&printerlist, errbuf, ERRBUF_SIZE);
		break;

	    case SERIAL:
	 	listnum = sysman_list_serial(&seriallist, errbuf, ERRBUF_SIZE);
		break;

	    case SOFTWARE:
		installed_sw_tree = sysman_list_sw(NULL);

#ifdef METER
		/* Set up FSspace for installed filesytems */
		(void)installed_fs_layout();
#endif

		sw_list_cnt=0;
		if (installed_sw_tree == NULL) {
			/* This is a serious error and prevents
			 * further use of software piece of tool.
 	 		 * This occurs when installation database
			 * is bad or OS is incompatalbe with admintool,
			 * namely, < 2.5
			 */
			XtVaSetValues(ctxt->softwareMenuItem, 
				XmNsensitive, False, NULL);
			/* set listnum to -1 to invoke display_error below */	
			listnum = -1;
			/* build error message */
			sprintf(errbuf, "%s", BAD_SW_LIB_MSG);		
		}
		else {
			listnum = build_sw_list(installed_sw_tree, 0, sw_view_all,0);
		}
		break;
	}

	if (listnum > 0) {
		entry_count = listnum;
		objlist = (obj_data**)
			malloc(entry_count * sizeof(obj_data*));
		for (i=0; i<listnum; i++) {
			objlist[i] = (obj_data*)malloc(sizeof(obj_data));
			objlist[i]->type = type;

			switch (type) {
			    case USER:
				copy_user(&objlist[i]->obj.user, &userlist[i]);
				break;
		
			    case GROUP:
				copy_group(&objlist[i]->obj.group, &grouplist[i]);
				break;
		
			    case HOST:
				copy_host(&objlist[i]->obj.host, &hostlist[i]);
				break;
		
			    case PRINTER:
				copy_printer(&objlist[i]->obj.printer, &printerlist[i]);
				break;
		
			    case SERIAL:
				copy_serial(&objlist[i]->obj.serial, &seriallist[i]);
				break;
		
			    case SOFTWARE:
				objlist[i]->obj.software.sw_type = sw_list[i].sw_type;
				objlist[i]->obj.software.sw_name = sw_list[i].sw_name;
				objlist[i]->obj.software.sw_id = sw_list[i].sw_id;
				objlist[i]->obj.software.version = sw_list[i].version;
				objlist[i]->obj.software.category = sw_list[i].category;
				objlist[i]->obj.software.vendor = sw_list[i].vendor;
				objlist[i]->obj.software.arch = sw_list[i].arch;
				objlist[i]->obj.software.date = sw_list[i].date;
				objlist[i]->obj.software.prodname = sw_list[i].prodname;
				objlist[i]->obj.software.prodvers = sw_list[i].prodvers;
				objlist[i]->obj.software.desc = sw_list[i].desc;
				objlist[i]->obj.software.basedir = sw_list[i].basedir;
				objlist[i]->obj.software.locale = sw_list[i].locale;
				objlist[i]->obj.software.install_reqs = sw_list[i].install_reqs;
				objlist[i]->obj.software.instance = sw_list[i].instance;
				objlist[i]->obj.software.size = sw_list[i].size;
				objlist[i]->obj.software.level = sw_list[i].level;
				break;
			}
		}
		XtVaSetValues(ctxt->objectList, XmNselectionPolicy,
			XmBROWSE_SELECT, NULL);

		switch (type) {
		    case USER:
			sysman_free_user_list(userlist, listnum);
			break;
	
		    case GROUP:
			sysman_free_group_list(grouplist, listnum);
			break;
	
		    case HOST:
			sysman_free_host_list(hostlist, listnum);
			break;
	
		    case PRINTER:
			sysman_free_printer_list(printerlist, listnum);
			break;
	
		    case SERIAL:
			sysman_free_serial_list(seriallist, listnum);
			break;
	
		    case SOFTWARE:
			XtVaSetValues(ctxt->objectList, XmNselectionPolicy,
				XmEXTENDED_SELECT, NULL);
			break;
		}

		/* sort list */
		if (type != SOFTWARE) {
			qsort(objlist, entry_count,
				sizeof(obj_data*), compare_name);
		}

		/* display list */
		display_host_list();
		current_type = type;

		set_menus_active(True);
		set_edit_functions_active(False);

		rtn = 1;
	}
	else if (listnum == 0) {
		entry_count = 0;
		objlist = NULL;
		XmListDeleteAllItems(ctxt->objectList);
		current_type = type;

		set_menus_active(True);
		set_edit_functions_active(False);

		rtn = 1;
	}
	else {
		display_error(sysmgrmain, errbuf);
		rtn = 0;
	}

	SetBusyPointer(False);

	return rtn;
}

static int
find_entry(
	void*	od
)
{
	int	u_bound, l_bound = 0;
	int	cmp;


	u_bound = entry_count - 1;

	/* Perform binary search */
	while (u_bound >= l_bound) {
		int i = l_bound + (u_bound - l_bound)/2;

		switch (current_type) {
		    case USER:
			cmp = strcmp(objlist[i]->obj.user.username_key,
				((SysmanUserArg*)od)->username_key);
			break;
	
		    case GROUP:
			cmp = strcmp(objlist[i]->obj.group.groupname_key,
				((SysmanGroupArg*)od)->groupname_key);
			break;
	
		    case HOST:
			cmp = strcmp(objlist[i]->obj.host.hostname_key,
				((SysmanHostArg*)od)->hostname_key);
			break;
	
		    case PRINTER:
			cmp = strcmp(objlist[i]->obj.printer.printername,
				((SysmanPrinterArg*)od)->printername);
			break;
	
		    case SERIAL:
			cmp = strcmp(objlist[i]->obj.serial.port,
				((SysmanSerialArg*)od)->port);
			if (cmp == 0) {
				cmp = strcmp(objlist[i]->obj.serial.pmtag_key,
					((SysmanSerialArg*)od)->pmtag_key);
				if (cmp == 0) {
					if (objlist[i]->obj.serial.svctag_key
					    == NULL) {
						/* object had "no service" */
						cmp = 0;
					} else {
						cmp = strcmp(
					  objlist[i]->obj.serial.svctag_key,
					  ((SysmanSerialArg*)od)->svctag_key);
					}
				}
			}
			break;
	
		    case SOFTWARE:
			break;
		}

		if (cmp > 0)
			u_bound = i-1;	/* entry comes before item */
		else if (cmp < 0)
			l_bound = i+1;	/* entry comes after item */
		else
			return i;	/* found entry */
	}

	return -1;
}

static int
find_insertion(
	void*	op
)
{
	int	u_bound, l_bound = 0;
	int	cmp;


	u_bound = entry_count - 1;

	/* Perform binary search */
	while (u_bound >= l_bound) {
		int i = l_bound + (u_bound - l_bound)/2;

		switch (current_type) {
		    case USER:
			cmp = strcmp(objlist[i]->obj.user.username,
				((SysmanUserArg*)op)->username);
			break;
	
		    case GROUP:
			cmp = strcmp(objlist[i]->obj.group.groupname,
				((SysmanGroupArg*)op)->groupname);
			break;
	
		    case HOST:
			cmp = strcmp(objlist[i]->obj.host.hostname,
				((SysmanHostArg*)op)->hostname);
			break;
	
		    case PRINTER:
			cmp = strcmp(objlist[i]->obj.printer.printername,
				((SysmanPrinterArg*)op)->printername);
			break;
	
		    case SERIAL:
			cmp = strcmp(objlist[i]->obj.serial.port,
				((SysmanSerialArg*)op)->port);
			if (cmp == 0) {
				cmp = strcmp(objlist[i]->obj.serial.pmtag_key,
					((SysmanSerialArg*)op)->pmtag_key);
				if (cmp == 0) {
					if (objlist[i]->obj.serial.svctag_key
					    == NULL) {
						/* object had "no service" */
						cmp = 0;
					} else {
						cmp = strcmp(
					  objlist[i]->obj.serial.svctag_key,
					  ((SysmanSerialArg*)op)->svctag_key);
					}
				}
			}
			break;
	
		    case SOFTWARE:
			break;
		}

		if (cmp > 0)
			u_bound = i-1;	/* entry comes before item */
		else
			l_bound = i+1;	/* entry comes after item */
	}

	return l_bound;
}

void
add_list_entry(
	int	objtype,
	void*	obj
)
{
	sysMgrMainCtxt* ctxt;
	char *		entry;
	XmString	xstr;
	int		index;
	obj_data*	newobj;


	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	/* add to main obj list */
	index = find_insertion(obj);
	objlist = (obj_data**)realloc(objlist,
				(entry_count+1) * sizeof(obj_data*));
	if (index < entry_count) {
		memmove(&objlist[index+1], &objlist[index],
				(entry_count-index) * sizeof(obj_data*));
	}
	newobj = (obj_data*)malloc(sizeof(obj_data));
	objlist[index] = newobj;
	newobj->type = objtype;

	switch (objtype) {
	    case USER:
		copy_user(&newobj->obj.user, obj);
		break;

	    case GROUP:
		copy_group(&newobj->obj.group, obj);
		break;

	    case HOST:
		copy_host(&newobj->obj.host, obj);
		break;

	    case PRINTER:
		copy_printer(&newobj->obj.printer, obj);
		break;

	    case SERIAL:
		copy_serial(&newobj->obj.serial, obj);
		break;

	    case SOFTWARE:
		break;
	}

	entry_count++;

	/* add to scrolling list */
	entry = get_display_string(objlist[index]);
	xstr = XmStringCreateLocalized(entry);
	XmListAddItemUnselected(ctxt->objectList, xstr, index+1);
	XmStringFree(xstr);

	XmListDeselectAllItems(ctxt->objectList);
	XmListSelectPos(ctxt->objectList, index+1, False);
	MakePosVisible(ctxt->objectList, index+1);

	set_edit_functions_active(True);
}

void
add_user_to_list(
	SysmanUserArg* user
)
{
	add_list_entry(USER, (void*)user);
}

void
add_group_to_list(
	SysmanGroupArg* group
)
{
	add_list_entry(GROUP, (void*)group);
}

void
add_host_to_list(
	SysmanHostArg* host
)
{
	add_list_entry(HOST, (void*)host);
}

void
add_printer_to_list(
	SysmanPrinterArg* printer
)
{
	add_list_entry(PRINTER, (void*)printer);
}

void
add_serial_to_list(
	SysmanSerialArg* serial
)
{
	add_list_entry(SERIAL, (void*)serial);
}

void
add_software_to_list(
	SysmanSWArg* software
)
{
	/*  
	add_list_entry(SOFTWARE, (void*)software);
	*/

	/* we will just rebuild the entire thing for now... */
	load_list(SOFTWARE);
}

static void
delete_list_entry(
	int	idx
)
{
	sysMgrMainCtxt*	ctxt;

	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	switch (objlist[idx]->type) {
	    case USER:
		free_user(&objlist[idx]->obj.user);
		break;

	    case GROUP:
		free_group(&objlist[idx]->obj.group);
		break;

	    case HOST:
		free_host(&objlist[idx]->obj.host);
		break;

	    case PRINTER:
		free_printer(&objlist[idx]->obj.printer);
		break;

	    case SERIAL:
		free_serial(&objlist[idx]->obj.serial);
		break;

	    case SOFTWARE:
		break;
	}
	free_mem(objlist[idx]);

	if ((idx+1) < entry_count) {
		memmove(&objlist[idx], &objlist[idx+1],
			(entry_count-(idx+1)) * sizeof(obj_data*));
	}
	entry_count--;

	XmListDeletePos(ctxt->objectList, idx+1);
}

/* update main list */
void
update_entry(
	void* o_ptr
)
{
	sysMgrMainCtxt*	ctxt;
	int		idx;
	XmString	xstr;
	void*		find_ptr;
	int		key_field_changed = 0;

	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	if (o_ptr == NULL)
		return;

	idx = find_entry(o_ptr);

	if (idx < 0) {
		/* should never happen */
		printf("Couldn't find object in main list!\n");
		return;
	}

	switch (objlist[idx]->type) {
	    case USER:
		key_field_changed = strcmp(objlist[idx]->obj.user.username,
			((SysmanUserArg*)o_ptr)->username);
		free_user(&objlist[idx]->obj.user);
		copy_user(&objlist[idx]->obj.user, (SysmanUserArg*)o_ptr);
		break;

	    case GROUP:
		key_field_changed = strcmp(objlist[idx]->obj.group.groupname,
			((SysmanGroupArg*)o_ptr)->groupname);
		free_group(&objlist[idx]->obj.group);
		copy_group(&objlist[idx]->obj.group, (SysmanGroupArg*)o_ptr);
		break;
	
	    case HOST:
		key_field_changed = strcmp(objlist[idx]->obj.host.hostname,
			((SysmanHostArg*)o_ptr)->hostname);
		free_host(&objlist[idx]->obj.host);
		copy_host(&objlist[idx]->obj.host, (SysmanHostArg*)o_ptr);
		break;

	    case PRINTER:
		key_field_changed = strcmp(
			objlist[idx]->obj.printer.printername,
			((SysmanPrinterArg*)o_ptr)->printername);
		free_printer(&objlist[idx]->obj.printer);
		copy_printer(&objlist[idx]->obj.printer,
			(SysmanPrinterArg*)o_ptr);
		break;

	    case SERIAL:
		key_field_changed = strcmp(objlist[idx]->obj.serial.port,
			((SysmanSerialArg*)o_ptr)->port);
		free_serial(&objlist[idx]->obj.serial);
		copy_serial(&objlist[idx]->obj.serial, (SysmanSerialArg*)o_ptr);
		break;

	    case SOFTWARE:
		break;
	}

	if (key_field_changed) {
		find_ptr = (void*)&objlist[idx]->obj.user;
		qsort(objlist, entry_count, sizeof(obj_data*), compare_name);
		display_host_list();
		idx = find_entry(find_ptr);
	}
	else {
		xstr = XmStringCreateLocalized(
			get_display_string(objlist[idx]));
		XmListReplaceItemsPos(ctxt->objectList, &xstr,1,idx+1);
		XmStringFree(xstr);
	}

	XmListSelectPos(ctxt->objectList, idx+1, True);
	MakePosVisible(ctxt->objectList, idx+1);
}

Boolean
check_unique(
	char* hostname,
	char* ip_addr,
	char* enet_addr,
	Widget dialog
)
{
	char*	name;
	char*	ip;
	char*	ether;
	char	msg[256];

	/* check to see if name, ip address, and ethernet address
	 * already exist in host list
	 */
/*
	for (int k=0; k < entry_count; k++) {
		Host* h_obj = objlist[k]->host;
		if (h_obj != host) {
			name = (char*)(const char*)h_obj->get_name();
			if (strcmp(hostname, name) == 0) {
				sprintf(msg, 
					"hostname `%s' already exists",
					name);
				display_error(dialog, msg);
				return False;
			}
			ip = (char*)(const char*)h_obj->get_ip_address();
			if (strcmp(ip_addr, ip) == 0) {
				sprintf(msg, 
					"IP address `%s' already exists",
					ip_addr);
				display_error(dialog, msg);
				return False;
			}
			ether = (char*)(const char*)h_obj->get_ethernet_address();
			if (*enet_addr && strcmp(enet_addr, ether) == 0) {
				sprintf(msg,
					"Ethernet address `%s' already exists",
					enet_addr);
				display_error(dialog, msg);
				return False;
			}
		}
	}
*/

	return True;
}

void
set_menus_active(Boolean active)
{
	sysMgrMainCtxt  *ctxt;

	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	XtSetSensitive(ctxt->editMenu, active ? True : False);

}

void
set_edit_functions_active(Boolean active)
{
	sysMgrMainCtxt  *ctxt;

	XtVaGetValues(sysmgrmain,
		XmNuserData, &ctxt,
		NULL);

	if (current_type == SERIAL)
		XtSetSensitive(ctxt->addMenuItem, False);
	else
		XtSetSensitive(ctxt->addMenuItem, True);

	if (current_type == SOFTWARE) {
		XtSetSensitive(ctxt->modifyMenuItem, False);
		XtVaSetValues(ctxt->detailsButton, 
				XmNsensitive, active, NULL);
	} else
		XtSetSensitive(ctxt->modifyMenuItem, active);

	XtSetSensitive(ctxt->deleteMenuItem, active);
}
