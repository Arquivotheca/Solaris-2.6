#ifndef lint
#pragma ident "@(#)pfgusedisks.c 1.43 96/10/02 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun Microsystems, Inc. All rights reserved.
 */

/*
 * Module:	pfgusedisks.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfglocales.h"

#include "pfgUseDisks_ui.h"

Widget pfgCreateDoubleList(void);

static void disksContinueCB(Widget, XtPointer, XtPointer);
static void disksRemoveCB(Widget, XtPointer, XtPointer);
static void disksAddCB(Widget, XtPointer, XtPointer);
static void disksRemoveAllCB(Widget remove, XtPointer clientD,
	XtPointer callD);
static void disksAutoAddCB(Widget remove, XtPointer clientD, XtPointer callD);
static void update_totals(SelLists *list);
static void disksListCB(Widget, XtPointer, XmListCallbackStruct *);
static void solarCB(Widget, XtPointer, XtPointer);
static int pfgAltBootQuery(char *);
static int pfgNoBootQuery(char *);
static void pfgDisksSetListAttributes(SelLists *lists);
static void pfgDiskMakeListItems(XmString **unselectItems, int *uCount,
	XmString **selectItems, int *sCount);
void pfgDiskPopulateLists(void);
static Disk_t *pfgGetDiskPtrFromXmName(XmString xmstr);
static int pfgDiskSelectedIsBootDisk(SelLists *lists);
static void remove_disk_from_list(XmString *, int, Disk_t *, SelLists *);
static Boolean first = True;

static Widget usedisks_dialog = NULL;
static SelLists *lists;

static WidgetList widget_list = NULL;

#define	PFG_UD_FMT	"%*d"

Widget
pfgCreateUseDisks(void)
{
	Widget useDisk;
	Disk_t	*bootdisk;
	Disk_t	*dp;
	int num;

	/*
	 * commit current disk configurations so we can reset back to them
	 * if necessary
	 */
	pfgCommitDisks();

	(void) DiskobjFindBoot(CFG_CURRENT, &bootdisk);

	/* attempt auto-selecting disks */
	if (first) {
		if (bootdisk != NULL) {
			WALK_DISK_LIST(dp) {
				if (dp == bootdisk)
					DiskAutoSelect(dp);
			}
		}
		first = False;
	}

	useDisk = pfgCreateDoubleList();
	pfgDisksSetListAttributes(lists);
	XtVaGetValues(lists->selectList,
		XmNitemCount, &num,
		NULL);

	if (num > 0)
		XtSetSensitive(pfgGetNamedWidget(widget_list, "changeButton"), True);
	else if (num == 0)
		XtSetSensitive(pfgGetNamedWidget(widget_list, "changeButton"), False);


	return (useDisk);
}

Widget
pfgCreateDoubleList(void)
{
	char *minChar, *recommChar;
	int minTotal, recommTotal;
	Dimension height, width;
	char	part_char;
	int	index;
	Disk_t *bootDisk;
	Widget	label;
	char	buf[150];

	Disk_t *disk_ptr;

	if (widget_list)
		free(widget_list);

	widget_list = NULL;

	lists = (SelLists *) xmalloc(sizeof (SelLists));

	usedisks_dialog = tu_usedisks_dialog_widget("usedisks_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(usedisks_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(usedisks_dialog),
		XmNdeleteResponse, XmDO_NOTHING,
		XmNtitle, TITLE_USEDISKS,
		NULL);
	pfgSetWidgetString(widget_list, "panelhelpText", MSG_USEDISKS);
	pfgSetWidgetString(widget_list, "availableLabel", PFG_UD_DONOTUSE);
	pfgSetWidgetString(widget_list, "selectedLabel", PFG_UD_USE);
	pfgSetWidgetString(widget_list, "addButton", PFG_UD_ADD);
	pfgSetWidgetString(widget_list, "removeButton", PFG_UD_REMOVE);
	pfgSetWidgetString(widget_list, "autoAddButton", PFG_UD_AUTO_ADD);
	pfgSetWidgetString(widget_list, "removeAllButton", PFG_UD_REMOVE_ALL);
	pfgSetWidgetString(widget_list, "changeButton", PFG_SELECTROOT);

	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "gobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "exitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	(void) DiskobjFindBoot(CFG_CURRENT, &bootDisk);
	if (bootDisk != NULL ) {
		/*
		 * get the device index for the boot device
		 */
		BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DEVICE, &index,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			NULL);
		if (index == -1 || part_char == 'p') {
			(void) sprintf(buf, PFG_BOOTDISKLABEL, disk_name(bootDisk));
		} else {
			(void) sprintf(buf, PFG_BOOTDEVICELABEL, disk_name(bootDisk),
						part_char, index);
		}
		pfgSetWidgetString(widget_list, "boot_label", buf);

	} else {
		XtUnmanageChild(pfgGetNamedWidget(widget_list, "boot_label"));
	}


	XtAddCallback(pfgGetNamedWidget(widget_list, "addButton"),
		XmNactivateCallback, disksAddCB, lists);
	XtAddCallback(pfgGetNamedWidget(widget_list, "removeButton"),
		XmNactivateCallback, disksRemoveCB, lists);
	XtAddCallback(pfgGetNamedWidget(widget_list, "autoAddButton"),
		XmNactivateCallback, disksAutoAddCB, lists);
	XtAddCallback(pfgGetNamedWidget(widget_list, "removeAllButton"),
		XmNactivateCallback, disksRemoveAllCB, lists);

	pfgDiskPopulateLists();

	/*
	 * create Edit solaris partition button if we are on an
	 * intel
	 */
	if (IsIsa("i386")) {
		pfgSetWidgetString(widget_list, "solarButton", PFG_UD_EDIT);
		XtAddCallback(pfgGetNamedWidget(widget_list, "solarButton"),
			XmNactivateCallback, solarCB, lists->selectList);
	} else {
		XtUnmanageChild(pfgGetNamedWidget(widget_list, "solarButton"));
	}

	pfgSetWidgetString(widget_list, "totalAvailableLabel", PFG_UD_TOTAVA);

	recommChar = (char *) xmalloc(strlen(PFG_UD_RECCOM) +
		MIN_TOTAL_SIZE + 1);
	recommTotal = DiskGetContentDefault();
	(void) sprintf(recommChar, PFG_UD_FMT, MIN_TOTAL_SIZE, recommTotal);
	pfgSetWidgetString(widget_list, "recommendLabel", PFG_UD_RECCOM);
	pfgSetWidgetString(widget_list, "recommendValue", recommChar);
	free(recommChar);

	minTotal = DiskGetContentMinimum();
	minChar = (char *) xmalloc(MIN_TOTAL_SIZE + 1);
	(void) sprintf(minChar, PFG_UD_FMT, MIN_TOTAL_SIZE, minTotal);
	pfgSetWidgetString(widget_list, "minimumLabel", PFG_UD_MINIMUM);
	pfgSetWidgetString(widget_list, "minimumValue", minChar);
	free(minChar);

	pfgSetWidgetString(widget_list, "useTotalLabel", PFG_UD_TOTSEL);

	XtVaSetValues(lists->selectList,
		XmNuserData, pfgGetNamedWidget(widget_list, "useTotalValue"),
		NULL);

	XtVaSetValues(lists->unselectList,
		XmNuserData,
			pfgGetNamedWidget(widget_list, "totalAvailableValue"),
		NULL);
	update_totals(lists);

	XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, disksContinueCB, lists);

	XtAddCallback(lists->unselectList, XmNbrowseSelectionCallback,
		disksListCB, (XtPointer) lists);
	XtAddCallback(lists->selectList, XmNbrowseSelectionCallback,
		disksListCB, (XtPointer) lists);

	XtAddCallback(lists->unselectList, XmNdefaultActionCallback,
		disksListCB, (XtPointer) lists);
	XtAddCallback(lists->selectList, XmNdefaultActionCallback,
		disksListCB, (XtPointer) lists);

	XtManageChild(usedisks_dialog);

	XtVaGetValues(pfgShell(usedisks_dialog),
		XmNwidth, &width,
		XmNheight, &height,
		NULL);

	XtVaSetValues(pfgShell(usedisks_dialog),
		XmNminWidth, width,
		XmNminHeight, height,
		NULL);

	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	return (usedisks_dialog);
}

static void
pfgDiskMakeListItems(XmString **unselectItems, int *uCount,
	XmString **selectItems, int *sCount)
{
	Disk_t *disk_ptr;
	XmString *sitems = NULL;
	XmString *uitems = NULL;
	int scnt = 0;
	int ucnt = 0;
	char *tmp;
	int sIndex = 0;
	int uIndex = 0;

	WALK_DISK_LIST(disk_ptr) {
		if (disk_selected(disk_ptr)) {
			scnt++;
		} else {
			ucnt++;
		}
	}

	if (ucnt)
		uitems = (XmString *)
		    xmalloc(sizeof (XmString) * ucnt);

	if (scnt)
		sitems = (XmString *) xmalloc(sizeof (XmString) * scnt);

	WALK_DISK_LIST(disk_ptr) {
		tmp = DiskMakeListName(disk_ptr, disk_selected(disk_ptr));
		if (disk_selected(disk_ptr)) {
#if 0
/* do something to make the boot disk always first in list ? */
			/* boot disk always first */
			if (DiskIsCurrentBootDisk(disk_ptr)) {
#endif
			sitems[sIndex++] = XmStringCreateLocalized(tmp);
		} else {
			uitems[uIndex++] = XmStringCreateLocalized(tmp);
		}
		if (tmp)
			free(tmp);
	}

	*sCount = scnt;
	*uCount = ucnt;
	*selectItems = sitems;
	*unselectItems = uitems;
}

void
pfgDiskPopulateLists(void)
{
	XmString *unselectItems;
	XmString *selectItems;
	int uCount;
	int sCount;
	int i;

	pfgDiskMakeListItems(&unselectItems, &uCount, &selectItems, &sCount);

	lists->unselectList =
		pfgGetNamedWidget(widget_list, "availableScrolledList");
	XtVaSetValues(lists->unselectList,
		XmNitems, unselectItems,
		XmNitemCount, uCount,
		NULL);

	lists->selectList =
		pfgGetNamedWidget(widget_list, "selectedScrolledList");
	XtVaSetValues(lists->selectList,
		XmNitems, selectItems,
		XmNitemCount, sCount,
		NULL);

	if (unselectItems != NULL) {
		for (i = 0; i < uCount; ++i)
			XmStringFree(unselectItems[i]);
		free(unselectItems);
	}
	if (selectItems != NULL) {
		for (i = 0; i < sCount; ++i)
			XmStringFree(selectItems[i]);
		free(selectItems);
	}

	XmListDeselectAllItems(lists->unselectList);
	XmListDeselectAllItems(lists->selectList);
}

static void
pfgDisksSetListAttributes(SelLists *lists)
{
	XmString *items = NULL;
	int num_selected;
	int num_total;


	write_debug(GUI_DEBUG_L1, "Entering pfgDisksSetListAttributes");

	if (!lists)
		return;

	/*
	 * Set things based on the state of the unselected list...
	 */
	XtVaGetValues(lists->unselectList,
		XmNselectedItems, &items,
		XmNselectedItemCount, &num_selected,
		XmNitemCount, &num_total,
		NULL);

	write_debug(GUI_DEBUG_L1, "Unselected List:");
	write_debug(GUI_DEBUG_NOHD, LEVEL2, "num_selected = %d", num_selected);
	write_debug(GUI_DEBUG_NOHD, LEVEL2, "num_total = %d", num_total);

	XtSetSensitive(
		pfgGetNamedWidget(widget_list, "autoAddButton"),
		(Boolean) num_total);
	XtSetSensitive(pfgGetNamedWidget(widget_list, "addButton"),
		(Boolean) num_selected);

	/*
	 * Set things based on the state of the selected list...
	 */
	XtVaGetValues(lists->selectList,
		XmNselectedItems, &items,
		XmNselectedItemCount, &num_selected,
		XmNitemCount, &num_total,
		NULL);

	if (num_total > 0)
		XtSetSensitive(pfgGetNamedWidget(widget_list, "changeButton"), True);
	else if (num_total == 0)
		XtSetSensitive(pfgGetNamedWidget(widget_list, "changeButton"), False);

	write_debug(GUI_DEBUG_L1, "Selected List:");
	write_debug(GUI_DEBUG_NOHD, LEVEL2, "num_selected = %d", num_selected);
	write_debug(GUI_DEBUG_NOHD, LEVEL2, "num_total = %d", num_total);

	XtSetSensitive(
		pfgGetNamedWidget(widget_list, "removeAllButton"),
		num_total > 0 ? True : False);
	if (num_selected) {
		XtSetSensitive(pfgGetNamedWidget(widget_list,
			"removeButton"), True);
	} else {
		XtSetSensitive(pfgGetNamedWidget(widget_list, "removeButton"),
			False);
	}

	if (IsIsa("i386")) {
		XtSetSensitive(
			pfgGetNamedWidget(widget_list, "solarButton"),
			(Boolean) num_selected);
	}

}

/* ARGSUSED */
void
disksRemoveCB(Widget remove, XtPointer clientD, XtPointer callD)
{
	XmString *selectItems;
	int sCount;
	SelLists *lists;
/* 	int ret; */
	Disk_t *diskPtr;
	char *str;
	Disk_t	*bootDisk;
	char	buf[150];
	int	dev_index;
	char	part_char;

	/* LINTED [pointer cast] */
	lists = (SelLists *) clientD;

	XtVaGetValues(lists->selectList,
		XmNselectedItems, &selectItems,
		XmNselectedItemCount, &sCount,
		NULL);

	diskPtr = pfgGetDiskPtrFromXmName(*selectItems);

	/*
	 * check to see if the disk the user is trying to deselect is
	 * the boot disk
	 */
	(void) DiskobjFindBoot(CFG_CURRENT, &bootDisk);
	if (bootDisk == diskPtr) {
		/*
		 * get the device index for the boot device
		 */
		BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DEVICE, &dev_index,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			NULL);
		if (dev_index == -1 || part_char == 'p') {
			(void) sprintf(buf, MSG_DESELECT_BOOT, disk_name(diskPtr));
		} else {
			(void) sprintf(buf, MSG_DESELECT_BOOT1, disk_name(diskPtr),
						part_char, dev_index);
		}
		if (!pfgAppQuery(XtParent(remove), buf)) {
			return;
		} else {
			remove_disk_from_list(selectItems, sCount,  diskPtr, lists);
		}

	} else {
			remove_disk_from_list(selectItems, sCount, diskPtr, lists);

	}
}

/* ARGUSED */
static void remove_disk_from_list(XmString *selected_items,
			int count,
			Disk_t *disk,
			SelLists *disk_lists)
{
	char *tmp;
	XmString newItems;

	if (disk == NULL) {
		return;
	}
	if (IsDiskModified(disk_name(disk))) {
		if (!pfgQuery(XtParent(remove), pfQCUSTOM)) {
			return;
		}
	}

	(void) deselect_disk(disk, NULL);
/*
 *	ret = deselect_disk(diskptr, NULL);
 *	if (ret != D_OK) {
 *		pfDiskErr(disk_name(disk), ret);
 *	}
 */

	tmp = DiskMakeListName(disk, 0);
	newItems = XmStringCreateLocalized(tmp);

	XmListAddItem(disk_lists->unselectList, newItems, 0);
	XmListDeleteItems(disk_lists->selectList, selected_items, count);
	XmStringFree(newItems);
	if (tmp)
		free(tmp);

	update_totals(disk_lists);

	XmListDeselectAllItems(disk_lists->unselectList);
	XmListDeselectAllItems(disk_lists->selectList);

	pfgDisksSetListAttributes(disk_lists);
}


/* ARGSUSED */
static void
disksRemoveAllCB(Widget remove, XtPointer clientD, XtPointer callD)
{
	/* LINTED [pointer cast] */
	SelLists *lists = (SelLists *) clientD;
	Disk_t *disk_ptr;
	XmString xmstr;
	char *tmp;

	WALK_DISK_LIST(disk_ptr) {
		if (!disk_selected(disk_ptr)) {
			continue;
		}

		if (IsDiskModified(disk_name(disk_ptr))) {
			/* should we do anything here?  -
			 * like warn them of deselecting a disk
			 * they've modified...
			 */
		}

		(void) deselect_disk(disk_ptr, NULL);

		tmp = DiskMakeListName(disk_ptr, 1);
		write_debug(GUI_DEBUG_L1, "Removing %s", tmp);
		xmstr = XmStringCreateLocalized(tmp);
		free(tmp);
		XmListDeleteItem(lists->selectList, xmstr);

		tmp = DiskMakeListName(disk_ptr, 0);
		xmstr = XmStringCreateLocalized(tmp);
		XmListAddItem(lists->unselectList, xmstr, 0);
		XmStringFree(xmstr);
	}

	update_totals(lists);

	XmListDeselectAllItems(lists->unselectList);
	XmListDeselectAllItems(lists->selectList);
	pfgDisksSetListAttributes(lists);
}

/* ARGSUSED */
static void
disksAutoAddCB(Widget remove, XtPointer clientD, XtPointer callD)
{
	DiskAutoSelect(NULL);
	pfgDiskPopulateLists();
	XmListDeselectAllItems(lists->unselectList);
	XmListDeselectAllItems(lists->selectList);
	pfgDisksSetListAttributes(lists);
	update_totals(lists);
}

/* ARGSUSED */
void
disksAddCB(Widget add, XtPointer clientD, XtPointer callD)
{
	XmString *unselectItems, newItems;
	int uCount;
	SelLists *lists;
	char diskName[20];
	Disk_t *disk_ptr;
	int ret;
	char *str;
	char *tmp;
	char	*buf;

	/* LINTED [pointer cast] */
	lists = (SelLists *) clientD;
	XtVaGetValues(lists->unselectList,
		XmNselectedItems, &unselectItems,
		XmNselectedItemCount, &uCount,
		NULL);

	disk_ptr = pfgGetDiskPtrFromXmName(*unselectItems);
	if (disk_ptr == NULL) {
		return;
	}

	if (disk_not_okay(disk_ptr) || disk_unusable(disk_ptr)) {
		char *msg;

		if (disk_bad_controller(disk_ptr)) {
			msg = DISK_PREP_BAD_CONTROLLER;
		} else if (disk_unk_controller(disk_ptr)) {
			msg = DISK_PREP_UNKNOWN_CONTROLLER;
		} else if (disk_cant_format(disk_ptr)) {
			msg = DISK_PREP_CANT_FORMAT;
		} else if (disk_no_pgeom(disk_ptr)) {
			msg = DISK_PREP_NOPGEOM;
		}

		buf = (char *)xmalloc(strlen(DISK_PREP_DISK_HOSED) +
			strlen(disk_name(disk_ptr)) +
			strlen(msg) + 32);

		sprintf(buf, DISK_PREP_DISK_HOSED, disk_name(disk_ptr), msg);
                
		pfAppError(NULL, buf);

		free(buf);
		return;
	}
	ret = select_disk(disk_ptr, NULL);
/*
	if (ret != D_OK) {
		pfgDiskError(usedisks_dialog, NULL, ret);
		return;
	}
*/

	if (disk_fdisk_req(disk_ptr)) {
		if (!get_solaris_part(disk_ptr, CFG_CURRENT)) {
			pfgCreateSolarPart(usedisks_dialog, disk_ptr);
			return;
		}
	}
	tmp = DiskMakeListName(disk_ptr, 1);
	newItems = XmStringCreateLocalized(tmp);

	XmListAddItem(lists->selectList, newItems, 0);
	XmListDeleteItems(lists->unselectList, unselectItems, uCount);
	XmStringFree(newItems);
	if (tmp)
		free(tmp);

	XmListDeselectAllItems(lists->unselectList);
	XmListDeselectAllItems(lists->selectList);
	pfgDisksSetListAttributes(lists);

	update_totals(lists);
}

/* ARGSUSED */
void
disksContinueCB(Widget w, XtPointer clientD, XtPointer callD)
{
	XmString *items;
	int count;
	SelLists *lists;
	Widget sList;
	char *str, *diskName, buf2[100];
	Boolean	ret;
	Disk_t	*bootDisk;

	/* LINTED [pointer cast] */
	lists = (SelLists *) clientD;
	sList = lists->selectList;
	XtVaGetValues(sList,
		XmNitems, &items,
		XmNitemCount, &count,
		NULL);

	 /* validate at least one disk in list */
	  
	if (count < 1) {
		pfWarn(pfErNODISKSUSED, pfErMessage(pfErNODISKSUSED), NULL);
		return;
	}

	write_debug(GUI_DEBUG_L1, "more than one disk");

	/* validate size or warn */
	if (DiskGetListTotal(1) < DiskGetContentMinimum()) {
		pfAppError(NULL, PFG_MN_BELOW);
		return;
	}
	write_debug(GUI_DEBUG_L1, "enough disk space selected");

	(void) BootobjCommit();
	write_debug(GUI_DEBUG_L1, "After BootobjCommit");
	(void) DiskobjFindBoot(CFG_CURRENT, &bootDisk);
	write_debug(GUI_DEBUG_L1, "after DiskobjFindBoot");
	if (bootDisk != NULL)
		diskName = disk_name(bootDisk);
	else
		diskName = "";

	/* query user if boot drive not selected */
	if (pfgIsBootSelected() == FALSE && count != 0) {
		ret = pfgNoBootQuery(diskName);
		if (ret == FALSE) {
			return;
		}
	} else {
		ret = True;
	}

	if (ret == True) {

		free(lists);

		if (widget_list)
			free(widget_list);

		widget_list = NULL;


		/*
		 * don't update profile, generate at the summary screen
		 * pfUpdateUseList();
		 */
		pfgSetAction(parAContinue);
	}
}

static void
update_totals(SelLists *lists)
{
	Widget sLabel, uLabel;
	XmString xmstr;
	char buf[120];

	if (!lists)
		return;

	XtVaGetValues(lists->unselectList,
		XmNuserData, &uLabel,
		NULL);
	XtVaGetValues(lists->selectList,
		XmNuserData, &sLabel,
		NULL);

	(void) sprintf(buf, PFG_UD_FMT, MIN_TOTAL_SIZE,
		DiskGetListTotal(0));
	xmstr = XmStringCreateLocalized(buf);
	XtVaSetValues(uLabel, XmNlabelString, xmstr, NULL);
	XmStringFree(xmstr);

	(void) sprintf(buf, PFG_UD_FMT, MIN_TOTAL_SIZE,
		DiskGetListTotal(1));
	xmstr = XmStringCreateLocalized(buf);
	XtVaSetValues(sLabel, XmNlabelString, xmstr, NULL);
	XmStringFree(xmstr);
}

/* ARGSUSED */
static void
disksListCB(Widget list, XtPointer client, XmListCallbackStruct *cbs)
{
	Widget left, right;
	SelLists *lists;

	/* LINTED [pointer cast] */
	lists = (SelLists *) client;
	left = lists->unselectList;
	right = lists->selectList;

	write_debug(GUI_DEBUG_L1, "Entering disksListCB");

	if (list == left)
		XmListDeselectAllItems(right);
	else if (list == right)
		XmListDeselectAllItems(left);

	if (cbs->reason == XmCR_DEFAULT_ACTION) {	/* double click */
#if 0
		if (pfgDiskSelectedIsBootDisk(lists)) {
			pfgDisksSetListAttributes(lists);
			return;
		}
#endif

		if (list == left)
			disksAddCB(list, client, (XtPointer) NULL);
		else
			disksRemoveCB(list, client, (XtPointer) NULL);

	} else {		/* single click */
		pfgDisksSetListAttributes(lists);
	}
}

/* ARGSUSED */
void
solarCB(Widget w, XtPointer selectList, XtPointer callD)
{
	Disk_t *disk_ptr;
	int count;
	XmString *items;
	char *str, diskName[32];

	XtVaGetValues(selectList,
		XmNselectedItems, &items,
		XmNselectedItemCount, &count,
		NULL);
	if (count > 0) {
		disk_ptr = pfgGetDiskPtrFromXmName(*items);
		if (disk_ptr == NULL) {
			return;
		}

		pfgBusy(pfgShell(w));

		pfgCreateSolarCust(usedisks_dialog, disk_ptr);
	}
}

static int
pfgDiskSelectedIsBootDisk(SelLists *lists)
{
	XmString *selected_items;
	int count;
	Disk_t	*disk_ptr;
	int ret;

	write_debug(GUI_DEBUG_L1, "pfgDiskSelectedIsBootDisk");

	XtVaGetValues(lists->selectList,
		XmNselectedItems, &selected_items,
		XmNselectedItemCount, &count,
		NULL);

	if (!count)
		return 0;

	disk_ptr = pfgGetDiskPtrFromXmName(*selected_items);
	if (!disk_ptr)
		return 0;

	ret = DiskIsCurrentBootDisk(disk_ptr);
	if (ret)
		write_debug(GUI_DEBUG_L1,
			"%s is the boot disk\n", disk_name(disk_ptr));

	return(ret);

}

static Disk_t *
pfgGetDiskPtrFromXmName(XmString xmstr)
{
	char *str;
	char buf[80];
	Disk_t *disk_ptr;

	XmStringGetLtoR(xmstr, XmSTRING_DEFAULT_CHARSET, &str);
	if (!str)
		return;
	(void) sscanf(str, "%s", buf);

	write_debug(GUI_DEBUG_L1,
		"pfgGetDiskPtrFromXmName: looking for %s", buf);

	XtFree(str);
	disk_ptr = find_disk(buf);

	return (disk_ptr);
}

/* move disk to use list if user request this */
void
moveDisk(Disk_t * diskPtr, int add)
{
	XmString *items, newItem[1];
	int count;
	Disk_t *ptr;
	char *str;
	char diskName[32];
	char tmp[80];
	char bootDiskIndicator[10];
	Disk_t *bootDisk;

	XtVaGetValues(!add ? lists->selectList : lists->unselectList,
		XmNselectedItems, &items,
		XmNselectedItemCount, &count,
		NULL);

	/* look for disk in selected list */

	if (count > 0) {
		XmStringGetLtoR(*items, XmSTRING_DEFAULT_CHARSET, &str);
		if (str == NULL) {
			return;
		}
		(void) sscanf(str, "%s", diskName);
		ptr = find_disk(diskName);
		if (ptr == NULL) {
			return;
		}
		if (ptr == diskPtr) {
			/* remove disk from list */
			if (add) {
				disksAddCB(pfgGetNamedWidget(widget_list,
						"addButton"),
						(XtPointer) lists, NULL);
			} else {
				/*
				 * recalculate sdisk size and replace
				 * current item with new item that refleclts
				 * correct size
				 */
				if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) == D_OK &&
						bootDisk != NULL) {
					if (strcmp(disk_name(ptr),
					disk_name(bootDisk)) == 0)
						(void) strcpy(bootDiskIndicator,
							"boot");
					else
						(void) strcpy(bootDiskIndicator,
							"");
				} else {
					(void) strcpy(bootDiskIndicator, "");
				}
				(void) sprintf(tmp, " %-6s %-9s %4d MB",
					diskName,
					bootDiskIndicator,
					get_solaris_part(ptr, CFG_CURRENT) ?
					blocks_to_mb_trunc(ptr,
					usable_sdisk_blks(ptr)) :
					0);
				newItem[0] = XmStringCreateLocalized(tmp);
				XmListReplaceItems(lists->selectList,
					items, 1, newItem);
				XmListSelectItem(lists->selectList, newItem[0], False);
				XmStringFree(newItem[0]);
			}
		}
	}
}

/* move disk to use list if user request this */
void
new_moveDisk(Disk_t * diskPtr, int add)
{
	XmString *items, newItem[1];
	int count;
	Disk_t *disk_ptr;
	char *str;
	char diskName[32];
	char *tmp;

	/*
	 * Skip this if fdisk config not brought up from Select Disks
	 * screen (i.e. from manual boot device route...)
	 */
	if (!usedisks_dialog)
		return;

	XtVaGetValues(add ? lists->unselectList : lists->selectList,
#if 1
		XmNselectedItems, &items,
		XmNselectedItemCount, &count,
#else
		XmNitems, &items,
		XmNitemCount, &count,
#endif
		NULL);

	/* look for disk in selected list */

	if (count > 0) {
		disk_ptr = pfgGetDiskPtrFromXmName(*items);
		if (disk_ptr == NULL) {
			return;
		}
		if (disk_ptr == diskPtr) {
			/* remove disk from list */
			if (add) {
				disksAddCB(pfgGetNamedWidget(widget_list,
					"addButton"),
					(XtPointer) lists, NULL);
			} else {
				/*
				 * recalculate sdisk size and replace
				 * current item with new item that reflects
				 * correct size
				 */
				tmp = DiskMakeListName(disk_ptr,
					disk_selected(disk_ptr));
#if 0
				newItem[0] = XmStringCreateLocalized(tmp);
				free(tmp);
				XmListReplaceItems(lists->selectList,
					items, 1, newItem);
				XmListSelectItem(lists->selectList,
					newItem[0], False);
				XmStringFree(newItem[0]);
#else
				disksRemoveCB(pfgGetNamedWidget(widget_list,
					"removeButton"),
					(XtPointer) lists, NULL);
#endif
			}
		}
	} else {
		/*
		 * it's staying in the add list, but the size is
		 * changing...
		 */
		XtVaGetValues(lists->selectList,
			XmNselectedItems, &items,
			XmNselectedItemCount, &count,
			NULL);
		disk_ptr = pfgGetDiskPtrFromXmName(*items);
		tmp = DiskMakeListName(disk_ptr, disk_selected(disk_ptr));
		newItem[0] = XmStringCreateLocalized(tmp);
		free(tmp);
		XmListReplaceItems(lists->selectList,
			items, 1, newItem);
		XmListSelectItem(lists->selectList,
			newItem[0], False);
		XmStringFree(newItem[0]);
	}
}

int
pfgAltBootQuery(char *diskName)
{
	Disk_t *  bootDisk;
	char *	  buff;
	Disk_t *  obp;
	int	 ret;

	if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) != D_OK ||
			bootDisk == NULL)
		return (TRUE);

	buff = xmalloc(strlen(MSG_ALTBOOT) + strlen(disk_name(bootDisk))
		+ strlen(diskName) + 100);
	(void) DiskobjFindBoot(CFG_EXIST, &obp);
	(void) sprintf(buff, MSG_ALTBOOT,
		obp == NULL ? gettext("unknown") : disk_name(obp), diskName);

	ret = pfgAppQuery(usedisks_dialog, buff);

	free(buff);

	return (ret);
}

int
pfgNoBootQuery(char *diskName)
{
	Disk_t *  bootDisk;
	char *	  buff;
	Disk_t *  obp;
	int	 ret;

	buff = xmalloc(strlen(MSG_NOBOOT) + strlen(diskName) + 100);
	(void) DiskobjFindBoot(CFG_EXIST, &obp);
	(void) sprintf(buff, MSG_NOBOOT,
		obp == NULL ? gettext("unknown") : disk_name(obp));

	ret = pfgChangeBootQuery(usedisks_dialog, buff);

	free(buff);

	return(ret);

}

int
pfgChangeBootQuery(Widget parent, char * buff)
{
	UI_MsgStruct *msg_info;
	int answer;

	write_debug(GUI_DEBUG_L1, "Entering pfgChangeBootQuery");

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->title = TITLE_BOOTDISKQUERY;
	msg_info->msg = buff;
	msg_info->help_topic = NULL;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_OK_STR;
	msg_info->btns[UI_MSGBUTTON_OTHER1].button_text = PFG_CHANGEBOOT;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = UI_BUTTON_CANCEL_STR;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	switch (UI_MsgResponseGet()) {
	case UI_MSGRESPONSE_OK:
		/*
		 * the user is letting us pick a boot device
		 * so wildcard the disk and device here
		 */
		BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, NULL,
			BOOTOBJ_DISK_EXPLICIT, 0,
			BOOTOBJ_DEVICE, -1,
			BOOTOBJ_DEVICE_EXPLICIT, 0,
			NULL);
		answer = TRUE;
		break;
	case UI_MSGRESPONSE_CANCEL:
		answer = FALSE;
		break;
	case UI_MSGRESPONSE_OTHER1:
		/*
		 * the user wants to define a boot device
		 * so bring up the screen that lets them do that
		 */
		pfgBusy(pfgShell(parent));
		(void) pfgCreateBootDisk(usedisks_dialog);
		answer = FALSE;
		break;
	}

	return (answer);
}


/* ARGSUSED */
void
disksGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgSetAction(parAGoback);
}
/* ARGUSED */
void
changeBootCB(Widget w, XtPointer clientD, XtPointer callD)
{
	Widget          dialog;

/*	XtUnmanageChild(pfgShell(w));
	XtDestroyWidget(pfgShell(w)); */

	dialog = pfgCreateBootDisk(usedisks_dialog);
}

