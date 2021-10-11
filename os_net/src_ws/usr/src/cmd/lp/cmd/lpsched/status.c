/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)status.c	1.11	96/04/10 SMI"	/* SVr4.0 1.7.1.3	*/

#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include <syslog.h>

#include "lpsched.h"

#define NCMP(X,Y)	(STRNEQU((X), (Y), sizeof(Y)-1))

extern char *LP_TRAY_UNMOUNT;

static void		load_pstatus ( void );
static void		load_fault_status ( void );
static void		load_cstatus ( void );
static void		put_multi_line ( FILE * , char * );
static PFSTATUS * parseFormList ( char *,short *);
static void markFormsMounted( PSTATUS *);


#define FAULT_MESSAGE_FILE "faultMessage"
static char		*pstatus	= 0,
			*cstatus	= 0;

/**
 ** load_status() - LOAD PRINTER/CLASS STATUS FILES
 **/

void
load_status(void)
{
	load_pstatus ();

	load_cstatus ();
	load_fault_status ();
	return;
}

/**
 ** load_pstatus() - LOAD PRITNER STATUS FILE
 **/

static void
load_pstatus(void)
{
	PSTATUS			*pps;

	char			*rej_reason,
				*dis_reason,
				*pwheel_name,
				buf[BUFSIZ],
				*name,
				*p;

	time_t			rej_date,
				dis_date;

	short			status;

	PFSTATUS		*ppfs;

	PWSTATUS		*ppws;

	int			i,
				len,
				total;

	time_t			now;

	register FILE		*fp;

	register int		f;
	short			numForms;


	(void) time(&now);

	if (!pstatus)
		pstatus = makepath(Lp_System, PSTATUSFILE, (char *)0);
	if ((fp = open_lpfile(pstatus, "r", 0)))
		while (!feof(fp)) {
			status = 0;
			total = 0;
			name = 0;
			rej_reason = 0;
			dis_reason = 0;
			ppfs = 0;

			for (f = 0; f < PST_MAX && fgets(buf, BUFSIZ, fp);
			    f++) {
				if (p = strrchr(buf, '\n'))
					*p = '\0';

				switch (f) {
				case PST_BRK:
					break;

				case PST_NAME:
					name = Strdup(buf);
					break;

				case PST_STATUS:
					if (NCMP(buf, NAME_DISABLED))
						status |= PS_DISABLED;
					p = strchr(buf, ' ');
					if (!p || !*(++p))
						break;
					if (NCMP(p, NAME_REJECTING))
						status |= PS_REJECTED;
					break;

				case PST_DATE:
					dis_date = (time_t)atol(buf);
					p = strchr(buf, ' ');
					if (!p || !*(++p))
						break;
					rej_date = (time_t)atol(p);
					break;

				case PST_DISREAS:
					len = strlen(buf);
					if (buf[len - 1] == '\\') {
						buf[len - 1] = '\n';
						f--;
					}
					if (dis_reason) {
						total += len;
						dis_reason = Realloc(
							dis_reason,
							total+1
						);
						strcat (dis_reason, buf);
					} else {
						dis_reason = Strdup(buf);
						total = len;
					}
					break;

				case PST_REJREAS:
					len = strlen(buf);
					if (buf[len - 1] == '\\') {
						buf[len - 1] = '\n';
						f--;
					}
					if (rej_reason) {
						total += len;
						rej_reason = Realloc(
							rej_reason,
							total+1
						);
						strcat (rej_reason, buf);
					} else {
						rej_reason = Strdup(buf);
						total = len;
					}
					break;

				case PST_PWHEEL:
					if (*buf) {
						ppws = search_pwtable(buf);
						pwheel_name = Strdup(buf);
					} else {
						ppws = 0;
						pwheel_name = 0;
					}
					break;

				case PST_FORM:
					ppfs = parseFormList (buf,&numForms);
					break;
				}
			}

			if (ferror(fp) || f && f != PST_MAX) {
				close_lpfile (fp);
				note("Had trouble reading file %s", pstatus);
				return;
			}

			if (!feof(fp) && name && (pps = search_ptable(name))) {
				pps->rej_date = rej_date;
				pps->status |= status;
				pps->forms = ppfs;
				if (ppfs) markFormsMounted(pps);
				pps->numForms = numForms;
				pps->pwheel_name = pwheel_name;
				if ((pps->pwheel = ppws))
					ppws->mounted++;
				pps->rej_reason = rej_reason;
				load_str(&pps->fault_reason, CUZ_PRINTING_OK);
				if (pps->printer->login) {
					pps->dis_date = now;
					pps->dis_reason =
						Strdup(CUZ_LOGIN_PRINTER);
				} else {
					pps->dis_date = dis_date;
					pps->dis_reason = dis_reason;
				}

			} else {
				if (ppfs)
					Free(ppfs);
				if (dis_reason)
					Free (dis_reason);
				if (rej_reason)
					Free (rej_reason);
			}
			if (name)
				Free (name);
		}

	if (fp) {
		if (ferror(fp)) {
			close_lpfile (fp);
			note("Had trouble reading file %s", pstatus);
			return;
		}
		close_lpfile (fp);
	}

	for (i = 0; i < PT_Size; i++)
		if (PStatus[i].printer->name && !PStatus[i].rej_reason) {
			PStatus[i].dis_reason = Strdup(CUZ_NEW_PRINTER);
			PStatus[i].rej_reason = Strdup(CUZ_NEW_DEST);
			PStatus[i].fault_reason = Strdup(CUZ_PRINTING_OK);
			PStatus[i].dis_date = now;
			PStatus[i].rej_date = now;
			PStatus[i].status |= PS_DISABLED | PS_REJECTED;
		}

	return;
}

/**
 ** load_fault_status() - LOAD PRITNER Fault STATUS FILE
 **/

static void 
load_fault_status(void)
{
	PSTATUS			*pps;

	char			*fault_reason = NULL,
				buf[BUFSIZ],
				*fault_status,
				*printerName,
				*p;

	int			i,
				len,
				total;


	register FILE		*fp;

	for (i = 0; i < PT_Size; i++) {
		printerName = PStatus[i].printer->name;
		if (printerName) {
			fault_status = makepath(Lp_A_Printers, printerName,
				FAULT_MESSAGE_FILE , (char *) 0);
			fault_reason = NULL;
			total = 0;

			if ((fp = open_lpfile(fault_status, "r", 0))) {
				while (fgets(buf, BUFSIZ, fp)) {
					len = strlen(buf);
					if (fault_reason) {
						total += len;
						fault_reason =
							Realloc(fault_reason,
								total+1);
						strcat (fault_reason, buf);
					} else {
						fault_reason = Strdup(buf);
						total = len;
					}
				}

				if (fault_reason &&
				    (pps = search_ptable(printerName))) {
					p = fault_reason + strlen(fault_reason)
						- 1;
					if (*p == '\n')
						*p = 0;
					load_str(&pps->fault_reason,
						fault_reason);
				}
				if (fault_reason)
					Free(fault_reason);

				fclose(fp);
			}
			Free(fault_status);
		}
	}
}

/**
 ** load_form_msg() - LOAD PRITNER Form STATUS FILE
 **/

char * 
load_form_msg(FSTATUS *pfs)
{
	char		*formMsg = NULL,
				buf[BUFSIZ];
	int			len, total;
	register FILE		*fp;
	char *msgFile;


	msgFile = makepath(Lp_A_Forms, pfs->form->name, FORMMESSAGEFILE,
			(char * )NULL);
	formMsg = NULL;
	total = 0;
	if ((fp = open_lpfile(msgFile, "r", 0))) {
		while (fgets(buf, BUFSIZ, fp)) {

			len = strlen(buf);
			if (formMsg) {
				total += len;
				formMsg = Realloc( formMsg, total+1);
				strcat (formMsg, buf);
			} else {
				formMsg = Strdup(buf);
				total = len;
			}
		}
	}
	Free(msgFile);
	return(formMsg);
}

/**
 ** load_cstatus() - LOAD CLASS STATUS FILE
 **/

static void
load_cstatus(void)
{
	CSTATUS			*pcs;
	char			*rej_reason,
				buf[BUFSIZ],
				*name,
				*p;
	time_t			rej_date;
	short			status;
	int			i,
				len,
				total;
	time_t			now;
	register FILE		*fp;
	register int		f;


	(void) time(&now);

	if (!cstatus) 
		cstatus = makepath(Lp_System, CSTATUSFILE, (char *)0);

	if ((fp = open_lpfile(cstatus, "r", 0)))
		while (!feof(fp)) {
			status = 0;

			total = 0;
			name = 0;

			rej_reason = 0;
			for (f = 0; f < CST_MAX && fgets(buf, BUFSIZ, fp);
			    f++) {
				if (p = strrchr(buf, '\n'))
					*p = '\0';
				switch (f) {
				case CST_BRK:
					break;

				case CST_NAME:
					name = Strdup(buf);
					break;

				case CST_STATUS:
					if (NCMP(buf, NAME_REJECTING))
						status |= PS_REJECTED;
					break;

				case CST_DATE:
					rej_date = (time_t)atol(buf);
					break;

				case CST_REJREAS:
					len = strlen(buf);
					if (buf[len - 1] == '\\') {
						buf[len - 1] = '\n';
						f--;
					}
					if (rej_reason) {
						total += len;
						rej_reason = Realloc(
							rej_reason,
							total+1
						);
						strcat (rej_reason, buf);
					} else {
						rej_reason = Strdup(buf);
						total = len;
					}
					break;
				}
			}

			if (ferror(fp) || f && f != CST_MAX) {
				close_lpfile (fp);
				note("Had trouble reading file %s", cstatus);
				return;
			}

			if (!feof(fp) && name && (pcs = search_ctable(name))) {
				pcs->rej_reason = rej_reason;
				pcs->rej_date = rej_date;
				pcs->status |= status;

			} else
				if (rej_reason)
					Free (rej_reason);

			if (name)
				Free (name);
		}

	if (fp) {
		if (ferror(fp)) {
			close_lpfile (fp);
			note("Had trouble reading file %s", cstatus);
			return;
		}
		close_lpfile (fp);
	}

	for (i = 0; i < CT_Size; i++)
		if (CStatus[i].class->name && !CStatus[i].rej_reason) {
			CStatus[i].status |= CS_REJECTED;
			CStatus[i].rej_reason = Strdup(CUZ_NEW_DEST);
			CStatus[i].rej_date = now;
		}

	return;
}

/**
 ** showForms()
 **/
char *
showForms(PSTATUS  *pps)
{
	int i;
	char			*formList = NULL;
	char buf[100];
	FSTATUS *pfs;
	PFSTATUS  *ppfs;
	short numForms;

	numForms = pps->numForms;
	ppfs = pps->forms;
	if (ppfs) {
		for (i = 0; i < numForms; i++) {
			pfs = ppfs[i].form;
			sprintf(buf, "%s%c", (pfs ? pfs->form->name : ""),
				*LP_SEP);

			if (addstring(&formList,buf)) { /* allocation failed */
				if (formList) {  
					Free(formList);
					formList = NULL;
				}
				return(NULL);
			}
		}
	}
	return(formList);
}

/**
 ** markFormsMounted()
 **/

void
markFormsMounted(PSTATUS *pps)
{
	int i;
	int numTrays;
	PFSTATUS *ppfs;
	FSTATUS *pfs;


	ppfs = pps->forms;
	if (ppfs) {
		numTrays = pps->numForms;
		for (i = 0; i < numTrays; i++) {
			pfs = ppfs[i].form;
			if (pfs)
				pfs->mounted++;
		}
	}
}

/**
 ** parseFormList()
 **/

static PFSTATUS *
parseFormList(char *formList, short *num)
{
	int i;
	FSTATUS *pfs;
	PFSTATUS  *ppfs;
	short numForms=0;
	char *endPtr,*ptr;


	ptr = strchr(formList,*LP_SEP);
	while (ptr)  {
		numForms++;
		ptr = strchr(ptr+1,*LP_SEP);
	}
	if ((numForms == 0) && (*formList))
		numForms = 1;

	if (numForms &&
	    (ppfs = (PFSTATUS *) Calloc(numForms, sizeof(PFSTATUS)))) {
		endPtr = strchr(formList,*LP_SEP);
		if (!endPtr)
			endPtr = formList + strlen(formList);

		ptr = formList;
		for (i = 0; endPtr && (i < numForms); i++) {
			*endPtr = 0;
			ppfs[i].form = pfs = search_ftable(ptr);
			ppfs[i].isAvailable =
				((pfs || (!LP_TRAY_UNMOUNT)) ? 1 : 0);
			ptr = endPtr+1;
			endPtr = strchr(ptr,*LP_SEP);
		}
		*num = numForms;
	} else {
		ppfs = NULL;
		*num = 0;
	}
	return(ppfs);
}

/**
 ** dump_pstatus() - DUMP PRINTER STATUS FILE
 **/

void
dump_pstatus(void)
{
	PSTATUS			*ppsend;
	FILE			*fp;
	register PSTATUS	*pps;
	register int		f;


	if (!pstatus)
		pstatus = makepath(Lp_System, PSTATUSFILE, (char *)0);
	if (!(fp = open_lpfile(pstatus, "w", MODE_READ))) {
		note ("Can't open file \"%s\" (%s).\n", pstatus, PERROR);
		return;
	}

	for (pps = PStatus, ppsend = &PStatus[PT_Size]; pps < ppsend; pps++)
		if (pps->printer->name)
			for (f = 0; f < PST_MAX; f++) switch (f) {
			case PST_BRK:
				(void)fprintf (fp, "+%s\n", STATUS_BREAK);
				break;
			case PST_NAME:
				(void)fprintf(fp, "%s\n",
					NB(pps->printer->name));
				break;
			case PST_STATUS:
				(void)fprintf(fp, "%s %s\n",
					(pps->status & PS_DISABLED ?
					    NAME_DISABLED : NAME_ENABLED),
					(pps->status & PS_REJECTED ?
					    NAME_REJECTING : NAME_ACCEPTING));
				break;
			case PST_DATE:
				(void)fprintf(fp, "%ld %ld\n", pps->dis_date,
					pps->rej_date);
				break;
			case PST_DISREAS:
				put_multi_line (fp, pps->dis_reason);
				break;
			case PST_REJREAS:
				put_multi_line (fp, pps->rej_reason);
				break;
			case PST_PWHEEL:
				(void)fprintf(fp, "%s\n", NB(pps->pwheel_name));
				break;
			case PST_FORM: {
				char *list;
				list = showForms(pps);
				(void)fprintf ( fp, "%s\n", (list ? list : ""));
				if (list)
					Free(list);
				break;
				}
			}

	close_lpfile (fp);

	return;
}

/**
 ** dump_fault_status() - DUMP PRINTER FAULT STATUS FILE
 **/

void
dump_fault_status(PSTATUS *pps)
{
	FILE		*fp;
	char		*fault_status, *printerName;

	printerName = pps->printer->name;
	fault_status = makepath(Lp_A_Printers, printerName, FAULT_MESSAGE_FILE,
			(char *) 0);
	if (!(fp = open_lpfile(fault_status, "w", MODE_READ))) {
		syslog(LOG_DEBUG, "Can't open file %s (%m)\n", fault_status);
	}
	else {
		fprintf(fp, "%s\n", pps->fault_reason);
		close_lpfile (fp);
	}

	Free(fault_status);
	return;
}

/**
 ** dump_form_status() - DUMP PRINTER FAULT STATUS FILE
 **/

void
dump_form_msg(FSTATUS *pfs, char *str)
{
	FILE	*fp;
	char	*msgFile, *msgDir;

	msgFile = makepath(Lp_A_Forms,pfs->form->name,FORMMESSAGEFILE,
			  (char * )NULL);
	if (!(fp = open_lpfile(msgFile, "w", MODE_READ))) {
		msgDir = makepath(Lp_A_Forms,pfs->form->name, (char * )NULL);
		mkdir(msgDir,MODE_EXEC); 
		Free(msgDir);
		if (!(fp = open_lpfile(msgFile, "w", MODE_READ))) {
			syslog(LOG_DEBUG, "Can't open file %s (%m)\n",
			       msgFile);
			return;
		}
	}

	fprintf(fp,"%s\n",str);

	close_lpfile (fp);
	Free(msgFile);

	return;
}

/**
 ** dump_cstatus() - DUMP CLASS STATUS FILE
 **/

void
dump_cstatus(void)
{
	CSTATUS			*pcsend;

	FILE			*fp;

	register CSTATUS	*pcs;

	register int		f;


	if (!cstatus)
		cstatus = makepath(Lp_System, CSTATUSFILE, (char *)0);
	if (!(fp = open_lpfile(cstatus, "w", MODE_READ))) {
		syslog(LOG_DEBUG, "Can't open file %s (%m)\n", cstatus);
		return;
	}

	for (pcs = CStatus, pcsend = &CStatus[CT_Size]; pcs < pcsend; pcs++)
		if (pcs->class->name)
			for (f = 0; f < CST_MAX; f++) switch (f) {
			case CST_BRK:
				(void)fprintf (fp, "%s\n", STATUS_BREAK);
				break;
			case CST_NAME:
				(void)fprintf (fp, "%s\n",
					NB(pcs->class->name));
				break;
			case CST_STATUS:
				(void)fprintf(fp, "%s\n",
					(pcs->status & CS_REJECTED ?
					    NAME_REJECTING : NAME_ACCEPTING)
				);
				break;
			case CST_DATE:
				(void)fprintf (fp, "%ld\n", pcs->rej_date);
				break;
			case CST_REJREAS:
				put_multi_line (fp, pcs->rej_reason);
				break;
			}

	close_lpfile (fp);

	return;
}

/**
 ** put_multi_line() - PRINT OUT MULTI-LINE TEXT
 **/

static void
put_multi_line(FILE *fp, char *buf)
{
	register char		*cp,
				*p;

	if (!buf) {
		(void)fprintf (fp, "\n");
		return;
	}

	for (p = buf; (cp = strchr(p, '\n')); ) {
		*cp++ = 0;
		(void)fprintf (fp, "%s\\\n", p);
		p = cp;
	}
	(void)fprintf (fp, "%s\n", p);
	return;
}
