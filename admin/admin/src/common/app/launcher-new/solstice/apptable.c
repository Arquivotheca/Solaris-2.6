#include "util.h"
#include "launcher.h"


int
read_appTable(void) {
}

AppInfo *
itemToAppInfo(XmString item)
{
	char * aname;
	int n;

	XmStringGetLtoR(item, XmSTRING_DEFAULT_CHARSET, &aname);
	n = lookup_appTable_entry((const char *)aname);
	if (n == -1)
		return(NULL);
	return ( &(launcherContext->l_appTable[n]) );
}

AppInfo *
dup_appTable(AppInfo * appTable, int appCount) 
{
	AppInfo * new_tab = malloc(appCount * sizeof(AppInfo));

	if (new_tab == NULL)
		fatal(catgets(catd, 1, 1, "dup_appTable: unable to malloc"));

	memmove((void *)new_tab, (void *)appTable, appCount * sizeof(AppInfo));
	return(new_tab);
}

free_appTable(AppInfo * appTable)
{
	if (appTable)
		free(appTable);
}

#define TABLE_SEG_SZ 20

AppInfo *
newAppTable(int entries)
{
	size_t n;
	size_t fl_sz, ex_sz;
	launcherContext_t * c;

	XtVaGetValues(launchermain, XmNuserData, &c, NULL);

	if (entries > 0)
		c->l_appTableMax=entries > TABLE_SEG_SZ ? entries:TABLE_SEG_SZ;
	else
		c->l_appTableMax += TABLE_SEG_SZ;	

	n = sizeof(AppInfo) * c->l_appTableMax;

	if (c->l_appTable == NULL) {
		c->l_appTable = (AppInfo *)malloc(n);
		if (c->l_appTable == NULL)
			fatal(catgets(catd, 1, 2, "PANIC: newAppTable unable to malloc memory"));
		memset((void *)c->l_appTable, 0, n);
	} else {
		ex_sz = c->l_appTableMax * sizeof(AppInfo);
		fl_sz = c->l_appCount * sizeof(AppInfo);
		c->l_appTable = (AppInfo *)realloc(c->l_appTable, n);
		memset(c->l_appTable + fl_sz, 0, ex_sz - fl_sz);
	}

	return(c->l_appTable);
}

int
lookup_appTable_entry(const char * name)
{
	launcherContext_t * c;
	int i;

	if (name == NULL)
	    return(-1);

	XtVaGetValues(launchermain, XmNuserData, &c, NULL);

	for (i = 0; i < c->l_appCount; i++) {
		/* 
		 * A fail-safe test. If it ever true, there is strangeness.
 		 */
		if (c->l_appTable[i].a_appName == NULL)
			return(-1);
		if (strcmp(name, c->l_appTable[i].a_appName) == 0)
			return(i);
	}
	return(-1);
}
	
int
update_appTable_entry(const char * appName, 
		      const char * appPath, 
		      const char * appArgs,
		      const char * iconPath, 
		      const char * scriptName,
		      registry_loc_t site)
{
	launcherContext_t * c;
	AppInfo * ai;
	int	n = -1;

	XtVaGetValues(launchermain, XmNuserData, &c, NULL);

	if (!appName) {
		display_error(propertyDialog, ADD_NONAME_MSG);
		return;
	}
	if (!appPath) {
		display_error(propertyDialog, ADD_NOPATH_MSG);
		return;
	}

	if ((n = lookup_appTable_entry(appName)) >= 0) { /* update existing */
		ai = &(c->l_appTable[n]);
		ai->a_obsolete = False;
/*	
		ai->a_show = SHOW;
*/
		ai->a_site = site;
		ai->a_appName = strdup(appName);
		ai->a_appPath = strdup(appPath);
		ai->a_appArgs = strdup(appArgs);
		ai->a_iconPath = strdup(iconPath);
		if (scriptName)
			ai->a_scriptName = strdup(scriptName);
		else
			ai->a_scriptName = NULL;
		return(n);
	}
	if (c->l_appCount == c->l_appTableMax) 
		newAppTable(NULL);

	n = c->l_appCount;
	ai = &(c->l_appTable[c->l_appCount]);
	memset((void *)ai, 0, sizeof(AppInfo));

	ai->a_obsolete = False;
	ai->a_show = (site == LOCAL) ? HIDE : SHOW;
	ai->a_site = site;
	ai->a_appName = strdup(appName);
	ai->a_appPath = strdup(appPath);
	ai->a_appArgs = strdup(appArgs);
	ai->a_iconPath = strdup(iconPath);
	if (scriptName)
		ai->a_scriptName = strdup(scriptName);
	else	
		ai->a_scriptName = NULL;

	c->l_appCount++;
	return(n);
}

void
remove_appTable_entry(const char * name)
{
	int n;
	AppInfo * ai = launcherContext->l_appTable;

	n = lookup_appTable_entry(name);
	if (n == -1)
		return;

	if (n < --launcherContext->l_appCount)
		memmove(&(ai[n]), &(ai[n+1]), 
			sizeof(AppInfo)*(launcherContext->l_appCount-n));
}
	

void
commitVisibility_appTable_entry()
{
	launcherContext_t * c;
	AppInfo * ai;
	int i;

	XtVaGetValues(launchermain, XmNuserData, &c, NULL);

	ai = c->l_appTable;
	for ( i = 0; i < c->l_appCount; i++) {
		if (ai[i].a_show == HIDE_PENDING)
			ai[i].a_show = HIDE;
		else if (ai[i].a_show == SHOW_PENDING)
			ai[i].a_show = SHOW;
	}
}

void
resetVisibility_appTable_entry()
{
	launcherContext_t * c;
	AppInfo * ai;
	int i;

	XtVaGetValues(launchermain, XmNuserData, &c, NULL);

	ai = c->l_appTable;
	for ( i = 0; i < c->l_appCount; i++) {
		if (ai[i].a_show == HIDE_PENDING)
			ai[i].a_show = SHOW;
		else if (ai[i].a_show == SHOW_PENDING)
			ai[i].a_show = HIDE;
	}
}

/*
 * App startup timer timeout callback.
 * The client_data is an appTable entry, appInfo
 *
 * timer and pid values are reset and toggle is
 * made sensitive.
 */
static XtTimerCallbackProc
launch_timeout(XtPointer cd, XtIntervalId * id)
{
	AppInfo * ai = (AppInfo *)cd;

	if (ai->a_timer == *id) {
		ai->a_timer = 0;
		ai->a_pid = 0;
		set_toggle_status(ai->a_toggle, True);
	}
}

/*
 * appTable is searched based on a toggle widget.
 * If found timer and pid values are reset and
 * timer is cancelled.
 * 
 * This is called upon window manager map event.
 */
Widget
reset_appTable_launcherInfo(pid_t pid)
{
	AppInfo * ai = launcherContext->l_appTable;
	int	i, max  = launcherContext->l_appCount;

	for (i = 0; i < max; i++) {
		if (ai[i].a_pid == pid) {
			ai[i].a_pid = 0;
			if (ai[i].a_timer) 
				XtRemoveTimeOut(ai[i].a_timer);
			ai[i].a_timer = 0;
			return(ai[i].a_toggle);
		}
	}
	return(NULL);
}

void
set_appTable_launcherInfo(Widget w, pid_t pid)
{
	AppInfo * ai = launcherContext->l_appTable;
	int	i, max  = launcherContext->l_appCount;

	for (i = 0; i < max; i++) {
		if (ai[i].a_toggle == w) {
			ai[i].a_pid = pid;
			ai[i].a_timer = XtAppAddTimeOut(GappContext, 
						LAUNCH_TIMEOUT,
						launch_timeout, &ai[i]);
			break;
		}
	}
}	
