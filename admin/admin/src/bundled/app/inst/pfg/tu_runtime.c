
/*
 * Name:      tu_runtime.c
 * $Author: trevor $
 *
 * Copyright  1992-93 Telesoft, 1993 Alsys
 * Version:   TeleUSE 2.7
 *
 * Purpose:   
 *            Base runtime for TeleUSE environment. This file contains
 *            a number of functions, some of them optional. Sections are:
 *            Base (widget, callback, accelerators, menu and logical display
 *            support),
 *            UIL support functions    (if UIL), 
 *            OPEN LOOK support        (if OPEN_LOOK), 
 *            extra converters         (if TU_CVT), 
 *            and extra pixmap support (if FULL_PIXMAP_SUPPORT)
 *
 * Created:   
 *            920422
 *
 *
 * $Log:	tu_runtime.c,v $
 * Revision 1.19  94/05/25  17:11:33  trevor
 * MAXPATHLEN defined too far down in the file
 * 
 * Revision 1.18  94/05/25  13:07:14  trevor
 * Change getwd so it's a macro, not a static function
 * Add sun SVR4 exclusion for setlinebuf
 * Add inclusion for getwd on SVR4
 * Changed default value for MAXPATHLEN
 * 
 * Revision 1.17  94/03/06  14:49:17  mike
 * irix4_0 doesn't support setlocale correctly.
 * 
 * Revision 1.16  94/03/04  10:37:15  mike
 * Add target for linux
 * 
 * Revision 1.15  94/02/16  17:51:12  trevor
 * Changed type of tu_pmdefs and tu_bmdefs so that they are
 * a vector of tu_pmdefs_p/tu_bmdefs_p instead of an array
 * of _tu_pmdefs/_tu_bmdefs
 * Also added tu_line_buffer and tu_setup_locale
 * 
 * Revision 1.14  94/02/09  18:14:51  tomv
 * bitmap/pixmap initialization needed for reusable components
 * 
 * Revision 1.13  94/01/25  17:58:19  trevor
 * *** empty log message ***
 * 
 * Revision 1.12  94/01/17  13:25:38  trevor
 * Remove user-defined resource support
 * 
 * Revision 1.8  93/12/17  16:46:43  trevor
 * Added a function to write in xbm format
 * 
 * Revision 1.7  93/11/09  09:44:03  trevor
 * Added two more enumeration choices for nodeAccess
 * 
 * Revision 1.6  93/11/05  13:56:50  pvachal
 * Added support for nodeAccess resource
 * 
 * Revision 1.5  93/09/17  09:09:59  johan
 * *** empty log message ***
 * 
 * Revision 1.4  93/09/07  16:56:36  trevor
 * *** empty log message ***
 * 
 * Revision 1.3  93/09/02  15:58:57  johan
 * *** empty log message ***
 * 
 * Revision 1.2  93/08/31  16:23:30  johan
 * Revision 1.1  93/08/03  14:26:20  mike
 * Initial 2.7 checkin
 * 
 * $Locker:  $
 */

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <pwd.h>
#include <ctype.h>

#if defined(SYSV) || defined(sun) || defined(sgi) || defined(linux)
#include <unistd.h>
#endif

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <Xm/Xm.h>
#include <Xm/RepType.h>
#if (XmVERSION <= 1) && (XmREVISION < 2)
#include <X11/Protocols.h>
#else
#include <Xm/Protocols.h>
#endif
#include <Xm/RowColumn.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/MwmUtil.h>

#if UIL 
#include <Mrm/MrmPublic.h>
#endif 

#include "tu_runtime.h"

#ifndef  _NO_DRAGDROP_SPEEDUP_
#include <Xm/DragDrop.h>
#endif

extern void endpwent();

/*LINTLIBRARY*/

#ifndef IBMFIX 
/* global variables used by a teleuse application */
externaldef(teleuse)
  tu_bmdefs_p* tu_bmdefs = NULL;
externaldef(teleuse)
  tu_pmdefs_p* tu_pmdefs = NULL;
externaldef(teleuse)
     char                   * tu_application_name = NULL;
externaldef(teleuse)
     char                   * tu_application_class = NULL;
externaldef(teleuse)
     Widget                   tu_global_top_widget = NULL;
externaldef(teleuse)
     XtAppContext             tu_global_app_context = NULL;
externaldef(teleuse)
     int                      tu_verbose_flag = 0;
externaldef(teleuse)
     tu_color_scheme          tu_global_color_scheme = NULL;
#else
extern tu_bmdefs_p*             tu_bmdefs;
extern tu_pmdefs_p*             tu_pmdefs;
extern char                   * tu_application_name;
extern char                   * tu_application_class;
extern Widget                   tu_global_top_widget;
extern XtAppContext             tu_global_app_context;
extern int                      tu_verbose_flag;
extern tu_color_scheme          tu_global_color_scheme;
#endif

static int tu_bmcount = 0;
static int tu_pmcount = 0;


/****************************************************************
 *
 * General utilities ...
 *
 ****************************************************************/

#ifdef MAXLEN
#undef MAXLEN
#endif

#define MAXLEN       1024
#define MAX_ENVL     100

#define EPUT1(x)     (void) fprintf(stderr, (x))
#define EPUT2(x,y)   (void) fprintf(stderr, (x), (y))
#define EPUT3(x,y,z) (void) fprintf(stderr, (x), (y), (z))

#define SKIPSP(s) \
  while (((*(s)) == ' ') || ((*(s)) == '\n') || ((*(s)) == '\t')) (s)++


/****************************************************************
 * getid:
 *     Extract the next alpha-numerical identifier from the
 *     input string.
 ****************************************************************/
static char *getid(pp, echs)
     char **pp;
     char *echs;
{
  char *ps;
  register char *p;
  register char *s;
  register char c;
  int l;

  /* Skip leading blanks */
  SKIPSP(*pp);
  p = ps = *pp;

  /* Find first non-identifer character */
  for (;c = *p;) {
    if (!isalnum(c) && c!='_' && (!echs || !strchr(echs, c)))
      break;
    p ++;
  }

  /* Find the length of the identifier */
  l = p - ps;
  if (l <= 0)
    /* None found */
    return NULL;
  *pp = p;

  /* Return the identifier */
  s = (char *) XtMalloc(l+1);
  (void) strncpy(s, ps, l);
  s[l] = '\0';
  return s;
}




/****************************************************************
 * tu_strdup:
 *     Duplicate a string
 ****************************************************************/
static char *tu_strdup(s)
     char *s;
{
  char *new_s = NULL;

  if (s) {
    new_s = (char *)malloc(strlen(s) + 1);
    if (new_s)
      (void) strcpy(new_s, s);
  }
  return new_s;
}


/****************************************************************
 * expand_symbols:
 *    This function expands home symbols and variables.
 ****************************************************************/
static void expand_symbols(in_str, out_str, maxlen)
     char *in_str;
     char *out_str;
     int maxlen;
{
#define PUT(x) \
  if (maxlen-- > 0) *out_str++ = x;

  char * envv;
  char * envp;
  char * sv;
  struct passwd * pw;

  while (*in_str) 
    switch (*in_str) {

    case '$':
      in_str++;
      sv = envv = getid(&in_str, (char*)NULL);
      if (envv) {
	if (envp = (char *)getenv(envv)) 
	  while (*envp) {
	    PUT(*envp);
	    envp++;
	  }
	else {
	  PUT('$');
	  while (*envv) {
	    PUT(*envv);
	    envv++;
	  }
	}
	XtFree(sv);
      } else
	PUT('$');
      break;

    case '~':
      in_str++;
      if (!((isalnum(*in_str) || *in_str == '_'))) {
	envp = (char *)getenv("HOME");
	while (*envp) {
	  PUT(*envp);
	  envp++;
	}
      } else {
	sv = envv = getid(&in_str, (char*)NULL);
	pw = getpwnam(envv);
	if (pw == NULL || pw->pw_dir == NULL) {
	  PUT('~');
	  while (*envv) {
	    PUT(*envv);
	    envv++;
	  }
	} else {
	  envp = pw->pw_dir;
	  while (*envp) {
	    PUT(*envp);
	    envp++;
	  }
	}
	endpwent();
	XtFree(sv);
      }	     
      break;

    default:
      PUT(*in_str);
      in_str++;
      break;
    }
  PUT('\0');

#undef PUT
}

#ifndef MAXPATHLEN
#define MAXPATHLEN      1024
#endif

#if defined(SYSV) || defined(SVR4)
#define getwd(pathname) getcwd(pathname,MAXPATHLEN)
#endif

/****************************************************************
 * tu_find_file:
 *   Searches for a file with a given file name. If the file is
 *   accessible, a copy of the file name is returned, else NULL
 *   is returned.
 *   If the file name contains an absolute or explicit relative
 *   path, then this path is used. Otherwise, the search directories
 *   given in the path variable are used.
 ****************************************************************/
/*CExtract*/
char *tu_find_file(filename, dlpath)
    String       filename;
    String     * dlpath;
{
  char     buf[MAXPATHLEN];
  char     newfile[MAXPATHLEN];
  char     nfile[MAXPATHLEN];
  char     fn[MAXPATHLEN];
  String   dir;
  String   tmp;
  int      dir_pos;

  if (!filename) 
    return (NULL);
  
  expand_symbols(filename, buf, MAXPATHLEN);
  filename = buf;
  
  if (filename[0] == '/' || filename[0] == '.') {
    if (access(filename, F_OK)==0) {
      if (filename[0] == '.') {
	if ((char*)getwd(newfile) == NULL)
	  return ((char *)NULL);
	
	(void) strcat(newfile, "/");
	(void) strcat(newfile, filename);
	tmp = tu_strdup(newfile);	 
      } else
	tmp = tu_strdup(filename);
      return (tmp);
    }
    return ((char*)NULL);
  }
	
  if (access(filename, F_OK) == 0) {
    if ((char *)getwd(newfile) == NULL)
      return ((char *)NULL);
    
    (void) strcat(newfile, "/");
    (void) strcat(newfile, filename);
    tmp = tu_strdup(newfile);
    return (tmp);
  }
	
  dir = NULL;
  if (dlpath != NULL) {
    (void)strcpy(fn, filename);

    dir_pos = 0;
    while (dir = dlpath[dir_pos++]) {
      (void) sprintf(nfile, "%s/%s", dir, fn);
      expand_symbols(nfile, newfile, MAXPATHLEN);
      if (access(newfile, F_OK)==0) break;
    }
  }
	
  if (!dir) 
    return NULL;
  tmp = tu_strdup(newfile);
  return tmp;
}

#if 0

/****************************************************************
 *
 * CUIL generated code support
 *
 ****************************************************************/

typedef struct _tu_lcw {
  String               name;
  tu_lc_func           value;
  struct _tu_lcw * next;
} tu_lcw_t, * tu_lcw_p;

static tu_lcw_p tu_lcw_list = NULL;

/****************************************************************
 * tu_init_tu_lookup_c_widget:
 *     Initialize widget creation
 ****************************************************************/
void tu_init_tu_lookup_c_widget(name,func)
  char *name;
  tu_lc_func func;
{
  static tu_lcw_p tu_lcw_last = NULL;
  tu_lcw_p ptr;

  ptr = (tu_lcw_p)malloc(sizeof(tu_lcw_t));
  ptr->name = tu_strdup(name);
  ptr->value = func;
  ptr->next = NULL;

  if (tu_lcw_last) {
    tu_lcw_last->next = ptr;
    tu_lcw_last = ptr;
  }
  else tu_lcw_list = tu_lcw_last = ptr;
}

/****************************************************************
 * tu_lookup_c_widget:
 *      Find the create function for the named widget and call
 *      this function to create the widget.
 ****************************************************************/
Widget tu_lookup_c_widget(parent,name,new_name)
    Widget parent;
    char *name;  
    char *new_name;
{
  tu_lcw_p ptr;
  tu_lc_func tu_node_func;

  ptr = tu_lcw_list;

  while (ptr) {
    if (strcmp(name,ptr->name) == 0) {
      tu_node_func = ptr->value;
      return ( (*tu_node_func)(new_name,parent,(Widget **)NULL) );
    }
    ptr = ptr->next;
  }

  return (NULL);
}
#endif

/****************************************************************
 *
 * Typical problems with C++ header files
 *
 ****************************************************************/

void tu_setup_locale()
{
#if !defined(sco) && !defined(venix)
  setlocale(LC_ALL, "");
#endif
}

void tu_line_buffer()
{
#if defined(sun) && !defined(SYSV) && !defined(SVR4)
  setlinebuf(stdout);
  setlinebuf(stderr);
#else
  setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
  setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
#endif
}



/****************************************************************
 *
 * Command line arguments ...
 *
 ****************************************************************/

static int sArgc;
static char **sArgv;


/****************************************************************
 * tu_set_arg_handler:
 *    Stores the command line argument for future access.
 ****************************************************************/
void tu_set_arg_handler(argc, argv)
     int argc;
     char **argv;
{
  int i;
  
  sArgc = argc;
  sArgv = (char **) malloc(sizeof(char *)*(argc+1));
  for (i=0;i<argc;i++) sArgv[i] = argv[i];
  sArgv[argc] = NULL;
}


/****************************************************************
 * tu_get_cmd_argc:
 *     Returns the initial argument count
 ****************************************************************/
int tu_get_cmd_argc()
{
  return sArgc;
}


/****************************************************************
 * tu_get_cmd_argv:
 *     Returns one of the initial arguments
 ****************************************************************/
char *tu_get_cmd_argv(index)
     int index;
{
  if (index >= 0 && index < sArgc)
    return sArgv[index];
  return NULL;
}


/****************************************************************
 * tu_get_cmd_argv2:
 *     Returns all of the initial arguments
 ****************************************************************/
char **tu_get_cmd_argv2()
{
  return sArgv;
}



/****************************************************************
 *
 * Widget creation routine support ...
 *
 ****************************************************************/

static tu_widget_create_fn *createArray;
static int caCount = 0;
static int caAlloced = 0;


/****************************************************************
 * tu_declare_create_method:
 *    The function is used by external routines to declare
 *    a function that is used to create widgets.
 ****************************************************************/
void tu_declare_create_method(create_fn)
     tu_widget_create_fn create_fn;
{
  int i;
  
  /* expand the space if needed */
  if (caCount == caAlloced) {
    caAlloced += 20;
    createArray = (tu_widget_create_fn *) 
      XtRealloc((char *)createArray, caAlloced*sizeof(tu_widget_create_fn));
  }

  for (i=0;i<caCount;i++)
    if (createArray[i] == create_fn)
      return;
  createArray[caCount++] = create_fn;
}


/****************************************************************
 * tu_create_widget: 
 *    This function is used as the gateway to all widget 
 *    creation routines.
 ****************************************************************/
Widget tu_create_widget(template, name, parent, disp, screen, rv)
     char *template;
     char *name;
     Widget parent;
     Display *disp;
     Screen *screen;
     tu_template_descr *rv;
{
  Widget w;
  int i;

#ifndef _NO_DRAGDROP_SPEEDUP_
  if (parent == NULL) XmDropSiteStartUpdate(tu_global_top_widget);
  else XmDropSiteStartUpdate(parent);
#endif

  if (name == NULL)
    name = template;

  for (i=0;i<caCount;i++) {
    w = (*createArray[i])(template, name, parent, disp, screen, rv);
    if (w != NULL)
      return w;
  }

#ifndef _NO_DRAGDROP_SPEEDUP_
  if (parent == NULL) XmDropSiteEndUpdate(tu_global_top_widget);
  else XmDropSiteEndUpdate(parent);
#endif

  return NULL;
}


/****************************************************************
 * tu_lookup_c_widget:
 *    This function is a 2.0 backward compability function
 *    that implements a subset of the tu_create_widget
 *    function.
 ****************************************************************/
Widget tu_lookup_c_widget(parent, name, new_name)
     Widget parent;
     char *name;
     char *new_name;
{
  return tu_create_widget(name, new_name, parent, NULL, NULL, NULL);
}
     


/****************************************************************
 *
 * Widget create hooks ...
 *    This functionality implements a hook function that
 *    can be added and that is called whenever the TeleUSE 
 *    runtime (pcd) or generated code (through cuil)
 *    has created a widget tree. Note the hook is per
 *    widget hierarcy and not per widget.
 *
 ****************************************************************/

static int widCreCount = 0;
static tu_widcre_hook_fn *widCreHooks;
static XtPointer *widCreHookClosures;


/****************************************************************
 * tu_add_widcre_hook:
 *    This function adds a hook function that is called 
 *    whenever a widget is created through TeleUSE.
 ****************************************************************/
void tu_add_widcre_hook(hookfn, closure)
     tu_widcre_hook_fn hookfn;
     XtPointer closure;
{
  widCreCount++;
  widCreHooks = (tu_widcre_hook_fn *)
    XtRealloc((char *)widCreHooks, sizeof(tu_widcre_hook_fn)*widCreCount);
  widCreHookClosures = (XtPointer *)
    XtRealloc((char *)widCreHookClosures, sizeof(XtPointer)*widCreCount);

  widCreHooks[widCreCount-1] = hookfn;
  widCreHookClosures[widCreCount-1] = closure;
}


/****************************************************************
 * tu_remove_widcre_hook:
 *    This function adds a hook function that is called 
 *    whenever a widget is created through TeleUSE.
 ****************************************************************/
void tu_remove_widcre_hook(hookfn, closure)
     tu_widcre_hook_fn hookfn;
     XtPointer closure;
{
  int i;

  for (i=0;i<widCreCount;i++) 
    if ((widCreHooks[i] == hookfn) && (widCreHookClosures[i] == closure)) {
      widCreCount--;
      for (;i<widCreCount;i++) {
	widCreHooks[i] = widCreHooks[i+1];
	widCreHookClosures[i] = widCreHookClosures[i+1];
      }
    }
}


/****************************************************************
 * tu_widcre_invoke_hooks:
 *    Call all defined hooks for the specified widget.
 ****************************************************************/
void tu_widcre_invoke_hooks(w)
     Widget w;
{
  int i;
  tu_widcre_hook_fn *hooks;
  XtPointer *closures;
  int cnt;

  if (widCreCount == 0)
    return;

  cnt = widCreCount;
  hooks = (tu_widcre_hook_fn *) XtMalloc(sizeof(tu_widcre_hook_fn)*cnt);
  closures = (XtPointer *) XtMalloc(sizeof(XtPointer)*cnt);

  for (i=0;i<cnt;i++) {
    hooks[i] = widCreHooks[i];
    closures[i] = widCreHookClosures[i];
  }

  for (i=0;i<cnt;i++) 
    (*hooks[i])(w, closures[i]);

  XtFree((char *)hooks);
  XtFree((char *)closures);
}



/****************************************************************
 *
 * C callback support routines ...
 *
 ****************************************************************/

static int ccb_count = 0;
static int ccb_alloc = 0;
static tu_ccb_def_t *ccb_defs = NULL;


/****************************************************************
 * tu_get_bang_string:
 *    The function returns the first substring in the
 *    bang '!' separated string.
 ****************************************************************/
/*CExtract*/
char *tu_get_bang_string(pstr)
     char ** pstr;
{
  register char * buf;
  register char * ps;
  register int    i;
  int             cl;
  
  cl  = 10;
  buf = (char *) XtMalloc(cl);
  i   = 0;

  ps = *pstr;
  while (*ps != '!' && *ps != '\0') {
    if (*ps == '\\') ps++;
    buf[i++] = *ps++;
    if ((i+1) >= cl) {
      cl += 20;
      buf = XtRealloc(buf, cl);
    }
  }
  buf[i] = '\0';
  if (*ps != '\0')
    ps++;
  *pstr = ps;

  return buf;
}


/****************************************************************
 * tu_ccb_define:
 *      This function is used to define a 'C' callback.
 *      The callback will then be bound directly to the
 *      callback of the widget that uses it.
 ****************************************************************/
/*CExtract*/
void tu_ccb_define(name, cb)
     String name;
     TuCallbackProc cb;
{
  if (ccb_count >= ccb_alloc) {
    ccb_alloc = (ccb_alloc+1)*2;
    ccb_defs  = (tu_ccb_def_t *)
      XtRealloc((char *)ccb_defs, ccb_alloc*sizeof(tu_ccb_def_t));
  }

  ccb_defs[ccb_count].name = tu_strdup(name);
  ccb_defs[ccb_count].cb   = cb;
  ccb_count++;
}


/****************************************************************
 * tu_ccb_find:
 *    Searches through the list of defined C callbacks and
 *    returns a pointer to the last callback with the specified
 *    name.
 ****************************************************************/
/*CExtract*/
tu_ccb_def_t *tu_ccb_find(name)
     String name;
{
  int i;

  for (i=ccb_count-1;i>=0;i--)
    if (strcmp(name, ccb_defs[i].name) == 0)
      return &ccb_defs[i];
  return NULL;
}


/****************************************************************
 * tu_ccb_find_func:
 *     This function translates a string into a function.
 *     The functions known have to be defined through
 *     calls to the functions defined above.
 ****************************************************************/
/*CExtract*/
TuCallbackProc tu_ccb_find_func(name)
     char *name;
{
  tu_ccb_def_t *deft;

  deft = tu_ccb_find(name);
  if (deft) return deft->cb;

  EPUT2("ERROR(Callback)\n  Function '%s' not declared\n", name);
  return NULL;
}


/****************************************************************
 * tu_ccb_get_all:
 *    This function allocates arrays and places all the 
 *    names and functions defined in these arrays.
 ****************************************************************/
/*CExtract*/
int tu_ccb_get_all(pnames, pfuncs)
     char ***pnames;
     TuCallbackProc **pfuncs;
{
  char **names;
  TuCallbackProc *funcs;
  int i;

  names = (char **) 
    XtMalloc(sizeof(char *) * (unsigned) ccb_count + 1);
  funcs = (TuCallbackProc *)
    XtMalloc(sizeof(TuCallbackProc) * (unsigned) ccb_count + 1);

  for (i=0;i<ccb_count;i++) {
    names[i] = ccb_defs[i].name;
    funcs[i] = ccb_defs[i].cb;
  }

  *pnames = names;
  *pfuncs = funcs;
  return ccb_count;
}


/****************************************************************
 * ccb_copy_args:
 *    Copys an arg list.
 ****************************************************************/
static tu_ccb_arg_p ccb_copy_args(args)
     tu_ccb_arg_p args;
{
  tu_ccb_arg_p cargs; 

  if (args == NULL)
    return NULL;

  cargs = (tu_ccb_arg_p) malloc(sizeof(tu_ccb_arg_t));
  cargs->name = tu_strdup(args->name);
  cargs->value = tu_strdup(args->value);
  cargs->next = ccb_copy_args(args->next);
  return cargs;
}


/****************************************************************
 * free_closure:
 ****************************************************************/
static void free_closure(widget, closure, calldata)
     Widget widget;
     XtPointer closure;
     XtPointer calldata;
{
  char *arg = (char *) closure;

  if (arg)
    free(arg);
}


/****************************************************************
 * free_ccbs:
 ****************************************************************/
static void free_ccbs(widget, closure, calldata)
     Widget widget;
     XtPointer closure;
     XtPointer calldata;
{
  tu_ccb_arg_p args = (tu_ccb_arg_p) closure;

  if (args) {
    free_ccbs(widget, (XtPointer)args->next, calldata);
    free(args->name);
    free(args->value);
    free((char *)args);
  }
}


/****************************************************************
 * tu_setup_close_window_callback:
 *    This function takes a callback and closure and
 *    sets up a close window callback.
 ****************************************************************/
/*CExtract*/
void tu_setup_close_window_callback(widget, func, ccb)
     Widget widget;
     TuCallbackProc func;
     tu_ccb_arg_p ccb;
{
  Widget shell;
  Atom wm_protocol;
  Atom delete_window;

  wm_protocol = XmInternAtom(XtDisplay(widget), "WM_PROTOCOLS", False);
  delete_window = XmInternAtom(XtDisplay(widget), "WM_DELETE_WINDOW", False);

  for (shell = widget;
       shell && !XtIsSubclass(shell, vendorShellWidgetClass);
       shell = XtParent(shell));

  if (shell) {
    Arg args[1];
    XtSetArg(args[0], XmNdeleteResponse, XmDO_NOTHING);
    XtSetValues(shell, args, 1);
    XmAddProtocolCallback(shell, wm_protocol, delete_window, 
			  (XtCallbackProc)func, (XtPointer)ccb);
  }
}

/****************************************************************
 * tu_ccb_set_callback:
 *    This function takes a callback name and callback
 *    definition and sets it up.
 ****************************************************************/
/*CExtract*/
void tu_ccb_set_callback(widget, attr, cbdef, tag)
     Widget widget;
     char * attr;
     tu_cd_def cbdef;
     char * tag;
{
  tu_ccb_def_t *cfdef;
  tu_ccb_arg_p args; 
  XtCallbackProc destroy_fn;
  XtPointer closure;

  cfdef = tu_ccb_find(cbdef->name);
  if (cfdef == NULL) {
    (void) fprintf(stderr, "C function '%s' not defined\n", cbdef->name);
    return;
  }

  args = cbdef->args;
  if ((args != NULL) && (args->next == NULL) &&
      (strcmp(args->name, TU_UNTAGGED_CLOSURE) == 0)) {
    destroy_fn = free_closure;
    closure = (XtPointer) tu_strdup(args->value);
  } else {
    destroy_fn = free_ccbs;
    closure = (XtPointer) ccb_copy_args(args);
  }

  if (strcmp(attr, MrmNcreateCallback) == 0) {
    (*cfdef->cb)(widget, closure, NULL);
    if (closure)
      (*destroy_fn)(widget, closure, NULL);
    return;
  }

  if (strcmp(attr, TuNcloseWindowCallback) == 0) {
    tu_setup_close_window_callback(widget, cfdef->cb, (tu_ccb_arg_p)closure);
  } else 
    XtAddCallback(widget, attr, (XtCallbackProc)cfdef->cb, closure);
  
  XtAddCallback(widget, XtNdestroyCallback, destroy_fn, closure);
}




#if UIL

/****************************************************************
 *
 * Callback name binding to UIL
 *
 ****************************************************************/

#ifndef TuNcloseWindowCallback
#define TuNcloseWindowCallback "closeWindowCallback"
#endif

/****************************************************************
 * tu_uil_ccb:
 *    UIL callback, this function should decode the 
 *    closure argument and call the the correct function
 *    (with appropriate closure) and define the callback. 
 *    This function is called at a create callback.
 ****************************************************************/
/*CExtract*/
void tu_uil_ccb(widget, closure, calldata)
    Widget    widget;
    XtPointer closure;
    XtPointer calldata;
{
  tu_ccb_arg_p ccb = NULL;
  tu_ccb_arg_p * pccb = &ccb;
  tu_ccb_arg_p new;
  TuCallbackProc func;
  XtCallbackProc destroy_fn;
  char * callback_name;
  char * func_name;
  char * ps;

  ps = (char *) closure;
  callback_name = tu_get_bang_string(&ps);

  if (*ps == '\0') {
    EPUT1("ERROR(UIL callback)\n");
    EPUT2("  callback string specification error string = '%s'\n", closure);
    return;
  }

  func_name = tu_get_bang_string(&ps);
  func = tu_ccb_find_func(func_name);
  if (func == NULL) {
    XtFree(func_name);
    XtFree(callback_name);
    return;
  }
  XtFree(func_name);

  while (*ps != '\0') {
    new = (tu_ccb_arg_p) XtMalloc(sizeof(*new));
    new->next = NULL;
    new->name = tu_get_bang_string(&ps);
    new->value = tu_get_bang_string(&ps);
    *pccb = new;
    pccb = &new->next;
  }

  if ((ccb != NULL) && (ccb->next == NULL) &&
      (strcmp(ccb->name, TU_UNTAGGED_CLOSURE) == 0)) {
    char * clstr = ccb->value;
    free(ccb->name); free((char *)ccb);
    ccb = (tu_ccb_arg_p) clstr;
    destroy_fn = free_closure;
  } else
    destroy_fn = free_ccbs;

  if (strcmp(callback_name, MrmNcreateCallback) == 0) {
    (*func)(widget, ccb, calldata);
    if (ccb)
      (*destroy_fn)(widget, (XtPointer)ccb, NULL);
    XtFree(callback_name);
    return;
  }

  if (strcmp(callback_name, TuNcloseWindowCallback) == 0) {
    tu_setup_close_window_callback(widget, func, ccb);
  } else
    XtAddCallback(widget, callback_name, (XtCallbackProc)func, (XtPointer)ccb);

  XtAddCallback(widget, XtNdestroyCallback, destroy_fn, (XtPointer)ccb);
  XtFree(callback_name);
}


/****************************************************************
 * tu_register_ccb_mrm:
 *    Register all C functions to mrm.
 ****************************************************************/
/*CExtract*/
void tu_uil_register_mrm_ccb()
{
  char           ** name;
  TuCallbackProc * cb;
  MrmRegisterArg * mrmargs;
  int              cnt;
  int              i;

  cnt = tu_ccb_get_all(&name, &cb);
  mrmargs = (MrmRegisterArg *) 
    XtMalloc(sizeof(MrmRegisterArg)*(unsigned)cnt + 1);

  for (i=0;i<cnt;i++) {
    mrmargs[i].name  = (String) name[i];
    mrmargs[i].value = (XtPointer) cb[i];
  }
  
  /* register in Mrm */
  if (MrmRegisterNames(mrmargs, cnt) != MrmSUCCESS) 
    EPUT1("ERROR(MrmRegisterNames)\n  Could not register names for Mrm\n");

  XtFree((char *)name);
  XtFree((char *)cb);
  XtFree((char *)mrmargs);
}


/****************************************************************
 * tu_add_tabgroup:
 *    This is a callback that is used to add a widget to a 
 *    tabgroup.
 ****************************************************************/
/*CExtract*/
void tu_uil_add_tabgroup(widget, closure, calldata)
     Widget    widget;
     XtPointer closure;
     XtPointer calldata;
{
  XmAddTabGroup(widget);
}



/****************************************************************
 *
 * Installing accelerators ...
 *
 ****************************************************************/

typedef struct {
  Widget   widget;
  char    *accelerator_string;
} WidgetAccCol;

static int n_collect = 0;
static int n_alloc = 0;
static WidgetAccCol *collection = NULL;


/****************************************************************
 * tu_uil_collect_accelerators:
 *    This function sets the collector to start remembering
 *    all accelerators that are used.
 ****************************************************************/
/*CExtract*/
void tu_uil_collect_accelerators()
{
  if (n_alloc > 0)
    return;
  n_alloc = 10;
  n_collect = 0;
  collection = (WidgetAccCol *) XtMalloc(n_alloc*sizeof(WidgetAccCol));
}


/****************************************************************
 * tu_uil_set_accelerators:
 *    This is a callback that is used to set accelerators
 *    for a widget.
 ****************************************************************/
/*CExtract*/
void tu_uil_set_accelerators(widget, closure, calldata)
     Widget    widget;
     XtPointer closure;
     XtPointer calldata;
{
  if (n_alloc) {
    if (n_collect == n_alloc) {
      n_alloc *= 2;
      collection = (WidgetAccCol *) 
	realloc((char *)collection, n_alloc*sizeof(WidgetAccCol));
    }
    collection[n_collect].widget = widget;
    collection[n_collect].accelerator_string = (char *)closure;
    n_collect++;
  }
}


/****************************************************************
 * tu_uil_install_accelerators:
 *    Take all the noted accelerators and install them.
 ****************************************************************/
/*CExtract*/
void tu_uil_install_accelerators()
{
  int i;

  if (n_alloc) {
    for (i=0;i<n_collect;i++)
      tu_acc_set_accelerators(collection[i].widget, 
			      collection[i].accelerator_string);
    free((char *)collection);
    n_alloc = 0;
  }
}


/****************************************************************
 * tu_uil_set_callback_attr:
 *    This function takes a bang string with two parts,
 *    the attribute name and value.
 ****************************************************************/
/*CExtract*/
void tu_uil_set_callback_attr(widget, closure, calldata)
     Widget widget;
     XtPointer closure;
     XtPointer calldata;
{
  char *attr_name;
  char *attr_value;
  char *attr_type;
  char *ps;
  Arg arg[1];
  long value;
  Boolean free_val;
  Boolean free_type;

  ps = (char *)closure;
  attr_name = tu_get_bang_string(&ps);
  attr_type = tu_get_bang_string(&ps);
  free_type = True;
  attr_value = tu_get_bang_string(&ps);
  free_val = True;

  if ((strcmp(attr_type, XmRVerticalDimension) == 0) ||
      (strcmp(attr_type, XmRHorizontalDimension) == 0)) {
    XtFree(attr_type);
    attr_type = XmRDimension;
    free_type = False;
  }
  else if ((strcmp(attr_type, XmRVerticalPosition) == 0) ||
      (strcmp(attr_type, XmRHorizontalPosition) == 0)) {
    XtFree(attr_type);
    attr_type = XmRPosition;
    free_type = False;
  }

  if (strcmp(attr_type, "String") != 0) {
    XrmValue src;
    XrmValue dst;
    src.addr = (XtPointer) attr_value;
    src.size = strlen(attr_value)+1;

    XtConvert(widget, "String", &src, attr_type, &dst);
    if (dst.addr == NULL)
      goto cleanup;

    /* Take care of shorter data */
    if (dst.size == sizeof(char))
      value = *((char*)dst.addr);
    else if (dst.size == sizeof(short))
      value = *((short*)dst.addr);
    else if (dst.size == sizeof(int))
      value = *((int*)dst.addr);
    else
      value = (long) dst.addr;
    XtFree (attr_value);
    attr_value = (char *) value;
    free_val = False;
  }

  XtSetArg(arg[0], attr_name, attr_value);
  XtSetValues(widget, arg, 1);

cleanup:
  XtFree(attr_name);
  if (free_type)
    XtFree(attr_type);
  if (free_val)
    XtFree(attr_value);
}


/****************************************************************
 * set_submenu_id:
 *   This callback is used to set the XmNsubMenuId attribute
 *   from UIL when the named widget is NOT UIL supported.
 ****************************************************************/
static void set_submenu_id(widget, ccb, calldata)
     Widget widget;
     tu_ccb_arg_p ccb;
     XtPointer calldata;
{
  Widget       owner;
  Widget       parent;
  Arg          args[5];
  int          n;

  owner = NULL;

  while (ccb) {
    if (strcmp(ccb->name, "owner") == 0) {
      parent = XtParent(widget);
      if (parent)
	parent = XtParent(widget);
      if (parent) {
	owner = tu_find_widget(parent, ccb->value, TU_FW_NORMAL, 0);
      }
    }
    ccb = ccb->next;
  }

  if (owner) {
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, widget); n++;
    XtSetValues(owner, args, n);
  }
}


/****************************************************************
 * set_main_window_area:
 *   UIL callback, this function takes care of setting
 *   a window area for a XmMainWindow widget.
 *   It exists only because the XmMainWindow widget is buggy.
 *   This function is called when a widget child to a
 *   XmMainWindw is created.
 ****************************************************************/
static void set_main_window_area(widget, ccb, calldata)
     Widget    widget;
     tu_ccb_arg_p ccb;
     XtPointer calldata;
{
  Widget       main_window;
  Widget       clip_window;
  Widget       parent;
  Widget       child;
  Arg          args[10];
  int          n;
  char       * attribute = NULL;
  int          mainw_up = 0;
  int          child_up = 0;

  /* The attribute to set on the XmMainWindow is stored in the
     callback pointer */

 for (; ccb; ccb = ccb->next) 
   if (strcmp(ccb->name, "attribute") == 0) 
     attribute = ccb->value;
  
  if (attribute == NULL) {
    EPUT1("ERROR(SetMainWindowAreas)\n");
    EPUT1("  No window area attribute to set.\n");
    return;
  }

  /* Traverse the widget tree upwards until we find a widget which
     is of the xmMainWindowWidgetClass. */

  for (main_window = widget;
       main_window && !XtIsSubclass(main_window, xmMainWindowWidgetClass);
       main_window = XtParent(main_window))
    mainw_up++;

  if (main_window == NULL) {
    EPUT1("ERROR(SetMainWindowAreas)\n");
    EPUT1("  Could not find XmMainWindow widget\n");
    EPUT2("  to set '%s' attribute on.\n", attribute);
    return;
  }

  /* Get hold of the clipWindow for the XmMainWindow,
     we do not want to set that as a window area. */

  n = 0;
  XtSetArg(args[n], XmNclipWindow, &clip_window); n++;
  XtGetValues(main_window, args, n);

  /* Make sure that the child's parent is the same as the widget */

  for (child = widget, parent = XtParent(child);
       child && (parent != main_window) && (parent != clip_window);
       child = parent) {
    if (parent)
      parent = XtParent(parent);
    child_up++;
  }

  if (child == NULL) {
    EPUT1("ERROR(SetMainWindowAreas)\n");
    EPUT1("  Could not find XmMainWindow widget child\n");
    EPUT2("  to set as '%s' attribute.\n", attribute);
    return;
  }

  if (main_window == child) {
    EPUT1("ERROR(SetMainWindowAreas)\n");
    EPUT1("  XmMainWindow widget and its child is same\n");
    return;
  }

  if (child_up >= mainw_up) {
    EPUT1("ERROR(SetMainWindowAreas)\n");
    EPUT1("  XmMainWindow widget is a child of its own child\n");
    return;
  }

  /* OK, we have a child and its parent is the widget, set the 
     window area attribute. */

  n = 0;
  XtSetArg(args[n], attribute, child); n++;
  XtSetValues(main_window, args, n);
}


/****************************************************************
 * tu_uil_init:
 *   This function defines the functions that TeleUSE needs
 *   to have defined for the UIL code to function properly.
 *   Observe that this is only what is needed when generating 
 *   PURE UIL code. If you want to be able to send D events,
 *   you have to define the tu_send_devent function, too.
 ****************************************************************/
/*CExtract*/
void tu_uil_init()
{
  static MrmRegisterArg mrmargs[] =
    { 
      { "tu_uil_ccb",               (XtPointer) tu_uil_ccb                },
      { "tu_uil_add_tabgroup",      (XtPointer) tu_uil_add_tabgroup       },
      { "tu_uil_set_callback_attr", (XtPointer) tu_uil_set_callback_attr  },
      { "tu_uil_set_accelerators",  (XtPointer) tu_uil_set_accelerators   },
    };
  
  MrmRegisterNames(mrmargs, XtNumber(mrmargs));

  tu_ccb_define("SetSubMenuId",      set_submenu_id);
  tu_ccb_define("SetMainWindowArea", set_main_window_area);
}

#endif /* UIL */



/****************************************************************
 *
 * Logical display handling
 *
 ****************************************************************/

struct display_rec {
  char *log_name;
  char *phys_name;
  Display *dpy;
};

/* Hold our displays */
static struct display_rec *dpy_rec = NULL;
static unsigned int dpy_cnt = 0;
static struct display_rec *default_dpy_rec = NULL;


/****************************************************************
 * tu_disp_set_dpy: 
 *   Define a logical/physical display mapping.
 ****************************************************************/
/*CExtract*/
void tu_disp_set_dpy(log_name, phys_name, dpy)
     char *log_name;
     char *phys_name;
     Display *dpy;
{
  int dpos;

  if (default_dpy_rec != NULL)
    dpos = default_dpy_rec - dpy_rec;

  if (dpy_rec == NULL)
    dpy_rec = (struct display_rec *) malloc(sizeof(*dpy_rec));
  else
    dpy_rec = (struct display_rec *) realloc(dpy_rec,
					     sizeof(*dpy_rec)*(dpy_cnt+1));

  dpy_rec[dpy_cnt].log_name = tu_strdup(log_name);
  dpy_rec[dpy_cnt].phys_name = tu_strdup(phys_name);
  dpy_rec[dpy_cnt].dpy = dpy;

  if (default_dpy_rec == NULL)
    default_dpy_rec = &dpy_rec[dpy_cnt];
  else
    default_dpy_rec = &dpy_rec[dpos];

  dpy_cnt++;
}


/****************************************************************
 * tu_disp_get_dpy: 
 *   Get the display of a logical name. NULL gives default 
 *   display.
 ****************************************************************/
/*CExtract*/
Display *tu_disp_get_dpy(log_name)
     char *log_name;
{
  int i;

  if (log_name == NULL)
    if (default_dpy_rec) return default_dpy_rec->dpy;
    else return NULL;

  for (i=0;i<dpy_cnt;i++)
    if (strcmp(log_name, dpy_rec[i].log_name) == 0)
      return dpy_rec[i].dpy;
  return NULL;
}


/****************************************************************
 * tu_disp_set_default: 
 *   Sets the default display of the application.
 ****************************************************************/
/*CExtract*/
void tu_disp_set_default(log_name)
     char *log_name;
{
  int i;

  if (log_name == NULL)
    return;

  for (i=0;i<dpy_cnt;i++)
    if (strcmp(log_name, dpy_rec[i].log_name) == 0)
      default_dpy_rec = &dpy_rec[i];
}


/****************************************************************
 * tu_disp_set_default_dpy: 
 *   Sets the default display of the application.
 ****************************************************************/
/*CExtract*/
void tu_disp_set_default_dpy(dpy)
     Display *dpy;
{
  int i;

  if (dpy == NULL)
    return;

  for (i=0;i<dpy_cnt;i++)
    if (dpy == dpy_rec[i].dpy) {
      default_dpy_rec = &dpy_rec[i];
      return;
    }
}


/****************************************************************
 * tu_disp_get_logical_name: 
 *   Get the logical name of this display
 ****************************************************************/
/*CExtract*/
char *tu_disp_get_logical_name(dpy)
     Display *dpy;
{
  int i;

  for (i=0;i<dpy_cnt;i++)
    if (dpy == dpy_rec[i].dpy) 
      return dpy_rec[i].log_name;
  return NULL;
}


/****************************************************************
 * tu_disp_close: 
 *   The function removes the information about one
 *   display from the data structures.
 ****************************************************************/
/*CExtract*/
void tu_disp_close(dpy)
     Display *dpy;
{
  int i;
  int j;
  int before;

  if (default_dpy_rec->dpy == dpy)
    default_dpy_rec = NULL;

  before = 0;
  for (i=0;i<dpy_cnt;i++)
    if (dpy == dpy_rec[i].dpy) {

      if (dpy_rec[i].log_name)
 	free((char *)dpy_rec[i].log_name);
      if (dpy_rec[i].phys_name)
 	free((char *)dpy_rec[i].phys_name);

      for (j=i;j<dpy_cnt;j++)
	dpy_rec[j] = dpy_rec[j+1];
      dpy_cnt--;

      if (!before && default_dpy_rec)
 	default_dpy_rec--;

      return;
    } else
      if (default_dpy_rec == &dpy_rec[i])
	before = 1;
}


/****************************************************************
 * tu_disp_get_temp_widget:
 *   The function returns a widget with the correct display
 *   and screen set.
 ****************************************************************/
/*CExtract*/
Widget tu_disp_get_temp_widget(log_name, screen_num)
     char *log_name;
     char *screen_num;
{
  int sno;
  Display *dpy;
  Screen *screen;
  static Display *last_display = NULL;
  static Screen *last_screen = NULL;
  static Widget last_widget = NULL;
  Arg args[10];
  int n;

  dpy = tu_disp_get_dpy(log_name);
  if (dpy == NULL) {
    EPUT1("ERROR(GetTempWidget)\n");
    EPUT1("  Could not get a display for converting a value\n");
    return NULL;
  }

  if (screen_num) {
    sno = strtol(screen_num, (char**)NULL, 10);
    screen = ScreenOfDisplay(dpy, sno);
  } else
    screen = DefaultScreenOfDisplay(dpy);

  if (dpy == last_display && screen == last_screen)
    return last_widget;

  if (last_widget != NULL)
    XtDestroyWidget(last_widget);

  n = 0;
  XtSetArg(args[n], XtNscreen, screen); n++;
  last_widget =
    XtAppCreateShell(tu_application_name, tu_application_class,
		     applicationShellWidgetClass,
		     dpy, args, n);

  last_display = dpy;
  last_screen = screen;
  return last_widget;
}



/****************************************************************
 *
 * Predefined operation for menus (actions and callbacks).
 * Xm popup support ...
 *
 ****************************************************************/


/****************************************************************
 * tu_menu_popup:
 *    This function pops up a menu. The arguments should
 *    be a XmMenuShell or an XmPopupMenu widget. 
 ****************************************************************/
/*CExtract*/
void tu_menu_popup(widget, name, xevent)
    Widget   widget;
    char   * name;
    XEvent * xevent;
{
  Widget          cwidget;
  Arg             args[10];
  unsigned char   type;
  char          * wname;
  WidgetClass     wclass;

  wname = tu_widget_name(widget);
  wclass = tu_widget_class(widget);
  if (xmRowColumnWidgetClass &&
      XtIsSubclass(widget, xmRowColumnWidgetClass)) {

    /* Get the rowcolumn type */
    XtSetArg(args[0], XmNrowColumnType, &type);
    XtGetValues(widget, args, 1);

    if (type != XmMENU_POPUP) 
      goto error;

    if (xevent)
      XmMenuPosition(widget, &xevent->xbutton);
    XtManageChild(widget);
    return;
  }

  if (xmMenuShellWidgetClass) {
    if (XtIsSubclass(widget, xmMenuShellWidgetClass)) {
      if (name != NULL)
	cwidget = tu_find_widget(widget, name, TU_FW_ANY, 1);
      else {
	cwidget = tu_find_widget(widget, "menu", TU_FW_ANY, 1);
	if (cwidget == NULL)
	  cwidget = tu_first_widget_child(widget);
      }
      if (cwidget == NULL)
	goto error;
      if (!XtIsSubclass(cwidget, xmRowColumnWidgetClass)) 
	goto error;
      if (xevent)
	XmMenuPosition(cwidget, &xevent->xbutton);
      XtManageChild(cwidget);
      return;
    }
  }

  if (xmDialogShellWidgetClass) {
    if (!XtIsSubclass(widget, xmDialogShellWidgetClass)) {
      name = tu_widget_name(widget);
      widget = XtParent(widget);
      if (widget == NULL)
	goto error;
    }
    
    if (!XtIsSubclass(widget, xmDialogShellWidgetClass)) 
      goto error;
  }

  if (name != NULL) 
    cwidget = tu_find_widget(widget, name, TU_FW_ANY, 2);
  else {
    cwidget = tu_find_widget(widget, "menu", TU_FW_ANY, 2);
    if (cwidget == NULL)
      cwidget = tu_first_widget_child(widget);
  }
  if (cwidget == NULL)
    goto error;
  XtManageChild(cwidget);
  return;

 error:
  EPUT1("ERROR(menu_support):\n");
  EPUT3("  Menu to be popped up (from widget '%s:%s')\n", wname,
	tu_class_name(wclass));
  EPUT1("  with no acceptable widget.\n");
  if (name != NULL)
    EPUT2("  Named widget that was searched for was '%s'\n", name);
}


/****************************************************************
 * tu_set_kbd_focus:
 *   This is an event handler that sets focus to the
 *   node specified throught the closure (the name of
 *   the node). The event handler is removed when it
 *   is invoked, so it is a one-shot thing.
 ****************************************************************/
void tu_set_kbd_focus(w, closure, event, cont)
     Widget w;
     XtPointer closure;
     XEvent *event;
     Boolean *cont;
{
  Widget child = (Widget)closure;
  Boolean traversalOn;
  Arg args[1];

  if (event->type == MapNotify) {
    XtRemoveEventHandler(w, StructureNotifyMask, False,
			 tu_set_kbd_focus, closure);
    traversalOn = False;
    XtSetArg(args[0], XmNtraversalOn, &traversalOn);
    XtGetValues(child, args, 1);
    if (!XtIsManaged(child) ||
	!XtIsRealized(child) ||
	!traversalOn)
      goto error;
    (void) XmProcessTraversal(child, XmTRAVERSE_CURRENT);
  }
  return;

 error:
  EPUT1("ERROR(kbd_focus):\n");
  EPUT2("  Child '%s' are eligible for focus.\n", tu_widget_name(child));
  return;
}


/****************************************************************
 * tu_handle_popup_menu:
 *   This is an action handler that pops up a menu if it 
 *   is enabled and the correct button was pressed.
 ****************************************************************/
void tu_handle_popup_menu(w, closure, event, cont)
     Widget w;
     XtPointer closure;
     XEvent *event;
     Boolean *cont;
{
  Widget menu = (Widget) closure;
  Boolean enabled;
  int whichButton;
  unsigned char rct;
  Arg args[10];

  if (event->type != ButtonPress)
    return;

  XtSetArg(args[0], XmNpopupEnabled, &enabled);
  XtSetArg(args[1], XmNwhichButton, &whichButton);
  XtSetArg(args[2], XmNrowColumnType, &rct);
  XtGetValues(menu, args, 3);

  if ((event->xbutton.button != whichButton) || 
      !enabled || 
      (rct != XmMENU_POPUP))
    return;

  XmMenuPosition(menu, &event->xbutton);
  XtManageChild(menu);
}


/**************************************************************** 
 * tu_menu_popdown:
 *     This function pops down a menu for any widget
 *     within the menu hierarchy.
 ****************************************************************/
/*CExtract*/
void tu_menu_popdown(widget)
     Widget widget;
{
  if (xmDialogShellWidgetClass == NULL ||
      xmMenuShellWidgetClass == NULL)
    return;

  while (widget != NULL) {
    if (XtIsSubclass(widget, xmDialogShellWidgetClass) ||
	XtIsSubclass(widget, xmMenuShellWidgetClass)) {	
      widget = tu_first_managed(widget);
      if (widget) XtUnmanageChild(widget);
      return;
    }
    widget = XtParent(widget);
  }
}


#define MAXWLS 100

/****************************************************************
 * tu_menu_set_enabled_popup:
 *    The function takes a popup menu and disables all other
 *    popups in a hierarchy.
 ****************************************************************/
void tu_menu_set_enabled_popup(menu)
     Widget menu;
{
  Widget   sh;
  Widget   initiator;
  int      cnt1, cnt2;
  int      i, j;
  Arg      args[1];
  Boolean  cv;
  Widget   pl[MAXWLS];
  Widget   nl[MAXWLS];

  sh = XtParent(menu);
  if (!XtIsSubclass(sh, xmMenuShellWidgetClass)) {
    EPUT1("ERROR(tu_menu_set_enabled_popup)\n");
    EPUT1("    menu structure is incorrect\n");
    return;
  }

  initiator = XtParent(sh);
  (void) tu_widget_children(initiator, pl, &cnt1, MAXWLS, TU_FW_POPUP);
  for (i=0;i<cnt1;i++) 
    if (XtIsSubclass(pl[i], xmMenuShellWidgetClass)) {
      (void) tu_widget_children(pl[i], nl, &cnt2, MAXWLS, TU_FW_NORMAL);
      for (j=0;j<cnt2;j++) 
	if (XtIsSubclass(nl[j], xmRowColumnWidgetClass)) {
	  XtSetArg(args[0], XmNpopupEnabled, &cv);
	  XtGetValues(nl[j], args, 1);
	  if (cv) {
	    XtSetArg(args[0], XmNpopupEnabled, False);
	    XtSetValues(nl[j], args, 1);
	  }
	}
    }

  XtSetArg(args[0], XmNpopupEnabled, &cv);
  XtGetValues(menu, args, 1);
  if (!cv) {
    XtSetArg(args[0], XmNpopupEnabled, True);
    XtSetValues(menu, args, 1);
  }
}


/****************************************************************
 * action_popup:
 *    This function is an action function that will pop up
 *    a motif menu.
 *    The named popup child could either be a XmPopupMenu or
 *    an XmMenuShell.
 *    In the case that the widget class is XmMenuShell, a child
 *    of the XmMenuShell should be named; otherwise, the first
 *    child will be used.
 *    If the widget class is XmPopupMenu, which is a XmMenuShell
 *    with a XmRowColumn widget as a child, then the name should
 *    be the name of the XmRowColumn widget.
 ****************************************************************/
static void xm_action_popup(widget, xev, strings, num_str)
     Widget     widget;
     XEvent   * xev;
     String   * strings;
     Cardinal * num_str;
{
  Widget   pop_widget;
  char   * name = NULL;

  if ((*num_str) >= 1) {
    name = strings[0];
    pop_widget = tu_find_widget(widget, name, TU_FW_ANY, 2);   /* MenuShell */
    if (pop_widget == NULL) {
      pop_widget = tu_find_widget(widget, name, TU_FW_ANY, 3); /* RowColumn */
    }
  }
  else {
    pop_widget = tu_find_widget(widget, "menu", TU_FW_ANY, 2); /* MenuShell */
    if (pop_widget == NULL) {
      pop_widget = tu_find_widget(widget, name, TU_FW_ANY, 3); /* RowColumn */
    }
    if (pop_widget == NULL) {
      /* No MenuShell or RowColumn, get first popup child which
       * is a row column and is enabled.
       */

      Widget pl[MAXWLS];
      Widget nl[MAXWLS];
      int i, icnt, j, jcnt;
      Boolean pe;
      unsigned char rct;
      Arg args[2];
      
      pop_widget = NULL;
      (void) tu_widget_children(widget, pl, &icnt, MAXWLS, TU_FW_POPUP);
      for (i=0;i<icnt;i++) 
	if (XtIsSubclass(pl[i], xmMenuShellWidgetClass)) {
	  (void) tu_widget_children(pl[i], nl, &jcnt, MAXWLS, TU_FW_NORMAL);
	  for (j=0;j<jcnt;j++)
	    if (XtIsSubclass(nl[j], xmRowColumnWidgetClass)) {
	      XtSetArg(args[0], XmNrowColumnType, &rct);
	      XtSetArg(args[1], XmNpopupEnabled, &pe);
	      XtGetValues(nl[j], args, 2);
	      if (rct == XmMENU_POPUP && pe) {
		pop_widget = nl[j];
		break;
	      }
	    }
	}
    }
  }
  if (pop_widget != NULL) 
    tu_menu_popup(pop_widget, name, xev);
}


/****************************************************************
 * action_popdown:
 *     This function pop downs any Motif menu.
 ****************************************************************/
static void xm_action_popdown(widget, xev, strings, num_str)
     Widget     widget;
     XEvent   * xev;
     String   * strings;
     Cardinal * num_str;
{
  tu_menu_popdown(widget);
}


/****************************************************************
 * action_manage:
 *    This function is an action function that will manage a widget.
 *    If the widget is a dialog shell, its child is managed.
 ****************************************************************/
static void xm_action_manage(widget, xev, strings, num_str)
     Widget     widget;
     XEvent   * xev;
     String   * strings;
     Cardinal * num_str;
{
  Widget   pop_widget;
  char   * name = NULL;

  if ((*num_str) >= 1) {
    name = strings[0];
    pop_widget = tu_find_widget(widget, name, TU_FW_ANY, 2);  
    if (pop_widget == NULL) {
      pop_widget = tu_find_widget(widget, name, TU_FW_ANY, 3); /* Xm*Dialog */
    }
  }
  else {
    pop_widget = 
      tu_find_widget_by_class(widget, "XmDialogShell", TU_FW_POPUP, 2);
  }
  if (pop_widget != NULL) {
    if (xmDialogShellWidgetClass &&
	XtIsSubclass(pop_widget, xmDialogShellWidgetClass))
      pop_widget = tu_first_child(pop_widget);
    if (pop_widget)
      XtManageChild(pop_widget);
  }
}


/****************************************************************
 * action_unmanage:
 *     This function unmanages a widget.
 ****************************************************************/
static void xm_action_unmanage(widget, xev, strings, num_str)
     Widget     widget;
     XEvent   * xev;
     String   * strings;
     Cardinal * num_str;
{
  Widget   pop_widget;
  char   * name = NULL;

  if ((*num_str) >= 1) {
    name = strings[0];
    pop_widget = tu_find_widget(widget, name, TU_FW_ANY, 2);  
    if (pop_widget == NULL) {
      pop_widget = tu_find_widget(widget, name, TU_FW_ANY, 3); /* Xm*Dialog */
    }
  }
  else {
    pop_widget = 
	tu_find_parent_by_class(widget, "XmDialogShell");
  }
  if (pop_widget != NULL) {
    if (xmDialogShellWidgetClass &&
	XtIsSubclass(pop_widget, xmDialogShellWidgetClass))
      pop_widget = tu_first_child(pop_widget);
    if (pop_widget)
      XtUnmanageChild(pop_widget);
  }
}


/****************************************************************
 * find_pn_child:
 *   This function has to hide the fact the certain 
 *   convenience widgets put in a MenuShell/DialogShell
 *   that is not seen now and then.
 ****************************************************************/
static Widget find_pn_child(w, name)
     Widget w;
     char * name;
{
  Widget   wc;
  Widget * wl;
  int      cnt;
  int      i;

  wc = tu_find_widget(w, name, TU_FW_POPUP, 1);
  if (wc != NULL) return wc;

  wl = tu_widget_children(w, (Widget *)NULL, &cnt, 0, TU_FW_POPUP);
  if (cnt) {
    for (i=0;i<cnt;i++)
      if (wc = tu_find_widget(wl[i], name, TU_FW_NORMAL, 1)) {
	free((char *) wl);
	return wc;
      }
    free((char *) wl);
  }    
  return NULL;
}


/****************************************************************
 * tu_xm_callback_unmanage:
 *     This function unmanages a widget.
 ****************************************************************/
void tu_xm_callback_unmanage(w, closure, calldata)
     Widget w;
     XtPointer closure;
     XtPointer calldata;
{
  XmAnyCallbackStruct *p = (XmAnyCallbackStruct *) calldata;
  XEvent *xev = p->event;
  tu_ccb_arg_p arg = (tu_ccb_arg_p) closure;
  char *name = NULL;
  Widget iw = w;

  name = NULL;
  if (arg == NULL) {
    w = tu_find_parent_by_class(w, "XmDialogShell");
    if (w == NULL) goto error;
  } else
    while (arg)
      {
	if (strcmp(arg->name, "find")==0) {
	  w = tu_widget_root_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_ANY, 0);
	}
	else if (strcmp(arg->name, "tf")==0) {
	  w = tu_widget_top_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	}
	else if (strcmp(arg->name, "rf")==0) {
	  w = tu_widget_root_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	}
	else if (strcmp(arg->name, "fn")==0) 
	  w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	else if (strcmp(arg->name, "pn")==0) 
	  w = find_pn_child(w, arg->value);
	else if (strcmp(arg->name, "menu")==0)
	  name = arg->value;
	else
	  w = NULL;
	if (w == NULL)
	  goto error;
	arg = arg->next;
      }
  

  if (w != NULL) {
    if (xmDialogShellWidgetClass &&
	XtIsSubclass(w, xmDialogShellWidgetClass))
      w = tu_first_child(w);
    if (w) {
      XtUnmanageChild(w);
      return;
    }
  }
 error:
  EPUT1("ERROR(unmanage_widget):\n");
  EPUT2("  Widget to be unmanaged from widget '%s'\n", tu_widget_name(iw));
  EPUT1("  and no acceptable widget could be found.\n");
}


/****************************************************************
 * tu_xm_callback_manage:
 *    This function is a callback function that manages a widget.
 *    If the widget is a dialog shell, its child is managed.
 ****************************************************************/
void tu_xm_callback_manage(w, closure, calldata)
     Widget w;
     XtPointer closure;
     XtPointer calldata;
{
  XmAnyCallbackStruct *p = (XmAnyCallbackStruct *) calldata;
  XEvent *xev = p->event;
  tu_ccb_arg_p arg = (tu_ccb_arg_p) closure;
  char *name = NULL;
  Widget iw = w;

  name = NULL;
  if (arg == NULL) {
    w = tu_find_widget_by_class(w, "XmDialogShell", TU_FW_POPUP, 2);
    if (w == NULL) goto error;
  } else
    while (arg)
      {
	if (strcmp(arg->name, "find")==0) {
	  w = tu_widget_root_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_ANY, 0);
	}
	else if (strcmp(arg->name, "tf")==0) {
	  w = tu_widget_top_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	}
	else if (strcmp(arg->name, "rf")==0) {
	  w = tu_widget_root_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	}
	else if (strcmp(arg->name, "fn")==0) 
	  w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	else if (strcmp(arg->name, "pn")==0) 
	  w = find_pn_child(w, arg->value);
	else if (strcmp(arg->name, "menu")==0)
	  name = arg->value;
	else
	  w = NULL;
	if (w == NULL)
	  goto error;
	arg = arg->next;
      }
  

  if (w != NULL) {
    if (xmDialogShellWidgetClass &&
	XtIsSubclass(w, xmDialogShellWidgetClass))
      w = tu_first_child(w);
    if (w) {
      XtManageChild(w);
      return;
    }
  }

 error:
  EPUT1("ERROR(manage_widget):\n");
  EPUT2("  Widget to be managed from widget '%s'\n", tu_widget_name(iw));
  EPUT1("  and no acceptable widget could be found.\n");
}


/****************************************************************
 * tu_xm_callback_popup:
 *    A callback version of menu popup. This function
 *    assumes that the callback is invoked from a 
 *    Motif widget, i.e., a reason structure is assumed.
 ****************************************************************/
void tu_xm_callback_popup(w, closure, calldata)
     Widget w;
     XtPointer closure;
     XtPointer calldata;
{
  XmAnyCallbackStruct *p = (XmAnyCallbackStruct *) calldata;
  XEvent *xev = p->event;
  tu_ccb_arg_p arg = (tu_ccb_arg_p) closure;
  char *name;
  Widget iw = w;

  name = NULL;
  if (arg == NULL) {
    w = tu_first_popup(w);
    if (w == NULL) goto error;
  } else
    while (arg)
      {
	if (strcmp(arg->name, "find")==0) {
	  w = tu_widget_root_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_ANY, 0);
	}
	else if (strcmp(arg->name, "tf")==0) {
	  w = tu_widget_top_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	}
	else if (strcmp(arg->name, "rf")==0) {
	  w = tu_widget_root_widget(w);
	  if (w)
	    w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	}
	else if (strcmp(arg->name, "fn")==0) 
	  w = tu_find_widget(w, arg->value, TU_FW_NORMAL, 0);
	else if (strcmp(arg->name, "pn")==0) 
	  w = find_pn_child(w, arg->value);
	else if (strcmp(arg->name, "menu")==0)
	  name = arg->value;
	else
	  w = NULL;
	if (w == NULL)
	  goto error;
	arg = arg->next;
      }
  tu_menu_popup(w, name, xev);
  return;

 error:
  EPUT1("ERROR(menu_support):\n");
  EPUT2("  Menu to be popped up from widget '%s'\n", tu_widget_name(iw));
  EPUT1("  and no acceptable widget could be found.\n");
}


/****************************************************************
 * tu_xm_callback_popdown:
 *   Pops down a Motif popup menu.
 ****************************************************************/
void tu_xm_callback_popdown(w, closure, calldata)
     Widget w;
     XtPointer closure;
     XtPointer calldata;
{
  tu_menu_popdown(w);
}


/****************************************************************
 * xm_manage_child:
 *    This implements the 'XmManageChild' built-in callback.
 *    The function unmanages a child that is specified through
 *    the arguments.
 ****************************************************************/
static void xm_manage_child(widget, ccb, calldata)
     Widget widget;
     tu_ccb_arg_p ccb;
     XtPointer calldata;
{
  Widget child;
  int    code;

  while (ccb) {
    if (strcmp(ccb->name, "child") == 0) {
      child = tu_find_widget(widget, ccb->value, TU_FW_NORMAL, 1);
      if (child == NULL)
	child = tu_find_widget(widget, ccb->value, TU_FW_ANY, 0);
    }
    else if (strcmp(ccb->name, "XmSpec") == 0) {
      code = atoi(ccb->value+1);
      if (ccb->value[0] == TU_UWC_MESSAGE_BOX)
	child = XmMessageBoxGetChild(widget, code);
      else if (ccb->value[0] == TU_UWC_SELECTION_BOX)
	child = XmSelectionBoxGetChild(widget, code);
      else if (ccb->value[0] == TU_UWC_FS_BOX)
	child = XmFileSelectionBoxGetChild(widget, code);
      else 
	child = NULL;
    }
    else
      child = NULL;
    
    if (child == NULL) {
      EPUT1("ERROR(XmManageChild)\n");
      EPUT3("  Illegal manage specification: '%s=%s'\n", 
	    ccb->name, ccb->value);
    }
    else 
      XtManageChild(child);
    
    ccb = ccb->next;
  }
}


/****************************************************************
 * xm_unmanage_child:
 *    This implements the 'XmUnmanageChild' built in callback.
 *    The function unmanages a child that is specified through
 *    the arguments.
 ****************************************************************/
static void xm_unmanage_child(widget, ccb, calldata)
     Widget widget;
     tu_ccb_arg_p ccb;
     XtPointer calldata;
{
  Widget child;
  int code;

  while (ccb) {
    if (strcmp(ccb->name, "child") == 0) {
      child = tu_find_widget(widget, ccb->value, TU_FW_ANY, 1);
      if (child == NULL)
	child = tu_find_widget(widget, ccb->value, TU_FW_ANY, 0);
    } 
    else if (strcmp(ccb->name, "XmSpec") == 0) {
      code = atoi(ccb->value+1);
      if (ccb->value[0] == TU_UWC_MESSAGE_BOX)
	child = XmMessageBoxGetChild(widget, code);
      else if (ccb->value[0] == TU_UWC_SELECTION_BOX)
	child = XmSelectionBoxGetChild(widget, code);
      else if (ccb->value[0] == TU_UWC_FS_BOX)
	child = XmFileSelectionBoxGetChild(widget, code);
      else 
	child = NULL;
    }
    else
      child = NULL;

    if (child == NULL) {
      EPUT1("ERROR(XmUnmanageChild)\n");
      EPUT3("  Illegal unmanage specification: '%s=%s'\n", 
	    ccb->name, ccb->value);
    }
    else
      XtUnmanageChild(child);

    ccb = ccb->next;
  }
}
 

/****************************************************************
 * callback_hcenter:
 *    This function is a callback function that will 
 *    update an offset of a widget so that it becomes centered
 *    in an XmForm.
 ****************************************************************/
static void callback_hcenter(w, closure, calldata)
     Widget w;
     XtPointer closure;
     XtPointer calldata;
{
  Widget p;
  Widget c;
  Arg args[10];
  Dimension width;
  int offset;

  if (XtParent(w) == NULL)
    goto error;

  c = w;
  for (;;) {
    p = XtParent(c);
    if (p == NULL)
      goto error;
    if (XtIsSubclass(p, xmFormWidgetClass)) 
      break;
    c = p;
  }
  
  XtSetArg(args[0], XmNwidth, &width);
  XtSetArg(args[1], XmNleftOffset, &offset);
  XtGetValues(c, args, 2);

  offset -= width/2;
  XtSetArg(args[0], XmNleftOffset, offset);
  XtSetValues(c, args, 1);
  return;

 error:
  printf("ERROR(TuCenterH):\n");
  printf("   No form to center, widget %s\n", tu_widget_name(w));
}


/****************************************************************
 * callback_vcenter:
 *    This function is a callback function that will 
 *    update an offset of a widget so that it becomes centered
 *    in an XmForm.
 ****************************************************************/
static void callback_vcenter(w, closure, calldata)
     Widget w;
     XtPointer closure;
     XtPointer calldata;
{
  Widget p;
  Widget c;
  Arg args[10];
  Dimension height;
  int offset;

  if (XtParent(w) == NULL)
    goto error;

  c = w;
  for (;;) {
    p = XtParent(c);
    if (p == NULL)
      goto error;
    if (XtIsSubclass(p, xmFormWidgetClass)) 
      break;
    c = p;
  }
  
  XtSetArg(args[0], XmNheight, &height);
  XtSetArg(args[1], XmNtopOffset, &offset);
  XtGetValues(c, args, 2);

  offset -= height/2;
  XtSetArg(args[0], XmNtopOffset, offset);
  XtSetValues(c, args, 1);
  return;

 error:
  printf("ERROR(TuCenterV):\n");
  printf("   No form to center, widget %s\n", tu_widget_name(w));
}


/****************************************************************
 * set_popup_handler:
 ****************************************************************/
/*ARGSUSED*/
static void set_popup_handler(widget, closure, calldata)
     Widget widget;
     XtPointer closure;
     XtPointer calldata;
{
  XtAddEventHandler(XtParent(XtParent(widget)),
		    ButtonPressMask, False,
		    tu_handle_popup_menu, (XtPointer)widget);
}


/****************************************************************
 * tu_xm_callbacks:
 *     Declares the action procedure that could be used to
 *     handle Motif menus.
 ****************************************************************/
/*CExtract*/
void tu_xm_callbacks()
{
  static int initiated = 0;
  static XtActionsRec actions[] =
    {
      { "TuPopup",   xm_action_popup   },
      { "TuPopdown", xm_action_popdown },

      { "TuManage",   xm_action_manage   },
      { "TuUnmanage", xm_action_unmanage },

      { "XmMenuPopup",   xm_action_popup   },
      { "XmMenuPopdown", xm_action_popdown },
    };
  
  if (!initiated) {
    XtAppAddActions(tu_global_app_context, actions, XtNumber(actions));

    tu_ccb_define("TuPopup", (TuCallbackProc)tu_xm_callback_popup);
    tu_ccb_define("TuPopdown", (TuCallbackProc)tu_xm_callback_popdown);
 
    tu_ccb_define("TuManage", (TuCallbackProc)tu_xm_callback_manage);
    tu_ccb_define("TuUnmanage", (TuCallbackProc)tu_xm_callback_unmanage);

    tu_ccb_define("TuCenterV", (TuCallbackProc)callback_vcenter);
    tu_ccb_define("TuCenterH", (TuCallbackProc)callback_hcenter);
    tu_ccb_define("TuInstallPopupHandler", (TuCallbackProc)set_popup_handler);

    tu_ccb_define("XmMenuPopup", (TuCallbackProc)tu_xm_callback_popup);
    tu_ccb_define("XmMenuPopdown", (TuCallbackProc)tu_xm_callback_popdown);
    tu_ccb_define("XmManageChild", (TuCallbackProc)xm_manage_child);
    tu_ccb_define("XmUnmanageChild", (TuCallbackProc)xm_unmanage_child);
    initiated = 1;
  }
}



/****************************************************************
 *
 * Xt Management & Keyboard focus callbacks
 *
 ****************************************************************/

/****************************************************************
 * xt_manage_child:
 *    This implements the 'XtManageChild' built-in callback.
 *    The function unmanages a child that is specified through
 *    the arguments.
 ****************************************************************/
static void xt_manage_child(widget, ccb, calldata)
     Widget widget;
     tu_ccb_arg_p ccb;
     XtPointer calldata;
{
  Widget child;

  while (ccb) {
    if (strcmp(ccb->name, "child") == 0) {
      child = tu_find_widget(widget, ccb->value, TU_FW_NORMAL, 1);
      if (child == NULL)
	child = tu_find_widget(widget, ccb->value, TU_FW_ANY, 0);
    } else
      child = NULL;

    if (child == NULL) {
      EPUT1("ERROR(XtManageChild)\n");
      EPUT3("  Illegal manage specification: '%s=%s'\n", 
	    ccb->name, ccb->value);
    } else 
      XtManageChild(child);

    ccb = ccb->next;
  }
}


/****************************************************************
 * xt_unmanage_child:
 *    This implements the 'XtUnmanageChild' built-in callback.
 *    The function unmanages a child that is specified through
 *    the arguments.
 ****************************************************************/
static void xt_unmanage_child(widget, ccb, calldata)
     Widget widget;
     tu_ccb_arg_p ccb;
     XtPointer calldata;
{
  Widget child;

  while (ccb) {
    if (strcmp(ccb->name, "child") == 0) {
      child = tu_find_widget(widget, ccb->value, TU_FW_ANY, 1);
      if (child == NULL)
	child = tu_find_widget(widget, ccb->value, TU_FW_ANY, 0);
    } 
    else
      child = NULL;
    
    if (child == NULL) {
      EPUT1("ERROR(XtUnmanageChild)\n");
      EPUT3("  Illegal unmanage specification: '%s=%s'\n", 
	    ccb->name, ccb->value);
    } else
      XtUnmanageChild(child);
    
    ccb = ccb->next;
  }
}


/****************************************************************
 * xt_set_keyboard_focus:
 *     A callback to set keyboard focus.
 ****************************************************************/
static void xt_set_keyboard_focus(widget, ccb, calldata)
     Widget widget;
     tu_ccb_arg_p ccb;
     XtPointer calldata;
{
  Widget parent;
  Widget child;

  parent = widget;
  child = NULL;

  while (ccb) {
    if (strcmp(ccb->name, "parent") == 0) {
      parent = tu_find_parent(parent, ccb->value);
      child = widget;
    } 
    else if (strcmp(ccb->name, "child") == 0) {
      child = tu_find_widget(parent, ccb->value, TU_FW_NORMAL, 0);
      widget = child;
    }
    else if (strcmp(ccb->name, "none") == 0) {
      child = NULL;
    }
    else
      goto error;
    if (parent == NULL)
      goto error;
    ccb = ccb->next;
  }
  XtSetKeyboardFocus(parent, child);
  return;

 error:
  EPUT1("ERROR(SetKeyboardFocus)\n");
  EPUT2("  Parameter problem for XtSetKeyboardFocus (widget '%s')\n",
	tu_widget_name(widget));
}


/****************************************************************
 * xt_destroy_parent:
 *     Kill the parent widget!
 ****************************************************************/
static void xt_destroy_parent(widget, closure, calldata)
     Widget widget;
     tu_ccb_arg_p closure;
     XtPointer calldata;
{
  Widget parent;
  parent = XtParent(widget);
  if (parent)
    XtDestroyWidget(parent);
}


/****************************************************************
 * exit_program_callback:
 *     Exit the program with a successful status code.
 ****************************************************************/
static void exit_program_callback(widget, closure, calldata)
     Widget widget;
     tu_ccb_arg_p closure;
     XtPointer calldata;
{
  exit(0);
}


/****************************************************************
 * tu_xt_callbacks:
 *    This function declares the predefined C callbacks.
 *    Currently, the following C callbacks are declared:
 *        XmManageChild, XmUnmanageChild,
 *        XtDestroyParent, SetKeyboardFocus
 ****************************************************************/
/*CExtract*/
void tu_xt_callbacks()
{
  static int initiated = 0;

  if (!initiated) {
    tu_ccb_define("XtManageChild",    xt_manage_child);
    tu_ccb_define("XtUnmanageChild",  xt_unmanage_child);
    tu_ccb_define("XtDestroyParent",  xt_destroy_parent);
    tu_ccb_define("SetKeyboardFocus", xt_set_keyboard_focus);

    /* This callback will be redefined if the D runtime is present */
    tu_ccb_define("TuExitProgram", exit_program_callback);

    initiated = 1;
  }
}



#if TU_CVT
/****************************************************************
 *
 * Color allocation routines ...
 *
 ****************************************************************/


/****************************************************************
 * tu_allocate_color:
 *     This function takes a color structure and allocates
 *     a pixel value using an optional color scheme structure
 *     and cmap. The function returns a pixel value.
 ****************************************************************/
/*CExtract*/
int tu_allocate_color(screen, cmap, color, colsch)
     Screen *screen;
     Colormap cmap;
     XColor *color;
     tu_color_scheme colsch;
{
  int cindex = -1;
  int i = 0;

  if (colsch == NULL) {
    /*
     * No color map scheme. Just go ahead and try to allocate the color.
     * Note that color structure passed in is
     * modified to contain 'true' rgb values and pixel value.
     */
    color->flags = DoRed|DoGreen|DoBlue;
    if (XAllocColor(DisplayOfScreen(screen), cmap, color)) 
      return 1;
    color->pixel = WhitePixelOfScreen(screen);
    return 0;
  }

  if (colsch->cs_fuzz > 0) {
    /* Find the color that is placed closest to the 
     * currently requested color structure. */
    long distance;
    long cur_min = 0x1000000;
    long fuzz;
    int dr, dg, db;

    for (i=0;i<colsch->cs_alloced;i++) 
      if (colsch->cs_refcnt[i] > 0) {
	dr = ((int) colsch->cs_colors[i].red - (int) color->red)/0x100;
	dg = ((int) colsch->cs_colors[i].green - (int) color->green)/0x100;
	db = ((int)colsch->cs_colors[i].blue - (int) color->blue)/0x100;
	distance = (dr*dr + dg*dg + db*db);
	if (distance < cur_min) {
	  cur_min = distance;
	  cindex = i;
	}
      }

    if (cindex >= 0) {
      fuzz = colsch->cs_fuzz*colsch->cs_fuzz;
      fuzz = (((fuzz*256)/100)*256)/100;
      if (cur_min < fuzz) {
	/* Reuse the color the was with the fuzzy range */
	colsch->cs_refcnt[cindex]++;
	color->pixel = colsch->cs_colors[cindex].pixel;
	return 1;
      }
    }
  }

  if (colsch->cs_use_alloced_cells) {
    if (colsch->cs_current == colsch->cs_max) {
      /* 
       * No free color cells, have to use closest cell. Note
       * that this means that the user has to update the
       * color scheme structure when the cell contents
       * is changed.
       */
      if (cindex < 0) {
	color->pixel = WhitePixelOfScreen(screen);
	return 0;
      }
      colsch->cs_refcnt[cindex]++;
      color->pixel = colsch->cs_colors[cindex].pixel;
      return 1;
    }
    /* 
     * There is a preallocated color cell, find it
     * and note it (move the rgbs to the cell and the
     * color scheme structure). 
     */
    for (i=0;i<colsch->cs_max;i++)
      if (colsch->cs_refcnt[i] == 0) {
	colsch->cs_refcnt[i] = 1;
	colsch->cs_current++;
	colsch->cs_colors[i].red = color->red;
	colsch->cs_colors[i].blue = color->blue;
	colsch->cs_colors[i].green = color->green;
	colsch->cs_colors[i].flags = DoRed|DoGreen|DoBlue;
	XStoreColor(DisplayOfScreen(screen), cmap, colsch->cs_colors+i);
	color->pixel = colsch->cs_colors[i].pixel;
	return 1;
      }
    /*
     * This should not happen as the number of used colors
     * indicates that there are free cells. Return dummy pixel.
     */
    color->pixel = WhitePixelOfScreen(screen);
    return 0;
  } 

  /* 
   * Ok, we do not have preallocated writable cells,
   * so we will go ahead and try to allocate one
   * unless we would exceed a specified maximum (then
   * we will take the closest color). If we have not
   * allocated enough data structures for the new color,
   * then the refcnt and colors structures are expanded.
   */
  if ((colsch->cs_max > 0) && (colsch->cs_current == colsch->cs_max)) {
    /* 
     * Not allowed to allocate more cells, take the closest
     * and update the ref cnt.
     */
    if (cindex < 0) {
      color->pixel = WhitePixelOfScreen(screen);
      return 0;
    }
    colsch->cs_refcnt[cindex]++;
    color->pixel = colsch->cs_colors[cindex].pixel;
    return 1;
  }
  
  /* Expand the structures if necessary */
  if (colsch->cs_current == colsch->cs_alloced) {
    colsch->cs_alloced += 10;
    colsch->cs_colors =
      (XColor *) XtRealloc((char *)colsch->cs_colors,
			   sizeof(XColor)*colsch->cs_alloced);
    colsch->cs_refcnt =
      (int *) XtRealloc((char *)colsch->cs_refcnt,
			sizeof(int)*colsch->cs_alloced);
    for (i=colsch->cs_current;i<colsch->cs_alloced;i++)
      colsch->cs_refcnt[i] = 0;
  }

  for (i=0;i<colsch->cs_alloced;i++)
    if (colsch->cs_refcnt[i] == 0) {
      colsch->cs_colors[i].red = color->red;
      colsch->cs_colors[i].green = color->green;
      colsch->cs_colors[i].blue = color->blue;
      colsch->cs_colors[i].flags = DoRed|DoGreen|DoBlue;
      if (XAllocColor(DisplayOfScreen(screen), cmap, colsch->cs_colors+i)) {
	colsch->cs_refcnt[i] = 1;
	colsch->cs_current++;
	color->pixel = colsch->cs_colors[i].pixel;
	return 1;
      } else {
	/* Color allocation failed, take closest if available. */
	if (cindex < 0) {
	  color->pixel = WhitePixelOfScreen(screen);
	  return 0;
	}
	colsch->cs_refcnt[cindex]++;
	color->pixel = colsch->cs_colors[cindex].pixel;
	return 1;
      }
    }

  /* SHOULDN'T */
  color->pixel = WhitePixelOfScreen(screen);  
  return 0;
}


/****************************************************************
 * tu_free_color:
 *     This function takes a color structure and deallocates
 *     a pixel value using an optional color scheme structure
 *     and cmap.
 ****************************************************************/
/*CExtract*/
void tu_free_color(screen, cmap, pixel, colsch)
     Screen *screen;
     Colormap cmap;
     unsigned long pixel;
     tu_color_scheme colsch;
{
  int i;

  if (colsch == NULL) {
    XFreeColors(DisplayOfScreen(screen), cmap, &pixel, 1, 0);
    return;
  }

  for (i=0;i<colsch->cs_alloced;i++)
    if ((colsch->cs_refcnt[i] > 0) && (pixel == colsch->cs_colors[i].pixel)) {
      if (--colsch->cs_refcnt[i] == 0) {
	colsch->cs_current--;
	if (colsch->cs_use_alloced_cells)
	  return;
	tu_free_color(screen, cmap, pixel, NULL);
      }
      break;
    }
}

/****************************************************************
 * tu_pds_allocate_colors:
 *     Allocate pixel values for the colors used.
 ****************************************************************/
/*CExtract*/
void tu_pds_allocate_colors(screen, cmap, v, pds, colsch)
     Screen *screen;
     Colormap cmap;
     XVisualInfo *v;
     tu_pixmap pds;
     tu_color_scheme colsch;
{
  struct tu_color_descr *ct;
  char tag;
  XColor color;
  char *colstr;
  struct tu_pixmap_color_pair *cp, *cpc, *cpg, *cpm, *cps;
  static int exp2[] = 
    { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
	2048, 4096, 8192, 16384, 32768, 65536 };
  int i;

  /* find out tag to look for ... */
  if (v->bits_per_rgb == 1)
    tag = 'm';
  else {
    switch (v->class) {
    case GrayScale:
    case StaticGray:
      tag = 'g';
      break;
    case PseudoColor:
    case StaticColor:
    case DirectColor:
    case TrueColor:
      tag = 'c';
      break;
    }
  }

  for (i=0;i<pds->pds_ncolors;i++) {
    ct = pds->pds_ct[i];

    /*
     * Look up matching color specifications. We will try to
     * match symbolic color names, color 'c', gray scale 'g',
     * and monochrome 'm'. If a specification is followed
     * by a number, then this specifies that that specification
     * should be used if exactly the number of colors in the
     * colormap matches.
     */
    cpc = NULL;
    cpg = NULL;
    cpm = NULL;
    cps = NULL;
    colstr = NULL;

    for (cp=ct->ct_colors;cp;cp=cp->next) {
      switch (cp->name[0]) {
      case 'c':
	if ((cp->name[1] == '\0') && (cpc == NULL))
	  cpc = cp;
	if (atoi(&cp->name[1]) == exp2[v->bits_per_rgb])
	  cpc = cp;
	break;

      case 'g':
	if ((cp->name[1] == '\0') && (cpg == NULL))
	  cpg = cp;
	if (atoi(&cp->name[1]) == exp2[v->bits_per_rgb])
	  cpg = cp;
	break;

      case 'm':
	if (cp->name[1] == '\0')
	  cpm = cp;
	break;

      case 's':
	if ((cp->name[1] == '\0') && (cps == NULL))
	  cps = cp;
	if (atoi(&cp->name[1]) == exp2[v->bits_per_rgb])
	  cps = cp;
	break;
      }
    }

    /* try to go symbolic */
    if (cps != NULL) {
      colstr = XGetDefault(DisplayOfScreen(screen), 
			   tu_application_name, 
			   cps->color);
    }

    /* get explicit specified */
    if (colstr == NULL)
      switch (tag) {
      case 'c':
	if (cpc)
	  colstr = cpc->color;
	else if (cpg) 
	  colstr = cpg->color;
	else if (cpm)
	  colstr = cpm->color;
	break;
	
      case 'g':
	if (cpg) 
	  colstr = cpg->color;
	else if (cpc)
	  colstr = cpc->color;
	else if (cpm)
	  colstr = cpm->color;
	break;
	
      case 'm':
	if (cpm)
	  colstr = cpm->color;
	else if (cpg) 
	  colstr = cpg->color;
	else if (cpc)
	  colstr = cpc->color;
	break;
      }
    
    /* Now we know what string we would like to have converted
     * to a pixel value. Note it and try to parse it.
     */
    ct->ct_color_name = colstr;
    if (colstr == NULL)
      ct->ct_pixel = WhitePixelOfScreen(screen);
    else
      if (strcmp(colstr, XtDefaultForeground) == 0) 
	ct->ct_pixel = BlackPixelOfScreen(screen);
      else if (strcmp(colstr, XtDefaultBackground) == 0) 
	ct->ct_pixel = WhitePixelOfScreen(screen);
      else if (!XParseColor(DisplayOfScreen(screen), cmap, colstr, &color)) {
	EPUT1("ERROR(AllocateColor)\n");
	EPUT2("  Color '%s' is not recognized\n", colstr);
	ct->ct_pixel = WhitePixelOfScreen(screen);
      } else {
	/* we now got a color structure, i.e. the rgb values
	 * for red, green and blue. The next step is to convert
	 * it into a pixel. */
	(void) tu_allocate_color(screen, cmap, &color, colsch);
	ct->ct_pixel = color.pixel;
      }
  }
}


/****************************************************************
 * tu_pds_free_colors:
 *     Deallocate pixel values for the colors used.
 ****************************************************************/
/*CExtract*/
void tu_pds_free_colors(screen, cmap, v, pds, colsch)
     Screen *screen;
     Colormap cmap;
     XVisualInfo *v;
     tu_pixmap pds;
     tu_color_scheme colsch;
{
  struct tu_color_descr *ct;  
  int i;

  for (i=0;i<pds->pds_ncolors;i++) {
    ct = pds->pds_ct[i];
    tu_free_color(screen, cmap, ct->ct_pixel, colsch);
    ct->ct_pixel = 0;
  }
}



/****************************************************************
 *
 * XPM3 Pixmap support routines
 *
 ****************************************************************/


/****************************************************************
 * tu_pds_free:
 *    This function reclaims all space used for a pds
 *    structure.
 ****************************************************************/
/*CExtract*/
void tu_pds_free(p)
     tu_pixmap p;
{
  struct tu_color_descr *ct;
  struct tu_pixmap_color_pair *cp;
  struct tu_pixmap_color_pair *cpn;
  int i;

  XtFree((char *)p->pds_name);
  if (p->pds_image != NULL)
    XDestroyImage(p->pds_image);
  XtFree((char *)p->pds_data);

  for (i=0;i<p->pds_ncolors;i++) {
    ct = p->pds_ct[i];
    if (ct == NULL)
      continue;
    XtFree((char *)ct->ct_ch);

    cp = ct->ct_colors;
    while (cp) {
      XtFree((char *)cp->name);
      XtFree((char *)cp->color);
      cpn = cp->next;
      XtFree((char *)cp);
      cp = cpn;
    }
    XtFree((char *)ct);
  }

  XtFree((char *)p->pds_ct);
  XtFree((char *)p);
}

/****************************************************************
 * skip_c_comment:
 *    Skip C-style comment (begins with slash-star, ends with
 *    star-slash).
 ****************************************************************/
static void skip_c_comment(ps)
char **ps;
{
  char *s;

  /* Return if the first two characters are not the comment initiator */
  if (!ps)
    return;
  s = *ps;
  if (!s || *s != '/' || s[1] != '*')
    return;

  /* Skip the comment initiator and the comment */
  s += 2;
  while (*s && (*s != '*' || s[1] != '/'))
    s ++;

  /* Skip the comment terminator and trailing whitespace */
  if (*s)
    s += 2;
  SKIPSP(s);
  *ps = s;
}



/****************************************************************
 * read_pixmap:
 *    Read a pixmap specification from a string. Only 8- and
 *    16-bit data structures are supported. If there are more
 *    than 256 colors in the image, then the 16-bit data format
 *    is used.
 ****************************************************************/
static int read_pixmap(s, pds)
	char *s;
	tu_pixmap pds;
{
  char *p;
  char *q;
  char *name, *tag, *color;
  struct tu_color_descr **ct, **ctc;
  struct tu_pixmap_color_pair *cp;
  struct tu_pixmap_color_pair **prev_p;
  int i, j, k;
  char buf[8192];

  /* clear the pds structure before we start */
  (void) memset((char *)pds, '\0', sizeof(struct _tu_pixmap));

  p = s;
  SKIPSP(p);

  if (strncmp(p, "/*", 2) != 0) 
    goto error;
  p += 2;

  SKIPSP(p);
  if (strncmp(p, "XPM", 3) != 0) 
    goto error;
  p += 3;

  SKIPSP(p);
  if (strncmp(p, "*/", 2) != 0) 
    goto error;
  p += 2;

  SKIPSP(p);
  if (strncmp(p, "static", 6) == 0) {
    p += 6;
    SKIPSP(p);
  }

  if (strncmp(p, "char", 4) != 0) 
    goto error;
  p += 4;
  SKIPSP(p);

  if (*p != '*')
    goto error;
  p++;
  SKIPSP(p);

  name = getid(&p, (char*)NULL);
  if (name == NULL) 
    goto error;
  pds->pds_name = name;
  SKIPSP(p);

  for (q="[]={";*q;q++) {
    if (*p != *q)
      goto error;
    p++;
    SKIPSP(p);
  }

  /* Pick up pixmap configuration */
  skip_c_comment(&p);

  if (*p != '"')
    goto error;
  p++;

  if (sscanf(p, "%d %d %d %d", &pds->pds_width, &pds->pds_height,
	    &pds->pds_ncolors, &pds->pds_nchars) != 4)
    goto error;
  while (*p != '\0' && *p != '"') p++;
  if (*p == '\0') goto error;
  p++;

  ct = (struct tu_color_descr **) 
    XtCalloc(pds->pds_ncolors, sizeof(struct tu_color_descr *));
  pds->pds_ct = ct;
  for (q=",";*q;q++) {
    if (*p != *q)
      goto error;
    p++;
    SKIPSP(p);
  }

  skip_c_comment(&p);
  for (i=0;i<pds->pds_ncolors;i++) {
    SKIPSP(p);
    if (*p != '"')
      goto error;
    p++;
    
    ct[i] = (struct tu_color_descr *) XtMalloc(sizeof(struct tu_color_descr));
    ct[i]->ct_ch = (char *) XtMalloc(pds->pds_nchars+1);
    for (k=0;k<pds->pds_nchars;k++)
      ct[i]->ct_ch[k] = *p++;
    ct[i]->ct_ch[k] = '\0';
    ct[i]->ct_index = i;
    ct[i]->ct_colors = NULL;

    prev_p = &ct[i]->ct_colors;
    for (;;) {
      SKIPSP(p);
      if (*p == '"') {
	p++;
	break;
      }

      tag = getid(&p, (char*)NULL);
      if (tag == NULL) goto 
	error;
      SKIPSP(p);
      
      color = getid(&p, "#");
      if (color == NULL) {
	free(tag);
	goto error;
      }
      
      cp = (struct tu_pixmap_color_pair *) 
	XtMalloc(sizeof(struct tu_pixmap_color_pair));
      cp->name = tag;
      cp->color = color;
      if (prev_p)
	*prev_p = cp;
      prev_p = &cp->next;
      cp->next = NULL;
    }
    SKIPSP(p);

    if (*p != ',')
      goto error;
    p++;
  }

  SKIPSP(p);
  skip_c_comment(&p);
  if (pds->pds_nchars == 1) {
    /* special case - make fast index table */
    unsigned char lkup_tbl[256];
    unsigned char *data, *dp;

    for (i=0;i<256;i++) lkup_tbl[i] = 0;
    for (i=0;i<pds->pds_ncolors;i++)
      lkup_tbl[(unsigned int)ct[i]->ct_ch[0]] = i;

    data = (unsigned char *) 
      malloc((unsigned)(pds->pds_width*pds->pds_height));
    if (data == NULL)
      goto error;
    pds->pds_data = data;

    dp = data;
    for (i=0;i<pds->pds_height;i++) {
      SKIPSP(p);
      if (*p != '"')
	goto error;
      p++;
      for (j=0;j<pds->pds_width;j++)
	*dp++ = lkup_tbl[(unsigned int)*p++];
      if (*p != '"')
	goto error;
      p++;
      SKIPSP(p);
      if ((i+1) != pds->pds_height) {
	if (*p != ',')
	  goto error;
	p++;
      } else 
	if (*p == ',')
	  p++;
      SKIPSP(p);
    }
  } else {
    /* Make it general but extremely slow */
    char ch;
    int cnt;
    unsigned char *data, *dp;
    unsigned short *data16, *dp16;
    struct tu_color_descr **lkup_tbl[256];

    for (k=0;k<256;k++) {
      ch = (char) k;
      
      /* Build a table for faster lookup */
      cnt = 0;
      for (i=0;i<pds->pds_ncolors;i++) 
	if (ct[i]->ct_ch[0] == ch) cnt++;
      lkup_tbl[k] = (struct tu_color_descr **)
	XtMalloc((cnt+1)*sizeof(struct tu_color_descr *));
      if (cnt > 0) {
	cnt = 0;
	for (i=0;i<pds->pds_ncolors;i++) 
	  if (ct[i]->ct_ch[0] == ch) 
	    lkup_tbl[k][cnt++] = ct[i];
      }
      lkup_tbl[k][cnt] = NULL;
    }      

    if (pds->pds_ncolors > 256) {
      data16 = (unsigned short *) 
	malloc((unsigned)(pds->pds_width*
			  pds->pds_height*
			  sizeof(unsigned short)));
      if (data16 == NULL)
	goto error;
      dp16 = data16;
      pds->pds_data = (unsigned char *) data16;
    } else {
      data = (unsigned char *) 
	malloc((unsigned)(pds->pds_width*
			  pds->pds_height*
			  sizeof(unsigned char)));
      if (data == NULL)
	goto error;
      dp = data;
      pds->pds_data = (unsigned char *) data;
    }

    for (i=0;i<pds->pds_height;i++) {
      SKIPSP(p);
      if (*p != '"')
	goto error;
      p++;
      for (j=0;j<pds->pds_width;j++) {
	for (k=0;k<pds->pds_nchars;k++) buf[k] = *p++;
	buf[k] = '\0';
	for (ctc = lkup_tbl[(unsigned int)buf[0]];
	     *ctc;
	     ctc++)
	  if (strcmp(buf, (*ctc)->ct_ch) == 0) {
	    if (pds->pds_ncolors > 256)
	      *dp16++ = (*ctc)->ct_index;
	    else
	      *dp++ = (*ctc)->ct_index;
	    break;
	  }
      }
      if (*p != '"')
	goto error;
      p++;
      SKIPSP(p);
      if ((i+1) != pds->pds_height) {
	if (*p != ',')
	  goto error;
	p++;
      } else 
	if (*p == ',')
	  p++;
      SKIPSP(p);
    }

    for (k=0;k<256;k++) 
      free((char *)lkup_tbl[k]);
  }

  for (q="};";*q;q++) {
    if (*p != *q)
      goto error;
    p++;
    SKIPSP(p);
  }
  return 1;

 error:
  tu_pds_free(pds);
  return 0;
}

/****************************************************************
 * tu_pds_read_xpm_file:
 *    This function reads an xpm pixmap file and builds a
 *    pixmap description structure (a pds). 
 ****************************************************************/
/*CExtract*/
tu_pixmap tu_pds_read_xpm_file(filename)
     char *filename;
{
  tu_pixmap p;
  struct stat sb;
  char *s;
  int fid;

  if (stat(filename, &sb) < 0)
    return NULL;

  fid = open(filename, O_RDONLY);
  if (fid == -1)
    return NULL;

  s = (char *) XtMalloc((int)sb.st_size+1);
  if (s == NULL) {
    (void) close(fid);
    return NULL;
  }

  if (read(fid, s, (int)sb.st_size) != sb.st_size) {
    (void) close(fid);
    free(s);
    return NULL;
  }
  (void) close(fid);

  p = (tu_pixmap) XtMalloc(sizeof(struct _tu_pixmap));
  if (!p) {
    free(s);
    return NULL;
  }

  s[sb.st_size] = '\0';
  if (!read_pixmap(s, p)) {
    free(s);
    return NULL;
  }

  free(s);

  return p;
}


/****************************************************************
 * tu_pds_read_xpm_string:
 *    This function reads an xpm pixmap string and builds a
 *    pixmap description structure (a pds). 
 ****************************************************************/
/*CExtract*/
tu_pixmap tu_pds_read_xpm_string(str)
     char *str;
{
  tu_pixmap p;

  p = (tu_pixmap) XtMalloc(sizeof(struct _tu_pixmap));
  if (!p)
    return NULL;

  if (!read_pixmap(str, p))
    return NULL;

  return p;
}

#if FULL_PIXMAP_SUPPORT

/****************************************************************
 * tu_pds_write_xbm_file:
 *   This function writes an xbm file from a pds structure.
 ****************************************************************/
int tu_pds_write_xbm_file(filename, pds)
char* filename;
tu_pixmap pds;
{
  FILE *f;
  int i;
  int j;
  int bit;
  int col;
  int mask;
  unsigned char *data;
  unsigned short *data16;
  char* color_bit;
  char* name;
  int w;
  int h;
  int n;
  struct tu_pixmap_color_pair *cp;  

  f = fopen(filename, "w");
  if (f == NULL)
    return 0;

  name = pds->pds_name;
  w = pds->pds_width;
  h = pds->pds_height;

  if (pds->pds_ncolors > 256) {
    data = NULL;
    data16 = (unsigned short *) pds->pds_data;
  }
  else {
    data = (unsigned char *) pds->pds_data;
    data16 = NULL;
  }

  /* Determine which colors are "foreground" */
  color_bit = (char*)malloc(pds->pds_ncolors);
  n = pds->pds_ncolors;
  for (i=0;i<n;i++) {
    color_bit[i] = 0;
    cp = pds->pds_ct[i]->ct_colors;
    while (cp) {
      if (!strcmp(cp->color, "black") ||
	  !strcmp(cp->color, XtDefaultForeground))
      {
	color_bit[i] = 1;
	break;
      }
      cp = cp->next;
    }
  }

  fprintf(f, "#define %s_width %d\n", name, w);
  fprintf(f, "#define %s_height %d\n", name, h);
  fprintf(f, "static unsigned char %s_bits[] = {", name);

  i = 0;
  j = 0;
  for (col=0;;col=(col+1)%12) {
    if (!col)
      fprintf(f, "\n  ");
    for (bit=0,mask=0; bit<8; bit++) {
      if (mask)
	mask >>= 1;
      if (color_bit[data ? *data++ : *data16++])
	mask |= 0x80;
      if (++j >= w) {
	j = 0;
	i ++;
	break;
      }
    }
    fprintf(f, " 0x%02x", mask);
    if (i >= h)
      break;
    fprintf(f, ",");
  }
  fprintf(f, "};\n");

  free((char*)color_bit);

  fclose(f);

  return 1;
}

/****************************************************************
 * tu_pds_write_xpm_file:
 *   This function writes an xpm file from a pds structure.
 ****************************************************************/
/*CExtract*/
int tu_pds_write_xpm_file(filename, pds)
     char *filename;
     tu_pixmap pds;
{
  FILE *f;
  int i, j;
  unsigned char *data;
  unsigned short *data16;
  struct tu_pixmap_color_pair *cp;  
  
  f = fopen(filename, "w");
  if (f == NULL)
    return 0;

  (void) fprintf(f, "/* XPM */\n");
  (void) fprintf(f, "static char * %s [] = {\n", pds->pds_name);
  (void) fprintf(f, "\"%d %d %d %d\",\n",
		 pds->pds_width, pds->pds_height, 
		 pds->pds_ncolors, pds->pds_nchars);

  for (i=0;i<pds->pds_ncolors;i++) {
    (void) fprintf(f, "\"%s", pds->pds_ct[i]->ct_ch);
    cp = pds->pds_ct[i]->ct_colors;
    while (cp) {
      (void) fprintf(f, " %s %s", cp->name, cp->color);
      cp = cp->next;
    }
    (void) fprintf(f, "\",\n");
  }

  if (pds->pds_ncolors > 256) 
    data16 = (unsigned short *) pds->pds_data;
  else
    data = (unsigned char *) pds->pds_data;

  for (i=0;i<pds->pds_height;i++) {
    (void) fprintf(f, "\"");
    if (pds->pds_ncolors > 256) 
      for (j=0;j<pds->pds_width;j++)
	(void) fprintf(f, "%s", pds->pds_ct[*data16++]->ct_ch);
    else 
      for (j=0;j<pds->pds_width;j++) 
	(void) fprintf(f, "%s", pds->pds_ct[*data++]->ct_ch);
    (void) fprintf(f, "\"");
    if ((i+1) != pds->pds_height)
      (void) fprintf(f, ",\n");
  }

  (void) fprintf(f, "};\n");
  
  (void) fclose(f);
  return 1;
}



/****************************************************************
 *
 * Bitmap to pds support ...
 *
 ****************************************************************/


/****************************************************************
 * read_bitmap:
 *    Read a bitmap specification from a string. The bits are 
 *    expanded into an 8-bit data structure.
 ****************************************************************/
static int read_bitmap(s, pds)
	char *s;
	tu_pixmap pds;
{
  char *p;
  char *q;
  char *name;
  struct tu_color_descr **ct;
  struct tu_pixmap_color_pair *cp;
  int i, j, k;
  unsigned char *dp;
  int val;

  /* clear the pds structure before we start */
  (void) memset((char *)pds, '\0', sizeof(struct _tu_pixmap));

  p = s;

  /* Skip leading C comments and whitespace */
  SKIPSP(p);
  for (;;) {
    q = p;
    skip_c_comment(&p);
    if (p == q)
      break;
  }

  if (strncmp(p, "#define", 7) != 0) 
    goto error;
  p += 7;

  SKIPSP(p);
  skip_c_comment(&p);
  name = getid(&p, (char*)NULL);
  if (name == NULL)
    goto error;
  i = strlen(name);
  if (i < 6)
    goto error;
  if (strcmp(&name[i-6], "_width") != 0)
    goto error;
  name[i-6] = '\0';
  pds->pds_name = name;

  SKIPSP(p);
  skip_c_comment(&p);
  if (sscanf(p, "%d", &pds->pds_width) != 1)
    goto error;
  while (*p != '\n' && *p != '\0') p++;
  if (*p != '\n') goto error;
  p++;

  SKIPSP(p);
  skip_c_comment(&p);
  if (strncmp(p, "#define", 7) != 0) 
    goto error;
  p += 7;

  SKIPSP(p);
  skip_c_comment(&p);
  name = getid(&p, (char*)NULL);
  if (name == NULL)
    goto error;
  free(name);

  SKIPSP(p);
  skip_c_comment(&p);
  if (sscanf(p, "%d", &pds->pds_height) != 1)
    goto error;
  while (*p != '\n' && *p != '\0') p++;
  if (*p != '\n') goto error;
  p++;

  pds->pds_ncolors = 2;
  pds->pds_nchars = 1;

  SKIPSP(p);
  skip_c_comment(&p);
  if (strncmp(p, "#define", 7) == 0) { /* possible hot spot */
    while (*p != '\n' && *p != '\0') p++;
    if (*p != '\n') goto error;
    p++;
  }
    
  SKIPSP(p);
  skip_c_comment(&p);
  if (strncmp(p, "#define", 7) == 0) { /* possible hot spot */
    while (*p != '\n' && *p != '\0') p++;
    if (*p != '\n') goto error;
    p++;
  }
    
  SKIPSP(p);
  skip_c_comment(&p);
  if (strncmp(p, "static", 6) == 0) {
    p += 6;
    SKIPSP(p);
  }

  if (strncmp(p, "unsigned", 8) == 0) {
    p += 8;
    SKIPSP(p);
  }

  if (strncmp(p, "char", 4) != 0) 
    goto error;
  p += 4;
  SKIPSP(p);

  name = getid(&p, (char*)NULL);
  if (name == NULL) 
    goto error;
  free(name);
  SKIPSP(p);

  for (q="[]={";*q;q++) {
    if (*p != *q)
      goto error;
    p++;
    SKIPSP(p);
  }

  dp = (unsigned char *) malloc(pds->pds_width*pds->pds_height);
  if (dp == NULL)
    goto error;
  pds->pds_data = dp;

  for (i=0;i<pds->pds_height;i++) {
    j = 0;
    do {
      SKIPSP(p);
      if (p[0] != '0' || p[1] != 'x')
	goto error;
      p += 2;
      if (sscanf(p, "%x", &val) != 1)
	goto error;
      while (*p != ',' && *p != '}' && *p != '\0') p++;
      if (*p == '\0') goto error;
      if (*p == ',') p++;
      k = 0;
      while (j<pds->pds_width && k<8) {      
	*dp++ = (val&1) != 0;
	k++;
	j++;
	val >>= 1;
      }
    } while (j < pds->pds_width);
  }

  SKIPSP(p);
  if (*p != '}')
    goto error;

  /* stuff in some color definitions */
  pds->pds_ct = (struct tu_color_descr **) 
    XtCalloc(2, sizeof(struct tu_color_descr *));

  pds->pds_ct[0] = 
    (struct tu_color_descr *) XtMalloc(sizeof(struct tu_color_descr));
  pds->pds_ct[0]->ct_ch = tu_strdup(" ");
  pds->pds_ct[0]->ct_index = 0;

  cp = (struct tu_pixmap_color_pair *) 
    XtCalloc(1, sizeof(struct tu_pixmap_color_pair));
  cp->name = tu_strdup("m");
  cp->color = tu_strdup("white");
  pds->pds_ct[0]->ct_colors = cp;

  pds->pds_ct[1] = 
    (struct tu_color_descr *) XtMalloc(sizeof(struct tu_color_descr));
  pds->pds_ct[1]->ct_ch = tu_strdup("*");
  pds->pds_ct[1]->ct_index = 1;

  cp = (struct tu_pixmap_color_pair *) 
    XtCalloc(1, sizeof(struct tu_pixmap_color_pair));
  cp->name = tu_strdup("m");
  cp->color = tu_strdup("black");
  pds->pds_ct[1]->ct_colors = cp;
  return 1;

 error:
  tu_pds_free(pds);
  return 0;
}


/****************************************************************
 * tu_pds_read_xbm_file:
 *    This function reads an xbm bitmap file and builds a
 *    pixmap description structure (a pds). 
 ****************************************************************/
/*CExtract*/
tu_pixmap tu_pds_read_xbm_file(filename)
     char *filename;
{
  tu_pixmap p;
  struct stat sb;
  char *s;
  int fid;

  if (stat(filename, &sb) < 0)
    return NULL;

  fid = open(filename, O_RDONLY);
  if (fid == -1)
    return NULL;

  s = (char *) malloc((int)sb.st_size+1);
  if (s == NULL) {
    (void) close(fid);
    return NULL;
  }

  if (read(fid, s, (int)sb.st_size) != sb.st_size) {
    (void) close(fid);
    free(s);
    return NULL;
  }

  p = (tu_pixmap) XtMalloc(sizeof(struct _tu_pixmap));

  s[sb.st_size] = '\0';
  if (!read_bitmap(s, p))
    return NULL;

  free(s);
  (void) close(fid);

  return p;
}


/****************************************************************
 * tu_pds_read_xbm_string:
 *    This function reads an xbm bitmap string and builds a
 *    pixmap description structure (a pds). 
 ****************************************************************/
/*CExtract*/
tu_pixmap tu_pds_read_xbm_string(str)
     char *str;
{
  tu_pixmap p;

  p = (tu_pixmap) XtMalloc(sizeof(struct _tu_pixmap));
  if (!read_bitmap(str, p))
    return NULL;

  return p;
}

#endif /* FULL_PIXMAP_SUPPORT */

#ifndef MIN_COLOR
#define MIN_COLOR 3
#endif

/****************************************************************
 * tu_pds_create_pixmap:
 *    This function creates a pixmap for the pds structure.
 *    The screen, colormap, and visual at which the pixmap 
 *    should be created should be supplied.
 ****************************************************************/
/*CExtract*/
void tu_pds_create_pixmap(screen, depth, v, pds)
     Screen *screen;
     unsigned int depth;
     XVisualInfo *v;
     tu_pixmap pds;
{
  GC gc;

  gc = XCreateGC(DisplayOfScreen(screen),
		 RootWindowOfScreen(screen),
		 0, NULL);

  pds->pds_pixmap = XCreatePixmap(DisplayOfScreen(screen),
				  RootWindowOfScreen(screen),
				  pds->pds_width, pds->pds_height,
				  depth);

  if (depth == 8) {
    if (pds->pds_image == NULL) {
      unsigned char *dp;
      unsigned char *sdp;
      unsigned char *dp_s;
      int i, j;

      dp_s = pds->pds_data;
      sdp = dp = (unsigned char *) malloc(pds->pds_width*pds->pds_height);
      for (i=0;i<pds->pds_height;i++)
	for (j=0;j<pds->pds_width;j++)
	  *dp++ = pds->pds_ct[*dp_s++]->ct_pixel;
      pds->pds_image = XCreateImage(DisplayOfScreen(screen), v->visual, 
				    8, ZPixmap, 0, (char *)sdp,
				    pds->pds_width, pds->pds_height, 8,
				    pds->pds_width);
    }
    XPutImage(DisplayOfScreen(screen), pds->pds_pixmap, gc,
	      pds->pds_image, 0, 0, 0, 0, pds->pds_width, pds->pds_height);
    XDestroyImage(pds->pds_image);
    pds->pds_image = NULL;
  } else if (depth == 1) {
    unsigned char *pd;
    unsigned char *spd;
    unsigned char *dp;
    int i, j, k;
    unsigned char bit;

    spd = pd = (unsigned char *) 
      XtMalloc(((pds->pds_width+7)/8)*pds->pds_height);
    dp = pds->pds_data;
    for (i=0;i<pds->pds_height;i++) {
      k = 8;
      bit = 0x80;
      j = 0;
      *pd = 0;
      do {
	if (k-- == 0) {
	  k = 7;
	  bit = 0x80;
	  pd++;
	  *pd = 0;
	}
	if (pds->pds_ct[*dp++]->ct_pixel)
	  (*pd) |= bit;
	bit >>= 1;
	j++;
      } while (j < pds->pds_width);
      pd++;
    }
    pds->pds_image = XCreateImage(DisplayOfScreen(screen), v->visual, 
				  1, ZPixmap, 0, (char *)spd,
				  pds->pds_width, pds->pds_height, 8,
				  (pds->pds_width+7)/8);
    XPutImage(DisplayOfScreen(screen), pds->pds_pixmap, gc,
	      pds->pds_image, 0, 0, 0, 0, pds->pds_width, pds->pds_height);
    XDestroyImage(pds->pds_image);
    pds->pds_image = NULL;
  } else {
    if (pds->pds_image == NULL) {
      unsigned char *dp_s;
      unsigned char *sdp;
      int i, j;

      /* This is terribly slow. I have to figure out what to do
	 to call XCreateImage instead */
      pds->pds_image = XGetImage(DisplayOfScreen(screen), 
				 RootWindowOfScreen(screen), 0, 0,
				 pds->pds_width, pds->pds_height, 
				 AllPlanes, ZPixmap);
      dp_s = pds->pds_data;
      for (i=0;i<pds->pds_height;i++)
	for (j=0;j<pds->pds_width;j++)
	  XPutPixel(pds->pds_image, j, i, pds->pds_ct[*dp_s++]->ct_pixel);
    }
    XPutImage(DisplayOfScreen(screen), pds->pds_pixmap, gc,
	      pds->pds_image, 0, 0, 0, 0, pds->pds_width, pds->pds_height);
    XDestroyImage(pds->pds_image);
    pds->pds_image = NULL;
  }

  XFreeGC(DisplayOfScreen(screen), gc);
}


/****************************************************************
 * tu_pds_free_pixmap:
 *    This function frees the pixmap in the pds structure.
 ****************************************************************/
/*CExtract*/
void tu_pds_free_pixmap(screen, pds)
     Screen *screen;
     tu_pixmap pds;
{
  if (pds->pds_pixmap != None)
    XFreePixmap(DisplayOfScreen(screen), pds->pds_pixmap);
  pds->pds_pixmap = None;
}



/****************************************************************
 *
 * Converter access function utility
 *
 ****************************************************************/


/****************************************************************
 * tu_convert: 
 *    Converts a value from a given (string) representation
 *    to the target representation.
 ****************************************************************/
/*CExtract*/
XtPointer tu_convert(widget, value, target, ok)
     Widget widget;
     String value;
     String target;
     int   *ok;
{
  XrmValue  xrmstr;
  XrmValue  xrmdst;
  long      rval;
  char *nval;

  if (!strcmp(target, XtRString))
    return (XtPointer)value;

  if (!strcmp(target, XmRFontList)) {
    nval = (char *) malloc(strlen(value) + 1);
    (void) strcpy(nval, value);
    value = nval;
  }

  xrmstr.addr = value;
  xrmstr.size = strlen(value);
  xrmstr.addr = (XtPointer) value;
  xrmdst.addr = (XtPointer) "NoError";
	
  if ((strcmp(target, XmRVerticalDimension) == 0) ||
      (strcmp(target, XmRHorizontalDimension) == 0))
    target = XmRDimension;
  if ((strcmp(target, XmRVerticalPosition) == 0) ||
      (strcmp(target, XmRHorizontalPosition) == 0))
    target = XmRPosition;
  
  XtConvert(widget, XtRString, &xrmstr, target, &xrmdst);

  if (!strcmp(target, XmRFontList)) {
    XtFree(nval);
  }

  if (!xrmdst.addr) {
    EPUT1("ERROR(TuConvert)\n");
    EPUT3("  Conversion of '%s:String' to '%s' failed\n",
	  value, target);
    *ok = 0;
    return (NULL);
  }	  
	
  /* 
   *  Take care of shorter data
   */
  if (xrmdst.size == sizeof(char))
    rval = *((char*)xrmdst.addr);
  else if (xrmdst.size == sizeof(short))
    rval = *((short*)xrmdst.addr);
  else if (xrmdst.size == sizeof(int))
    rval = *((int*)xrmdst.addr);
  else if (xrmdst.size == sizeof(long))
    rval = (long) *((long*)xrmdst.addr);
  else {
    rval = (long) XtMalloc(xrmdst.size);
    (void) memcpy((char *)rval, xrmdst.addr, xrmdst.size);
  }
  
  *ok = 1;
  return ((XtPointer)rval);
}



/****************************************************************
 *
 * Utilities for converters...
 *
 ****************************************************************/
/* This macro is used to terminate a converter */
/* See Xt Intrisic manual page ...             */
#define done(type, value) \
  { \
    if (toVal->addr != NULL) { \
      if (toVal->size < sizeof(type)) { \
	toVal->size = sizeof(type); \
	return False; \
      }  \
      *(type *)(toVal->addr) = (value); \
    } \
    else { \
      static type static_val; \
      static_val = (value); \
      toVal->addr = (XtPointer) &static_val; \
    } \
    toVal->size = sizeof(type); \
    return True; \
  }


/****************************************************************
 * copy_string:
 *
 *    Copies a string and allocates memory for the new string.
 *    A reference to the new string is returned. If no memory can 
 *    be allocated, NULL is returned.
 *
 ****************************************************************/
static char *copy_string(str)
    char * str;
{
  int    len;
  char * new;
   
  if (str == NULL) 
    return (NULL);
  len = strlen(str);
  new = (char *) malloc((unsigned int)len+1);
  if (!new)
    return (NULL);
  (void) memcpy((char *)new, (char *)str, len+1);
  return (new);
}



/****************************************************************
 * 
 * TeleUSE-specific converters for Motif
 *
 ****************************************************************/

#define MAXSPLITSTRINGS  200


/****************************************************************
 * StringsAreEqual:
 *   Compare two strings and return true if equal.
 *   The comparison is on lower cased strings.  It is the caller's
 *   responsibility to ensure that test_str is already lower cased.
 ****************************************************************/
static Boolean StringsAreEqual(in_str, test_str)
     register char * in_str;
     register char * test_str;
{
  register int i;
  register int j;

  i = *in_str;
  if (((in_str[0] == 'X') || (in_str[0] == 'x')) &&
      ((in_str[1] == 'M') || (in_str[1] == 'm')))
    in_str +=2;
  
  for (;;)
    {
      i = *in_str;
      j = *test_str;
      
      if (isupper (i)) i = tolower(i);
      if (i != j) return (False);
      if (i == 0) return (True);
      in_str++;
      test_str++;
    }
}


/****************************************************************
 * is_japanix:
 *    check whether its an extension character
 ****************************************************************/
static int is_japanix(ch)
     int ch;
{
  ch &= 0xff;
  return (ch > 127);
}


/****************************************************************
 * create_string_list:
 *    This function splits one string into a number of
 *    smaller strings.
 *    The string is divided into strings by means of delimiters.
 *    A delimiter is a punctuation character (non-alphanumeric,
 *    non-control). Delimiters like [{<( closes with )>}].
 *    The string between two equal delimiters is considered to be
 *    one sub-string. 
 *    The procedure the number of substrings extracted,
 *    the substrings in a string list, and a boolean that tells
 *    if the operation was successful or not. 
 ****************************************************************/
static Boolean create_string_list(str, pList, pCnt)
     char *str;
     char ***pList;
     int *pCnt;
{
  char *list[MAXSPLITSTRINGS];
  int cnt = 0;
  char *chp = str;
  char *chs;
  char delimiter;
  int len;
  char **res;
  
  while (*chp) {
    while (isspace(*chp))
      ++chp;
    if (*chp == '\0') break;
    
    chs = chp;
    delimiter = *chp++;
    if (!is_japanix(delimiter) && ispunct(delimiter)) {
      ++chs;
      if (delimiter == '[') delimiter = ']';
      else if (delimiter == '<') delimiter = '>';
      else if (delimiter == '{') delimiter = '}';
      else if (delimiter == '(') delimiter = ')';
      
      while (*chp && (*chp != delimiter)) chp++;
      
    } else {
      while (*chp && *chp != '\n') {
	if (*chp == '\\' && chp[1] == '\n')
	  ++chp;
	chp++;
      }
    }

    len = chp-chs;
    if (len > 0) {
      list[cnt] = (char *) malloc((unsigned int)len+1);
      if (!list[cnt]) 
	return False;
      (void) strncpy(list[cnt], chs, len);
      list[cnt][len] = '\0';
      cnt++;
      if (*chp) chp++;
    }
    else 
      return False;
  }
  
  list[cnt++] = NULL;
  res = (char **) malloc((unsigned int)cnt*sizeof(char *));
  if (res) 
    (void) memcpy((XtPointer)res, (XtPointer)list,
		     cnt*sizeof(char *));
  else
    return False;
  
  *pList = res;
  *pCnt = cnt-1;
  return ((*chp) ? False : True);
}


/****************************************************************
 * free_strings:
 *   Frees strings allocated by the 'split_string' procedure.
 *   The list and the string pointed to be the list are freed.
 *   The list must be NULL terminated.
 ****************************************************************/
static void free_strings(list)
     char **list;
{
  char **save = list;
  
  if (!list)
    return;
  
  while (*list) {
    free((char *)*list);
    list++;
  }
  free((char *)save);
}


/****************************************************************
 * tu_cvt_string_to_xmstring
 *	Converts an ASCII string to a compound String (XmString),
 *	with a somewhat different syntax than the OSF/Motif default.
 *	Syntax:
 *	  \C"character set name" -- specify a new character set
 *	  \D			 -- default character set
 *	  \L			 -- Left-to-right orientation
 *	  \R			 -- Right-to-left orientation	
 *	  \\			 -- Single backslash
 *	  \n			 -- newline
 *	  <newline>		 -- newline
 *	  \<octal number>	 -- byte a la C-string syntax
 *	  \<null>		 -- embedded '\0'
 ****************************************************************/
/*CExtract*/
XmString tu_cvt_string_to_xmstring(str)
     char *str;
{
  XmString xs;
  register char *s;
  char *p, *dst;
  char c;
#ifdef OPENWARE
  XmStringCharSet *dcs_array = (XmStringCharSet *)XmjpDEFAULT_CHARSET_ARRAY;
  XmStringCharSet *charset_array = dcs_array;
#endif  
  XmStringCharSet dcs = (XmStringCharSet)XmSTRING_DEFAULT_CHARSET;
  XmStringCharSet charset = dcs;
  XmString x1, x2;

  if (str == NULL)
    return NULL;

  s = str;
  while (*s)
    if (*s == '\n' || ((*s == '\\') && (s[1] != '\0')))
      break;
    else
      s++;

  if (*s == 0)
#ifdef OPENWARE
    return XmjpCreateConcatWords(str, XmjpDEFAULT_CHARSET_ARRAY);
#else
    return XmStringCreate(str, XmSTRING_DEFAULT_CHARSET);
#endif  
  xs = NULL;
  if (str == NULL) str = "";
  s = XtMalloc(strlen(str) + 1);
  dst = s;
  p = s;

  while ((c = *str++) != '\0') {
    if (c == '\n') {
      *s = '\0';
      if (p != s) {
	x1 = xs;
#ifdef OPENWARE
	x2 = XmjpCreateConcatWords(p, charset_array);
#else
	x2 = XmStringCreate(p, charset);
#endif
	xs = XmStringConcat(x1, x2);
	XmStringFree(x1);
	XmStringFree(x2);
      }
      x1 = xs;
      x2 = XmStringSeparatorCreate();
      xs = XmStringConcat(x1, x2);
      XmStringFree(x1);
      XmStringFree(x2);
      p = ++s;
    } else if ((c == '\\') && (*str != '\0')) {

      switch (c= *str++) {
      case 'C':			/* Characterset */
	if (p != s) {
	  *s++= '\0';
	  x1 = xs;
#ifdef OPENWARE
	  x2 = XmjpCreateConcatWords(p, charset_array);
#else
	  x2 = XmStringCreate(p, charset);
#endif
	  xs = XmStringConcat(x1, x2);
	  XmStringFree(x1);
	  XmStringFree(x2);
	  p = s;
	}
	c = *str++;

	if (c == '"') {
	  if ((p= strchr(str, '"')) != NULL) {
	    p = XtMalloc(p-str+1);
	    if (charset != (XmStringCharSet)dcs)
	      XtFree(charset);
	    charset = p;

	    while ((c= *str++) != '"' && c != '\0')
	      *p++ = c;

	    *p = '\0';
	    p = s;
	  }

	} else if (c == '\'') {
	  if ((p= strchr(str, '\'')) != NULL) {
	    p = XtMalloc(p-str+1);
	    if (charset != (XmStringCharSet)dcs)
	      XtFree(charset);
	    charset = p;
	    
	    while ((c= *str++) != '\'' && c != '\0')
	      *p++ = c;
	    
	    *p = '\0';
	    p = s;
	  }

	} else {
	  *s++ = 'C';
	}
	break;

      case 'D':
	if (p != s) {
	  *s++ = '\0';
	  x1 = xs;
#ifdef OPENWARE
	  x2 = XmjpCreateConcatWords(p, charset_array);
#else
	  x2 = XmStringCreate(p, charset);
#endif
	  xs = XmStringConcat(x1, x2);
	  XmStringFree(x1);
	  XmStringFree(x2);
	  p = s;
	}
	if (charset != (XmStringCharSet)dcs)
	  XtFree(charset);
	charset = (XmStringCharSet)dcs;
	break;

      case 'L':			/* Left To Right */
	if (p != s) {
	  *s++= '\0';
	  x1 = xs;
#ifdef OPENWARE
	  x2 = XmjpCreateConcatWords(p, charset_array);
#else
	  x2 = XmStringCreate(p, charset);
#endif
	  xs = XmStringConcat(x1, x2);
	  XmStringFree(x1);
	  XmStringFree(x2);
	  p = s;
	}
	x1 = xs;
	x2 = XmStringDirectionCreate(XmSTRING_DIRECTION_L_TO_R);
	xs = XmStringConcat(x1, x2);
	XmStringFree(x1);
	XmStringFree(x2);
	break;

      case 'R':			/* Right To Left */
	if (p != s) {
	  *s++= '\0';
	  x1 = xs;
#ifdef OPENWARE
	  x2 = XmjpCreateConcatWords(p, charset_array);
#else
	  x2 = XmStringCreate(p, charset);
#endif
	  xs = XmStringConcat(x1, x2);
	  XmStringFree(x1);
	  XmStringFree(x2);
	  p = s;
	}
	x1 = xs;
	x2 = XmStringDirectionCreate(XmSTRING_DIRECTION_R_TO_L);
	xs = XmStringConcat(x1, x2);
	XmStringFree(x1);
	XmStringFree(x2);
	break;

      case '\\':		/* Single backslash */
	*s++ = '\\';
	break;

      case '\0':		/* Ascii NULL */
	*s++ = '\0';
	break;

#ifdef notdef
      case 'f':			/* Formfeed */
	*s++ = '\f';
	break;

      case 'r':			/* Return */
	*s++ = '\r';
	break;

      case 't':			/* Tab */
	*s++ = '\t';
	break;
#endif

      case 'n':			/* Newline */
	*s = '\0';
	if (p != s) {
	  x1 = xs;
#ifdef OPENWARE
	  x2 = XmjpCreateConcatWords(p, charset_array);
#else
	  x2 = XmStringCreate(p, charset);
#endif
	  xs = XmStringConcat(x1, x2);
	  XmStringFree(x1);
	  XmStringFree(x2);
	}
	x1 = xs;
	x2 = XmStringSeparatorCreate();
	xs = XmStringConcat(x1, x2);
	XmStringFree(x1);
	XmStringFree(x2);
	p = ++s;
	break;

      default:
	if ('0' <= c && c <= '7') {
	  *s = c - '0';
	  c = *str;
	  if ('0' <= c && c <= '7') {
	    *s = (*s) * 8 + c - '0';
	    c = *++str;
	    if ('0' <= c && c <= '7') {
	      *s = (*s) * 8 + c - '0';
	      ++str;
	    }
	  }
	} else {
	  *s = c;
	}
	++s;
      }
    } else {
      *s++ = c;
    }
  }
  *s = '\0';

  if (p != s) {
    x1 = xs;
#ifdef OPENWARE
    x2 = XmjpCreateConcatWords(p, charset_array);
#else
    x2 = XmStringCreate(p, charset);
#endif
    xs = XmStringConcat(x1, x2);
    XmStringFree(x1);
    XmStringFree(x2);
  }

#ifdef OPENWARE
  if (xs == NULL) {
    x2 = XmjpCreateConcatWords(dst, charset_array);
  }
#else
  if (xs == NULL) {
    xs = XmStringCreate(dst, charset);
  }
#endif
  if (charset != (XmStringCharSet)dcs)
    XtFree(charset);
  XtFree(dst);

  return xs;
}


/****************************************************************
 * tu_cvt_xmstring_to_string
 *     This function returns a normal string for an xmstring,
 *     which can be converted back to the same xmstring.
 ****************************************************************/
/*CExtract*/
char *tu_cvt_xmstring_to_string(str, charset_info)
     XmString str;
     int charset_info;
{
  XmStringContext ctxt;
  String text;
  String s;
  XmStringCharSet charset;
  XmStringDirection direction;
  XmStringComponentType component;
  unsigned char tag;
  unsigned short unknown_length;
  unsigned char *unknown_value;
  int inc, len;
#ifdef OPENWARE
  String js;
  int	 jlen;
#endif
  
  if (str == NULL || !XmStringInitContext(&ctxt, str))
    return NULL;
  s = NULL;
  len = 0;

#ifdef OPENWARE
  XmjpGetLtoRWords(str,XmjpDEFAULT_CHARSET_ARRAY,&js);
  jlen = 0;
#endif

  for (;;) {
    
    component =
      XmStringGetNextComponent(ctxt, &text, &charset, &direction, &tag,
			       &unknown_length, &unknown_value);
    switch (component)
      {      
      case XmSTRING_COMPONENT_CHARSET:
	if (charset_info == False) { 
	  XtFree(charset);
	  break; 
	}
	if (charset != (XmStringCharSet)XmSTRING_DEFAULT_CHARSET) {
	  inc = strlen(charset) + 4;
	  s = XtRealloc(s, len+inc+1);
	  (void) strcpy(&s[len], "\\C'");
	  (void) strcpy(&s[len+3], charset);
	  len += inc;
	  s[len-1]= '\'';
	  s[len]= '\0';
	  XtFree((char *)charset);
	}
	break;
	
      case XmSTRING_COMPONENT_TEXT:
      case XmSTRING_COMPONENT_LOCALE_TEXT:
	inc= strlen(text);
	s= XtRealloc(s, len+inc+1);               /* Plus '\0' */
#ifdef	OPENWARE
	(void) strncpy(&s[len], js, inc);
	js += inc;
#else
	(void) strcpy(&s[len], text);
#endif
	len += inc;
	XtFree(text);
	break;
	
      case XmSTRING_COMPONENT_UNKNOWN:
	XtFree((char *)unknown_value);
	XmStringFreeContext(ctxt);
	return s;
	
      case XmSTRING_COMPONENT_DIRECTION:
	if (charset_info) {
	  s= XtRealloc(s, len+2+1);
	  if (direction == XmSTRING_DIRECTION_L_TO_R)
	    (void) strcpy(&s[len], "\\L");
	  else if (direction == XmSTRING_DIRECTION_R_TO_L)
	    (void) strcpy(&s[len], "\\R");
	  len += 2;
	}
	break;
	
      case XmSTRING_COMPONENT_SEPARATOR:
	s= XtRealloc(s, len+1+1);
	s[len] = '\n';
	s[len+1] = '\0';
	len += 1;
	break;
	
      case XmSTRING_COMPONENT_END:
#ifdef	OPENWARE
	s[len] = '\0';
#endif
	XmStringFreeContext(ctxt);
	return s;
	
      default:
	break;
      }
  }
}


/************************************************************************
 * _XmCvtStringToXmString:
 *   Convert a string to a XmString.
 *   'string' may contain \n and linefeed to indicate a separator
 ************************************************************************/
/*ARGSUSED*/
static Boolean _XmCvtStringToXmString(display, args, num_args, 
				      fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  static XmString xs;

  if (*num_args != 0)
    XtError("String to XmString conversion needs no arguments");

  if (fromVal->addr == NULL)
    return False;

  xs = tu_cvt_string_to_xmstring((char *)fromVal->addr);
  *converter_data = NULL;

  done(XmString, xs)
}


/****************************************************************
 * _XmCvtFreeXmString:
 *    Frees up the memory used by an XmString.
 ****************************************************************/
/*ARGSUSED*/
static void _XmCvtFreeXmString(app, to, data, args, num_args)
     XtAppContext  app;
     XrmValue    * to;
     XtPointer     data;
     XrmValue    * args;
     Cardinal    * num_args;
{
  XmString xs;

  xs = *((XmString *) to->addr);
  if (xs) 
    XmStringFree(xs);
}


/************************************************************************
 * _XmCvtStringToXmStringTable:
 *     Convert a string to an XmStringTable.
 ************************************************************************/
/*ARGSUSED*/
static Boolean _XmCvtStringToXmStringTable(display, args, num_args, 
					   fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  static XmString *xm_list;
  String *list, str;
  int list_count, i;
  Boolean got_list;

  *converter_data = NULL;
  if (*num_args != 0)
    XtError("String to XmStringTable conversion needs no arguments");

  str = (char *)fromVal->addr;
  if (str == NULL)
    return False;

  list_count = 0;
  xm_list = NULL;

  got_list = create_string_list(str, &list, &list_count);
  if (!got_list || list == NULL)
    return False;

  xm_list = (XmString *)XtMalloc((list_count + 1) * sizeof (XmString));
  for (i = 0; i < list_count; i++)
    xm_list[i] = tu_cvt_string_to_xmstring(list[i]);

  xm_list[list_count] = NULL;
  free_strings(list);

  done(XmString *, xm_list)
}


/****************************************************************
 * _XmCvtFreeXmStringTable:
 *    Frees up the memory used by an XmStringTable.
 ****************************************************************/
/*ARGSUSED*/
static void _XmCvtFreeXmStringTable(app, to, data, args, num_args)
     XtAppContext  app;
     XrmValue    * to;
     XtPointer     data;
     XrmValue    * args;
     Cardinal    * num_args;
{
  XmStringTable xst;
  int i;

  xst = *((XmStringTable *) to->addr);
  if (xst) {
    for (i=0;xst[i];i++)
      XmStringFree(xst[i]);
    XtFree((char *)xst);
  }
}


/************************************************************************
 * _CvtStringToStringTable:
 *     Convert a string to a StringTable.
 ************************************************************************/
/*ARGSUSED*/
static Boolean _CvtStringToStringTable(display, args, num_args, 
				       fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  String *list, str;
  int list_count, i;
  Boolean got_list;

  *converter_data = NULL;
  if (*num_args != 0)
    XtError("String to StringTable conversion needs no arguments");

  str = (char *)fromVal->addr;
  if (str == NULL)
    return False;

  list_count = 0;

  got_list = create_string_list(str, &list, &list_count);
  if (!got_list || list == NULL)
    return False;

  done(String *, list)
}


/****************************************************************
 * _CvtFreeStringTable:
 *    Frees up the memory used by a StringTable.
 ****************************************************************/
/*ARGSUSED*/
static void _CvtFreeStringTable(app, to, data, args, num_args)
     XtAppContext  app;
     XrmValue    * to;
     XtPointer     data;
     XrmValue    * args;
     Cardinal    * num_args;
{
  String * xst;
  int i;

  xst = *((String **) to->addr);
  if (xst) 
    free_strings(xst);
}


/****************************************************************
 * CvtStringToTextPosition:
 *   This function converts a string to an integer number.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToTextPosition(display, args, num_args, 
				       fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  static int  val;
  char         * str;

  *converter_data = NULL;
  str = (char *) fromVal->addr;
  (void) sscanf(str, "%d", &val);
  done(int, val)
}


/****************************************************************
 * CvtStringToLong:
 *   This function converts a string to a long integer.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToLong(display, args, num_args, 
			       fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  static long  val;
  char * str;

  *converter_data = NULL;
  str = (char *) fromVal->addr;
  val = strtol(str, (char**)NULL, 10);
  done(long, val)
}



/************************************************************************
 * CvtStringToSelectionDialogType:
 *   Special converter function for the XmSelectionBox widget's
 *   XmNdialogType attribute.  (name clash with XmMessageBox's ditto).
 ************************************************************************/
/* ARGSUSED */
static Boolean CvtStringToSelectionDialogType(display, args, num_args, 
					      fromVal, toVal, 
					      converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  char                 * in_str = (char *) (fromVal->addr);
  static unsigned char   i;

  *converter_data = NULL;
  if (StringsAreEqual(in_str, "dialog_work_area"))
    i = XmDIALOG_WORK_AREA;
  else if (StringsAreEqual(in_str, "dialog_prompt"))
    i = XmDIALOG_PROMPT;
  else if (StringsAreEqual(in_str, "dialog_selection"))
    i = XmDIALOG_SELECTION;
  else if (StringsAreEqual(in_str, "dialog_command"))
    i = XmDIALOG_COMMAND;
  else if (StringsAreEqual(in_str, "dialog_file_selection"))
    i = XmDIALOG_FILE_SELECTION;
  else {
    XtStringConversionWarning ((char *)fromVal->addr, XmRDialogType);
    return False;
  }
  done(unsigned char, i)
}


/************************************************************************
 * CvtStringToMwmInputMode:
 *   Special converter function for the Vendor shell mwmInputMode
 ************************************************************************/
/* ARGSUSED */
static Boolean CvtStringToMwmInputMode(display, args, num_args, 
				       fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  char       * in_str = (char *) (fromVal->addr);
  static int   i;

  *converter_data = NULL;
  if (StringsAreEqual(in_str, "mwm_input_modeless"))
    i = MWM_INPUT_MODELESS;
  else if (StringsAreEqual(in_str, "mwm_input_primary_application_modal"))
    i = MWM_INPUT_PRIMARY_APPLICATION_MODAL;
  else if (StringsAreEqual(in_str, "mwm_input_system_modal"))
    i = MWM_INPUT_SYSTEM_MODAL;
  else if (StringsAreEqual(in_str, "mwm_input_full_application_modal"))
    i = MWM_INPUT_FULL_APPLICATION_MODAL;
  else {
    XtStringConversionWarning ((char *)fromVal->addr, TuRMwmInputMode);
    return False;
  }
  done(int, i)
}


/************************************************************************
 * CvtStringToMultiClick:
 *   Converts a string to a MultiClick.
 ************************************************************************/
/* ARGSUSED */
static Boolean CvtStringToMultiClick(display, args, num_args, 
				     fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  char                 * in_str = (char *) (fromVal->addr);
  static unsigned char   i;
  
  *converter_data = NULL;
  if (StringsAreEqual(in_str, "multiclick_discard"))
    i = XmMULTICLICK_DISCARD;
  else if (StringsAreEqual(in_str, "multiclick_keep"))
    i = XmMULTICLICK_KEEP;
  else {
    XtStringConversionWarning ((char *)fromVal->addr, XmRMultiClick);
    return False;
  }
  done(unsigned char, i)
}


/****************************************************************
 * CvtStringToIString:
 *   Converts a XmRString to a TuRIString. 
 *   Dummy function.
 *   The real function is defined and added in the EHT package.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToIString(display, args, num_args, 
				  fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  char          * in_str;
  char          * converted_str;
  static char   * out_str;

  *converter_data = NULL;
  in_str  = (char *) (fromVal->addr);
  out_str = in_str;

  done(String, out_str)
}


/************************************************************************
 * CvtStringToMnemonic:
 *   Converts a string to a Mnemonic. Motif does not have an XmRString
 *   to XmRMnemonic type converter. The attribute XmNmnemonic is actually
 *   of the type XmRKeySym (for which exists a String->KeySym converter)
 *
 *   Dummy function.
 *   The real function is defined and added in the EHT package.
 *
 *   The reason for this converter is that we want to be able to specify
 *   an EHT error/help string code so that we can handle multiple languages
 *   for the mnemonic.
 *   This means that we will to have define the XmNmnemonic attribute
 *   as being of the resource type XmRMnemonic instead of what OSF has
 *   defined it as, which is XmRKeySym, but this should not be a problem.
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean CvtStringToMnemonic(display, args, num_args, 
				   fromVal, toVal, converter_data)
     Display            * display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer          * converter_data;
{
  static KeySym   key_sym;
  char          * in_str;
  char          * ep;
  XrmValue        xrmstr;
  XrmValue        xrmdst;
  long            v;

  *converter_data = NULL;
  in_str = (char *) (fromVal->addr);

  /* special hack for UIL */
  v = strtol(in_str, &ep, 0);
  if ((v >= 0x20) &&
      (!ep || (ep < in_str) || (ep >= (in_str + strlen(in_str))))) {
    key_sym = (KeySym) v;
    goto out;
  }

  /*
   *  Call the X Converter for converting a XtRString to XtRKeySym.
   */
  xrmstr.size = strlen(in_str);
  xrmstr.addr = (XPointer) in_str;
  xrmdst.addr = (XPointer) NULL;

  if (!XtConvertAndStore(tu_global_top_widget,
			 XmRString, &xrmstr,
			 XmRKeySym, &xrmdst)) {
    /*
     *  Error in conversion
     */
    XtStringConversionWarning ((char *)in_str, XmRMnemonic);
    return False;
  }

  key_sym = *(KeySym *) xrmdst.addr;

 out:
  done(KeySym, key_sym)
}


/****************************************************************
 * tu_xm_resource_converters:
 *    This procedure initializes the converters added to Xm
 *    by TeleUSE.
 ****************************************************************/
/*CExtract*/
void tu_xm_resource_converters()
{
  XmRepTypeInstallTearOffModelConverter();

  {
    static char* access_names[] =
      {TuNAccessNone,
       TuNAccessInternal,
       TuNAccessDiscard,
       TuNAccessLocked,
       TuNAccessAny};
    static unsigned char access_values[] = {0, 1, 2, 3, 4};
    (void)XmRepTypeRegister(TuRNodeAccessibility,
		            access_names,
		            access_values,
		            XtNumber(access_names));
  }					

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, XmRXmString,
			_XmCvtStringToXmString, 
			(XtConvertArgList) NULL, 0,
			XtCacheAll | XtCacheRefCount, _XmCvtFreeXmString);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, XmRXmStringTable,
			_XmCvtStringToXmStringTable, 
			(XtConvertArgList) NULL, 0,
			XtCacheAll | XtCacheRefCount, _XmCvtFreeXmStringTable);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, XmRStringTable,
			_CvtStringToStringTable, 
			(XtConvertArgList) NULL, 0,
			XtCacheAll | XtCacheRefCount, _CvtFreeStringTable);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, TuRSelectionDialogType,
			CvtStringToSelectionDialogType,
			(XtConvertArgList) NULL, 0,
			XtCacheAll, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, TuRMwmInputMode,
			CvtStringToMwmInputMode,
			(XtConvertArgList) NULL, 0,
			XtCacheAll, NULL);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, XmRTextPosition, 
			CvtStringToTextPosition,
			(XtConvertArgList) NULL, (Cardinal)0,
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, XmRMultiClick,
			CvtStringToMultiClick,
			(XtConvertArgList) NULL, 0,
			XtCacheAll, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, TuRIString,
			CvtStringToIString,
			(XtConvertArgList) NULL, 0,
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRString, XmRMnemonic,
			CvtStringToMnemonic,
			(XtConvertArgList) NULL, 0,
			XtCacheNone, NULL);
}



/****************************************************************
 *
 * TeleUSE-specific resource converters ...
 *
 ****************************************************************/


#define MAXDIRS          100
#define MAXWIDGETS       100


/* Image directory lists */

static char *image_dirs[MAXDIRS] = { NULL };

typedef struct _name_convert_table {
  char   * name;
  XtPointer  value;
} name_convert_tbl_t;

/****************************************************************
 * append_default:
 *    This function appends the X default list to the pathlist.
 *    This is to be compatible with Xmu converters.
 ****************************************************************/
static void append_default(list, last)
      char **list;
      int last;
{
#define SEPARATORS ", :"

  Display *dpy;
  char *def_path;
  char buf[200];
  char *pp;

  if (tu_application_name == NULL)
    return;

  dpy = tu_disp_get_dpy((char *) NULL);
  if (dpy == NULL) 
    return;

  def_path = XGetDefault(dpy, tu_application_name, "bitmapFilePath");
  if (def_path == NULL)
    return;

  pp = buf;
  while (*def_path) {
    if (strchr(SEPARATORS, *def_path)) {
      *pp = '\0';
      if ((int)strlen(buf) > 0) 
	list[last++] = copy_string(buf);
      pp = buf;
      def_path++;
    } else {
      if (*def_path == '\\' && def_path[1] != '\0')
	def_path++;
      *pp++ = *def_path++;
    }
  }
  *pp = '\0';
  if ((int)strlen(buf) > 0) 
    list[last++] = copy_string(buf);

  list[last] = NULL;

#undef SEPARATORS
}


/****************************************************************
 * tu_set_image_path:
 *    This function sets the image path. The value of the 
 *    bitmapFilePath resource variable (which is the value that 
 *    normal X applications uses) is appended to the path.
 ****************************************************************/
/*CExtract*/
void tu_set_image_path(path_list, cnt)
     char  **path_list;
     int     cnt;
{
  int i;
  char buf[MAXLEN];
  for (i=0;image_dirs[i];i++)
    free((char *)image_dirs[i]);
  for (i=0;i<cnt;i++) {
    expand_symbols(path_list[i],buf,MAXLEN);
    image_dirs[i] = tu_strdup(buf);
  }
  image_dirs[i] = NULL;  
  append_default(image_dirs, i);
}


/****************************************************************
 * tu_get_image_path:
 *    This function returns the image path. The path is a NULL
 *    terminated string list. The caller is responsible for freeing.
 ****************************************************************/
/*CExtract*/
char **tu_get_image_path()
{
  char **local_image_dirs;
  int i;

  for (i=0;image_dirs[i];i++);
  local_image_dirs = (char **)calloc(i+1, sizeof(char *));

  for (i=0;image_dirs[i];i++)
    local_image_dirs[i] = tu_strdup(image_dirs[i]);
  local_image_dirs[i] = NULL;
  return local_image_dirs;
}


/****************************************************************
 * read_compiled_pixmap:
 *    This function reads a compiled-in pixmap.
 * 
 ****************************************************************/
static tu_pixmap read_compiled_pixmap(uxpm)
     tu_pmdefs_p uxpm;
{
  char *p;
  char *tag, *color;
  struct tu_color_descr **ct, **ctc;
  struct tu_pixmap_color_pair *cp;
  struct tu_pixmap_color_pair **prev_p;
  int i, j, k;
  tu_pixmap pds;
  char buf[10];

  pds = (tu_pixmap)XtMalloc(sizeof(struct _tu_pixmap));

  /* clear the pds structure before we start */
  (void) memset((char *)pds, '\0', sizeof(struct _tu_pixmap));

  p = uxpm->pixmap[0];
  /* Pick up pixmap configuration */
  if (sscanf(p, "%d %d %d %d", 
	     &pds->pds_width, &pds->pds_height,
	     &pds->pds_ncolors, &pds->pds_nchars) != 4)
    goto error;

  ct = (struct tu_color_descr **) 
    XtMalloc(pds->pds_ncolors * sizeof(struct tu_color_descr *));
  pds->pds_ct = ct;

  for (i=0;i<pds->pds_ncolors;i++) {
    p = uxpm->pixmap[i+1];

    ct[i] = (struct tu_color_descr *) XtMalloc(sizeof(struct tu_color_descr));
    ct[i]->ct_ch = (char *) XtMalloc(pds->pds_nchars+1);
    for (k=0;k<pds->pds_nchars;k++)
      ct[i]->ct_ch[k] = *p++;
    ct[i]->ct_ch[k] = '\0';
    ct[i]->ct_index = i;
    ct[i]->ct_colors = NULL;

    prev_p = &ct[i]->ct_colors;
    for (;*p;) {
      tag = getid(&p, (char*)NULL);
      if (tag == NULL) 
	goto error;
      SKIPSP(p);
      
      color = getid(&p, "#");
      if (color == NULL) {
	free(tag);
	goto error;
      }
      SKIPSP(p);
      
      cp = (struct tu_pixmap_color_pair *) 
	XtMalloc(sizeof(struct tu_pixmap_color_pair));
      cp->name = tag;
      cp->color = color;
      if (prev_p)
	*prev_p = cp;
      prev_p = &cp->next;
      cp->next = NULL;
    }
  }

  if (pds->pds_nchars == 1) {
    /* special case - make fast index table */
    unsigned char lkup_tbl[256];
    unsigned char *data, *dp;

    for (i=0;i<256;i++) lkup_tbl[i] = 0;
    for (i=0;i<pds->pds_ncolors;i++)
      lkup_tbl[(unsigned int)ct[i]->ct_ch[0]] = i;

    data = (unsigned char *) 
      malloc((unsigned)(pds->pds_width*pds->pds_height));
    if (data == NULL)
      goto error;
    pds->pds_data = data;

    dp = data;
    for (i=0;i<pds->pds_height;i++) {
      p = uxpm->pixmap[i+pds->pds_ncolors+1];
      for (j=0;j<pds->pds_width;j++)
	*dp++ = lkup_tbl[(unsigned int)*p++];
    }
  } else {
    /* Make it general but extremely slow */
    char ch;
    int cnt;
    unsigned char *data, *dp;
    unsigned short *data16, *dp16;
    struct tu_color_descr **lkup_tbl[256];

    for (k=0;k<256;k++) {
      ch = (char) k;
      
      /* Build a table for faster lookup */
      cnt = 0;
      for (i=0;i<pds->pds_ncolors;i++) 
	if (ct[i]->ct_ch[0] == ch) cnt++;
      lkup_tbl[k] = (struct tu_color_descr **)
	XtMalloc((cnt+1)*sizeof(struct tu_color_descr *));
      if (cnt > 0) {
	cnt = 0;
	for (i=0;i<pds->pds_ncolors;i++) 
	  if (ct[i]->ct_ch[0] == ch) 
	    lkup_tbl[k][cnt++] = ct[i];
      }
      lkup_tbl[k][cnt] = NULL;
    }      

    if (pds->pds_ncolors > 256) {
      data16 = (unsigned short *) 
	malloc((unsigned)(pds->pds_width*
			  pds->pds_height*
			  sizeof(unsigned short)));
      if (data16 == NULL)
	goto error;
      dp16 = data16;
      pds->pds_data = (unsigned char *) data16;
    } else {
      data = (unsigned char *) 
	malloc((unsigned)(pds->pds_width*
			  pds->pds_height*
			  sizeof(unsigned char)));
      if (data == NULL)
	goto error;
      dp = data;
      pds->pds_data = (unsigned char *) data;
    }

    for (i=0;i<pds->pds_height;i++) {
      p = uxpm->pixmap[i+pds->pds_ncolors+1];
      for (j=0;j<pds->pds_width;j++) {
	for (k=0;k<pds->pds_nchars;k++) buf[k] = *p++;
	buf[k] = '\0';
	for (ctc = lkup_tbl[(unsigned int)buf[0]];
	     *ctc;
	     ctc++)
	  if (strcmp(buf, (*ctc)->ct_ch) == 0) {
	    if (pds->pds_ncolors > 256)
	      *dp16++ = (*ctc)->ct_index;
	    else
	      *dp++ = (*ctc)->ct_index;
	    break;
	  }
      }
    }

    for (k=0;k<256;k++) 
      free((char *)lkup_tbl[k]);
  }

  return pds;

 error:
  tu_pds_free(pds);
  return NULL;
}

/****************************************************************
 * tu_register_bitmaps:
 *   This functions will add a set of bitmaps from either a 
 *   reusable GUI component or a main application to the 
 *   'tu_bmdefs' structure.
 ****************************************************************/
void tu_register_bitmaps (bitmaps, n_bm)
tu_bmdefs_p bitmaps;
int n_bm;
{
  int i;
  tu_bmdefs = (tu_bmdefs_p*)XtRealloc((char*)tu_bmdefs,
    (tu_bmcount + n_bm + 1) * sizeof(tu_bmdefs_p));
  for (i=0; i<n_bm; i++)
    tu_bmdefs[tu_bmcount++] = &(bitmaps[i]);
  tu_bmdefs[tu_bmcount] = NULL;
}

/****************************************************************
 * tu_make_bitmap:
 *   This function converts a string to a bitmap, i.e., a one-bit
 *   deep pixmap. The bitmap is read in the bitmap(1)
 *   or pixmap (XPM3) format. The function first tries to match a
 *   compiled-in bitmap with the name specified, then a 
 *   compiled-in bitmap with name <name>.xbm, then a file with the
 *   specified name, then a file with name <name>.xbm. If none of
 *   these match, the procedure is repeated for pixmap files, with
 *   extension .xbm replaced with .xpm. Extensions are only added
 *   if the name does not contain a dot.
 ****************************************************************/
Pixmap tu_make_bitmap(name, screen, pmdefs, bmdefs)
     char * name;
     Screen *screen;
     tu_pmdefs_p* pmdefs;
     tu_bmdefs_p* bmdefs;
{
  Pixmap             pixmap;
  char             * filename;
  unsigned int       width;
  unsigned int       height;
  int                xhot;
  int                yhot;
  int                i;
  tu_bmdefs_p        uxbm;
  tu_pmdefs_p        uxpm;
  char               buf[MAXLEN];
  tu_pixmap          pds;

  if ((int)strlen(name) > (MAXLEN/2))
    return None;

  /* Add .xbm */
  (void) sprintf(buf, "%s.xbm", name);

  if (bmdefs) {
    for (i=0; uxbm=bmdefs[i]; i++)
      if (strcmp(name, uxbm->name) == 0) {
	pixmap = 
	  XCreateBitmapFromData(DisplayOfScreen(screen),
				RootWindowOfScreen(screen),
				(char *)uxbm->bitmap,
				uxbm->width,
				uxbm->height);
	goto got_it;
      }
    
    for (i=0; uxbm=bmdefs[i]; i++)
      if (strcmp(buf, uxbm->name) == 0) {
	pixmap = 
	  XCreateBitmapFromData(DisplayOfScreen(screen),
				RootWindowOfScreen(screen),
				(char *)uxbm->bitmap,
				uxbm->width,
				uxbm->height);
	goto got_it;
      }
  }
    
  /* not a compiled-in bitmap, try to find the file */
  filename = tu_find_file(name, image_dirs);

  if (filename == NULL) 
    /* try with added .xbm */
    filename = tu_find_file(buf, image_dirs);

  if (filename != NULL) {
    if (XReadBitmapFile(DisplayOfScreen(screen), RootWindowOfScreen(screen),
			filename, &width, &height, &pixmap, &xhot, &yhot)
	!= BitmapSuccess) {
      if (filename) free(filename);
      return None;
    }
    if (filename) free(filename);
    goto got_it;
  }

  /* No bitmaps with this name, try with pixmap */
  /* string literal ? */
  pds = tu_pds_read_xpm_string(name);
  /* add .xpm */
  (void) sprintf(buf, "%s.xpm", name);

  if (!pds) {
    if (pmdefs) {
      for (i=0; uxpm=pmdefs[i]; i++)
	if (strcmp(name, uxpm->name) == 0) {
	  pds = read_compiled_pixmap(uxpm);
	  break;
	}
      
      if (!pds && !strchr(name, '.')) {
	for (i=0; uxpm=pmdefs[i]; i++)
	  if (strcmp(buf, uxpm->name) == 0) {
	    pds = read_compiled_pixmap(uxpm);
	    break;
	  }
      }
    }
  }

  if (!pds) {
    /* not a compiled-in pixmap, try to find the file */
    filename = tu_find_file(name, image_dirs);
    
    if (filename != NULL && !strchr(name, '.')) 
      /* try with added .xpm */
      filename = tu_find_file(buf, image_dirs);
    
    if (filename) {
      pds = tu_pds_read_xpm_file(filename);
      free(filename);
    }
  }
  
  if (pds) {
    XVisualInfo xvis, *xvi;
    Visual *v;
    int n;
    
    v = DefaultVisualOfScreen(screen);
    xvis.visualid = v->visualid;
    xvi = XGetVisualInfo(DisplayOfScreen(screen), VisualIDMask, &xvis, &n);
    
    tu_pds_allocate_colors(screen, DefaultColormapOfScreen(screen), 
			   xvi, pds, NULL);
    
    tu_pds_create_pixmap(screen, 1, xvi, pds);
    pixmap = pds->pds_pixmap;
    tu_pds_free_colors(screen, DefaultColormapOfScreen(screen), 
		       xvi, pds, NULL);
    XtFree((char*)xvi);
    tu_pds_free(pds);
    goto got_it;
  }
  return None;

 got_it:
  return pixmap;
}


/****************************************************************
 * CvtStringToBitmap:
 *    Converts a string to a bitmap using compiled-in paths
 *    and file system files, etc.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToBitmap(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display *display;
     XrmValue *args;
     Cardinal *num_args;
     XrmValue *fromVal;
     XrmValue *toVal;
     XtPointer *converter_data;
{
  Pixmap             pixmap;
  char             * name;
  Screen           * screen;

  if (*num_args != 1)
    XtErrorMsg("wrongParameters","CvtStringToBitmap","TuCvtError",
	       "String to bitmap conversion needs screen argument",
	       (String *)NULL, (Cardinal *)NULL);

  name   = (char *)fromVal->addr;
  screen = *((Screen **) args[0].addr);

  pixmap = tu_make_bitmap(name, screen, tu_pmdefs, tu_bmdefs);
  *converter_data = NULL;

  if (pixmap == None) {
    XtStringConversionWarning(name, "Bitmap");
    return False;
  }
  done(Pixmap, pixmap)
}


/****************************************************************
 * tu_cvt_to_bitmap:
 *    This function uses the standard converter to convert
 *    a string into a bitmap.
 ****************************************************************/
/*CExtract*/
Pixmap tu_cvt_to_bitmap(widget, name)
     Widget widget;
     char *name;
{
  XrmValue src;
  XrmValue dst;

  src.addr = (XtPointer) name;
  src.size = strlen(name)+1;
  dst.addr = "Error";

  XtConvert(widget, XtRString, &src, "Bitmap", &dst);
  if (dst.addr == NULL) return None;
  return *((Pixmap *) dst.addr);
}

/****************************************************************
 * tu_register_pixmaps:
 *   This functions will add a set of pixmaps from either a
 *   reusable GUI component or a main application to the
 *   'tu_pmdefs' structure.
 ****************************************************************/
#define PMINFO_DELIMITER " "
void tu_register_pixmaps (pixmaps, n_pm)
tu_pmdefs_p pixmaps;
int n_pm;
{
  int i;
  tu_pmdefs = (tu_pmdefs_p*)XtRealloc((char*)tu_pmdefs,
    (tu_pmcount + n_pm + 1) * sizeof(tu_pmdefs_p));
  for (i=0; i<n_pm; i++)
    tu_pmdefs[tu_pmcount++] = &(pixmaps[i]);
  tu_pmdefs[tu_pmcount] = NULL;
}

/****************************************************************
 * tu_make_pixmap:
 *   The function converts a string to a pixmap, i.e., a one-bit
 *   or more, deep pixmap. The pixmap read must be in the bitmap(1)
 *   format or the pixmap (XPM3) format. If bitmap is used, the 
 *   string specification format should be "string[(fg,bg[,depth])]".
 *   If depth is not specified, then the default depth of the screen 
 *   is used. 
 *   The search order to find the bitmap/pixmap data depends
 *   on the depth of the screen and the name specified. If the depth
 *   is 1, the function tries bitmaps first. Otherwise, it starts with
 *   pixmaps. If foreground (background, depth) is specified, only
 *   bitmaps are searched.
 *   The search pattern for depth 1 is described below:
 *   The function first tries to match a compiled-in bitmap 
 *   with the name specified, then a compiled-in bitmap with the name 
 *   <name>.xbm, then a file with the specified name, then a file 
 *   with name <name>.xbm. If none of these match, the procedure is 
 *   repeated for pixmap files, with extension .xbm replaced with .xpm.
 *   Extensions are only added if the name does not contain a dot.
 ****************************************************************/
Pixmap tu_make_pixmap(name, screen, cmap, pmdefs, bmdefs, r_pds)
     char * name;
     Screen * screen;
     Colormap cmap;
     tu_pmdefs_p* pmdefs;
     tu_bmdefs_p* bmdefs;
     tu_pixmap *r_pds;
{
  Pixmap                 pixmap;
  Pixmap                 bitmap;
  tu_pixmap              pds;
  char                 * filename;
  unsigned int           width;
  unsigned int           height;
  int                    xhot;
  int                    yhot;
  char                 * lpar;
  char                   pname[MAXLEN];
  char                   fg[MAXLEN];
  char                   bg[MAXLEN];
  Pixel                  fg_color;
  Pixel                  bg_color;
  char                   depth[MAXLEN];
  int                    depthflg;
  int                    depthval;
  int                    i;
  tu_bmdefs_p            uxbm;
  tu_pmdefs_p            uxpm;
  Widget                 widget;
  Arg                    arg[10];
  XrmValue               srcval;
  XrmValue               dstval;
  XGCValues              gcv;
  GC                     gc;
  Boolean                allowXPM;
  Boolean                XPMdone = False;
  Boolean                XBMdone = False;
  char                   buf[MAXLEN];

  /* no pds yet */
  if (r_pds) 
    *r_pds = NULL;

  /* string literal ? */
  if (pds = tu_pds_read_xpm_string(name))
    goto got_pixmap;

  if ((int)strlen(name) > (MAXLEN/2))
    return None;

  /*--------------------------  Parse name  ------------------------------*
   * Check for foreground, background & depth specifications. If there,
   * don't allow use of pixmap format.
   */
#define GOPAST(p, c) while (*(p) && *(p) != c) p++; p++
  if (strchr(name, '(')) {
    char  * p;
    int     i;
    
    allowXPM = False;
    p = name;
    SKIPSP(p);
    i = 0;
    while (*p && !isspace(*p) && *p != '(')
      pname[i++] = *p++;
    pname[i] = '\0';
    
    GOPAST(p, '(');
    if (!strchr(p, ','))
      return None;
    
    SKIPSP(p);
    i = 0;
    while (*p && !isspace(*p) && *p != ',')
      fg[i++] = *p++;
    fg[i] = '\0';
    
    GOPAST(p, ',');
    if (strchr(p, ',')) {
      SKIPSP(p);
      i = 0;
      while (*p && !isspace(*p) && *p != ',')
	bg[i++] = *p++;
      bg[i] = '\0';
      
      GOPAST(p, ',');
      if (!strchr(p, ')'))
	return None;
      
      SKIPSP(p);
      i = 0;
      while (*p && !isspace(*p) && *p != ')')
	depth[i++] = *p++;
      depth[i] = '\0';
      depthflg = True;
    } else {
      if (!strchr(p, ')'))
	return None;
      SKIPSP(p);
      i = 0;
      while (*p && !isspace(*p) && *p != ')')
	bg[i++] = *p++;
      bg[i] = '\0';
      depthflg = False;
    }
  } else {
    char  * p;
    int     i;

    p = name;
    SKIPSP(p);
    i = 0;
    while (*p && !isspace(*p)) {
      pname[i++] = *p++;
      if (i > (MAXLEN-2))
	break;
    }
    pname[i] = '\0';
    (void) strcpy(fg, XtDefaultForeground);
    (void) strcpy(bg, XtDefaultBackground);
    depthflg = False;
    allowXPM = True;
  }
#undef GOPAST

  /*-------------------------------------------------------------------*
   * See if we should start (use) pixmaps or bitmaps */
  if (!allowXPM) {
    XPMdone = True; /* don't do XPM */
    goto do_bitmap;
  }
  if (DefaultDepthOfScreen(screen) < MIN_COLOR) /* start with bitmaps */
    goto do_bitmap;

  /*--------------------------  Pixmaps  ------------------------------*/
 do_pixmap:
  pds = NULL;
  if (!XPMdone) {
    XPMdone = True;
    /* add .xpm */
    (void) sprintf(buf, "%s.xpm", name);

    if (pmdefs) {
      for (i=0; uxpm=pmdefs[i]; i++)
	if (strcmp(name, uxpm->name) == 0) {
	  pds = read_compiled_pixmap(uxpm);
	  goto got_pixmap;
	}
    
      for (i=0; uxpm=pmdefs[i]; i++)
	if (strcmp(buf, uxpm->name) == 0) {
	  pds = read_compiled_pixmap(uxpm);
	  goto got_pixmap;
	}
    }
    
    /* not a compiled-in pixmap, try to find the file */
    filename = tu_find_file(name, image_dirs);
    
    if (filename == NULL) 
      /* try with added .xpm */
      filename = tu_find_file(buf, image_dirs);
    
    if (filename) {
      pds = tu_pds_read_xpm_file(filename);
      free(filename);
    }
  }

 got_pixmap:

  if (pds) {
    XVisualInfo xvis, *xvi;
    Visual *v;
    int n;
    
    v = DefaultVisualOfScreen(screen);
    xvis.visualid = v->visualid;
    xvi = XGetVisualInfo(DisplayOfScreen(screen), VisualIDMask, &xvis, &n);
    tu_pds_allocate_colors(screen, cmap, xvi, pds, tu_global_color_scheme);
    tu_pds_create_pixmap(screen, DefaultDepthOfScreen(screen), xvi, pds);
    pixmap = pds->pds_pixmap;
    if (r_pds) 
      *r_pds = pds;
    XtFree((char*)xvi);
    goto got_it;
  }

  /*--------------------------  Bitmaps  ------------------------------*/
 do_bitmap:
  if (!XBMdone) {
    XBMdone = True;
    /* Add .xbm */
    (void) sprintf(buf, "%s.xbm", pname);
    
    if (bmdefs) {
      for (i=0; uxbm=bmdefs[i]; i++)
	if (strcmp(pname, uxbm->name) == 0) {
	  bitmap = 
	    XCreateBitmapFromData(DisplayOfScreen(screen),
				  RootWindowOfScreen(screen),
				  (char *)uxbm->bitmap,
				  uxbm->width,
				  uxbm->height);
	  width  = uxbm->width;
	  height = uxbm->height;
	  goto got_bitmap;
      }
      
      for (i=0; uxbm=bmdefs[i]; i++)
	if (strcmp(buf, uxbm->name) == 0) {
	  bitmap = 
	    XCreateBitmapFromData(DisplayOfScreen(screen),
				  RootWindowOfScreen(screen),
				  (char *)uxbm->bitmap,
				  uxbm->width,
				  uxbm->height);
	  width  = uxbm->width;
	  height = uxbm->height;
	  goto got_bitmap;
	}
    }
    
    /* not a compiled-in bitmap, try to find the file */
    filename = tu_find_file(pname, image_dirs);
    
    if (filename == NULL) /* add .xbm */
      filename = tu_find_file(buf, image_dirs);
    
    if (filename != NULL) {
      if (XReadBitmapFile(DisplayOfScreen(screen), RootWindowOfScreen(screen),
			  filename, &width, &height, &bitmap, &xhot, &yhot)
	  != BitmapSuccess) {
	free(filename);
	if (XPMdone) {
	  XtStringConversionWarning(name, "Pixmap");
	  return None;
	} else 
	  goto do_pixmap;
      }
      free(filename);
      goto got_bitmap;
    }
    goto do_pixmap;
  }

  /*----  No bitmap found, no pixmap found. Tough luck... -----*/
  return None;

  /*--------------------------  Color bitmap  ------------------------------*/
 got_bitmap:
  /* OK, we have a bitmap, let's color it */
  srcval.addr = (XtPointer) fg;
  srcval.size = strlen(fg);
    
  /* Get temporary widget */
  if (tu_global_top_widget &&
      XtScreen(tu_global_top_widget) == screen) {
    Arg garg[1];
    Colormap glcmap;

    XtSetArg(garg[0], XtNcolormap, &glcmap);
    XtGetValues(tu_global_top_widget, garg, 1);
    if (glcmap == cmap) 
      widget = tu_global_top_widget;
    else 
      widget = NULL;
  } else
    widget = NULL;

  if (widget == NULL) {
    XtSetArg(arg[0], XtNscreen, screen);
    XtSetArg(arg[1], XtNcolormap, cmap);
    widget = 
      XtAppCreateShell("telelus_temp_shell", "Telelus_temp_shell",
		       applicationShellWidgetClass, 
		       DisplayOfScreen(screen), arg, 2);
  }
  
  XtConvert(widget, XtRString, &srcval, XtRPixel, &dstval);
  if (dstval.addr == NULL) {
    XtStringConversionWarning(name, "Pixmap");
    if (widget != tu_global_top_widget)
      XtDestroyWidget(widget);
    return None;
  }
  fg_color = *((Pixel *)dstval.addr);
      
  srcval.addr = (XtPointer) bg;
  srcval.size = strlen(bg);
  XtConvert(widget, XtRString, &srcval, XtRPixel, &dstval);
  if (dstval.addr == NULL) {
    XtStringConversionWarning(name, "Pixmap");
    if (widget != tu_global_top_widget)
      XtDestroyWidget(widget);
    return None;
  }
  bg_color = *((Pixel *)dstval.addr);
  if (widget != tu_global_top_widget)
    XtDestroyWidget(widget);
  
  if (depthflg) {
    depthval = atoi(depth);
    if (depth == 0) {
      XtStringConversionWarning(name, "Pixmap");
      return None;
    }
  } else {
    depthval = DefaultDepthOfScreen(screen);
  }
  
  pixmap = XCreatePixmap(DisplayOfScreen(screen),
			 RootWindowOfScreen(screen),
			 width, height, depthval);
  
  gcv.foreground = fg_color;
  gcv.background = bg_color;
  gcv.fill_style = FillOpaqueStippled;
  gcv.stipple = bitmap;
  gc = XCreateGC(DisplayOfScreen(screen),
		 RootWindowOfScreen(screen),
		 GCForeground|GCBackground|GCFillStyle|GCStipple,
		 &gcv);
  XFillRectangle(DisplayOfScreen(screen),
		 pixmap, gc, 0, 0, width, height);
  XFreeGC(DisplayOfScreen(screen), gc);
  XFreePixmap(DisplayOfScreen(screen), bitmap);

  /*-------------------------------------------------------------------*/
 got_it:
  return pixmap;
}


/****************************************************************
 * CvtStringToPixmap:
 *    Converts a string to a pixmap using the built-in
 *    pixmaps and bitmaps (as well as the file system).
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToPixmap(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display   *display;
     XrmValue  *args;
     Cardinal  *num_args;
     XrmValue  *fromVal;
     XrmValue  *toVal;
     XtPointer *converter_data;
{
  static Pixmap pixmap;
  tu_pixmap pds;
  char * name;
  Screen * screen;
  Colormap cmap;

  if (*num_args != 2)
    XtErrorMsg("wrongParameters","CvtStringToPixmap","TuCvtError",
	       "String to pixmap conversion needs screen & colormap arguments",
	       (String *)NULL, (Cardinal *)NULL);
  
  name   = (char *)fromVal->addr;
  screen = *((Screen **) args[0].addr);
  cmap   = *((Colormap *) args[1].addr);

  pixmap = tu_make_pixmap(name, screen, cmap, tu_pmdefs, tu_bmdefs, &pds);
  *converter_data = (XtPointer) pds;

  if (pixmap == None) {
    XtStringConversionWarning(name, "Pixmap");
    return False;
  }
  done(Pixmap, pixmap)
}


/****************************************************************
 * DestroyPixmap:
 *    This function destroys the pixmap in the cache.
 ****************************************************************/
static void DestroyPixmap(app, to, converter_data, args, num_args)
     XtAppContext app;
     XrmValue    *to;
     XtPointer    converter_data;
     XrmValue    *args;
     Cardinal    *num_args;
{
  Screen *screen;
  Colormap cmap;
  tu_pixmap pds;
  XVisualInfo xvis, *xvi;
  Visual *v;
  int n;

  screen = *((Screen **) args[0].addr);
  cmap   = *((Colormap *) args[1].addr);

  pds = (tu_pixmap)converter_data;
  if (pds == NULL) {
    XFreePixmap(DisplayOfScreen(screen), *(Pixmap *)to->addr);
    return;
  }

  if (*num_args != 2)
    XtAppErrorMsg(app, "wrongParameters","DestroyPixmap", "TuError",
		  "Freeing a pixmap requires screen and colormap arguments",
		  (String *)NULL, (Cardinal *)NULL);
  
  v = DefaultVisualOfScreen(screen);
  xvis.visualid = v->visualid;
  xvi = XGetVisualInfo(DisplayOfScreen(screen), VisualIDMask, &xvis, &n);

  tu_pds_free_pixmap(screen, pds);
  tu_pds_free_colors(screen, cmap, xvi, pds, tu_global_color_scheme);
  XtFree((char*)xvi);
  tu_pds_free(pds);
}

     
/****************************************************************
 * DestroyBitmap:
 *    This function destroys the pixmap in the cache.
 ****************************************************************/
/* ARGSUSED */
static void DestroyBitmap(app, to, converter_data, args, num_args)
     XtAppContext app;
     XrmValue    *to;
     XtPointer    converter_data;
     XrmValue    *args;
     Cardinal    *num_args;
{
  Screen *screen;

  if (*num_args != 1)
    XtAppErrorMsg(app, "wrongParameters","DestroyBitmap", "TuError",
		  "Freeing a bitmap requires screen argument",
		  (String *)NULL, (Cardinal *)NULL);
  
  screen = *((Screen **) args[0].addr);

  XFreePixmap(DisplayOfScreen(screen), *(Pixmap *)to->addr);
}
     

/****************************************************************
 * tu_cvt_to_pixmap:
 *    This function uses the standard converter to convert
 *    a string into a pixmap.
 ****************************************************************/
/*CExtract*/
Pixmap tu_cvt_to_pixmap(widget, name)
     Widget widget;
     char *name;
{
  XrmValue src;
  XrmValue dst;

  src.addr = (XtPointer) name;
  src.size = strlen(name)+1;
  dst.addr = "Error";

  XtConvert(widget, XtRString, &src, XtRPixmap, &dst);
  if (dst.addr == NULL) return None;
  return *((Pixmap *) dst.addr);
}


/****************************************************************
 * CompareISOLatin1:
 ****************************************************************/
static int CompareISOLatin1 (first, second)
    char *first, *second;
{
    register unsigned char *ap, *bp;

    for (ap = (unsigned char *) first, bp = (unsigned char *) second;
	 *ap && *bp; ap++, bp++) {
	register unsigned char a, b;

	if ((a = *ap) != (b = *bp)) {
	    /* try lowercasing and try again */

	    if ((a >= XK_A) && (a <= XK_Z))
	      a += (XK_a - XK_A);
	    else if ((a >= XK_Agrave) && (a <= XK_Odiaeresis))
	      a += (XK_agrave - XK_Agrave);
	    else if ((a >= XK_Ooblique) && (a <= XK_Thorn))
	      a += (XK_oslash - XK_Ooblique);

	    if ((b >= XK_A) && (b <= XK_Z))
	      b += (XK_a - XK_A);
	    else if ((b >= XK_Agrave) && (b <= XK_Odiaeresis))
	      b += (XK_agrave - XK_Agrave);
	    else if ((b >= XK_Ooblique) && (b <= XK_Thorn))
	      b += (XK_oslash - XK_Ooblique);

	    if (a != b) break;
	}
    }
    return (((int) *bp) - ((int) *ap));
}

#if RV_SUPPORT
#include <X11/InitialI.h>
#endif

#define PXLMSG \
  "String to pixel conversion needs screen and colormap arguments"


/****************************************************************
 * CvtStringToPixel:
 *    This function does a conversion of a color[,background/foreground]
 *    color specification.
 ****************************************************************/
static Boolean CvtStringToPixel(dpy, args, num_args, fromVal, 
				toVal, closure_ret)
     Display*	dpy;
     XrmValuePtr args;
     Cardinal    *num_args;
     XrmValuePtr	fromVal;
     XrmValuePtr	toVal;
     XtPointer	*closure_ret;
{
  String            value = (String)fromVal->addr;
  String            mono = NULL;
  char  	    str[200];
  XColor	    screenColor;
  XColor	    exactColor;
  Screen	    *screen;
  Colormap	    colormap;
  Status	    status;
  String            params[1];
  Cardinal	    num_params=1;
#if RV_SUPPORT
  XtPerDisplay      pd = _XtGetPerDisplay(dpy);
#endif
 
  if (*num_args != 2)
    XtAppErrorMsg(XtDisplayToApplicationContext(dpy),
		  "wrongParameters", "cvtStringToPixel",
		  "TuCvtError",
		  PXLMSG,
		  (String *)NULL, (Cardinal *)NULL);
  
  screen = *((Screen **) args[0].addr);
  colormap = *((Colormap *) args[1].addr);

  SKIPSP(value);
  if (mono = strchr(value, ',')) {
    int    len;
    len = mono - value;
    mono++;
    SKIPSP(mono);
    if (DefaultDepthOfScreen(screen) < MIN_COLOR) {
      if (CompareISOLatin1(mono, "background") == 0)
	strcpy(str, XtDefaultBackground);
      else if (CompareISOLatin1(mono, "foreground") == 0)
	strcpy(str, XtDefaultForeground);
      else {
	params[0] = mono;
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
			"badFormat", "cvtStringToPixel",
			"TuCvtError",
			"Monochrome specification illegal \"%s\"",
			params, &num_params);
	*closure_ret = False;
	return False;
      }
    } else {
      strncpy(str, value, len);
      str[len] = '\0';
    }
  } else
    strcpy(str, value);

  if (CompareISOLatin1(str, XtDefaultBackground) == 0) {
    *closure_ret = False;
#if RV_SUPPORT
    if (pd->rv) done(Pixel, BlackPixelOfScreen(screen))
    else	    done(Pixel, WhitePixelOfScreen(screen))
#else
    done(Pixel, WhitePixelOfScreen(screen))
#endif
  }
  if (CompareISOLatin1(str, XtDefaultForeground) == 0) {
    *closure_ret = False;
#if RV_SUPPORT
    if (pd->rv) done(Pixel, WhitePixelOfScreen(screen))
    else	    done(Pixel, BlackPixelOfScreen(screen))
#else
    done(Pixel, BlackPixelOfScreen(screen))
#endif
  }
  
  status = XParseColor(DisplayOfScreen(screen), colormap,
		       (char*)str, &screenColor);
  
  if (status == 0) {
    params[0] = str;
    if (*str == '#')
      XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		      "badFormat", "cvtStringToPixel",
		      "TuCvtError",
		      "RGB color specification \"%s\" has invalid format",
		      params, &num_params);
    else
      XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		      "badValue", "cvtStringToPixel",
		      "TuCvtError", 
		      "Color name \"%s\" is not defined in server database",
		      params, &num_params);

    *closure_ret = False;
    return False;
  } else
     if (!tu_allocate_color(screen, colormap, &screenColor,
			    tu_global_color_scheme)) {
       params[0] = str;
       XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		       "noColormap", "cvtStringToPixel",
		       "TuCvtError", 
		       "Cannot allocate colormap entry for \"%s\"",
		       params, &num_params);
       *closure_ret = False;
       return False;
     }       

  *closure_ret = (char*)True;
  done(Pixel, screenColor.pixel)
}


/****************************************************************
 * FreePixel:
 *     This function frees a pixel allocated by the converter.
 ****************************************************************/
/* ARGSUSED */
static void FreePixel(app, toVal, closure, args, num_args)
     XtAppContext       app;
     XrmValuePtr	toVal;
     XtPointer	        closure;
     XrmValuePtr	args;
     Cardinal	      * num_args;
{
  Screen	  * screen;
  Colormap	    colormap;
  
  if (*num_args != 2)
    XtAppErrorMsg(app, "wrongParameters","freePixel", "TuError",
		  "Freeing a pixel requires screen and colormap arguments",
		  (String *)NULL, (Cardinal *)NULL);
  
  screen = *((Screen **) args[0].addr);
  colormap = *((Colormap *) args[1].addr);
  
  if (closure) 
    tu_free_color(screen, colormap, *(unsigned long*)toVal->addr,
		  tu_global_color_scheme);
}


/****************************************************************
 * CvtStringToButton:
 *     This function converts a string to a button code. The
 *     strings that are accepted are AnyButton and Button1-5.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToButton(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display   *display;
     XrmValue  *args;
     Cardinal  *num_args;
     XrmValue  *fromVal;
     XrmValue  *toVal;
     XtPointer *converter_data;
{
  char                    * name;
  name_convert_tbl_t      * ctbl;
  static unsigned int       button;
  static name_convert_tbl_t buttons[] = {
    { "AnyButton", (XtPointer)AnyButton },
    { "Button1", (XtPointer) Button1 },
    { "Button2", (XtPointer) Button2 },
    { "Button3", (XtPointer) Button3 },
    { "Button4", (XtPointer) Button4 },
    { "Button5", (XtPointer) Button5 },
    { NULL, NULL },
  };

  *converter_data = NULL;
  name = (char *)fromVal->addr;
  
  for (ctbl = buttons; ctbl->name; ctbl++)
    if (strcmp(ctbl->name, name)==0) {
      button = (unsigned int) ctbl->value;
      done(unsigned int, button)
    }
  return False;
}


/****************************************************************
 * CvtStringToFloat:
 *   This function converts a string to a floating point number.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToFloat(display, args, num_args,
			     fromVal, toVal, converter_data)
     Display   *display;
     XrmValue  *args;
     Cardinal  *num_args;
     XrmValue  *fromVal;
     XrmValue  *toVal;
     XtPointer *converter_data;
{
  static float   flt;
  char         * str;

  *converter_data = NULL;
  str = (char *) fromVal->addr;

  (void) sscanf(str, "%f", &flt);
  done(float, flt)
}


/****************************************************************
 *  CvtStringToDouble:
 *   This function converts a string to a floating point
 *   double precision number.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToDouble(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display   *display;
     XrmValue  *args;
     Cardinal  *num_args;
     XrmValue  *fromVal;
     XrmValue  *toVal;
     XtPointer *converter_data;

{
  static double  dbl;
  char         * str;

  *converter_data = NULL;
  str = (char *) fromVal->addr;
  
  (void) sscanf(str, "%lf", &dbl);
  done(double, dbl)
}


#ifndef BITMAPDIR
#define BITMAPDIR "/usr/include/X11/bitmaps"
#endif

#define FONTSPECIFIER		"FONT "

/****************************************************************
 * CvtStringToCursor:
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToCursor(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display   *display;
     XrmValue  *args;
     Cardinal  *num_args;
     XrmValue  *fromVal;
     XrmValue  *toVal;
     XtPointer *converter_data;
{
  static Cursor   cursor;
  char          * name = (char *)fromVal->addr;
  Screen        * screen;
  register int    i;
  char            filename[MAXPATHLEN];
  char            maskname[MAXPATHLEN];
  Pixmap          source;
  Pixmap          mask;
  static XColor   bgColor = {0, 0xffff, 0xffff, 0xffff};
  static XColor   fgColor = {0, 0, 0, 0};
  unsigned int    width;
  unsigned int    height;
  int             xhot;
  int             yhot;
  char          * chp;
  int             idummy;
  Widget          widget;

  static struct _CursorName {
    char	      * name;
    unsigned int	shape;
  } cursor_names[] = {
    {"X_cursor",		XC_X_cursor		},
    {"arrow",		        XC_arrow		},
    {"based_arrow_down",	XC_based_arrow_down     },
    {"based_arrow_up",	        XC_based_arrow_up       },
    {"boat",		        XC_boat		        },
    {"bogosity",		XC_bogosity		},
    {"bottom_left_corner",	XC_bottom_left_corner   },
    {"bottom_right_corner",	XC_bottom_right_corner  },
    {"bottom_side",		XC_bottom_side		},
    {"bottom_tee",		XC_bottom_tee		},
    {"box_spiral",		XC_box_spiral		},
    {"center_ptr",		XC_center_ptr		},
    {"circle",		        XC_circle		},
    {"clock",		        XC_clock		},
    {"coffee_mug",		XC_coffee_mug		},
    {"cross",		        XC_cross		},
    {"cross_reverse",	        XC_cross_reverse        },
    {"crosshair",		XC_crosshair		},
    {"diamond_cross",	        XC_diamond_cross        },
    {"dot",			XC_dot			},
    {"dotbox",		        XC_dotbox		},
    {"double_arrow",	        XC_double_arrow	        },
    {"draft_large",		XC_draft_large		},
    {"draft_small",		XC_draft_small		},
    {"draped_box",		XC_draped_box		},
    {"exchange",		XC_exchange		},
    {"fleur",		        XC_fleur		},
    {"gobbler",		        XC_gobbler		},
    {"gumby",		        XC_gumby		},
    {"hand1",		        XC_hand1		},
    {"hand2",		        XC_hand2		},
    {"heart",		        XC_heart		},
    {"icon",		        XC_icon		        },
    {"iron_cross",		XC_iron_cross		},
    {"left_ptr",		XC_left_ptr		},
    {"left_side",		XC_left_side		},
    {"left_tee",		XC_left_tee		},
    {"leftbutton",		XC_leftbutton		},
    {"ll_angle",		XC_ll_angle		},
    {"lr_angle",		XC_lr_angle		},
    {"man",			XC_man			},
    {"middlebutton",	        XC_middlebutton	        },
    {"mouse",		        XC_mouse		},
    {"pencil",		        XC_pencil		},
    {"pirate",		        XC_pirate		},
    {"plus",		        XC_plus		        },
    {"question_arrow",	        XC_question_arrow	},
    {"right_ptr",		XC_right_ptr		},
    {"right_side",		XC_right_side		},
    {"right_tee",		XC_right_tee		},
    {"rightbutton",		XC_rightbutton		},
    {"rtl_logo",		XC_rtl_logo		},
    {"sailboat",		XC_sailboat		},
    {"sb_down_arrow",	        XC_sb_down_arrow        },
    {"sb_h_double_arrow",	XC_sb_h_double_arrow    },
    {"sb_left_arrow",	        XC_sb_left_arrow        },
    {"sb_right_arrow",	        XC_sb_right_arrow       },
    {"sb_up_arrow",		XC_sb_up_arrow		},
    {"sb_v_double_arrow",	XC_sb_v_double_arrow    },
    {"shuttle",		        XC_shuttle		},
    {"sizing",		        XC_sizing		},
    {"spider",		        XC_spider		},
    {"spraycan",		XC_spraycan		},
    {"star",		        XC_star		        },
    {"target",		        XC_target		},
    {"tcross",		        XC_tcross		},
    {"top_left_arrow",	        XC_top_left_arrow       },
    {"top_left_corner",   	XC_top_left_corner	},
    {"top_right_corner",	XC_top_right_corner     },
    {"top_side",		XC_top_side		},
    {"top_tee",		        XC_top_tee		},
    {"trek",		        XC_trek		        },
    {"ul_angle",		XC_ul_angle		},
    {"umbrella",		XC_umbrella		},
    {"ur_angle",		XC_ur_angle		},
    {"watch",		        XC_watch		},
    {"xterm",		        XC_xterm		},
  };

  struct _CursorName *cache;

  *converter_data = NULL;
  if (*num_args != 2)
    XtErrorMsg("wrongParameters","CvtStringToCursor","TuCvtError",
	       "String to cursor conversion needs screen & widget argument",
	       (String *)NULL, (Cardinal *)NULL);

  screen = *((Screen **) args[0].addr);
  widget = *((Widget *) args[1].addr);
  
  if (0 == strncmp(FONTSPECIFIER, name, strlen(FONTSPECIFIER))) {
    char source_name[MAXPATHLEN], mask_name[MAXPATHLEN];
    int source_char, mask_char, fields;
    Font source_font, mask_font;
    XrmValue fromString, toFont;
    
    fields = sscanf(name, "FONT %s %d %s %d",
		    source_name, &source_char,
		    mask_name, &mask_char);
    if (fields < 2) {
      XtStringConversionWarning( name, "Cursor" );
      return False;
    }
    
    fromString.addr = source_name;
    fromString.size = strlen(source_name);
    XtConvert(widget, XtRString, &fromString, XtRFont, &toFont);
    if (toFont.addr == NULL) {
      XtStringConversionWarning( name, "Cursor" );
      return False;
    }
    source_font = *(Font*)toFont.addr;
    
    switch (fields) {
    case 2:		/* defaulted mask font & char */
      mask_font = source_font;
      mask_char = source_char;
      break;
      
    case 3:		/* defaulted mask font */
      mask_font = source_font;
      mask_char = atoi(mask_name);
      break;
      
    case 4:		/* specified mask font & char */
      fromString.addr = mask_name;
      fromString.size = strlen(mask_name);
      XtConvert(widget, XtRString, &fromString, XtRFont, &toFont);
      if (toFont.addr == NULL) {
	XtStringConversionWarning( name, "Cursor" );
	return False;
      }
      mask_font = *(Font*)toFont.addr;
    }
    
    cursor = XCreateGlyphCursor( DisplayOfScreen(screen), source_font,
				 mask_font, source_char, mask_char,
				 &fgColor, &bgColor );
    done(Cursor, cursor)
  }
  
  for (i=0, cache = cursor_names; i < XtNumber(cursor_names); i++, cache++ ) {
    if (strcmp(name, cache->name) == 0) {
	cursor = XCreateFontCursor(DisplayOfScreen(screen), cache->shape );
      done(Cursor, cursor)
    }
  }
  
  /* isn't a standard cursor in cursorfont; try to open a bitmap file */
  chp = tu_find_file(name, image_dirs);
  if (chp) {
    strcpy(filename, chp);
    free(chp);
  } else {
    strcpy(filename, name);
  }
  
  if (XReadBitmapFile(DisplayOfScreen(screen), RootWindowOfScreen(screen),
		      filename, &width, &height, &source, &xhot, &yhot)
      != BitmapSuccess) {
    XtStringConversionWarning(name, "Cursor");
    return False;
  }

  (void) strcpy( maskname, filename );
  (void) strcat( maskname, "Mask" );
  if (XReadBitmapFile(DisplayOfScreen(screen), RootWindowOfScreen(screen),
		      maskname, &width, &height, &mask, &idummy, &idummy)
      != BitmapSuccess) {
    (void) strcpy(maskname, filename);
    (void) strcat(maskname, "msk");
    if (XReadBitmapFile(DisplayOfScreen(screen),RootWindowOfScreen(screen),
			maskname, &width, &height, &mask, &idummy, &idummy)
	!= BitmapSuccess) {
      mask = None;
    }
  }
  cursor = XCreatePixmapCursor( DisplayOfScreen(screen), source, mask,
				&fgColor, &bgColor, xhot, yhot );
  XFreePixmap( DisplayOfScreen(screen), source );
  if (mask != None) XFreePixmap( DisplayOfScreen(screen), mask );
  done(Cursor, cursor)
}


/****************************************************************
 * CvtWidgetToString:
 *    This function takes a widget and returns the name of 
 *    that widget.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtWidgetToString(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display   * display;
     XrmValue  * args;
     Cardinal  * num_args;
     XrmValue  * fromVal;
     XrmValue  * toVal;
     XtPointer * converter_data;
{
  Widget   widget;
  char   * name;

  *converter_data = NULL;
  if (*num_args != 0)
    XtErrorMsg("wrongParameters", "CvtWidgetToString", "TuCvtError",
	       "WidgetToString conversion needs no arg", NULL, 0);

  widget = (Widget)fromVal->addr;
  if (widget == NULL)
    name = NULL;
  else
    name = tu_widget_name(widget);
  done(char *, name)
}


/****************************************************************
 * CvtStringToWidgetChild:
 *    This function takes a widget and the name of a child 
 *    to be set.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToWidgetChild(display, args, num_args,
				      fromVal, toVal, converter_data)
     Display   * display;
     XrmValue  * args;
     Cardinal  * num_args;
     XrmValue  * fromVal;
     XrmValue  * toVal;
     XtPointer * converter_data;
{
  Widget   widget;
  Widget   child;
  char   * name;
  String   params[1];
  Cardinal num_params = 1;

  *converter_data = NULL;
  if (*num_args != 1)
    XtErrorMsg("wrongParameters", "CvtStringToWidgetChild", "TuCvtError",
	       "StringToWidget conversion needs widget arg", NULL, 0);
  
  name   = (char *)fromVal->addr;
  widget = *(Widget*)args[0].addr;
  child  = tu_find_widget(widget, name, TU_FW_ANY, 0);

  if (child == NULL) {
    params[0] = name;
    XtAppWarningMsg(XtDisplayToApplicationContext(display),
		    "CvtStringToWidgetChild", "child not found",
		    "TuCvtError", "Child not found: \"%s\"", 
		    params, &num_params);
    return False;
  }
  done(Widget, child)
}


/****************************************************************
 * find_scrolled_window:
 *  If we have XmScrolledList or XmScrolledText children,
 *  the XmText/XmList child is a child of an XmScrolledWindow. 
 *  The name of this XmScrolledWindow is the same as the XmText/XmList
 *  widget except that it ends with "SW". So, we try to find a direct
 *  child with name <name>SW that has a child with name <name>. The 
 *  XmScrolledWindow is returned.
 *
 ****************************************************************/
static Widget find_scrolled_window(widget, name)
     Widget widget;
     String name;
{
  Widget sw_child;
  Widget child;
  char   sw_name[256];

  (void) strcpy(sw_name, name);
  (void) strcat(sw_name, "SW");

  child = NULL;
    
  sw_child = tu_find_widget(widget, sw_name, TU_FW_NORMAL, 2);
  if (sw_child) {
    /*  Found the XmScrolledWindow */
    child = tu_find_widget(sw_child, name, TU_FW_NORMAL, 2);
    if (child) {
      child = sw_child;
    }
  }

  return (child);
}
  

/****************************************************************
 * CvtStringToDirectWidgetChild:
 *    This function takes a widget and the name of a direct
 *    widget child whose widget pointer should be returned.
 *    Special cases for XmScrolledList or XmScrolledText named
 *    children, and XmMainWindow or XmScrolledWindow as parents.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToDirectWidgetChild(display, args, num_args,
					    fromVal, toVal, converter_data)
     Display   * display;
     XrmValue  * args;
     Cardinal  * num_args;
     XrmValue  * fromVal;
     XrmValue  * toVal;
     XtPointer * converter_data;
{
  Widget     parent;
  Widget     child;
  char     * name;
  String     params[1];
  Cardinal   num_params = 1;

  *converter_data = NULL;
  if (*num_args != 1)
    XtErrorMsg("wrongParameters", "CvtStringToDirectWidgetChild", 
	       "TuCvtError", "StringToWidget conversion needs widget arg",
	       NULL, 0);
  
  name   = (char *)fromVal->addr;
  parent = *(Widget*)args[0].addr;
  child  = tu_find_widget(parent, name, TU_FW_NORMAL, 2);

  if (child == NULL) {
    /* Looking for XmScrolledList or XmScrolledText children ?
     */
    child = find_scrolled_window(parent, name);

    if (child == NULL) {
      /* XmMainWindow or XmScrolledWindow parent ? 
       *
       * If so, "direct" widget children are placed under an 
       * XmDrawingArea widget which is named "ScrolledWindowClipWindow".
       * Look for the named children in this widget tree.
       */
      Widget da_widget;
      
      da_widget = tu_find_widget(parent, "ScrolledWindowClipWindow",
				 TU_FW_NORMAL, 2);
      if (da_widget) {
	child = tu_find_widget(da_widget, name, TU_FW_NORMAL, 2);
	if (child == NULL) {
	  /* Looking for XmScrolledList or XmScrolledText children ?
	   */
	  child = find_scrolled_window(da_widget, name);
	}
      }
    }
    
    if (child == NULL) {
      params[0] = name;
      XtAppWarningMsg(XtDisplayToApplicationContext(display),
		      "CvtStringToDirectWidgetChild", "child not found",
		      "tuCvtError", "Child not found: \"%s\"", 
		      params, &num_params);
      return False;
    }
  }
  done(Widget, child)
}


/****************************************************************
 * CvtStringToSiblingWidget:
 *    This function takes a widget and the name of a sibling 
 *    widget whose widget pointer should be returned.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToSiblingWidget(display, args, num_args,
					fromVal, toVal, converter_data)
     Display   * display;
     XrmValue  * args;
     Cardinal  * num_args;
     XrmValue  * fromVal;
     XrmValue  * toVal;
     XtPointer * converter_data;
{
  Widget   widget;
  Widget   child;
  char   * name;
  String   params[1];
  Cardinal num_params = 1;

  *converter_data = NULL;
  if (*num_args != 1)
    XtErrorMsg("wrongParameters", "CvtStringToSiblingWidget", "TuCvtError",
	       "StringToWidget conversion needs widget arg", NULL, 0);

  name   = (char *)fromVal->addr;
  widget = *(Widget*)args[0].addr;
  widget = XtParent(widget);

  if (widget != NULL) {
    child = tu_find_sibling(widget, name);

    if (child == NULL) {
      /* Looking for XmScrolledText or XmScrolledList sibling ?
       * If so, return the scrolled window.
       */
      Widget sw_child;
      char   sw_name[256];
      
      (void) strcpy(sw_name, name);
      (void) strcat(sw_name, "SW");
      
      sw_child = tu_find_sibling(widget, sw_name);
      if (sw_child) {
	child = tu_find_widget(sw_child, name, TU_FW_NORMAL, 2);
	if (child) 
	  child = sw_child;
      }
    }
  }
  else
    child = NULL;

  if (child == NULL) {
    params[0] = name;
    XtAppWarningMsg(XtDisplayToApplicationContext(display),
		    "CvtStringToSiblingWidget", "child not found",
		    "tuCvtError", "Child not found: \"%s\"", 
		    params, &num_params);
    return False;
  }
  done(Widget, child)
}


/****************************************************************
 * CvtStringToSubMenuWidget:
 *    This function takes a widget and the name of a subMenu 
 *    to be used.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToSubMenuWidget(display, args, num_args,
					fromVal, toVal, converter_data)
     Display   * display;
     XrmValue  * args;
     Cardinal  * num_args;
     XrmValue  * fromVal;
     XrmValue  * toVal;
     XtPointer * converter_data;
{
  Widget   widget;
  Widget   child;
  char   * name;
  String   params[1];
  Cardinal num_params = 1;

  *converter_data = NULL;
  if (*num_args != 1)
    XtErrorMsg("wrongParameters", "CvtStringToSubMenuWidget", "TuCvtError",
	       "StringToWidget conversion needs widget arg", NULL, 0);

  name   = (char *)fromVal->addr;
  widget = *(Widget*)args[0].addr;

  child  = tu_find_submenu_widget(widget, name);

  if (child == NULL) {
    params[0] = name;
    XtAppWarningMsg(XtDisplayToApplicationContext(display),
		    "CvtStringToSubMenuWidget", "child not found",
		    "tuCvtError", "Child not found: \"%s\"", 
		    params, &num_params);
    return False;
  }
  done(Widget, child)
}


/****************************************************************
 * CvtStringToSubMenuItemWidget:
 *    This function takes a widget and the name of an item in
 *    its subMenu to be used.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToSubMenuItemWidget(display, args, num_args,
					    fromVal, toVal, converter_data)
     Display   * display;
     XrmValue  * args;
     Cardinal  * num_args;
     XrmValue  * fromVal;
     XrmValue  * toVal;
     XtPointer * converter_data;
{
  Widget   widget;
  Widget   child;
  char   * name;
  String   params[1];
  Cardinal num_params = 1;

  *converter_data = NULL;
  if (*num_args != 1)
    XtErrorMsg("wrongParameters", "CvtStringToSubMenuItemWidget", 
	       "TuCvtError", "StringToWidget conversion needs widget arg",
	       NULL, 0);

  name   = (char *)fromVal->addr;
  widget = *(Widget*)args[0].addr;

  child  = tu_find_submenu_item_widget(widget, name);

  if (child == NULL) {
    params[0] = name;
    XtAppWarningMsg(XtDisplayToApplicationContext(display),
		    "CvtStringToSubMenuItemWidget", "child not found",
		    "tuCvtError", "Child not found: \"%s\"", 
		    params, &num_params);
    return False;
  }
  done(Widget, child)
}


/****************************************************************
 * CvtStringToWidget:
 *    This function takes a widget and the name of a widget to
 *    be found.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToWidget(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display   * display;
     XrmValue  * args;
     Cardinal  * num_args;
     XrmValue  * fromVal;
     XrmValue  * toVal;
     XtPointer * converter_data;
{
  Widget   widget;
  Widget   child;
  char   * name;
  String   params[1];
  Cardinal num_params = 1;

  *converter_data = NULL;
  if (*num_args != 1)
    XtErrorMsg("wrongParameters", "CvtStringToWidget", "TuCvtError",
	       "StringToWidget conversion needs widget arg", NULL, 0);

  name   = (char *)fromVal->addr;
  widget = *(Widget*)args[0].addr;

  child = NULL;
  if (widget != NULL)
    child = tu_find_widget(widget, name, TU_FW_ANY, 0);

  if (child == NULL) {
    widget = tu_widget_top_widget(widget);
    child  = tu_find_widget(widget, name, TU_FW_ANY, 0);
  }

  if (child == NULL) {
    widget = tu_widget_root_widget(widget);
    child  = tu_find_widget(widget, name, TU_FW_ANY, 0);
  }

  if (child == NULL) {
    params[0] = name;
    XtAppWarningMsg(XtDisplayToApplicationContext(display),
		    "CvtStringToWidget", "child not found",
		    "tuCvtError", "Child not found: \"%s\"", 
		    params, &num_params);
    return False;
  }
  done(Widget, child)
}


/****************************************************************
 * CvtStringToScreen:
 *    This function takes a string (an integer) and returns
 *    the corresponding screen.
 ****************************************************************/
/* ARGSUSED */
static Boolean CvtStringToScreen(display, args, num_args,
				 fromVal, toVal, converter_data)
     Display *display;
     XrmValue *args;
     Cardinal *num_args;
     XrmValue *fromVal;
     XrmValue *toVal;
     XtPointer *converter_data;
{
  Screen        * screen;
  char          * sno;
  char          * p;
  int             scr_no;

  *converter_data = NULL;
  if (*num_args != 0) {
    XtErrorMsg("wrongParameters", "cvtStringToScreen", "TuCvtError",
	       "String to screen conversion needs no args", NULL, 0);
    return False;
  }

  sno = (char *)fromVal->addr;
  scr_no = strtol(sno, &p, 0);
  
  if (p != NULL && p >= sno && p < (sno+strlen(sno))) {
    XtErrorMsg("wrongParameters", "cvtStringToScreen", "TuCvtError",
	       "Integer screen string does not contain integer", NULL, 0);
    return False;
  }

  if (scr_no >= ScreenCount(display)) {
    XtErrorMsg("wrongParameters", "cvtStringToScreen", "TuCvtError",
	       "Screen number not defined for display", NULL, 0);
    return False;
  }

  screen = ScreenOfDisplay(display, scr_no);
  done(Screen *, screen)
}


/****************************************************************
 * getnum:
 *    Extracts a number out of a string.
 ****************************************************************/
static int getnum(get_comma, p, f)
     Boolean get_comma;
     char **p;
     int *f;
{
  char tmp[20];
  char *st;
  char *s = *p;
  int size;

  if (get_comma) {
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    if (*s != ',') {
      *f = False;
      return -1;
    }
    s++;
  }

  while (*s == ' ' || *s == '\t' || *s == '\n') s++;
  st = s;
  if (*s == '-') s++;
  while (isalnum(*s)) s++;
  size = s-st;
  if (size > 19)
    size = 19;
  strncpy(tmp, st, size);
  tmp[size] = '\0';
  *p = s;
  return strtol(tmp, &s, 0);
}


/****************************************************************
 * CvtStringToXRectList:
 *   The function converts a string to a null-terminated list
 *   of X rectangles.
 ****************************************************************/
/*ARGSUSED*/
static Boolean CvtStringToXRectList(display, args, num_args, 
				    fromVal, toVal, converter_data)
     Display             *display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer           *converter_data;
{
  XRectangle        ** xrl;
  XRectangle        *  dp;
  XRectangle           rect;
  char              *  str;
  int                  pos;
  int                  f;
  String               params[1];
  Cardinal             num_params = 1;
  int                  loop;

  *converter_data = NULL;

  dp  = NULL;
  xrl = NULL;
  f   = True;

  for (loop = 0; loop<2; loop++) {

    pos = 0;
    str = (char *) fromVal->addr;
    while (*str == ' ' || *str == '\t' || *str == '\n') str++;
    
    while (*str == '(') {
      str++;
      rect.x = getnum(False, &str, &f);
      rect.y = getnum(True, &str, &f);
      rect.width = getnum(True, &str, &f);
      rect.height = getnum(True, &str, &f);
      while (*str == ' ' || *str == '\t' || *str == '\n') str++;
      if (*str != ')' || !f)
	goto fault;
      str++;
      if (loop > 0) {
	xrl[pos] = dp++;
	*xrl[pos] = rect;
	pos++;
      } else
	pos++;
    }

    if (loop == 0) {
      xrl = (XRectangle **) XtMalloc(sizeof(XRectangle *) * (pos+1));
      dp = (XRectangle *) XtMalloc(sizeof(XRectangle) * pos);
    } else {
      xrl[pos] = NULL;
    }

  }

  /* all done */
  done(XRectangle **, xrl)

 fault:
  params[0] = (char *) fromVal->addr;;
  XtAppWarningMsg(XtDisplayToApplicationContext(display),
		  "CvtStringToXRectList", "syntax error",
		  "tuCvtError", "XRectangle List syntax error: \"%s\"", 
		  params, &num_params);
  return False;
}


/****************************************************************
 * CvtStringToIntTable:
 *   This function converts a string to a null-terminated list
 *   of integers.
 ****************************************************************/
/*ARGSUSED*/
static Boolean CvtStringToIntTable(display, args, num_args, 
				   fromVal, toVal, converter_data)
     Display             *display;
     XrmValuePtr          args;
     Cardinal            *num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer           *converter_data;
{
  int	            *  int_list = NULL;
  char              *  str;
  int                  f = True;
  int		       i = 0;
  int 		       size = 0;
  char               * last_s;

  *converter_data = NULL;
  str = (char *) fromVal->addr;
  if (str == NULL) return False;
  last_s = NULL;

  while (*str != '\0') {
    if (i >= size) {
      size += 10;
      int_list = (int *)XtRealloc((char *)int_list, sizeof(int)* size);
    }
    int_list[i++] = getnum(i > 0, &str, &f);
    if (last_s == str) {
      char *params[1];
      Cardinal num_params = 1;
      params[0] = (char *) fromVal->addr;
      XtAppWarningMsg(XtDisplayToApplicationContext(display),
		      "CvtStringToIntTable", "syntax error",
		      "tuCvtError", "Integer table syntax error: \"%s\"", 
		      params, &num_params);
      return False;
    } else
      last_s = str;
  }
  
  /* all done */
  done(int *, int_list)
}


/****************************************************************
 * CvtFreeIntTable:
 *    Frees up the memory used by an IntTable.
 ****************************************************************/
/*ARGSUSED*/
static void CvtFreeIntTable(app, to, data, args, num_args)
     XtAppContext  app;
     XrmValue    * to;
     XtPointer     data;
     XrmValue    * args;
     Cardinal    * num_args;
{
  int *int_list;

  int_list = *((int **) to->addr);
  if (int_list) 
    XtFree((char *)int_list);
}


/****************************************************************
 * CvtStringToDashList:
 *   This function converts a string to a zero-terminated string
 *   of values used as the dash list for a GC.
 ****************************************************************/
/*ARGSUSED*/
static Boolean CvtStringToDashList(display, args, num_args, 
				   fromVal, toVal, converter_data)
     Display             *display;
     XrmValuePtr          args;
     Cardinal           * num_args;
     XrmValuePtr	  fromVal;
     XrmValuePtr	  toVal;
     XtPointer           *converter_data;
{
  unsigned char      * dl;
  unsigned char        l;
  char               * str;
  int                  pos;
  int                  f;
  String               params[1];
  Cardinal             num_params = 1;
  int                  loop;
  int                  nfirst;

  *converter_data = NULL;

  dl = NULL;
  f  = True;
  for (loop = 0; loop<2; loop++) {

    pos = 0;
    nfirst = False;
    str = (char *) fromVal->addr;
    while (*str == ' ' || *str == '\t' || *str == '\n') str++;
    
    while (*str != '\0') {
      l = getnum(nfirst, &str, &f);
      nfirst = True;
      if (!f) goto fault;
      while (*str == ' ' || *str == '\t' || *str == '\n') str++;
      if (loop > 0) {
	dl[pos] = l;
	pos++;
      } else
	pos++;
    }

    if (loop == 0) {
      dl = (unsigned char *) XtMalloc(sizeof(unsigned char) * (pos+1));
    } else {
      dl[pos] = 0;
    }
  }

  /* all done */
  done(unsigned char *, dl)

 fault:
  params[0] = (char *) fromVal->addr;
  XtAppWarningMsg(XtDisplayToApplicationContext(display),
		  "CvtStringToDashList", "syntax error",
		  "tuCvtError", "Dash List syntax error: \"%s\"", 
		  params, &num_params);
  return False;
}


/****************************************************************
 *
 * X resource conversion procedure arguments 
 *
 ****************************************************************/
#include <X11/IntrinsicP.h> 
#include <X11/CoreP.h> 

static XtConvertArgRec screenCvtArg[] = {
  {
    XtWidgetBaseOffset, 
    (XtPointer)XtOffset(Widget, core.screen), 
    sizeof(Screen *)
  }
};

static XtConvertArgRec cursorCvtArg[] = {
  {
    XtWidgetBaseOffset,
    (XtPointer)XtOffset(Widget, core.screen), 
    sizeof(Screen *)
  },
  {
    XtBaseOffset,
    (XtPointer)XtOffset(Widget, core.self), 
    sizeof(Widget)
  },
};

static XtConvertArgRec widgetCvtArgs[] = {
  {
    XtBaseOffset,
    (XtPointer)XtOffset(Widget, core.self), 
    sizeof(Widget)
  },
};

static XtConvertArgRec bitmapConvertArg[] = {
  { 
    XtWidgetBaseOffset,
    (XtPointer)XtOffset(Widget, core.screen), 
    sizeof(Screen *)
  },
};

static XtConvertArgRec pixmapConvertArg[] = {
  {
    XtWidgetBaseOffset,
    (XtPointer)XtOffset(Widget, core.screen), 
    sizeof(Screen *)
  },
  {
    XtWidgetBaseOffset,
    (XtPointer)XtOffset(Widget, core.colormap), 
    sizeof(Colormap)
  },
};


/****************************************************************
 * tu_xt_resource_converters:
 *    This procedure initializes the converters defined
 *    in this file.
 ****************************************************************/
/*CExtract*/
void tu_xt_resource_converters()
{
  /*
   * new version of type converters ...
   */
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "UIMSBitmap", 
			CvtStringToBitmap,
			bitmapConvertArg, XtNumber(bitmapConvertArg),
			XtCacheAll|XtCacheRefCount, DestroyBitmap);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Bitmap", 
			CvtStringToBitmap,
			bitmapConvertArg, XtNumber(bitmapConvertArg),
			XtCacheAll|XtCacheRefCount, DestroyBitmap);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Picture",
			CvtStringToPixmap,
			pixmapConvertArg, XtNumber(pixmapConvertArg), 
			XtCacheAll|XtCacheRefCount, DestroyPixmap);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "UIMSPixmap",
			CvtStringToPixmap,
			pixmapConvertArg, XtNumber(pixmapConvertArg),
			XtCacheAll|XtCacheRefCount, DestroyPixmap);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Pixmap",
			CvtStringToPixmap,
			pixmapConvertArg, XtNumber(pixmapConvertArg),
			XtCacheAll|XtCacheRefCount, DestroyPixmap);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Pattern",
			CvtStringToPixmap,
			pixmapConvertArg, XtNumber(pixmapConvertArg), 
			XtCacheAll|XtCacheRefCount, DestroyPixmap);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, XtRPixel,
			CvtStringToPixel,
			pixmapConvertArg, XtNumber(pixmapConvertArg),
			XtCacheByDisplay, FreePixel);

#ifndef __osf__
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Cursor",
			CvtStringToCursor,      
			cursorCvtArg, XtNumber(cursorCvtArg),
			XtCacheNone, NULL);
#endif
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Button",
			CvtStringToButton,
			NULL, 0, XtCacheNone, NULL);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Double",
			CvtStringToDouble,
			NULL, 0, XtCacheNone, NULL);
  
  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Float",
			CvtStringToFloat,
			NULL, 0, XtCacheNone, NULL);
  
  XtAppSetTypeConverter(tu_global_app_context,
			TuRWidgetChild, XtRString, 
			CvtWidgetToString, 
			NULL, 0, XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRWidgetChild, 
			CvtStringToWidgetChild, 
			widgetCvtArgs, XtNumber(widgetCvtArgs),
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRDirectWidgetChild, 
			CvtStringToDirectWidgetChild, 
			widgetCvtArgs, XtNumber(widgetCvtArgs),
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			TuRDirectWidgetChild, XtRString, 
			CvtWidgetToString, 
			NULL, 0, XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRSiblingWidget, 
			CvtStringToSiblingWidget, 
			widgetCvtArgs, XtNumber(widgetCvtArgs),
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			TuRSiblingWidget, XtRString, 
			CvtWidgetToString, 
			NULL, 0, XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRSubMenuWidget, 
			CvtStringToSubMenuWidget, 
			widgetCvtArgs, XtNumber(widgetCvtArgs),
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			TuRSubMenuWidget, XtRString, 
			CvtWidgetToString, 
			NULL, 0, XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRSubMenuItemWidget, 
			CvtStringToSubMenuItemWidget, 
			widgetCvtArgs, XtNumber(widgetCvtArgs),
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			TuRSubMenuItemWidget, XtRString, 
			CvtWidgetToString, 
			NULL, 0, XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, XmRWidget, 
			CvtStringToWidget, 
			widgetCvtArgs, XtNumber(widgetCvtArgs),
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XmRWidget, XtRString, 
			CvtWidgetToString, 
			NULL, 0, XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, XtRScreen, 
			CvtStringToScreen,
			NULL, 0,
			XtCacheNone, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRXRectangleList, 
			CvtStringToXRectList,
			NULL, 0,
			XtCacheAll, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRIntTable, 
			CvtStringToIntTable,
			NULL, 0,
			XtCacheAll | XtCacheRefCount , CvtFreeIntTable);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, TuRDashList, 
			CvtStringToDashList,
			NULL, 0,
			XtCacheAll, NULL);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, XtRStringArray,
			_CvtStringToStringTable, 
			(XtConvertArgList) NULL, 0,
			XtCacheAll | XtCacheRefCount, _CvtFreeStringTable);

  XtAppSetTypeConverter(tu_global_app_context,
			XtRString, "Long",
			CvtStringToLong,
			NULL, 0,
			XtCacheNone, NULL);

}


#endif  /* TU_CVT */



/****************************************************************
 * 
 * Widget handling support routines
 * 
 ****************************************************************/

#define BY_NAME  1
#define BY_CLASS 2

/* 
 * Have to do this - only way to get hold of popup children, and a
 * few other things. Yuck!
 */
#include <X11/IntrinsicP.h> 
#include <X11/CoreP.h> 


/****************************************************************
 * tu_widget_name:
 *     Returns the name of the widget (stored in the widget).
 ****************************************************************/
/*CExtract*/
char * tu_widget_name(widget)
    Widget widget;
{
  return XtName(widget);
}


/****************************************************************
 * tu_widget_xrm_name:
 ****************************************************************/
/*CExtract*/
XrmName tu_widget_xrm_name(widget)
     Widget widget;
{
  return widget->core.xrm_name;
}


/****************************************************************
 * tu_widget_class:
 ****************************************************************/
WidgetClass tu_widget_class(widget)
     Widget widget;
{
  return XtClass(widget);
}


/****************************************************************
 * tu_widget_top_widget:
 *      Returns the first parent (this widget included) 
 *      that is a subclass of Shell.
 ****************************************************************/
/*CExtract*/
Widget tu_widget_top_widget(widget)
     Widget widget;
{
  while (widget) {
    if (XtIsSubclass(widget, shellWidgetClass))
      return widget;
    widget = XtParent(widget);
  }
  return NULL;
}


/****************************************************************
 * tu_widget_root_widget:
 *      Returns the first parent (this widget included) 
 *      that is a subclass of TopLevelShell or 
 *      ApplicationShell.
 ****************************************************************/
/*CExtract*/
Widget tu_widget_root_widget(widget)
     Widget widget;
{
  while (widget) {
    if (XtIsSubclass(widget, topLevelShellWidgetClass))
      return widget;
    if (XtIsSubclass(widget, applicationShellWidgetClass))
      return widget;
    widget = XtParent(widget);
  }
  return NULL;
}


/****************************************************************
 * tu_widget_children:
 *     This routine fetches all children (depending on
 *     the flg argument) for a widget. The array into 
 *     which the widgets are placed is either sent
 *     as an argument or, if the argument is NULL, allocated
 *     in the function. If a max argument > 0 is given, then
 *     this is the maximum number of children that is 
 *     considered.
 ****************************************************************/
/*CExtract*/
Widget *tu_widget_children(widget, array, pcount, max, flg)
     Widget widget;
     Widget *array;
     int *pcount;
     int max;
     int flg;
{
  unsigned int num_children;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i;
  int offset;

  num_children = 0;

  if (flg & TU_FW_POPUP)
    if (XtIsSubclass(widget, widgetClass))
      num_children += widget->core.num_popups;

  if (flg & TU_FW_NORMAL)
    if (XtIsSubclass(widget, compositeWidgetClass)) {
      XtSetArg(args[0], XtNnumChildren, &nchildren);
      XtGetValues(widget, args, 1);
      num_children += nchildren;
    }

  if (max > 0 && max < num_children)
    num_children = max;

  if ((long) num_children > 0)
    if (array == NULL) {
      array = (Widget *) malloc(sizeof(Widget)*num_children);
      if (array == NULL) {
	*pcount = 0;
	return NULL;
      }
    }

  offset = 0;
  if (flg & TU_FW_NORMAL) 
    if (XtIsSubclass(widget, compositeWidgetClass)) {
      XtSetArg(args[0], XtNchildren, &children);
      XtGetValues(widget, args, 1);
      for (i=0;i<nchildren;i++)
	if (i < num_children)
	  array[i] = children[i];
	else
	  break;
      offset += i;;
    }

  if (flg & TU_FW_POPUP)
    if (XtIsSubclass(widget, widgetClass))
      for (i=0;i<widget->core.num_popups;i++)
	if ((i + offset) < num_children)
	  array[i + offset] = widget->core.popup_list[i];
	else
	  break;

  *pcount = num_children;
  return array;
}


/****************************************************************
 * tu_widget_num_children:
 *     Returns the number of (specified) children.
 ****************************************************************/
/*CExtract*/
int tu_widget_num_children(widget, flg)
     Widget widget;
     int flg;
{
  unsigned int num_children;
  Arg args[1];
  Cardinal nchildren;

  num_children = 0;

  if (flg & TU_FW_POPUP)
    if (XtIsSubclass(widget, widgetClass))
      num_children += widget->core.num_popups;

  if (flg & TU_FW_NORMAL)
    if (XtIsSubclass(widget, compositeWidgetClass)) {
      XtSetArg(args[0], XtNnumChildren, &nchildren);
      XtGetValues(widget, args, 1);
      num_children += nchildren;
    }

  return num_children;
}


/****************************************************************
 * tu_widget_child_placement:
 *   Returns the position (insert pos) of the widget 
 *   specified. A negative number (-1) is returned if the
 *   widget is not a child of its parent!!!
 ****************************************************************/
int tu_widget_child_placement(widget)
     Widget widget;
{
  Widget p = XtParent(widget);
  Cardinal nch;
  WidgetList warr;
  Arg args[2];
  int i;

  if (p == NULL)
    return -1;

  if (XtIsSubclass(p, compositeWidgetClass)) {
    XtSetArg(args[0], XtNchildren, &warr);
    XtSetArg(args[1], XtNnumChildren, &nch);
    XtGetValues(p, args, 2);
    for (i=0; i<nch; i++)
      if (warr[i] == widget)
	return i;
  }
  return -1;
}

/****************************************************************
 * tu_widget_move_child:
 *     This routine moves a widget to the given place of the child
 *     list of the parent. The parameter 'way' tells whether
 *     the child should be moved upwards (TU_MC_UP), downwards
 *     (TU_MC_DOWN), or to a specified place (TU_MC_PLACE).
 *     The parameter 'number' specifies the number of positions
 *     the widget should be moved (0 means top/bottom), in case
 *     of up/downwards movement, or a specific place number
 *     (1 is first, 0 is last).
 ****************************************************************/
/*CExtract*/
void tu_widget_move_child(widget, way, number)
     Widget widget;
     int    way;
     int    number;
{
  Widget        wp;
  WidgetList    warr;
  Cardinal      nch;
  Arg           args[2];
  int           i;
  int           found;

  wp = XtParent(widget);

  if (wp) {
    found = -1;
    if (XtIsSubclass(wp, compositeWidgetClass)) {
      XtSetArg(args[0], XtNchildren, &warr);
      XtSetArg(args[1], XtNnumChildren, &nch);
      XtGetValues(wp, args, 2);
      for (i=0; i<nch; i++)
	if (warr[i]==widget) {
	  found=i;
	  break;
	}
    }
    if (found<0 && XtIsSubclass(wp, widgetClass)) {
      nch = wp->core.num_popups;
      warr = wp->core.popup_list;
      for (i=0; i<nch; i++)
	if (warr[i]==widget) {
	  found=i;
	  break;
	}
    }
    if (found>=0) {
      if (way==TU_MC_PLACE) {
	if (number==0)
	  way=TU_MC_DOWN;
	else {
	  number--;
	  if (found>number) {
	    way=TU_MC_UP;
	    number=found-number;
	  }
	  else if (found<number) {
	    way=TU_MC_DOWN;	
	    number=number-found;
	  }
	  else
	    return;
	}
      }
      if (way==TU_MC_UP) {
	for (i=found; i>0; i--) {
	  warr[i] = warr[i-1];
	  number--;
	  if (number==0) {
	    i--;
	    break;
	  }
	}
	warr[i] = widget;
      }
      else if (way==TU_MC_DOWN) {
	for (i=found; i<nch-1; i++) {
	  warr[i] = warr[i+1];
	  number--;
	  if (number==0) {
	    i++;
	    break;
	  }
	}
	warr[i] = widget;
      }
    }
  }
}


/****************************************************************
 * find_widget:
 *    Searches for a widget recursively until all widgets have
 *    been considered or a matching widget is found. The match
 *    can be either by name or by class name, according to 'way'.
 *    The flg parameter specifies if normal and/or popup children
 *    should be considered. The depth count tells the number 
 *    of more widgets levels that should be considered.
 ****************************************************************/
static Widget find_widget(widget, target, way, flg, depth)
     Widget widget;
     XrmQuark target;
     int way;
     int flg;
     int depth;
{
  Widget w;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i;

  switch (way)
    {
    case BY_NAME:
      if (target == widget->core.xrm_name)
	return widget;
      break;
    case BY_CLASS:
      if (target == widget->core.widget_class->core_class.xrm_class)
	return widget;
      break;
    }

  /* 
   * note that a zero or negative depth will search the 
   * whole widget hierarchy.
   */
  depth--;
  if (depth == 0)
    return NULL;

  if (flg & TU_FW_NORMAL)
    if (XtIsSubclass(widget, compositeWidgetClass)) {
      XtSetArg(args[0], XtNchildren, &children);
      XtSetArg(args[1], XtNnumChildren, &nchildren);
      XtGetValues(widget, args, 2);

      for (i=0;i<nchildren;i++)
	if (w = find_widget(children[i], target, way, flg, depth))
	  return w;
    }

  if (flg & TU_FW_POPUP)
    if (XtIsSubclass(widget, widgetClass))
      for (i=0;i<widget->core.num_popups;i++)
	if (w = find_widget(widget->core.popup_list[i],
			    target, way, flg, depth))
	  return w;

  return NULL;
}


/****************************************************************
 * tu_find_widget:
 *    Searches for a widget (see find_widget above).
 ****************************************************************/
/*CExtract*/
Widget tu_find_widget(widget, name, flg, depth)
    Widget widget;
    char * name;
    int    flg;
    int    depth;
{
  return find_widget(widget, XrmStringToQuark(name), BY_NAME, flg, depth);
}


/****************************************************************
 * tu_find_sibling:
 *    Searches for a sibling widget (see find_widget above).
 ****************************************************************/
/*CExtract*/
Widget tu_find_sibling(widget, name)
    Widget widget;
    char * name;
{
  XrmQuark xrm_name = XrmStringToQuark(name);
  Widget w;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i;

  if (XtIsSubclass(widget, compositeWidgetClass)) {
    XtSetArg(args[0], XtNchildren, &children);
    XtSetArg(args[1], XtNnumChildren, &nchildren);
    XtGetValues(widget, args, 2);
    for (i=0;i<nchildren;i++) {
      w = children[i];
      if (w->core.xrm_name == xrm_name)
	return w;
    }
  }
  return NULL;
}


/****************************************************************
 * tu_find_parent:
 *    Searches for a named parent from a specifed node.
 ****************************************************************/
/*CExtract*/
Widget tu_find_parent(widget, name)
     Widget widget;
     char *name;
{
  XrmName xrm_name;

  xrm_name = XrmStringToQuark(name);
  while (widget)
    if (widget->core.xrm_name == xrm_name) return widget;
    else widget = XtParent(widget);
  return NULL;
}


/****************************************************************
 * tu_find_submenu_widget:
 *    Searches for a widget by going to the parent and
 *    examining the popups for a child.
 ****************************************************************/
/*CExtract*/
Widget tu_find_submenu_widget(widget, name)
     Widget widget;
     char *name;
{
  XrmName qname;
  Widget w;
  Widget parent;
  int i;

  qname = XrmStringToQuark(name);
  parent = XtParent(widget);

  if (parent == NULL || !XtIsSubclass(parent, widgetClass))
    return NULL;

  for (i=0;i<parent->core.num_popups;i++)
    if (w = find_widget(parent->core.popup_list[i], qname, BY_NAME,
			TU_FW_NORMAL, 2))
      return w;

  parent = XtParent(parent);
  if (parent == NULL || !XtIsSubclass(parent, widgetClass))
    return NULL;

  for (i=0;i<parent->core.num_popups;i++)
    if (w = find_widget(parent->core.popup_list[i], qname, BY_NAME,
			TU_FW_NORMAL, 2))
      return w;
  
  return NULL;
}


/****************************************************************
 * tu_find_submenu_item_widget:
 *    Searches for a widget by going to the parent and
 *    examining the popups to see if a child is found
 *    in one of these popups.
 ****************************************************************/
/*CExtract*/
Widget tu_find_submenu_item_widget(widget, name)
     Widget widget;
     char *name;
{
  XrmName qname;
  Widget w;
  Widget w2, w3;
  Widget parent;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i, j;

  qname = XrmStringToQuark(name);
  parent = XtParent(widget);

  if (parent == NULL || !XtIsSubclass(parent, widgetClass))
    return NULL;

  for (i=0;i<parent->core.num_popups;i++) {
    w = parent->core.popup_list[i];
    if (!XtIsSubclass(w, compositeWidgetClass))
      continue;
    XtSetArg(args[0], XtNchildren, &children);
    XtSetArg(args[1], XtNnumChildren, &nchildren);
    XtGetValues(w, args, 2);
    for (j=0; j < nchildren; j++) {
      w2 =  children[j];
      if (w3 = find_widget(w2, qname, BY_NAME, TU_FW_NORMAL, 2))
	return w3;
    }
  }
  return NULL;
}


/****************************************************************
 * tu_find_widget_by_class:
 *    Searches for a widget (see find_widget described earlier).
 ****************************************************************/
/*CExtract*/
Widget tu_find_widget_by_class(widget, class_name, flg, depth)
    Widget widget;
    char * class_name;
    int    flg;
    int    depth;
{
  return find_widget(widget, XrmStringToQuark(class_name), BY_CLASS,
		     flg, depth);
}


/****************************************************************
 * tu_find_parent_by_class:
 *    Searches for a named parent from a specifed node.
 ****************************************************************/
/*CExtract*/
Widget tu_find_parent_by_class(widget, class_name)
     Widget widget;
     char *class_name;
{
  XrmClass xrm_name;

  xrm_name = XrmStringToQuark(class_name);
  while (widget)
    if (widget->core.widget_class->core_class.xrm_class == xrm_name)
      return widget;
    else
      widget = XtParent(widget);
  return NULL;
}


/****************************************************************
 * tu_first_child:
 *   Returns the first normal child (if any).
 ****************************************************************/
/*CExtract*/
Widget tu_first_child(widget)
     Widget widget;
{
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];

  if (XtIsSubclass(widget, compositeWidgetClass)) {
    XtSetArg(args[0], XtNchildren, &children);
    XtSetArg(args[1], XtNnumChildren, &nchildren);
    XtGetValues(widget, args, 2);
    if (nchildren > 0)
      return children[0];
  }
  return NULL;
}


/****************************************************************
 * tu_first_widget_child:
 *   Returns the first normal child (subclass of core).
 ****************************************************************/
/*CExtract*/
Widget tu_first_widget_child(widget)
     Widget widget;
{
  Widget w;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i;

  if (XtIsSubclass(widget, compositeWidgetClass)) {
    XtSetArg(args[0], XtNchildren, &children);
    XtSetArg(args[1], XtNnumChildren, &nchildren);
    XtGetValues(widget, args, 2);
    for (i=0; i < nchildren; i++) {
      w = children[i];
      if (XtIsSubclass(w, widgetClass))
	return w;
    }
  }
  return NULL;
}


/****************************************************************
 * tu_first_popup:
 *   Returns the first popup child (if any).
 ****************************************************************/
/*CExtract*/
Widget tu_first_popup(widget)
     Widget widget;
{
  if (XtIsSubclass(widget, widgetClass))
    if ((long)widget->core.num_popups > 0)
      return widget->core.popup_list[0];
  return NULL;
}


/****************************************************************
 * tu_first_managed:
 *      Returns the first managed child (if any).
 ****************************************************************/
/*CExtract*/
Widget tu_first_managed(widget)
     Widget widget;
{
  Widget w;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i;

  if (XtIsSubclass(widget, compositeWidgetClass)) {
    XtSetArg(args[0], XtNchildren, &children);
    XtSetArg(args[1], XtNnumChildren, &nchildren);
    XtGetValues(widget, args, 2);
    for (i=0; i < nchildren; i++) {
      w = children[i];
      if (XtIsManaged(w)) return w;
    }
  }
  return NULL;
}


/****************************************************************
 * tu_class_name:
 *     Returns the name of a class.
 ****************************************************************/
/*CExtract*/
char *tu_class_name(widget_class)
     WidgetClass widget_class;
{
  return widget_class->core_class.class_name;
}


/****************************************************************
 * tu_class_super_class:
 *   Returns the WidgetClass for the super class of the given
 *   WidgetClass.
 ****************************************************************/
/*CExtract*/
WidgetClass tu_class_super_class(widget_class)
    WidgetClass widget_class;
{
  WidgetClass  result;

  result = NULL;

  if (widget_class) {
    result = widget_class->core_class.superclass;
  }
  return (result);
}


/****************************************************************
 * tu_acc_install_all_accelerators:
 *   I think that this definition is much more useful than
 *   the standard X11. This is the one used when the 
 *   installAccelerators attribute is set with 'All:'.
 ****************************************************************/
/*CExtract*/
void tu_acc_install_all_accelerators(destination, source)
     Widget destination;
     Widget source;
{
  register int i;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];

  /* Recurse down normal children */
  if (XtIsComposite(destination)) {
    XtSetArg(args[0], XtNchildren, &children);
    XtSetArg(args[1], XtNnumChildren, &nchildren);
    XtGetValues(destination, args, 2);
    for (i = 0; i < nchildren; i++) {
      tu_acc_install_all_accelerators(children[i], source);
    }
  }

  /* Recurse down popup children */
  if (XtIsSubclass(destination, widgetClass)) {
#if DO_FORWARD_TO_POPUPS
    for (i = 0; i < destination->core.num_popups; i++) {
      tu_acc_install_all_accelerators(destination->core.popup_list[i], source);
    }
#endif
    /* Finally, apply procedure to this widget */
    XtInstallAccelerators(destination, source);
  }
}


/****************************************************************
 * tu_acc_set_accelerators:
 *   This is a function that takes a widget and a named
 *   installation target. It tries to find the target and install
 *   the accelerators.
 ****************************************************************/
/*CExtract*/
void tu_acc_set_accelerators(widget, target)
     Widget widget;
     char *target;
{
  Widget destination = NULL;
  char *p = (char *) target;
  char name[200];
  Widget w;
  int bflg = 0;

  if ((p == NULL) || (strlen(p) == 0))
    return;

  if (strncmp(p, "All:", 4) == 0) {
    bflg = 1;
    p += 4;
    while (*p == ' ') 
      p++;
    (void) strcpy(name, p);
    p = &name[strlen(p)-1];
    while (((*p == ' ') || (*p == '\n')) && (p > name))
      *p-- = '\0';
    p = name;
  }

  w = widget;
  while (w) {
    destination = tu_find_widget(w, p, TU_FW_NORMAL, 0);
    if (destination != NULL)
      break;
    if (XtIsSubclass(w, shellWidgetClass))
      break;
    w = XtParent(w);
  }

  if (destination)
    if (bflg)
      tu_acc_install_all_accelerators(destination, widget);
    else
      XtInstallAccelerators(destination, widget);
  else {
    EPUT1("ERROR(InstallAccelerators)\n");
    EPUT2("  Accelerator Installation: Widget '%s' not found\n", p);
    EPUT3("  [Invoked from node '%s:%s']\n", 
	  tu_widget_name(widget),
	  tu_class_name(tu_widget_class(widget)));
  }
}



#if OPEN_LOOK

/****************************************************************
 * 
 * Open Look support routines
 * 
 ****************************************************************/

#define SUNNEWS "X11/NeWS - Sun Microsystems"

static unsigned int get_modifier(str)
     char *str;
{
  unsigned int modifiers = 0;
  unsigned int maskBit;
  char *mod;
  Boolean notFlag;
  
  if (!str)
    /* The default is any modifiers */
    return AnyModifier;

  SKIPSP(str);

  while ((*str != '<') && (*str != '\0')) {
    if (*str == '~') {
      notFlag = TRUE;
      str++;
    } else 
      notFlag = FALSE;
    
    mod = getid(&str, (char*)NULL);
    if (mod == NULL) {
      return 0;
    }
    
    if (strcmp(mod, "None") == 0)
      maskBit = None;
    else if (strcmp(mod, "Shift") == 0)
      maskBit = ShiftMask;
    else if (strcmp(mod, "Lock") == 0)
      maskBit = LockMask;
    else if (strcmp(mod, "Ctrl") == 0)
      maskBit = ControlMask;
    else if (strcmp(mod, "Meta") == 0)
      maskBit = Mod1Mask;
    else if (strcmp(mod, "Alt") == 0)
      maskBit = Mod2Mask;
    else if (strcmp(mod, "Mod1") == 0)
      maskBit = Mod1Mask;
    else if (strcmp(mod, "Mod2") == 0)
      maskBit = Mod2Mask;
    else if (strcmp(mod, "Mod3") == 0)
      maskBit = Mod3Mask;
    else if (strcmp(mod, "Mod4") == 0)
      maskBit = Mod4Mask;
    else if (strcmp(mod, "Mod5") == 0)
      maskBit = Mod5Mask;
    else
      maskBit = 0;
    
    if (notFlag) 
      modifiers &= ~maskBit;
    else 
      modifiers |= maskBit;
    SKIPSP(str);
  }
  
  return modifiers;

}

static Boolean globalOLMenuFlag;

/****************************************************************
 * unused_button_event_handler:
 *    A button has been pressed somewhere in the hierarchy
 *    where MIT should have delivered the event differently.
 *    And nobody wants it...
 ****************************************************************/
/*ARGSUSED*/
static void unused_button_event_handler(w, closure, event, cont)
     Widget    w;
     XtPointer closure;
     XEvent  * event;
     Boolean * cont;
{
  if (globalOLMenuFlag) {
    globalOLMenuFlag = False;
    XUngrabPointer(XtDisplay(w), CurrentTime);
  }
}


/****************************************************************
 * button_event_handler:
 *    A button has been pressed somewhere in the hierarchy
 *    where MIT should have delivered the event differently.
 ****************************************************************/
/*ARGSUSED*/
static void button_event_handler(w, closure, event, cont)
     Widget    w;
     XtPointer closure;
     XEvent  * event;
     Boolean * cont;
{
  Widget            rc;
  XEvent            remapped;
  int               x, y;
  Window            child;
  unsigned int      button;
  char             *event_spec;
  Arg               args[3];
  unsigned int      modifier;
  Boolean           popupEnabled;

  if (event == NULL) return;

  rc = (Widget)closure;

  XtSetArg(args[0], XmNmenuPost, &event_spec);
  XtSetArg(args[1], XmNwhichButton, &button);
  XtSetArg(args[2], XmNpopupEnabled, &popupEnabled);
  XtGetValues(rc, args, 3);
  if (!popupEnabled)
    return;

  modifier = get_modifier(event_spec);

  if ((event->type == ButtonPress || event->type == ButtonRelease) &&
      (((button == event->xbutton.button) ||
	(button == AnyButton)) &&
       ((modifier == event->xbutton.state) ||
	(modifier == AnyModifier)))) {
#ifdef DEBUGX
      (void) printf("Remapped button press: 0x%x -> 0x%x\n",
		    w, XtParent(XtParent(rc)));
#endif
      remapped = *event;
      remapped.xbutton.window = XtWindow(XtParent(XtParent(rc)));
      (void) XTranslateCoordinates(XtDisplay(w), 
				   event->xbutton.window,
				   remapped.xbutton.window,
				   event->xbutton.x,
				   event->xbutton.y,
				   &x, &y, &child);
      remapped.xbutton.x = x;
      remapped.xbutton.y = y;
      XtDispatchEvent(&remapped);
      /* X11R4 (or greater) */
      *cont = False;
      globalOLMenuFlag = False;
    } else
      globalOLMenuFlag = True;
}


/****************************************************************
 * set_event_handlers:
 *     Sets the event handlers that are needed to remap
 *     an event to a rowcolumn widget.
 ****************************************************************/
static void set_event_handlers(rc, top_w, widget)
     Widget rc;
     Widget top_w;
     Widget widget;
{
  Widget w;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i;

  /* we don't care for gadgets */
  if (!XtIsSubclass(widget, widgetClass))
    return;

  /* take away the old one (if there) */
  XtRemoveEventHandler(widget, ButtonPressMask|ButtonReleaseMask, 
		       False, button_event_handler, (XtPointer)rc);

#ifdef DEBUGX
  (void) printf("Remapping 0x%x -> 0x%x\n", widget, top_w);
#endif

  if (top_w != widget) {
    /* set the event handlers needed */
    XtInsertEventHandler(widget, ButtonPressMask|ButtonReleaseMask, False,
			 button_event_handler, (XtPointer)rc, XtListHead);
    XtInsertEventHandler(widget, ButtonPressMask|ButtonReleaseMask, False,
			 unused_button_event_handler, NULL, XtListTail);

  }

  if (XtIsSubclass(widget, compositeWidgetClass)) {

    XtSetArg(args[0], XtNnumChildren, &nchildren);
    XtSetArg(args[1], XtNchildren, &children);
    XtGetValues(widget, args, 2);

    for (i=0;i<nchildren;i++) {
      w = children[i];
      if (w)
	set_event_handlers(rc, top_w, w);
    }
  }
}

 
/****************************************************************
 * tu_ol_fix_widget:
 *    Searches for a widget recursively until all widgets have
 *    been considered. All row columns' popup children are 
 *    examined to see if they are popups, and the event handlers 
 *    are added to handle button events.
 ****************************************************************/
/*CExtract*/
void tu_ol_fix_widget(widget)
     Widget widget;
{
  Widget w;
  Widget ww;
  WidgetList children;
  Cardinal nchildren;
  Arg args[2];
  int i;
  int j;

  if (xmRowColumnWidgetClass == NULL)
    return;

  if (widget == NULL)
    return;

  if (!XtIsSubclass(widget, widgetClass))
    return;

  /* take away the old one (if there) */
  XtRemoveEventHandler(widget, ButtonPressMask|ButtonReleaseMask, 
		       False, button_event_handler, (XtPointer)widget);
  
  if (XtIsSubclass(widget, compositeWidgetClass)) {

    XtSetArg(args[0], XtNnumChildren, &nchildren);
    XtSetArg(args[1], XtNchildren, &children);
    XtGetValues(widget, args, 2);

    for (i=0;i<nchildren;i++) {
      w = children[i];
      if (w)
	tu_ol_fix_widget(w);
    }
  }

  /* Go through popups. Unfortunately the only way to do this */

  for (i=0;i<widget->core.num_popups;i++) {
    w = widget->core.popup_list[i];
    tu_ol_fix_widget(w);
    if (XtIsSubclass(w, compositeWidgetClass)) {

      XtSetArg(args[0], XtNnumChildren, &nchildren);
      XtSetArg(args[1], XtNchildren, &children);
      XtGetValues(w, args, 2);

      for (j=0;j<nchildren;j++) {
	ww = children[j];
	if (XtIsSubclass(ww, xmRowColumnWidgetClass)) {
	  unsigned char rctype;

	  XtSetArg(args[0], XmNrowColumnType, &rctype);
	  XtGetValues(ww, args, 1);
	  if (rctype == XmMENU_POPUP) {
	    set_event_handlers(ww, widget, widget);
	  }
	}
      }
    }
  }
}


/****************************************************************
 * tu_ol_fix_hierarchy:
 *   This functions check if the server used is a X/NeWS server.
 *   If that is the case, then special event handlers are inserted
 *   to take care of Sun's special garb mode.
 ****************************************************************/
/*CExtract*/
void tu_ol_fix_hierarchy(w)
     Widget w;
{
  Widget top;
  char *s;

  if (w == NULL)
    return;

  if (!XtIsSubclass(w, widgetClass))
    return;

  s = ServerVendor(XtDisplay(w));
  if (s && (strncmp(s, SUNNEWS, strlen(SUNNEWS)) == 0)) {
    top = w;
    while (!XtIsSubclass(top, shellWidgetClass))
      top = XtParent(top);
    tu_ol_fix_widget(top);
  }
}

#endif /* OPEN_LOOK */

