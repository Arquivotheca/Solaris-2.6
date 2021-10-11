#include <stdio.h>
#include <unistd.h>
#include <sys/swap.h>

main()
{
	/* print physical memory in MB */
	/*
	(void) printf("%d\n",
	    (sysconf(_SC_PHYS_PAGES) >> 10) * (sysconf(_SC_PAGESIZE) >> 10));
	*/

        struct anoninfo ai;
	int available;
 
	if (swapctl(SC_AINFO, &ai) == -1)
		(void) printf("0\n");

	available = ai.ani_max - ai.ani_resv;

	(void) printf("%d\n", available * (sysconf(_SC_PAGESIZE) >> 10));
}
