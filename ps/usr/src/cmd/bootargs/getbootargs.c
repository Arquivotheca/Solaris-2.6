/* here is the x86 version */

#include <stdio.h>
#include <fcntl.h>
#include "rootprop_io.h"

#define BOOTA "boot-args"

main( int argc, char **argv)
{
int fd;
char errmsg[80];
rootprop_arg_t args;
char filename[80];

	if (argc == 2)
		strcpy(filename, argv[1]);
	else
		strcpy(filename, "/dev/rootprop");

	if ((fd = open(filename, O_RDONLY)) == -1) {
		sprintf(errmsg,"%s: Bad open", argv[0] );
		perror(errmsg);
		exit(0);
	}

	args.pname = BOOTA;
	args.pnamelen = strlen(BOOTA)+1;

	if (ioctl(fd, ROOTPROP_LEN, &args) == -1) {
		sprintf(errmsg, "%s: bad ioctl", argv[0] );
		perror(errmsg);
		exit(0);
	}

	args.pbuf = (char *)malloc(args.pbuflen);
	args.pname = BOOTA;
	args.pnamelen = strlen(BOOTA)+1;

	if (ioctl(fd, ROOTPROP_PROP, &args) == -1) {
		sprintf(errmsg, "%s: bad ioctl", argv[0] );
		perror(errmsg);
		exit(0);
	}

	printf("%s\n", args.pbuf);

	close(fd);

	exit(1);
}
