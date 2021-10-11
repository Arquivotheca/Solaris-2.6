#pragma ident "@(#)solprep.c	1.1 95/03/17 Sun Microsystems"

/* solprep.cc */

#include <sys/types.h>
#include <unistd.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <stdio.h>

static void
map_window_event_handler(Widget w, XtPointer cd, XEvent *ev, Boolean * flag)
{
 
    pid_t       ppid[2];
    Atom        mapped_property;
    Display     *d = XtDisplayOfObject(w);
    XtPointer dummy = cd;
    Boolean   * bdummy = flag;
 
    if (ev->type == MapNotify) {
 
        mapped_property = XInternAtom(d, "ADM_APP_MAPPED_PROPERTY", True);
 
        if (mapped_property != None) {
 
            ppid[0] = getppid();
            ppid[1] = getpid();
 
            XChangeProperty(d, RootWindowOfScreen(XtScreen(w)),
                mapped_property, XA_INTEGER, 32, PropModeReplace,
                (unsigned char *)ppid, 2);
        }
 
        XtRemoveEventHandler(w, XtAllEvents, True,
            map_window_event_handler, NULL);
    }
}
 
void
solstice_launcher_setup(Widget w)
{
	XtAddEventHandler(w, StructureNotifyMask, False,
            map_window_event_handler, NULL);
}
