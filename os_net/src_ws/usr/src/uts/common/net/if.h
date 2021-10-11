/*
 * Copyright (c) 1992-1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef	_NET_IF_H
#define	_NET_IF_H

#pragma ident	"@(#)if.h	1.7	96/09/26 SMI"
/* if.h 1.26 90/05/29 SMI; from UCB 7.1 6/4/86		*/

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with three parameters:
 *	(*ifp->if_output)(ifp, m, dst)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of a internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */

/*
 * Structure defining a queue for a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 */
struct ifnet {
	char	*if_name;		/* name, e.g. ``en'' or ``lo'' */
	short	if_unit;		/* sub-unit for lower level driver */
	short	if_mtu;			/* maximum transmission unit */
	short	if_flags;		/* up/down, broadcast, etc. */
	short	if_timer;		/* time 'til if_watchdog called */
	u_short	if_promisc;		/* net # of requests for promisc mode */
	int	if_metric;		/* routing metric (external only) */
	struct	ifaddr *if_addrlist;	/* linked list of addresses per if */
	struct	ifqueue {
		struct	mbuf *ifq_head;
		struct	mbuf *ifq_tail;
		int	ifq_len;
		int	ifq_maxlen;
		int	ifq_drops;
	} if_snd;			/* output queue */
/* procedure handles */
	int	(*if_init)();		/* init routine */
	int	(*if_output)();		/* output routine */
	int	(*if_ioctl)();		/* ioctl routine */
	int	(*if_reset)();		/* bus reset routine */
	int	(*if_watchdog)();	/* timer routine */
/* generic interface statistics */
	int	if_ipackets;		/* packets received on interface */
	int	if_ierrors;		/* input errors on interface */
	int	if_opackets;		/* packets sent on interface */
	int	if_oerrors;		/* output errors on interface */
	int	if_collisions;		/* collisions on csma interfaces */
/* end statistics */
	struct	ifnet *if_next;
	struct	ifnet *if_upper;	/* next layer up */
	struct	ifnet *if_lower;	/* next layer down */
	int	(*if_input)();		/* input routine */
	int	(*if_ctlin)();		/* control input routine */
	int	(*if_ctlout)();		/* control output routine */
	struct map *if_memmap;		/* rmap for interface specific memory */
};

#define	IFF_UP		0x1		/* interface is up */
#define	IFF_BROADCAST	0x2		/* broadcast address valid */
#define	IFF_DEBUG	0x4		/* turn on debugging */
#define	IFF_LOOPBACK	0x8		/* is a loopback net */
#define	IFF_POINTOPOINT	0x10		/* interface is point-to-point link */
#define	IFF_NOTRAILERS	0x20		/* avoid use of trailers */
#define	IFF_RUNNING	0x40		/* resources allocated */
#define	IFF_NOARP	0x80		/* no address resolution protocol */
#define	IFF_PROMISC	0x100		/* receive all packets */
#define	IFF_ALLMULTI	0x200		/* receive all multicast packets */
#define	IFF_INTELLIGENT	0x400		/* protocol code on board */
#define	IFF_MULTICAST	0x800		/* supports multicast */
#define	IFF_MULTI_BCAST	0x1000		/* multicast using broadcast address */
#define	IFF_UNNUMBERED	0x2000		/* non-unique address */
#define	IFF_PRIVATE	0x8000		/* do not advertise */

/*
 * The IFF_MULTICAST flag indicates that the network can support the
 * transmission and reception of higher-level (e.g., IP) multicast packets.
 * It is independent of hardware support for multicasting; for example,
 * point-to-point links or pure broadcast networks may well support
 * higher-level multicasts.
 */

/* flags set internally only: */
#define	IFF_CANTCHANGE \
	(IFF_BROADCAST | IFF_POINTOPOINT | IFF_RUNNING | IFF_PROMISC | \
	IFF_MULTICAST | IFF_MULTI_BCAST | IFF_UNNUMBERED)

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splimp().
 */
#define	IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define	IF_DROP(ifq)		((ifq)->ifq_drops++)
#define	IF_ENQUEUE(ifq, m) { \
	(m)->m_act = 0; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_head = m; \
	else \
		(ifq)->ifq_tail->m_act = m; \
	(ifq)->ifq_tail = m; \
	(ifq)->ifq_len++; \
}
#define	IF_PREPEND(ifq, m) { \
	(m)->m_act = (ifq)->ifq_head; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_tail = (m); \
	(ifq)->ifq_head = (m); \
	(ifq)->ifq_len++; \
}
/*
 * Packets destined for level-1 protocol input routines
 * have a pointer to the receiving interface prepended to the data.
 * IF_DEQUEUEIF extracts and returns this pointer when dequeueing the packet.
 * IF_ADJ should be used otherwise to adjust for its presence.
 */
#define	IF_ADJ(m) { \
	(m)->m_off += sizeof (struct ifnet *); \
	(m)->m_len -= sizeof (struct ifnet *); \
	if ((m)->m_len == 0) { \
		struct mbuf *n; \
		MFREE((m), n); \
		(m) = n; \
	} \
}
#define	IF_DEQUEUEIF(ifq, m, ifp) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_act) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_act = 0; \
		(ifq)->ifq_len--; \
		(ifp) = *(mtod((m), struct ifnet **)); \
		IF_ADJ(m); \
	} \
}
#define	IF_DEQUEUE(ifq, m) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_act) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_act = 0; \
		(ifq)->ifq_len--; \
	} \
}

#define	IFQ_MAXLEN	50
#define	IFNET_SLOWHZ	1		/* granularity is 1 second */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
	struct	sockaddr ifa_addr;	/* address of interface */
	union {
		struct	sockaddr ifu_broadaddr;
		struct	sockaddr ifu_dstaddr;
	} ifa_ifu;
#define	ifa_broadaddr	ifa_ifu.ifu_broadaddr	/* broadcast address */
#define	ifa_dstaddr	ifa_ifu.ifu_dstaddr	/* other end of p-to-p link */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	struct	ifaddr *ifa_next;	/* next address for interface */
};

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */
struct	ifreq {
#define	IFNAMSIZ	16
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		char	ifru_oname[IFNAMSIZ];	/* other if name */
		struct	sockaddr ifru_broadaddr;
		int	ifru_index;		/* interface index */
		short	ifru_flags;
		int	ifru_metric;
		char	ifru_data[1];		/* interface dependent data */
		char	ifru_enaddr[6];
		int	if_muxid[2];		/* mux id's for arp and ip */

		/* Struct for FDDI ioctl's */
		struct ifr_dnld_reqs {
			caddr_t	v_addr;
			caddr_t	m_addr;
			caddr_t	ex_addr;
			u_int	size;
		} ifru_dnld_req;

		/* Struct for FDDI stats */
		struct ifr_fddi_stats {
			u_int	stat_size;
			caddr_t	fddi_stats;
		} ifru_fddi_stat;

		struct ifr_netmapents {
			u_int	map_ent_size,	/* size of netmap structure */
				entry_number;	/* index into netmap list */
			caddr_t	fddi_map_ent;	/* pointer to user structure */
		} ifru_netmapent;

		/* Field for generic ioctl for fddi */

		struct ifr_fddi_gen_struct {
			int	ifru_fddi_gioctl; /* field for gen ioctl */
			caddr_t ifru_fddi_gaddr;  /* Generic ptr to a field */
		} ifru_fddi_gstruct;

	} ifr_ifru;

#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_oname	ifr_ifru.ifru_oname	/* other if name */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
#define	ifr_enaddr	ifr_ifru.ifru_enaddr	/* ethernet address */
#define	ifr_index 	ifr_ifru.ifru_index	/* interface index  */

/* FDDI specific */
#define	ifr_dnld_req	ifr_ifru.ifru_dnld_req
#define	ifr_fddi_stat	ifr_ifru.ifru_fddi_stat
#define	ifr_fddi_netmap	ifr_ifru.ifru_netmapent	/* FDDI network map entries */
#define	ifr_fddi_gstruct ifr_ifru.ifru_fddi_gstruct

#define	ifr_ip_muxid	ifr_ifru.if_muxid[0]
#define	ifr_arp_muxid	ifr_ifru.if_muxid[1]
};

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */
struct	ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

typedef struct if_data {
				/* generic interface information */
	u_char	ifi_type;	/* ethernet, tokenring, etc */
	u_char	ifi_addrlen;	/* media address length */
	u_char	ifi_hdrlen;	/* media header length */
	u_long	ifi_mtu;	/* maximum transmission unit */
	u_long	ifi_metric;	/* routing metric (external only) */
	u_long	ifi_baudrate;	/* linespeed */
				/* volatile statistics */
	u_long	ifi_ipackets;	/* packets received on interface */
	u_long	ifi_ierrors;	/* input errors on interface */
	u_long	ifi_opackets;	/* packets sent on interface */
	u_long	ifi_oerrors;	/* output errors on interface */
	u_long	ifi_collisions;	/* collisions on csma interfaces */
	u_long	ifi_ibytes;	/* total number of octets received */
	u_long	ifi_obytes;	/* total number of octets sent */
	u_long	ifi_imcasts;	/* packets received via multicast */
	u_long	ifi_omcasts;	/* packets sent via multicast */
	u_long	ifi_iqdrops;	/* dropped on input, this interface */
	u_long	ifi_noproto;	/* destined for unsupported protocol */
	struct	timeval ifi_lastchange; /* last updated */
} if_data_t;


/*
 * Message format for use in obtaining information about interfaces
 * from the routing socket
 */
typedef struct if_msghdr {
	u_short	ifm_msglen;	/* to skip over non-understood messages */
	u_char	ifm_version;	/* future binary compatability */
	u_char	ifm_type;	/* message type */
	int	ifm_addrs;	/* like rtm_addrs */
	int	ifm_flags;	/* value of if_flags */
	u_short	ifm_index;	/* index for associated ifp */
	struct	if_data ifm_data; /* statistics and other data about if */
} if_msghdr_t;

/*
 * Message format for use in obtaining information about interface addresses
 * from the routing socket
 */
typedef struct ifa_msghdr {
	u_short	ifam_msglen;	/* to skip over non-understood messages */
	u_char	ifam_version;	/* future binary compatability */
	u_char	ifam_type;	/* message type */
	int	ifam_addrs;	/* like rtm_addrs */
	int	ifam_flags;	/* route flags */
	u_short	ifam_index;	/* index for associated ifp */
	int	ifam_metric;	/* value of ipif_metric */
} ifa_msghdr_t;

#ifdef	__cplusplus
}
#endif

#endif	/* _NET_IF_H */
