
/*
 * Name:      tu_runtime.h
 * $Author: amber $
 *
 * Copyright  1992-93 Telesoft, 1993 Alsys
 * Version:   TeleUSE 2.7
 *
 * Purpose:   
 *            TeleUSE runtime header file
 *
 * $Log:	tu_runtime.h,v $
 * Revision 1.12  94/02/28  09:59:52  amber
 * Added TuNdialogName
 * 
 * Revision 1.11  94/02/17  11:06:05  trevor
 * Add tu_register_{bit,pix}maps to the interface
 * 
 * Revision 1.10  94/02/16  17:52:03  trevor
 * Changed type of tu_pmdefs and tu_bmdefs so that they are
 * a vector of tu_pmdefs_p/tu_bmdefs_p instead of an array
 * of _tu_pmdefs/_tu_bmdefs
 * Also added tu_line_buffer and tu_setup_locale
 * 
 * Revision 1.9  94/01/19  15:27:20  ka
 * Added externalrefs for tu_pmdefs and tu_bmdefs
 * 
 * Revision 1.8  94/01/17  13:25:24  trevor
 * Removed user-defined resource support
 * 
 * Revision 1.6  93/12/17  16:46:46  trevor
 * Added a function to write in xbm format
 * 
 * Revision 1.5  93/11/09  09:44:14  trevor
 * Added two more enumeration choices for nodeAccess
 * 
 * Revision 1.4  93/11/05  13:55:56  pvachal
 * Added support for nodeAccess resource
 * 
 * Revision 1.3  93/09/28  09:45:06  johan
 * *** empty log message ***
 * 
 * Revision 1.2  93/08/31  16:23:34  johan
 * Revision 1.1  93/08/03  14:26:22  mike
 * Initial 2.7 checkin
 * 
 * $Locker:  $
 */

#ifndef _tu_runtime_h_
#define _tu_runtime_h_

#ifndef RV_SUPPORT
#define RV_SUPPORT 0
#endif

#ifndef OPEN_LOOK
#define OPEN_LOOK 1
#endif

#ifndef UIL
#define UIL 1
#endif

#ifndef TU_CVT
#define TU_CVT 1
#endif

#ifndef FULL_PIXMAP_SUPPORT 
#define FULL_PIXMAP_SUPPORT 1
#endif

#ifndef TU_STRINGS_ONLY
#include <X11/Intrinsic.h>
#if TU_CVT
#include <Xm/Xm.h>
#endif
#endif

/* copied from mit/X distribution */
#ifndef NeedFunctionPrototypes
#if defined(FUNCPROTO) || __STDC__ || \
    defined(__cplusplus) || defined(c_plusplus)
#define NeedFunctionPrototypes 1
#else
#define NeedFunctionPrototypes 0
#endif
#endif /* NeedFunctionPrototypes */

#ifndef _NO_PROTO
#if !NeedFunctionPrototypes 
#define _NO_PROTO 1
#endif
#endif

/*$CExtractStart*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/****************************************************************
 *
 * Representation Classes
 *
 ****************************************************************/

#ifndef TuCWidget
#define TuCWidget "Widget"
#endif

#ifndef TuCSortFunc
#define TuCSortFunc "SortFunc"
#endif

#ifndef TuCShowWidget
#define TuCShowWidget "ShowWidget"
#endif

/****************************************************************
 **
 ** Representation Types
 **
 ****************************************************************/

#define XtRXEvent "XEvent"
#define XmRXEvent XtRXEvent

#define XtRIntTable  "IntTable"
#define XmRIntTable  XtRIntTable

#define TuRIntTable             "IntTable"
#define TuRWidgetChild          "WidgetChild"
#define TuRDirectWidgetChild    "DirectWidgetChild"
#define TuRSiblingWidget        "SiblingWidget"
#define TuRSubMenuWidget        "SubMenuWidget"
#define TuRSubMenuItemWidget    "SubMenuItemWidget"
#define TuRSelectionDialogType  "SelectionDialogType"
#define TuRPixmap               "Pixmap"
#define TuRBitmap               "Bitmap"
#define TuRCursor               "Cursor"
#define TuRXEvent               "XEvent"
#define TuRWidgetRep            "WidgetRep"
#define TuRImmediateCallback    "ImmediateCallback"
#define TuRMwmInputMode         "MwmInputMode"
#define TuRXRectangleList       "XRectangleList"
#define TuRDashList             "DashList"
#define TuRSelectionArray       "SelectionArray"
#define TuRIString              "IString"
#define TuRNodeAccessibility    "NodeAccessibility"

/****************************************************************
 **
 **  Resources Names
 **
 ****************************************************************/

#ifndef MrmNcreateCallback
#define MrmNcreateCallback "createCallback"
#endif

/*
 *  Oldies
 */

#define TuNUnmanaged           "Unmanaged"
#define TuNInTabGroup          "InTabGroup"
#define TuNDisplay             "Display"
#define TuNTopShell            "TopShell"
#define TuNKbdFocus            "KbdFocus"
#define TuNtemplateAttributes  "templateAttributes"


/*
 *  Control attributes
 */
#define TuNmanaged                 "managed"
#define TuNtopShell                "topShell"
#define TuNdisplay                 "display"
#define TuNkbdFocus                "kbdFocus"
#define TuNinTabGroup              "inTabGroup"
#define TuNcloseWindowCallback     "closeWindowCallback"
#define TuNuserDefinedAttributes   "userDefinedAttributes"
#define TuNinstallAccelerators     "installAccelerators"
#define TuNnodesUsed               "nodesUsed"
#define TuNdefaultCharacterSet	   "defaultCharacterSet"
#define TuNinstallPopupHandler     "installPopupHandler"
#define TuNdiscardedResources      "discardedResources"
#define TuNlockedResources         "lockedResources"
#define TuNlockedTemplate	   "lockedTemplate"
#define TuNchildOrder              "childOrder"

#define TuNmainNode                "mainNode"
#define TuNdefaultInsert           "defaultInsert"
#define TuNdefaultPopupInsert      "defaultPopupInsert"
#define TuNinsertAllowed           "insertAllowed"
#define TuNinsertPopupAllowed      "insertPopupAllowed"
#define TuNresizeAllowed           "resizeAllowed"
#define TuNmoveAllowed             "moveAllowed"

#define TuNsoftWidget              "softWidget"
#define TuNreturnWidget            "returnWidget"

#define TuNtemplatePrefix          "templatePrefix"
#define TuNwidgetName              "widgetName"
#define TuNnoUilName               "noUilName"
#define TuNnodeAccess              "nodeAccess"

#define TuNdialogName              "dialogName"

/*
 *  Attribute values
 */
#define TuNTrue        "True"
#define TuNFalse       "False"
#define TuNButton1     "Button1"
#define TuNButton2     "Button2"
#define TuNButton3     "Button3"
#define TuNButton4     "Button4"
#define TuNButton5     "Button5"

#define TuNAccessNone           "access_none"
#define TuNAccessInternal       "access_internal"
#define TuNAccessDiscard        "access_discard"
#define TuNAccessLocked         "access_locked"
#define TuNAccessAny            "access_any"
/*
 *  Resource names for extra XmSelectionBox attributes
 */

#define TuNshowList           "showList"
#define TuNshowListLabel      "showListLabel"
#define TuNshowSelectionLabel "showSelectionLabel"
#define TuNshowText           "showText"
#define TuNshowSeparator      "showSeparator"
#define TuNshowOkButton       "showOkButton"
#define TuNshowApplyButton    "showApplyButton"
#define TuNshowCancelButton   "showCancelButton"
#define TuNshowHelpButton     "showHelpButton"
#define TuNshowDefaultButton  "showDefaultButton"
#define TuNshowWorkArea       "showWorkArea"

/*
 *  Resource names for extra XmFileSelectionBox attributes
 *  (inherits the extra XmSelectionBox attributes)
 */

#define TuNshowFilterLabel    "showFilterLabel"
#define TuNshowFilterText     "showFilterText"
#define TuNshowDirList        "showDirList"
#define TuNshowDirListLabel   "showDirListLabel"

/*
 *  Resource names for extra XmMessageBox attributes
 */

#define TuNshowSymbolLabel    "showSymbolLabel"
#define TuNshowMessageLabel   "showMessageLabel"

/*
 *  Resource names for extra XmCommand attributes
 */

#define TuNshowCommandText    "showCommandText"
#define TuNshowPromptLabel    "showPromptLabel"
#define TuNshowHistoryList    "showHistoryList"

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
/*$CExtractEnd*/

#define TU_UNTAGGED_CLOSURE "_closure"

#ifndef TU_STRINGS_ONLY

/*$CExtractStart*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/****************************************************************
 *
 * Global variables ...
 *
 ****************************************************************/

externalref String             tu_application_name;
externalref String             tu_application_class;
externalref Widget             tu_global_top_widget;
externalref XtAppContext       tu_global_app_context;
externalref int                tu_verbose_flag;

#ifndef _NO_TELEUSE_2_0_
#define application_name   tu_application_name
#define application_class  tu_application_class
#define global_top_widget  tu_global_top_widget
#define global_app_context tu_global_app_context
#define ux_executable      tu_application_name
#define ux_verbose         tu_verbose_flag

#define ux_get_cmd_argc tu_get_cmd_argc
#define ux_get_cmd_argv tu_get_cmd_argv
#define ux_get_cmd_argv2 tu_get_cmd_argv2
#endif


/****************************************************************
 *
 * C callback support routines definitions and declarations ...
 *
 ****************************************************************/

typedef struct _tu_ccb_arg {
  String               name;
  String               value;
  struct _tu_ccb_arg * next;
} tu_ccb_arg_t, * tu_ccb_arg_p;

#ifdef _NO_PROTO
typedef void (*TuCallbackProc)();
#else
typedef void (*TuCallbackProc)(Widget, tu_ccb_arg_p, XtPointer);
#endif

#ifdef _NO_PROTO
typedef void (*tu_widcre_hook_fn)();
#else
typedef void (*tu_widcre_hook_fn)(Widget, XtPointer);
#endif

typedef XtPointer tu_template_descr;

#ifdef _NO_PROTO
typedef Widget (*tu_widget_create_fn)();
#else
typedef Widget (*tu_widget_create_fn)(char *templ,
				      char *name,
				      Widget parent,
				      Display *disp,
				      Screen *screen,
				      tu_template_descr *rv);
#endif

typedef struct _tu_ccb_def_t {
  String               name;
  TuCallbackProc       cb;
} tu_ccb_def_t;

typedef enum { 
  tu_cbt_devent, 
  tu_cbt_ccb
} tu_callback_type;

typedef struct _tu_cd_def
  *tu_cd_def, tu_cd_def_rec;

struct _tu_cd_def {
  tu_callback_type  type;
  char            * name;
  tu_ccb_arg_p      args;
};

typedef struct _tu_bmdefs* tu_bmdefs_p;
typedef struct _tu_pmdefs* tu_pmdefs_p;

externalref tu_pmdefs_p*        tu_pmdefs;
externalref tu_bmdefs_p*        tu_bmdefs;

#ifndef _NO_TELEUSE_1_1_
/* backward compatibility */
typedef struct _tu_ccb_arg 
  ccb_arg_t, *ccb_arg_p;
typedef struct _tu_ccb_def_t
  ux_ccb_def_t;
#endif

typedef struct _tu_color_scheme
  *tu_color_scheme;

typedef struct _tu_pixmap
  *tu_pixmap;

#ifndef MrmNcreateCallback
#define MrmNcreateCallback     "createCallback"
#endif

#ifndef _NO_TELEUSE_2_0_

#define ux_define_c_callback(name, cb, status) \
  { (status)->all = tu_status_ok; \
    tu_ccb_define(name, cb); }

#define ux_app_context() \
    tu_global_app_context

#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
/*$CExtractEnd*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/****************************************************************
 *
 * Widget support & definitions
 *
 ****************************************************************/

#define TU_FW_NORMAL            0x0001
#define TU_FW_POPUP             0x0002
#define TU_FW_ANY               (TU_FW_NORMAL|TU_FW_POPUP)

#define TU_MC_UP                0
#define TU_MC_DOWN              1
#define TU_MC_PLACE             2

#define TU_UWC_MESSAGE_BOX     'a'
#define TU_UWC_SELECTION_BOX   'b'
#define TU_UWC_FS_BOX          'c'



#if TU_CVT
/****************************************************************
 *
 * X11R5 XPM utility definitions 
 *
 ****************************************************************/


struct tu_pixmap_color_pair {
  char                        * name;
  char                        * color;
  struct tu_pixmap_color_pair * next;
};

struct tu_color_descr {
  char                        * ct_ch;
  int                           ct_index;
  unsigned long                 ct_pixel;
  char                        * ct_color_name;
  struct tu_pixmap_color_pair * ct_colors;
};

struct _tu_pixmap {
  char                        * pds_name;
  unsigned int                  pds_width;
  unsigned int                  pds_height;
  int                           pds_ncolors;
  int                           pds_nchars;
  struct tu_color_descr      ** pds_ct;
  unsigned char               * pds_data;
  XImage                      * pds_image;
  Pixmap                        pds_pixmap;
};


/*
 * TeleUSE color scheme description:
 *
 * TeleUSE may optionally use a more 'smart' color management scheme.
 * This scheme tells the color allocation routine how to allocate 
 * colors. Colors can, of course, be allocated without using this
 * scheme (the color scheme parameter is passed as NULL).
 *
 * The first part of the color scheme is the fuzz factor. The fuzz
 * factor describes the difference between a previously used color
 * and a new color. If the difference is less than fuzz, then the
 * old color is reused.
 *
 * If a color can not be allocated, the color that is closest in the
 * scheme structure to the one requested in the call is used.
 *
 * The second field is use_alloced_cells, which is used when
 * the user has previously allocated a number of writable color
 * cells and wants the color allocation software to use these cells.
 * When the field is set, then the max, alloced, colors, and refcnt fields
 * have to be initialized to contain their true values (i.e., zero/NULL
 * will not work). The color structures should be set up to hold
 * the pixel values available and the refcnt values should be set to 
 * zero. In this mode, the color cells are updated with the color
 * specified. The XColor structure in the scheme data structure 
 * is updated to contain the actual rgb triplets stored in the
 * color table.
 *
 * The refcnt field being zero indicates that the color
 * structure is not used.
 */

struct _tu_color_scheme {
  int                          cs_fuzz;
  int                          cs_use_alloced_cells;
  int                          cs_max;
  int                          cs_current;
  XColor                     * cs_colors;
  int                        * cs_refcnt;
  int                          cs_alloced;
};

/* Color scheme used by pixmap converter. Initially set to NULL, defined
   by tu_runtime.c. If an application wishes to use color schemes, this
   structure has to be initialized and allocated before conversions take 
   place. */

externalref tu_color_scheme tu_global_color_scheme;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

/*$CExtractStart*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct _tu_bmdefs {
  char           *name;
  unsigned int    width;
  unsigned int    height;
  unsigned char  *bitmap;
};

struct _tu_pmdefs {
  char           *name;
  char          **pixmap;
};

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
/*$CExtractEnd*/


/*$CExtractStart*/
#ifdef _NO_PROTO

/****************************************************************
 *
 * Command line utilities...
 *
 ****************************************************************/

extern void tu_set_arg_handler();
    /* int argc; */ 
    /* char **argv; */

extern int tu_get_cmd_argc();

extern char *tu_get_cmd_argv();
   /* int index; */

extern char **tu_get_cmd_argv2();
 
extern void tu_setup_locale();

extern void tu_line_buffer();

#else

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern void tu_set_arg_handler(int argc, char **argv);

extern int tu_get_cmd_argc(void);

extern char *tu_get_cmd_argv(int index);

extern char **tu_get_cmd_argv2(void);

extern void tu_setup_locale();

extern void tu_line_buffer();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif
/*$CExtractEnd*/


#ifdef _NO_PROTO

/****************************************************************
 *
 * General utilities ...
 *
 ****************************************************************/

extern char * tu_find_file();
    /* String filename; */
    /* String * dlpath; */

/****************************************************************
 *
 * CUIL generated code support
 *
 ****************************************************************/

typedef Widget (*tu_lc_func)();

extern void tu_init_tu_lookup_c_widget();
    /* char *name; */
    /* tu_lc_func; */

extern Widget tu_lookup_c_widget();
    /* Widget parent; */
    /* char *name;    */
    /* char *new_name;*/

/****************************************************************
 *
 * C callback support routines ...
 *
 ****************************************************************/

extern char * tu_get_bang_string();
    /* char ** pstr; */

extern void tu_ccb_define();
    /* String name; */
    /* TuCallbackProc cb; */

extern tu_ccb_def_t * tu_ccb_find();
    /* String name; */

extern TuCallbackProc tu_ccb_find_func();
    /* char * name; */

extern int tu_ccb_get_all();
    /* char *** pnames; */
    /* TuCallbackProc ** pfuncs; */

extern void tu_ccb_set_callback();
    /* Widget widget; */
    /* char * attr; */
    /* tu_cd_def cbdef; */
    /* char * tag; */

extern void tu_setup_close_window_callback();
    /* Widget widget; */
    /* TuCallbackProc func; */
    /* tu_ccb_arg_p ccb; */

extern void tu_xm_callback_popup();
    /* Widget w; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

extern void tu_xm_callback_popdown();
    /* Widget w; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

extern void tu_xm_callback_manage();
    /* Widget w; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

extern void tu_xm_callback_unmanage();
    /* Widget w; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

#if UIL

/****************************************************************
 *
 * Callback name binding to UIL
 *
 ****************************************************************/

extern void tu_uil_init();

extern void tu_uil_ccb();
    /* Widget widget; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

extern void tu_uil_register_mrm_ccb();

extern void tu_uil_add_tabgroup();
    /* Widget widget; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

extern void tu_uil_set_callback_attr();
    /* Widget widget; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

/****************************************************************
 *
 * Installing accelerators ...
 *
 ****************************************************************/

extern void tu_uil_collect_accelerators();

extern void tu_uil_set_accelerators();
    /* Widget widget; */
    /* XtPointer closure; */
    /* XtPointer calldata; */

extern void tu_uil_install_accelerators();

#endif /* UIL */

/****************************************************************
 *
 * Functions to create widgets by name 
 *
 ****************************************************************/

extern void tu_declare_create_method();
    /* tu_widget_create_fn create_fn; */

extern Widget tu_create_widget();
    /* char *templ; */
    /* char *name; */
    /* Widget parent; */
    /* Display *disp; */
    /* Screen *screen; */
    /* tu_template_descr *rv; */

extern Widget tu_lookup_c_widget();
    /* Widget parent; */
    /* char *name; */
    /* char *new_name; */


/****************************************************************
 *
 * Functions used to implement the create hooks 
 *
 ****************************************************************/

extern void tu_add_widcre_hook();
    /* tu_widcre_hook_fn hookfn; */
    /* XtPointer closure; */

extern void tu_remove_widcre_hook();
    /* tu_widcre_hook_fn hookfn; */
    /* XtPointer closure; */

extern void tu_widcre_invoke_hooks();
    /* Widget w; */


/****************************************************************
 *
 * Logical display handling
 *
 ****************************************************************/

extern void tu_disp_set_dpy();
    /* char * log_name; */
    /* char * phys_name; */
    /* Display * dpy; */

extern Display * tu_disp_get_dpy();
    /* char * log_name; */

extern void tu_disp_set_default();
    /* char * log_name; */

extern void tu_disp_set_default_dpy();
    /* Display * dpy; */

extern char * tu_disp_get_logical_name();
    /* Display * dpy; */

extern void tu_disp_close();
    /* Display * dpy; */

extern Widget tu_disp_get_temp_widget();
    /* char * log_name; */
    /* char * screen_num; */

/****************************************************************
 *
 * Predefined operation for menus 
 *
 ****************************************************************/

extern void tu_menu_popup();
    /* Widget widget; */
    /* char * name; */
    /* XEvent * xevent; */

extern void tu_set_kbd_focus();
    /* Widget w; */
    /* XtPointer closure; */
    /* XEvent *event; */
    /* Boolean *cont; */

extern void tu_handle_popup_menu();
    /* Widget w; */
    /* XtPointer closure; */
    /* XEvent *event; */
    /* Boolean *cont; */

extern void tu_menu_popdown();
    /* Widget widget; */

extern void tu_menu_set_enabled_popup();
    /* Widget menu; */

/****************************************************************
 *
 * Predefined callback initialization
 *
 ****************************************************************/

extern void tu_xm_callbacks();

extern void tu_xt_callbacks();

#if TU_CVT

/****************************************************************
 *
 * Conversion package
 *
 ****************************************************************/

/****************************************************************
 *
 * Color allocation routines
 *
 ****************************************************************/

extern int tu_allocate_color();
    /* Screen * screen; */
    /* Colormap cmap; */
    /* XColor * color; */
    /* tu_color_scheme colsch; */

extern void tu_free_color();
    /* Screen * screen; */
    /* Colormap cmap; */
    /* unsigned long pixel; */
    /* tu_color_scheme colsch; */

extern void tu_pds_allocate_colors();
    /* Screen * screen; */
    /* Colormap cmap; */
    /* XVisualInfo * v; */
    /* tu_pixmap pds; */
    /* tu_color_scheme colsch; */

extern void tu_pds_free_colors();
    /* Screen * screen; */
    /* Colormap cmap; */
    /* XVisualInfo * v; */
    /* tu_pixmap pds; */
    /* tu_color_scheme colsch; */

/****************************************************************
 *
 * XPM3 Pixmap support routines
 *
 ****************************************************************/

extern void tu_pds_free();
    /* tu_pixmap p; */

extern tu_pixmap tu_pds_read_xpm_file();
    /* char * filename; */

extern tu_pixmap tu_pds_read_xpm_string();
    /* char * str; */

extern void tu_pds_create_pixmap();
    /* Screen * screen; */
    /* unsigned int depth; */
    /* XVisualInfo * v; */
    /* tu_pixmap pds; */

extern void tu_pds_free_pixmap();
    /* Screen * screen; */
    /* tu_pixmap pds; */

#if FULL_PIXMAP_SUPPORT

extern int tu_pds_write_xbm_file();
    /* char * filename; */
    /* tu_pixmap pds; */

extern int tu_pds_write_xpm_file();
    /* char * filename; */
    /* tu_pixmap pds; */

/****************************************************************
 *
 * Bitmap to tu_pixmap support
 *
 ****************************************************************/

extern tu_pixmap tu_pds_read_xbm_file();
    /* char * filename; */

extern tu_pixmap tu_pds_read_xbm_string();
    /* char * str; */

#endif /* FULL_PIXMAP_SUPPORT */

/****************************************************************
 *
 * Converter access function utility
 *
 ****************************************************************/

extern XtPointer tu_convert();
    /* Widget widget; */
    /* String value; */
    /* String target; */
    /* int * ok; */

/****************************************************************
 * 
 * TeleUSE-specific converters for Motif
 *
 ****************************************************************/

extern XmString tu_cvt_string_to_xmstring();
    /* char * str; */

extern char * tu_cvt_xmstring_to_string();
    /* XmString str; */
    /* int charset_info; */

extern void tu_xm_resource_converters();

/****************************************************************
 *
 * TeleUSE-specific resource converters (other)
 *
 ****************************************************************/

extern void tu_register_bitmaps ();
    /* tu_bmdefs_p bitmaps; */
    /* int n_bm; */

extern void tu_register_pixmaps ();
    /* tu_pmdefs_p pixmaps; */
    /* int n_pm; */

extern void tu_set_image_path();
    /* char ** path_list; */
    /* int cnt; */

extern char ** tu_get_image_path();

extern Pixmap tu_make_pixmap();
    /* char * name; */
    /* Screen * screen; */
    /* Colormap cmap; */
    /* tu_pmdefs_p* pmdefs; */
    /* tu_bmdefs_p* bmdefs; */
    /* tu_pixmap *r_pds; */

extern Pixmap tu_make_bitmap();
    /* char * name; */
    /* Screen *screen; */
    /* tu_pmdefs_p* pmdefs; */
    /* tu_bmdefs_p* bmdefs; */

extern Pixmap tu_cvt_to_bitmap();
    /* Widget widget; */
    /* char * name; */

extern Pixmap tu_cvt_to_pixmap();
    /* Widget widget; */
    /* char * name; */

extern void tu_xt_resource_converters();

#endif /* TU_CVT */

/****************************************************************
 * 
 * Widget handling support routines
 * 
 ****************************************************************/

extern char * tu_widget_name();
    /* Widget widget; */

extern XrmName tu_widget_xrm_name();
    /* Widget widget; */

extern WidgetClass tu_widget_class();
    /* Widget widget; */

extern Widget tu_widget_top_widget();
    /* Widget widget; */

extern Widget tu_widget_root_widget();
    /* Widget widget; */

extern Widget * tu_widget_children();
    /* Widget widget; */
    /* Widget * array; */
    /* int * pcount; */
    /* int max; */
    /* int flg; */

extern int tu_widget_num_children();
    /* Widget widget; */
    /* int flg; */

extern int tu_widget_child_placement();
    /* Widget widget; */

extern void tu_widget_move_child();
    /* Widget widget; */
    /* int way; */
    /* int number; */


extern Widget tu_find_widget();
    /* Widget widget; */
    /* char * name; */
    /* int flg; */
    /* int depth; */

extern Widget tu_find_sibling();
    /* Widget widget; */
    /* char * name; */

extern Widget tu_find_parent();
    /* Widget widget; */
    /* char * name; */

extern Widget tu_find_submenu_widget();
    /* Widget widget; */
    /* char * name; */

extern Widget tu_find_submenu_item_widget();
    /* Widget widget; */
    /* char * name; */

extern Widget tu_find_widget_by_class();
    /* Widget widget; */
    /* char * class_name; */
    /* int flg; */
    /* int depth; */

extern Widget tu_find_parent_by_class();
    /* Widget widget; */
    /* char * class_name; */


extern Widget tu_first_child();
    /* Widget widget; */

extern Widget tu_first_widget_child();
    /* Widget widget; */

extern Widget tu_first_popup();
    /* Widget widget; */

extern Widget tu_first_managed();
    /* Widget widget; */


extern char * tu_class_name();
    /* WidgetClass widget_class; */

extern WidgetClass tu_class_super_class();
    /* WidgetClass widget_class; */

/****************************************************************
 *
 * Installing accelerators ...
 *
 ****************************************************************/

extern void tu_acc_install_all_accelerators();
    /* Widget destination; */
    /* Widget source; */

extern void tu_acc_set_accelerators();
    /* Widget widget; */
    /* char * target; */

#if OPEN_LOOK

/****************************************************************
 * 
 * Open Look support routines
 * 
 ****************************************************************/

extern void tu_ol_fix_widget();
    /* Widget widget; */

extern void tu_ol_fix_hierarchy();
    /* Widget w; */

#endif /* OPEN_LOOK */

#else

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


/****************************************************************
 *
 * General utilities ...
 *
 ****************************************************************/

extern char * tu_find_file(String filename,
                           String * dlpath);

/****************************************************************
 *
 * CUIL generated code support
 *
 ****************************************************************/
typedef Widget (*tu_lc_func)();
extern void tu_init_tu_lookup_c_widget(char *name, tu_lc_func func);

extern Widget tu_lookup_c_widget(Widget parent,char *name,char *new_name);

/****************************************************************
 *
 * C callback support routines ...
 *
 ****************************************************************/

extern char * tu_get_bang_string(char ** pstr);

extern void tu_ccb_define(String name,
                          TuCallbackProc cb);

extern tu_ccb_def_t * tu_ccb_find(String name);

extern TuCallbackProc tu_ccb_find_func(char * name);

extern int tu_ccb_get_all(char *** pnames,
                          TuCallbackProc ** pfuncs);

extern void tu_ccb_set_callback(Widget widget,
				char * attr,
				tu_cd_def cbdef,
				char * tag);

extern void tu_setup_close_window_callback(Widget widget,
					   TuCallbackProc func,
					   tu_ccb_arg_p ccb);

extern void tu_xm_callback_popup(Widget w, 
				 XtPointer closure,
				 XtPointer calldata);

extern void tu_xm_callback_popdown(Widget w,
				   XtPointer closure,
				   XtPointer calldata);

extern void tu_xm_callback_manage(Widget w,
				  XtPointer closure,
				  XtPointer calldata);

extern void tu_xm_callback_unmanage(Widget w,
				    XtPointer closure,
				    XtPointer calldata);

#if UIL

/****************************************************************
 *
 * Callback name binding to UIL
 *
 ****************************************************************/

extern void tu_uil_init(void);

extern void tu_uil_ccb(Widget widget,
                       XtPointer closure,
                       XtPointer calldata);

extern void tu_uil_register_mrm_ccb(void);

extern void tu_uil_add_tabgroup(Widget widget,
                                XtPointer closure,
                                XtPointer calldata);

extern void tu_uil_set_callback_attr(Widget widget,
                                     XtPointer closure,
                                     XtPointer calldata);

/****************************************************************
 *
 * Installing accelerators ...
 *
 ****************************************************************/

extern void tu_uil_collect_accelerators(void);

extern void tu_uil_set_accelerators(Widget widget,
                                    XtPointer closure,
                                    XtPointer calldata);

extern void tu_uil_install_accelerators(void);

#endif /* UIL */

/****************************************************************
 *
 * Functions to create widgets by name 
 *
 ****************************************************************/

extern void tu_declare_create_method(tu_widget_create_fn create_fn);

extern Widget tu_create_widget(char *templ,
			       char *name,
			       Widget parent,
			       Display *disp,
			       Screen *screen,
			       tu_template_descr *rv);

extern Widget tu_lookup_c_widget(Widget parent,
				 char *name,
				 char *new_name);


/****************************************************************
 *
 * Functions used to implement the create hooks 
 *
 ****************************************************************/

extern void tu_add_widcre_hook(tu_widcre_hook_fn hookfn,
                               XtPointer closure);

extern void tu_remove_widcre_hook(tu_widcre_hook_fn hookfn,
                                  XtPointer closure);

extern void tu_widcre_invoke_hooks(Widget w);


/****************************************************************
 *
 * Logical display handling
 *
 ****************************************************************/

extern void tu_disp_set_dpy(char * log_name,
                            char * phys_name,
                            Display * dpy);

extern Display * tu_disp_get_dpy(char * log_name);

extern void tu_disp_set_default(char * log_name);

extern void tu_disp_set_default_dpy(Display * dpy);

extern char * tu_disp_get_logical_name(Display * dpy);

extern void tu_disp_close(Display * dpy);

extern Widget tu_disp_get_temp_widget(char * log_name,
                                      char * screen_num);

/****************************************************************
 *
 * Predefined operation for menus 
 *
 ****************************************************************/

extern void tu_menu_popup(Widget widget,
                          char * name,
                          XEvent * xevent);

extern void tu_set_kbd_focus(Widget w,
			     XtPointer closure,
			     XEvent *event,
			     Boolean *cont);

extern void tu_handle_popup_menu(Widget w,
				 XtPointer closure,
				 XEvent *event,
				 Boolean *cont);

extern void tu_menu_popdown(Widget widget);

extern void tu_menu_set_enabled_popup(Widget menu);

/****************************************************************
 *
 * Predefined callback initialization
 *
 ****************************************************************/

extern void tu_xm_callbacks(void);

extern void tu_xt_callbacks(void);

#if TU_CVT

/****************************************************************
 *
 * Conversion package
 *
 ****************************************************************/

/****************************************************************
 *
 * Color allocation routines
 *
 ****************************************************************/

extern int tu_allocate_color(Screen * screen,
			     Colormap cmap,
			     XColor * color,
			     tu_color_scheme colsch);

extern void tu_free_color(Screen * screen,
                          Colormap cmap,
                          unsigned long pixel,
                          tu_color_scheme colsch);

extern void tu_pds_allocate_colors(Screen * screen,
                                   Colormap cmap,
                                   XVisualInfo * v,
                                   tu_pixmap pds,
                                   tu_color_scheme colsch);

extern void tu_pds_free_colors(Screen * screen,
                               Colormap cmap,
                               XVisualInfo * v,
                               tu_pixmap pds,
                               tu_color_scheme colsch);

/****************************************************************
 *
 * XPM3 Pixmap support routines
 *
 ****************************************************************/

extern void tu_pds_free(tu_pixmap p);

extern tu_pixmap tu_pds_read_xpm_file(char * filename);

extern tu_pixmap tu_pds_read_xpm_string(char * str);

extern void tu_pds_create_pixmap(Screen * screen,
                                 unsigned int depth,
                                 XVisualInfo * v,
                                 tu_pixmap pds);

extern void tu_pds_free_pixmap(Screen * screen,
                               tu_pixmap pds);

#if FULL_PIXMAP_SUPPORT

extern int tu_pds_write_xbm_file(char * filename,
                                 tu_pixmap pds);

extern int tu_pds_write_xpm_file(char * filename,
                                 tu_pixmap pds);

/****************************************************************
 *
 * Bitmap to tu_pixmap support
 *
 ****************************************************************/

extern tu_pixmap tu_pds_read_xbm_file(char * filename);

extern tu_pixmap tu_pds_read_xbm_string(char * str);

#endif /* FULL_PIXMAP_SUPPORT */

/****************************************************************
 *
 * Converter access function utility
 *
 ****************************************************************/

extern XtPointer tu_convert(Widget widget,
                            String value,
                            String target,
                            int * ok);

/****************************************************************
 * 
 * TeleUSE-specific converters for Motif
 *
 ****************************************************************/

extern XmString tu_cvt_string_to_xmstring(char * str);

extern char * tu_cvt_xmstring_to_string(XmString str,
                                        int charset_info);

extern void tu_xm_resource_converters(void);

/****************************************************************
 *
 * TeleUSE-specific resource converters (other)
 *
 ****************************************************************/

extern void tu_register_bitmaps (tu_bmdefs_p bitmaps, int n_bm);

extern void tu_register_pixmaps (tu_pmdefs_p pixmaps, int n_pm);

extern void tu_set_image_path(char ** path_list,
                              int cnt);

extern char ** tu_get_image_path(void);

extern Pixmap tu_make_pixmap(char * name,
			     Screen * screen,
			     Colormap cmap,
			     tu_pmdefs_p* pmdefs,
			     tu_bmdefs_p* bmdefs,
			     tu_pixmap *r_pds);

extern Pixmap tu_make_bitmap(char * name,
			     Screen *screen,
			     tu_pmdefs_p* pmdefs,
			     tu_bmdefs_p* bmdefs);

extern Pixmap tu_cvt_to_bitmap(Widget widget,
                            char * name);

extern Pixmap tu_cvt_to_pixmap(Widget widget,
                            char * name);

extern void tu_xt_resource_converters(void);

#endif /* TU_CVT */

/****************************************************************
 * 
 * Widget handling support routines
 * 
 ****************************************************************/

extern char * tu_widget_name(Widget widget);

extern XrmName tu_widget_xrm_name(Widget widget);

extern WidgetClass tu_widget_class(Widget widget);

extern Widget tu_widget_top_widget(Widget widget);

extern Widget tu_widget_root_widget(Widget widget);

extern Widget * tu_widget_children(Widget widget,
                                   Widget * array,
                                   int * pcount,
                                   int max,
                                   int flg);

extern int tu_widget_num_children(Widget widget,
                                  int flg);

extern int tu_widget_child_placement(Widget widget);

extern void tu_widget_move_child(Widget widget,
                                 int way,
                                 int number);


extern Widget tu_find_widget(Widget widget,
                             char * name,
                             int flg,
                             int depth);

extern Widget tu_find_sibling(Widget widget,
                              char * name);

extern Widget tu_find_parent(Widget widget,
                             char * name);

extern Widget tu_find_submenu_widget(Widget widget,
                                     char * name);

extern Widget tu_find_submenu_item_widget(Widget widget,
                                          char * name);

extern Widget tu_find_widget_by_class(Widget widget,
                                      char * class_name,
                                      int flg,
                                      int depth);

extern Widget tu_find_parent_by_class(Widget widget,
                                      char * class_name);


extern Widget tu_first_child(Widget widget);

extern Widget tu_first_widget_child(Widget widget);

extern Widget tu_first_popup(Widget widget);

extern Widget tu_first_managed(Widget widget);


extern char * tu_class_name(WidgetClass widget_class);

extern WidgetClass tu_class_super_class(WidgetClass widget_class);

/****************************************************************
 *
 * Installing accelerators ...
 *
 ****************************************************************/

extern void tu_acc_install_all_accelerators(Widget destination,
                                            Widget source);

extern void tu_acc_set_accelerators(Widget widget,
                                    char * target);

#if OPEN_LOOK

/****************************************************************
 * 
 * Open Look support routines
 * 
 ****************************************************************/

extern void tu_ol_fix_widget(Widget widget);

extern void tu_ol_fix_hierarchy(Widget w);

#endif /* OPEN_LOOK */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _NO_PROTO */

#endif /* _TU_RUNTIME_ */

#endif /* TU_STRINGS_ONLY */
