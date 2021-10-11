#ifndef lint
#pragma ident "@(#)v_pfg_rfs.c 1.6 96/06/23 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_pfg_rfs.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"

static void pfFreeName(pfName_t *name);
static char *v_conv(char from, char to, char *str);

#define	must_be(s, c)	if (*s++ != c) return (0)
#define	skip_digits(s)	while (isdigit(*s)) s++

pfErCode
pfValidRem(Remote_FS * rem)
{
	if (!pfIsValidIPAddr(rem->c_ip_addr))
		return (pfErIPADDR);
	if (!pfIsValidMountPoint(rem->c_mnt_pt))
		return (pfErMOUNT);
	if (!pfIsValidRemoteOptions(rem->c_mount_opts))
		return (pfErNFSPRESERVE);
	return (pfOK);
}

/* returns 0 if preserve used */
int
pfIsValidRemoteOptions(char *opts)
{
	if (!opts)
		return (0);

	if (strcasecmp(opts, "preserve") == 0)
		return (0);

	return (1);
}

pfErCode
pfValidMountPoint(char *name)
{
	if (debug)
		(void) printf("pfvalidate:pfValidMountPoint =%s=\n", name);

	if (!name)
		return (pfErMOUNT);

	if (*name == '\0')
		return (pfOK);

	if (strcmp(name, "swap") == 0)
		return (pfOK);

	if (*name != '/')
		return (pfErMOUNTNOSLASH);

	if (strchr(name, ' '))
		return (pfErMOUNTSPACECHAR);

	/* maybe add a test for "//" in string per jj */

	if ((int) strlen(name) > (int) PATH_MAX) /* 1024 in limits.h */
		return (pfErMOUNTMAXCHARS);

	return (pfOK);

}


/* 0 means invalid */
int
pfIsValidMountPoint(char *name)
{
	return ((pfValidMountPoint(name) == pfOK) ? 1 : 0);
}


/* returns 0 if not a valid ip address ('-' OK) */
int
pfIsValidIPAddr(char *addr)
{
	int num;
	char *p;

	if (strlen(addr) == 0)
		return (1);

	if ((p = strchr(addr, '.')) == NULL)
		return (0);

	*p = '\0';
	num = atoi(addr);
	if (num < 0 || num > 255)
		return (0);

	*p = '.';
	addr = p + 1;
	if ((p = strchr(addr, '.')) == NULL)
		return (0);

	*p = '\0';
	num = atoi(addr);
	if (num < 0 || num > 255)
		return (0);

	*p = '.';
	addr = p + 1;
	if ((p = strchr(addr, '.')) == NULL)
		return (0);

	*p = '\0';
	num = atoi(addr);
	if (num < 0 || num > 255)
		return (0);

	*p = '.';
	addr = p + 1;
	num = atoi(addr);
	if (num < 0 || num > 255)
		return (0);

	return (1);
}


char *
getDefaultRfs(char *local)
{
	static char path[MAXPATHLEN];
	Module *prod = get_current_product();

	path[0] = '\0';
	if (prod && prod->info.prod &&
	    prod->info.prod->p_name &&
	    prod->info.prod->p_version) {
		if (strcmp("/usr", local) == 0) {
			(void) sprintf(path, "/export/exec/%s_%s_%s.all/usr",
			    v_conv(' ', '_', prod->info.prod->p_name),
			    v_conv(' ', '_', prod->info.prod->p_version),
			    get_default_inst());
		}
	}
	return (path);
}

static Remote_FS *Remotes = NULL;	/* static list of remote filesys */


/* pfAppend*: Add a node to a list, note the first arg is location of head */

pfErCode
pfAppendName(pfName_t **head, pfName_t *newbee)
{
	pfName_t *n;

	if (!*head)
		*head = newbee;
	else {
		for (n = *head; n->next; n = n->next);
		n->next = newbee;
	}
	return (pfOK);
}

pfErCode
pfAppendRem(Remote_FS **head, Remote_FS *newbee)
{
	Remote_FS *r;

	if (!*head)
		*head = newbee;
	else {
		for (r = *head; r->c_next; r = r->c_next);
		r->c_next = newbee;
	}
	return (pfOK);
}

pfName_t *
pfNewName(char *name)
{
	pfName_t *link;

	link = (pfName_t *) xmalloc(sizeof (pfName_t));
	link->name = (char *) xmalloc(strlen(name) + 1);

	(void) strcpy(link->name, name);
	link->next = NULL;

	return (link);
}

void
pfSetRemoteFS(Remote_FS *remotes)
{
	Remotes = remotes;
}

Remote_FS *
pfGetRemoteFS()
{
	return (Remotes);
}

Remote_FS *
pfNewRem(TestMount c_test_mounted, char *c_mnt_pt, char *c_hostname,
    char *c_ip_addr, char *c_export_path, char *c_mount_opts)
{
	Remote_FS *link;

	link = (Remote_FS *) xmalloc(sizeof (Remote_FS));
	link->c_mnt_pt = (char *) xmalloc(strlen(c_mnt_pt) + 1);
	link->c_hostname = (char *) xmalloc(strlen(c_hostname) + 1);
	link->c_ip_addr = (char *) xmalloc(strlen(c_ip_addr) + 1);
	link->c_export_path = (char *) xmalloc(strlen(c_export_path) + 1);
	link->c_mount_opts = (char *) xmalloc(strlen(c_mount_opts) + 1);

	(void) strcpy(link->c_mnt_pt, c_mnt_pt);
	(void) strcpy(link->c_hostname, c_hostname);
	(void) strcpy(link->c_ip_addr, c_ip_addr);
	(void) strcpy(link->c_export_path, c_export_path);
	(void) strcpy(link->c_mount_opts, c_mount_opts);
	link->c_test_mounted = c_test_mounted;
	link->c_next = NULL;

	return (link);
}

Remote_FS *
pfDupRem(Remote_FS *rem)
{
	return (rem ? pfNewRem(rem->c_test_mounted, rem->c_mnt_pt,
		rem->c_hostname, rem->c_ip_addr, rem->c_export_path,
		rem->c_mount_opts) : NULL);
}

Remote_FS *
pfDupRemList(Remote_FS *rem)
{
	Remote_FS *newrem, *ret;

	if (!rem)
		return (NULL);

	ret = newrem = pfDupRem(rem);

	for (rem = rem->c_next; rem; rem = rem->c_next) {
		newrem->c_next = pfDupRem(rem);
		newrem = newrem->c_next;
	}
	newrem->c_next = NULL;
	return (ret);
}

/* pfFree*: functions free structs */
void
pfFreeName(pfName_t *name)
{
	if (!name)
		return;
	if (debug > 10)
		(void) printf("pfdata:pfFreeName 0x%x 0x%x\n",
			(int) name->name, (int) name);
	free(name->name);
	free(name);
}



void
pfFreeRem(Remote_FS *rem)
{
	if (!rem)
		return;
	if (debug > 10)
		(void) printf("pfdata:pfFreeRem 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		    (int) rem->c_mnt_pt, (int) rem->c_hostname,
		    (int) rem->c_ip_addr, (int) rem->c_export_path,
		    (int) rem->c_mount_opts, (int) rem);
	free(rem->c_mnt_pt);
	free(rem->c_hostname);
	free(rem->c_ip_addr);
	free(rem->c_export_path);
	free(rem->c_mount_opts);
	free(rem);
}

void
pfFreeNameList(pfName_t *name)
{
	if (!name)
		return;
	if (name->next)
		pfFreeNameList(name->next);
	pfFreeName(name);
}

void
pfFreeRemList(Remote_FS *rem)
{
	if (!rem)
		return;
	if (rem->c_next)
		pfFreeRemList(rem->c_next);
	pfFreeRem(rem);
}


static char *
v_conv(char from, char to, char *str)
{
	register char *bp;

	for (bp = str; bp && *bp; ++bp)
		if (*bp == from)
			*bp = to;

	return (str);
}
