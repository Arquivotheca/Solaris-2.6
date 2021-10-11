/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#ifndef lint
#ident	"@(#)xv_load.c 1.12 93/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef i386
#define	DEFAULT_DEVICE	"/dev/dsk/c0t6d0p0"
#else
#define	DEFAULT_DEVICE	"/dev/dsk/c0t6d0s0"
#endif
#define	DEFAULT_DIR	"/cdrom"

int
LoadMedia(MediaType type,
	char	*device,
	char	*dir,
	int	interactive)
{
	Module	*media = (Module *)0;
	int	status;

	switch (type) {
	case CDROM:
		/*
		 * Quick super-user check
		 */
		if (geteuid() != 0) {
			asktoproceed(Loadscreen, gettext(
			    "Sorry, you are only allowed to use\n"
			    "mounted directories as source media when\n"
			    "you run the program as a non super-user.\n"));
			break;
		}

		if (path_is_block_device(device) != SUCCESS) {
			asktoproceed(Loadscreen, gettext(
		    "You must supply the name of a block special device.\n"));
			break;
		}

		if (path_is_readable(dir) != SUCCESS) {
			/*
			 * Mount point doesn't exist; if
			 * running interactively we'll
			 * see if user wants it created,
			 * otherwise we'll just do it.
			 */
			if (!interactive || confirm(Loadscreen,
			    xstrdup(gettext("Create")),
			    xstrdup(gettext("Cancel")),
			    xstrdup(gettext(
				"Directory `%s' does not exist.\n"
				"You can choose to create it or cancel your\n"
				"attempted mount.\n")),
			    dir) == 0)
				break;
			if (path_create(dir, SWM_DIR_FLAG) != SUCCESS) {
				asktoproceed(Loadscreen, gettext(
				    "Cannot create directory `%s':  %s\n"),
					dir, strerror(errno));
				break;
			}
		}
		/*
		 * Now have mount point
		 */
		media = add_specific_media((char *)0, device);
		status = mount_media(media, dir, CDROM);

		switch (status) {
		case SUCCESS:
			break;
		case EBUSY:
			asktoproceed(Loadscreen, gettext(
			    "Your attempt to mount `%s' on `%s'\n"
			    "has failed.  One or both of the device or mount\n"
			    "point are already in use.  See mount(1M) for\n"
			    "further information.\n"),
				device, dir);
			media = (Module *)0;
			break;
		case ENXIO:
			asktoproceed(Loadscreen, gettext(
			    "Your attempt to mount `%s' on `%s'\n"
			    "has failed.  There does not appear to be a disk\n"
			    "caddy inserted in the drive.\n"),
				device, dir);
			media = (Module *)0;
			break;
		case EINVAL:
			asktoproceed(Loadscreen, gettext(
			    "Your attempt to mount `%s' on `%s'\n"
			    "has failed.  Disk partition `%s' is not\n"
			    "formatted with a ufs or hsfs file system.\n"),
				device, dir, device);
			media = (Module *)0;
			break;
		case ERR_VOLUME:	/* XXX not errno */
			asktoproceed(Loadscreen, gettext(
			    "Your attempt to mount `%s' on `%s'\n"
			    "has failed.  The device `%s' is controlled\n"
			    "by the volume management system and must be\n"
			    "treated as a mounted directory.\n"),
				device, dir, device);
			media = (Module *)0;
			break;
		default:
			asktoproceed(Loadscreen, gettext(
			    "Your attempt to mount `%s' on `%s'\n"
			    "has failed:  %s.\n"),
				device, dir, strerror(status));
			media = (Module *)0;
			break;
		}
		break;

	default:
	case MOUNTED_FS:
		if (path_is_readable(dir) == SUCCESS)
			media = add_media(dir);
		else
			asktoproceed(Loadscreen, gettext(
		    "Directory `%s' does not exist or is not readable.\n"),
				dir);
		break;
	}

	if (media) {
		status = load_media(media, 1);	/* use .packagetoc if present */

		switch (status) {
		case ERR_INVALIDTYPE:
			asktoproceed(Loadscreen,
				gettext("Media type invalid\n"));
			media = (Module *)0;
			break;
		case ERR_NOMEDIA:
			asktoproceed(Loadscreen,
				gettext("Media not found\n"));
			media = (Module *)0;
			break;
		case ERR_UMOUNTED:
			asktoproceed(Loadscreen,
				gettext("Media not mounted\n"));
			media = (Module *)0;
			break;
		case ERR_NOPROD:
		case ERR_NOLOAD:
			asktoproceed(Loadscreen, gettext(
			    "Cannot find any installable software on the\n"
			    "media.  To be recognizable as containing\n"
			    "software, the media must have either a .cdtoc\n"
			    "file or installable software (packages or 4.x\n"
			    "unbundled products) in its root directory.  If\n"
			    "the software on the media is grouped in sub-\n"
			    "directories, try specifying the name of one\n"
			    "of the subdirectories.\n"));
			media = (Module *)0;
			break;
		case SUCCESS:
		case ERR_NOFILE:	/* no .clustertoc file */
			set_source_media(media);
			break;
		default:
			asktoproceed(Loadscreen,
			    gettext("Unspecified media error (%d)\n"),
				status);
			media = (Module *)0;
			break;
		}
	}
	return (media != (Module *)0);
}

void
SetMediaType(int ui_type,
	Xv_opaque control,	/* control panel, for caret placement */
	Xv_opaque dev_item,	/* device name panel item */
	Xv_opaque dir_item,	/* directory name panel item */
	Xv_opaque eject_button)	/* eject button panel item */
{
	switch (ui_type) {
	case 0:		/* local CD-ROM */
		xv_set(dev_item, PANEL_INACTIVE, FALSE, 0);
		xv_set(dir_item, PANEL_INACTIVE, FALSE, 0);
		xv_set(eject_button, PANEL_INACTIVE, FALSE, 0);
		xv_set(control, PANEL_CARET_ITEM, dev_item, 0);
		break;
	default:
	case 1:		/* Directory */
		xv_set(dev_item, PANEL_INACTIVE, TRUE, 0);
		xv_set(dir_item, PANEL_INACTIVE, FALSE, 0);
		xv_set(eject_button, PANEL_INACTIVE, TRUE, 0);
		xv_set(control, PANEL_CARET_ITEM, dir_item, 0);
		break;
	}
}

/*
 * Called when user presses "eject"
 */
void
EjectMedia(MediaType type,
	char	*device,
	char	*dir)
{
	Module	*media = (Module *)0;

	/*
	 * XXX For the time being, do local CD-ROMs only
	 */
	if (type != CDROM)
		return;

	if (device && device[0] == '\0')
		device = (char *)0;
	if (dir && dir[0] == '\0')
		dir = (char *)0;

	if (device) {
		media = find_media((char *)0, device);
		if (media == (Module *)0)
			asktoproceed(Loadscreen, gettext(
			    "The device you specified\n(%s) is not mounted.\n"),
				device);
	} else if (dir) {
		media = find_media(dir, (char *)0);
		if (media == (Module *)0)
			asktoproceed(Loadscreen, gettext(
			    "Nothing is mounted on the directory\n"
			    "you specified (%s)."),
				dir);
	} else
		asktoproceed(Loadscreen, gettext(
		    "You must specify a device,\n"
		    "a directory, or both."));

	if (media != (Module *)0) {
		set_eject_on_exit(1);
		unload_media(media);
		if (media == get_source_media()) {
			set_source_media((Module *)0);
			if (get_mode() == MODE_INSTALL)
				BrowseModules(MODE_INSTALL, VIEW_UNSPEC);
		}
		set_eject_on_exit(swm_eject_on_exit);
	}
}

MediaType
LoadTypeToMediaType(load_type)
	int	load_type;
{
	MediaType type;

	switch (load_type) {
	case 0:
		type = CDROM;
		break;
	case 1:
		type = MOUNTED_FS;
		break;
	default:
		type = ANYTYPE;
		break;
	}
	return (type);
}

/*ARGSUSED*/
void
ResetMedia(Panel_item	type,
	Panel_item	device,
	Panel_item	eject,
	Panel_item	dir)
{
	Module	*media = get_source_media();

	if (media == (Module *)0) {
		xv_set(type, PANEL_VALUE, 0, NULL);
		xv_set(device,
			PANEL_VALUE,	DEFAULT_DEVICE,
			PANEL_INACTIVE,	FALSE,
			NULL);
		xv_set(eject, PANEL_INACTIVE, FALSE, NULL);
		xv_set(dir,
			PANEL_VALUE,	DEFAULT_DIR,
			PANEL_INACTIVE,	FALSE,
			NULL);
	} else {
		Media	*info = media->info.media;

		xv_set(type,
			PANEL_VALUE,	info->med_type == CDROM ? 0 : 1,
			NULL);
		xv_set(device,
			PANEL_VALUE,	info->med_device,
			PANEL_INACTIVE,	info->med_type == CDROM ? TRUE : FALSE,
			NULL);
		xv_set(eject,
			PANEL_INACTIVE,	info->med_type == CDROM ? TRUE : FALSE,
			NULL);
		xv_set(dir,
			PANEL_VALUE,	info->med_dir,
			PANEL_INACTIVE,	FALSE,
			NULL);
	}
}
