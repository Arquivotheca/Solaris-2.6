/*
 * eject_popup.c --
 *
 *   A program invoked by "eject" command to display
 *   a popup window with a message telling the user to
 *   manually remove media from drive. This only makes sense
 *   on an X86 machine as its floppies are not software
 *   ejectable.
 *
 *   Expected is one command line argument as follows:
 *
 *   -n name of the media to remove
 *
 *   If all command line args are not present then a usage
 *   error will occur.
 */

/*
 * Copyright (c) 1995 by Sunsoft.
 */

#pragma ident "@(#)eject_popup.c  1.7  95/02/01 Sunsoft"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/defaults.h>
#include <xview/panel.h>
#include <xview/frame.h>
#include <xview/notice.h>

#define	MAX_BUF_LEN 256

/*
 * Forward declare my functions.
 */

static void create_popup(char *);
static void get_display_info(Display **, Xv_Screen *, int *, int *,
	int *, int *, Panel);
static void center_popup(Panel, int, int);
static void quit();
static void usage();

static void handle_ok(Panel_item, Event *);
static void pushpin_out(Frame);
static void set_resources();

/*
 * Global widget handles
 */

static	Frame popup;		/* handle to popup window */
static	Panel panel;		/* handle to default panel of popup */
static	Panel_item message1;	/* handle to message 1 on popup */
static	Panel_item ok_button;	/* handle to ok button on popup */

/*
 * Making these global simplifies data passing a bunch.
 */

static	char *prog_name;	/* argv[0] */

main(int argc, char *argv[])
{


	Display *dpy;		/* pointer to X display structure */
	Xv_Screen screen;	/* XView screen object */
	int screen_no;		/* X screen number */
	int screen_width;	/* X screen width */
	int screen_height;	/* X screen height */
	int screen_depth;	/* X screen depth */

	int fd;			/* Console file descriptor */
	int c;			/* Used to parse command line args */
	int name_ok = 0;	/* boolean status of cmd line name arg */
	char *media_name;	/* cmd line arg used in message */
	char buf[BUFSIZ];	/* used to build HELPPATH */
	char *helppath;		/* used to fetch HELPPATH from user env */

#ifdef DEBUG

	fprintf(stderr, "BEGIN eject_popup\n");

	/*
	 * Dump out this program's uid/gid stuff.
	 */

	fprintf(stderr, "uid = %ld\n", (uid_t) getuid());
	fprintf(stderr, "euid = %ld\n", (uid_t) geteuid());
	fprintf(stderr, "gid = %ld\n", (uid_t) getgid());
	fprintf(stderr, "egid = %ld\n", (uid_t) getegid());

#endif

	/*
	 * Redirect all messages to system console
	 */

	fd = open("/dev/console", O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	/*
	 * Init X stuff. Turn on I18N support.
	 */

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE, XV_LOCALE_DIR, DOMAIN_DIR, NULL);

#ifdef DEBUG
	if (!XSupportsLocale()) {
		fprintf(stderr, "eject_popup: locale '%s' NOT supported\n",
					setlocale(LC_ALL, NULL));
		exit(1);
	} else {
		fprintf(stderr, "eject_popup: locale '%s' supported\n",
					setlocale(LC_ALL, NULL));
	}
#endif

	/*
	 * Set the current text domain for I18N
	 */

#if !defined(TEXT_DOMAIN)
#define	TEXT_DOMAIN	"SYS_TEST"
#endif

	(void) textdomain(TEXT_DOMAIN);

	/*
	 * process arguments not munched by xv_init()
	 */

	prog_name = argv[0];

	while ((c = getopt(argc, argv, "n:")) != EOF) {
		switch (c) {
		case 'n':
			name_ok++;
			media_name = optarg;
			break;
		default:
			usage();
			exit(-1);
		}
	}

#ifdef DEBUG

	/*
	 * Dump out command line arguments
	 */

	fprintf(stderr, "name = '%s'\n", media_name);
#endif

	if (!name_ok) {
		usage();
		exit(-1);
	}

	/*
	 * Set up HELPPATH resources for this app.
	 * Base lead directory on their current locale.
	 * This way they can get localized OW help.
	 */

#ifdef DEBUG
	fprintf(stderr, "eject_popup: current locale = '%s'\n",
		setlocale(LC_MESSAGES, NULL));
#endif

	sprintf(buf, "HELPPATH=/usr/lib/locale/%s/LC_MESSAGES",
		setlocale(LC_MESSAGES, NULL));

	if (helppath = getenv("HELPPATH")) {
		strcat(buf, ":");
		strcat(buf, helppath);
	}

	putenv(buf);

#ifdef DEBUG
	fprintf(stderr, "eject_popup: HELPPATH = '%s'\n", buf);
#endif

	/*
	 * Set any resources we may need for this app.
	 */

	set_resources();

	/*
	 * Build the gui
	 */

	create_popup(media_name);

	/*
	 * Fetch display info so we can center popup window
	 */

	get_display_info(&dpy, &screen, &screen_no,
	    &screen_height, &screen_width, &screen_depth, popup);

	center_popup(popup, screen_height, screen_width);

	xv_main_loop(popup);

	/*
	 * Should never be reached as quit() used to exit.
	 */

	return (0);

} /* end main() */

/*
 * Function Name: set_resources()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   Enable the various XView resources
 *   needed by this program.
 */

static void
set_resources()
{

	/*
	 * We must make sure the user doesn't have
	 * the window.iconic resource set to TRUE
	 * when we display popup. If user does then
	 * we are hoseth'ed. Our gui won't show up.
	 *
	 * So we must set it to false for the duration
	 * of this program, and return it to the user's
	 * value before we leave.
	 */

	defaults_set_boolean("window.iconic", 0);

} /* end function set_resources() */


/*
 * Function Name: create_popup()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   Create the popup command frame. It needs to
 *   be large enough to handle an I18N message item
 *   and one button.
 */

static void
create_popup(char *media_name)
{

#define	TITLE		"Removable Media Manager"

	Rect popup_rect;		/* popup window rect data */
	Rect *msg1_rect;		/* msg 1 widget rect data */
	Rect *ok_rect;			/* ok button widget rect data */

	int button_x_gap;		/* pixel gap between buttons */
	int button_y_loc;		/* y axis location of buttons */
	int panel_item_y_gap = 15;	/* pixels between widgets */
	int panel_width;		/* panel width to hold widgets */
	char buf[MAX_BUF_LEN + 1];	/* used to hold message */

#ifdef DEBUG
	fprintf(stderr, "create_popup: \n");
#endif


	popup = (Frame) xv_create(NULL, FRAME_CMD,
			    WIN_USE_IM, FALSE,
			    FRAME_LABEL, gettext(TITLE),
			    FRAME_CMD_PUSHPIN_IN, TRUE,
			    FRAME_DONE_PROC, pushpin_out,
			    NULL);

	frame_get_rect(popup, &popup_rect);

#ifdef DEBUG
	fprintf(stderr, "Popup Frame %d Wide by %d High\n",
			popup_rect.r_width, popup_rect.r_height);
#endif

	panel = (Panel) xv_get(popup, FRAME_CMD_PANEL);
	xv_set(panel,
		XV_HELP_DATA, "eject_popup:panel",
		PANEL_ITEM_Y_GAP, panel_item_y_gap,
		NULL);

	/*
	 * Create the message.
	 */

	/* TRANSLATION_NOTE
	 *
	 * The %s in this string is the name(s) of the physical device(s)
	 * which can be ejected. It's a comma delimted list in the case
	 * of multiple devices.
	 *
	 * For example: floppy1
	 *		cdrom1
	 *		floppy0, floppy1
	 */

	sprintf(buf, gettext("%s can now be manually ejected"), media_name);
	message1 = (Panel_item) xv_create(panel, PANEL_MESSAGE,
					PANEL_LABEL_STRING, buf,
					NULL);

	msg1_rect = (Rect *) xv_get(message1, PANEL_ITEM_RECT);

#ifdef DEBUG
	fprintf(stderr, "Message 1 is %d Wide by %d High\n",
				msg1_rect->r_width, msg1_rect->r_height);
#endif

	/*
	 * Now create the buttons with default sizes in the popup.
	 */

	ok_button = (Panel_item) xv_create(panel, PANEL_BUTTON,
				XV_HELP_DATA, "eject_popup:ok",
				PANEL_LABEL_STRING, gettext("OK"),
				PANEL_NOTIFY_PROC, handle_ok,
				NULL);

	ok_rect = (Rect *) xv_get(ok_button, PANEL_ITEM_RECT);

#ifdef DEBUG
	fprintf(stderr, "OK is %d Wide by %d High\n",
			ok_rect->r_width, ok_rect->r_height);
#endif

	/*
	 * We have one slight problem due to the dynamic nature of the messages.
	 * It appears that it is possible for XView to create a message widget
	 * that is wider than the panel containing it.  I was unable to figure
	 * out a way around this so I put in this kludge.  Sorry.
	 *
	 * We'll enlarge panel to the larger of the message widths/hint widths
	 * plus 20 pixels.
	 *
	 * We use the panel_width variable to remember the actual final width.
	 */

	panel_width = msg1_rect->r_width + 20; /* add fudge */
	xv_set(panel, XV_WIDTH, panel_width, NULL);

	/*
	 * Now that we're sure of each widget's size we can finalize the
	 * height of our panel.  We'll allow the panel to extend the
	 * Y gap distance beyond the bottom of the row of buttons.
	 * There are 3 gaps in the Y direction.
	 * Don't need to save height in a var as it's never used after
	 * initial layout.
	 */

	xv_set(panel, XV_HEIGHT, (3*panel_item_y_gap) + msg1_rect->r_height +
				ok_rect->r_height,
				NULL);

#ifdef DEBUG
	fprintf(stderr, "Panel is %d Wide by %d High\n", panel_width,
			(3*panel_item_y_gap) + msg1_rect->r_height +
			ok_rect->r_height);
#endif

	/*
	 * Center message1 within the panel.
	 */

	xv_set(message1,
		XV_X, (int) (panel_width - msg1_rect->r_width)/2,
		XV_Y, panel_item_y_gap,
		NULL);

	/*
	 * Place the button beneath the message and center them.
	 */

	button_y_loc = (2*panel_item_y_gap) + msg1_rect->r_height;

	button_x_gap = (int) (panel_width - ok_rect->r_width) / 2;

#ifdef DEBUG
	fprintf(stderr, "putting ok at x=%d,  y=%d\n",
			button_x_gap, button_y_loc);
#endif

	xv_set(ok_button,
		XV_X, button_x_gap,
		XV_Y, button_y_loc,
		NULL);

	/*
	 * Shrink the frame to the panel's dimensions.
	 */

	window_fit(popup);

} /* end create_popup() */

/*
 * Function Name: get_display_info()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   Query the X Display for its size and other
 *   characteristics. Pass info back via args.
 */

static void
get_display_info(Display **dpy, Xv_screen *screen, int *screen_no,
			int *screen_height, int *screen_width,
			int *screen_depth, Panel popup)
{

#ifdef DEBUG
	fprintf(stderr, gettext("get_display_info: \n"));
#endif

	*dpy = (Display *) xv_get(popup, XV_DISPLAY);
	*screen = (Xv_Screen) xv_get(popup, XV_SCREEN);
	*screen_no = (int) xv_get(*screen, SCREEN_NUMBER);

	*screen_width = DisplayWidth(*dpy, *screen_no);
	*screen_height = DisplayHeight(*dpy, *screen_no);
	*screen_depth = DefaultDepth(*dpy, *screen_no);

#ifdef DEBUG
	fprintf(stderr, "In get_display_info():\n");
	fprintf(stderr, "Server screen = '%s'\n", (*dpy)->vendor);
	fprintf(stderr, "Screen #%d: width: %d, height: %d, depth: %d\n",
			*screen_no, *screen_width, *screen_height,
			*screen_depth);
#endif

} /* end get_display_info() */

/*
 * Function Name: center_popup()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   Center the popup in the middle of the users
 *   X display. Use height and width to determine
 *   center.
 */

static void
center_popup(Panel popup, int screen_height, int screen_width)
{

	Rect rect;

#ifdef DEBUG
	fprintf(stderr, gettext("center_popup: \n"));
#endif

	frame_get_rect(popup, &rect);

	rect.r_top = (int) (screen_height - rect.r_height)/2;
	rect.r_left = (int) (screen_width - rect.r_width)/2;

	frame_set_rect(popup, &rect);

} /* end center_popup() */

/*
 * Function Name: handle_ok()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   This is the callback routine for the "OK"
 *   button on the popup window. Since we must
 *   do the same thing for an "OK" and "Unpin"
 *   we just call quit() to do the real work.
 */

static void
handle_ok(Panel_item item, Event *event)
{

#ifdef DEBUG
	fprintf(stderr, gettext("handle_ok: \n"));
#endif

	quit();

} /* end handle_ok() */

/*
 * Function Name: pushpin_out()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   This is the callback routine for the "Unpin" event.
 *   Since we must do the same thing for an "OK" and the
 *   "Unpin" we just call quit() to do the real work.
 */

static void
pushpin_out(Frame popup)
{

#ifdef DEBUG
	fprintf(stderr, gettext("pushpin out: \n"));
#endif

	quit();

} /* end pushpin_out() */

/*
 * Function Name: quit()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   This is the routine used to exit this program.
 *   main() should never exit.
 */

static void
quit()
{

#ifdef DEBUG
	fprintf(stderr, gettext("quit: \n"));
	fprintf(stderr, "END eject_popup\n");
#endif

	exit(0);

} /* end quit() */

/*
 * Function Name: usage()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *  Print out the proper cmd line usage.
 */

static void
usage()
{

	/* TRANSLATION_NOTE
	 *
	 * The %s in this string is the name of the program.
	 */

	fprintf(stderr, gettext("usage: %s -n media_name\n"), prog_name);

} /* end usage() */
