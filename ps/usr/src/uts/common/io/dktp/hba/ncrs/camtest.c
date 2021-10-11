/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)camtest.c	1.2	94/06/30 SMI"

/*
 * This routine comes from Annex A of the CAM XPT/SIM, Rev 3.0, 
 * document dated April 27, 1992, page 55.
 */

typedef	unsigned int UINT;
typedef unsigned long ULNG;
/*
 * Convert from logical block count to Synlinder, Sector and Head (int 13)
 */


int
CAMsetsize(	ULNG	 capacity,
		UINT	*cyls,
		UINT	*hds,
		UINT	*secs )
{
	UINT	rv = 0;
	ULNG	heads;
	ULNG	sectors;
	ULNG	cylinders;
	ULNG	temp;

    cylinders = 1024L;	/* Set number of sylinders to max value */
    sectors = 62L;		/* Max out number of sectors per track */

    temp = cylinders * sectors;		/* Compute divisor for heads */
    heads = capacity / temp;		/* Compute value for number of heads */
    if (capacity % temp) {		/*  If no remainder, done! */
	heads++;			/* Else, increment number of heads */
	temp = cylinders * heads;	/* Compute divisor for sectors */
	sectors = capacity / temp;	/* Compute for sectors per trk */
	if (capacity % temp) {		/* If no remainder, done! */
	    sectors++;			/* Else, increment number of sectors */
	    temp = heads * sectors;	/* Compute divisor for cylinders */
	    cylinders = capacity / temp;/* Compute number of cylinders */
	}
    }
    if (cylinders == 0) rv = 1;		/* Give error if 0 cylinders */

    *cyls = (UINT)cylinders;
    *secs = (UINT)sectors;
    *hds  = (UINT)heads;
    return (rv);
}

#include <stdio.h>
#include <stdlib.h>

main(	int	argc,
	char	**argv )
{

	long	capacity;
	UINT	cyls;
	UINT	hds;
	UINT	secs;
	int	rc;

	capacity = strtol(*(argv + 1), NULL, 0);


	rc = CAMsetsize(capacity, &cyls, &hds, &secs);


	printf("rc=%d capacity=%ld cyls=%d hds=%d secs=%d\n"
		, rc, capacity, cyls, hds, secs);

	return (0);
}
