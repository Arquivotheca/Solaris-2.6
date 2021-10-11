#ifndef	TTY_MSGS_H
#define	TTY_MSGS_H

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * This file contains text strings specific to the tty interface.
 *
 * The strings are organized by screen.  Each screen is responsible
 * for entry or confirmation of one or more attributes, and typically
 * has strings of the following types:
 *
 *	*_TITLE		The title at the top of the screen
 *	*_TEXT		The summary text for the screen
 *	*_PROMPT	The prompt string for an attribute
 *	*_CONFIRM	The name of the attribute for confirmation screens
 *
 * In addition, attributes with multiple choices (menus) will have
 * a string per choice.  If the menu and confirmation strings for
 * these choices are different, the will be differentiated as
 * *_PROMPT_* and *_CONFIRM_*.
 */

#pragma	ident	"@(#)tty_msgs.h 1.23 96/10/17"

#include "sysid_msgs.h"

/*
 * Intro
 */

/*
 * i18n:	The messages between this comment and the "last message"
 *	comment apply to the tty interface (CUI) only.
 */
#define	PARADE_INTRO_TEXT	dgettext(TEXT_DOMAIN, \
	"The Solaris installation program "\
	"is divided into a series of short sections "\
	"where you'll be prompted to provide "\
	"information for the installation. "\
	"At the end of each section, you'll be able to change "\
	"the selections you've made before continuing."\
	"\n\n" \
	"About navigation...\n" \
	"\t- The mouse cannot be used\n" \
	"\t- If your keyboard does not have function keys, or they do not " \
	"\n" \
	"\t  respond, press ESC; the legend at the bottom of the screen " \
	"\n" \
	"\t  will change to show the ESC keys to use for navigation.")

#define	INTRO_TEXT		dgettext(TEXT_DOMAIN, \
	"On the next screens, you must identify this " \
	"system as networked or non-networked, and set " \
	"the default time zone and date/time.\n\n" \
	"If this system is networked, the software will try " \
	"to find the information it needs to identify your " \
	"system; you will be prompted to supply any " \
	"information it cannot find.\n\n" \
	"> To begin identifying this system, press F2.")

/*
 * Locale
 */

#define	LOCALE_PROMPT		dgettext(TEXT_DOMAIN, \
	"%2d) Enter %d for English")
/*
 * i18n: This string is be used to create a sentence such as
 * "0) Enter 0 for French"
 */
#define	ENTER_THIS_ITEM		"%2d) Enter %d for"

#define	ENTER_A_NUMBER	"Enter a number and press Return: "

#define	SELECT_LANG_TITLE	"Select A Language"

#define	SELECT_LOCALE_TITLE	dgettext(TEXT_DOMAIN, \
					"Select A Locale")

#define	SELECT_LOCALE_TEXT	dgettext(TEXT_DOMAIN, \
	"On this screen you can select the locale you want displayed\n" \
	"for the screens that follow. The locale you select will also\n" \
	"display on your desktop after you reboot the system.  Selecting\n" \
	"a locale determines how online information is displayed for a\n" \
	"specific locale or region (for example, time, date, spelling,\n" \
	"and monetary value).")

#define	RETURN_TO_PREVIOUS	dgettext(TEXT_DOMAIN, \
					"%2d) Return to Previous Screen")

/*
 * Terminal type
 */

#define	TERMINAL_TEXT	dgettext(TEXT_DOMAIN, \
	"What type of terminal are you using?\n")

#define	TELEVIDEO_925		dgettext(TEXT_DOMAIN, "Televideo 925")
#define	TELEVIDEO_910		dgettext(TEXT_DOMAIN, "Televideo 910")
#define	WYSE_50			dgettext(TEXT_DOMAIN, "Wyse Model 50")
#define	SUN_WORKSTATION		dgettext(TEXT_DOMAIN, "Sun Workstation")
#define	SUN_CMDTOOL		dgettext(TEXT_DOMAIN, "Sun Command Tool")
#define	SUN_PC			dgettext(TEXT_DOMAIN, "PC Console")
#define	ADM31			dgettext(TEXT_DOMAIN, "Lear Siegler ADM31")
#define	H19			dgettext(TEXT_DOMAIN, "Heathkit 19")
#define	VT100			dgettext(TEXT_DOMAIN, "DEC VT100")
#define	VT52			dgettext(TEXT_DOMAIN, "DEC VT52")
#define	ANSI			dgettext(TEXT_DOMAIN, "ANSI Standard CRT")
#define	XTERM			dgettext(TEXT_DOMAIN, \
	"X Terminal Emulator (xterms)")
#define	OTHER_TERMINAL		dgettext(TEXT_DOMAIN, "Other\n")

#define	TERMINAL_PROMPT		dgettext(TEXT_DOMAIN, \
	"Type the number of your choice and press Return: ")

#define	TERMINAL_NOT_FOUND	dgettext(TEXT_DOMAIN, \
	"\nThe specified terminal type is not found " \
	"in /usr/share/lib/terminfo.\n\n")

#define	TERMINAL_NONE		dgettext(TEXT_DOMAIN, \
	"You did not enter a terminal type.\n\n")

#define	TERMINAL_OTHER_TEXT	dgettext(TEXT_DOMAIN, \
	"\n> Specify a valid terminal type exactly as it is listed " \
	"in the terminfo\n  database, including capitalization and " \
	"punctuation.\n\n")

#define	TERMINAL_OTHER_PROMPT	dgettext(TEXT_DOMAIN, "Terminal type: ")

/*
 * Network interface initialization
 */

#define	NETWORKED_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify whether this system is " \
	"connected to a network.  If you specify Yes, the system " \
	"should be connected to the network by an Ethernet or " \
	"similar network adapter.\n\n" \
	"> To make a selection, use the arrow keys to highlight the " \
	"option and\n  press Return to mark it [X].\n\n")

/*
 * Host name
 */

#define	HOSTNAME_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must enter your host name, which identifies " \
	"this system on the network.  The name must be unique within " \
	"your domain; creating a duplicate host name will cause problems " \
	"on the network after you install Solaris.\n\n" \
	"A host name must be at least two characters; it can contain " \
	"letters, digits, and minus signs (-).\n\n")

/*
 * Primary network interface
 */

#define	PRIMARY_NET_TEXT	dgettext(TEXT_DOMAIN, \
	"On this screen you must specify which of the following network " \
	"adapters is the system's primary network interface.  Usually " \
	"the correct choice is the lowest number.  However, do not guess; " \
	"ask your system administrator if you're not sure.\n\n" \
	"> To make a selection, use the arrow keys to highlight the " \
	"option and\n  press Return to mark it [X].\n\n")

/*
 * IP address
 */

#define	HOSTIP_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must enter the Internet Protocol (IP) " \
	"address for this system.  It must be unique and follow your " \
	"site's address conventions, or a system/network failure could " \
	"result.\n\n" \
	"IP addresses contain four sets of numbers " \
	"separated by periods (for example 129.200.9.1).\n\n")

/*
 * Name service
 */

#define	NAME_SERVICE_TEXT	dgettext(TEXT_DOMAIN, \
	"On this screen you must provide name service information.  " \
	"Select NIS+ or NIS if this system is known to the name server; " \
	"select Other if your site is using another name service (for " \
	"example, DCE or DNS); select None if your site is not using a name " \
	"service, or if it is not yet established.\n\n" \
	"> To make a selection, use the arrow keys to highlight the option \n "\
	" and press Return to mark it [X].\n\n")

/*
 * Locate name server automatically?
 */

#define	BROADCAST_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify how to find a name server for " \
	"this system.  You can let the software try to find one, " \
	"or you can specify one.  The software can find a name server " \
	"only if it is on your local subnet.\n\n" \
	"> To make a selection, use the arrow keys to highlight the " \
	"option and\n  press Return to mark it [X].\n\n")

/*
 * Domain name
 */

#define	DOMAIN_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify the domain where this system " \
	"resides.  Make sure you enter the name correctly including " \
	"capitalization and punctuation.\n\n")

/*
 * Specify name server
 */

#define	NISSERVER_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must enter the host name and IP address of your " \
	"name server.  Host names must be at least two characters, and may " \
	"contain letters, digits, and minus signs (-).  IP addresses must " \
	"contain four sets of numbers separated by periods (for example " \
	"129.200.9.1).\n\n")

/*
 * Is network subnetted?
 */

#define	SUBNETTED_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify whether this system is part of " \
	"a subnet.  If you specify incorrectly, the system will have " \
	"problems communicating on the network after you reboot.\n\n" \
	"> To make a selection, use the arrow keys to highlight the " \
	"option and\n  press Return to mark it [X].\n\n")

/*
 * Subnet mask
 */

#define	NETMASK_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify the netmask of your subnet.  " \
	"A default netmask is shown; do not accept the default unless " \
	"you are sure it is correct for your subnet.  A netmask must " \
	"contain four sets of numbers separated by periods (for example " \
	"255.255.255.0).\n\n")

/*
 * Timezone main menu (regions)
 */

#define	TIMEZONE_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify your default time zone.  " \
	"You can specify a time zone in three ways:  select " \
	"one of the geographic regions from the list, select other - offset " \
	"from GMT, or other - specify time zone file.\n\n" \
	"> To make a selection, use the arrow keys to highlight the " \
	"option and\n  press Return to mark it [X].\n\n")

/*
 * Timezone (timezone menu)
 */

#define	TZ_INDEX_TEXT		dgettext(TEXT_DOMAIN, \
	"> To make a selection, use the arrow keys to highlight " \
	"the option and\n  press Return to mark it [X].\n\n")

/*
 * Timezone (GMT offset)
 */

#define	TZ_GMT_TEXT		dgettext(TEXT_DOMAIN, \
	"Specify the number of hours of difference between Greenwich " \
	"Mean Time (also called Coordinated Universal Time) and your " \
	"time zone.  If you are east of Greenwich, England, enter " \
	"a positive number 1 through 13; if you are west of Greenwich, " \
	"England enter a negative number -12 through -1.\n\n")

/*
 * Timezone (rules file)
 */

#define	TZ_FILE_TEXT		dgettext(TEXT_DOMAIN, \
	"Specify the time zone file you want to use.  The file must already " \
	"be in the /usr/share/lib/zoneinfo directory.  You do not need " \
	"to enter the full path.\n\n")

/*
 * Date and time
 */

#define	DATE_AND_TIME_TEXT	dgettext(TEXT_DOMAIN, \
	"> Accept the default date and time or enter\n" \
	"  new values.")

/*
 * Confirmation
 */

#define	CONFIRM_TEXT		dgettext(TEXT_DOMAIN, \
	"> Confirm the following information.  If it is correct, press F2;\n" \
	"  to change any information, press F4.\n\n")

/*
 * Root password
 */

#define	PASSWORD_INSTRUCTION	dgettext(TEXT_DOMAIN, \
	"On this screen you can create a root password.\n\n" \
	"A root password can contain any number of characters, but only " \
	"the first eight characters in the password are significant. " \
	"(For example, if you create `a1b2c3d4e5f6' as your root password, " \
	"you can use `a1b2c3d4' to gain root access.)\n\n" \
	"You will be prompted to type the root password twice; " \
	"for security, the password will not be displayed on the " \
	"screen as you type it.\n\n" \
	"> If you do not want a root password, press RETURN twice.\n\n")

#define	PASSWORD_PROMPT		dgettext(TEXT_DOMAIN, "Root password: ")
#define	PW_REENTER		dgettext(TEXT_DOMAIN, \
	"Re-enter your root password.")
#define	PW_TOO_LONG		dgettext(TEXT_DOMAIN, \
	"The password you entered is too long.  Enter " \
	"a shorter password.  Remember, only the first eight " \
	"characters in your password are significant.")
#define	PW_MISMATCH		dgettext(TEXT_DOMAIN, \
	"Your password entries do not match.  Try again.")
#define	KY_REENTER		dgettext(TEXT_DOMAIN, \
	"Please re-enter your password.")

/*
 * "generic" messages
 */

#define	CONTINUE_TO_ADVANCE		dgettext(TEXT_DOMAIN, \
	"> Press F2 to go to the next screen.")

#define	DISMISS_TO_ADVANCE		dgettext(TEXT_DOMAIN, \
	"> Press F2 to dismiss this message.")

#define	PRESS_RETURN_TO_CONTINUE	dgettext(TEXT_DOMAIN, \
	"Press Return to continue.")

/*
 * i18n: on-line "howto" and "topics" for tty host name screen
 */
#define	HOSTNAME_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the host name")
#define	HOSTNAME_TOPICS			dgettext(TEXT_DOMAIN, "Host Name")

/*
 * i18n: "howto" and "topics" for tty network connectivity screen
 */
#define	NETWORKED_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying network connectivity status")
#define	NETWORKED_TOPICS		dgettext(TEXT_DOMAIN, \
	"Network Connectivity")

/*
 * i18n: "howto" and "topics" for tty primary network screen
 */
#define	NETIF_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the primary network interface")
#define	NETIF_TOPICS			dgettext(TEXT_DOMAIN, \
	"Primary Network Interface")

/*
 * i18n: "howto" and "topics" for tty IP address screen
 */
#define	HOSTIP_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the Internet Protocol (IP) address")
#define	HOSTIP_TOPICS			dgettext(TEXT_DOMAIN, \
	"IP Address")

/*
 * i18n: "howto" and "topics" for tty name service screen
 */
#define	NS_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying a name service")
#define	NS_TOPICS			dgettext(TEXT_DOMAIN, \
	"Name Service")

/*
 * i18n: "howto" and "topics" for tty domain name screen
 */
#define	DOMAIN_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying a name service domain")
#define	DOMAIN_TOPICS			dgettext(TEXT_DOMAIN, "Domain Name")

/*
 * i18n: "howto" and "topics" for tty name server location screen
 */
#define	BROADCAST_HOWTO			dgettext(TEXT_DOMAIN, \
	"Locating a name service server")
#define	BROADCAST_TOPICS		dgettext(TEXT_DOMAIN, \
	"Locating Servers")

/*
 * i18n: "howto" and "topics" for tty name server specification screen
 */
#define	SERVER_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying a name service server")
#define	SERVER_TOPICS			dgettext(TEXT_DOMAIN, \
	"Name Servers")

/*
 * i18n: "howto" and "topics" for tty subnets screen
 */
#define	SUBNET_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying if the network is subnetted")
#define	SUBNET_TOPICS			dgettext(TEXT_DOMAIN, "Subnets")

/*
 * i18n: "howto" and "topics" for tty netmask screen
 */
#define	NETMASK_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the netmask")
#define	NETMASK_TOPICS			dgettext(TEXT_DOMAIN, "Netmask")

/*
 * i18n: "howto" and "topics" for tty timezone (region/zone pair) screens
 */
#define	TZREGION_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the timezone")
#define	TZREGION_TOPICS			dgettext(TEXT_DOMAIN, "Time Zone")

/*
 * i18n: "howto" and "topics" for tty timezone (rules file) screen
 */
#define	TZFILE_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the timezone")
#define	TZFILE_TOPICS			dgettext(TEXT_DOMAIN, "Time Zone")

/*
 * i18n: "howto" and "topics" for tty timezone (gmt offset) screen
 */
#define	TZGMT_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the timezone")
#define	TZGMT_TOPICS			dgettext(TEXT_DOMAIN, "Time Zone")

/*
 * i18n: "howto" and "topics" for tty date and time screen
 */
#define	DATE_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the date and time")
#define	DATE_TOPICS			dgettext(TEXT_DOMAIN, "Date and Time")

/*
 * i18n: "howto" and "topics" for tty confirmation screens
 */
#define	CONFIRM_HOWTO			dgettext(TEXT_DOMAIN, \
	"Confirming information")
#define	CONFIRM_TOPICS			dgettext(TEXT_DOMAIN, \
	"Confirm Information")

/*
 * i18n: "howto" and "topics" for tty intro screen and default help
 */
#define	NAV_HOWTO			dgettext(TEXT_DOMAIN, \
	"General navigational help")
#define	NAV_TOPICS			dgettext(TEXT_DOMAIN, \
	"Navigation")

/*
 * i18n: "reference" for tty on-line help; LAST TTY-SPECIFIC MESSAGE
 */
#define	GLOSSARY			dgettext(TEXT_DOMAIN, "Glossary")

#endif	TTY_MSGS_H
