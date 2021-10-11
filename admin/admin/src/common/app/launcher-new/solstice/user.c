#include <stdio.h>
#include <stdlib.h>

#include "launcher.h"
#include "util.h"

extern int newNumCols;

int chosenNumColumns = 3;
static char fname[128];

extern SolsticeApp	* local_apps;
extern AppInfo * apps = NULL;

#define BAD_FORMAT_MSG \
      catgets(catd, 1, 74, "Bad format in config file $HOME/.solsticerc")

#define VERSION	1

static int rcfileversion;

/*
 * Strip whitespace off front/back of string and
 * return ptr into same string at first non-white
 * character.
 */

static char * 
rm_spaces(char * t)
{
	char * p;
	int l;

	/* remove any space at front */
	while (isspace(*t)) t++;
	l = strlen(t);

	/* remove blanks at end */
	p = t + l - 1;
	while (isspace(*p)) {
	    *p = '\0'; 
	    p--; 
	}
	return(t);
}

int
read_user_config(char * fname)
{
	FILE * fp;
	char s[128];
	int rc, d, cnt, num_entries;
	unsigned int c, ord, site, show;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		/*
		display_warning(NULL, "Unable to open user config file $HOME/.solsticerc");
		*/
		return(0);
	}
	/* 
         * Peek at first char looking for a version string
         * Version string is Vnn is n is 0-9.
         *
         * This is to support compatibility with old format.
         */
	c = fgetc(fp);
	if (c == 'V') {
		fgets(s, 128, fp);
		rcfileversion = atoi(s);
	}
	else
		ungetc(c, fp);
		
	d = fscanf(fp, "%d %d\n", &num_entries, &chosenNumColumns);
	if (d != 2) {
		display_error(launchermain, BAD_FORMAT_MSG);
		fclose(fp);
		return(0);
	}
	newNumCols = chosenNumColumns;
	if (num_entries <= 0)
		return;

	newAppTable(num_entries);
	cnt = 0;
	/*
         * A new format for .solstice file requires two different
         * approaches to reading it. Older versions contain only 
	 * a list of LOCAL applications and do not retain order
         * information. New format (VERSION==1) contain order
         * and site (LOCAL or GLOBAL) information.
         *
         * Old format: 
         *	N C
	 *      app0
	 *      app1
         *      ...
         *   	appN-1
         *
 	 * New format:
	 *      V1
         *	N C
	 *      o s v app0
	 *      o s v app1
         *      ...
         *   	o s v appN-1
	 *
	 * Where N is no. of apps listed in file, C is no. of display
	 * columns, o is display ordinal of app and s is site, LOCAL (=0)
	 * or GLOBAL (=1).
 	 */

	if (rcfileversion == VERSION) {
	    while ((rc = fscanf(fp, "%d %d %d", &ord, &site, &show)) == 3) {
		char * tmp;
		int l;

	        if (fgets(s, 128, fp) == NULL) {
			display_error(launchermain, BAD_FORMAT_MSG);
			launcherContext->l_appCount = cnt;
			fclose(fp);
			return(cnt);	
		}
		/* when first read from user config file, it is assumed
		 * entry is obsolete. Subsequent read of system registry
  		 * will update those entries that are supported.
		 */	
		launcherContext->l_appTable[cnt].a_obsolete = True;
		tmp = strdup(s);

		if (launcherContext->l_appTable[ord].a_appName) {

		    /* A fail-safe check. Config file has duplicate 
                     * ordinal values.
		     */
			/* 
   			 * FIX ME...when the powers-that-be relax
                         * restrictions on L10N impacts, create
			 * a more informative error message.
			 */
			display_error(launchermain, BAD_FORMAT_MSG);
			free(tmp);
			continue;
		}
		tmp = rm_spaces(tmp);

		launcherContext->l_appTable[ord].a_appName = tmp;
		launcherContext->l_appTable[ord].a_displayOrdinal = ord;
		launcherContext->l_appTable[ord].a_site = (registry_loc_t)site;
		launcherContext->l_appTable[ord].a_show = (visibility_t) show;
		cnt++;
	    }
	    /* 
             * fscanf returned something I did not expect... 
             * appTable is ok up to cnt, so just display error 
             * and continue.
             */
	    if (rc != EOF && rc < 3)
		display_error(launchermain, BAD_FORMAT_MSG);
	} else {
	    while (fgets(s, 128, fp) != NULL) {
		char * tmp;
		int l;

		/* when first read from user config file, it is assumed
		 * entry is obsolete. Subsequent read of system registry
  		 * will update those entries that are supported.
		 */	
		launcherContext->l_appTable[cnt].a_obsolete = True;
		tmp = strdup(s);
		tmp = rm_spaces(tmp);
	     	launcherContext->l_appTable[cnt].a_appName = tmp;
	     	launcherContext->l_appTable[cnt].a_site = LOCAL;
		launcherContext->l_appTable[ord].a_show = SHOW;
	     	cnt++;
	    }
	}
	launcherContext->l_appCount = cnt;
	fclose(fp);
	return(cnt);
}

/*
 * Write only "new" format.
 */

void
write_user_config(char * fname, AppInfo * apps, int appcount, int showcnt)
{
	FILE * fp;
	int write_cnt = 0, i;
	int hidecnt = 0;

	fp = fopen(fname, "w");
	if (fp == NULL) {
		display_warning(launchermain, catgets(catd, 1, 75, "Unable to write configuration to $HOME./solsticerc"));
		return;
	}

	fprintf(fp, "V%d\n", VERSION);
	fprintf(fp, "%d %d\n", appcount, chosenNumColumns);
	for (i = 0; i < appcount; i++) {
		if (apps[i].a_show == SHOW) 
			fprintf(fp, "%d %d %d %s\n", apps[i].a_displayOrdinal,
					       apps[i].a_site,
					       apps[i].a_show,
					       apps[i].a_appName);
		else {
		/* For those apps that are hidden I write out an
		 * ordinal value that will place them at the end
                 * of table when read back in.
		 */
			fprintf(fp, "%d %d %d %s\n", 
					showcnt + hidecnt,
					apps[i].a_site,
					apps[i].a_show,
					apps[i].a_appName);
			hidecnt++;
		}
	}	
	fclose(fp);
}

