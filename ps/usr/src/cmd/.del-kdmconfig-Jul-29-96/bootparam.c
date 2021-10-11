/*
 * Copyright (c) 1994, by Sun Microsystems, Inc.
 *
 * bootparam.h: Bootparam manipulation publice interface.
 *
 * Description:
 *  This file provides the implementation for bootparam handler
 *  functions. These are the routines that provide the access and rewriting
 *  of the server's /etc/bootparams file when kdmconfig is invoked with the
 *  -s option.
 *
 * The following exported routines are found in this file
 *
 *  void bootparam_commit(char *);
 *  int bootparam_get(char *);
 *
 */

#pragma ident "@(#)bootparam.c 1.7 94/08/12 SMI"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/systeminfo.h>
#include "ui.h"
#include "windvc.h"
#include "util.h"

static int _skip_sep(char *str)
{
int index = 0;
int onchar = 0;
	while(!onchar) {
		onchar = 1;
		while((str[index] == ' ') || (str[index] == '\t')) {
			onchar = 0;
			index++;
		}
		if ((str[index] == '\\') && (str[index+1] == '\n')) {
			 index += 2;
				onchar = 0;
		}
	}
	return(index);
}

static int
_fill_ptrs( char *str, char **ptrs, int ind)
{
int x = 0;
int y;
int inquote = 0;
int shift = 0;
char *c, *du;

	str = &str[_skip_sep(str)];

	/* find the keywords in the buffer */
	if (strstr(str,"display=")) ptrs[ind++] = strstr(str, "display=");
	ptrs[ind] = NULL;
	if (strstr(str,"keyboard=")) ptrs[ind++] = strstr(str, "keyboard=");
	ptrs[ind] = NULL;
	if (strstr(str,"pointer=")) ptrs[ind++] = strstr(str, "pointer=");
	ptrs[ind] = NULL;
	if (strstr(str,"monitor=")) ptrs[ind++] = strstr(str, "monitor=");
	ptrs[ind] = NULL;
	
	

	/* add nulls in right place - account for quoted strings */
	for(x=0; (x<4 ) && ptrs[x]; x++) {
		c= strchr(ptrs[x], ':');
		while(!_skip_sep(c) || inquote) {
			if (c[0] == '"') 
				inquote = (inquote+1) % 2;
			c++;
		}
		c[0] = '\0';
	}

	/* shift out the quotes - separately since we need the NULL's first */
	inquote = 0;
	for(x=0; (x<4) && ptrs[x]; x++) {
		c= strchr(ptrs[x], ':');
		while(c[0]) {
			if (c[0] == '"') {
				if (c[1]) {
					du = strdup(&c[1]);
					strcpy(c, du); /* shift the quote away */
					free(du);
				}
				else
					c[0] = '\0'; /* close-quote at end of string */
			}
			else
				c++;
		}
	}

	return(ind);
}

static void
_find_keywords(char *buf, char **ptrs, char *client)
{
	int x = 0, y, latch;
	char sstr[1024];
	char *entry= NULL;
	char *WILD = "*";
	char *ptr1[4];

	ptrs[0] = NULL;


	/* find the client entry */
	/* Look at the beginning of the file */
	if (!strncmp(buf, client, strlen(client)))
		entry = strdup(buf);
	else {
		/* Look after newlines */
		sprintf(sstr, "\n%s", client);
		if (strstr(buf, sstr))
			entry = strdup(strstr(buf, sstr));
	}

	/* Isolate the string by finding a 'real' newline. Replace other
	 * newlines in the string with blanks.
	 */
	if (entry) {
		while (entry[0] == '\n') entry++;
		while(entry[x]) {
			if (entry[x] == '\\')
				if (entry[x+1] == '\n') {
					if (entry[x+1]) {
						strcpy(&entry[x], &entry[x+1]);
						entry[x] = ' ';
						x--;
					}
					else {
						entry[x] = 0;
						break;
					}
				}
			if (entry[x] == '\n')
				entry[x] = 0;
			else
				x++;
		}
		ptr1[0] = NULL;
		_fill_ptrs(entry, ptr1, 0);
		for(x=0; (x<4) && ptr1[x]; x++)
			ptrs[x] = strdup(ptr1[x]);
		if (x<4) ptrs[x] = NULL;
		free(entry); entry = NULL;
	}

	/* Do the same thing, looking at wildcard entry */
	if (!strncmp(buf, WILD, strlen(WILD)))
		entry = strdup(buf);
	else {
		/* Look after newlines */
		sprintf(sstr, "\n%s", WILD);
		if (strstr(buf, sstr))
			entry = strdup(strstr(buf, sstr));
	}

	if (entry) {
		while (entry[0] == '\n') entry++;
		while(entry[x]) {
			if (entry[x] == '\\')
				if (entry[x+1] == '\n') {
					if (entry[x+2]) {
						strcpy(&entry[x], &entry[x+2]);
						x--;
					}
					else {
						entry[x] = 0;
						break;
					}
				}
			if (entry[x] == '\n')
				entry[x] = 0;
			else
				x++;
		}
		ptr1[0] = NULL;
		_fill_ptrs(entry, ptr1, 0);

		/*
	 	 * prtrs contains the client entries, 
	 	 * ptr1 contains the wildcard entries.
	 	 * Look at each wildcard entry, and if there is no keyword for the
	 	 * same entry in the client entry, copy the wc to the client array 
	 	 */
		for(x=0; (x<4) && ptr1[x]; x++) {
			latch = 0;
			for(y=0; (y<4) && ptrs[y]; y++) {
				if (!strncmp(ptrs[y], ptr1[x],7)) latch++;
			}
			if (!latch) {
				ptrs[y] = strdup(ptr1[x]);
				if (y < 2) ptrs[y+1] = NULL;
			}
		}
		free(entry);
	}
	return;
	
}

static char *
_build_string( cat_t catname)
{
char buf[1024]; /* Max size of bootparam entry */
char hostname[256];
NODE node = get_selected_device(catname);
ATTRIB *alist;
VAL val;
int x; char cvt[32];

	if (!node) return("");

	sysinfo(SI_HOSTNAME, hostname, 256);
	sprintf(buf,"%s=%s:%s", catname, hostname, node->name);

	/* now add attributes */
	alist = get_attrib_name_list(node);
	for(x=0; alist[x]; x++) {
		val = get_selected_attrib_value(node, alist[x]);
		strcat(buf,";");
		strcat(buf,alist[x]);
		strcat(buf,"=");
		if (get_attrib_type(node, alist[x]) == VAL_STRING) 
			if (strchr(val,' ')) /* bracket with quotes */
				sprintf(cvt, "\"%s\"", val);
			else
				sprintf(cvt, "%s", val);
		else 
			sprintf(cvt,"%d", *(int *)val);
		strcat(buf, cvt);
	}

	return(strdup(buf));
}

char *
_find_space(char * c)
{
int inquote = 0;
static char *str = NULL;
static int index = 0;
static int length = 0;
static char *orig = NULL;

		if (c) {
			str = c;
			index = 0;
			length = strlen(str);
		}

		/* Skip leading blanks */
		index += _skip_sep(&str[index]);
		orig = &str[index];

		/* find the next blank after character */
		while(str[index] && ((!_skip_sep(&str[index])) || inquote)) {
			if (str[index] == '"') 
				inquote = (inquote+1) % 2;
			index++;
		}
		str[index] = '\0';
		if (index < length) index++;
	return(orig[0] ? orig : NULL);
}

void
bootparam_commit(char * client)
{
char *inbuf;
char *entry, *pre, *pos;
char *optr, *ib;
char *kstr, *dstr, *pstr, *mstr;
char *oldstr = NULL;
char *outstr, *newstr;
char *rec;
FILE *fp; char sstr[32];
char *buf, newl[2];
char *empty = "";
int x = 0; char *c;

	/* read in existing bootparams file */
	inbuf = NULL;
	if (read_generic("/etc/bootparams", &inbuf, FILE_READ)) {
		inbuf = strdup("");
	}
	if (!inbuf[0]) inbuf = empty;

	/*
	 * Make three strings:
	 *  pre - contains all data up to the entry (could be empty)
	 *  pos - contains all data after the entry (could be empty)
	 *  entry - contains the entry itself (could be NULL)
	 *
	 *  If there is no entry, everything goes into pre.
	 */
	
	pre = inbuf;   /* This is a given - could be empty, though */
	entry = inbuf; /* Lets start here and look for the tag */
	pos = empty;   /* default - if there is no entry */

	/* Find the entry */
	while(entry && (entry = strstr(entry, client))) {
		if ((entry == inbuf) || 
			((entry > inbuf) && (*(entry -1) == '\n')))
			break; /* found it */
		entry++; /* skip just found one */
	} /* entry comes out of this NULL if not found */

	/* If found at the beginning of the string ... */
	if (entry == inbuf) pre = empty;

	/* entry and pre are taken care of. Now deal with pos */
	/* find the first newline not preceeded by a backslash */

	if (entry) { /* The only way to have a post, is to have an entry. */
		while(entry[x] != '\n') {
			if (entry[x] == '\\') x++; /* skip backslashed things */
			x++;
		}
		pos = &entry[++x];
		if (!pos) pos = empty;
	}

	/* We know pos is null terminated. Put nulls after entry and pre
	 * if they are not the end of the line.
	 */
	if (entry) {
		if (pre != empty)
			*(entry-1) = '\0';
		if (pos != empty)
			*(pos -1) = '\0';
		/*
	 	 * Now remove the embedded newlines from the entry
	 	 */
		while(c = strstr(entry,"\\\n")) {
			c[0] = ' '; /* convert to just a blank */
			c[1] = ' '; /* convert to just a blank */
		}
	}

	/* Make three strings out of selected devices */

	kstr = _build_string(KEYBOARD_CAT);
	dstr = _build_string(DISPLAY_CAT);
	pstr = _build_string(POINTER_CAT);
	mstr = _build_string(MONITOR_CAT);

	/* Chop existing string up, removing our keywords */

	if (entry) {
		oldstr = strdup(entry);
		ib = entry; /* set up pointers to old and new versions */
		optr = oldstr;
		while(rec = _find_space(ib)) {
			ib = NULL;
			if ((!strncmp(rec, "display=", 8)) ||
		    	(!strncmp(rec, "pointer=", 8)) ||
		    	(!strncmp(rec, "keyboard=", 9))||
			(!strncmp(rec, "monitor=",8))) continue;
			else {
				sprintf(optr, "%s ", rec);
				optr += (strlen(rec)+1);
			}
		}
	}
	else {
		oldstr = (char *)malloc(strlen(client)+2);
		sprintf(oldstr,"%s ", client);
	}

	newstr = (char *)malloc(strlen(oldstr) + 
	       	strlen(kstr) + strlen(dstr) + strlen(mstr) + strlen(pstr) + 10);
	sprintf(newstr, "%s%s %s %s %s\n", oldstr, kstr, dstr, mstr,pstr);

	/* trim trailing blanks, before newline */
	for(x=strlen(newstr)-2; newstr[x] == ' '; x--) {
		newstr[x] = '\n'; newstr[x+1] = '\0';
	}

	/* eliminate entry entirely if it has only the client */
	if (newstr[strlen(client)] == '\n') newstr = empty;

	/* Sync it to the disk */
	buf = malloc(strlen(pre)+strlen(newstr)+strlen(pos)+2);
	if ((pre != empty) && entry)
		sprintf(newl,"\n");
	else
		sprintf(newl,"");
	sprintf(buf,"%s%s%s%s",pre, newl, newstr, pos);
	write_generic("/etc/bootparams", buf, FILE_READ);

	/* Wow, we really abused strdup and malloc here! */
	free(buf); free(kstr); free(dstr); free(pstr);
	free(inbuf); free(oldstr); free(newstr);
}

int
bootparam_get(char * client)
{
	char *ptrs[4];
	char *buf = NULL;
	int x;
	char *ptr;
	cat_t catname;
	NODE node;

	if (read_generic("/etc/bootparams", &buf, FILE_READ)) {
		buf = strdup("");
	}
	_find_keywords(buf, ptrs, client);
	for(x=0; (ptrs[x] != NULL) && (x<4); x++){
		ptr = strchr(ptrs[x], ':'); ptr++;
		catname = strtok(ptrs[x],"=");
		node = parse_node(ptr, catname);
		set_selected_device(node);
	}

	for(x=0; (x<4) && ptrs[x]; x++) free(ptrs[x]);
	free(buf);
	return(CONFIRM_ALL);
}

void
bootparam_remove(char * client)
{
	bootparam_commit(client);
}
