
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 *  All rights reserved.
 */

#pragma	ident	"@(#)db_method_wrappers.c	1.31	95/01/27 SMI"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <signal.h>

#include <stdlib.h>
#include <sys/systeminfo.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpc/rpc.h>
#include <rpcsvc/nis.h>

#include "sysidtool.h"
#include "cl_database_parms.h"
#include "admldb.h"

#define	UN(string) ((string) ? (string) : "")

extern int errno;

static int set_entry(int table, char *key, char *val);
static int nis_get_entry(int table, char *key, char *val);
static int yp_get_entry(int table, char *key, char *val);
static int cb(int, char *, int, char *, int, char *);

static char *yp_locale_key;

/*
 * update "hosts" and "netmasks" tables in /etc
 * lookup "netmasks", "timezone" and "locale" from name service
 */
static struct {
	struct {
		char	*name;
		int	key_fld;
		int	val_fld;
	} etc;
	struct {
		char	*name;
		int	key_fld;
		int	val_fld;
	} yp;
	struct {
		char	*name;
		char	*keyname;
	} nis;
} tbl[4] = {
{{"hosts", 1, 0}, {"", 0, 0}, {"", ""}},
{{"netmasks", 0, 1}, {"netmasks.byaddr", 0, 0}, {"netmasks.org_dir", "addr"}},
{{"", 0, 0}, {"timezone.byname", 1, 0}, {"timezone.org_dir", "name"}},
{{"", 0, 0}, {"locale.byname", 0, 1}, {"locale.org_dir", "name"}}
};

/*
 * Convert the NS string to the NS num
 */

int
conv_ns(char *ns)
{
	if (strcmp(ns, DB_VAL_NS_NIS_PLUS) == 0)
		return (NISPLUS);
	else if (strcmp(ns, DB_VAL_NS_NIS) == 0)
		return (NIS);
	else
		return (UFS);
}

int
set_ent_hosts(char *ip_addr, char *hostname, char *alias, char *errmess)
{
	int	ret_stat;
	char	entry[256];

	(void) fprintf(debugfp, "set_ent_hosts: %s, %s, %s\n",
		ip_addr, hostname, UN(alias));

	sprintf(entry, "%s\t%s\t%s\n", ip_addr, hostname, UN(alias));
	ret_stat = set_entry(HOSTS_TBL, hostname, entry);

	errmess[0] = 0;
	return (ret_stat);
}

/*
 * Set the netmask in /etc/netmasks.
 */

int
set_ent_netmask(char *netnum, char *netmask, char *errmess)
{
	int	ret_stat;
	char	entry[256];

	(void) fprintf(debugfp, "set_ent_netmask: %s, %s\n", netnum, netmask);

	sprintf(entry, "%s\t%s\n", netnum, netmask);
	ret_stat = set_entry(NETMASKS_TBL, netnum, entry);

	errmess[0] = 0;
	return (ret_stat);
}


static int
set_entry(int table, char *key, char *val)
{
	FILE *rfp, *wfp;
	char tmpname[128], table_name[128], buff[1024], dup[1034];
	char *p;
	int fld, keypos, done = 0;

	(void) fprintf(debugfp, "set_entry: %s: %s", key, val);

	if (testing)
		return ((*sim_handle()) (SIM_DB_MODIFY, UFS, table, key, val));

	sprintf(tmpname, "/etc/inet/sysid%d", getpid());
	sprintf(table_name, "/etc/inet/%s", tbl[table].etc.name);
	if ((wfp = fopen(tmpname, "w")) == NULL) {
		sprintf(tmpname, "/tmp/sysid%d", getpid());
		if ((wfp = fopen(tmpname, "w")) == NULL)
			return (FAILURE);
		sprintf(table_name, "/tmp/root/etc/inet/%s",
			tbl[table].etc.name);
	}

	if ((rfp = fopen(table_name, "r")) != NULL) {

		keypos = tbl[table].etc.key_fld;

		while (fgets(buff, 1024, rfp) == buff) {
			/* printf("%s", buff); */
			strcpy(dup, buff);

			p = strtok(buff, " \t\n");
			for (fld = 0; fld < keypos; fld++)
				p = strtok(NULL, " \t\n");

			if (p && strcmp(p, key) == 0) {
				if (fputs(val, wfp) == EOF)
					break;
				done = 1;
			} else
				if (fputs(dup, wfp) == EOF)
					break;
		}

		fclose(rfp);
	}

	if (!done)
		fputs(val, wfp);

	fclose(wfp);

	if (rename(tmpname, table_name))
		(void) fprintf(debugfp, "rename failed: %s %s\n",
			tmpname, table_name);

	return (SUCCESS);
}

int
get_entry(int ns_type, int table, char *key, char *val)
{
	int cnt, status;
	static int dead = 0;

	(void) fprintf(debugfp, "get_entry %s\n", key);

	if (dead) {
		(void) fprintf(debugfp, "get_entry: dead!\n");
		return (NS_TRYAGAIN);
	}

	for (cnt = 0; cnt < 5; cnt++) {
		if (testing)
			status = (*sim_handle())(SIM_DB_LOOKUP, ns_type, table,
			    key, val);

		else if (ns_type == NISPLUS)
			status = nis_get_entry(table, key, val);
		else
			status = yp_get_entry(table, key, val);

		switch (status) {
		case NS_SUCCESS:
			(void) fprintf(debugfp, "get_entry got: %s\n", val);
			return (SUCCESS);
		case NS_NOTFOUND:
			(void) fprintf(debugfp, "get_entry: failed\n");
			return (NS_NOTFOUND);
		default:
			break;
		}

		if (cnt < 4) {
			(void) fprintf(debugfp, "get_entry: retrying...\n");
			sleep(3);
		}
	}

	(void) fprintf(debugfp, "get_entry: failed\n");
	dead = 1;
	return (NS_TRYAGAIN);
}

static int
nis_get_entry(int table, char *key, char *val)
{
	char 		index[NIS_MAXNAMELEN];
	nis_result 	*res;
	entry_col	*ec;

	/* Get the table object using expand name */
	res = nis_lookup((nis_name) tbl[table].nis.name, EXPAND_NAME);
	(void) fprintf(debugfp, "nis+: lookup status %d\n", res->status);
	switch (res->status) {
	case NIS_SUCCESS:
		break;
	case NIS_TRYAGAIN:
		return (NS_TRYAGAIN);
	default:
		return (NS_NOTFOUND);
	}

	/* Construct the search criteria for the table */
	(void) sprintf(index, "[%s=%s],%s.%s", tbl[table].nis.keyname,
		key, NIS_RES_OBJECT(res)->zo_name,
		NIS_RES_OBJECT(res)->zo_domain);
	(void) nis_freeresult(res);

	res = nis_list((const nis_name) index, 0, 0, 0);
	(void) fprintf(debugfp, "nis+: list status %d\n", res->status);
	switch (res->status) {
	case NIS_SUCCESS:
		/* FALLTHRU */
	case NIS_S_SUCCESS:
		break;
	case NIS_TRYAGAIN:
		return (NS_TRYAGAIN);
	default:
		return (NS_NOTFOUND);
	}

	if (NIS_RES_NUMOBJ(res) > 1)
		(void) fprintf(debugfp, "nis+: multiple entries: %s\n", index);

	ec = NIS_RES_OBJECT(res)[0].EN_data.en_cols.en_cols_val;
	if (! ec[1].ec_value.ec_value_len) {
		(void) fprintf(debugfp, "nis+: no value: %s\n", index);
		return (NS_NOTFOUND);
	}

	(void) strcpy(val, ec[1].ec_value.ec_value_val);
	return (NS_SUCCESS);
}

static jmp_buf env;

/* ARGSUSED */
static void
sigalarm_handler(int sig)
{
	longjmp(env, 1);
}

static int
yp_get_entry(int table, char *key, char *val)
{
	int	res, len, fld;
	char	*entry, *p;
	char	domain[DOM_NM_LN + 1];
	void (*savesig) (int);

	(void) sysinfo(SI_SRPC_DOMAIN, domain, DOM_NM_LN);

	savesig = signal(SIGALRM, sigalarm_handler);
	(void) alarm(30);

	if (setjmp(env) != 0) {
		fprintf(debugfp, "yp_get_entry: timed out!\n");
		(void) signal(SIGALRM, savesig);
		return (NS_TRYAGAIN);
	}

	/*
	 * the locale table is defined wrong.  it is indexed by its
	 * value instead of by the key, so it must be searched
	 * linearly to check for a match.
	 */
	if (table == LOCALE_TBL) {
		struct ypall_callback ypcb;

		ypcb.foreach = cb;
		ypcb.data = val;
		yp_locale_key = key;
		val[0] = 0;
		res = yp_all(domain, tbl[table].yp.name, &ypcb);
		(void) alarm(0);
		(void) signal(SIGALRM, savesig);
		(void) fprintf(debugfp, "yp: yp_all status: %d\n", res);

		switch (res) {
		case 0:
			break;
		case YPERR_YPBIND:
			return (NS_TRYAGAIN);
		default:
			return (NS_NOTFOUND);
		}

		if (val[0])
			return (NS_SUCCESS);
		return (NS_NOTFOUND);
	}

	res = yp_match(domain, tbl[table].yp.name, key, strlen(key),
		&entry, &len);
	(void) alarm(0);
	(void) signal(SIGALRM, savesig);
	(void) fprintf(debugfp, "yp: yp_match status: %d\n", res);

	switch (res) {
	case 0:
		break;
	case YPERR_YPBIND:
		return (NS_TRYAGAIN);
	default:
		return (NS_NOTFOUND);
	}

	p = strtok(entry, " \t\n");
	for (fld = 0; fld < tbl[table].yp.val_fld; fld++)
		p = strtok(NULL, " \t\n");

	if (p)
		strcpy(val, p);
	else
		strcpy(val, "");

	return (NS_SUCCESS);
}

/*ARGSUSED*/
static int
cb(int stat, char *key, int keylen, char *val, int vallen, char *data)
{
	char *p;

	if (stat == YP_TRUE) {
		val[vallen] = 0;
		p = strtok(val, " \t\n");

		if (p && strcmp(p, yp_locale_key) == 0) {
			key[keylen] = 0;
			strcpy(data, key);
			return (1);
		}
	}
	return (0);
}

/*
 * Get the name services in use by this system.
 */
void
system_namesrv(char *domainname, char *ns_type)
{
	int fd;
	char tmp_str[MAXPATHLEN+1];
	nis_result *rp;
	char domain_buf[MAX_DOMAINNAME+3];
	struct stat stat_buf;

	if (testing) {
		(*sim_handle())(SIM_GET_NSINFO, domainname, ns_type);
		return;
	}

	/*
	 * See if NIS+ in use.
	 */
	if ((fd = open("/var/nis/NIS_COLD_START", O_RDONLY)) > 0) {
		fprintf(debugfp, "name service found = NIS+\n");
		(void) close(fd);
		/* Make sure name ends in a period */
		if (domainname[strlen(domainname)-1] == '.') {
			(void) strcpy(domain_buf, domainname);
		} else {
			(void) sprintf(domain_buf, "%s.", domainname);
		}
		rp = nis_lookup(domain_buf, (u_long) NO_CACHE);
		if ((rp->status == NIS_SUCCESS) ||
		    (rp->status == NIS_S_SUCCESS)) {
			(void) sprintf(ns_type, DB_VAL_NS_NIS_PLUS);
			return;
		}
	}

	/*
	 * See if NIS in use.
	 */
	(void) sprintf(tmp_str, "/var/yp/binding/%s", domainname);
	fprintf(debugfp, "stat %s\n", tmp_str);

	if (stat(tmp_str, &stat_buf) == 0) {
		(void) fprintf(debugfp, "name service found = NIS\n");
		(void) sprintf(ns_type, DB_VAL_NS_NIS);
		return;
	}
	else
		(void) fprintf(debugfp, "error on stat = %d\n", errno);

	/*
	 * Return that UFS is in use.
	 */
	fprintf(debugfp, "name service found = UFS\n");
	(void) sprintf(ns_type, DB_VAL_NS_UFS);
}
