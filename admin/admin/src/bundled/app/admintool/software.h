/*
 * Copyright (c) 1994 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
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

#ifndef	_SOFTWARE_H
#define	_SOFTWARE_H

#pragma ident "@(#)software.h	1.19 95/05/04 Sun Microsystems"

#include <Xm/Xm.h>
#include "spmisoft_api.h"
#include "media.h"


/* This is index in installFileSystems table of longest FS name */
#define LONGESTNAME_FS 5
#define MAXNAMELEN_FS (strlen(installFileSystems[LONGESTNAME_FS]) + 1)
#define N_INSTALL_FS 6

#define	SIZE_LENGTH 20
#define	MOUNTP_LENGTH 20 /* string length of space->mount */
#define	INDENT 28
#define	SELECT_OFFSET 20
#define MAX_SPACE_FS 15 	/* maximum number of file systems in space */
				/* calculations */
typedef struct {
	Widget select;		/* selection button widget, contains pointer */
				/*  to cluster/package module */
	ModStatus orig_status;	/* original status of module */
	int level;		/* indention level of module */
	Widget size;
} SelectionList;

typedef struct {
	Pixmap select;
	Pixmap unselect;
	Pixmap partial;
	Pixmap required;
} SelectPixmaps;

typedef struct {
	int mode;
	SelectionList SelectList[800];
	int Count;
	Widget total;
	Widget prodLabel, abbrevLabel, vendorLabel, descriptLabel;
	Widget spaceWidget[MAX_SPACE_FS];
	int maxSpace;
} TreeData;

typedef struct {
	Widget 	v_modulesForm;
	Widget  v_title;
	Widget	v_scrollw;
	Widget  v_rc;
	Widget  v_size;
	Widget	v_totalLabel;
	Widget	v_detailPane;
	Widget  v_pkginfoForm;	
	Widget  v_pkginfoText;	
	Widget	v_dependencyForm;
	Widget	v_dependencyText;
	Widget 	v_selectButton;
	Widget 	v_deselectButton;
	Widget	v_basedir_form;
	Widget	v_basedir_label;
	Widget	v_basedir_value;
        char  * v_locale;
	TreeData *v_swTree;
} ViewData;

typedef	struct
{
	enum sw_image_location sw_image_loc;
	unsigned int isSolarisImage; /* is image Solaris OS? */
	char*	pkg_path;      /* this is actual path where SUNW* dirs reside */
	char*	install_path;  /* this is what user input as source dir       */
			       /* it may != to pkg_path if Solaris product    */
	char*	install_device;
	Widget	shell;
	Widget	add_dialog;
	Widget	media_form;
	Widget	media_label;
	Widget	media_value;
	Widget	separator;
	Widget	set_media_btn;
	Widget  sw_form;		
	Widget	scrollwin;
	Widget	rowcol;
	Widget	info_form;
	Widget  totalLabel;
	Widget	customize_btn;
	Widget  detail;
	Widget	license_btn;
        Widget  meter_btn;
	Widget	button_box;
	Widget  selectToggleButton;
	Module  * module_tree;
	Module  * top_level_module;
        Module  * current_product; /* For multi-product cd */
	ViewData * view;
} addCtxt;

typedef struct fud {
	Boolean		f_selected;
	Widget		f_toggle;
	Module		* f_module;	
	Module		* f_copyOfModule; /* save subtree during custom */
	Modinfo		* f_mi;
        char		* f_locale;
	TreeData	* f_swTree;
	struct fud	* f_next;
} swFormUserData;

/*
 * For every Entry created by CreateEntry, an EntryData struct
 * is placed on the userData of the entry form. The Module
 * field points to the 'base' pkg, the Modinfo field pts to
 * the pkg specific info for a specific locale. For CLUSTERS
 * and non-loczalized pkgs, the e_mi field is same as e_m->info.mod
 */
typedef struct {
	Module *  e_m; 
	Modinfo * e_mi;
} EntryData;

extern swFormUserData	* FocusData;

extern ModStatus getModuleStatus(Module *, char *);
extern swFormUserData * fud_create(Widget, Module *, Modinfo *);
extern void free_fud_list();
extern void traverse_fud_list(void (*)(swFormUserData *, caddr_t), caddr_t);
extern void set_selected_fud(swFormUserData *, caddr_t);
extern void selected_fud_size(swFormUserData *, caddr_t);

extern Boolean pkg_exist(char * , char *);
extern Module * get_parent_product(Module *);

FSspace ** admintool_space_meter(char **);

#define SZ_LABEL_IX 2

#define	SUCCESS 0

#define MODE_INSTALL 0
#define MODE_REMOVE 1


#endif	/* _SOFTWARE_H */

