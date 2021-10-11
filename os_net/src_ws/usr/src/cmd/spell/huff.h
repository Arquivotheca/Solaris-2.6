/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)huff.h	1.6	94/09/23 SMI"	/* SVr4.0 1.2	*/
extern struct huff {
	long xn;
	int xw;
	long xc;
	long xcq;	/* (c,0) */
	long xcs;	/* c left justified */
	long xqcs;	/* (q-1,c,q) left justified */
	long xv0;
} huffcode;
#define	n huffcode.xn
#define	w huffcode.xw
#define	c huffcode.xc
#define	cq huffcode.xcq
#define	cs huffcode.xcs
#define	qcs huffcode.xqcs
#define	v0 huffcode.xv0

double	huff(float);
int	rhuff(FILE *);
void	whuff(void);
