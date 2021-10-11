#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
 
#define YES 1
#define NO  0

#define OFFSET 10


extern XtAppContext	GappContext;
extern Widget		GtopLevel;
extern Display		*Gdisplay;
extern int		Gscreen;

#define RSC_CVT( rsc_name, rsc_value ) \
        XtVaTypedArg, (rsc_name), XmRString, (rsc_value), strlen(rsc_value) + 1

extern int 	AskUser(Widget, char *, char *, char *, int);
extern int 	Confirm(Widget, char*, int*, char*);
extern XmFontList ConvertFontList(char *);
extern void 	display_error(Widget, char *);
extern void 	display_warning(Widget, char*);
extern void 	format_the_date (int, int, int, char *);
extern void 	free_mem(void *);
extern void     ForceDialog(Widget w);
extern void 	helpCB(Widget, char*, XtPointer);
extern Widget 	get_shell_ancestor(Widget);
extern int 	min2(int, int);
extern void 	stringcopy ( char *, char *, int);
extern int 	wildmatch(char *, char *);
extern void 	yesnoCB(Widget, XtPointer, XmAnyCallbackStruct *);
extern int 	load_XPM(Widget w, char * pmfile, Pixel bg, Pixmap *pm);

extern void debug_log(FILE *, char *, char *, ...);
