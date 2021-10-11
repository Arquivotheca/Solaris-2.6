/*
 * volmissing_popup.c --
 *
 *   A program invoked by "volmissing -p" when an X
 *   window session is active and a NOTIFY event occurs
 *   in the volume management system. The popup allows
 *   the user to "Cancel" the pending i/o transaction or
 *   insert the missing volume and continue.
 *
 *   All message information is picked up from the volume
 *   management environment variables inherited by this
 *   process via fork/exec.
 */

/*
 * Copyright (c) 1995 by Sunsoft.
 */

#pragma ident "@(#)volmissing_popup.c  1.8  95/02/01 Sunsoft"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <xview/xview.h>
#include <xview/defaults.h>
#include <xview/panel.h>
#include <xview/frame.h>
#include <xview/notice.h>

#include "volmissing_popup.h"

/*
 * Forward declare my functions.
 */

static void create_error_message();
static void create_hint();
static void create_popup();
static void create_notice();
static void get_display_info(Display **, Xv_Screen *, int *, int *,
    int *, int *, Panel);
static void center_popup(Panel, int, int);
static int alias_snapshot(int);
static void eject_bogus_media();
static void quit();
static void usage();

static void ok_to_check_media(Panel_item, Event *);
static void cancel_io(Panel_item, Event *);
static void check_media();
static void pushpin_out(Frame);
static void set_resources();

/*
 * Global widget stuff
 */

static	Xv_Notice notice;	/* Notice widget used in application */
static	Frame popup;		/* handle to popup window */
static	Panel panel;		/* handle to default panel of popup */

static	Panel_item *error_msgs;	/* array of handles to lines of error message */
static	Rect **error_msg_rects;	/* error msg widget rect data array */
static	int num_error_msgs;	/* number of lines in the error message */

static	Panel_item *hints;	/* array of handles to hint msg lines */
static	Rect **hint_rects;	/* hint msg widget rect data array */
static	int	num_hints;	/* number of lines in the hint message */

static	Panel_item ok_button;	/* popup ok button handle */
static	Panel_item cancel_button;	/* popup cancel button handle */

/*
 * Making these global simplifies data passing a bunch.
 */

static	char *prog_name;		/* argv[0] */
static	char *vol_volumename;
static	char *vol_mediatype;
static	char ejectable_media_string[MAXNAMLEN+1];


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

	char buf[BUFSIZ];	/* used to build HELPPATH */
	char *helppath;		/* used to fetch HELPPATH from user env */
	char *sccs_id;		/* used to write sccs id stuff to .po file */

#ifdef DEBUG

	fprintf(stderr, "BEGIN volmissing_popup\n");

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
		fprintf(stderr, "volmissing_popup: locale '%s' NOT supported\n",
					setlocale(LC_ALL, NULL));
		exit(1);
	} else {
		fprintf(stderr, "volmissing_popup: locale '%s' supported\n",
					setlocale(LC_ALL, NULL));
	}
#endif

#if !defined(TEXT_DOMAIN)
#define	TEXT_DOMAIN	"SYS_TEST"
#endif

	(void) textdomain(TEXT_DOMAIN);

	/*
	 * process arguments not munched by xv_init()
	 */

	prog_name = argv[0];

	while ((c = getopt(argc, argv, "")) != EOF) {
		switch (c) {
		default:
			usage();
			exit(-1);
		}
	}

	/*
	 * Set up HELPPATH resources for this app.
	 * Base lead directory on their current locale.
	 * This way they can get localized OW help.
	 */

#ifdef DEBUG
	fprintf(stderr, "volmissing_popup: current locale = '%s'\n", setlocale(LC_MESSAGES, NULL));
#endif

	sprintf(buf, "HELPPATH=/usr/lib/locale/%s/LC_MESSAGES", setlocale(LC_MESSAGES, NULL));

	if (helppath = getenv("HELPPATH")) {
		strcat(buf, ":");
		strcat(buf, helppath);
	}

	putenv(buf);

#ifdef DEBUG
	fprintf(stderr, "volmissing_popup: HELPPATH = '%s'\n", buf);
#endif

	/*
	 * Set any resources we may need for this app.
	 */

	set_resources();

	/*
	 * Make sure an SCCS ID string is written to the portable
	 * message file.
	 */

	sccs_id = gettext(SCCS_ID);

	/*
	 * Build the gui
	 */

	create_popup();
	create_notice();

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
 * Function Name: create_hint()
 *
 * Globals Modified: hints, hint_rects, num_hints
 *
 * Return Code: None
 *
 * Description:
 *
 *	Take the hint string returned from gettext and
 * 	parse it into individual strings as it may
 * 	contain embedded newlines. Create a panel
 *	message item for each string and save the
 *	geometry rectangle for the widget. Also
 *	save a count of the number of lines in the
 *	hint.
 *
 *	The hint message describes the functionality of the
 *	OK and Cancel button widgets.
 */

static void
create_hint()
{

	int hint_cnt;
	char *tmp_ptr;
	char *l10n_hint_msg = NULL;	/* l10n'ed hint message string */

	/*
	 * Create the hint message widget array.
	 * We get the l10n'ed hint message and figure out
	 * how many lines it contains (we count newlines).
	 * Then malloc space for that many panel message items.
	 * This way we can display the hint message, potentially a
	 * multiline message, as a series of single mesage widgets.
	 * This technique will allow us to pretty up the display
	 * of this hint message using proper justification.
	 */

	l10n_hint_msg = strdup(gettext(HINT_MSG));

#ifdef DEBUG
	fprintf(stderr, "create_hint: l10n_hint_message = '%s'\n", l10n_hint_msg);
#endif 

	num_hints = 1;	/* There will always be at least one line */ 
	for (tmp_ptr = l10n_hint_msg; *tmp_ptr != '\0'; tmp_ptr++) {
		if (*tmp_ptr == '\n')
			num_hints++;
	}

#ifdef DEBUG
	fprintf(stderr, "create_hint: num_hints = %d\n", num_hints);
#endif 

	/*
	 * Test for the special case where the string may have
	 * a trailing newline and the next char is the NULL byte.
	 * In this case there is one less string than we think.
	 * We have to deduct one from the newline count. Also,
	 * we'll blotto the bogus newline for easier processing.
	 */

	tmp_ptr--;
	if (*tmp_ptr == '\n') {
		num_hints--;
		*tmp_ptr = '\0';
	}

#ifdef DEBUG
	fprintf(stderr, "create_hint: num_hints = %d (after specil check)\n", num_hints);
#endif 

	hints = (Panel_item *) malloc(num_hints * sizeof(Panel_item));

	if (hints == NULL) {
		perror("volmissing_popup: create_hint: hints malloc failed");
		exit(-1);
	} else {

#ifdef DEBUG
	fprintf(stderr, "create_hint: hints malloc'ed OK\n");
#endif 
		/* 
		 * Now lets take each line of the l10n message and
		 * create a panel message item for it. We'll store it
		 * in the hints array for easy use.
		 */

		hint_cnt = 0;

#ifdef DEBUG
	fprintf(stderr, "create_hint: hint count = %d\n", hint_cnt);
#endif 

		tmp_ptr = strtok(l10n_hint_msg, "\n");

#ifdef DEBUG
	fprintf(stderr, "create_hint: tmp_ptr (after 1st strtok) = '%s'\n", tmp_ptr);
#endif 

		while (tmp_ptr != NULL) {

#ifdef DEBUG
	fprintf(stderr, "create_hint: hint string = '%s'\n", tmp_ptr);
#endif 

			hints[hint_cnt] = (Panel_item) xv_create(panel, PANEL_MESSAGE,
						PANEL_LABEL_STRING, tmp_ptr,
						NULL);
#ifdef DEBUG
	fprintf(stderr, "create_hint: stored hint string = '%s'\n", xv_get(hints[hint_cnt], PANEL_LABEL_STRING));
#endif 

			hint_cnt++;
			tmp_ptr = strtok(NULL, "\n");
		}

	}

	/*
	 * Now get the rectangles for each panel_message
	 * Malloc up the appropriate space for the number
	 * of hint messages.
	 */

	hint_rects = (Rect **) malloc(num_hints * sizeof(Rect *));

	if (hint_rects == NULL) {
		perror("volmissing_popup: create_hint: hint_rects malloc failed");
		exit(-1);
	} else {

		/* 
		 * Now lets hint panel message widgets and get their
		 * geometry rectangles.
		 */

		for (hint_cnt = 0; hint_cnt < num_hints; hint_cnt++) {
			hint_rects[hint_cnt] = (Rect *) xv_get(hints[hint_cnt], PANEL_ITEM_RECT);


#ifdef DEBUG
	fprintf(stderr, "hints[%d] is %d Wide by %d High\n", hint_cnt,
			hint_rects[hint_cnt]->r_width, hint_rects[hint_cnt]->r_height);
#endif

		}
	}

} /* end create_hint() */


/*
 * Function Name: create_error_message()
 *
 * Globals Modified: error_msgs, error_msg_rects, num_error_msgs
 *                   vol_volumename, vol_mediatype
 *
 * Return Code: None
 *
 * Description:
 *
 *	Take the error message string returned from gettext and
 * 	parse it into individual strings as it may
 * 	contain embedded newlines. Create a panel
 *	message item for each string and save the
 *	geometry rectangle for the widget. Also
 *	save a count of the number of lines in the
 *	error_message.
 *
 */

static void
create_error_message()
{

	int error_msg_cnt;
	char *tmp_ptr;
	char *l10n_error_msg = NULL;	/* l10n'ed error message string */

	/*
	 * Volume Management Environment Variables.
	 */

	char	*vol_user;
	char	*vol_username;
	char	*vol_gecos = NULL;
	char	*vol_message;

	struct	passwd *pw;
	int 	message_len;    

	/*
	 * Grab the volume mgmt environment variables
	 * so we can build our message.
	 */

	vol_user = getenv("VOLUME_USER");
	vol_volumename = getenv("VOLUME_NAME");
	vol_mediatype = getenv("VOLUME_MEDIATYPE");

	/*
	 * If no environment variables we have to exit 
	 * with an error.
	 */

	if ((vol_user == NULL) || (vol_volumename == NULL) ||
		(vol_mediatype == NULL)) {

		fprintf(stderr, "%s: %s\n", prog_name, gettext(ENV_ERROR_MSG));

		exit(-1);

	}

	if ((pw = getpwnam(vol_user)) == NULL) {
		/* No passwd entry for this user */
		vol_username = vol_user;
		vol_gecos = NULL;
	} else {
		vol_username = strdup(pw->pw_name);
		vol_gecos = strdup(pw->pw_gecos);
	}

	/*
	 * L10N the proper error message format string. 
	 * Which format we use depends on whether the user
	 * is root not. For non-root users, we use different
	 * messages if they don't have a gecos.
	 */

	if (pw->pw_uid != 0) 
		if (vol_gecos != NULL) {


			/* TRANSLATION_NOTE
	 		 * 
	 		 * The 1st %s in this string is the login name of the end user.
	 		 * The 2nd %s in this string is the real name of the end user.
	 		 * The 3rd %s in this string is the type of media (cdrom/floppy).
	 		 * The 4th %s in this string is the label name of media (e.g., scratch).
	 		 */

			vol_message = strdup(gettext(STD_USER_MSG));
		} else {

			/* TRANSLATION_NOTE
	 		 * 
	 		 * The 1st %s in this string is the login name of the end user.
	 		 * The 2nd %s in this string is the type of media (cdrom/floppy).
	 		 * The 3rd %s in this string is the label name of media (e.g., scratch).
	 		 */

			vol_message = strdup(gettext(STD_USER_MSG_NO_GECOS));
		}
	else {

		/* TRANSLATION_NOTE
	 	 * 
	 	 * The 1st %s in this string is the type of media (cdrom/floppy).
	 	 * The 2nd %s in this string is the label name of media (e.g., scratch).
	 	 */

		vol_message = strdup(gettext(STD_ROOT_MSG));
	}

#ifdef DEBUG
	fprintf(stderr, "create_error_message: BEGIN ENV VARS\n\n");

	fprintf(stderr, "vol_user = '%s'\n", vol_user);
	fprintf(stderr, "vol_username = '%s'\n", vol_username);
	fprintf(stderr, "vol_gecos = '%s'\n", vol_gecos);
	fprintf(stderr, "vol_volumename = '%s'\n", vol_volumename);
	fprintf(stderr, "vol_mediatype = '%s'\n", vol_mediatype);

	fprintf(stderr, "pw->pw_name = '%s'\n", pw->pw_name);
	fprintf(stderr, "pw->pw_uid  = %d\n", pw->pw_uid);
	fprintf(stderr, "pw->pw_gid         = %d\n", pw->pw_gid);

	fprintf(stderr, "vol_message = '%s'\n", vol_message);

	fprintf(stderr, "create_error_message: END ENV VARS\n\n");
#endif

	/*
	 * Set up the popup message using the proper standard message string.
	 * Don't know how big it will be so we'll have to malloc some space.
	 * Also, we'll check to see if root caused this volmissing, if so
	 * we'll use a different format so it's prettier for the user. 
	 */

	if (pw->pw_uid == 0) {
		message_len = strlen(vol_message) + strlen(vol_mediatype) +
				strlen(vol_volumename);
	} else {
		if (vol_gecos != NULL) {
			message_len = strlen(vol_message) + strlen(vol_username) +
				strlen(vol_mediatype) +strlen(vol_volumename) +
				strlen(vol_gecos);
		}
		else {
			message_len = strlen(vol_message) + strlen(vol_username) +
				strlen(vol_mediatype) + strlen(vol_volumename);
		}
	}

	l10n_error_msg = (char *) malloc(message_len);

	if (l10n_error_msg == NULL) {
	  fprintf(stderr, "volmissing_popup : malloc error message failed\n");
	  exit(-1);
	}

	if (pw->pw_uid == 0) {
		sprintf(l10n_error_msg, vol_message, vol_mediatype, vol_volumename);
	} else {
		
		if (vol_gecos != NULL)
			sprintf(l10n_error_msg, vol_message, vol_username,
	      			vol_gecos, vol_mediatype, vol_volumename);
		else
			sprintf(l10n_error_msg, vol_message, vol_username,
	      			vol_mediatype, vol_volumename);

	}

#ifdef DEBUG
	fprintf(stderr, "create_error_message: l10n_error_msg = '%s'\n", l10n_error_msg);
#endif

	/*
	 * Create the error message widget array.
	 * We get the l10n'ed error message and figure out
	 * how many lines it contains (we count newlines).
	 * Then malloc space for that many panel message items.
	 * This way we can display the error message, potentially a
	 * multiline message, as a series of single message widgets.
	 * This technique will allow us to pretty up the display
	 * of this error message using proper justification.
	 */

	num_error_msgs = 1;	/* There will always be at least one line */ 
	for (tmp_ptr = l10n_error_msg; *tmp_ptr != '\0'; tmp_ptr++) {
		if (*tmp_ptr == '\n')
			num_error_msgs++;
	}

#ifdef DEBUG
	fprintf(stderr, "create_error_message: num_error_msgs = %d\n", num_error_msgs);
#endif

	/*
	 * Test for the special case where the string may have
	 * a trailing newline and the next char is the NULL byte.
	 * In this case there is one less string than we think.
	 * We have to deduct one from the newline count. Also,
	 * we'll blotto the bogus newline for easier processing.
	 */

	tmp_ptr--;
	if (*tmp_ptr == '\n') {
		num_error_msgs--;
		*tmp_ptr = '\0';
	}

#ifdef DEBUG
	fprintf(stderr, "create_error_message: num_error_msgs (after special case check) = %d\n", num_error_msgs);
#endif

	error_msgs = (Panel_item *) malloc(num_error_msgs * sizeof(Panel_item));

	if (error_msgs == NULL) {
		perror("volmissing_popup: create_error_message: error_msgs malloc failed");
		exit(-1);
	} else {

		/* 
		 * Now lets take each line of the l10n message and
		 * create a panel message item for it. We'll store it
		 * in the error_msgs array for easy use.
		 */

		error_msg_cnt = 0;
		tmp_ptr = strtok(l10n_error_msg, "\n");
		while (tmp_ptr != NULL) {

#ifdef DEBUG
	fprintf(stderr, "create_error_message: l10n error string = '%s'\n", tmp_ptr);
#endif

			error_msgs[error_msg_cnt] =  (Panel_item) xv_create(panel, PANEL_MESSAGE,
						PANEL_LABEL_STRING, tmp_ptr,
						NULL);

#ifdef DEBUG
	fprintf(stderr, "create_error_message: stored l10n error string = '%s'\n",
					(char *) xv_get(error_msgs[error_msg_cnt], PANEL_LABEL_STRING));
#endif

			error_msg_cnt++;
			tmp_ptr = strtok(NULL, "\n");
		}

	}

	/*
	 * Now get the rectangles for each panel_message
	 * Malloc up the appropriate space for the number
	 * of error messages.
	 */

	error_msg_rects = (Rect **) malloc(num_error_msgs * sizeof(Rect *));

	if (error_msg_rects == NULL) {
		perror("volmissing_popup: create_error_message: error_msg_rects malloc failed");
		exit(-1);
	} else {

		/* 
		 * Now lets get error_msg panel message widgets and get their
		 * geometry rectangles.
		 */

		for (error_msg_cnt = 0;
			error_msg_cnt < num_error_msgs; error_msg_cnt++) {

			error_msg_rects[error_msg_cnt] =
			(Rect *) xv_get(error_msgs[error_msg_cnt], PANEL_ITEM_RECT);


#ifdef DEBUG
	fprintf(stderr, "create_error_message: error_msgs[%d] is %d Wide by %d High\n",
			error_msg_cnt, error_msg_rects[error_msg_cnt]->r_width,
			error_msg_rects[error_msg_cnt]->r_height);
#endif

		}
	}

} /* end create_error_message() */

/*
 * Function Name: create_popup()
 *
 * Globals Modified:
 *
 *   popup, panel, error_msg_rects, hint_rects,
 *   ok_button, cancel_button
 *
 * Return Code: None
 *
 * Description:
 *
 *   Create the popup command frame. It needs to
 *   be large enough to handle 2 dynamic size
 *   messages (error and hint) that are broken
 *   into pieces. It also has OK and Cancel buttons.
 */

static void
create_popup()
{

	Rect popup_rect;		/* popup window rect data */
	Rect *ok_rect;			/* ok button widget rect data */
	Rect *cancel_rect;		/* cancel button widget rect data */

	int button_x_gap;		/* pixel gap between buttons */
	int button_y_loc;		/* y axis location of buttons */
	int panel_item_y_gap = 15;	/* pixels between widgets */
	int panel_width;		/* panel width to hold widgets */
	int i;				/* lcv */
	int rects_height;		/* sum of all widget heights */
	int height_offset;		/* summing var of widget heights */
	int widest_error;		/* widest error msg widget */
	int widest_hint;		/* widest hint msg widget */

#ifdef DEBUG
	fprintf(stderr, "create_popup: \n");
#endif

	popup = (Frame) xv_create(NULL, FRAME_CMD,
					WIN_USE_IM, FALSE,
			    		FRAME_LABEL, gettext(TITLE),
			    		FRAME_CMD_PUSHPIN_IN, TRUE,
			    		FRAME_DONE_PROC, pushpin_out,
			    		NULL);

#ifdef DEBUG
	fprintf(stderr, "xv_create of Popup Frame done\n");
#endif

	frame_get_rect(popup, &popup_rect);

#ifdef DEBUG
	fprintf(stderr, "Popup Frame %d Wide by %d High\n",
			popup_rect.r_width, popup_rect.r_height);
#endif

	panel = (Panel) xv_get(popup, FRAME_CMD_PANEL);
	xv_set(panel,
		XV_HELP_DATA, "volmissing_popup:panel",
		PANEL_ITEM_Y_GAP, panel_item_y_gap,
		NULL);

	/*
	 * Create the error message widgets and the hint
	 * widgets.
	 */

	create_error_message();
	create_hint();

	/*
	 * Now create the buttons with default sizes in the popup.
	 */

	ok_button = (Panel_item) xv_create(panel, PANEL_BUTTON,
				XV_HELP_DATA, "volmissing_popup:ok",
				PANEL_LABEL_STRING, gettext("OK"),
				PANEL_NOTIFY_PROC, ok_to_check_media,
				NULL);

	ok_rect = (Rect *) xv_get(ok_button, PANEL_ITEM_RECT);

#ifdef DEBUG
	fprintf(stderr, "OK is %d Wide by %d High\n",
			ok_rect->r_width, ok_rect->r_height);
#endif

	cancel_button = (Panel_item) xv_create(panel, PANEL_BUTTON,
				XV_HELP_DATA, "volmissing_popup:cancel",
				PANEL_LABEL_STRING, gettext("Cancel"),
				PANEL_NOTIFY_PROC, cancel_io,
				NULL);

	cancel_rect = (Rect *) xv_get(cancel_button, PANEL_ITEM_RECT);

#ifdef DEBUG
	fprintf(stderr, "CANCEL is %d Wide by %d High\n",
			cancel_rect->r_width, cancel_rect->r_height);
#endif

	/*
	 * We have one slight problem due to the dynamic nature of the messages.
	 * It appears that it is possible for XView to create a message widget
	 * that is wider than the panel containing it.  I was unable to figure
	 * out a way around this so I put in this kludge.  Sorry.
	 *
	 * We'll simply look through all of our rectangles and see which is 
	 * widest. And we'll use that width plus 20 pixels for our panel width.
	 *
	 * We'll initialize the panel_width to the width of the
	 * OK and Cancel buttons. 
	 *
	 * As we run the the error message rects and hint rects we identify
	 * the longest, respectively, which we'll use later for justifying
	 * the messages.
	 *
	 * As we run through the various rects we'll sum up the overall height
	 * of the rects. This will be used later to determine the real height of the 
	 * panel.
	 */

	panel_width = ok_rect->r_width + cancel_rect->r_width;

	widest_error = 0;
	rects_height = 0;
	for (i=0; i < num_error_msgs; i++) {

		if (error_msg_rects[i]->r_width > panel_width)
			panel_width = error_msg_rects[i]->r_width;

		if (error_msg_rects[i]->r_width > widest_error)
			widest_error = error_msg_rects[i]->r_width;

		rects_height += error_msg_rects[i]->r_height;

	}

	widest_hint = 0;
	for (i=0; i < num_hints; i++) {

		if (hint_rects[i]->r_width > panel_width)
			panel_width = hint_rects[i]->r_width;

		if (hint_rects[i]->r_width > widest_hint)
			widest_hint = hint_rects[i]->r_width;

		rects_height += hint_rects[i]->r_height;

	}

	/*
	 * Add in the fudge.
	 */

	panel_width += 20;
	xv_set(panel, XV_WIDTH, panel_width, NULL);

	/*
	 * Now that we're sure of each widget's size we can finalize the
	 * height of our panel.  We'll allow the panel to extend the
	 * Y gap distance beyond the bottom of the row of buttons.
	 * There are 4 gaps in the Y direction.
	 * Don't need to save height in a var as it's never used after
	 * initial layout.
	 */

	xv_set(panel, XV_HEIGHT, (4*panel_item_y_gap) + rects_height +
				ok_rect->r_height,
				NULL);

#ifdef DEBUG
	fprintf(stderr, "Panel is %d Wide by %d High\n", panel_width,
			(4*panel_item_y_gap) + rects_height + ok_rect->r_height);
#endif

	/*
	 * Position error message strings on panel. We'll use the widest string
	 * within the error message as the basis for justifying the others.
	 * The widest string will be centered on the panel. The others will end
	 * up being left justified relative to the longest string.
	 *
	 * Also, there's a gap vertically before we display the first error
	 * message string. Subsequent strings are butted up against the previous.
	 */

	xv_set(error_msgs[0],
		XV_X, (int) (panel_width - widest_error)/2,
		XV_Y, panel_item_y_gap,
		NULL);

	height_offset = error_msg_rects[0]->r_height;

	for (i=1; i < num_error_msgs; i++) {

		xv_set(error_msgs[i],
			XV_X, (int) (panel_width - widest_error)/2,
			XV_Y, panel_item_y_gap + height_offset,
			NULL);
		
		height_offset += error_msg_rects[i]->r_height;

	}

	/*
	 * Position hint strings above the buttons and below the messages.
	 * There will be a gap between the last message string and
	 * the first hint string.
	 */

#ifdef DEBUG
	fprintf(stderr, "hint[0] = '%s'\n", xv_get(hints[0], PANEL_LABEL_STRING));
	fprintf(stderr, "hint[0] being placed at %d x and %d y\n",
					(panel_width - widest_hint)/2,
					(2*panel_item_y_gap) + height_offset);
#endif

	xv_set(hints[0],
		XV_X, (int) (panel_width - widest_hint)/2,
		XV_Y, (2*panel_item_y_gap) + height_offset,
		NULL);

	height_offset += hint_rects[0]->r_height;

	for (i=1; i < num_hints; i++) {

		xv_set(hints[i],
			XV_X, (int) (panel_width - widest_hint)/2,
			XV_Y, (2*panel_item_y_gap) + height_offset,
			NULL);

		height_offset += hint_rects[i]->r_height;

	}

	/*
	 * Place the buttons beneath the message and center them.
	 */

	button_y_loc = (3*panel_item_y_gap) + height_offset; 

	button_x_gap = (int) (panel_width - ok_rect->r_width -
				cancel_rect->r_width - 20)/2;

#ifdef DEBUG
	fprintf(stderr, "putting ok at x=%d,  y=%d\n", button_x_gap, button_y_loc);
#endif

	xv_set(ok_button,
		XV_X, button_x_gap,
		XV_Y, button_y_loc,
		NULL);

#ifdef DEBUG
	fprintf(stderr, "putting cancel at x=%d,  y=%d\n", button_x_gap + ok_rect->r_width + 20, button_y_loc);
#endif

	xv_set(cancel_button,
		XV_X, button_x_gap + ok_rect->r_width + 20,
		XV_Y, button_y_loc,
		NULL);

	/*
	 * Shrink the frame to the panel's dimensions.
	 */

	window_fit(popup);

} /* end create_popup() */

/*
 * Function Name: create_notice()
 *
 * Globals Modified:
 *
 *   notice
 *
 * Return Code: None
 *
 * Description:
 *
 *   Create a locking notice popup with an "OK" button.
 *   No callback needed as only one response possible.
 */

static void
create_notice(){

#ifdef DEBUG
	fprintf(stderr, "create_notice: \n");
#endif

	notice = xv_create(popup, NOTICE,
				NOTICE_LOCK_SCREEN, TRUE,
				NULL);

} /* end create_notice() */

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
	fprintf(stderr, "get_display_info: \n");
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
	fprintf(stderr, "center_popup: \n");
#endif

	frame_get_rect(popup, &rect);

	rect.r_top = (int) (screen_height - rect.r_height)/2;
	rect.r_left = (int) (screen_width - rect.r_width)/2;

	frame_set_rect(popup, &rect);

} /* end center_popup() */

/*
 * Function Name: ok_to_check_media()
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
 *   we just call check_media() to do the real work.
 */

static void
ok_to_check_media(Panel_item item, Event *event)
{

#ifdef DEBUG
	fprintf(stderr, "ok_to_check_media: \n");
#endif

	check_media();

} /* end ok() */

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
 *   "Unpin" we just call check_media() to do the real work.
 */

static void
pushpin_out(Frame popup)
{

#ifdef DEBUG
	fprintf(stderr, "pushpin out: \n");
#endif

	check_media();

	/*
	 * If we ever get to this point it's because
	 * check_media encountered and error. We still
	 * need the popup pinned so we'll have to re-set the
	 * pushpin.
	 */

	xv_set(popup, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);

} /* end pushpin_out() */

/*
 * Function Name: cancel_io()
 *
 * Globals Modified: None
 *
 * Return Code: None
 *
 * Description:
 *
 *   This is the callback routine for the "Cancel" button.
 *   This routine does a fork/exec of the volcancel command
 *   and waits for it to finish. It then interprets the
 *   outcome of the volcancel and notifies the user if
 *   a problem occurred. If volcancel succeeded then
 *   call the quit routine to exit this program.
 */

static void
cancel_io(Panel_item item, Event *event)
{

	pid_t pid;		/* process id of child */
	int fd;			/* file descriptor */
	int exit_code;		/* volcancel exit code */
	char buf[BUFSIZ];	/* sufficiently large to hold variable length L10N messages */
	char *msg_format;	/* used for I18N messages */

#ifdef DEBUG
	fprintf(stderr, "cancel_io: \n");
#endif

	/*
	 * Call volcancel on the specified volume
	 * and then grab its return code.
	 * Use fork/exec to run volcancel.
	 */

	if ((pid = fork()) == 0) {

#ifdef DEBUG
		fprintf(stderr, "forking volcancel %s\n", vol_volumename);
#endif
		/* get rid of those nasty err messages */


#ifdef DEBUG
		fd = open("/dev/console", O_RDWR);
#else
		fd = open("/dev/null", O_RDWR);
#endif

		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		execl("/usr/lib/vold/volcancel", "volcancel", vol_volumename, NULL);
		perror("volmissing_popup: cancel_io: exec of volcancel failed");
		exit(EXECL_FAILED);

	}

	/*
	 * Parent - will wait for child (volcancel) to exit.
	 */

	while (wait(&exit_code) != pid)
		;

	if (WIFEXITED(exit_code) != 0) {

#ifdef DEBUG
	fprintf(stderr, "cancel_io: WIFEXITED successful\n");
#endif

		exit_code = WEXITSTATUS(exit_code);

	} else {

#ifdef DEBUG
	fprintf(stderr, "cancel_io: WIFEXITED failed\n");
#endif

		exit_code = VOLCANCEL_UNKNOWN_ERROR;
	}

#ifdef DEBUG
	fprintf(stderr, "cancel_io: volcancel exit_code after WEXITSTATUS = %d\n",
					exit_code);
#endif

	/*
	 * Interpret volcancel exit code. If volcancel worked exit quietly.
	 * Otherwise, notify user of volcancel status.
	 *
	 * Volcancel/Child return codes :
	 *
	 *  0			= succeeded (volume i/o cancelled)
	 *  VOLCANCEL_USAGE_ERROR = volcancel usage error
	 *  VOLMGT_NOT_RUNNING    = volume management not running
	 *  VOLCANCEL_OPEN_ERROR  = open error during cancel
	 *  VOLCANCEL_IOCTL_ERROR = ioctl error during cancel
	 *
	 *  EXECL_FAILED	= the execl failed for volcancel
	 */

	if (exit_code != 0) {

#ifdef DEBUG		
		fprintf(stderr, "cancel_io: volcancel had trouble\n");
#endif			


		/*
		 * Volcancel had trouble - notify user.
		 */

		switch (exit_code) {
		case VOLMGT_NOT_RUNNING:
			msg_format = gettext(NOT_RUNNING_MSG);
			sprintf(buf, msg_format);
			break;
		case VOLCANCEL_USAGE_ERROR:
			msg_format = gettext(VOLCANCEL_USAGE_MSG);
			sprintf(buf, msg_format);
			break;
		case VOLCANCEL_OPEN_ERROR:

			/* TRANSLATION_NOTE
	 		 * 
	 		 * The 1st %s in this string is the type of media (cdrom/floppy).
	 		 * The 2nd %s in this string is the label name of media (e.g., scratch).
	 		 */

			msg_format = gettext(VOLCANCEL_OPEN_MSG);
			sprintf(buf, msg_format, vol_mediatype, vol_volumename);
			break;
		case VOLCANCEL_IOCTL_ERROR:

			/* TRANSLATION_NOTE
	 		 * 
	 		 * The 1st %s in this string is the type of media (cdrom/floppy).
	 		 * The 2nd %s in this string is the label name of media (e.g., scratch).
	 		 */

			msg_format = gettext(VOLCANCEL_IOCTL_MSG);
			sprintf(buf, msg_format, vol_mediatype, vol_volumename);
			break;
		case EXECL_FAILED:
			msg_format = gettext(VOLCANCEL_EXECL_MSG);
			sprintf(buf, msg_format);
			break;
		default:
			msg_format = gettext(UNKNOWN_VOLCANCEL_EXIT);
			sprintf(buf, msg_format);
			break;
		}

		/*
		 * Display the notice which locks the screen.
		 * The only way the user can continue is to
		 * acknowledge by pressing the "OK" button.
		 * Given only one response, we don't need
		 * to check the return status from the notice.
		 */

		xv_set(notice, NOTICE_MESSAGE_STRING, buf,
			NOTICE_BUTTON_YES, gettext("OK"),
			NOTICE_FOCUS_XY, event_x(event), event_y(event),
			XV_SHOW, TRUE,
			NULL);

	} /* end if volcancel problem */

	/*
	 * regardless of the volcancel return code we must exit.
	 * If everything went ok then there's nothing more to
	 * do. If volcancel returned an error then we are hosethed
	 * and there's no point in having the volmissing_gui
	 * on the screen.
	 */

	quit();

} /* end cancel_io() */

/*
 * Function Name: eject_bogus_media()
 *
 * Globals Modified: ejectable_media_string
 *
 * Return Code: None 
 *
 * Description:
 *
 *   This routine is called when the user presses the eject
 *   button on the notice. It allows the user to easily eject
 *   media that was incorrectly inserted into a drive in 
 *   response to a volmissing event. The notice with the 
 *   "eject" button will only be visible as a result of the
 *   user putting in an incorrect volume. 
 * 
 *   The media that will be ejected is contained in the ejectable_media_string.
 *   It will be munged into a form that execv can handle and will be saved in
 *   ejectable_media.
 */

static void
eject_bogus_media()
{

	pid_t pid;		/* process id of child */
	int fd;			/* file descriptor */
	int exit_code;		/* execv'ed commands exit code */
	int i;			/* loop control */
	char ejectable_media_string_copy[MAXNAMLEN+1];	/* identify media needing ejection */
	char buf[BUFSIZ];	/* sufficiently large to hold variable length L10N messages */
	char *msg_format;	/* used to get pointer to L10N message */
	char *tokptr;		/* used to parse ejectable_media_string */
	char **ejectable_media;	/* used to build ejectable media names for execv */
	Rect *ok_rect;		/* holds OK button's geometry */

#ifdef DEBUG
	fprintf(stderr, "entering eject_bogus_media()\n");
#endif

	/*
	 * Count the number of tokens in the ejectable_media_string
	 * and add 2 (one for "eject" comamnd name and one for null byte).
	 * This will give us the correct number of char pointers to malloc
	 */

	 (void) strcpy(ejectable_media_string_copy, ejectable_media_string);

	i=0;
	tokptr = strtok(ejectable_media_string_copy, " ");
	while (tokptr != NULL) {
		i++;
		tokptr = strtok(NULL, " ");
	}

#ifdef DEBUG
	fprintf(stderr, "Found %d tokens malloc space for %d tokens\n", i, i+2);
#endif

        /*
	 * Malloc space for the ejectable_media vector array.
	 */

	ejectable_media = (char **) malloc(sizeof(char *) * (i+2));

	if (ejectable_media == NULL) {
		perror("volmissing_popup: eject_bogus_media: malloc failed");
		exit(-1);
	}

        /*
	 * Walk through the ejectable_media_string and pull each
	 * alias out and insert it into the vector array.
	 * Malloc space as we go.
	 * vector[0] is the "eject" command name
	 * and vector[n] is for the NULL terminator.
	 */

	ejectable_media[0] = (char *) malloc(strlen("eject") + 1);

	if (ejectable_media[0] == NULL) {
		perror("volmissing_popup: eject_bogus_media: malloc failed");
		exit(-1);
	}

	(void) strcpy(ejectable_media[0], "eject");

	i=1;
	tokptr = strtok(ejectable_media_string, " ");
	while (tokptr != NULL) {

		ejectable_media[i] = (char *) malloc(strlen(tokptr) + 1);

		if (ejectable_media[i] == NULL) {
			perror("volmissing_popup: eject_bogus_media: malloc failed");
			exit(-1);
		}

	 	(void) strcpy(ejectable_media[i], tokptr);

		tokptr = strtok(NULL, " ");
		i++;
	}

	ejectable_media[i] = NULL;

#ifdef DEBUG
	fprintf(stderr, "final dump of ejectable_media\n\n");
        for(i=0; ejectable_media[i] != NULL; i++) 
		fprintf(stderr, "ejectable_media[%d] = '%s'\n", i, ejectable_media[i]);
#endif

	/*
	 * Fork/exec the eject command on the ejectable_media
	 * string.
	 */

	if ((pid = fork()) == 0) {

#ifdef DEBUG
		fprintf(stderr, "forking eject %s\n", ejectable_media);
#endif

		/* get rid of those nasty err messages */
		fd = open("/dev/null", O_RDWR);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		execv("/bin/eject", ejectable_media);
		perror("volmissing_popup: eject_bogus_media: eject failed");
		exit(EXECL_FAILED);

	}

	/*
	 * Parent - will wait for child (eject) to exit.
	 */

	while (wait(&exit_code) != pid)
		;

	if (WIFEXITED(exit_code) != 0) {

#ifdef DEBUG
	fprintf(stderr, "eject_bogus_media: WIFEXITED successful\n");
#endif

		exit_code = WEXITSTATUS(exit_code);

	} else {

#ifdef DEBUG
	fprintf(stderr, "eject_bogus_media: WIFEXITED failed\n");
#endif

		exit_code = EJECT_UNKNOWN_ERROR;
	}

#ifdef DEBUG
	fprintf(stderr, "eject_bogus_media: eject exit_code = %d\n",
					exit_code);
#endif

	/*
	 * Interpret eject exit code. If eject worked we quietly continue.
	 * Otherwise, notify user of eject status, and quit.
	 *
	 * eject/Child return codes :
	 *
	 *  0			= eject succeeded
	 *  EJECT_FAILED	= eject failed
	 *  EJECT_USAGE_ERROR	= eject invalid args specified
	 *  EJECT_IOCTL_ERROR	= ioctl error during eject
	 *  EJECT_WORKED_X86	= eject succeeded on X86
	 *
	 *  EJECT_EXECL_FAILED	= the execl failed for eject
	 */

	if ((exit_code != 0) && (exit_code != EJECT_WORKED_X86)) {

#ifdef DEBUG
		fprintf(stderr, "eject_bogus_media: eject had trouble\n");
#endif			

		/* 
		 * Put the ejectable_media array into one string for messaging.
		 * Skip [0] as it hold the "eject" command string.
		 * The reason we have to do this is that strtok trashed 
		 * the old copy. So we better re-build it.
		 */

		ejectable_media_string_copy[0] = '\0';
		for (i=1; ejectable_media[i] != '\0'; i++) {
			strcat(ejectable_media_string_copy, ejectable_media[i]);
			strcat(ejectable_media_string_copy, " ");
		}

		/*
		 * Eject had trouble (maybe) - notify user.
		 */

		switch (exit_code) {
		case EJECT_FAILED:
			msg_format = gettext(EJECT_FAILED_MSG);
			sprintf(buf, msg_format);
			break;
		case EJECT_USAGE_ERROR:
			msg_format = gettext(EJECT_USAGE_MSG);
			sprintf(buf, msg_format);
			break;
		case EJECT_IOCTL_ERROR:

			/* TRANSLATION_NOTE
	 		 * 
	 		 * The %s in this string is the name(s) of the physical device(s)
	 		 * which can't be ejected. It's a comma delimted list in the case
	 		 * of multiple devices.
	 		 *
	 		 * For example: floppy1
	 		 *              cdrom1
	 		 *              floppy0, floppy1
	 		 */

			msg_format = gettext(EJECT_IOCTL_MSG);
			sprintf(buf, msg_format, ejectable_media_string_copy);
			break;
		case EXECL_FAILED:
			msg_format = gettext(EJECT_EXECL_MSG);
			sprintf(buf, msg_format);
			break;
		default:
			msg_format = gettext(UNKNOWN_EJECT_EXIT);
			sprintf(buf, msg_format);
			break;
		}

		/*
		 * Display the notice which locks the screen.
		 * The only way the user can continue is to
		 * acknowledge by pressing the "OK" button.
		 * Given only one response, we don't need
		 * to check the return status from the notice.
		 *
		 * Caveat: This notice will eminate from the
		 * OK button on the main volmissing popup. This
		 * is caused by the fact that when the notice
		 * with the "Eject" Button dissapears we are
		 * left with the main popup.
		 */

		ok_rect = (Rect *) xv_get(ok_button, PANEL_ITEM_RECT);

		xv_set(notice, NOTICE_MESSAGE_STRING, buf,
			NOTICE_BUTTON_YES, gettext("OK"),
			NOTICE_FOCUS_XY, ok_rect->r_left + 10, ok_rect->r_top + 5,
			XV_SHOW, TRUE,
			NULL);

		/* 
		 * We must quit this program. These are serious
		 * errors that require the system administrator to
		 * restart volmgt.
		 */

		quit();

	} /* end if eject problem */

        /*
	 * Free up space malloc'ed for ejectable_media arg vector
	 */

	for (i=0; ejectable_media[i] != '\0'; i++) {
		free(ejectable_media[i]);
	}

	free((char *) ejectable_media);

	/*
	 * Null out the global ejectable_media_string so we can start over
	 */

        ejectable_media_string[0] = '\0';

} /* end function eject_bogus_media() */

/*
 * Function Name: check_media()
 *
 * Globals Modified: None
 *
 * Return Code: None 
 *
 * Description:
 *
 *   This is the routine called by ok_to_check_media() and
 *   pushpin_out(). It calls volmgt_check() to see if the
 *   user has something being managed presently by volmgt.
 *   If we find something being managed we check to see
 *   it is the "missing" volume. If it is the "missing"
 *   volume we call quit() to exit this program. If the
 *   volume being managed is not the "missing" volume we
 *   notify the user and wait for them to acknowledge.
 *   If they put in a bogus volume we allow them to eject it.
 */

static void
check_media()
{

	extern	char *media_getattr();
	extern	int volmgt_check();

	Rect *ok_rect;		/* ok rectangle geometry */
	int exit_code;		/* volcheck exit code */
	int bogus_media = 0;	/* bogus media flag */
	char buf[BUFSIZ];	/* sufficiently large to hold variable length L10n messages */
	char *msg_format;	/* used to point to L10N message */
	char *label_string;	/* used to localize labels */
	char *attr_ptr = NULL;	/* points to media attribute */

	/*
	 * Take a snapshot of the /vol/dev/aliases directory
	 * before we do the volmgt_check. This way we'll be
	 * able to tell if the user puts in the wrong media
	 * in response to the popup. If so, we'll be able to
	 * eject the proper media.
	 */

	(void) alias_snapshot(INIT);

	/*
	 * Call volmgt_check. Pass in NULL to tell it
	 * to search through all managed media for something
	 * currently in a drive.
	 */

#ifdef DEBUG
	fprintf(stderr, "check_media: calling volmgt_check(NULL)\n");
#endif

	exit_code = volmgt_check(NULL);

#ifdef DEBUG
	fprintf(stderr, "volmgt_check returned %d\n", exit_code);
#endif

	/*
	 * Interpret volmgt_check exit code and
	 * make sure things went as expected.
	 *
	 * 0 = NO media being managed
	 * 1 = some media being managed (may not be right media)
	 */

	if (exit_code == 1) {

		/*
		 * Sneaky way to see if correct media was inserted.
		 * If it was inserted the vold daemon should have set the
		 * "s-location" attribute to its current location.
		 */

		sprintf(buf, "/vol/rdsk/%s", vol_volumename);
		attr_ptr = (char *) media_getattr(buf, "s-location");

		if (attr_ptr == NULL) {
#ifdef DEBUG
			fprintf(stderr, "desired media NOT present.\n");
#endif
			/*
			 * Determine if the user put in the wrong media
			 * and identify it so it can be ejected.
			 */

			if (alias_snapshot(FOLLOW_UP)) {

				/*
				 * something bogus - flag it so we
				 * can make the eject button visible later.
				 */

#ifdef DEBUG
				fprintf(stderr, "Wrong media inserted - eject it\n");
#endif

				bogus_media=1;
			}
			else {

				/*
				 * nothing bogus - our volume is missing
				 */

#ifdef DEBUG
				fprintf(stderr, "No bogus media - desired volume missing\n");
#endif

				bogus_media = 0;
			}

		} else {
#ifdef DEBUG
			fprintf(stderr, "desired media present.\n");
#endif
			free(attr_ptr); /* media_getattr malloc'ed it */
			quit();
		}

	}

#ifdef DEBUG
	fprintf(stderr, "check_media: media is not present\n");
#endif

	if ((exit_code == 0) || (exit_code = 1 && !bogus_media)) {

		/* TRANSLATION_NOTE
 		 * 
 		 * The 1st %s in this string is the type of media (cdrom/floppy).
 		 * The 2nd %s in this string is the label name of media (e.g., scratch).
 		 */

		msg_format = gettext(VOLUME_NOT_FOUND);
		sprintf(buf, msg_format, vol_mediatype, vol_volumename);
		label_string = gettext("OK");
	} else {

		/* TRANSLATION_NOTE
	 	 * 
	 	 * The %s in this string is the name(s) of the physical device(s)
	 	 * which can be ejected. It's a comma delimted list in the case
	 	 * of multiple devices.
	 	 *
	 	 * For example: floppy1
	 	 *              cdrom1
	 	 *              floppy0, floppy1
	 	 */

		msg_format = gettext(BOGUS_VOLUME_FOUND);
		sprintf(buf, msg_format, ejectable_media_string);
		label_string = gettext("Eject");
	}

	/*
	 * Display the notice which locks the screen.
	 * The only way the user can continue is to
	 * acknowledge by pressing the "OK" button.
	 * Given only one response, we don't need
	 * to check the return status from the notice.
	 * Want to make this notice eminate from the
	 * OK button on the popup (even if user caused
	 * this by pulling the pushpin). Get the new
	 * ok_rect as it may have been moved
	 * around the screen.
	 */

	ok_rect = (Rect *) xv_get(ok_button, PANEL_ITEM_RECT);

	xv_set(notice, NOTICE_MESSAGE_STRING, buf,
		NOTICE_BUTTON_YES, label_string,
		NOTICE_FOCUS_XY, ok_rect->r_left + 10, ok_rect->r_top + 5,
		XV_SHOW, TRUE,
		NULL);

	/*
	 * If the user needs to eject some bogus media we'll
	 * do it for them after they've acknowledged the notice.
	 */

	if (bogus_media)
		eject_bogus_media();
        
} /* end check_media() */

/*
 * Function Name: alias_snapshot()
 *
 * Globals Modified: ejectable_media_string
 *
 * Return Code: 0 = no new aliases since last time you checked.
 *              1 = new alias(es) appeared (returned via ejectable_media)
 *
 * Description:
 *
 *   This is the routine used to check /vol/dev/aliases and tell
 *   us if something new has been "managed" since the last time
 *   we checked. Ultimately this function will be used to identify
 *   which device contains a "bogus" floppy inserted by the user
 *   in response to a volmissing event.
 */

static int
alias_snapshot(int state)
{

	struct node {
		char name[MAXNAMLEN + 1];
		struct node *next;
	};

	int ret_val = 0;		/* function return code */
	int new = 1;			/* loop control */
	static int first_time = 1;	/* For static processing */
	DIR *alias_dir = NULL;		/* pointer to /vol/dev/aliases */
	static struct node *head;	/* head of list of files in dir */
	static struct node *tail;	/* tail of list of files in dir */
	struct node *dir_node = NULL;	/* pointer to node in list */
	struct node *tmp_node = NULL;	/* pointer to node in list */
	struct dirent *direntp;		/* pointer to a directory entry struct */

#ifdef DEBUG
	fprintf(stderr, "entering alias_snapshot : state = %d\n", state);
#endif

	if ((alias_dir = opendir("/vol/dev/aliases")) == NULL) {
		perror("volmissing_popup: alias_snapshot: opendir failed");
		exit(-1);
	}

	if (state == INIT) {

		if (first_time) {
			head = NULL;
			first_time--;
		}

		/*
		 * Remove old snapshot
		 */

		dir_node = head;
		while (dir_node != NULL) {
			tmp_node = dir_node->next;
			free(dir_node);
			dir_node = tmp_node;
		}

		head = NULL;

		/*
		 * Build an initial snapshot of the alias dir.
		 */

		while ((direntp = readdir(alias_dir)) != NULL) {
			if ((strcmp(direntp->d_name, ".") != 0) &&
			    (strcmp(direntp->d_name, "..") !=0 )) {

				dir_node = (struct node *)
						malloc(sizeof(struct node)); 
				if (dir_node == NULL) {
					perror("volmissing_popup: malloc failed");
					exit(-1);
				}

				strcpy(dir_node->name, direntp->d_name);
				dir_node->next = NULL;

				if (head == NULL) {
					head = dir_node;
				} else {
					tail->next = dir_node;
				}

				tail = dir_node;

			}
		}

		(void) closedir(alias_dir);

		ret_val = 0;

#ifdef DEBUG
		dir_node = head;
		fprintf(stderr, "Begin alias list\n\n");
		while (dir_node != NULL) {
			fprintf(stderr, "alias name = '%s'\n", dir_node->name);
			dir_node = dir_node->next;
		}
		fprintf(stderr, "\nEnd alias list\n\n");
#endif

	} else if (state == FOLLOW_UP) {

		ejectable_media_string[0] = '\0';

		/*
		 * Walk through the current alias directory files
		 * and see if they are in the initial snapshot.
		 * We'll only focus on the "problem" mediatype.
		 * If they aren't then concatenate it's name to
		 * the list of ejectable media.
		 */

		while ((direntp = readdir(alias_dir)) != NULL) {
			if ((strcmp(direntp->d_name, ".") != 0) &&
			    (strcmp(direntp->d_name, "..") != 0)) {

				if (strncmp(direntp->d_name, vol_mediatype,
					strlen(vol_mediatype)) == 0) {

					/*
					 * This has the desired mediatype.
					 * See if it's new.
					 */

					new = 1;
					dir_node = head;
					while (new && (dir_node != NULL)) {
						if (strcmp(dir_node->name,
							direntp->d_name) == 0)
							new--;

						dir_node = dir_node->next;
					}

					if (new) {

						/*
						 * Add new bogus volume to
						 * ejectable_media_string
						 */

						strcat(ejectable_media_string,
							direntp->d_name);
						strcat(ejectable_media_string,
							" ");

					}
				}

			}
		}

#ifdef DEBUG
	fprintf(stderr, "FOLLOW_UP: ejectable_media_string = '%s'\n",
			ejectable_media_string);
#endif

		/*
		 * Set up return code stuff.
		 */

		if (ejectable_media_string[0] != '\0')
			ret_val = 1;
		else
			ret_val = 0;

	} else {
		fprintf(stderr, "%s: alias_snapshot: unknown state\n",
				prog_name);
		exit(-1);
	}

	return (ret_val);

} /* end function alias_snapshot() */

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
	fprintf(stderr, "quit: \n");
	fprintf(stderr, "END volmissing_popup\n");
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
	 * The %s in this string is the name of this application.
	 */

	fprintf(stderr, gettext(USAGE), prog_name);

} /* end usage() */
