#pragma ident "@(#)memcheck.cc   1.3     92/11/25 SMI"

/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

/************************************************************************
 *																		*
 *			Copyright (c) 1985 by										*
 *		Digital Equipment Corporation, Maynard, MA						*
 *			All rights reserved.										*
 *																		*
 *	 The information in this software is subject to change	without 	*
 *	 notice  and should not be construed as a commitment by Digital 	*
 *	 Equipment Corporation. 											*
 *																		*
 *	 Digital assumes no responsibility for the use	or	reliability 	*
 *	 of its software on equipment which is not supplied by Digital. 	*
 *																		*
 *   Redistribution and use in source and binary forms are permitted	*
 *   provided that the above copyright notice and this paragraph are	*
 *	 duplicated in all such forms and that any documentation,			*
 *	 advertising materials, and other materials related to such 		*
 *   distribution and use acknowledge that the software was developed	*
 *   by Digital Equipment Corporation. The name of Digital Equipment	*
 *   Corporation may not be used to endorse or promote products derived	*
 *	 from this software without specific prior written permission.		*
 *	 THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR     *
 *	 IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED 	*
 *   WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.*
 *   Do not take internally. In case of accidental ingestion, contact	*
 *	 your physician immediately.										*
 *																		*
 *	 (heavily modified by Patrick Curran, December 1991)				*
 *                                                                      *
 ************************************************************************/

/* $RCSfile: memcheck.cc $ $Revision: 1.2 $ $Date: 1992/09/12 15:23:04 $ */


#ifdef MEMDB

#include "precomp.h"

#ifndef  PRE_COMPILED_HEADERS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include "memconf.h"

#endif	// PRE_COMPILED_HEADERS


extern "C"
{
void CUI_fatal(char * format, ...);
}


//
//	globals
//

char *MEMfile = NULL;				// hint for memory-checker
int  MEMline = 0;					// hint for memory-checker


/*
 *	malloc() and free() recording: these functions manage a set
 *	of data files that are updated whenever a pointer is allocated or freed,
 *	as well as gathering stats about dynamic memory use and leakage.
 *
 *	Marcus J. Ranum, 1990. (mjr@decuac.dec.com)
 */


/*
 *	there is some egregious use of globals, void functions, and whatnot
 *	in this code. it is mostly due to the constraint that nothing must
 *	be visible to the outside, and that the code must be structurally
 *	simple. error checking is pitched out the window in spots, since an
 *	error in the memcheck code should not affect whatever is being
 *	instrumented if at all possible. this means no errors, no signals,
 *	nothing. (this message brought to you by my ego, in case you think
 *	I don't know how to write better code than this) :)
 *
 *	mjr, hacking on Christmas, 1990.
 */


#define REC_UNINIT  000
#define	REC_INITTED	001
#define	REC_ERR		002
#define	REC_ON		010
#define	REC_ONOFF	020
static	int	rec_state = REC_UNINIT;


/*
 *	this method of storing the symbol maps is not the most efficient, but
 *	then that's not important here, since all we're trying to do is find
 *	memory leaks. if you choose to improve the symbol management to use
 *	bucketed hash tables, please contact the author and the code will be
 *	updated :) - besides, since we do file I/O every time you malloc or
 *	free something, there's no way in hell this is going to set any new
 *	records for speed.
 */


/* storage for code/line # entry */

struct	sym
{
	char	*file;
	int		lineno;
	int		mapno;
	int		mallcnt;
	float	avsiz;
	struct	sym *next;
};


/* static symbol map */

static	struct
{
	FILE	*fp;
	FILE	*log;
	int	fd;

	long	nalloc; 				/* count of allocations */
	long	nrlloc; 				/* count of re-allocations */
	long	nfree;					/* count of frees */
	long	nbfree; 				/* count of bad frees */
	long	ninuse; 				/* known allocated memory in use */
	float	avgsiz; 				/* average malloc size */

	/* one entry per pointer returned by malloc */

	int pmap;						/* current next ptr map to alloc */
	struct	ptr *phash[MEM_HASHSIZE];

	/* one entry per line of code that calls malloc/realloc, etc */

	int lmap;						/* current next line map to alloc */
	struct	sym *shash[MEM_HASHSIZE];	 /* hash access */

} map;


/*
 *	we must make copies of filenames passed as hints;
 *	here's where we store them
 */

static char **hintFiles  = NULL;
static int	numHintFiles = 0;
static int	hintIndex	 = 0;


/*
 *	 return pointer to (possibly new copy) of a hinted filename
 */

static char *hintFile(char *file)
{
	/* first check to see if file is already stored */

	int i;
	char *newFile;

	for(i = 0; i < hintIndex; i++)
	{
		if(strcmp(hintFiles[i], file) == 0)
			return(hintFiles[i]);
	}

	/* we must add it - allocate/reallocate array if necessary */

	if(!hintFiles)
	{
		numHintFiles = 10;
		hintFiles = (char **)malloc(numHintFiles * sizeof(char *));
		if(!hintFiles)
			CUI_fatal("Out of memory for hintfiles array in memcheck.cc");
	}

	if(hintIndex >= numHintFiles)
	{
		hintFiles = (char **)realloc(hintFiles,
					(10 + numHintFiles) * sizeof(char *));
		if(!hintFiles)
			CUI_fatal("Out of memory for hintfiles array in memcheck.cc");
	}

	/* now add at end of array */

	newFile = strdup(file);
	if(!newFile)
		CUI_fatal("Out of memory for filename in memcheck.cc");

	hintFiles[hintIndex++] = newFile;
	return(newFile);
}


/*
 *	format a symbol map entry into specified buffer
 */

static void fsym(char *buffer, struct sym *s)
{
	sprintf(buffer," \"%s\"", s->file);
	if(s->lineno != -1)
		sprintf(buffer + strlen(buffer), " line:%d", s->lineno);
}


/*
 *	save an entry to the .lines file
 */

static void savesym(struct sym *s)
{
	if(map.fp == (FILE *)0)
		return;

	fprintf(map.fp, "%d\t%d\t%.1f\t%d\t%s\n",
			s->mapno, s->mallcnt, s->avsiz, s->lineno, s->file);
}


/*
 *	save an entry in the pointer map file
 */

static void saveptr(struct ptr *p)
{
	if(lseek(map.fd,(long)(p->map * sizeof(p->dsk)),0) !=
		(long)(p->map * sizeof(p->dsk)))
	{
		CUI_fatal("memcheck: cannot seek in pointer map file");
	}

	if(write(map.fd, (char *)&(p->dsk), sizeof(p->dsk)) != sizeof(p->dsk))
	{
		CUI_fatal("memcheck: cannot write in pointer map file");
	}
}


/* initialize everything - symbol tables, files, and maps */

static void initmap(void)
{
	register int	xp;

	if(rec_state & REC_INITTED)
		return;

	if((map.fp = fopen(LINESFILE,"w")) == (FILE *)0)
		return;
	if((map.fd = open(PTRFILE, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0600)) < 0)
	{
		fclose(map.fp);
		return;
	}

	map.log = stderr;
	map.lmap = map.pmap = 0;
	map.nalloc = map.nrlloc = map.nfree = map.nbfree = 0L;
	map.ninuse = 0L;
	map.avgsiz = 0.0;

	for(xp = 0; xp < MEM_HASHSIZE; xp++)
	{
		map.phash[xp] = (struct ptr *)0;
		map.shash[xp] = (struct sym *)0;
	}

	rec_state = REC_INITTED | REC_ON;
}


/* set logging to a FILE * */

void memSetLog(FILE *fp)
{
	map.log = fp;
}


/* return state of the recorder */

int memRecording(void)
{
	return((rec_state & REC_ON) && !(rec_state & REC_ERR));
}


/* turn on or off recording */

int memSetRecording(int val)
{
	if(val)
		rec_state |= REC_ON;
	else
		rec_state &= ~REC_ON;

	if(map.fp != (FILE *)0)
		fflush(map.fp);

	rec_state |= REC_ONOFF;
	return(0);
}


/* lookup a pointer record - search pointer hash table */

static struct ptr *lookupptr(void *ptr)
{
	register struct	ptr	*p;

	/* this probably give simply terrible hash performance */

	p = map.phash[(unsigned long)ptr % MEM_HASHSIZE];
	while(p != (struct ptr *)0)
	{
		if(ptr == p->ptr)
			return(p);
		p = p->next;
	}
	return((struct ptr *)0);
}


/*
 * polynomial conversion ignoring overflows
 * [this seems to work remarkably well, in fact better
 * then the ndbm hash function. Replace at your own risk]
 * use: 65599	nice.
 *      65587   even better. 
 * author: oz@nexus.yorku.ca
 */

static unsigned int dbm_hash(char *str)
{
	unsigned int n = 0;

	while(*str != '\0')
		n = *str++ + 65599 * n;
	return(n);
}


/* lookup a line/source entry by name (search hash table) */

static struct sym *lookupsymbyname(char *nam, int line)
{
	struct sym *s;
	char   *p = nam;

	if(p == (char *)0)
		p = "unknown";

	s = map.shash[(dbm_hash(p) + line) % MEM_HASHSIZE];
	while(s != (struct sym *)0)
	{
		if(!strcmp(s->file,nam) && s->lineno == line)
			return(s);
		s = s->next;
	}

	return((struct sym *)0);
}


/* lookup a line/source entry by number (exhaustively search hash table) */

static struct sym *lookupsymbynum(int num)
{
	struct sym *s;
	int 	   x;

	for(x = 0; x < MEM_HASHSIZE; x++)
	{
		s = map.shash[x];
		while(s != (struct sym *)0)
		{
			if(s->mapno == num)
				return(s);
			s = s->next;
		}
	}
	return((struct sym *)0);
}


/* stuff a pointer's value in the pointer map table */

static void storeptr(void *ptr, int size, char *file, int line)
{
	register struct	ptr	*p;
	register struct	sym	*s;
	int			hv;

	/*
	 *	if there is no existing symbol entry for this line of code...
	 *	we must needs make one - and painful it is...
	 */

	if((s = lookupsymbyname(file, line)) == (struct sym *)0)
	{
		s = (struct sym *)malloc(sizeof(struct sym));
		if(s == (struct sym *)0)
		{
			CUI_fatal("memcheck: cannot allocate sym entry");
		}

		/*
		 *	this is funky - since we know the filename is (?)
		 *	compiled-in, we can just keep a pointer to it,
		 *	rather than copying our own version of it.
		 */

		if(file != (char *)0)
			s->file = file;
		else
			s->file = "unknown";

		s->mapno = map.lmap++;

		/* add sym to hash table */

		s->next = map.shash[hv = ((dbm_hash(s->file) + line) % MEM_HASHSIZE)];
		map.shash[hv] = s;
 
		s->lineno = line;
		s->mallcnt = 1;
		s->avsiz = size;
		savesym(s);
	}
	else
	{
		/* found an already defined symbol - store some averages */

		s->avsiz = ((s->avsiz * s->mallcnt) + size) / (s->mallcnt + 1);
		(s->mallcnt)++;
	}

	p = lookupptr(ptr);
	if(p != (struct ptr *)0 && p->dsk.siz != 0)
	{
		char buffer[120];
		sprintf(buffer,
				"pointer (%d bytes) re-allocated without being freed (%s %d)",
				(int)size, file, line);

		struct sym *x;
        if((x = lookupsymbynum(p->dsk.smap)) != (struct sym *)0)
		{
			char *ptr = buffer + strlen(buffer);
			sprintf(ptr, "\nlast allocated ");
			ptr = buffer + strlen(buffer);
			fsym(ptr, x);
		}
		CUI_fatal(buffer);
	}

	/* heavy sigh - no entry for this pointer - make one */

	if(p == (struct ptr *)0)
	{
		p = (struct ptr *)malloc(sizeof(struct ptr));
		if(p == (struct ptr *)0)
		{
			CUI_fatal("memcheck: cannot expand pointer table");
		}

		/* link it in */

		p->next = map.phash[(unsigned long)ptr % MEM_HASHSIZE];
		map.phash[(unsigned long)ptr % MEM_HASHSIZE] = p;
	}

	/* if we get to here (hazoo! hazaa!) both 's' and 'p' are OK */

	p->ptr = ptr;
	p->dsk.siz = size;
	p->dsk.smap = s->mapno;
	p->map = map.pmap++;

	/* store the size */

	map.ninuse += size;

	saveptr(p);
}


/*
 *	record an allocation
 */

void recordMalloc(void *ptr, size_t size, char *file, int line)
{
	if(!(rec_state & REC_INITTED))
		initmap();

	// if we have hints, use them instead of passed file, line

	if(MEMfile)
	{
		file = hintFile(MEMfile);
		line = MEMline;
	}

	if((rec_state & REC_ON) && !(rec_state & REC_ERR))
		storeptr(ptr, (int)size, file, line);

	map.avgsiz = ((map.avgsiz * map.nalloc) + size) / (map.nalloc + 1);
	map.nalloc++;

	// clear hints

	MEMfile = NULL;
}


/*
 *	 mark a pointer as now being free; will not return if we are freeing
 *	 something that's already been freed, or that wasn't allocated
 *	 (we treat these as fatal errors)
 */

void recordFree(void *ptr, char *file, int line)
{
	if(!(rec_state & REC_INITTED))
		initmap();

	// if we have hints, use them instead of passed file, line

	if(MEMfile)
	{
		file = hintFile(MEMfile);
		line = MEMline;
	}

    if((rec_state & REC_ON) && !(rec_state & REC_ERR))
	{
		struct ptr *p = lookupptr(ptr);
		if(p == (struct ptr *)0)
		{
			CUI_fatal("pointer freed that was never allocated: %s %d",
					 file, line);
		}

		if(p != (struct ptr *)0 && p->dsk.siz == 0)
		{
			char buffer[120];

			sprintf(buffer, "pointer re-freed when already free (%s %d)",
							file, line);

            struct sym *x;
			if((x = lookupsymbynum(p->dsk.smap)) != (struct sym *)0)
			{
				char *ptr = buffer + strlen(buffer);
				sprintf(ptr, "\nlast allocated:");
				ptr = buffer + strlen(buffer);
				fsym(ptr, x);
			}
			CUI_fatal(buffer);
		}

		/* get some free */

		map.ninuse -= p->dsk.siz;

		/* write in the map that it is free */

		p->dsk.siz = 0;
		saveptr(p);
		map.nfree++;
    }

	// clear hints

	MEMfile = NULL;
}


/*
 *	 dump everything we know about nothing in particular
 */

void memWriteStats(void)
{
	register struct sym	*s;
	register int		x;

	if(map.fp == (FILE *)0)
		return;

	fseek(map.fp,0L,0);

	/* dump our life's story */

	fprintf(map.fp, "#total allocations:%ld\n", map.nalloc);
	fprintf(map.fp, "#total re-allocations:%ld\n", map.nrlloc);
	fprintf(map.fp, "#total frees:%ld\n", map.nfree);

	if(map.nbfree != 0L)
		fprintf(map.fp, "#bad/dup frees:%ld\n",map.nbfree);

	fprintf(map.fp, "#total allocated never freed:%ld\n",map.ninuse);

	fprintf(map.fp, "#average size of allocations:%.1f\n",map.avgsiz);

	/* note if we detected an internal error */

	if(rec_state & REC_ERR)
		fprintf(map.fp, "#(figures maye be inaccurate due to error)\n");

	/* note if the system was on all the time ? */

	if(!(rec_state & REC_ON) || (rec_state & REC_ONOFF))
		fprintf(map.fp, "#(figures may be inaccurate as recording was off)\n");

	/* write the legend */

	fprintf(map.fp, "#map#\tcalls\tave\tline#\tfile\n");

	for(x = 0; x < MEM_HASHSIZE; x++)
	{
		s = map.shash[x];
		while(s != (struct sym *)0)
		{
			savesym(s);
			s = s->next;
		}
	}

	fflush(map.fp);
}


#endif // MEMDB


