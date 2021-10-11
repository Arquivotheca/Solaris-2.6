/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 * doconfig.c: Control routine for kdmconfig -c option
 *
 * Description:
 *
 *  This routine controls the flow for the configuration option
 *  of kdmconfig. It invokes the exists, the silent and possibly the
 *  interactive routines for each device, then possibly presents the
 *  confirmation screen before commiting the changes.
 *
 */

#pragma ident "@(#)doconfig.c 1.16 94/08/10 SMI"


#include "kdmconfig.h"
#include "ui.h"
#include "silent.h"
#include "exists.h"
#include "except.h"
#include "bootparam.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "util.h"

/*
 * UI loop states
 */
#define	DO_INTRO	0
#define	DO_DISPLAY	1
#define	DO_KEYBOARD	2
#define	DO_POINTER	3
#define	DO_CONFIRM	4
#define DO_MONITOR	5

#define OWCONFIG_FILE "/etc/openwin/server/etc/OWconfig"
static int
OWconfig_exists()
{
struct stat st;

	if ((stat(OWCONFIG_FILE, &st) == -1) &&
	    (errno == ENOENT)) 
		return(0);
	else
		return(1);
}

static void
config_state_machine(int do_confirm)
{
	int state = DO_KEYBOARD;
	int direction = NAV_FORWARD;
	int confirm_ok = CONFIRM_NO;

	if (do_confirm == CONFIRM_NONE)
		return;

	/*
	 * Loop through until all interactive selections are entered
	 * and confirmed.
	 */

	set_except_mode(INTERACTIVE_MODE);

	if (!get_server_mode())
		ui_wintro(); /* what a hack! */

	while (confirm_ok == CONFIRM_NO) {
		int get_mon = FALSE;

		switch (state) {
		case DO_KEYBOARD:
			if ((do_confirm & CONFIRM_KBD) != CONFIRM_NONE)
				get_keyboard_interactive(direction);
			/* FALLTHROUGH */
		case DO_INTRO:
			if (!get_server_mode()) {
				if (((do_confirm & CONFIRM_DIS) != CONFIRM_NONE) || 
			    	((do_confirm & CONFIRM_PTR) != CONFIRM_NONE))
					direction = ui_intro();
				/* Used to transmit bypass key */
				if (direction == NAV_BACKWARD) {
					state = DO_CONFIRM;
					do_confirm = CONFIRM_KBD; /* Opted to skip others */
					break;
				}
				direction = NAV_FORWARD;
			}
			/* FALLTHROUGH */
		case DO_DISPLAY:
			if ((do_confirm & CONFIRM_DIS) != CONFIRM_NONE)
				direction = get_display_interactive(direction);
			if (direction == NAV_BACKWARD) {
				state = DO_KEYBOARD; /* Skip to before intro */
				break;
			}
			if ( !get_xin())
				goto get_pointer;
			/* FALLTHROUGH */

                case DO_MONITOR:
                        if ((do_confirm & CONFIRM_DIS) != CONFIRM_NONE)
                                direction = get_monitor_interactive(direction);
                        if (direction == NAV_BACKWARD) {
                                state = DO_DISPLAY; /* Skip to before intro */
                                break;
                        }
                        /* FALLTHROUGH */
get_pointer:
		case DO_POINTER:
			if ((do_confirm & CONFIRM_PTR) != CONFIRM_NONE)
				direction = get_pointer_interactive(direction);
			if (direction == NAV_BACKWARD) {
				state = DO_DISPLAY;
				break;
			}
			/* FALLTHROUGH */
		case DO_CONFIRM:
			if (do_confirm != CONFIRM_NONE)
				confirm_ok = get_config_confirm(do_confirm);
			direction = NAV_FORWARD;
			state = DO_KEYBOARD;
			break;
		}
	}

}

static int
do_silent()
{
	int do_confirm = CONFIRM_NONE;
	set_except_mode(SILENT_MODE);

	/* First go for the silent mode */
	verb_msg("%s ...", KDMCONFIG_MSGS(KDMCONFIG_GET_DIS));
	if (force_prompt ||
	    ((!display_exists) && (get_display_silent() != 0)))  {
		do_confirm |= CONFIRM_DIS;
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_NOT_FOUND));
	}
	else
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_FOUND));

	/* only get the monitor info if the display card was an xinside dev */
	if( get_xin_silent() ){
        	verb_msg("%s ...", KDMCONFIG_MSGS(KDMCONFIG_GET_MON));
        	if (force_prompt ||
            		((!monitor_exists) && (get_monitor_silent() != 0)))  {
                		do_confirm |= CONFIRM_MON;
                		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_NOT_FOUND));
        	}
        	else
                	verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_FOUND));
	}

	verb_msg("%s ...", KDMCONFIG_MSGS(KDMCONFIG_GET_PTR));
	if (force_prompt ||
	    ((!pointer_exists) && (get_pointer_silent() != 0)))  {
		do_confirm |= CONFIRM_PTR;
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_NOT_FOUND));
	}
	else
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_FOUND));

	verb_msg("%s ...", KDMCONFIG_MSGS(KDMCONFIG_GET_KBD));
	if (force_prompt ||
	    ((!keyboard_exists) && (get_keyboard_silent() != 0)))  {
		do_confirm |= CONFIRM_KBD;
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_NOT_FOUND));
	}
	else
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_FOUND));

	return (do_confirm);
}

void
do_config(void)
{
	int do_confirm;

	/* Initialize subsystems */
	ui_init();
	dvc_init();
	
	if ((!get_server_mode()) && (OWconfig_exists())) return;

	if (get_server_mode()) check_nsswitch();

	do_confirm = ( get_server_mode() ? 
		         bootparam_get(get_client()) : do_silent() );

	config_state_machine(do_confirm);
	/*
	 * Commit all those selections.
	 */
	(get_server_mode() ? 
		bootparam_commit(get_client()) : dvc_commit());

	return;
}
