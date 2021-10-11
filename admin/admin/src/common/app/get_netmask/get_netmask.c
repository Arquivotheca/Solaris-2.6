/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident   "@(#)get_netmask.c 1.2     96/10/10 SMI"

/*
 *	             N E T M A S K . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "NETMASK" 
 * facility to get the netmask of the install boot server.
 *
 */

#include <stdio.h>
#include <errno.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <ctype.h>
#include <netdb.h>

#define bzero(s, len) memset(s, 0, len)
#define bcopy(src, dst, len) memcpy(dst, src, len)

#define	MAXPACKET	(65536-60-8)	/* max packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

char	*malloc();

u_char	*packet;
int	packlen;
int	i;
extern	int errno;

int s;			/* Socket file descriptor */
struct hostent *hp;	/* Pointer to host info */

struct sockaddr whereto;	/* Who to send request to */
int datalen = 64-8;		/* How much data */

char *hostname;
char hnamebuf[MAXHOSTNAMELEN];

static u_char outpack[MAXPACKET];

int ident;
int bufspace = 48*1024;
char *inet_ntoa(),*strcpy(),*strncpy();
u_long inet_addr();
int	icmp_send_type = ICMP_MASKREQ;
int	icmp_rcv_type = ICMP_MASKREPLY;

/*
 * 			M A I N
 */
main(argc, argv)
char *argv[];
{
	struct sockaddr_in from;
	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	int c, k, hostind = 0;
	struct protoent *proto;
	static u_char *datap = &outpack[8+sizeof(struct timeval)];
	int fromlen = sizeof (from);
	int cc;

	if (argc <= 1) {
		fprintf(stderr,"ERROR: hostname not specified\n");
		exit(1);
	} else hostind = argc-1;

	bzero((char *)&whereto, sizeof(struct sockaddr) );
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(argv[hostind]);
	if(to->sin_addr.s_addr != (unsigned)-1) {
		strcpy(hnamebuf, argv[hostind]);
		hostname = hnamebuf;
	} else {
		hp = gethostbyname(argv[hostind]);
		if (hp) {
			to->sin_family = hp->h_addrtype;
			bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
			strncpy( hnamebuf, hp->h_name, sizeof(hnamebuf)-1 );
			hostname = hnamebuf;
		} else {
			printf("%s: unknown host %s\n", argv[0], argv[hostind]);
			exit(1);
		}
	}

	packlen = datalen + 60 + 76;	/* MAXIP + MAXICMP */
	if( (packet = (u_char *)malloc((unsigned)packlen)) == NULL ) {
		fprintf( stderr, "netmask: malloc failed\n" );
		exit(1);
	}

	ident = getpid() & 0xFFFF;

	if ((proto = getprotobyname("icmp")) == NULL) {
		fprintf(stderr, "icmp: unknown protocol\n");
		exit(10);
	}
	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		perror("netmask: socket");
		exit(5);
	}

	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&bufspace,
			 sizeof(bufspace));

	icmp_request();

	if ( (cc=recvfrom(s, (char *)packet, packlen, 0, (struct sockaddr *)&from, &fromlen)) >= 0) {
		pr_pack( (char *)packet, cc, &from );
	}
}

/*
 * Compose and transmit an ICMP NETMASK REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
icmp_request()
{
	register struct icmp *icp = (struct icmp *) outpack;
	int i, cc;
	register struct timeval *tp = (struct timeval *) &outpack[8];

	icp->icmp_type = icmp_send_type;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = 0;
	icp->icmp_id = ident;		/* ID */

	cc = datalen+8;			/* skips ICMP portion */

	/* Compute ICMP checksum here */
	icp->icmp_cksum = in_cksum( (u_short *)icp, cc );

	/* cc = sendto(s, msg, len, flags, to, tolen) */
	i = sendto( s, (char *)outpack, cc, 0, &whereto, sizeof(struct sockaddr) );

	if( i < 0 || i != cc )  {
		if( i<0 )  perror("sendto");
		printf("netmask: wrote %s %d chars, ret=%d\n",
			hostname, cc, i );
		fflush(stdout);
	}
}

/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
pr_pack( buf, cc, from )
char *buf;
int cc;
struct sockaddr_in *from;
{
	struct ip *ip;
	register struct icmp *icp;
	register int i, j;
	register u_char *cp,*dp;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];
	int hlen, triptime, dupflag;

	/* Check the IP header */
	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if( cc < hlen + ICMP_MINLEN ) {
		fflush(stdout);
		return;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if( icp->icmp_type == icmp_rcv_type ) {
		if( icp->icmp_id != ident )
			return;			/* 'Twas not our ECHO */

		if(icmp_send_type == ICMP_MASKREQ) 
		    printf("%x\n", icp->icmp_mask);
	}

	fflush(stdout);
}

/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

