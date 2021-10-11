/*
 * Copyright 1991 NCR Corporation - Dayton, Ohio, USA
 *
 *	Copyright (c) 1994-1996 Sun Microsystems, Inc.
 *	All rights reserved.
 */

#pragma ident "@(#)lm_server.c 1.57     96/10/17 SMI" /* NCR OS2.00.00 1.2 */

/*
 * These are the interfaces needed by the server side of the Lock Manager.
 * The code in this file is independent of the version of the NLM protocol.
 * For code to specifically support version 1-3 see the file lm_nlm_server.c.
 * For code to specifically support version 4 see the file lm_nlm4_server.c
 */

#include <sys/types.h>
#include <sys/cred.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/flock.h>
#include <sys/share.h>
#include <sys/kmem.h>
#include <sys/netconfig.h>
#include <sys/proc.h>
#include <sys/stream.h>
#include <sys/systm.h>
#include <sys/strsubr.h>

#include <sys/sysmacros.h>
#include <sys/user.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/debug.h>
#include <sys/pathname.h>
#include <sys/callb.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/sm_inter.h>
#include <nfs/nfs.h>
#include <nfs/nfssys.h>
#include <nfs/nfs_clnt.h>
#include <nfs/export.h>
#include <nfs/rnode.h>
#include <nfs/lm.h>
#include <nfs/lm_server.h>
#include <rpcsvc/sm_inter.h>

#include <sys/cmn_err.h>
#include <sys/mutex.h>

/*
 * Lock Manager server side global variable declarations
 */
lm_server_status_t lm_server_status = LM_UP;
struct lm_vnode *lm_vnodes = NULL;
struct kmem_cache *lm_vnode_cache = NULL;
kmutex_t lm_vnodes_lock;
unsigned int lm_vnode_len = 0;
kcondvar_t lm_status_cv;

#ifdef DEBUG
int lm_gc_sysids = 0;
#endif DEBUG

/*
 * When lock service is established, we remember the pid for lockd, so that
 * we can kill it when shutting down the lock manager.
 */
static pid_t lockd_pid = 0;

/*
 * lm_xprts is a null-terminated (next == NULL) list protected by lm_lck.
 */
static struct lm_xprt *lm_xprts = NULL;
static struct kmem_cache *lm_xprt_cache = NULL;

/*
 * the system portion of the owner handle
 */
long lm_owner_handle_sys;

/*
 * static function prototypes (forward declarations).
 */
static void free_shares(struct lm_vnode *lv);
static void free_all_vnodes(void);
static int  lm_block_match(lm_block_t *target, struct flock64 *flkp, struct
		lm_vnode *lmvp, int *exactp);
static void lm_free_tables(void);
static void lm_xprtclose(const SVCXPRT *xprt);

/*
 * Lock Manager Server system call.
 * Does all of the work of running a LM server.
 * uap->fd is the fd of an open transport provider.
 */
int
lm_svc(struct lm_svc_args *uap)
{
	int error;			/* intermediate error value */
	int retval = 0;			/* final return value */
	int tries;
	struct file *fp = NULL;
	SVCXPRT *xprt = NULL;		/* handle to use with given config */
	struct knetconfig config;
	struct lm_sysid *ls;
	struct lm_sysid *me;

	/*
	 * Initialize global variables.
	 */
	lm_sa = *uap;	/* Structure copy. */

	/*
	 * check the version number
	 */
	if (lm_sa.version != LM_SVC_CUR_VERS) {
			cmn_err(CE_WARN, "lm_svc: expected version %d, got %d",
				LM_SVC_CUR_VERS, uap->version);
			return (EINVAL);
	}

	/*
	 * Make sure that we won't accidentally clobber a struct flock
	 * l_pad area (this check has to go somewhere...).
	 */
#ifndef lint
	ASSERT(sizeof (pad_info_t) == 4 * sizeof (long));
#endif

	/*
	 * Check permissions and fd.  Once we get the file pointer, we must
	 * release it before returning.
	 */
	if (!suser(CRED())) {
		return (EPERM);
	}

	if ((fp = GETF(uap->fd)) == NULL) {
		return (EBADF);
	}

	if (!lm_caches_created) {
		lm_caches_init();
	}

	/* set up knetconfig structure for tli routines */
	config.knc_semantics = uap->n_proto == LM_TCP ?
			NC_TPI_COTS_ORD : NC_TPI_CLTS;
	config.knc_protofmly = uap->n_fmly == LM_INET ? NC_INET : NC_LOOPBACK;
#ifdef LOOPBACK_LOCKING
	if (uap->n_proto == LM_TCP || uap->n_proto == LM_UDP) {
		config.knc_proto = uap->n_proto == LM_TCP ? NC_TCP : NC_UDP;
	} else {
		config.knc_proto = NC_NOPROTO;
	}
#else
	config.knc_proto = uap->n_proto == LM_TCP ? NC_TCP : NC_UDP;
#endif
	config.knc_rdev	= uap->n_rdev;


	lm_debuA(1, "svc",
	"fm= %s, pr= %s, dv= %x, db= %u, handle_tm= %u, rexmit_tm=%u, gr= %u",
		(int)config.knc_protofmly, (int)config.knc_proto,
		config.knc_rdev, lm_sa.debug, lm_sa.timout,
		lm_sa.retransmittimeout, lm_sa.grace);

	mutex_enter(&lm_lck);

	/*
	 * Check whether the lock manager is shutting down.  Once it's
	 * down, it's okay to restart it, but if it's still shutting down,
	 * we should bail out.
	 */
	if (lm_server_status == LM_SHUTTING_DOWN) {
		retval = EAGAIN;
		mutex_exit(&lm_lck);
		goto done;
	}

	/*
	 * If the lock manager is restarting or starting for the first
	 * time, remember lockd's pid, so that we can kill it when we
	 * shutdown.
	 */
	if (lm_server_status == LM_DOWN || lockd_pid == 0) {
		lockd_pid = curproc->p_pid;
		/*
		 * set up the system portion of the owner handle id
		 * Currently this is the first four characters of the
		 * name for this node.  It is possible that this won't
		 * be unique.  The protocol does not require this to
		 * be unique.  It is also possible that utsname.nodename
		 * will be shorter than LM_OH_SYS_LEN.  This is also
		 * OK because this data is used as binary data and is
		 * not interpreted.
		 */
		strncpy((char *)&lm_owner_handle_sys,
			utsname.nodename, LM_OH_SYS_LEN);
	}

	/*
	 * The NCR code used the amount of time since lm_svc was last called
	 * to determine if this is a restart.  Now that the lm_server_status
	 * state machine exists it is used to determine if this is a restart
	 * When this lm_svc is called in the LM_DOWN consider this a
	 * restart.  If we are restarting, then:
	 * - Release all locks held by the LM.
	 * - Inform the status monitor and the local locking code.
	 */
	if (lm_server_status == LM_DOWN) {
		lm_debu3(9, "svc",
			"lockmanager server is starting up\n");
		/*
		 * Set the start_time so that other LM daemons do not
		 * enter this code.
		 */
		lm_stat.start_time = time;
		lm_server_status = LM_UP;
		flk_set_lockmgr_status(FLK_LOCKMGR_UP);
		mutex_exit(&lm_lck);

		/*
		 * Release all locks held by LM.
		 * Reset the SM told indicator.  They will be reestablished
		 * by a reclaim.  Release all client handles.
		 */
		rw_enter(&lm_sysids_lock, RW_READER);
		for (ls = lm_sysids; ls; ls = ls->next) {
			lm_ref_sysid(ls);
			lm_unlock_client(ls);
			ls->sm_server = FALSE;
			lm_rel_sysid(ls);
		}
		rw_exit(&lm_sysids_lock);
		lm_flush_clients(NULL);

		/*
		 * Inform the SM that we have restarted.
		 */
		me = lm_get_me();

		for (tries = 0; tries < LM_RETRY; tries++) {
			error = lm_callrpc(me, SM_PROG, SM_VERS, SM_SIMU_CRASH,
				xdr_void, NULL, xdr_void, NULL, LM_CR_TIMOUT,
				LM_RETRY);
			if (error == 0 || error != EIO)
				break;
			(void) lm_delay(LM_STATD_DELAY * (1 << tries) * hz);
		}
		lm_debu5(1, "svc", "[%x]: lm_callrpc returned %d",
			(int)curthread, error);
		if (error != 0) {
			nfs_cmn_err(error, CE_WARN,
			    "lockd: cannot contact statd (%m), continuing");
		}

		lm_rel_sysid(me);

		/*
		 * After performing SM_SIMU_CRASH, all /etc/sm files have been
		 * deleted.  Therefore inform SM again if necessary.  Also
		 * reclaim all locks held on NFS servers, they know that we
		 * have restarted. Sleep a little before reclaiming; the
		 * status monitor must have time enough to inform the servers.
		 */
		rw_enter(&lm_sysids_lock, RW_READER);
		for (ls = lm_sysids; ls; ls = ls->next) {
			lm_ref_sysid(ls);
			lm_debu5(1, "svc", "[%x]: processing sysid %x",
				(int)curthread, ls->sysid);
			if (ls->sm_client) {
				ls->sm_client = FALSE;
				lm_sm_client(ls, me);
			}
			if (ls->sm_server) {
				ls->sm_server = FALSE;
				lm_sm_server(ls, me);
			}
			lm_rel_sysid(ls);
		}
		rw_exit(&lm_sysids_lock);

		(void) lm_delay(LM_RLOCK_SLP*hz);

		rw_enter(&lm_sysids_lock, RW_READER);
		lm_relock_server(NULL);
		rw_exit(&lm_sysids_lock);

		/*
		 * Set the start_time again.
		 * It might take a long time to do a SM_SIMU_CRASH, and we
		 * want all clients to have a full grace period.
		 */
		mutex_enter(&lm_lck);
		lm_stat.start_time = time;
		mutex_exit(&lm_lck);
	} else {
		mutex_exit(&lm_lck);
	}

	/*
	 * Allow this xprt to be mapped to `config' via fp.  In
	 * NCR's design, an LM_SVC registration thread slept in the
	 * kernel and eventually serviced its `own' NLM requests
	 * (those for its config/addr/xprt).  Such a thread
	 * therefore always had access to its original knetconfig
	 * information, stashed in its u-area.
	 *
	 * Here the calling thread leaves the kernel and clone
	 * threads of the original service thread are fired off to
	 * service NLM requests.  The clone threads have no
	 * knowledge of what their `original' knetconfig was -
	 * unless we save it for them now.
	 */
	(void) lm_saveconfig(fp, &config);

	/*
	 * Create a transport endpoint and create one or more kernel threads
	 * to run the LM service loop (svc_run).
	 */
	error = svc_tli_kcreate(fp, 0, NULL, NULL, lm_sa.max_threads, &xprt);
	if (error) {
		lm_debu4(1, "svc", "svc_tli_kcreate returned %d", error);
		lm_rmconfig(fp);
		RELEASEF(uap->fd);
		lm_exit();
	}

	mutex_enter(&lm_lck);
	lm_stat.servers = lm_sa.max_threads;
	mutex_exit(&lm_lck);

	lm_debu4(1, "svc", "xprt= %x", (int)xprt);

	lm_debu5(1, "svc", "Running, servers= %d, start_time= %x\n",
			lm_stat.servers, lm_stat.start_time);

	if (xprt == NULL) {
		lm_rmconfig(fp);
	} else {
		/*
		 * Register a cleanup routine in case the transport gets
		 * destroyed.  If the registration fails for some reason,
		 * it means that the transport is already being destroyed.
		 * This shouldn't happen, but it's probably not worth a
		 * panic.
		 */
		if (!svc_control(xprt, SVCSET_CLOSEPROC,
		    (void *)lm_xprtclose)) {
#ifdef DEBUG
			cmn_err(CE_PANIC,
			    "lm_svc: couldn't register xprt callback.");
#else
			cmn_err(CE_WARN,
			    "lm_svc: couldn't register xprt callback.");
#endif
			retval = EBADF;
			goto done;
		}

		/*
		 * Register the LM services.  Version 2 is used for
		 * callbacks from the status monitor.
		 */
		(void) svc_register(xprt, NLM_PROG, NLM_VERS,
				    lm_nlm_dispatch, FALSE);

		(void) svc_register(xprt, NLM_PROG, NLM_VERS2,
				    lm_nlm_dispatch, FALSE);
		(void) svc_register(xprt, NLM_PROG, NLM_VERS3,
				    lm_nlm_dispatch, FALSE);
		(void) svc_register(xprt, NLM_PROG, NLM4_VERS,
				    lm_nlm4_dispatch, FALSE);
	}

done:
	if (fp != NULL)
		RELEASEF(uap->fd);
	return (retval);
}

/*
 * Tell the SM to monitor a server for a client.
 * Do only tell SM if not already told.
 */
void
lm_sm_client(struct lm_sysid *ls, struct lm_sysid *me)
{
	struct mon m;
	struct sm_stat_res ssr;
	int	error;

	lm_debu6(4, "sm_client", "server= %s, sysid= %x, sm_client= %d",
			(int)ls->name, ls->sysid, ls->sm_client);

	if (!ls->sm_client) {
		m.mon_id.mon_name = ls->name;
		m.mon_id.my_id.my_name = utsname.nodename;
		m.mon_id.my_id.my_prog = NLM_PROG;
		m.mon_id.my_id.my_vers = NLM_VERS2;
		m.mon_id.my_id.my_proc = PRV_RECOVERY;

		error = lm_callrpc(me, SM_PROG, SM_VERS, SM_MON, xdr_mon,
			(caddr_t)&m,
			xdr_sm_stat_res, (caddr_t)&ssr, LM_SM_TIMOUT, LM_RETRY);
		if (error == 0) {
			ls->sm_client = (ssr.res_stat == stat_succ);
		} else {
			ls->sm_client = 0;
		}
		ls->sm_client = (ssr.res_stat == stat_succ);
		lm_rel_sysid(me);
	}
}

/*
 * Tell the SM to monitor a client for a server.
 * Do only tell SM if not already told.
 */
void
lm_sm_server(struct lm_sysid *ls, struct lm_sysid *me)
{
	struct mon m;
	struct sm_stat_res ssr;
	int	error;

	lm_debu6(4, "sm_server", "client= %s, sysid= %x, sm_server= %d",
		(int)ls->name, ls->sysid, ls->sm_server);

	if (!ls->sm_server) {
		m.mon_id.mon_name = ls->name;
		m.mon_id.my_id.my_name = utsname.nodename;
		m.mon_id.my_id.my_prog = NLM_PROG;
		m.mon_id.my_id.my_vers = NLM_VERS2;
		m.mon_id.my_id.my_proc = PRV_CRASH;

		error = lm_callrpc(me, SM_PROG, SM_VERS, SM_MON, xdr_mon,
		    (caddr_t)&m,
		    xdr_sm_stat_res, (caddr_t)&ssr, LM_SM_TIMOUT, LM_RETRY);
		if (error == 0) {
			ls->sm_server = (ssr.res_stat == stat_succ);
		} else {
			ls->sm_server = 0;
		}
		lm_rel_sysid(me);
	}
}

/*
 * Release the lm_vnode, that is:
 *	- decrement count
 *	- if count==0 release vnode iff no NFS-locks or NFS-shares exist.
 */
void
lm_rel_vnode(struct lm_vnode *lv)
{
	int flag = FREAD | FWRITE;
	int error;
	struct vnode *tmpvp;
	struct flock64 flk;
	struct shrlock shr;

	ASSERT(lv->count > 0);
	mutex_enter(&lm_lck);

	if (--(lv->count) == 0) {
		/*
		 * Check NFS locks and shares.
		 *
		 * F_HASREMOTELOCKS passes back a boolean flag in l_rpid:
		 * 1 ==> vp has NFS locks, else 0.
		 */
		error = VOP_FRLOCK(lv->vp, F_HASREMOTELOCKS, &flk, flag,
				(u_offset_t)0, CRED());

		if (error == 0 && l_has_rmt(&flk) == 0) {
			/*
			 * No NFS locks exist on lv->vp, check shares.
			 * Set sysid == 0 to lookup all sysids.
			 * NOTE: the access field is overloaded as a
			 * boolean flag, 1 == vp has NFS shares, else 0.
			 */
			shr.access = 0;
			shr.sysid = 0;
			error = VOP_SHRLOCK(lv->vp, F_HASREMOTELOCKS,
					&shr, flag);
			if (error == 0 && shr.access == 0) {
				/*
				 * No NFS locks or shares exist on lv->vp.
				 * Release vnode and mark lm_vnode as free.
				 */
				tmpvp = lv->vp;
				lv->vp = NULL;
				bzero((caddr_t)&lv->fh2, sizeof (nfs_fhandle));
				bzero((caddr_t)&lv->fh3, sizeof (nfs_fh3));
				VN_RELE(tmpvp);
			}
		}
	}

	lm_debu7(3, "rel_vnode",
		"cnt= %d, vp= %x, v_cnt= %d, v_flk= %x, sh= %x",
		lv->count, (int)lv->vp, (lv->vp ? lv->vp->v_count : -1),
		lv->vp ? (int)lv->vp->v_filocks : NULL);

	mutex_exit(&lm_lck);
}

/*
 * Free any unused lm_vnode's.
 */
/*ARGSUSED*/
void
lm_free_vnode(void *cdrarg)
{
	struct lm_vnode *lv;
	struct lm_vnode *prevlv = NULL;	/* previous kept lm_vnode */
	struct lm_vnode *nextlv = NULL;

	mutex_enter(&lm_vnodes_lock);
	mutex_enter(&lm_lck);
	lm_debu4(5, "free_vnode", "start length: %d\n", lm_vnode_len);

	for (lv = lm_vnodes; lv != NULL; lv = nextlv) {
		nextlv = lv->next;
		if (lv->vp != NULL) {
			prevlv = lv;
		} else {
			if (prevlv == NULL) {
				lm_vnodes = nextlv;
			} else {
				prevlv->next = nextlv;
			}
			ASSERT(lm_vnode_len != 0);
			--lm_vnode_len;
			ASSERT(lv->count == 0);
			ASSERT(lv->blocked == NULL);
			kmem_cache_free(lm_vnode_cache, lv);
		}
	}

	lm_debu4(5, "free_vnode", "end length: %d\n", lm_vnode_len);
	mutex_exit(&lm_lck);
	mutex_exit(&lm_vnodes_lock);
}

/*
 * Remove all locks and shares set by the client having lm_sysid `ls'.
 */
void
lm_unlock_client(struct lm_sysid *ls)
{
	struct lm_vnode *lv;
	struct flock64 flk;
	struct shrlock shr;
	int flag = FREAD | FWRITE;
	int locks_released = 0;

	lm_debu5(4, "ulck_clnt", "name= %s, sysid= %x", (int)ls->name,
		ls->sysid);

	/*
	 * Release all locks and shares held by the client, and unblock
	 * (deny) all threads blocking on locks for the client.
	 *
	 * That is, for each vnode in use release any lock/share held by
	 * client.  Note:  we have to increment count, so that the lm_vnode
	 * is not released while we are using it.
	 */
	mutex_enter(&lm_vnodes_lock);

	for (lv = lm_vnodes; lv; lv = lv->next) {
		mutex_enter(&lm_lck);
		if (lv->vp == NULL) {
			mutex_exit(&lm_lck);
			continue;
		}
		lv->count++;

		if (locks_released == 0) {
			/*
			 * Release *all* locks of this client.
			 * XXX: too long a time to hold locks here?
			 */
			flk.l_type = F_UNLKSYS;
			flk.l_whence = 0;
			flk.l_start = 0;
			flk.l_len = 0;
			flk.l_sysid = ls->sysid;
			flk.l_pid = 0;

			(void) VOP_FRLOCK(lv->vp, F_RSETLK, &flk,
				flag, (u_offset_t)0, CRED());
			locks_released = 1;
		}

		/*
		 * Release any shares held by this sysid
		 * by setting own_len == 0 and then let go our hold on vp.
		 */
		shr.access = 0;
		shr.deny = 0;
		shr.sysid = ls->sysid;
		shr.pid = 0;
		shr.own_len = 0;
		shr.owner = NULL;

		(void) VOP_SHRLOCK(lv->vp, F_UNSHARE, &shr, flag);

		mutex_exit(&lm_lck);
		lm_rel_vnode(lv);
	}

	mutex_exit(&lm_vnodes_lock);

	/*
	 * release all threads waiting for responses to granted messages
	 */
	lm_release_blocks(ls->sysid);

	/*
	 * Make sure we reinform the SM (why is this necessary?).
	 */
	ls->sm_client = FALSE;
}

/*
 * A client machine has rebooted.
 * Release all locks and shares held for the client.
 */
/* ARGSUSED */
void
lm_crash(void *gen_args,
	void *resp,			/* unused */
	struct lm_nlm_disp *disp,	/* unused */
	struct lm_sysid *ls,		/* unused */
	u_int xid)			/* unused */
{
	status *s = (status *)gen_args;

	lm_debu5(2, "crash", "mon_name= %s, state= %d", (int)s->mon_name,
		s->state);

	if (s->mon_name) {
		rw_enter(&lm_sysids_lock, RW_READER);
		for (ls = lm_sysids; ls; ls = ls->next) {
			if (strcmp(s->mon_name, ls->name) == 0) {
				lm_ref_sysid(ls);
				lm_unlock_client(ls);
				lm_flush_clients(ls);
				lm_rel_sysid(ls);
			}
		}
		rw_exit(&lm_sysids_lock);
	}
	lm_debu3(2, "crash", "End");
}

/*
 * Retransmit (reclaim) all locks held by client on server.
 *
 * XXX NCR porting issues:
 *  1. For HA, need heuristic to decide which sysids are for takeover server?
 */
void
lm_relock_server(char *server)
{
	struct lm_sysid *ls;
	locklist_t *llp, *next_llp;

	lm_debu4(4, "rlck_serv", "server= %s",
		server ? (int)server : (int)"(NULL)");

	/*
	 * We can't verify that caller has lm_sysids_lock as a reader the
	 * way we'd like to, but at least we can assert that somebody does.
	 */
	ASSERT(RW_READ_HELD(&lm_sysids_lock));

	/*
	 * 0.  Make sure we have lm_sysids_lock, so that list is frozen.
	 * 1.  Walk lm_sysids list to map `server' to any and all sysid(s)
	 *		we have for it:
	 * 2.  foreach (sysid):
	 * 3.    get the list of active locks for this sysid.
	 * 4.    foreach (lock on list):
	 * 5. 		Reclaim lock (lm_reclaim_lock()) or
	 *			signal SIGLOST.
	 * 6.		free(lock list entry).
	 * 7.    end.
	 * 8.  end.
	 */

	lm_debu3(1, "rlck_serv", "entering lm_lck");
	for (ls = lm_sysids; ls; ls = ls->next) {
		if (server != NULL && strcmp(server, ls->name))
			continue;
		/*
		 * Fudge the reference count for the lm_sysid.  We know
		 * that the entry can't go away because we hold the lock
		 * for the list.  As long as we don't pass the lm_sysid to
		 * any other routines, we don't need to bump the reference
		 * count.
		 */
		lm_debu4(1, "rlck_serv", "calling VOP_FRLOCK(ACT) on %x",
			ls->sysid | LM_SYSID_CLIENT);

		llp = flk_get_active_locks(ls->sysid | LM_SYSID_CLIENT, NOPID);
		lm_debu4(1, "rlck_serv", "VOP_FRLOCK(ACT) returned llp %x",
			(int)llp);
		while (llp) {
			lm_debu4(1, "rlck_serv", "calling lm_reclaim(%x)",
				(int)llp);
			lm_reclaim_lock(llp->ll_vp, &llp->ll_flock);
			next_llp = llp->ll_next;
			VN_RELE(llp->ll_vp);
			kmem_free(llp, sizeof (*llp));
			llp = next_llp;
		}
	}
}

void
lm_reclaim_lock(struct vnode *vp, struct flock64 *flkp)
{
	if (VTOMI(vp)->mi_vers == NFS_V3) {
	    lm_nlm4_reclaim(vp, flkp);
	} else {
	    lm_nlm_reclaim(vp, flkp);
	}
}

/*
 * nlm dispatch bookkeeping routines.
 */

/*
 * bump the counter keeping track of the number of outstanding requests
 * check on the status of the server to make sure it is still up
 * return 0 if everything is OK
 * return 1 if an error is found (the server is not up)
 * N.B. the counter of keeping track of the number of outstanding requests
 * is bumped even if there is an error.  This means the caller must call
 * nlm_dispatch_exit before returning from the dispatch routine.
 */
int
nlm_dispatch_enter(register SVCXPRT *xprt)
{
	int
		error;

#ifdef DEBUG
	if (lm_gc_sysids) {
		lm_free_sysid_table(NULL);
	}
#endif

	mutex_enter(&lm_lck);
	/*
	 * this is a new request.  bump the count of outstanding requests
	 */
	++lm_num_outstanding;
	/*
	 * We shouldn't be getting new requests if we're down.
	 */
	if (lm_server_status != LM_UP) {
		if (lm_server_status == LM_DOWN) {
			mutex_exit(&lm_lck);
			cmn_err(CE_WARN,
				"lm_nlm_dispatch: unexpected request.");
		} else {
			mutex_exit(&lm_lck);
		}
		svcerr_systemerr(xprt);	/* could just drop on the floor? */
		error = 1;
	} else {
		mutex_exit(&lm_lck);
		error = 0;
	}
	return (error);
}

/*
 * decrement the counter keeping track of the number of outstanding requests
 * if the server status is in the process of coming down and this is the
 * last outstanding request, then bring the server completely down by
 * releasing all the dynamically allocated resources.
 */

void
nlm_dispatch_exit()
{
	mutex_enter(&lm_lck);
	/*
	 * this request is now done.  decrement the number of outstanding
	 * requests.  If this is the last request and we are in the process
	 * of shutting down, mark the lock manager as down and free all of
	 * our resources (including remote locks already granted).  As soon
	 * as we have dynamic garbage collection
	 * in the kernel, we won't need to free the tables anymore.
	 */
	--lm_num_outstanding;
	if ((lm_num_outstanding == 0) &&
		(lm_server_status == LM_SHUTTING_DOWN)) {
		flk_set_lockmgr_status(FLK_LOCKMGR_DOWN);
		mutex_exit(&lm_lck);
		lm_free_tables();
		/*
		 * Now that all the cleanup has happened, mark the lock
		 * manager as down and notify anyone waiting for this
		 * event.
		 */
		mutex_enter(&lm_lck);
		lm_server_status = LM_DOWN;
		cv_signal(&lm_status_cv);
		mutex_exit(&lm_lck);
	} else {
		mutex_exit(&lm_lck);
	}
}

/*
 * Shut down the lock manager and return when everything is done.  Returns
 * zero for success or an errno value.
 */
int
lm_shutdown()
{
	proc_t *p;

	if (!suser(CRED())) {
		return (EPERM);
	}
	if (lockd_pid == 0) {
		return (ESRCH);		/* lock manager wasn't started */
	}

	/*
	 * Try to signal lockd.  If there's no such pid, it probably means
	 * that lockd has already gone away for some reason, so just wait
	 * until the lock manager is completely down.
	 */
	mutex_enter(&pidlock);
	p = prfind(lockd_pid);
	if (p != NULL) {
		psignal(p, SIGTERM);
	}
	mutex_exit(&pidlock);

	mutex_enter(&lm_lck);
	while (lm_server_status != LM_DOWN) {
		cv_wait(&lm_status_cv, &lm_lck);
	}
	mutex_exit(&lm_lck);

	return (0);
}

/*
 * Callback routine for when a transport is closed.  Removes the config for
 * the transport from the config table.  If this is the last transport,
 * initiate the shut down sequence for the lockmanager
 */
void
lm_xprtclose(const SVCXPRT *xprt)
{
	lm_rmconfig(xprt->xp_fp);

	mutex_enter(&lm_lck);
	if (lm_numconfigs == 0) {
		/*
		 * Record that we are shutting down.  Tell the local
		 * locking code to wake up sleeping remote requests.
		 */
		if (lm_server_status == LM_UP) {
			lm_server_status = LM_SHUTTING_DOWN;
			flk_set_lockmgr_status(FLK_WAKEUP_SLEEPERS);
		}
		/*
		 * if there are no outstanding requests, then the lock manager
		 * is completely down.  Mark it as so and finish the shut down
		 * process
		 */
		if (lm_num_outstanding == 0) {
			flk_set_lockmgr_status(FLK_LOCKMGR_DOWN);
			mutex_exit(&lm_lck);
			lm_free_tables();
			mutex_enter(&lm_lck);
			lm_server_status = LM_DOWN;
			cv_signal(&lm_status_cv);
			mutex_exit(&lm_lck);
		} else {
			mutex_exit(&lm_lck);
		}
	} else {
		mutex_exit(&lm_lck);
	}
}

/*
 * Try to free all the server-side dynamically allocated tables.
 * Some tables may be shared with the client-side code, so it may not be safe
 * to completely free them up.
 */
static void
lm_free_tables()
{
	ASSERT(lm_server_status != LM_UP);
	ASSERT(lm_numconfigs == 0);
	ASSERT(lm_num_outstanding == 0);

	lm_free_xprt_map();
	free_all_vnodes();		/* also frees shares */
	lm_flush_clients_mem(NULL);
	lm_free_sysid_table(NULL);
}

/*
 * Free all the lm_vnodes in the lock manager.
 */
static void
free_all_vnodes()
{
	struct lm_vnode *lv;
	struct lm_vnode *nextlv = NULL;

	ASSERT(lm_server_status != LM_UP);
	ASSERT(lm_numconfigs == 0);
	ASSERT(lm_num_outstanding == 0);

	mutex_enter(&lm_vnodes_lock);
	mutex_enter(&lm_lck);
	for (lv = lm_vnodes; lv != NULL; lv = nextlv) {
		nextlv = lv->next;
		/*
		 * Since there are no outstanding requests, there should be
		 * no lm_block entries associated with the lm_vnode.
		 */
		ASSERT(lv->blocked == NULL);

		if (lv->vp) {
			free_shares(lv);
			VN_RELE(lv->vp);
		}
		ASSERT(lv->count == 0);
		kmem_cache_free(lm_vnode_cache, lv);
	}
	lm_vnode_len = 0;
	lm_vnodes = NULL;
	mutex_exit(&lm_lck);
	mutex_exit(&lm_vnodes_lock);
}

/*
 * Free any file-sharing records for the given vnode.
 */
static void
free_shares(struct lm_vnode *lv)
{
	struct shrlock shr;
	int flag = FREAD|FWRITE;

	/*
	 * Release any remote shares by specifying sysid == 0 and pid == 0
	 * and own_len == 0.
	 */
	shr.access = 0;
	shr.deny = 0;
	shr.sysid = 0;
	shr.pid = 0;
	shr.own_len = 0;
	shr.owner = NULL;

	(void) VOP_SHRLOCK(lv->vp, F_UNSHARE, &shr, flag);
}

/*
 * Determine if this sysid has any share requests outstanding.
 * Loop through all active vnodes testing for shares with this sysid
 */
int
lm_shr_sysid_has_locks(sysid_t sysid)
{
	int result = 0;
	struct lm_vnode *lv;
	struct shrlock shr;
	int flag = FREAD | FWRITE;
	int error;

	mutex_enter(&lm_vnodes_lock);
	mutex_enter(&lm_lck);
	for (lv = lm_vnodes; lv != NULL; lv = lv->next) {
		if (lv->vp) {
			/*
			 * NOTE: the access field is overloaded as a
			 * boolean flag, 1 == vp has NFS shares, else 0.
			 */
			shr.access = 0;
			shr.sysid = sysid;
			error = VOP_SHRLOCK(lv->vp, F_HASREMOTELOCKS,
					&shr, flag);
			if (error == 0 && shr.access != 0) {
				result = 1;
				break;
			}
		}
	}
	mutex_exit(&lm_lck);
	mutex_exit(&lm_vnodes_lock);

	return (result);
}


/*
 * Create the kmem caches for server-only tables.
 */
void
lm_server_caches_init()
{
	ASSERT(MUTEX_HELD(&lm_lck));

	lm_vnode_cache = kmem_cache_create("lm_vnode",
		sizeof (struct lm_vnode), 0, NULL, NULL,
		lm_free_vnode, NULL, NULL, 0);
	lm_xprt_cache = kmem_cache_create("lm_xprt",
		sizeof (struct lm_xprt), 0, NULL, NULL, NULL, NULL, NULL, 0);
}

/*
 * Save an xprt in lm_xprts.
 *
 * This simply allows us to map a service thread to its unique
 * clone xprt.
 */
struct lm_xprt *
lm_savexprt(SVCXPRT *xprt)
{
	struct lm_xprt *lx;

	mutex_enter(&lm_lck);
	for (lx = lm_xprts; lx; lx = lx->next) {
		if (lx->valid == 0) {
			break;
		}
	}

	if (lx == (struct lm_xprt *)NULL) {
		lx = kmem_cache_alloc(lm_xprt_cache, KM_SLEEP);
		lx->next = lm_xprts;
		lm_xprts = lx;
	}
	lx->thread = curthread;
	lx->xprt = xprt;
	lx->valid = 1;
	mutex_exit(&lm_lck);

	lm_debu7(7, "savexprt", "lm_xprt= %x thread= %x xprt= %x next= %x",
		(int)lx, (int)lx->thread, (int)lx->xprt, (int)lx->next);
	return (lx);
}

/*
 * Fetch lm_xprt corresponding to current thread.
 */
struct lm_xprt *
lm_getxprt()
{
	struct lm_xprt *lx;

	mutex_enter(&lm_lck);
	for (lx = lm_xprts; lx; lx = lx->next) {
		if (lx->valid && lx->thread == curthread)
			break;
	}

	if (lx == (struct lm_xprt *)NULL) {	/* should never happen */
		lm_debu4(7, "getxprt", "no lm_xprt for thread %x!",
			(int)curthread);
	}
	mutex_exit(&lm_lck);

	ASSERT(lx != NULL);
	lm_debu4(7, "getxprt", "lx= %x", (int)lx);
	return (lx);
}

void
lm_relxprt(SVCXPRT *xprt)
{
	struct lm_xprt *lx;

	mutex_enter(&lm_lck);
	for (lx = lm_xprts; lx; lx = lx->next) {
		if (lx->valid && lx->thread == curthread && lx->xprt == xprt) {
			break;
		}
	}

	if (lx == (struct lm_xprt *)NULL) {	/* should never happen */
		lm_debu4(7, "relxprt", "no lm_xprt for thread %x!",
			(int)curthread);
	} else {
		lx->valid = 0;
	}
	mutex_exit(&lm_lck);

	lm_debu4(7, "relxprt", "lx= %x", (int)lx);
}

/*
 * Free all the entries in the xprt table.
 */

void
lm_free_xprt_map()
{
	struct lm_xprt *lx;
	struct lm_xprt *nextlx;

	ASSERT(lm_server_status != LM_UP);
	ASSERT(lm_numconfigs == 0);
	ASSERT(lm_num_outstanding == 0);

#ifdef lint
	nextlx = (struct lm_xprt *)NULL;
#endif
	mutex_enter(&lm_lck);
	for (lx = lm_xprts; lx; lx = nextlx) {
		nextlx = lx->next;
		ASSERT(!lx->valid);
		kmem_cache_free(lm_xprt_cache, lx);
	}

	lm_xprts = NULL;
	mutex_exit(&lm_lck);
}

/*
 * Routines to process the list of blocked lock requests.
 */

/*
 * Initialize an lm_block struct.
 */
void
lm_init_block(lm_block_t *new, struct flock64 *flkp, struct lm_vnode *lv,
		netobj *id)
{
	new->lmb_state = LMB_PENDING;
	new->lmb_no_callback = FALSE;
	new->lmb_flk = flkp;
	new->lmb_id = id;
	new->lmb_vn = lv;
	new->lmb_next = NULL;
}

/*
 * add an item to the list of blocked locked requests for the associated
 * lm_vnode.
 * N.B. This routine must be called with the mutex lm_lck held
 */
void
lm_add_block(lm_block_t *new)
{
	struct lm_vnode *lv;

	ASSERT(MUTEX_HELD(&lm_lck));
	ASSERT(new->lmb_state == LMB_PENDING);
	lv = new->lmb_vn;
	new->lmb_next = lv->blocked;
	lv->blocked = new;
}

/*
 * remove an entry from the list of blocked lock requests
 * N.B. This routine must be called with the mutex lm_lck held
 */
void
lm_remove_block(lm_block_t *target)
{
	lm_block_t *cur;
	lm_block_t **lmbpp;
	struct lm_vnode *lv;

	ASSERT(MUTEX_HELD(&lm_lck));
	lv = target->lmb_vn;
	lmbpp = &lv->blocked;
	cur = lv->blocked;
	while (cur != (lm_block_t *)NULL) {
		if (cur == target) {
			/* remove it from the list and quit */
			*lmbpp = cur->lmb_next;
			return;
		}
		lmbpp = &(cur->lmb_next);
		cur = cur->lmb_next;
	}
#ifdef DEBUG
	/*
	 * lm_block_lock thought there was an entry for target on
	 * the list but is was not there.
	 */
	cmn_err(CE_PANIC, "lm_remove_block: missing entry in list");
#endif DEBUG
}

/*
 * Check whether an lm_block already exists that either matches or overlaps
 * the given request.
 *
 * req_id is the "unique identifier" for the new request.  For asynchronous
 * calls it should be the same as the request cookie.  For synchronous
 * calls, it should refer to the RPC transaction ID.
 *
 * The caller should hold lm_lck.
 *
 * Return value:
 * LMM_REXMIT	the given request is a retransmission of an earlier
 *		request.  *lmb_resp is set to point to the lm_block for the
 *		earlier request.
 *
 * LMM_FULL	the given request matches an earlier request.  It may be a
 *		retransmission.  *lmb_resp is set to point to the lm_block
 *		for the earlier request.
 *
 * LMM_PARTIAL	the given request overlaps the region of an earlier
 *		request.  *lmb_resp is set to point to the lm_block for
 *		the earlier request.
 *
 * LMM_NONE	the given request does not match any known lm_block.
 *
 */
lm_match_t
lm_find_block(struct flock64 *req_flkp, struct lm_vnode *req_lv,
		netobj *req_id, lm_block_t **lmb_resp)
{
	lm_block_t *partial_lmbp = NULL;
	lm_block_t *full_lmbp = NULL;
	lm_block_t *lmbp;
	lm_match_t result;
	int exact;

	ASSERT(MUTEX_HELD(&lm_lck));
	for (lmbp = req_lv->blocked; lmbp != NULL; lmbp = lmbp->lmb_next) {
		/*
		 * Try to find a retransmission.  Keep track of full and
		 * partial matches.
		 */
		if (lm_block_match(lmbp, req_flkp, req_lv, &exact)) {
			if (exact) {
				if (lm_netobj_eq(req_id, lmbp->lmb_id)) {
					*lmb_resp = lmbp;
					return (LMM_REXMIT);
				} else {
					full_lmbp = lmbp;
				}
			} else {
				partial_lmbp = lmbp;
			}
		}
	}

	/*
	 * Didn't find a retransmission.  Did we find a full or partial match?
	 */
	result = LMM_NONE;
	if (full_lmbp != NULL) {
		*lmb_resp = full_lmbp;
		result = LMM_FULL;
	} else if (partial_lmbp != NULL) {
		*lmb_resp = partial_lmbp;
		result = LMM_PARTIAL;
	}

	return (result);
}

/*
 * Release all entries associated with a particular sysid from the
 * lists of blocked lock requests.
 */
void
lm_release_blocks(sysid_t target)
{
	lm_block_t *lmbp;
	struct lm_vnode *lv;

	mutex_enter(&lm_vnodes_lock);
	mutex_enter(&lm_lck);
	for (lv = lm_vnodes; lv != NULL; lv = lv->next) {
		for (lmbp = lv->blocked; lmbp != NULL; lmbp = lmbp->lmb_next) {
			if (lmbp->lmb_flk->l_sysid == target) {
				/* disable any pending GRANTED callback RPC */
				lmbp->lmb_no_callback = TRUE;
			}
		}
	}
	mutex_exit(&lm_lck);
	mutex_exit(&lm_vnodes_lock);
}

/*
 * Compare a lock request with an lm_block entry.  The return value
 * is non-zero if the request matches, and zero if the request does
 * not match.  A match is defined as having the same lm_vnode, sysid,
 * and pid, and the same or overlapping region.  *exactp is set to non-zero if
 * the regions match exactly, zero if they just overlap.
 *
 * The region comparisons assume the lock regions are represented
 * as an offset and length from the beginning of the file.
 */
static int
lm_block_match(lm_block_t *target, struct flock64 *flkp, struct lm_vnode *lmvp,
	int *exactp)
{
	u_offset_t end_r;
	u_offset_t start_r;
	u_offset_t end_t;
	u_offset_t start_t;

	if (target->lmb_vn != lmvp)
		return (0);
	if (target->lmb_flk->l_sysid != flkp->l_sysid)
		return (0);
	if (target->lmb_flk->l_pid != flkp->l_pid)
		return (0);

	/*
	 * get starting and ending points of the lock request
	 * MAX_U_OFFSET_T is used for "to EOF."
	 */
	start_r = flkp->l_start;
	if (flkp->l_len == 0) {
		end_r = MAX_U_OFFSET_T;
	} else {
		end_r = start_r + (flkp->l_len - 1);
	}
	start_t = target->lmb_flk->l_start;
	if (target->lmb_flk->l_len == 0) {
		end_t = MAX_U_OFFSET_T;
	} else {
		end_t = start_t + (target->lmb_flk->l_len - 1);
	}

	/*
	 * The only check left is region overlap.  If there is overlap
	 * return that a match was found.
	 *
	 * This comparison for overlap was taken from the OVERLAP
	 * macro in sys/flock_impl.h.
	 */
	if (((start_t <= start_r) && (start_r <= end_t)) ||
		((start_r <= start_t) && (start_t <= end_r))) {
		/* overlap */
		*exactp = (start_t == start_r && end_t == end_r);
		return (1);
	} else {
		/* no overlap */
		return (0);
	}
}


/*
 * Search through the lm_vnode's blocked list for requests matching the one
 * specified by <flkp, lmvp> and set the "no callback" flag for each such
 * request.  There is a match if the lmvp is found for the same process id,
 * and any portion of the lock region.
 *
 * The caller of this routine should hold lm_lck.
 */
void
lm_cancel_granted_rxmit(struct flock64 *flkp, struct lm_vnode *lmvp)
{
	lm_block_t *lmbp;
	int exact;			/* unused */

	ASSERT(MUTEX_HELD(&lm_lck));
	for (lmbp = lmvp->blocked; lmbp != (lm_block_t *)NULL;
	    lmbp = lmbp->lmb_next) {
		if (lm_block_match(lmbp, flkp, lmvp, &exact)) {
			lmbp->lmb_no_callback = TRUE;
		}
	}
}

#ifdef DEBUG

/*
 * Dump the lm_block list for the given lm_vnode.  The caller should hold
 * lm_lck.
 */
void
lm_dump_block(struct lm_vnode *lv)
{
	lm_block_t *lmbp;
	u_int id;

	ASSERT(MUTEX_HELD(&lm_lck));
	for (lmbp = lv->blocked; lmbp != NULL; lmbp = lmbp->lmb_next) {
		if (lmbp->lmb_id->n_bytes != NULL)
			id = *(u_int *)(lmbp->lmb_id->n_bytes);
		else
			id = 0;
		printf("%x: [%d %s [off=%llu len=%llu type=%d pid=%d] %u]\n",
		    lmbp, lmbp->lmb_state,
		    (lmbp->lmb_no_callback ? "X" : " "),
		    lmbp->lmb_flk->l_start, lmbp->lmb_flk->l_len,
		    lmbp->lmb_flk->l_type, lmbp->lmb_flk->l_pid, id);
	}
}

#endif /* DEBUG */
