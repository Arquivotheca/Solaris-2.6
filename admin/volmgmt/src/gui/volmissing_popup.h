/*
 * Copyright (c) 1995 by Sunsoft.
 */

#ifndef	_VOLMISSING_POPUP_H
#define	_VOLMISSING_POPUP_H

#pragma	ident	"@(#)volmissing_popup.h  1.7  95/02/01 Sunsoft"

/*
 * A few useful constants
 */

#define	VOLCANCEL_UNKNOWN_ERROR	-1
#define	VOLCANCEL_USAGE_ERROR	1
#define	VOLMGT_NOT_RUNNING	2
#define	VOLCANCEL_OPEN_ERROR	3
#define	VOLCANCEL_IOCTL_ERROR	4
#define	EXECL_FAILED		-99
#define EJECT_UNKNOWN_ERROR	-1
#define EJECT_FAILED		1
#define EJECT_USAGE_ERROR	2
#define	EJECT_IOCTL_ERROR	3
#define	EJECT_WORKED_X86	4
#define INIT			1
#define FOLLOW_UP		2
#define	MAX_BUF_LEN		256

#define SCCS_ID 	"@(#)volmissing_popup.h  1.7 95/02/01 SMI"

#define	STD_USER_MSG	"User %s (%s) has requested that a %s volume\n\
named %s be loaded into a drive."

#define	STD_USER_MSG_NO_GECOS	"User %s has requested that a %s volume\n\
named %s be loaded into a drive."

#define	STD_ROOT_MSG	"The system has requested that a %s volume\n\
named %s be loaded into a drive."

#define	HINT_MSG	"Please insert the requested volume and press OK,\n\
or press Cancel to withdraw the user's request."

#define	TITLE		"Removable Media Manager"

#define	NOT_RUNNING_MSG "The Volume Management system is not running.\n\
Contact your System Administrator to restart Volume Management."

#define	VOLCANCEL_USAGE_MSG 	"The volcancel command was called incorrectly.\n\
Please report this to Sun Technical Support."

#define	VOLCANCEL_OPEN_MSG	"Unable to OPEN %s volume named %s.\n\
Contact your System Administrator to restart Volume Management."

#define	VOLCANCEL_IOCTL_MSG	"Unable to CANCEL I/O for %s volume named %s.\n\
Contact your System Administrator to restart Volume Management."

#define	VOLCANCEL_EXECL_MSG	"Unable to execl volcancel.\n\
Contact your System Administrator to restart Volume Management."

#define	UNKNOWN_VOLCANCEL_EXIT	"The volcancel command returned an unknown exit value.\n\
Contact your System Administrator to restart Volume Management."

#define VOLUME_NOT_FOUND	"%s volume named %s not found.\nMake sure \
there is media in the device."

#define BOGUS_VOLUME_FOUND	"You have inserted an incorrect volume(s) into %s.\n\
Press the Eject button to remove the incorrect volume(s)."

#define	EJECT_FAILED_MSG	"The eject command failed.\n\
Contact your System Administrator to restart Volume Management."

#define	EJECT_USAGE_MSG 	"The eject command was called incorrectly.\n\
Please report this to Sun Technical Support."

#define	EJECT_IOCTL_MSG		"Unable to eject these devices - %s\n\
Contact your System Administrator to restart Volume Management."

#define	EJECT_EXECL_MSG		"Unable to execl eject.\n\
Contact your System Administrator to restart Volume Management."

#define	UNKNOWN_EJECT_EXIT	"The eject command returned an unknown exit value.\n\
Contact your System Administrator to restart Volume Management."

#define ENV_ERROR_MSG		"failed due to undefined environment variables"

#define USAGE	"usage: %s can not be run by and end-user\n"

#endif /* _VOLMISSING_POPUP_H */

