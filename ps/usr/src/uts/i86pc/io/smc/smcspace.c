
/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All rights reserved
 */

#ident	"@(#)smcspace.c	1.4	93/12/15 SMI"
#include "sys/types.h"
#include "sys/stream.h"
#include "sys/ddi.h"
#include "sys/ethernet.h"
#include "sys/sunddi.h"
#include "sys/smc.h"

#define	MAXMULTI	16	/* Number of multicast addrs/board */
#define	WDMINORS	16
#define	WDBOARDS	4

int		wd_minors = WDMINORS;
struct wddev	wddevs[WDMINORS * WDBOARDS];
struct wdstat	wdstats[WDBOARDS];
int		wd_boardcnt = WDBOARDS;
int		wd_address_list[WDBOARDS];
int		wd_boards_found;
kmutex_t	wd_lock;	/* lock for this module */

struct wdmaddr	wdmultiaddrs[WDBOARDS * MAXMULTI];

int		wd_multisize = MAXMULTI;	/* # of multicast addrs/board */

#if defined(WDDEBUG)
int		wd_debug = WDDEBUG;	/* can be enabled dynamically */
#endif
int		wd_inetstats = 1;	/* keep inet interface stats */

struct wdparam	wdparams[WDBOARDS];


