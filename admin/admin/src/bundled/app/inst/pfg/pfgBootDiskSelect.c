#ifndef lint
#pragma ident "@(#)pfgbootdiskselect.c 1.49 95/11/07 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgbootdiskselect2.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgBootDiskSelect_ui.h"

Widget		*disk_toggle;
Widget		existing_bd;
WidgetList	*disk_id1_widget_list;
WidgetList	widget_list;
Widget		*option_menu;
Widget		*solarpart_label;
Widget		*device_menu;
static int		numDisks;
char		*selected_disk;
int		device_index;
int		part_count;
Widget		Parent;
Widget		prom_toggle;

void bootdiskOkCB(Widget, XtPointer, XtPointer);
void bootdiskCancelCB(Widget, XtPointer, XtPointer);
void bootdiskResetCB(Widget, XtPointer, XtPointer);
void set_boot_device(Widget, XtPointer, XtPointer);

Widget
pfgCreateBootDisk(Widget parent)
{
	Widget		bootdisk_dialog;
	Widget 		scrolled_form;
	Widget		toggle = NULL;
	Widget		disk_widget = NULL;
	Widget		device_widget = NULL;
	Disk_t		*bootDisk, *dp;
	Disk_t		*boot;
	char		bootName[16];
	char		bootDiskName[16];
	char		partbuf[20];
	char		*name;
	char		part_char;
	int		i, j, n;
	int		partnum;
	int		count;
	Dimension	toggle_width, combo_width;
	Arg		args[10];
	char		**device_strings;
	XmString	*device_xmstrings;
	XmString	name_string;
	Widget		menu_button;
	int		update;		/* can we update the PROM ? */
	int		allow_update;
	char		label_buf[32];
	int		index;
	char		*dialog_buf;
	char		*tmp_buf;
	int		dindex;

	Parent = parent;
	/*
	 * create the boot device selection dialog
	 */
	bootdisk_dialog = tu_bootdisk_dialog2_widget("bootdisk_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(bootdisk_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(bootdisk_dialog),
		XmNtitle, TITLE_SELECT_BOOT_DEVICE,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);
	xm_SetNoResize(pfgTopLevel, bootdisk_dialog);

	dialog_buf = (char *) xmalloc(strlen(MSG_BOOTDISK) +
				strlen(MSG_BOOTDISK_ANY) +
				strlen(MSG_BOOTDISK_REBOOT) +
				(2 * strlen(APP_SLICE)) +
				(2 * strlen(APP_PARTITION)) + 1);

	dialog_buf[0] = '\0';

	tmp_buf = (char *) xmalloc(strlen(MSG_BOOTDISK_ANY) +
				strlen(APP_SLICE) +
				strlen(APP_PARTITION) +
				strlen(MSG_BOOTDISK_REBOOT) + 1);

	tmp_buf[0] = '\0';

	BootobjGetAttribute(CFG_CURRENT,
		BOOTOBJ_DEVICE_TYPE, &part_char,
		BOOTOBJ_PROM_UPDATEABLE, &update,
		NULL);


	/*
	 * build the message according to the architecture and
	 * prom update conditions
	 */
	if (part_char == 's') {
		/*
		 * for sparc we do give the user the ANY choice, so add this
		 * into the message that is displayed
		 */
		if (update == 1) {
			(void) sprintf(tmp_buf, MSG_BOOTDISK_ANY,
					APP_SLICE, MSG_BOOTDISK_REBOOT);
			(void) sprintf(dialog_buf, MSG_BOOTDISK, APP_SLICE,
					tmp_buf);
		} else {
			(void) sprintf(tmp_buf, MSG_BOOTDISK_ANY,
					APP_SLICE, "");
			(void) sprintf(dialog_buf, MSG_BOOTDISK, APP_SLICE,
					tmp_buf);
		}
	} else {
		/*
		 * we don't have the ANY choice for ppc or x86 so we don't
		 * need to put this in the message
		 */
		if (update == 1) {
			(void) sprintf(dialog_buf, MSG_BOOTDISK, APP_DISK,
				MSG_BOOTDISK_REBOOT);
		} else {
			(void) sprintf(dialog_buf, MSG_BOOTDISK, APP_DISK,
				"");
		}
	}


	/*
	 * set up the on-screen text and button labels
	 */
	pfgSetWidgetString(widget_list, "panelhelpText",
		dialog_buf);
	pfgSetWidgetString(widget_list, "okButton", PFG_OKAY);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "resetButton", PFG_RESET);
	pfgSetWidgetString(widget_list, "customButton", PFG_CUSTOMIZE);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);
	pfgSetWidgetString(widget_list, "disk_label", PFG_DISKLABEL);
	pfgSetWidgetString(widget_list, "device_label", PFG_DEVICELABEL);
	pfgSetWidgetString(widget_list, "existing_label", PFG_EXISTING_BOOT);

	if (!IsIsa("sparc")) {
		XtUnmanageChild(pfgGetNamedWidget(widget_list,
						"existing_label"));
	}

	prom_toggle = pfgGetNamedWidget(widget_list, "rebootButton");
	BootobjGetAttribute(CFG_CURRENT,
		BOOTOBJ_PROM_UPDATEABLE, &update,
		BOOTOBJ_PROM_UPDATE, &allow_update,
		NULL);

	if (allow_update == 1)
		XmToggleButtonSetState(prom_toggle, True, False);
	else
		XmToggleButtonSetState(prom_toggle, False, False);


	if (update == 1)
		pfgSetWidgetString(widget_list, "rebootButton", UPDATE_PROM_QUERY);
	else
		XtUnmanageChild(prom_toggle);

	existing_bd = pfgGetNamedWidget(widget_list, "existing_value");
	if (!IsIsa("sparc"))
		XtUnmanageChild(existing_bd);

	disk_widget = pfgGetNamedWidget(widget_list, "disk_label");
	device_widget = pfgGetNamedWidget(widget_list, "device_label");

	if (!IsIsa("sparc"))
		XtUnmanageChild(device_widget);

	/*
	 * get the widget id of the form that will contain the
	 * device selection objects
	 */
	scrolled_form = pfgGetNamedWidget(widget_list, "form_list");

	/*
	 * are we dealing with slices ('s') or partitions ('p') ?
	 */
	BootobjGetAttribute(CFG_CURRENT,
		BOOTOBJ_DEVICE_TYPE, &part_char,
		NULL);
	/*
	 * how many partitions are there?  it depends on the architecture
	 */
	part_count = NUMPARTS;

	count = 0;
	WALK_DISK_LIST(dp) {
		if (disk_okay(dp) && disk_selected(dp)) {
			count++;
			numDisks = count;
		}
	}

	write_debug(GUI_DEBUG_L1, "There are %d disks in the disklist",
							numDisks);

	/*
	 * walk the list of disks and create the list of devices for
	 * each one
	 */

	disk_toggle = (Widget *) xcalloc(sizeof (Widget) * (numDisks + 1));
	disk_id1_widget_list = (WidgetList *) xcalloc(sizeof (WidgetList) *
							(numDisks + 1));
	device_menu = (Widget *) xcalloc(sizeof (Widget) * (numDisks + 1));
	option_menu = (Widget *) xcalloc(sizeof (Widget) * (numDisks + 1));
	solarpart_label = (Widget *) xcalloc(sizeof (Widget) * (numDisks + 1));

	i = 0;
	WALK_DISK_LIST(dp) {
		if (disk_selected(dp)) {
			device_strings = (char **) xmalloc(sizeof (char *) *
							(part_count + 1));
			device_xmstrings =
					(XmString *) xmalloc(sizeof (XmString) *
							(part_count + 1));

			/*
			 * create the disk selection toggle
			 */

			disk_toggle[i] = tu_disk_id1_widget("disk_toggle",
					scrolled_form, &disk_id1_widget_list[i]);

			toggle = pfgGetNamedWidget(disk_id1_widget_list[i],
								"diskToggle1");

			/*
			 * get the name of the disk
			 */
			name = disk_name(dp);
			name_string = XmStringCreateLocalized(name);
			XtVaSetValues(toggle,
				XmNuserData, name,
				XmNlabelString, name_string,
				NULL);

			XtVaGetValues(toggle,
				XmNwidth, &toggle_width,
				NULL);

			/*
			 * display the existing boot device for reference
			 * to set
			 */
			if (DiskobjFindBoot(CFG_EXIST, &boot) == D_OK &&
					boot != NULL) {
				(void) strcpy(bootName, disk_name(boot));
				XtVaSetValues(existing_bd,
					XmNlabelString, XmStringCreateLocalized(bootName),
					NULL);
			} else {
				(void) strcpy(bootName, "");
				XtUnmanageChild(existing_bd);
				XtUnmanageChild(pfgGetNamedWidget(widget_list,
						"existing_label"));
			}

			/*
			 * if the disk is the boot disk then set it's toggle button
			 * to set
			 */
			if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) == D_OK &&
					bootDisk != NULL)
				(void) strcpy(bootDiskName, disk_name(bootDisk));
			else
				(void) strcpy(bootDiskName, "");

			if (numDisks == 1 ||
				BootobjIsExplicit(CFG_CURRENT,
					BOOTOBJ_DISK_EXPLICIT) &&
				streq(name, bootDiskName)) {
				XtVaSetValues(toggle,
						XmNset, True,
						NULL);
				selected_disk = xstrdup(name);
				write_debug(GUI_DEBUG_L1,
						"The boot disk is %s",
						selected_disk);
			}

			/*
			 * set up the starting partition number for fdisk and
			 * non-fdisk systems
			 */
				
			/*
			 * partition numbers for sdisk start at 0
			 */
			partnum = 0;

			/*
			 * create the device name strings for the disk
			 */
			for (j = 0; j < part_count; j++) {
				(void) sprintf(partbuf, "%s%c%d", name,
							part_char, partnum);
				device_strings[j] = xstrdup(partbuf);
				device_xmstrings[j] = XmStringCreateLocalized(partbuf);
				if (strcmp(name, bootName) == 0 && j == 0) {
					XtVaSetValues(existing_bd,
						XmNlabelString, device_xmstrings[j],
						NULL);
				}
				if (strcmp(name, bootDiskName) == 0 && j == 0) {
					/*
					 * set the default values for the device_index
					 * and the selected_disk
					 */
					(void) BootobjGetAttribute(CFG_CURRENT,
							BOOTOBJ_DEVICE, &dindex,
							NULL);
					if (dindex == -1)
						device_index = -1;
					else
						device_index = dindex;
					selected_disk = xstrdup(name);
					write_debug(GUI_DEBUG_L1,
						"The device index is %d",
						device_index);
					write_debug(GUI_DEBUG_L1,
						"The boot disk is %s",
						selected_disk);
				}
				partnum++;
			}
			/*
			 * the ANY choice is added to the list to allow the
			 * user to wildcard the device for a specific disk
			 */
			(void) sprintf(partbuf, "%s", PFG_ANYSTRING);
			device_strings[part_count] = xstrdup(partbuf);
			device_xmstrings[part_count] = XmStringCreateLocalized(partbuf);

			option_menu[i] = pfgGetNamedWidget(disk_id1_widget_list[i],
								"optionMenu");
			if (!IsIsa("sparc"))
				XtUnmanageChild(option_menu[i]);

			if ((strcmp(name, bootDiskName) == 0 &&
				BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) ||
				numDisks == 1) {
				XtSetSensitive(option_menu[i], True);
				(void) BootobjGetAttribute(CFG_CURRENT,
						BOOTOBJ_DEVICE, &dindex,
						NULL);
			} else {
				XtSetSensitive(option_menu[i], False);
			}

			solarpart_label[i] = pfgGetNamedWidget(disk_id1_widget_list[i],
							"solar_part_device_label");

			device_menu[i] = pfgGetNamedWidget(disk_id1_widget_list[i],
								"pulldownMenu");
			for (j = 0; j < part_count; j++) {
				n = 0;
				XtSetArg(args[n], XmNlabelString, device_xmstrings[j]); n++;
				XtSetArg(args[n], XmNuserData, device_strings[j]); n++;
				menu_button = XmCreatePushButton(device_menu[i],
						"menu_button", args, n);
				XtManageChild(menu_button);
				/*
				 * fix for bug id 1253300, only set menu history
				 * if the current disk is the boot disk, and
				 * the device is explicit and we're on the spcified
				 * slice
				 */
				if (strcmp(name, bootDiskName) == 0 &&
				BootobjIsExplicit(CFG_CURRENT,BOOTOBJ_DEVICE_EXPLICIT)
				&& j == dindex) {
						XtVaSetValues(option_menu[i],
							XmNmenuHistory, menu_button,
							NULL);
				}
				XtAddCallback(menu_button, XmNactivateCallback,
						set_boot_device, option_menu[i]);

			}
			n = 0;
			XtSetArg(args[n], XmNlabelString, device_xmstrings[part_count]); n++;
			XtSetArg(args[n], XmNuserData, device_strings[part_count]); n++;
			menu_button = XmCreatePushButton(device_menu[i],
					"menu_button", args, n);
			if (!streq(name, bootDiskName) || dindex == -1 ||
			!BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DEVICE_EXPLICIT)) {
				XtVaSetValues(option_menu[i],
						XmNmenuHistory, menu_button,
						NULL);
			}
			XtManageChild(menu_button);
			XtAddCallback(menu_button, XmNactivateCallback,
					set_boot_device, option_menu[i]);

			/*
			 * if this is the first combo box, attach it to the form
			 *
			 * if this is not the first combo box, attach it to
			 * the previous combo box
			 */
			if (i == 0) {
				XtVaSetValues(disk_toggle[i],
					XmNtopAttachment, XmATTACH_FORM,
					NULL);
			} else {
				XtVaSetValues(disk_toggle[i],
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, device_menu[i-1],
					NULL);
			}


			XtVaGetValues(device_menu[i],
				XmNwidth, &combo_width,
				NULL);

			XtVaSetValues(disk_widget,
				XmNwidth, toggle_width,
				NULL);
			if (IsIsa("sparc")) {
				XtVaSetValues(device_widget,
					XmNwidth, combo_width,
					NULL);
			}

		i++;



		} else {
			if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) == D_OK &&
						bootDisk != NULL)
				(void) strcpy(bootDiskName, disk_name(bootDisk));
			else
				(void) strcpy(bootDiskName, "");

			if (strcmp(disk_name(dp), bootDiskName) == 0) {
				XtVaSetValues(existing_bd,
					XmNlabelString,
						XmStringCreateLocalized(bootDiskName),
					NULL);
				selected_disk = xstrdup(bootDiskName);
				write_debug(GUI_DEBUG_L1, "The boot disk is %s",
						selected_disk);
			}
		}

	}

	if (numDisks > 1) {
		/*
		 * create a No Preference choice and make it the
		 * default selection on the screen
		 */

		/*
		 * create the No Preference toggle
		 */

		disk_toggle[i] = tu_disk_id1_widget("disk_toggle",
			scrolled_form, &disk_id1_widget_list[i]);

		toggle = pfgGetNamedWidget(disk_id1_widget_list[i],
						"diskToggle1");
		/*
		 * the No Preference choice doesn't need a pulldown menu
		 * so unmanage it
		 */
		option_menu[i] = pfgGetNamedWidget(disk_id1_widget_list[i],
						"optionMenu");
		XtUnmanageChild(option_menu[i]);
		option_menu[i] = NULL;

		/*
		 * this one is OK... set no pref to default if the boot disk
		 * is not explicitly set
		 */
		if (!BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) {
			XtVaSetValues(toggle,
				XmNset, True,
				NULL);
			/*
			 * set the default disk and device values in case
			 * the user just hits continue
			 */
			selected_disk = xstrdup("");
			device_index = -1;

			(void) BootobjSetAttribute(CFG_CURRENT,
				BOOTOBJ_DISK, selected_disk,
				BOOTOBJ_DEVICE, device_index,
				NULL);
		}

		/*
		 * set the toggle label to No Preference
		 */
		pfgSetWidgetString(disk_id1_widget_list[i],
				"diskToggle1",
				APP_NOPREF_CHOICE);

		XtVaSetValues(toggle,
			XmNuserData, NULL,
			NULL);

		XtVaGetValues(toggle,
			XmNwidth, &toggle_width,
			NULL);

		XtVaSetValues(disk_toggle[i],
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, disk_toggle[i - 1],
			NULL);

	}

	/*
	 * manage the newly created dialog
	 */
	XtManageChild(bootdisk_dialog);

	/*
	 * set the default focus to the ok button
	 */
	(void) XmProcessTraversal(pfgGetNamedWidget(widget_list, "okButton"),
				XmTRAVERSE_CURRENT);

	free(device_strings);
	XtFree(device_xmstrings);
	return (bootdisk_dialog);
}

/* ARGSUSED1 */
void
bootdiskOkCB(Widget w, XtPointer clientD, XtPointer callD)
{
	int	j;
	int	updateable;
	Boolean	set;
	char	buf[50];
	Widget	bd_label;
	char	part_char;
	int	index;
	char	name[32];

	if (selected_disk == NULL)
		device_index = -1;

	if (selected_disk == NULL) {
		write_debug(GUI_DEBUG_L1, "The selected boot disk is no Pref");
	 } else {
		write_debug(GUI_DEBUG_L1, "The selected boot disk is %s",
						selected_disk);
	}

	write_debug(GUI_DEBUG_L1, "The selected device index is %d", device_index);

	(void) BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, selected_disk,
			BOOTOBJ_DISK_EXPLICIT, (strcmp(selected_disk, "") == 0) ? 0 : 1,
			BOOTOBJ_DEVICE, device_index,
			BOOTOBJ_DEVICE_EXPLICIT, device_index == -1 ? 0 : 1,
			NULL);

	if (prom_toggle != NULL) {
		BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_PROM_UPDATEABLE, &updateable,
			NULL);
		if (updateable == 1) {
			set = XmToggleButtonGetState(prom_toggle);
			if (set == True) {
				write_debug(GUI_DEBUG_L1, "update the PROM");
				BootobjSetAttribute(CFG_CURRENT,
					BOOTOBJ_PROM_UPDATE, 1,
					NULL);
			} else {
				write_debug(GUI_DEBUG_L1, "DON'T update the PROM");
				BootobjSetAttribute(CFG_CURRENT,
					BOOTOBJ_PROM_UPDATE, 0,
					NULL);
			}
		}
	}

	(void) BootobjCommit();

	XtUnmanageChild(pfgShell(w));
	XtDestroyWidget(pfgShell(w));
	pfgUnbusy(pfgShell(Parent));

	bd_label = XtNameToWidget(Parent, "*boot_label");
	(void) BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, name,
			BOOTOBJ_DEVICE, &index,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			NULL);

	if (index == -1 && strcmp(name, "") != 0) {
		(void) sprintf(buf, PFG_BOOTDISKLABEL, name);
	} else if (index == -1 && strcmp(name, "") == 0 &&
		!BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) {
		(void) sprintf(buf, PFG_BOOTDISKLABEL, APP_NOPREF_CHOICE);
	} else if (index != -1 &&
		!BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) {
		(void) sprintf(buf, PFG_BOOTDISKLABEL, APP_NOPREF_CHOICE);
	} else {
		(void) sprintf(buf, PFG_BOOTDEVICELABEL, name,
				part_char, index);
	}

	XtVaSetValues(bd_label,
		XmNlabelString, XmStringCreateLocalized(buf),
		NULL);

	/*
	 * free the widget list(s)
	 */
	free(widget_list);
	for (j = 0; j < numDisks + 1; j++) {
		free(disk_id1_widget_list[j]);
	}
	XtFree(option_menu);
	XtFree(device_menu);
	XtFree(disk_toggle);

	pfgDiskPopulateLists();

}

/* ARGSUSED1 */
void
bootdiskCancelCB(Widget w, XtPointer clientD, XtPointer callD)
{
	(void) BootobjRestore(CFG_COMMIT);

	XtUnmanageChild(pfgShell(w));
	XtDestroyWidget(pfgShell(w));
	pfgUnbusy(pfgShell(Parent));

}

/* ARGSUSED0 */
void
bootdiskResetCB(Widget w, XtPointer clientD, XtPointer callD)
{
	Widget		toggle;
	Widget		option;
	Widget		device_pulldown_menu;
	int		i;
	Disk_t		*dp;
	Disk_t		*bootDisk;
	char		bootDiskName[16];
	int		dev_index;
	int		item_index;
	WidgetList	menu_children;
	Cardinal	numChildren;
	char		label_buf[32];
	int		index;
	int		isBoot;
	char		part_char;

	(void) BootobjRestore(CFG_COMMIT);

	/*
	 * update the screen to show the new current state
	 */

        BootobjGetAttribute(CFG_CURRENT,
                BOOTOBJ_DEVICE_TYPE, &part_char,
                NULL);

	i = 0;
	WALK_DISK_LIST(dp) {
		if (disk_selected(dp)) {

			/*
			 * If the disk is the boot disk then set it's toggle 
                         * button to set.
			 */

			if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) == D_OK &&
					bootDisk != NULL)
				(void) strcpy(bootDiskName, disk_name(bootDisk));
			else
				(void) strcpy(bootDiskName, "");

			toggle = pfgGetNamedWidget(disk_id1_widget_list[i],
				"diskToggle1");
			option = option_menu[i];
			isBoot = !strcmp(disk_name(dp), bootDiskName);

			if ((isBoot && 
				BootobjIsExplicit(CFG_CURRENT,
                                        BOOTOBJ_DISK_EXPLICIT)) || 
				numDisks == 1) {

				XtVaSetValues(toggle,
					XmNset, True,
					NULL);

				BootobjGetAttribute(CFG_CURRENT,
					BOOTOBJ_DEVICE, &dev_index,
					BOOTOBJ_DEVICE_TYPE, &part_char,
					NULL);

				write_debug(GUI_DEBUG_L1,
					"The current dev index is %d",
					dev_index);

				/*
				 *	If it's a sparc disk then it has
				 *	a device menu.
				 */

				if(part_char == 's') {
					item_index           = dev_index;
					device_pulldown_menu = device_menu[i];

					XtVaGetValues(device_pulldown_menu,
						XmNchildren, &menu_children,
						XmNnumChildren, &numChildren,
						NULL);
	
                                	if(isBoot && 
						BootobjIsExplicit(CFG_CURRENT,
                                        	BOOTOBJ_DISK_EXPLICIT)) {
						XtVaSetValues(option,
							XmNmenuHistory, 
							menu_children[
								item_index],
							NULL);
					}
					else {
						XtVaSetValues(option,
                                                	XmNmenuHistory,
                                                	menu_children[
								numChildren-1],
                                                	NULL);
					}
					XtSetSensitive(option, True);
                                } 

				selected_disk = xstrdup(disk_name(dp));
				write_debug(GUI_DEBUG_L1, "The boot disk is %s",
						selected_disk);
			} else {
				XtVaSetValues(toggle,
						XmNset, False,
						NULL);
				if(part_char == 's') {
					XtSetSensitive(option, False);
				}
			}
		i++;
		}
	}
        /*
         *    Set the "no preference" toggle.
         */
	if (numDisks > 1) {
		toggle = pfgGetNamedWidget(disk_id1_widget_list[i],
					"diskToggle1");
		if (!BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) {
			selected_disk = xstrdup("");
			XtVaSetValues(toggle,
				XmNset, True,
				NULL);
		}
                else {
			XtVaSetValues(toggle,
				XmNset, False,
				NULL);
		}
	}
}

/* ARGUSED */
void
set_boot_device(Widget w, XtPointer clientD, XtPointer callD)
{
	XmString	device_label;
	short		index;
	char		*device_name;
	Widget		option_list;
	Widget		selected_option;


	/*
	 * get the widget id of the options menu that this even came from
	 */

	option_list = (Widget) clientD;

	/*
	 * get the widget id of the selected item in the option menu
	 */
	XtVaGetValues(option_list,
		XmNmenuHistory, &selected_option,
		NULL);

	/*
	 * get the index of the selected item in the option menu list
	 */
	XtVaGetValues(selected_option,
		XmNpositionIndex, &index,
		NULL);
	/*
	 * get the XmString and character values for the name of the
	 * selected device
	 */
	XtVaGetValues(w,
		XmNlabelString, &device_label,
		XmNuserData, &device_name,
		NULL);

	/*
	 * adjust the position index to get the real device index
	 */
	if (streq(device_name, PFG_ANYSTRING))
		device_index = -1;
	else if (disk_fdisk_exists(first_disk()))
		device_index = index + 1;
	else
		device_index = index;

	write_debug(GUI_DEBUG_L1, "The device index is %d", device_index);
	write_debug(GUI_DEBUG_L1, "The current boot device is %s", device_name);

	/*
	 * set the boot object attributes according to the user's selection
	 */
	(void) BootobjSetAttribute(CFG_CURRENT,
		BOOTOBJ_DEVICE, device_index,
		BOOTOBJ_DEVICE_EXPLICIT, device_index == -1 ? 0 : 1,
		NULL);

}

/* ARGUSED */
void
change_toggle_states(Widget w, XtPointer clientD, XmToggleButtonCallbackStruct *state)
{
	Widget		toggle;
	int		i;
	char		*toggle_label;
	Widget		options;
	Widget		selected_option;
	char		*label;
	short		index;
	int		count;

	/*
	 * loop through all of the disks that are selected
	 * (they are choices on this screen)
	 */
	if (numDisks > 1)
		count = numDisks + 1;
	else
		count = numDisks;

	for (i = 0; i < count; i++) {
		/*
		 * get the toggle button and option menu values for the
		 * i'th disk
		 */
		toggle = pfgGetNamedWidget(disk_id1_widget_list[i], "diskToggle1");
		options = option_menu[i];
		if (toggle == w) {
			/*
			 * the widget that set off the callback is the
			 * current one we're looking at in the list
			 */
			if (state->set == False) {
				/*
				 * the toggle button was turned off,
				 * grey out the option menu
				 */
				if (options != NULL)
					XtSetSensitive(options, False);
			} else {
				/*
				 * the toggle button was turned on,
				 * turn on the option menu
				 */
				if (options != NULL) {
					XtSetSensitive(options, True);
					/*
					 * get the widget id of the selected button
					 * and the character version of the
					 * label on the button (it's the device name)
					 */
					XtVaGetValues(options,
						XmNmenuHistory, &selected_option,
						NULL);
					XtVaGetValues(selected_option,
						XmNuserData, &label,
						NULL);

					write_debug(GUI_DEBUG_L1,
						"The current Boot device is %s",
						label);
					/*
					 * get the index of the selected item in the
					 * option menu
					 */

					XtVaGetValues(selected_option,
						XmNpositionIndex, &index,
						NULL);


					/*
					 * adjust the position index to get the real
					 * device index
					 */
					if (streq(label, PFG_ANYSTRING))
				 		device_index = -1;
				 	else if (disk_fdisk_exists(first_disk()))
				 		device_index = index + 1;
				 	else
				 		device_index = index;


					write_debug(GUI_DEBUG_L1,
						"The device index is %d",
						device_index);
				}

				/*
				 * get the value for the selected disk
				 */
				XtVaGetValues(w,
					XmNuserData, &toggle_label,
					NULL);

				if (toggle_label != NULL)
					selected_disk = xstrdup(toggle_label);
				else
					selected_disk = xstrdup("");

				if (selected_disk != NULL) {
					write_debug(GUI_DEBUG_L1,
						"The Boot disk is %s",
						selected_disk);
				} else if (selected_disk == NULL) {
					write_debug(GUI_DEBUG_L1,
						"The Boot disk is No preference");
				}

				(void) BootobjSetAttribute(CFG_CURRENT,
					BOOTOBJ_DISK, selected_disk,
					BOOTOBJ_DISK_EXPLICIT,
						(strcmp(selected_disk, "") == 0) ? 0 : 1,
					NULL);

			}
		} else {
			/*
			 * the widget that set off the callback is NOT the
			 * current one we're looking at in the list
			 */

			/*
			 * grey out the option menu
			 */
			if (options != NULL)
				XtSetSensitive(options, False);

			/*
			 * set the toggle button state to False
			 */
			if (toggle != NULL) {
				XtVaSetValues(toggle,
					XmNset, False,
					NULL);
			}
		}

	}


}

void
set_prom_update(Widget w, XtPointer clientD, XmToggleButtonCallbackStruct *state)
{
	if (state->set == False) {
		BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_PROM_UPDATE, 0,
			NULL);
	} else {
		BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_PROM_UPDATE, 1,
			NULL);
	}
}
 return widget_array[WI_PROMTOGGLE];
}



/****************************************************************
 * create_method:
 *     This function creates a widget hierarchy using the
 *     functions generated above.
 ****************************************************************/
static Widget create_method(char               * temp,
                            char               * name,
                            Widget               parent,
                            Display            * disp,
                            Screen             * screen,
                            tu_template_descr  * retval)
{
  Widget w;

  sDisplay = disp;
  sScreen = screen;

  /* check each node against its name and call its
   * create function if appropriate */
  w = NULL;
  if (strcmp(temp, "bootdisk_dialog2") == 0){
    w = tu_bootdisk_dialog2_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "disk_id1") == 0){
    w = tu_disk_id1_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "promToggle") == 0){
    w = tu_promToggle_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

