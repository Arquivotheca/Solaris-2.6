/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)point.c	1.5	92/07/14 SMI"	/* SVr4.0 1.2	*/

point(xi,yi){
		move(xi,yi);
		label(".");
		return;
}
