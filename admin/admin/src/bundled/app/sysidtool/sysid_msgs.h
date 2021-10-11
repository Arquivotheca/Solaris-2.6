#ifndef	SYSID_MSGS_H
#define	SYSID_MSGS_H

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * This file contains text strings generic to the sysidtool programs.
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

#pragma	ident	"@(#)sysid_msgs.h 1.25 96/02/05"

#include <libintl.h>

/*
 * Intro
 */
#define	PARADE_INTRO_TITLE	dgettext(TEXT_DOMAIN, \
	"The Solaris Installation Program")
#define	INTRO_TITLE		dgettext(TEXT_DOMAIN, "Identify This System")

/*
 * Locale
 */
/*
 * i18n: Do not translate this message.  Instead, in place of the word
 * i18n: English, use the english name of the language you are translating
 * i18n: to.  For example, if you are translating to French, the msgstr field
 * i18n: should contain the english word French.
 */
#define	LANG_IN_ENGLISH		dgettext(TEXT_DOMAIN, "English")
#define LANGUAGE_LABEL		dgettext(TEXT_DOMAIN, "Languages")
#define LOCALE_LABEL		dgettext(TEXT_DOMAIN, "Locales")

/*
 * Generic validation errors
 */
#define	NOT_A_DIGIT		dgettext(TEXT_DOMAIN, \
	"You entered a character that is not a digit.")

#define	NOTHING_ENTERED		dgettext(TEXT_DOMAIN, \
	"You did not enter a value.")

#define	MIN_VALUE_EXCEEDED	dgettext(TEXT_DOMAIN, \
	"The value you entered is too low.")

#define	MAX_VALUE_EXCEEDED	dgettext(TEXT_DOMAIN, \
	"The value you entered is too high.")

#define	NOTHING_SELECTED	dgettext(TEXT_DOMAIN, \
	"You did not enter a selection.")

#define	INVALID_NUMBER		dgettext(TEXT_DOMAIN, \
	"Invalid selection.")

/*
 * Host name
 */
#define	HOSTNAME_TITLE		dgettext(TEXT_DOMAIN, "Host Name")

#define	HOSTNAME_PROMPT		dgettext(TEXT_DOMAIN, "Host name:")

#define	HOSTNAME_CONFIRM	HOSTNAME_PROMPT

/*
 * Network interface initialization
 */

#define	NETWORKED_TITLE		dgettext(TEXT_DOMAIN, "Network Connectivity")

#define	NETWORKED_PROMPT	dgettext(TEXT_DOMAIN, "Networked:")

#define	NETWORKED_CONFIRM	NETWORKED_PROMPT

#define	NETWORKED_NO	dgettext(TEXT_DOMAIN, "No")
#define	NETWORKED_YES	dgettext(TEXT_DOMAIN, "Yes")

/*
 * Primary network interface
 */

#define	PRIMARY_NET_TITLE	dgettext(TEXT_DOMAIN, \
	"Primary Network Interface")

#define	PRIMARY_NET_PROMPT	dgettext(TEXT_DOMAIN, "Network interfaces:")

#define	PRIMARY_NET_CONFIRM	dgettext(TEXT_DOMAIN, \
	"Primary network interface:")

/*
 * IP address
 */

#define	HOSTIP_TITLE		dgettext(TEXT_DOMAIN, "IP Address")

#define	HOSTIP_PROMPT		dgettext(TEXT_DOMAIN, "IP address:")

#define	HOSTIP_CONFIRM		HOSTIP_PROMPT


/*
 * Name service
 */

#define	NAME_SERVICE_TITLE	dgettext(TEXT_DOMAIN, "Name Service")

#define	NAME_SERVICE_PROMPT	dgettext(TEXT_DOMAIN, "Name service:")

#define	NAME_SERVICE_CONFIRM	NAME_SERVICE_PROMPT

#define	NIS_VERSION_3		dgettext(TEXT_DOMAIN, "NIS+")
#define	NIS_VERSION_2		dgettext(TEXT_DOMAIN, "NIS (formerly yp)")
#define	OTHER_NAMING_SERVICE	dgettext(TEXT_DOMAIN, "Other")
#define	NO_NAMING_SERVICE	dgettext(TEXT_DOMAIN, "None")

/*
 * Bad name service
 */

#define	BAD_NIS_TITLE		dgettext(TEXT_DOMAIN, "Name Service Error")

#define	BAD_NIS_PROMPT		dgettext(TEXT_DOMAIN, \
	"Enter new name service information?")

/*
 * Locate name server automatically?
 */

#define	BROADCAST_TITLE		dgettext(TEXT_DOMAIN, "Name Server")

#define	BROADCAST_PROMPT	dgettext(TEXT_DOMAIN, "Name server:")

#define	BROADCAST_CONFIRM	BROADCAST_PROMPT

#define	BROADCAST		dgettext(TEXT_DOMAIN, "Find one")
#define	SPECNAME		dgettext(TEXT_DOMAIN, "Specify one")

#define	BROADCAST_S		BROADCAST
#define	SPECNAME_S		SPECNAME

/*
 * Domain name
 */

#define	DOMAIN_TITLE		dgettext(TEXT_DOMAIN, "Domain Name")

#define	DOMAIN_PROMPT		dgettext(TEXT_DOMAIN, "Domain name:")

#define	DOMAIN_CONFIRM		DOMAIN_PROMPT

/*
 * Specify name server
 */

#define	NISSERVER_TITLE		dgettext(TEXT_DOMAIN, "Name Server Information")

#define	NISSERVERNAME_PROMPT	dgettext(TEXT_DOMAIN, "Server's host name:")

#define	NISSERVERADDR_PROMPT	dgettext(TEXT_DOMAIN, "Server's IP address:")

#define	NISSERVERNAME_CONFIRM	NISSERVERNAME_PROMPT

#define	NISSERVERADDR_CONFIRM	NISSERVERADDR_PROMPT

/*
 * Is network subnetted?
 */

#define	SUBNETTED_TITLE		dgettext(TEXT_DOMAIN, "Subnets")

#define	SUBNETTED_PROMPT	dgettext(TEXT_DOMAIN, \
	"System part of a subnet:")

#define	SUBNETTED_CONFIRM	SUBNETTED_PROMPT

#define	SUBNETTED_YES		dgettext(TEXT_DOMAIN, "Yes")
#define	SUBNETTED_NO		dgettext(TEXT_DOMAIN, "No")

/*
 * Subnet mask
 */

#define	NETMASK_TITLE		dgettext(TEXT_DOMAIN, "Netmask")

#define	NETMASK_PROMPT		dgettext(TEXT_DOMAIN, "Netmask:")

#define	NETMASK_CONFIRM		NETMASK_PROMPT

/*
 * Timezone
 */

#define	TIMEZONE_TITLE		dgettext(TEXT_DOMAIN, "Time Zone")

#define	TIMEZONE_CONFIRM	dgettext(TEXT_DOMAIN, "Time zone:")

/*
 * Timezone region
 */

#define	TZ_REGION_TITLE		dgettext(TEXT_DOMAIN, "Geographic Region")

#define	TZ_REGION_PROMPT	dgettext(TEXT_DOMAIN, "Regions:")

#define	AFRICA			dgettext(TEXT_DOMAIN, "Africa")
#define	WESTERN_ASIA		dgettext(TEXT_DOMAIN, "Asia, Western")
#define	EASTERN_ASIA		dgettext(TEXT_DOMAIN, "Asia, Eastern")
#define	AUSTRALIA_NEWZEALAND	dgettext(TEXT_DOMAIN, "Australia / New Zealand")
#define	CANADA			dgettext(TEXT_DOMAIN, "Canada")
#define	EUROPE			dgettext(TEXT_DOMAIN, "Europe")
#define	MEXICO			dgettext(TEXT_DOMAIN, "Mexico")
#define	SOUTH_AMERICA		dgettext(TEXT_DOMAIN, "South America")
#define	UNITED_STATES		dgettext(TEXT_DOMAIN, "United States")
#define	GMT_OFFSET		dgettext(TEXT_DOMAIN, "other - offset from GMT")
#define	TZ_FILE_NAME		dgettext(TEXT_DOMAIN, \
	"other - specify time zone file")

/*
 * Timezone (timezone menu)
 */

#define	TZ_INDEX_TITLE		dgettext(TEXT_DOMAIN, "Time Zone")

#define	TZ_INDEX_PROMPT		dgettext(TEXT_DOMAIN, "Time zones:")

#define	EGYPT			dgettext(TEXT_DOMAIN, "Egypt")
#define	LIBYA			dgettext(TEXT_DOMAIN, "Libya")
#define	TURKEY			dgettext(TEXT_DOMAIN, "Turkey")
#define	WESTERN_USSR		dgettext(TEXT_DOMAIN, "Western Soviet Union")
#define	IRAN			dgettext(TEXT_DOMAIN, "Iran")
#define	ISRAEL			dgettext(TEXT_DOMAIN, "Israel")
#define	SAUDI_ARABIA		dgettext(TEXT_DOMAIN, "Saudi Arabia")
#define	CHINA			dgettext(TEXT_DOMAIN, \
	"People's Republic of China")
#define	TAIWAN			dgettext(TEXT_DOMAIN, \
	"Republic of China / Taiwan")
#define	HONGKONG		dgettext(TEXT_DOMAIN, "Hong Kong")
#define	JAPAN			dgettext(TEXT_DOMAIN, "Japan")
#define	KOREA			dgettext(TEXT_DOMAIN, "Republic of Korea")
#define	SINGAPORE		dgettext(TEXT_DOMAIN, "Singapore")
#define	TASMANIA		dgettext(TEXT_DOMAIN, "Tasmania")
#define	QUEENSLAND		dgettext(TEXT_DOMAIN, "Queensland")
#define	NORTH			dgettext(TEXT_DOMAIN, "North")
#define	SOUTH			dgettext(TEXT_DOMAIN, "South")
#define	WEST			dgettext(TEXT_DOMAIN, "West")
#define	VICTORIA		dgettext(TEXT_DOMAIN, "Victoria")
#define	NEW_SOUTH_WALES		dgettext(TEXT_DOMAIN, "New South Wales")
#define	BROKEN_HILL		dgettext(TEXT_DOMAIN, "Broken Hill")
#define	YANCOWINNA		dgettext(TEXT_DOMAIN, "Yancowinna")
#define	LHI			dgettext(TEXT_DOMAIN, "LHI")
#define	NEW_ZEALAND		dgettext(TEXT_DOMAIN, "New Zealand")
#define	NEWFOUNDLAND		dgettext(TEXT_DOMAIN, "Newfoundland")
#define	ATLANTIC		dgettext(TEXT_DOMAIN, "Atlantic")
#define	EASTERN			dgettext(TEXT_DOMAIN, "Eastern")
#define	CENTRAL			dgettext(TEXT_DOMAIN, "Central")
#define	EAST_SASKATCHEWAN	dgettext(TEXT_DOMAIN, "East Saskatchewan")
#define	MOUNTAIN		dgettext(TEXT_DOMAIN, "Mountain")
#define	PACIFIC			dgettext(TEXT_DOMAIN, "Pacific")
#define	YUKON			dgettext(TEXT_DOMAIN, "Yukon")
#define	EIRE			dgettext(TEXT_DOMAIN, "Ireland")
#define	BRITAIN			dgettext(TEXT_DOMAIN, "Great Britain")
#define	ICELAND			dgettext(TEXT_DOMAIN, "Iceland")
#define	POLAND			dgettext(TEXT_DOMAIN, "Poland")
#define	WESTERN_EUROPE		dgettext(TEXT_DOMAIN, "Western Europe")
#define	MIDDLE_EUROPE		dgettext(TEXT_DOMAIN, "Middle Europe")
#define	EASTERN_EUROPE		dgettext(TEXT_DOMAIN, "Eastern Europe")
#define	MEXICO_BAJA_NORTE	dgettext(TEXT_DOMAIN, "Mexico / Baja Norte")
#define	MEXICO_BAJA_SUR		dgettext(TEXT_DOMAIN, "Mexico / Baja Sur")
#define	MEXICO_GENERAL		dgettext(TEXT_DOMAIN, "Mexico / General")
#define	CUBA			dgettext(TEXT_DOMAIN, "Cuba")
#define	BRAZIL_EAST		dgettext(TEXT_DOMAIN, "Brazil East")
#define	BRAZIL_WEST		dgettext(TEXT_DOMAIN, "Brazil West")
#define	BRAZIL_ACRE		dgettext(TEXT_DOMAIN, "Brazil Acre")
#define	BRAZIL_DE_NORONHA	dgettext(TEXT_DOMAIN, "Brazil De Noronha")
#define	CHILE_CONTINENTAL	dgettext(TEXT_DOMAIN, "Chile Continental")
#define	CHILE_EASTER_ISLAND	dgettext(TEXT_DOMAIN, "Chile Easter Island")
#define	USA_EASTERN		dgettext(TEXT_DOMAIN, "Eastern")
#define	USA_CENTRAL		dgettext(TEXT_DOMAIN, "Central")
#define	USA_MOUNTAIN		dgettext(TEXT_DOMAIN, "Mountain")
#define	USA_PACIFIC		dgettext(TEXT_DOMAIN, "Pacific")
#define	USA_ALASKA		dgettext(TEXT_DOMAIN, "Alaska")
#define	USA_EAST_INDIANA	dgettext(TEXT_DOMAIN, "East-Indiana")
#define	USA_ARIZONA		dgettext(TEXT_DOMAIN, "Arizona")
#define	USA_MICHIGAN		dgettext(TEXT_DOMAIN, "Michigan")
#define	USA_SAMOA		dgettext(TEXT_DOMAIN, "Samoa")
#define	USA_ALEUTIAN		dgettext(TEXT_DOMAIN, "Aleutian")
#define	USA_HAWAII		dgettext(TEXT_DOMAIN, "Hawaii")

/*
 * Timezone (GMT offset)
 */

#define	TZ_GMT_TITLE		dgettext(TEXT_DOMAIN, "Offset From GMT")

#define	TZ_GMT_PROMPT		dgettext(TEXT_DOMAIN, "Hours offset:")

/*
 * Timezone (rules file)
 */

#define	TZ_FILE_TITLE		dgettext(TEXT_DOMAIN, "Time Zone File")

#define	TZ_FILE_PROMPT		dgettext(TEXT_DOMAIN, "File name:")

/*
 * Date and time
 */

#define	DATE_AND_TIME_TITLE	dgettext(TEXT_DOMAIN, "Date and Time")

#define	YEAR		dgettext(TEXT_DOMAIN, "Year   (4 digits) :")
#define	MONTH		dgettext(TEXT_DOMAIN, "Month  (1-12)     :")
#define	DAY		dgettext(TEXT_DOMAIN, "Day    (1-31)     :")
#define	HOUR		dgettext(TEXT_DOMAIN, "Hour   (0-23)     :")
#define	MINUTE		dgettext(TEXT_DOMAIN, "Minute (0-59)     :")

#define	DATE_AND_TIME_CONFIRM	dgettext(TEXT_DOMAIN, "Date and time:")

/*
 * Confirmation
 */

#define	CONFIRM_TITLE		dgettext(TEXT_DOMAIN, "Confirm Information")

#define	CONFIRM_PROMPT		dgettext(TEXT_DOMAIN, \
	"Is the following information correct?")

/*
 * Errors
 */

#define	VALIDATION_ERROR_TITLE	dgettext(TEXT_DOMAIN, "Validation Error")

#define	NOTICE_TITLE		dgettext(TEXT_DOMAIN, \
	"System Identification Notice")

#define	ERROR_TITLE		dgettext(TEXT_DOMAIN, \
	"System Identification Error")

#define	DLOPEN_FAILED		dgettext(TEXT_DOMAIN, \
	"The dynamic library you specified could not be opened.\n")

#define	XTINIT_FAILED		dgettext(TEXT_DOMAIN, \
	"The X Window System toolkit initialization routine failed.\n")

#define	FOUR_PART_IP_ADDR	dgettext(TEXT_DOMAIN, \
	"An IP address must contain four sets of numbers separated by " \
	"periods (example 129.200.9.1).")

#define	MAX_IP_PART		dgettext(TEXT_DOMAIN, \
	"Each component of an IP address must be between 0 and 255.")

#define	IP_NOT_IN_RANGE		dgettext(TEXT_DOMAIN, \
	"The IP address is out of the range of valid addresses.")

#define	IP_IN_UNSPEC_RANGE		dgettext(TEXT_DOMAIN, \
	"IP addresses in the range 224.0.0.0 to 255.255.255.255 are reserved.")

#define	HOST_LENGTH		dgettext(TEXT_DOMAIN, \
	"A host name must be between %d and %d characters.")

#define	HOST_CHARS		dgettext(TEXT_DOMAIN, \
	"A host name can only contain letters, digits, and minus signs (-).")

#define	HOST_MINUS		dgettext(TEXT_DOMAIN, \
	"A host name cannot begin or end with a minus sign (-).")

#define	FOUR_PART_NETMASK	dgettext(TEXT_DOMAIN, \
	"A netmask must contain four sets of numbers separated by periods " \
	"(example 255.255.255.0).")

#define	MAX_NETMASK_PART	dgettext(TEXT_DOMAIN, \
	"Each component of a netmask must be between 0 and 255.")

#define	DOMAIN_LENGTH		dgettext(TEXT_DOMAIN, \
	"A domain name must be between %d and %d characters.")

#define	BAD_IP_ADDR		dgettext(TEXT_DOMAIN, \
	"An error occurred while trying to set the IP address %s " \
	"on the %s network interface.")

#define	BAD_UP_FLAG		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the '%s' flag on the %s network interface: %s.")

#define	BAD_HOSTNAME		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the host name to %s on the network interface %s: %s.")

#define	BAD_NODENAME		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the host name to %s: %s.")

#define	BAD_LOOPBACK		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the loopback name-to-address mapping for %s: %s.")

#define	BAD_HOSTS_ENT		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the host name %s in the hosts file: %s.")

#define	BAD_DOMAIN		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the domain name to %s: %s.")

#define	STORE_NETMASK		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the netmask %s for network number %s in /etc/netmasks: %s")

#define	BAD_NETMASK		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the netmask %s on the network interface %s: %s.")

#define	BAD_TIMEZONE		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the time zone to %s: %s.")

#define	BAD_DATE		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to set the date to %s: %s.")

#define	BAD_YEAR		dgettext(TEXT_DOMAIN, \
	"You have entered an incorrect year.")

#define	GET_ETHER		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to get the Ethernet address for the network interface %s: %s.")

#define	BAD_ETHER		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying to update the " \
	"ethers database with Ethernet address %s for host name %s: %s.")

#define	BAD_BOOTP		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to update the bootparams database for host name %s: %s.")

#define	BAD_NETMASK_ENT		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to update the netmasks database for netmask %s: %s.")

#define	BAD_TIMEZONE_ENT	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to update the timezone database for time zone %s: %s.")

#define	BAD_NISSERVER_ENT	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying to set the name " \
	"service server name %s and IP address %s in the hosts file: %s.")

#define	BAD_YP_ALIASES		dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to initialize the NIS aliases file for domain %s: %s.\n\nTo " \
	"resolve this problem, run 'ypinit -c' after the system has " \
	"booted.  You will need to reboot the machine after doing this.")

#define	BAD_YP_BINDINGS1	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying to set up the " \
	"NIS binding directory for domain %s to facilitate " \
	"automatic server location: %s.")

#define	BAD_YP_BINDINGS2	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying to initialize this " \
	"system as an NIS client for domain %s: %s.\n\nTo resolve " \
	"this problem, run 'ypinit -c' after the system has booted, " \
	"then reboot the system, or manually enter new name service " \
	"information.")

#define	BAD_NIS_SERVER1	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying to locate an NIS+ server " \
	"for domain %s: %s\nThe NIS+ server that responded is: %s.\n\nTo " \
	"resolve this problem, run 'nisclient -i' after the system has " \
	"booted, or manually enter new name service information.")

#define	BAD_NIS_SERVER2	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying to initialize this system " \
	"as an NIS+ client of server %s for domain %s: %s.\n\nTo resolve " \
	"this problem, run 'nisclient -i' after the system has " \
	"booted, or manually enter new name service information.")

#define	NIS_SERVER_ACCESS dgettext(TEXT_DOMAIN, \
	"Error: NIS+ access denied for domain %s.\n\n" \
	"You can continue system configuration with no NIS+ credentials, " \
	"or you can manually enter new name service information now.")

#define	NSSWITCH_FAIL1	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying to configure the " \
	"name service switch file: %s.\n\nTo resolve this problem, " \
	"run '/usr/bin/cp %s %s' after the system has booted, then " \
	"reboot the system.")

#define	NSSWITCH_FAIL2	dgettext(TEXT_DOMAIN, \
	"The following error occurred while trying " \
	"to configure the name service switch file: %s.")

#define	NO_IPADDR	dgettext(TEXT_DOMAIN, \
	"The IP address previously set on the network " \
	"interface %s is no longer available.  The system state " \
	"is corrupted.  System identification can no longer continue.")

#define	NO_NETMASK	dgettext(TEXT_DOMAIN, \
	"The netmask previously assigned to this system " \
	"(network number %s) is no longer available.  The system " \
	"state is corrupted.  System identification can no longer continue.")

#define	BAD_TZ_FILE_NAME	dgettext(TEXT_DOMAIN, \
	"The time zone file name you selected " \
	"does not exist on this system.  Select a different " \
	"file name, or press Cancel to select a time zone from the list " \
	"of geographic region.")

#define	CANT_DO_PASSWORD	dgettext(TEXT_DOMAIN, \
	"Due to an internal error, this process cannot set the superuser " \
	"password at this time.  After the system is booted, you should " \
	"immediately log in as root and set the password by running the " \
	"program `/bin/passwd'.\n\nThe error that occurred was: %s")

#define	CANT_DO_PASSWORD_PLUS	dgettext(TEXT_DOMAIN, \
	"Due to an internal error, this process cannot set the superuser " \
	"password at this time.  After the system is booted, you should " \
	"immediately log in as root and set the password by running the " \
	"program `/bin/passwd'.  You should also set up an encryption key " \
	"for this host by running the program `/usr/bin/chkey'.\n\n" \
	"The error that occurred was: %s")

#define	CANT_DO_KEYLOGIN	dgettext(TEXT_DOMAIN, \
	"The root password does not decrypt this system's secret key. " \
	"Therefore, you cannot create an encryption key for this host at " \
	"this time.  After the system is booted, you should immediately " \
	"log in as root and enter this hosts network password by running " \
	"the program `/usr/bin/keylogin -r'.")

#define	NO_ERROR_FUNC		dgettext(TEXT_DOMAIN, \
	"Can't find get_err_string function in user interface module!")

#define	NO_TITLE_FUNC		dgettext(TEXT_DOMAIN, \
	"Can't find get_attr_title function in user interface module!")

#define	NO_TEXT_FUNC		dgettext(TEXT_DOMAIN, \
	"Can't find get_attr_text function in user interface module!")

#define	NO_PROMPT_FUNC		dgettext(TEXT_DOMAIN, \
	"Can't find get_attr_prompt function in user interface module!")

#define	NO_NAME_FUNC		dgettext(TEXT_DOMAIN, \
	"Can't find get_attr_name function in user interface module!")

#define	UNKNOWN_ERROR		dgettext(TEXT_DOMAIN, "Unknown error (%d)")

#define	NO_MEMORY		dgettext(TEXT_DOMAIN, \
	"The program ran out of memory and cannot continue.")

/*
 * Busy messages
 */

#define	PLEASE_WAIT_S		dgettext(TEXT_DOMAIN, "Please wait...")

#define	PLEASE_WAIT		dgettext(TEXT_DOMAIN, \
	"Configuring parameters, %d seconds left to complete.")

#define	GOODBYE_NETINIT		dgettext(TEXT_DOMAIN, "Just a moment...")

#define	GOODBYE_NISINIT		dgettext(TEXT_DOMAIN, "Just a moment...")

#define	GOODBYE_SYSINIT		dgettext(TEXT_DOMAIN, \
	"System identification is completed.")

/*
 * Catch all
 */

#define	YES			dgettext(TEXT_DOMAIN, "Yes")
#define	NO			dgettext(TEXT_DOMAIN, "No")

#endif	SYSID_MSGS_H
