#pragma ident "@(#)iconwin.c	1.1 95/03/17 Sun Microsystems"

/* iconwin.cc */

#include <stdlib.h>
#include <nl_types.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <sys/types.h>
#include <unistd.h>

void
set_icon_pixmap(Widget shell, Pixmap image, char *label)
{
	Window 		window, root;
	unsigned int 	width, height, border_width, depth;
	int 		x, y;
	Display 	*dpy = XtDisplay(shell);

	/* Get the icon window for the shell. */
	XtVaGetValues(shell, XmNiconWindow, &window, NULL);
	
	if (!window && image != NULL && image != XmUNSPECIFIED_PIXMAP) {
		/* 
		 * Create a window if there is not one currently
		 * associated with the shell. Make it as big as the pixmap
		 * we are going to use.
		 */
		if (!XGetGeometry(dpy, image, &root, &x, &y, 
			&width, &height, &border_width, &depth) ||
		    !(window = XCreateSimpleWindow(dpy, root, 0, 0, width, 
			height, (unsigned)0, CopyFromParent, CopyFromParent))) {
			      XtVaSetValues(shell, 
					XmNiconPixmap, image, 
					XmNiconName, label,
					NULL);
			      return;
		}
		/* Set the window to the shell. */
		XtVaSetValues(shell, XmNiconWindow, window, NULL);
	}
	XtVaSetValues(shell, XmNiconName, label, NULL);
	/* Set the windows background pixmap to be the image. */
	if (image != NULL && image != XmUNSPECIFIED_PIXMAP) {
		XSetWindowBackgroundPixmap(dpy, window, image);
		/* Redisplay the window. */
		XClearWindow(dpy, window);
	}

	return;
}
