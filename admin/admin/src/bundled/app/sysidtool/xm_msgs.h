#ifndef XM_MSGS_H
#define	XM_MSGS_H

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

#pragma	ident	"@(#)xm_msgs.h 1.22 96/08/06"

#include "sysid_msgs.h"

/*
 * Intro
 */

/*
 * i18n:	The messages from here to the end of the
 *	button labels apply to the GUI only.
 */
#define	PARADE_INTRO_TEXT	dgettext(TEXT_DOMAIN, \
	"The Solaris installation program "\
	"is divided into a series of short sections "\
	"where you'll be prompted to provide "\
	"information for the installation. "\
	"At the end of each section, you'll be able to change "\
	"the selections you've made before continuing.")

#define	INTRO_TEXT		dgettext(TEXT_DOMAIN, \
	"On the next screens, you must identify this " \
	"system as networked or non-networked, and set " \
	"the default time zone and date/time.\n\n" \
	"If this system is networked, the software will try " \
	"to find the information it needs to identify the " \
	"system; you will be prompted to supply any " \
	"information it cannot find.\n\n" \
	"> To begin identifying the system, choose\n   Continue.")

/*
 * Locale
 */
#define	LOCALE_TEXT	dgettext(TEXT_DOMAIN, "Select Language and Locale")

/*
 * i18n: This string is be used to create a sentence such as
 * "Select this item for Spanish"
 */
#define	SELECT_THIS_ITEM	"Select this item for"

#define	SELECT_LOCALE_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you can select the language and locale you want " \
	"displayed for screens that follow, and on your desktop after you " \
	"reboot the system.  Selecting a locale determines how online " \
	"information is displayed for a specific language or region " \
	"(for example, time, dates, spellings, and monetary values).\n\n" \
	"> To accept the default language and locale (highlighted), press Continue.\n\n" \
	"> To change the default setting, select a language from the list on the left,\n"\
	"   then select a locale from the list on the right.")

#define	LOCALE_PROMPT		dgettext(TEXT_DOMAIN, \
	"Select this item for %s (%s)")
#define	SHORT_FORM_LOCALE_PROMPT		dgettext(TEXT_DOMAIN, \
	"%s (%s)")

/*
 * Network interface initialization
 */

#define	NETWORKED_TEXT	dgettext(TEXT_DOMAIN, \
	"On this screen you must specify whether this system is " \
	"connected to a network.  If you specify Yes, the system " \
	"should be connected to the network by an Ethernet or " \
	"similar network adapter.")

/*
 * Host name
 */

#define	HOSTNAME_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must enter a host name, which identifies " \
	"this system on the network.  The name must be unique within " \
	"the domain in which it resides; creating a duplicate host name " \
	"will cause problems on the network after you install Solaris.\n\n" \
	"A host name must be at least two characters; it can contain " \
	"letters, digits, and minus signs (-).")

/*
 * Primary network interface
 */

#define	PRIMARY_NET_TEXT	dgettext(TEXT_DOMAIN, \
	"On this screen you must specify which of the following network " \
	"adapters is the system's primary network interface.  Usually " \
	"the correct choice is the lowest number.  However, do not guess; " \
	"ask your system administrator if you're not sure.")

/*
 * IP address
 */

#define	HOSTIP_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must enter the Internet Protocol (IP) " \
	"address for this system.  It must be unique and follow your " \
	"site's address conventions, or a system/network failure could " \
	"result.\n\n" \
	"IP addresses contain four sets of numbers " \
	"separated by periods (for example 129.200.9.1).")

/*
 * Name service
 */

#define	NAME_SERVICE_TEXT	dgettext(TEXT_DOMAIN, \
	"On this screen you must provide name service information.\n\n" \
	"> Select NIS+ or NIS if this system is known to the name server; " \
	"Select Other if your site is using another name service (for " \
	"example, DCE or DNS); select None if your site is not using a name " \
	"service, or if it is not yet established.\n\n")

/*
 * Locate name server automatically?
 */

#define	BROADCAST_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify how to find a name server for " \
	"this system.  You can let the software try to find one, " \
	"or you can specify one.  The software can find a name server " \
	"only if it is on your local subnet.")

/*
 * Domain name
 */

#define	DOMAIN_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify the domain where this system " \
	"resides.  Make sure you enter the name correctly including " \
	"capitalization and punctuation.")

/*
 * Specify name server
 */

#define	NISSERVER_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must enter the host name and IP address of your " \
	"name server.  Host names must be at least two characters, and may " \
	"contain letters, digits, and minus signs (-).  IP addresses must " \
	"contain four sets of numbers separated by periods (for example " \
	"129.200.9.1).")

/*
 * Is network subnetted?
 */

#define	SUBNETTED_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify whether this system is part of " \
	"a subnet.  If you specify incorrectly, the system will have " \
	"problems communicating on the network after you reboot.")

/*
 * Subnet mask
 */

#define	NETMASK_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify the netmask of your subnet.  " \
	"A default netmask is shown; do not accept the default unless " \
	"you are sure it is correct for your subnet.  A netmask must " \
	"contain four sets of numbers separated by periods (for example " \
	"255.255.255.0).")

/*
 * Timezone (timezone menu)
 */

#define	TIMEZONE_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must select how to specify your default time " \
	"zone.\n\n" \
	"> Select one of the three methods and\n" \
	"   choose Set.")

#define	TIMEZONE_PROMPT		dgettext(TEXT_DOMAIN, "Specify timezone by:")

#define	TZ_BY_REGION		dgettext(TEXT_DOMAIN, "Geographic region")
#define	TZ_BY_GMT		dgettext(TEXT_DOMAIN, "Offset from GMT")
#define	TZ_BY_FILE		dgettext(TEXT_DOMAIN, "Time zone file")

/*
 * Timezone region (main menu)
 */

#define	TZ_REGION_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you can specify your default time zone by " \
	"geographic region.\n\n" \
	"> Select a region from the list on the left and\n" \
	"   a time zone from the list on the right.")

/*
 * Timezone (GMT offset)
 */

#define	TZ_GMT_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you must specify your default time zone as an " \
	"offset from Greenwich Mean Time.  If you are east of Greenwich, " \
	"England, specify a positive number 1 through 13; if you are west " \
	"of Greenwich, England, specify a negative number -12 through -1.\n\n" \
	"> To specify the hours offset from Greenwich\n" \
	"   Mean Time, move the slider.")

/*
 * Timezone (rules file)
 */

#define	TZ_FILE_TEXT		dgettext(TEXT_DOMAIN, \
	"On this screen you can specify your default time zone by " \
	"pointing to a file in the /usr/share/lib/zoneinfo directory.\n\n" \
	"> Specify the file name (path is not necessary),\n" \
	"   or choose Select for a list of files in the\n" \
	"   zoneinfo directory.")

/*
 * Date and time
 */

#define	DATE_AND_TIME_TEXT	dgettext(TEXT_DOMAIN, \
	"> Accept the default date and time or enter\n" \
	"   new values.")

/*
 * Confirmation
 */

#define	CONFIRM_TEXT		dgettext(TEXT_DOMAIN, \
	"> Confirm the following information.  If it is correct,\n" \
	"   choose Continue; to change any information\n" \
	"   choose Change.")

/*
 * Misc labels
 */

/*
 * i18n: label for GUI "Filter" label
 */
#define	FILTER_LABEL		dgettext(TEXT_DOMAIN, "Filter")

/*
 * i18n: label for GUI "Directories" label
 */
#define	DIR_LABEL		dgettext(TEXT_DOMAIN, "Directories")

/*
 * i18n: label for GUI "Files" label
 */
#define	FILES_LABEL		dgettext(TEXT_DOMAIN, "Files")

/*
 * i18n: label for GUI "SELECTION" label
 */
#define	SELECTION_LABEL		dgettext(TEXT_DOMAIN, "Selection")

/*
 * i18n: label for GUI "Continue" button
 */
#define	CONTINUE_BUTTON		dgettext(TEXT_DOMAIN, "Continue")

/*
 * i18n: label for GUI "OK" button
 */
#define	OK_BUTTON		dgettext(TEXT_DOMAIN, "OK")

/*
 * i18n: label for GUI "Change" button (confirmation screens)
 */
#define	CHANGE_BUTTON		dgettext(TEXT_DOMAIN, "Change")

/*
 * i18n: label for GUI "Cancel" button
 */
#define	CANCEL_BUTTON		dgettext(TEXT_DOMAIN, "Cancel")

/*
 * i18n: label for GUI "Help" button
 */
#define	HELP_BUTTON		dgettext(TEXT_DOMAIN, "Help")

/*
 * i18n: label for GUI "Default" button
 */
#define	DEFAULT_BUTTON		dgettext(TEXT_DOMAIN, "Default")

/*
 * i18n: label for GUI timezone "Set" button
 */
#define	SET_BUTTON		dgettext(TEXT_DOMAIN, "Set...")

/*
 * i18n: label for GUI "Filter" button
 */
#define	FILTER_BUTTON		dgettext(TEXT_DOMAIN, "Filter")

/*
 * i18n: label for timezone file selector button
 */
#define	SELECT_BUTTON		dgettext(TEXT_DOMAIN, "Select...")

/*
 * i18n: title for GUI "working" notice popups
 */
#define	WORKING_TITLE		dgettext(TEXT_DOMAIN, \
	"System Identification Status")

/*
 * Error messages
 */

/*
 * i18n: title for GUI validation error popups
 */
#define	VALIDATION_ERROR_TITLE	dgettext(TEXT_DOMAIN, "Validation Error")

#define	DISMISS_ERROR_TEXT	dgettext(TEXT_DOMAIN, \
	"You cannot exit this segment of the Solaris installation " \
	"program. You can change selections at confirmation screens, " \
	"or you can reboot the system and restart the Solaris " \
	"installation program. To reboot, power-cycle your system, " \
	"or use the appropriate keyboard sequence to abort your system.")

/*
 * i18n: on-line "howto" and "topics" for GUI host name screen
 */
#define	HOSTNAME_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the host name")
#define	HOSTNAME_TOPICS			dgettext(TEXT_DOMAIN, "hostname.help")

/*
 * i18n: "howto" and "topics" for GUI network connectivity screen
 */
#define	NETWORKED_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying network connectivity status")
#define	NETWORKED_TOPICS		dgettext(TEXT_DOMAIN, \
	"standalone.help")

/*
 * i18n: "howto" and "topics" for GUI primary network screen
 */
#define	NETIF_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the primary network interface")
#define	NETIF_TOPICS			dgettext(TEXT_DOMAIN, "netif.help")

/*
 * i18n: "howto" and "topics" for GUI IP address screen
 */
#define	HOSTIP_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the Internet Protocol (IP) address")
#define	HOSTIP_TOPICS			dgettext(TEXT_DOMAIN, "ipaddr.help")

/*
 * i18n: "howto" and "topics" for GUI name service screen
 */
#define	NS_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying a name service")
#define	NS_TOPICS			dgettext(TEXT_DOMAIN, \
	"name_service.help")

/*
 * i18n: "howto" and "topics" for GUI domain name screen
 */
#define	DOMAIN_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying a name service domain")
#define	DOMAIN_TOPICS			dgettext(TEXT_DOMAIN, "domain.help")

/*
 * i18n: "howto" and "topics" for GUI name server location screen
 */
#define	BROADCAST_HOWTO			dgettext(TEXT_DOMAIN, \
	"Locating a name service server")
#define	BROADCAST_TOPICS		dgettext(TEXT_DOMAIN, "location.help")

/*
 * i18n: "howto" and "topics" for GUI name server specification screen
 */
#define	SERVER_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying a name service server")
#define	SERVER_TOPICS			dgettext(TEXT_DOMAIN, "nsserver.help")

/*
 * i18n: "howto" and "topics" for GUI subnets screen
 */
#define	SUBNET_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying if the network is subnetted")
#define	SUBNET_TOPICS			dgettext(TEXT_DOMAIN, "subnet.help")

/*
 * i18n: "howto" and "topics" for GUI netmask screen
 */
#define	NETMASK_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the sub-network mask")
#define	NETMASK_TOPICS			dgettext(TEXT_DOMAIN, "netmask.help")

/*
 * i18n: "howto" and "topics" for GUI timezone (region/zone pair) screens
 */
#define	TZREGION_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the timezone")
#define	TZREGION_TOPICS			dgettext(TEXT_DOMAIN, "timezone.help")

/*
 * i18n: "howto" and "topics" for GUI timezone (rules file) screen
 */
#define	TZFILE_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the timezone")
#define	TZFILE_TOPICS			dgettext(TEXT_DOMAIN, "timezone.help")

/*
 * i18n: "howto" and "topics" for GUI timezone (gmt offset) screen
 */
#define	TZGMT_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the timezone")
#define	TZGMT_TOPICS			dgettext(TEXT_DOMAIN, "timezone.help")

/*
 * i18n: "howto" and "topics" for GUI date and time screen
 */
#define	DATE_HOWTO			dgettext(TEXT_DOMAIN, \
	"Specifying the date and time")
#define	DATE_TOPICS			dgettext(TEXT_DOMAIN, "date.help")

/*
 * i18n: "howto" and "topics" for GUI confirmation screens
 */
#define	CONFIRM_HOWTO			dgettext(TEXT_DOMAIN, \
	"Confirming information")
#define	CONFIRM_TOPICS			dgettext(TEXT_DOMAIN, "confirm.help")

/*
 * i18n: "howto" and "topics" for GUI intro screen and default help
 */
#define	NAV_HOWTO			dgettext(TEXT_DOMAIN, \
	"General navigational help")
#define	NAV_TOPICS			dgettext(TEXT_DOMAIN, "navigate.help")

/*
 * i18n: "reference" for GUI on-line help; LAST GUI-SPECIFIC MESSAGE
 */
#define	GLOSSARY			dgettext(TEXT_DOMAIN, "Glossary")

#endif	XM_MSGS_H
