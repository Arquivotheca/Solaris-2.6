#include <errno.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <nfs/nfs.h>
#include <sys/syscall.h>
#include <netinet/in.h>


#define GETFSIND	1	/* translate fs id to ftype index */
#define CLIENT		1	/* #defined in <pn.h> */
#define MS_RFFLAGS	(MS_CACHE)

/*
 * Flags bits passed to mount(2), from the SVR4 sys/mount.h header file.
 */
#define	MS_RDONLY       0x01    /* read only bit */
#define MS_DATA         0x04    /* 6-argument mount */
#define MS_NOSUID       0x10    /* Setuid programs disallowed */
#define MS_REMOUNT      0x20    /* Remount */
#define MS_NOTRUNC      0x40    /* Return ENAMETOOLONG for long filenames */

/*
 * structs netbuf, knetconfig, and nfsarg from SVR4
 */


struct netbuf {
        unsigned int maxlen;
        unsigned int len;
        char *buf;
};

struct knetconfig {
        unsigned long   knc_semantics;  /* token name */
        char            *knc_protofmly; /* protocol family */
        char            *knc_proto;     /* protocol */
        dev_t           knc_rdev;       /* device id */
        unsigned long   knc_unused[8];
};

struct nfsarg {
        struct netbuf           *addr;          /* file server address */
        struct netbuf           *syncaddr;      /* secure NFS time sync address
*/
        struct knetconfig       *knconf;        /* transport knetconfig struct */
        char                    *hostname;      /* server's hostname */
        char                    *netname;       /* server's netname */
        caddr_t                 fh;             /* File handle to be mounted */
        int                     flags;          /* flags */
        int                     wsize;          /* write size in bytes */
        int                     rsize;          /* read size in bytes */
        int                     timeo;          /* initial timeout in .1 secs */
        int                     retrans;        /* times to retry send */
        int                     acregmin;       /* attr cache file min secs */
        int                     acregmax;       /* attr cache file max secs */
        int                     acdirmin;       /* attr cache dir min secs */
        int                     acdirmax;       /* attr cache dir max secs */
};

int mount(type, dir, flags, data)
char	*type, *dir;
int	flags;
caddr_t data;
{
	int idx, nflags=0;
	int returnValue;
	char fstr[32];
	struct nfsarg narg;
	struct nfsarg *na=&narg;
	struct nfs_args *nfsa;

	if (strcmp(type, "4.2") == 0)
		strcpy(fstr, "ufs");
	else if (strcmp(type, "lo") == 0)
		strcpy(fstr, "lo");
	else if (strcmp(type, "nfs") == 0)
		strcpy(fstr, "nfs");

	if ((idx = sysfs(GETFSIND, fstr)) == -1)
		return(-1);
	
	nflags = MS_NOTRUNC;
	switch(flags) {
		case M_RDONLY: nflags |= MS_RDONLY;
		case M_NOSUID: nflags |= MS_NOSUID;
		case M_REMOUNT: nflags |= MS_REMOUNT;
	}

	if (strcmp(type, "4.2") == 0)
		return(_syscall(SYS_mount, data, dir, nflags, idx, 0, 0));
	else if (strcmp(type, "lo") == 0)
		return(_syscall(SYS_mount, data, dir, nflags, idx, 0, 0));
	else if (strcmp(type, "nfs") == 0) {
		nflags |= MS_DATA;
		nfsa = (struct nfs_args *)data;
		if ((na->addr = 
	      	    (struct netbuf *)malloc(sizeof(struct netbuf))) == NULL)
			return(-1);
		if ((na->syncaddr = 
		   (struct netbuf *)malloc(sizeof(struct netbuf))) == NULL) {
			free(na->addr);
			return(-1);
		}
		if ((na->knconf = 
		   (struct knetconfig *)malloc(sizeof(struct knetconfig))) == NULL) {
			free(na->addr);
			free(na->syncaddr);
			return(-1);
		}
		na->addr->maxlen = sizeof(struct sockaddr_in);
		na->addr->len = na->addr->maxlen;
		na->addr->buf = (char *)nfsa->addr;
		na->syncaddr->maxlen = na->addr->maxlen;
		na->syncaddr->len = na->syncaddr->maxlen;
		na->syncaddr->buf = (char *)nfsa->addr;
		strcpy(na->hostname, nfsa->hostname);
		strcpy(na->netname, nfsa->netname);
		na->fh = nfsa->fh;
		na->flags = nfsa->flags;
		na->wsize = nfsa->wsize;
		na->rsize = nfsa->rsize;
		na->timeo = nfsa->timeo;
		na->retrans = nfsa->retrans;
		na->acregmin = nfsa->acregmin;
		na->acregmax = nfsa->acregmax;
		na->acdirmin = nfsa->acdirmin;
		na->acdirmax = nfsa->acdirmax;
		returnValue = (_syscall(SYS_mount, data, dir, nflags, idx, na, 
						sizeof(struct nfsarg)));
		free(na->addr);
		free(na->syncaddr);
		free(na->knconf);
		return(returnValue);
	}		
}
