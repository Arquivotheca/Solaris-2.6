#include "launcher.h"
#include "util.h"

void solstice_error(Widget, int);

extern AppInfo * newAppTable(int);
extern int update_appTable_entry(const char *, const char *, const char *, 
				const char *, const char *, registry_loc_t);

int 
merge_registry(char * name, registry_loc_t site)
{
	int s_cnt, n, ui, si;
	SolsticeApp * s_apps;
	AppInfo * ai;

	s_cnt = solstice_get_apps(&s_apps, name);
	if (s_cnt < 0) {
		return(s_cnt);
	}

	if (launcherContext->l_appTable == NULL) {
		/* all the entries are "new" */
		ai = launcherContext->l_appTable = newAppTable(s_cnt);
		for (si = 0; si < s_cnt; si++) {
			ai[si].a_site = site;
			ai[si].a_show = SHOW;
			ai[si].a_obsolete = False;
			ai[si].a_appName = strdup(s_apps[si].name);
			ai[si].a_iconPath = strdup(s_apps[si].icon_path);
			ai[si].a_appPath = strdup(s_apps[si].app_path);
			ai[si].a_appArgs = strdup(s_apps[si].app_args);
			ai[si].a_scriptName = NULL;
		}
		launcherContext->l_appCount = s_cnt;
		return(0);
	}
	for (si = 0; si < s_cnt; si++)
		update_appTable_entry(s_apps[si].name,
		    	s_apps[si].app_path, 
		    	s_apps[si].app_args, 
		    	s_apps[si].icon_path, 
			(char *)NULL, site);

	return(0);
}

static char str[256];

void
remove_obsolete_entries()
{
	AppInfo * ai = launcherContext->l_appTable;
	int i = launcherContext->l_appCount-1;

	if (ai == NULL)
	    return;

	do {
		if (ai[i].a_obsolete) 
			remove_appTable_entry(ai[i].a_appName);
		i--;	
	} while (i > -1);
}

void
report_obsolete_entries()
{
	Boolean flag = False;
	int i;
	AppInfo * ai = launcherContext->l_appTable;

	sprintf(str, "%s\n\t", catgets(catd, 1, 40, "The following applications are no longer available\nin the Solstice Application Registry:\n\t"));
	for (i = 0; i < launcherContext->l_appCount; i++) {
		if (ai[i].a_obsolete) {
			strcat(str, ai[i].a_appName);
			strcat(str, catgets(catd, 1, 41, "\n\t"));
			flag = True;	
		}
	}
	if (flag) {
		display_warning(launchermain, str);
		remove_obsolete_entries();
	}
}

void
solstice_error(Widget w, int code)
{
	static char serr[256];
	int err_flg = 1;

	switch (code) {
	case LAUNCH_BAD_INPUT:
		sprintf(serr, catgets(catd, 1, 42, "Solstice Registry Error [%d]:\nEither Application name or path is missing"), code);
		break;
	case LAUNCH_LOCKED:
		sprintf(serr, catgets(catd, 1, 43, "Solstice Registry Error [%d]:\nregistry is locked"), code);
		break;
	case LAUNCH_DUP:
		sprintf(serr, catgets(catd, 1, 44, "Solstice Registry Error [%d]:\nattempt to add duplicate entry"), code);
		break;
	case LAUNCH_NO_ENTRY:
		sprintf(serr, catgets(catd, 1, 45, "Solstice Registry Error [%d]:\napplication is not in registry"), code);
		break;
	case LAUNCH_NO_REGISTRY:
		sprintf(serr, catgets(catd, 1, 46, "Solstice Registry Error [%d]:\nunable to locate registry\nCheck whether Adminsuite software is installed"), code);
		/* In this case, just issue a warning. */
		err_flg = 0;
		break;
	case LAUNCH_ERROR:
	default:
		sprintf(serr, catgets(catd, 1, 47, "Solstice Registry Error [%d]"), code);
		break;
	}
	if (err_flg)
	    display_error(w, serr);
	else
	    display_warning(w, serr);
}
	

