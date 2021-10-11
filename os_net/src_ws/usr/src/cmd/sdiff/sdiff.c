/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)sdiff.c	1.10	96/04/18 SMI"	/* SVr4.0 1.4.1.7	*/
	/*	sdiff [-l] [-s] [-w #] [-o output] file1 file2
	*	does side by side diff listing
	*	-l leftside only for identical lines
	*	-s silent; only print differences
	*	-w # width of output
	*	-o output  interactive creation of new output
		commands:
			s	silent; do not print identical lines
			v	turn off silent
			l	copy left side to output
			r	copy right side to output
			e l	call ed with left side
			e r	call ed with right side
			e b	call ed with cat of left and right
			e	call ed with empty file 
			q	exit from program

	*	functions:
		cmd	decode diff commands
		put1	output left side
		put2	output right side
		putmid	output gutter
		putline	output n chars to indicated file
		getlen	calculate length of strings with tabs
		cmdin	read and process interactive cmds
		cpp	copy from file to file
		edit	call ed with file
	*/
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <limits.h>

#define	LMAX	BUFSIZ
#define	BMAX	BUFSIZ
#define STDOUT 1
#define WGUTTER 6
#define WLEN	(WGUTTER * 2 + WGUTTER + 2)
#define PROMPT '%'

char	BLANKS[100] = "                                                                                                ";
char	GUTTER[WGUTTER] = "     ";
char	*DIFF	= "diff -b ";
char	diffcmd[BMAX];
char	inbuf[10];

int	llen	= 130;		/* Default maximum line length written out */
int	hlen;		/* Half line length with space for gutter */
int	len1;		/* Calculated length of left side */
int	nchars;		/* Number of characters in left side - used for tab expansion */
char	change = ' ';
int	leftonly = 0;	/* if set print left side only for identical lines */
int	silent = 0;	/* if set do not print identical lines */
int	midflg = 0;	/* set after middle was output */
int	rcode = 0;	/* return code */

char *pgmname;

char	*file1;
FILE	*fdes1;

char	*file2;
FILE	*fdes2;

FILE	*diffdes;

int oflag;
char	*ofile;
FILE	*odes;

char	*ltemp;
FILE	*left;

char	*rtemp;
FILE	*right;

FILE *tempdes;
char *temp;

int from1, to1, from2, to2;		/* decoded diff cmd- left side from to; right side from, to */
int num1, num2;		/*line count for left side file and right */

char *filename();
char *fgetline();
char *mktemp();
FILE *popen();

main(argc,argv)
int argc;
char	**argv;
{
	extern onintr();
	int com;
	register int n1, n2, n;
	register char *bp;

	if (signal(SIGHUP,SIG_IGN)!=SIG_IGN)
#ifdef __STDC__
		signal((int)SIGHUP,(void (*)(int))onintr);
#else
		signal((int)SIGHUP,(void (*))onintr);
#endif
	if (signal(SIGINT,SIG_IGN)!=SIG_IGN)
#ifdef __STDC__
		signal((int)SIGINT,(void (*)(int))onintr);
#else
		signal((int)SIGINT,(void (*))onintr);
#endif
	if (signal(SIGPIPE,SIG_IGN)!=SIG_IGN)
#ifdef __STDC__
		signal((int)SIGPIPE,(void (*)(int))onintr);
#else
		signal((int)SIGPIPE,(void (*))onintr);
#endif
	if (signal(SIGTERM,SIG_IGN)!=SIG_IGN)
#ifdef __STDC__
		signal((int)SIGTERM,(void (*)(int))onintr);
#else
		signal((int)SIGTERM,(void (*))onintr);
#endif

	setlocale(LC_ALL, "");

	pgmname = argv[0];
	while(--argc>1 && **++argv == '-'){
		switch(*++*argv){

		case 'w':
			/* -w# instead of -w # */
			if(*++*argv)
				llen = atoi(*argv);
			else {
				argc--;
				llen = atoi(*++argv);
			}
			if(llen < WLEN) 
				error("Wrong line length %s",*argv);
			if(llen > LMAX)
				llen = LMAX;
			break;

		case 'l':
			leftonly++;
			break;

		case 's':
			silent++;
			break;
		case 'o':
			oflag++;
			argc--;
			ofile = *++argv;
			break;
		default:
			error("Illegal argument: %s",*argv);
		}
	}
	if(argc != 2){
		fprintf(stderr,"Usage: sdiff [-l] [-s] [-o output] [-w #] file1 file2\n");
		exit(2);
	}

	file1 = *argv++;
	file2 = *argv;
	file1=filename(file1,file2);
	file2=filename(file2,file1);
	hlen = (llen - WGUTTER +1)/2;

	if((fdes1 = fopen(file1,"r")) == NULL)
		error("Cannot open: %s",file1);

	if((fdes2 = fopen(file2,"r")) == NULL)
		error("Cannot open: %s",file2);

	if(oflag){
		if(!temp)
			temp = mktemp("/tmp/sdiffXXXXXX");
		ltemp = mktemp("/tmp/sdifflXXXXXX");
		if((left = fopen(ltemp,"w")) == NULL)
			error("Cannot open temp %s",ltemp);
		rtemp = mktemp("/tmp/sdiffrXXXXXX");
		if((right = fopen(rtemp,"w")) == NULL)
			error("Cannot open temp file %s",rtemp);
		if((odes = fopen(ofile,"w")) == NULL)
			error("Cannot open output %s",ofile);
	}
	/* Call DIFF command */
	strcpy(diffcmd,DIFF);
	strcat(diffcmd,file1);
	strcat(diffcmd," ");
	strcat(diffcmd,file2);
	diffdes = popen(diffcmd,"r");

	num1 = num2 = 0;

	/* Read in diff output and decode commands
	*  "change" is used to determine character to put in gutter
	*  num1 and num2 counts the number of lines in file1 and 2
	*/

	n = 0;
	while((bp = fgetline(diffdes)) != NULL){
		change = ' ';
		com = cmd(bp);

	/* handles all diff output that is not cmd
	   lines starting with <, >, ., --- */
		if(com == 0)
			continue;

	/* Catch up to from1 and from2 */
		rcode = 1;
		n1=from1-num1;
		n2=from2-num2;
		n= n1>n2?n2:n1;
		if(com =='c' && n>0)
			n--;
		if(silent)
			fputs(bp,stdout);
		while(n-- > 0){
			put1();
			put2();
			if(!silent)
				putc('\n',stdout);
			midflg = 0;
		}

	/* Process diff cmd */
		switch(com){

		case 'a':
			change = '>';
			while(num2<to2){
				put2();
				putc('\n',stdout);
				midflg = 0;
			}
			break;

		case 'd':
			change = '<';
			while(num1<to1){
				put1();
				putc('\n',stdout);
				midflg = 0;
			}
			break;

		case 'c':
			n1 = to1-from1;
			n2 = to2-from2;
			n = n1>n2?n2:n1;
			change = '|';
			do {
				put1();
				put2();
				putc('\n',stdout);
				midflg = 0;
			} while(n--);

			change = '<';
			while(num1<to1){
				put1();
				putc('\n',stdout);
				midflg = 0;
			}

			change = '>';
			while(num2<to2){
				put2();
				putc('\n',stdout);
				midflg = 0;
			}
			break;

		default:
			fprintf(stderr,"%c: cmd not found\n",cmd);
			break;
		}

		if(oflag==1 && com!=0){
			cmdin();
			if((left = fopen(ltemp,"w")) == NULL)
				error("main: Cannot open temp %s",ltemp);
			if((right = fopen(rtemp,"w")) == NULL)
				error("main: Cannot open temp %s",rtemp);
		}
	}
	/* put out remainder of input files */
	while(put1()){
		put2();
		if(!silent)
			putc('\n',stdout);
		midflg = 0;
	}
	if(odes)
		fclose(odes);
	sremove();
	exit(rcode);
}

put1()
{
	/* len1 = length of left side */
	/* nchars = num of chars including tabs */

	register char *bp;

	if((bp = fgetline(fdes1)) != NULL){
		len1 = getlen(0,bp);
		if((!silent || change != ' ') && len1 != 0)
			putline(stdout,bp,nchars);
		
		if(oflag){
		/*put left side either to output file
		  if identical to right
		  or left temp file if not */

			if(change == ' ')
				putline(odes,bp,strlen(bp));
			else
				putline(left,bp,strlen(bp));
		}
		if(change != ' ')
			putmid(1);
		num1++;
		return(1);
	} else 
		return(0);
}

put2()
{

	register char *bp;

	if((bp = fgetline(fdes2)) != NULL){
		getlen((hlen+WGUTTER)%8,bp);

		/* if the left and right are different they are always
		   printed.
		   If the left and right are identical
		   right is only printed if leftonly is not specified
		   or silent mode is not specified 
		   or the right contains other than white space (len1 !=0)
		*/
		if(change != ' '){
		
		/* put right side to right temp file only
		   because left side was written to output for 
		   identical lines */

			if(oflag)
				putline(right,bp,strlen(bp));
			
			if(midflg == 0)
				putmid(1);
			putline(stdout,bp,nchars);
		} else
			if(!silent && !leftonly && len1!=0) {
				if(midflg == 0)
					putmid(1);
				putline(stdout,bp,nchars);
			}
		num2++;
		len1 = 0;
		return(1);
	} else {
		len1 = 0;
		return(0);
	}
}

putline(file,start,num)
FILE *file;
char *start;
int num;
{

	register char *cp, *end;
	register i, len, d_col;
	wchar_t	 wc;

	cp = start;
	end = cp + num;
	while(cp < end) {
		if (isascii(*cp)) {	
			putc(*cp++, file);
			continue;
		}

		if ((len = end - cp) > MB_LEN_MAX)
			len = MB_LEN_MAX;

		if ((len = mbtowc(&wc, cp, len)) <= 0) {
			putc(*cp++, file);
			continue;
		}

		if ((d_col = wcwidth(wc)) <= 0)
			d_col = len;

		if ((cp + d_col) > end)
			return;
			
		for (i = 0; i < len; i++)
			putc(*cp++, file);
	}
}

cmd(start)
char *start;
{

	char *cp, *cps;
	int com;

	if(*start == '>' || *start == '<' || *start == '-' || *start == '.')
		return(0);

	cp = cps = start;
	while(isdigit(*cp))
		cp++;
	from1 = atoi(cps);
	to1 = from1;
	if(*cp == ','){
		cp++;
		cps = cp;
		while(isdigit(*cp))
			cp++;
		to1 = atoi(cps);
	}

	com = *cp++;
	cps = cp;

	while(isdigit(*cp))
		cp++;
	from2 = atoi(cps);
	to2 = from2;
	if(*cp == ','){
		cp++;
		cps = cp;
		while(isdigit(*cp))
			cp++;
		to2 = atoi(cps);
	}
	return(com);
}

getlen(startpos,buffer)
unsigned char *buffer;
int startpos;
{
	/* get the length of the string in buffer
	*  expand tabs to next multiple of 8
	*/

	register unsigned char *cp;
	register int slen, tlen, len, d_col;
	int notspace;
	wchar_t	wc;

	nchars = 0;
	notspace = 0;
	tlen = startpos;
	for(cp=buffer; *cp != '\n'; cp++){
		if(*cp == '\t'){
			slen = tlen;
			tlen += 8 - (tlen%8);
			if(tlen>=hlen) {
				tlen= slen;
				break;
			}
			nchars++;
			continue;
		}

		if (isascii(*cp)) {
			slen = tlen;
			tlen++;
			if(tlen>=hlen) {
				tlen= slen;
				break;
			}
			if(!isspace(*cp))
				notspace = 1;
			nchars++;
			continue;
		}

		if ((len = mbtowc(&wc, (char *)cp, MB_LEN_MAX)) <= 0) {
			slen = tlen;
			tlen++;
			if(tlen>=hlen) {
				tlen= slen;
				break;
			}
			notspace = 1;
			nchars++;
			continue;
		}

		if ((d_col = wcwidth(wc)) <= 0)
			d_col = len;

		slen = tlen;
		tlen += d_col;
		if(tlen > hlen) {
			tlen= slen;
			break;
		}
		notspace = 1;
		cp += len - 1;
		nchars += len;
	}
	return(notspace?tlen:0);
}

putmid(bflag)
int bflag;
{
	/* len1 set by getlen to the possibly truncated
	*  length of left side
	*  hlen is length of half line
	*/


	midflg = 1;
	if(bflag)
		putline(stdout,BLANKS,(hlen-len1));
	GUTTER[2] = change;
	putline(stdout,GUTTER,5);
}

error(s1,s2)
char *s1, *s2;
{
	fprintf(stderr,"%s: ",pgmname);
	fprintf(stderr,s1,s2);
	putc('\n',stderr);
	sremove();
	exit(2);
}

onintr()
{
	sremove();
	exit(rcode);
}

sremove()
{
	if(ltemp)
		unlink(ltemp);
	if(rtemp)
		unlink(rtemp);
	if(temp)
		unlink(temp);
}

cmdin()
{
	char *cp, *ename;
	int notacc;

	fclose(left);
	fclose(right);
	notacc = 1;
	while(notacc){
		putc(PROMPT,stdout);
		cp = fgets(inbuf,10,stdin);
		switch(*cp){

		case 's':
			silent = 1;
			break;

		case 'v':
			silent = 0;
			break;

		case 'q':
			sremove();
			exit(rcode);

		case 'l':
			cpp(ltemp,left,odes);
			notacc = 0;
			break;

		case 'r':
			cpp(rtemp,right,odes);
			notacc = 0;
			break;

		case 'e':
			while(*++cp == ' ')
				;
			switch(*cp){
			case 'l':
			case '<':
				notacc = 0;
				ename = ltemp;
				edit(ename);
				break;

			case 'r':
			case '>':
				notacc = 0;
				ename = rtemp;
				edit(ename);
				break;

			case 'b':
			case '|':
				if((tempdes = fopen(temp,"w")) == NULL)
					error("Cannot open temp file %s",temp);
				cpp(ltemp,left,tempdes);
				cpp(rtemp,right,tempdes);
				fclose(tempdes);
				notacc = 0;
				ename = temp;
				edit(ename);
				break;

			case '\n':
				if((tempdes=fopen(temp,"w")) == NULL)
					error("Cannot open temp %s",temp);
				fclose(tempdes);
				notacc = 0;
				ename = temp;
				edit(ename);
				break;
			default:
				fprintf(stderr,"Illegal command %s reenter\n",cp);
				break;
			}
			if(notacc == 0)
				cpp(ename,tempdes,odes);
			break;

		default:
			fprintf(stderr,"Illegal command reenter\n");
			break;
		}
	}
}

cpp(from,fromdes,todes)
char *from;
FILE *fromdes,*todes;
{
	char tempbuf[BMAX+1];

	if((fromdes = fopen(from,"r")) == NULL)
		error("cpp: Cannot open %s",from);
	while((fgets(tempbuf,BMAX,fromdes) != NULL))
		fputs(tempbuf,todes);
	fclose(fromdes);
}

edit(file)
char *file;
{
	int i;
	pid_t pid;

	void (*oldintr) ();

	switch(pid=fork()){

	case (pid_t)-1:
		error("Cannot fork","");
	case (pid_t)0:
		execl("/usr/bin/ed", "ed", file, 0);
	}

	oldintr = signal(SIGINT, SIG_IGN);	/*ignore interrupts while in ed */
	while(pid != wait(&i))
		;
	signal(SIGINT,oldintr);		/*restore previous interrupt proc */
}

char *filename(pa1, pa2)
char *pa1, *pa2;
{
	register int c;
	register char  *a1, *b1, *a2;
	struct stat stbuf;
	a1 = pa1;
	a2 = pa2;
	if(stat(a1,&stbuf)!=-1 && ((stbuf.st_mode&S_IFMT)==S_IFDIR)) {
		b1 = pa1 = (char *)malloc(100);
		while(*b1++ = *a1++) ;
		b1[-1] = '/';
		a1 = b1;
		while(*a1++ = *a2++)
			if(*a2 && *a2!='/' && a2[-1]=='/')
				a1 = b1;
	}
	else if(a1[0] == '-' && a1[1] == 0 && temp ==0) {
		if (fstat(fileno(stdin), &stbuf) == -1)
			error("Cannot process stdin");
		pa1 = temp = mktemp("/tmp/sdiffXXXXXX");
		if((tempdes = fopen(temp,"w")) == NULL)
			error("Cannot open temp %s",temp);
		while((c=getc(stdin)) != EOF)
			putc(c,tempdes);
		fclose(tempdes);
	}
	return(pa1);
}

/*
 * like fgets, but reads upto and including a newline, 
 * the data is stored in a reusable dynamic buffer that grows to fit
 * the largest line in the file, the buffer is NULL terminated
 * returns a pointer to the dynamic buffer.
 */
char *
fgetline(fp)
FILE *fp;
{
	
	static char *bp = NULL;
	static int blen = 0;
	char *ret;
	int sl;
	int c;

	if ( bp == NULL )
	{
		/* allocate it for the first time */
		bp = malloc(BUFSIZ);
		if ( bp == NULL )
			error("fgetline: malloc failed", NULL);
		blen = BUFSIZ;
	}

	fgets(bp, blen, fp);

	if ( feof(fp) )
		return NULL;

	while ( (sl = strlen(bp)) == blen-1 && *(bp+blen-2) != '\n' )
	{
		/* still more data, grow the buffer */
		blen *= 2;
		bp = realloc(bp, blen);
		if ( bp == NULL )
			error("fgetline: realloc failed", NULL);
		/* continue reading and add to end of buffer */
		fgets(bp+sl, blen-sl, fp);
	}
	return bp;
}
