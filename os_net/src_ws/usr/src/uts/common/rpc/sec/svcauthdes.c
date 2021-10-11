/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)svcauthdes.c	1.23	96/04/25 SMI"	/* SVr4.0 1.10	*/

/*
 *  		PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *  		Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *  	(c) 1986, 1987, 1988, 1989  Sun Microsystems, Inc.
 *  	(c) 1983, 1984, 1985, 1986, 1987, 1988, 1989  AT&T.
 *		  All rights reserved.
 */

/*
 * svcauth_des.c, server-side des authentication
 *
 * We insure for the service the following:
 * (1) The timestamp microseconds do not exceed 1 million.
 * (2) The timestamp plus the window is less than the current time.
 * (3) The timestamp is not less than the one previously
 *	seen in the current session.
 *
 * It is up to the server to determine if the window size is
 * too small.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strsubr.h>
#include <sys/tiuser.h>
#include <sys/tihdr.h>
#include <sys/t_kuser.h>
#include <sys/t_lock.h>
#include <sys/debug.h>
#include <sys/time.h>

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_des.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <rpc/svc_auth.h>
#include <rpc/clnt.h>
#include <rpc/des_crypt.h>

#define	USEC_PER_SEC ((u_long) 1000000L)
#define	BEFORE(t1, t2) timercmp(t1, t2, < /* COMMENT HERE TO DEFEAT CSTYLE */)

/*
 * LRU cache of conversation keys and some other useful items.
 */
#define	DEF_AUTHDES_CACHESZ 128
static int authdes_cachesz = DEF_AUTHDES_CACHESZ;
struct cache_entry {
	des_block key;			/* conversation key */
	char *rname;			/* client's name */
	u_int window;			/* credential lifetime window */
	struct timeval laststamp;	/* detect replays of creds */
	char *localcred;		/* generic local credential */
	int index;			/* where are we in array? */
	struct cache_entry *prev;	/* prev entry on LRU linked list */
	struct cache_entry *next;	/* next entry on LRU linked list */
};
static struct cache_entry *authdes_cache /* [authdes_cachesz] */;
static struct cache_entry *cache_head;	/* cache (in LRU order) */
static struct cache_entry *cache_tail;	/* cache (in LRU order) */

static void	cache_init(void);
static short	cache_victim(void);
static void	cache_ref(short);
static short	cache_spot(des_block *, char *, struct timeval *);
static void	invalidate(char *);

/*
 * cache statistics
 */
static struct {
	u_long ncachehits;	/* times cache hit, and is not replay */
	u_long ncachereplays;	/* times cache hit, and is replay */
	u_long ncachemisses;	/* times cache missed */
} svcauthdes_stats;

/*
 * Service side authenticator for AUTH_DES
 */
enum auth_stat
_svcauth_des(register struct svc_req *rqst, register struct rpc_msg *msg)
{
	register long *ixdr;
	des_block cryptbuf[2];
	register struct authdes_cred *cred;
	struct authdes_verf verf;
	int status;
	register struct cache_entry *entry;
	short sid;
	des_block *sessionkey;
	des_block ivec;
	u_int window;
	struct timeval timestamp;
	u_long namelen;
	struct area {
		struct authdes_cred area_cred;
		char area_netname[MAXNETNAMELEN+1];
	} *area;

	mutex_enter(&authdes_lock);
	if (authdes_cache == NULL) {
		cache_init();
	}
	mutex_exit(&authdes_lock);

	/* LINTED pointer alignment */
	area = (struct area *)rqst->rq_clntcred;
	cred = (struct authdes_cred *)&area->area_cred;

	/*
	 * Get the credential
	 */
	/* LINTED pointer alignment */
	ixdr = (long *)msg->rm_call.cb_cred.oa_base;
	cred->adc_namekind = IXDR_GET_ENUM(ixdr, enum authdes_namekind);
	switch (cred->adc_namekind) {
	case ADN_FULLNAME:
		namelen = IXDR_GET_U_LONG(ixdr);
		if (namelen > MAXNETNAMELEN) {
			return (AUTH_BADCRED);
		}
		cred->adc_fullname.name = area->area_netname;
		bcopy((char *)ixdr, cred->adc_fullname.name,
			(u_int)namelen);
		cred->adc_fullname.name[namelen] = 0;
		ixdr += (RNDUP(namelen) / BYTES_PER_XDR_UNIT);
		cred->adc_fullname.key.key.high = (u_long)*ixdr++;
		cred->adc_fullname.key.key.low = (u_long)*ixdr++;
		cred->adc_fullname.window = (u_long)*ixdr++;
		break;
	case ADN_NICKNAME:
		cred->adc_nickname = (u_long)*ixdr++;
		break;
	default:
		return (AUTH_BADCRED);
	}

	/*
	 * Get the verifier
	 */
	/* LINTED pointer alignment */
	ixdr = (long *)msg->rm_call.cb_verf.oa_base;
	verf.adv_xtimestamp.key.high = (u_long)*ixdr++;
	verf.adv_xtimestamp.key.low =  (u_long)*ixdr++;
	verf.adv_int_u = (u_long)*ixdr++;

	mutex_enter(&authdes_lock);

	/*
	 * Get the conversation key
	 */
	if (cred->adc_namekind == ADN_FULLNAME) {
		sessionkey = &cred->adc_fullname.key;
		if (key_decryptsession(cred->adc_fullname.name, sessionkey) !=
		    RPC_SUCCESS) {
			RPCLOG(1,
			    "_svcauth_des: key_decryptsessionkey failed\n", 0);
			mutex_exit(&authdes_lock);
			return (AUTH_BADCRED); /* key not found */
		}
	} else { /* ADN_NICKNAME */
		sid = cred->adc_nickname;
		if (sid >= authdes_cachesz) {
			RPCLOG(1, "_svcauth_des: bad nickname %d\n", sid);
			mutex_exit(&authdes_lock);
			return (AUTH_BADCRED);	/* garbled credential */
		}
		sessionkey = &authdes_cache[sid].key;
	}


	/*
	 * Decrypt the timestamp
	 */
	cryptbuf[0] = verf.adv_xtimestamp;
	if (cred->adc_namekind == ADN_FULLNAME) {
		cryptbuf[1].key.high = cred->adc_fullname.window;
		cryptbuf[1].key.low = verf.adv_winverf;
		ivec.key.high = ivec.key.low = 0;
		status = cbc_crypt((char *)sessionkey, (char *)cryptbuf,
			2*sizeof (des_block), DES_DECRYPT,
			(char *)&ivec);
	} else {
		status = ecb_crypt((char *)sessionkey, (char *)cryptbuf,
			sizeof (des_block), DES_DECRYPT);
	}
	if (DES_FAILED(status)) {
		RPCLOG(1, "_svcauth_des: decryption failure\n", 0);
		mutex_exit(&authdes_lock);
		return (AUTH_FAILED);	/* system error */
	}

	/*
	 * XDR the decrypted timestamp
	 */
	ixdr = (long *)cryptbuf;
	timestamp.tv_sec = IXDR_GET_LONG(ixdr);
	timestamp.tv_usec = IXDR_GET_LONG(ixdr);

	/*
	 * Check for valid credentials and verifiers.
	 * They could be invalid because the key was flushed
	 * out of the cache, and so a new session should begin.
	 * Be sure and send AUTH_REJECTED{CRED, VERF} if this is the case.
	 */
	{
		struct timeval current;
		int nick;
		int winverf;

		if (cred->adc_namekind == ADN_FULLNAME) {
			window = IXDR_GET_U_LONG(ixdr);
			winverf = IXDR_GET_U_LONG(ixdr);
			if (winverf != window - 1) {
				RPCLOG(1,
				"_svcauth_des: window verifier mismatch %d\n",
				    winverf);
				mutex_exit(&authdes_lock);
				return (AUTH_BADCRED);	/* garbled credential */
			}
			sid = cache_spot(sessionkey, cred->adc_fullname.name,
			    &timestamp);
			if (sid < 0) {
				RPCLOG(1,
				"_svcauth_des: replayed credential sid %d\n",
				    sid);
				mutex_exit(&authdes_lock);
				return (AUTH_REJECTEDCRED);	/* replay */
			}
			nick = 0;
		} else {	/* ADN_NICKNAME */
			window = authdes_cache[sid].window;
			nick = 1;
		}

		if ((u_long)timestamp.tv_usec >= USEC_PER_SEC) {
			RPCLOG(1, "_svcauth_des: invalid usecs %d\n",
			    timestamp.tv_usec);
			/* cached out (bad key), or garbled verifier */
			mutex_exit(&authdes_lock);
			return (nick ? AUTH_REJECTEDVERF : AUTH_BADVERF);
		}
#ifdef _AUTH_CHECKTIME
		if (nick && BEFORE(&timestamp,
				    &authdes_cache[sid].laststamp)) {
			RPCLOG(1, "_svcauth_des: timestamp before last seen\n",
			    0);
			mutex_exit(&authdes_lock);
			return (AUTH_REJECTEDVERF);	/* replay */
		}
#endif
/*
		(void) gettimeofday(&current, (struct timezone *)NULL);
*/
		current.tv_sec = hrestime.tv_sec;
		current.tv_usec = hrestime.tv_nsec/1000;

		current.tv_sec -= window;	/* allow for expiration */
		if (!BEFORE(&current, &timestamp)) {
			RPCLOG(1, "_svcauth_des: timestamp expired\n", 0);
			/* replay, or garbled credential */
			mutex_exit(&authdes_lock);
			return (nick ? AUTH_REJECTEDVERF : AUTH_BADCRED);
		}
	}

	/*
	 * Set up the reply verifier
	 */
	verf.adv_nickname = sid;

	/*
	 * xdr the timestamp before encrypting
	 */
	ixdr = (long *)cryptbuf;
	IXDR_PUT_LONG(ixdr, timestamp.tv_sec - 1);
	IXDR_PUT_LONG(ixdr, timestamp.tv_usec);

	/*
	 * encrypt the timestamp
	 */
	status = ecb_crypt((char *)sessionkey, (char *)cryptbuf,
	    sizeof (des_block), DES_ENCRYPT);
	if (DES_FAILED(status)) {
		RPCLOG(1, "_svcauth_des: encryption failure\n", 0);
		mutex_exit(&authdes_lock);
		return (AUTH_FAILED);	/* system error */
	}
	verf.adv_xtimestamp = cryptbuf[0];


	/*
	 * Serialize the reply verifier, and update rqst
	 */
	/* LINTED pointer alignment */
	ixdr = (long *)msg->rm_call.cb_verf.oa_base;
	*ixdr++ = (long)verf.adv_xtimestamp.key.high;
	*ixdr++ = (long)verf.adv_xtimestamp.key.low;
	*ixdr++ = (long)verf.adv_int_u;

	rqst->rq_xprt->xp_verf.oa_flavor = AUTH_DES;
	rqst->rq_xprt->xp_verf.oa_base = msg->rm_call.cb_verf.oa_base;
	rqst->rq_xprt->xp_verf.oa_length =
		(char *)ixdr - msg->rm_call.cb_verf.oa_base;

	/*
	 * We succeeded, commit the data to the cache now and
	 * finish cooking the credential.
	 */
	entry = &authdes_cache[sid];
	entry->laststamp = timestamp;
	cache_ref(sid);
	if (cred->adc_namekind == ADN_FULLNAME) {
		cred->adc_fullname.window = window;
		cred->adc_nickname = sid;	/* save nickname */
		if (entry->rname != NULL) {
			mem_free(entry->rname, strlen(entry->rname) + 1);
		}
		entry->rname =
		    (char *)mem_alloc((u_int)strlen(cred->adc_fullname.name)
					    + 1);
		if (entry->rname != NULL) {
			(void) strcpy(entry->rname, cred->adc_fullname.name);
		} else {
			/* EMPTY */
			RPCLOG(1, "_svcauth_des: out of memory\n", 0);
		}
		entry->key = *sessionkey;
		entry->window = window;
		invalidate(entry->localcred); /* mark any cached cred invalid */
	} else { /* ADN_NICKNAME */
		/*
		 * nicknames are cooked into fullnames
		 */
		cred->adc_namekind = ADN_FULLNAME;
		cred->adc_fullname.name = entry->rname;
		cred->adc_fullname.key = entry->key;
		cred->adc_fullname.window = entry->window;
	}
	mutex_exit(&authdes_lock);
	return (AUTH_OK);	/* we made it! */
}

/*
 * Initialize the cache
 */
static void
cache_init(void)
{
	register short i;

	ASSERT(MUTEX_HELD(&authdes_lock));
	authdes_cache = (struct cache_entry *)
		mem_alloc(sizeof (struct cache_entry) * authdes_cachesz);
	bzero((char *)authdes_cache,
		sizeof (struct cache_entry) * authdes_cachesz);

	/*
	 * Initialize the lru chain (linked-list)
	 */
	for (i = 1; i < (authdes_cachesz - 1); i++) {
		authdes_cache[i].index = i;
		authdes_cache[i].next = &authdes_cache[i + 1];
		authdes_cache[i].prev = &authdes_cache[i - 1];
	}
	cache_head = &authdes_cache[0];
	cache_tail = &authdes_cache[authdes_cachesz - 1];

	/*
	 * These elements of the chain need special attention...
	 */
	cache_head->index = 0;
	cache_tail->index = authdes_cachesz - 1;
	cache_head->next = &authdes_cache[1];
	cache_head->prev = cache_tail;
	cache_tail->next = cache_head;
	cache_tail->prev = &authdes_cache[authdes_cachesz - 2];
}

/*
 * Find the lru victim
 */
static short
cache_victim(void)
{

	ASSERT(MUTEX_HELD(&authdes_lock));
	return (cache_head->index);	/* list in lru order */
}

/*
 * Note that sid was referenced
 */
static void
cache_ref(register short sid)
{
	register struct cache_entry *curr = &authdes_cache[sid];


	ASSERT(MUTEX_HELD(&authdes_lock));

	/*
	 * move referenced item from its place on the LRU chain
	 * to the tail of the chain while checking for special
	 * conditions (mainly for performance).
	 */
	if (cache_tail == curr) {			/* no work to do */
		/*EMPTY*/;
	} else if (cache_head == curr) {
		cache_head = cache_head->next;
		cache_tail = curr;
	} else {
		(curr->next)->prev = curr->prev;	/* fix thy neighbor */
		(curr->prev)->next = curr->next;
		curr->next = cache_head;		/* fix thy self... */
		curr->prev = cache_tail;
		cache_head->prev = curr;		/* fix the head  */
		cache_tail->next = curr;		/* fix the tail  */
		cache_tail = curr;			/* move the tail */
	}

}

/*
 * Find a spot in the cache for a credential containing
 * the items given.  Return -1 if a replay is detected, otherwise
 * return the spot in the cache.
 */
static short
cache_spot(register des_block *key, char *name, struct timeval *timestamp)
{
	register struct cache_entry *cp;
	register int i;
	register u_long hi;

#ifdef lint
	timestamp = timestamp;
#endif
	ASSERT(MUTEX_HELD(&authdes_lock));
	hi = key->key.high;
	for (cp = authdes_cache, i = 0; i < authdes_cachesz; i++, cp++) {
		if (cp->key.key.high == hi &&
		    cp->key.key.low == key->key.low &&
		    cp->rname != NULL &&
		    bcmp(cp->rname, name, strlen(name) + 1) == 0) {
#ifdef _AUTH_CHECKTIME
			if (BEFORE(timestamp, &cp->laststamp)) {
				mutex_enter(&svcauthdesstats_lock);
				svcauthdes_stats.ncachereplays++;
				mutex_exit(&svcauthdesstats_lock);
				return (-1); /* replay */
			}
#endif
			mutex_enter(&svcauthdesstats_lock);
			svcauthdes_stats.ncachehits++;
			mutex_exit(&svcauthdesstats_lock);
			return (i);	/* refresh */
		}
	}
	mutex_enter(&svcauthdesstats_lock);
	svcauthdes_stats.ncachemisses++;
	mutex_exit(&svcauthdesstats_lock);
	return (cache_victim());	/* new credential */
}

/*
 * Local credential handling stuff.
 * NOTE: bsd unix dependent.
 * Other operating systems should put something else here.
 */
#define	UNKNOWN 	-2	/* grouplen, if cached cred is unknown user */
#define	INVALID		-1 	/* grouplen, if cache entry is invalid */

struct bsdcred {
	uid_t uid;		/* cached uid */
	gid_t gid;		/* cached gid */
	short grouplen;	/* length of cached groups */
	short groups[NGROUPS_UMAX];	/* cached groups */
};

/*
 * Map a des credential into a unix cred.
 * We cache the credential here so the application does
 * not have to make an rpc call every time to interpret
 * the credential.
 */
int
authdes_getucred(const struct authdes_cred *adc, uid_t *uid, gid_t *gid,
	short *grouplen, register gid_t *groups)
{
	unsigned sid;
	register int i;
	uid_t i_uid;
	gid_t i_gid;
	int i_grouplen;
	struct bsdcred *cred;

	sid = adc->adc_nickname;
	if (sid >= authdes_cachesz) {
		RPCLOG(1, "authdes_getucred:  invalid nickname\n", 0);
		return (0);
	}
	mutex_enter(&authdes_lock);
	/* LINTED pointer alignment */
	cred = (struct bsdcred *)authdes_cache[sid].localcred;
	if (cred == NULL) {
		cred = (struct bsdcred *)mem_alloc(sizeof (struct bsdcred));
		authdes_cache[sid].localcred = (char *)cred;
		cred->grouplen = INVALID;
	}
	if (cred->grouplen == INVALID) {
		/*
		 * not in cache: lookup
		 */
		if (netname2user(adc->adc_fullname.name, &i_uid, &i_gid,
			&i_grouplen, (int *) groups) != RPC_SUCCESS) {
			RPCLOG(1, "authdes_getucred:  unknown netname\n", 0);
			cred->grouplen = UNKNOWN;	/* mark as lookup up */
							/* but not found */
			mutex_exit(&authdes_lock);
			return (0);
		}
		RPCLOG(1, "authdes_getucred:  missed ucred cache\n", 0);
		*uid = cred->uid = i_uid;
		*gid = cred->gid = i_gid;
		*grouplen = cred->grouplen = (short)i_grouplen;
		for (i = i_grouplen - 1; i >= 0; i--) {
			cred->groups[i] = groups[i]; /* int to short */
		}
		mutex_exit(&authdes_lock);
		return (1);
	} else if (cred->grouplen == UNKNOWN) {
		/*
		 * Already lookup up, but no match found
		 */
		mutex_exit(&authdes_lock);
		return (0);
	}

	/*
	 * cached credentials
	 */
	*uid = cred->uid;
	*gid = cred->gid;
	*grouplen = cred->grouplen;
	for (i = cred->grouplen - 1; i >= 0; i--) {
		groups[i] = cred->groups[i];	/* short to int */
	}
	mutex_exit(&authdes_lock);
	return (1);
}

static void
invalidate(char *cred)
{

	if (cred == NULL) {
		return;
	}
	/* LINTED pointer alignment */
	((struct bsdcred *)cred)->grouplen = INVALID;
}
