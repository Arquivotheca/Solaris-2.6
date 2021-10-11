#include <stdio.h>
#include <stdlib.h>
#include <sys/ddi.h>
#include <Xm/List.h>

#include "util.h"
#include "launcher.h"

extern AppInfo 	* itemToAppInfo(XmString);

Widget
which_list(int ** index_list, int * cnt)
{
	Widget list_w = NULL;

	if (XmListGetSelectedPos(configContext->c_appList, 
			index_list, cnt))
		list_w = configContext->c_appList;
	else if (XmListGetSelectedPos(configContext->c_hideappList,
			index_list, cnt))
		list_w = configContext->c_hideappList;
	return(list_w);
}

set_edit_functions(configContext_t * c, 
			Boolean rm_flag, 
			Boolean prop_flag, 
			visibility_t v)
{
	XtVaSetValues(c->c_propertyButton, XmNsensitive, prop_flag, NULL);
	XtVaSetValues(c->c_rmAppButton, XmNsensitive, rm_flag, NULL);
	XtVaSetValues(c->c_hideButton, XmNsensitive, v == HIDE, NULL);
	XtVaSetValues(c->c_showButton, XmNsensitive, v == SHOW, NULL);
}


void
selectionCB(Widget w, XtPointer cd, XmListCallbackStruct * cbs)
{
	AppInfo * ai;
	configContext_t * c = (configContext_t *) cd;
	Boolean rm_flag = True;

	if (cbs->selected_item_count) {
	    ai = itemToAppInfo(cbs->item);
	    if (ai)
	    	rm_flag = (ai->a_site == LOCAL);

	    if (w == c->c_appList) {
		/* turn off selection in Hide list */
		XmListDeselectAllItems(c->c_hideappList);
		set_edit_functions(c, rm_flag, True, HIDE);
	    } else { 
		/* turn off selection in Show list */
		XmListDeselectAllItems(c->c_appList);
		set_edit_functions(c, rm_flag, True, SHOW);
	    }
	}
	else {
		set_edit_functions(c, False, False, -1);
	}
}

void
doubleclickCB( Widget w, XtPointer cd, XmListCallbackStruct * cbs)
{
	configContext_t * c = (configContext_t *) cd;

	show_propertyDialog(c);
}

void
delete_appList_entry(Widget list_w, int ix) 
{
	XmListDeletePos(list_w, ix);
}

void
add_appList_entry(Widget list_w, int ix, char * name)
{
	configContext_t * c;
	XmString xmstr;

	c = configContext;

	xmstr = XmStringCreateLocalized(name);
	XmListAddItem(list_w, xmstr, ix);
	XmStringFree(xmstr);
}

void
replace_appList_entry(Widget list_w, int ix, char * name)
{
	configContext_t * c;
	XmString xmstr;

	c = configContext;

	xmstr = XmStringCreateLocalized(name);
	XmListReplaceItemsPos(list_w, &xmstr, 1, ix);
	XmStringFree(xmstr);
}

/*
 * Write each apps name to List widget
 * Return no. of items that will be shown
 * in list.
 */
int	
display_appList(Widget list_w, visibility_t status)
{
	int 			i;
	XmString		*xstrtab;
	int			cnt = launcherContext->l_appCount;
	int			show_cnt = 0;

	if (cnt == 0)
		return;

	xstrtab = (XmString*)malloc(cnt * sizeof(XmString));

	for (i=0; i < cnt; i++) 
		if (launcherContext->l_appTable[i].a_show == status) {
			xstrtab[show_cnt] = XmStringCreateLocalized(launcherContext->l_appTable[i].a_appName);
			show_cnt++;
		}

	XmListDeleteAllItems(list_w);
	if (show_cnt)
		XmListAddItems(list_w, xstrtab, show_cnt, 1);

	for (i=0; i < show_cnt; i++)
		XmStringFree(xstrtab[i]);

	free_mem(xstrtab);
	return(show_cnt);
}
	

void
swap_appList_entries(Widget list_w, int i0, int i1)
{
	XmString i0str, i1str;
	XmStringTable items;
	int	cnt;
	
	XtVaGetValues(list_w, XmNitemCount, &cnt, XmNitems, &items, NULL);

	i0str = XmStringCopy(items[i0-1]);
	i1str = XmStringCopy(items[i1-1]);
	XmListReplaceItemsPos(list_w, &i1str, 1, i0);
	XmListReplaceItemsPos(list_w, &i0str, 1, i1);
	XmStringFree(i0str);
	XmStringFree(i1str);
}

XmString
get_appList_item(Widget list_w, int ix)
{
	int icnt;
	XmStringTable	items;

	XtVaGetValues(list_w, XmNitems, &items, XmNitemCount, &icnt, NULL);

	if (ix < 1 || ix > icnt)
		return(NULL);
	return(items[ix-1]);
}


