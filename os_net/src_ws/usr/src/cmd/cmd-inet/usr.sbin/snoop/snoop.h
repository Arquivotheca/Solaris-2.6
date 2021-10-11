/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef	_SNOOP_H
#define	_SNOOP_H

#pragma ident	"@(#)snoop.h	1.13	96/06/07 SMI"	/* SunOS	*/

#include <rpc/types.h>
#include <sys/pfmod.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flags to control packet info display
 */
#define	F_NOW		0x00000001	/* display in realtime */
#define	F_SUM		0x00000002	/* display summary line */
#define	F_ALLSUM	0x00000004	/* display all summary lines */
#define	F_DTAIL		0x00000008	/* display detail lines */
#define	F_TIME		0x00000010	/* display time */
#define	F_ATIME		0x00000020	/* display absolute time */
#define	F_RTIME		0x00000040	/* display relative time */
#define	F_DROPS		0x00000080	/* display drops */
#define	F_LEN		0x00000100	/* display pkt length */
#define	F_NUM		0x00000200	/* display pkt number */
#define	F_WHO		0x00000400	/* display src/dst */

#define	MAXLINE		(160)		/* max len of detail line */

/*
 * The RPC XID cache structure.
 * When analyzing RPC protocols we
 * have to cache the xid of the RPC
 * request together with the program
 * number, proc, version etc since this
 * information is missing in the reply
 * packet.  Using the xid in the reply
 * we can lookup this previously stashed
 * information in the cache.
 *
 * For RPCSEC_GSS flavor, some special processing is
 * needed for the argument interpretation based on its
 * control procedure and service type.  This information
 * is stored in the cache table during interpretation of
 * the rpc header and will be referenced later when the rpc
 * argument is interpreted.
 */
#define	XID_CACHE_SIZE 256
struct cache_struct {
	int xid_num;	/* RPC transaction id */
	int xid_frame;	/* Packet number */
	int xid_prog;	/* RPC program number */
	int xid_vers;	/* RPC version number */
	int xid_proc;	/* RPC procedure number */
	unsigned int xid_gss_proc; /* control procedure */
	int xid_gss_service; /* none, integ, priv */
} xid_cache[XID_CACHE_SIZE];

#if defined(__STDC__)
extern char *get_sum_line(void);
extern char *get_detail_line(int, int);
extern struct timeval prev_time;
extern char *getflag(int, int, char *, char *);
extern void show_header(char *, char *, int);
extern void xdr_init(char *, int);
extern char *get_line(int, int);
extern char getxdr_char(void);
extern char showxdr_char(char *);
extern u_char getxdr_u_char(void);
extern u_char showxdr_u_char(char *);
extern short getxdr_short(void);
extern short showxdr_short(char *);
extern u_short getxdr_u_short(void);
extern u_short showxdr_u_short(char *);
extern long getxdr_long(void);
extern long showxdr_long(char *);
extern u_long getxdr_u_long(void);
extern u_long showxdr_u_long(char *);
extern longlong_t getxdr_longlong(void);
extern longlong_t showxdr_longlong(char *);
extern u_longlong_t getxdr_u_longlong(void);
extern u_longlong_t showxdr_u_longlong(char *);
extern char *getxdr_opaque(char *, int);
extern char *getxdr_string(char *, int);
extern char *showxdr_string(int, char *);
extern char *getxdr_bytes(u_int *);
extern void xdr_skip(int);
extern int getxdr_pos(void);
extern void show_space(void);
extern void show_trailer(void);
extern char *getxdr_date(void);
extern char *showxdr_date(char *);
extern char *showxdr_date_ns(char *);
extern char *getxdr_hex(int);
extern char *showxdr_hex(int, char *);
extern bool_t getxdr_bool(void);
extern bool_t showxdr_bool(char *);
extern char *concat_args(char **, int);
extern int pf_compile(char *, int);
extern void compile(char *, int);
extern void load_names(char *);
extern void cap_open_read(char *);
extern void cap_open_write(char *);
extern void cap_read(int, int, int, void (*)(), int);
extern void initdevice(char *, u_long, u_long, struct timeval *,
		struct packetfilt *, int);
extern void net_read(int, int, void (*)(), int);
extern void click(int);
extern void show_pktinfo(int, int, char *, char *, struct timeval *,
		struct timeval *, int, int);
extern void show_line(char *);
extern char *getxdr_time(void);
extern char *showxdr_time(char *);
#else
extern char *get_sum_line();
extern char *get_detail_line();
extern struct timeval prev_time;
extern char *getflag();
extern void show_header();
extern void xdr_init();
extern char *get_line();
extern char getxdr_char();
extern char showxdr_char();
extern u_char getxdr_u_char();
extern u_char showxdr_u_char();
extern short getxdr_short();
extern short showxdr_short();
extern u_short getxdr_u_short();
extern u_short showxdr_u_short();
extern long getxdr_long();
extern long showxdr_long();
extern u_long getxdr_u_long();
extern u_long showxdr_u_long();
extern longlong_t getxdr_longlong();
extern longlong_t showxdr_longlong();
extern u_longlong_t getxdr_u_longlong();
extern u_longlong_t showxdr_u_longlong();
extern char *getxdr_opaque();
extern char *getxdr_string();
extern char *showxdr_string();
extern char *getxdr_bytes();
extern void xdr_skip();
extern int getxdr_pos();
extern void show_space();
extern void show_trailer();
extern char *getxdr_date();
extern char *showxdr_date();
extern char *showxdr_date_ns();
extern char *getxdr_hex();
extern char *showxdr_hex();
extern bool_t getxdr_bool();
extern bool_t showxdr_bool();
extern char *concat_args();
extern int pf_compile();
extern void compile();
extern void load_names();
extern void cap_open();
extern void cap_read();
extern void initdevice();
extern void net_read();
extern void click();
extern void show_pktinfo();
extern void show_line();
extern char *getxdr_time();
extern char *showxdr_time();
#endif

/*
 * Describes characteristics of the Media Access Layer.
 * The mac_type is one of the supported DLPI media
 * types (see <sys/dlpi.h>).
 * The mtu_size is the size of the largest frame.
 * The header length is returned by a function to
 * allow for variable header size - for ethernet it's
 * just a constant 14 octets.
 * The interpreter is the function that "knows" how
 * to interpret the frame.
 */
typedef struct interface {
	u_int	mac_type;
	u_int	mtu_size;
	u_int	(*header_len)(char *);
	u_int 	(*interpreter)(int, char *, int, int);
	u_int	mac_hdr_fixed_size;
} interface_t;

#define	IF_HDR_FIXED	0
#define	IF_HDR_VAR	1

extern interface_t INTERFACES[], *interface;

#ifdef __cplusplus
}
#endif

#endif	/* _SNOOP_H */
