/*
 * Root Properties driver test program.
 *
 */

#pragma ident "@(#)rootptest.c 1.1 93/03/26 Sun Microsystems Inc."

#include <stdio.h>
#include "rootprop_io.h"
#include <errno.h>
#include <sys/fcntl.h>

main( int argc, char ** argv)
{

#define PROP "boot-path"
#define PROPLEN 10

rootprop_arg_t rootp = { PROPLEN, PROP, 0, NULL };
char propname[64]="machine_type";

int fd = open("/dev/rootprop", O_RDONLY );
	printf("Enter a property to search for(<CR> Exits): ");
	gets(propname);
	while(propname[0] ) {
		rootp.pname = propname;
		rootp.pnamelen = strlen(propname)+1;
		printf("Looking for prop length..");
		if (ioctl(fd, ROOTPROP_LEN, &rootp)) {
			printf("FAILED (%d).\n", errno);
		}
		else {
			printf("It is %d\n", rootp.pbuflen);
			printf("Looging for prop value..");
			rootp.pbuf = (char *)malloc(rootp.pbuflen);
			rootp.pname = propname;
			rootp.pnamelen = strlen(propname)+1;
			if(ioctl(fd, ROOTPROP_PROP, &rootp)) {
				printf("FAILED (%d).\n", errno);
			}
			else
				printf("It is \"%s\"\n",rootp.pbuf);
		}
		printf("Enter a property to search for(<CR> Exits): ");
		gets(propname);
	}
	close(fd);
}
