/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 *
 * util.c: Common utility routines
 *
 * Description:
 *
 *
 * External Routines:
 */

#pragma ident "@(#)util.c 1.8 95/02/27 SMI"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>

#include "windvc.h"
#include "util.h"
#include "kdmconfig_msgs.h"


#define	BLKSIZE 1024

/*
 * The SYNTAX of bootparams lines is as follows:
 *
 *  <entry>   ::= <keyword>="<NAME>;<attrs>"
 *  <attrs>   ::= [<ATTNAME>=<VALUE>;][<attrs>]
 *  <NAME>    ::= Alphanumeric Identifier
 *  <ATTNAME> ::= Alphanumeric Identifier
 *  <VALUE>   ::= Alphanumeric Identifier
 *
 * All Attribute values are strings. NUMERIC Attributes will be converted
 * appropriately.
 *
 * No attributes need to be named. Unnamed attributes will be left
 * as the default. The device name (NAME) must be specified.
 *
 * examples:
 *    pointer="logi;buttons=3;dev=/dev/tty01;"
 *
 *    pointer="kdmouse;"
 *
 *    display="far60;size=16x9;"
 *
 */

/*
 * opens a file or a process and reads its contents or stdout completely
 * into a buffer.
 */
int
read_generic(char *cmd, char **bufptr, int cmdflag)
{
	int ct = 0;
	int blks = 0;
	FILE *fp;

	if (cmdflag == PROCESS_READ)
		fp = popen(cmd, "r");
	else
		fp = fopen(cmd, "r");

	if (!fp)
		return (-1);

	*bufptr = NULL;

	while (!feof(fp)) {
		if (ct >= BLKSIZE * blks) {
			*bufptr = realloc(*bufptr, BLKSIZE*(++blks));
			memset(*bufptr+(BLKSIZE*(blks-1)), 0, BLKSIZE);
		}
		ct += fread(*bufptr+ct, 1, BLKSIZE-(ct%BLKSIZE), fp);
	}

	if (cmdflag == PROCESS_READ)
		return (pclose(fp));
	else
		return (fclose(fp));
}


/*
 * Takes a buffer to completely rewrite a file, or nis+ map
 */
void
write_generic(char *cmd, char *bufptr, int cmdflag)
{
	int ct = 0;
	int blks = 0;
	FILE *fp;

	fp = fopen(cmd, "w");

	if (!fp)
		return;

	fwrite(bufptr, strlen(bufptr), 1, fp);

	fclose(fp);
}

/*
 * Given a string with a name-value pair in it, it terminates the string
 * after the pair. It moves the pointer passed in past the pair.
 */
static char *
_get_devname(char ** devstr)
{
	char *x, *ret;

	if (!**devstr)
		return (NULL);

	ret = *devstr;
	if (x = strchr(*devstr, ';')) {
		*devstr = ++x;
		return (strtok(ret, ";"));
	} else {
		*devstr += strlen(*devstr); /* point to null */
		return (ret);
	}
}

static char *
_next_pair(NODE node, char ** devstr, ATTRIB * attrib, VAL * value)
{
	char *att, *val;

	if (att = _get_devname(devstr)) {
		*attrib = att;
		if (val = strchr(att, '=')) {
			*val = '\0';
			val++;
			if (get_attrib_type(node, att) == VAL_NUMERIC)
				print_num(val, *(int *)value, 0);
			else if (get_attrib_type(node, att) == VAL_UNUMERIC)
				print_num(val, *(unsigned int *)value, 1);
			else
				*value = val;
		}
		return (att);
	}
	return (NULL);
}


NODE
parse_node(char *buf, cat_t catname)
{
char *name;
NODE node;
ATTRIB attr;
VAL value;
char dummy[1024];


	/* find the name of the device */
	name = _get_devname(&buf);
	if (!name)
		return (NULL);

	/* Get that node */
	node = get_node_by_name(catname, name);

	if (!node) {
		(void) sprintf(dummy,
			"Node %s defined for %s not found", name, catname);
		ui_notice(dummy);
		return (NULL);
	}

	/* Set all of the defined attributes for that node */
	while (_next_pair(node, &buf, &attr, &value))
		set_attrib_value(node, attr, value);

	return (node);
}

void
check_nsswitch()
{
FILE *fp;
char st[256];
char *wd;

	fp = fopen("/etc/nsswitch.conf", "r+");
	while (fgets(st, sizeof (st), fp)) {
		if (st[0] == '\n') continue;
		wd = strtok(st, " \t");
		if (wd[0] == '#')
			continue;

		/*
		 * This logic only passes if we have a local version of
		 * the bootparams database. For a nis or nisplus version,
		 * we bomb. This may be modified to support nisplus also.
		 */
		if (!strcmp(wd, "bootparams:")) {
			wd = strtok(NULL, " \t");
			if (!strcmp(wd, "files"))
				break; /* What we want to see */
			else {
				ui_error(KDMCONFIG_MSGS(KDMCONFIG_NSSWITCH), 0);
				exit(-1);
			}
		}
	}
	fclose(fp);
}

static char * client = NULL;
static int server_mode = 0;

void
set_server_mode(char * c)
{
	server_mode++;
	client = strdup(c);
	return;
}

int
get_server_mode()
{
	return (server_mode);
}

char *
get_client()
{
	return (client);
}
