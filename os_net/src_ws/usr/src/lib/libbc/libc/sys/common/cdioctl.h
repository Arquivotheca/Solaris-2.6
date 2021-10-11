/*
 * @(#)srdef.h 1.11 89/12/18 Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *
 * Defines for SCSI direct access devices modified for CDROM, based on sddef.h
 *
 */

/*
 * CDROM io controls type definitions
 */
struct cdrom_msf {
	unsigned char	cdmsf_min0;	/* starting minute */
	unsigned char	cdmsf_sec0;	/* starting second */
	unsigned char	cdmsf_frame0;	/* starting frame  */
	unsigned char	cdmsf_min1;	/* ending minute   */
	unsigned char	cdmsf_sec1;	/* ending second   */
	unsigned char	cdmsf_frame1;	/* ending frame	   */
};

struct cdrom_ti {
	unsigned char	cdti_trk0;	/* starting track */
	unsigned char	cdti_ind0;	/* starting index */
	unsigned char	cdti_trk1;	/* ending track */
	unsigned char	cdti_ind1;	/* ending index */
};

struct cdrom_tochdr {
	unsigned char	cdth_trk0;	/* starting track */
	unsigned char	cdth_trk1;	/* ending track */
};

struct cdrom_tocentry {
	unsigned char	cdte_track;
	unsigned char	cdte_adr	:4;
	unsigned char	cdte_ctrl	:4;
	unsigned char	cdte_format;
	union {
		struct {
			unsigned char	minute;
			unsigned char	second;
			unsigned char	frame;
		} msf;
		int	lba;
	} cdte_addr;
	unsigned char	cdte_datamode;
};

struct cdrom_subchnl {
	unsigned char	cdsc_format;
	unsigned char	cdsc_audiostatus;
	unsigned char	cdsc_adr:	4;
	unsigned char	cdsc_ctrl:	4;
	unsigned char	cdsc_trk;
	unsigned char	cdsc_ind;
	union {
		struct {
			unsigned char	minute;
			unsigned char	second;
			unsigned char	frame;
		} msf;
		int	lba;
	} cdsc_absaddr;
	union {
		struct {
			unsigned char	minute;
			unsigned char	second;
			unsigned char	frame;
		} msf;
		int	lba;
	} cdsc_reladdr;
};

/*
 * definition of audio volume control structure
 */
struct cdrom_volctrl {
	unsigned char	channel0;
	unsigned char	channel1;
	unsigned char	channel2;
	unsigned char	channel3;
};

struct cdrom_read {
	int	cdread_lba;
	caddr_t	cdread_bufaddr;
	int	cdread_buflen;
};

/*
 * CDROM io control commands
 */
#define	CDROMPAUSE	_IO(c, 10)	/* Pause Audio Operation */

#define	CDROMRESUME	_IO(c, 11)	/* Resume paused Audio Operation */

#define	CDROMPLAYMSF	_IOW(c, 12, struct cdrom_msf)	/* Play Audio MSF */
#define	CDROMPLAYTRKIND	_IOW(c, 13, struct cdrom_ti)	/*
							 * Play Audio
`							 * Track/index
							 */
#define	CDROMREADTOCHDR	\
		_IOR(c, 103, struct cdrom_tochdr)	/* Read TOC header */
#define	CDROMREADTOCENTRY	\
	_IOWR(c, 104, struct cdrom_tocentry)		/* Read a TOC entry */

#define	CDROMSTOP	_IO(c, 105)	/* Stop the cdrom drive */

#define	CDROMSTART	_IO(c, 106)	/* Start the cdrom drive */

#define	CDROMEJECT	_IO(c, 107)	/* Ejects the cdrom caddy */

#define	CDROMVOLCTRL	\
	_IOW(c, 14, struct cdrom_volctrl)	/* control output volume */

#define	CDROMSUBCHNL	\
	_IOWR(c, 108, struct cdrom_subchnl)	/* read the subchannel data */

#define	CDROMREADMODE2	\
	_IOW(c, 110, struct cdrom_read)		/* read CDROM mode 2 data */

#define	CDROMREADMODE1	\
	_IOW(c, 111, struct cdrom_read)		/* read CDROM mode 1 data */

