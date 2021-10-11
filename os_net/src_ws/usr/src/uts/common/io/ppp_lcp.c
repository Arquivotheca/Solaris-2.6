/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)ppp_lcp.c 1.18	96/09/06 SMI"

/*
 * Implements the link-control-protocol specific operations for PPP
 */


#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/stropts.h>
#include <sys/ddi.h>
#include <sys/dlpi.h>
#include <sys/debug.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/strlog.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/t_lock.h>
#include <sys/strsun.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#ifdef ISERE_TREE
#include <ppp/vjcomp.h>
#include <ppp/ppp_ioctl.h>
#include <ppp/ppp_sys.h>
#include <ppp/ppp_pap.h>
#include <ppp/ppp_chap.h>
#include <ppp/ppp_extern.h>
#else
#include <sys/vjcomp.h>
#include <sys/ppp_ioctl.h>
#include <sys/ppp_sys.h>
#include <sys/ppp_pap.h>
#include <sys/ppp_chap.h>
#include <sys/ppp_extern.h>
#endif /* ISERE_TREE */

/*
 * data structure holding LCP implemented options pointers
 */
static pppOption_t	*lcp_imp_opts[PPP_MAX_LCP_OPTS+1];

static int		mru(pppMachine_t *, pppOption_t *);
static int		ascm(pppMachine_t *, pppOption_t *);
static int		auth_type(pppMachine_t *, pppOption_t *);
static int		addr_ctrl(pppMachine_t *, pppOption_t *);
static int		proto_field(pppMachine_t *, pppOption_t *);
static int		magic_number(pppMachine_t *, pppOption_t *);
static int		in_magic_number(pppMachine_t *, pppOption_t *);
static lqm_op_t		ntoh_lqm_op_t(lqm_op_t);
static lqm_op_t		hton_lqm_op_t(lqm_op_t);
static chap_op_t	ntoh_chap_op_t(chap_op_t);
static chap_op_t	hton_chap_op_t(chap_op_t);


/*
 * lcp_external_event()
 *
 * Process an event generated by another layer, or the lm.
 */
void
lcp_external_event(lp, exevent)
pppLink_t		*lp;
u_int			exevent;
{
	u_int	event;
	pppMachine_t	*machp;

	switch (exevent) {
	case PPPIN_UP:
		event = Up;
		break;
	case PPPIN_DOWN:
		event = Down;
		break;
	case PPPIN_OPEN:
		event = Open;
		break;
	case PPPIN_CLOSE:
		event = Close;
		break;
	case PPPIN_TIMEOUT1:
		machp = lp -> lcp;
		if (machp -> timedoutid != machp -> restart)
			return;
		machp -> restart = 0;
		if (machp -> linkp -> looped_back) {
			machp -> loopback_counter--;
			if (machp -> loopback_counter > 0) {
				ppp_start_restart_timer(machp);
				event = Togt0;
			} else {
				event = Toeq0;
				ppp_error_ind(machp -> linkp,
				    pppLoopedBack, NULL, (u_int) 0);
			}
		} else {
			machp -> restart_counter--;
			if (machp -> restart_counter > 0) {
				ppp_start_restart_timer(machp);
				event = Togt0;
			} else {
				event = Toeq0;
				if (REQ_SENT_STATE_PPP(machp -> state))
					ppp_error_ind(machp -> linkp,
					    pppConfigFailed, NULL, (u_int) 0);
			}
		}

	}
	mutex_enter(&lp-> lplock);
	ppp_fsm(lp -> lcp, event);
	mutex_exit(&lp-> lplock);
}

void
ppp_apply_lcp_option(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	u_long templ;
	lqm_op_t			lqmdata, templqm;

	switch (op -> type) {

	case MagicNumber:
		bcopy(op -> data, &templ, sizeof (u_int));
		machp->linkp-> conf.pppLinkLocalMagicNumber =
		    ntohl(templ);
		break;
	case AddrCtrlCompress:
		machp->linkp->conf.pppLinkRecvAddrComp = 1;
		break;

	case ProtoFieldCompress:
		machp->linkp->conf.pppLinkRecvProtoComp = 1;
		break;

	case MaxReceiveUnit:
		machp->linkp->conf.pppLinkLocalMRU =
		    ntohs(* (u_short *) op -> data);
		break;

	case AuthenticationType:
		machp->linkp->remauth =
		    ntohs(* (u_short *) op -> data);
		break;
	case LinkQualityMon:
		switch (ntohs(* (u_short *) op -> data)) {
		case pppLQM_REPORT:
			bcopy(op -> data, &templqm, sizeof (templqm));
			lqmdata = ntoh_lqm_op_t(templqm);
			machp -> linkp -> inLQM = lqmdata.proto;
			machp -> linkp -> lqmstruct.in_rep_period =
			    lqmdata.rep_period;
		}
		break;
	}
}

/*
 * PPP option negotiation routines...
 */


/*
 * proto_field()
 *
 * Peer has asked for protocol negotiation.
 */
/* ARGSUSED1 */
static int
proto_field(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	if (machp-> linkp-> conf.pppLinkMediaType == pppAsync) {
		machp-> linkp-> conf.pppLinkSendProtoComp = 1;
		return (OPT_OK);
	} else
		return (OPT_NOK);
}


/*
 * addr_ctrl()
 *
 * Peer has asked for address and control compression.
 */
/* ARGSUSED1 */
static int
addr_ctrl(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	if (machp-> linkp-> conf.pppLinkMediaType == pppAsync) {
		machp-> linkp-> conf.pppLinkSendAddrComp = 1;
		return (OPT_OK);
	} else
		return (OPT_NOK);
}


/*
 * in_magic_number()
 *
 * We received a NAK to our magic number.  If the number in it is different
 * from the last one we sent then the link is not looped back, so we can
 * send an scr with this unique number.	 If the number is the same as the
 * last one we sent, generate a new number for the next scr.
 */
static int
in_magic_number(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	u_long		magic_num, tempm;

	bcopy(op -> data, &tempm, sizeof (tempm));
	magic_num = ntohl(tempm);

	if (machp -> linkp-> conf.pppLinkNAKMagicNumber != magic_num) {
		tempm = htonl(machp -> linkp-> conf.pppLinkNAKMagicNumber);
		bcopy(&tempm, op -> data, sizeof (tempm));
		machp -> linkp -> conf.pppLinkREQMagicNumber =
		    machp -> linkp -> conf.pppLinkNAKMagicNumber;
		machp -> linkp -> looped_back = 0;
		machp-> loopback_counter =
		    machp->linkp->conf.pppLinkMaxLoopCount;
		return (OPT_OK);
	} else {
		magic_num = ppp_rand();
		tempm = htonl(magic_num);

		bcopy(&tempm, op -> data, sizeof (tempm));
		machp -> linkp -> conf.pppLinkREQMagicNumber = magic_num;
		machp -> linkp -> looped_back = 1;

		return (OPT_OK);
	}
}


/*
 * link_qual()
 *
 * Remote is attempting to negotiate link quality reporting
 */

/*
 * Link quality monitoriting is not implemented
 * properly for SunLink PPP 2.0
 * We just reject the option when the peer tries
 * to nogotiate the use of this protocol with us
 */

/*ARGSUSED*/
static int
link_qual(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	return (OPT_NOK);
}


/*
 * magic_number()
 *
 * Remote host is attempting to negotiate a magic number
 */
static int
magic_number(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	u_long		magic_num, tempm;

	bcopy(op -> data, &tempm, sizeof (tempm));
	magic_num = ntohl(tempm);

	if (machp-> linkp -> conf.pppLinkREQMagicNumber != magic_num) {
		machp -> linkp -> conf.pppLinkLoopCount =
		    machp -> linkp -> conf.pppLinkMaxLoopCount;
		machp -> linkp -> conf.pppLinkRemoteMagicNumber =
			magic_num;
		machp -> linkp -> looped_back = 0;
		machp-> loopback_counter =
		    machp->linkp->conf.pppLinkMaxLoopCount;
		return (OPT_OK);
	} else {
		machp -> linkp -> looped_back = 1;
		magic_num = ppp_rand();
		tempm = htonl(magic_num);
		bcopy(&tempm, op -> data, sizeof (tempm));
		machp -> linkp-> conf.pppLinkNAKMagicNumber = magic_num;

		/* Our link seems to be looped-back. Restoring default ACCM. */
		machp->linkp->conf.pppLinkRemoteACCMap = (u_long)PPP_DEF_ASCM;

		return (OPT_LOOP);
	}
}

/*
 * mru()
 *
 * negotiate MaximumReceiveUnit
 *
 * Returns: OPT_OK if value was acceptable,
 *	    OPT_NOK if option was unacceptable for negotiation
 *	    OPT_NEW if new value is proposed
 *
 */
static int
mru(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
/*
 * this is the default mru option
 * [RFC1331 Page 44]
 */
	static pppOption_t	default_mru_opt =
				    { MaxReceiveUnit, 4,
				    { HI_OF(PPP_DEF_MRU),  LO_OF(PPP_DEF_MRU) },
				};
	register pppOption_t	*new_opt;
	u_short			proposed_mru, allowed_value, temps;


	if (op -> length != 4)
		return (OPT_NOK);

	bcopy(op -> data, &temps, sizeof (temps));
	proposed_mru = ntohs(temps);

/*
 * look and see what the user has suggested for this option
 */
	new_opt = machp->outbound[op->type];

	if (new_opt == NULL) {

/*
 * the user has not suggested a value, so pick up the default
 * and allow the predefined maximum
 */
		new_opt = &default_mru_opt;
		allowed_value = PPP_MAX_MRU;

	} else {
/*
 * use this option, and punt to the next one in the
 * user's list for the next time round
 */
		bcopy(new_opt->data, &temps, sizeof (temps));
		allowed_value = ntohs(temps);
		machp->outbound[op->type] = machp->outbound[op->type]->next;
	}

	if (proposed_mru < PPP_MIN_MRU || proposed_mru > allowed_value) {

/*
 * propose new option
 */
		bcopy(new_opt, op, new_opt->length);
		bcopy(new_opt->data, &temps,
		    sizeof (temps));
		machp->linkp->conf.pppLinkRemoteMRU = ntohs(temps);
		return (OPT_NEW);
	}
/*
 * his value is acceptable
 */
	machp->linkp->conf.pppLinkRemoteMRU = proposed_mru;
	return (OPT_OK);
}


/*
 * nak_mru()
 *
 * negotiate MaximumReceiveUnit
 *
 * Returns: OPT_OK if value was acceptable,
 *	    OPT_NOK if option was unacceptable for negotiation
 *	    OPT_NEW if new value is proposed
 *
 */
/* ARGSUSED */
static int
nak_mru(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	u_short temps, proposed_mru;

	if (op -> length != 4)
		return (OPT_NOK);

	bcopy(op -> data, &temps, sizeof (temps));
	proposed_mru = ntohs(temps);

	if (proposed_mru < PPP_MIN_MRU || proposed_mru > PPP_MAX_MRU) {
		return (OPT_NOK);
	}
	return (OPT_OK);
}


/*
 * ascm()
 *
 * negotiate AsyncControlMap
 *
 * Returns: OPT_OK if value was acceptable,
 *	    OPT_NOK if option was unacceptable for negotiation
 *	    OPT_NEW if new value is proposed
 */
/* ARGSUSED */
static int
ascm(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	u_long templ;

/*
 * Async Control Map is always accepted.
 */

	if (op -> length != 6)
		return (OPT_NOK);

	if (machp-> linkp-> conf.pppLinkMediaType == pppAsync) {
		bcopy(op-> data, &templ, sizeof (templ));

		machp->linkp->conf.pppLinkRemoteACCMap = ntohl(templ);

		return (OPT_OK);
	} else
		return (OPT_NOK);
}


/*
 * auth_type()
 *
 * negotiate the authentication method for this connection
 *
 * Returns: OPT_OK if value was acceptable,
 *	    OPT_NOK if option was unacceptable for negotiation
 *	    OPT_NEW if new value is proposed
 */
static int
auth_type(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	u_short				temps;
	chap_op_t			tempchap, chap_op;

/*
 * look through the list of authenticators the user has made available
 * for this connection to see if requested one is ok
 */
	if (op -> length < 4)
		return (OPT_NOK);

	bcopy(op-> data, &temps, sizeof (temps));
	switch (ntohs(temps)) {
	case pppCHAP:
		if (op -> length != 5)
			return (OPT_OK);
		if (!(machp -> linkp -> conf.pppLocAuthok & DO_CHAP))
			return (OPT_NOK);
		bcopy(op -> data, &tempchap, sizeof (tempchap));
		chap_op = ntoh_chap_op_t(tempchap);
		switch (chap_op.algorithm) {
		case MD5:
			machp->linkp->locauth = pppCHAP;
			return (OPT_OK);
		}
		break;
	case pppAuthPAP:
		if (op -> length != 4)
			return (OPT_NOK);
		if (!(machp -> linkp -> conf.pppLocAuthok & DO_PAP))
			return (OPT_NOK);
		machp->linkp->locauth = pppAuthPAP;
		return (OPT_OK);
	}

/*
 * this Authentication method is not acceptable to the user,
 * refuse the option.	Here, we should offer one of the
 * supported methods I suppose as OPT_NEW, but I'll be mean
 * for the moment
 */
	return (OPT_NOK);
}


/*
 * inauth_type()
 *
 * Remote has NAKed out auth-type negtiation, return OPT_NOK since
 * we don't want the remote side to tell us how they want to athenticated
 */
/* ARGSUSED */
static int
inauth_type(machp, op)
pppMachine_t	*machp;
pppOption_t	*op;
{
	return (OPT_NOK);
}


/*
 * ppp_lcp_initialize()
 *
 * Initialize the lcp layer.
 */
int
ppp_lcp_initialize(void)
{

	int error;

/*
 * allocate lcp implementation configuration information.
 * This represents which options are allowable for this
 * implementation.  If the remote party asks for something
 * which is not in this list then it will be rejected.
 */
	bzero(lcp_imp_opts, sizeof (lcp_imp_opts));

/*
 * params to add_opt are  [ptr] [type] [length] [&data] [negotiate func]
 */

	error =
/*
 * this is what is implemented
 *
 * MaximumReceiveUnit negotiation handler is mru()
 *
 * [RFC1331 page 44]
 */

	add_opt(lcp_imp_opts, MaxReceiveUnit, 0, NULL, mru) |

/*
 * AsyncControlMap is always accepted, though this being
 * a synchronous implementation it will not do any character
 * mapping
 *
 * AsyncControlMap negotiation handler is ascm()
 *
 * [RFC1331 page 45]
 */

	add_opt(lcp_imp_opts, AsyncControlMap, 0, NULL, ascm) |

/*
 * AddrCtrlCompress handler is addr_ctrl()
 */

	add_opt(lcp_imp_opts, AddrCtrlCompress, 0, NULL, addr_ctrl) |

/*
 * ProtoFieldCompress handler is proto_field()
 */

	add_opt(lcp_imp_opts, ProtoFieldCompress, 0, NULL, proto_field) |

/*
 * AuthenticationType handler is auth_type()
 *
 * [RFC1331 page 47]
 */

	add_opt(lcp_imp_opts, AuthenticationType, 0, NULL, auth_type) |

/*
 * MagicNumber handler is magic_number()
 *
 * [RFC1331 page 53]
 */

	add_opt(lcp_imp_opts, MagicNumber, 0, NULL, magic_number) |

/*
 * Link Quality handler is link_qual()
 *
 * [RFC1331 page 49]
 */

	add_opt(lcp_imp_opts, LinkQualityMon, 0, NULL, link_qual);

	return (error);
}


/*
 * ppp_set_auth()
 *
 * Set the authentication that will be used on the link.
 *
 * Returns 0 on success, nonzero on error
 */
int
ppp_set_auth(machp, conf)
	register pppMachine_t	*machp;
	pppAuthControlEntry_t *conf;
{
	chap_op_t			chap_auth, tempchap;
	u_short				temps;
	int				rc;

	if (machp -> linkp -> conf.pppLinkVersion == pppVer1) {
		if (conf -> pppAuthTypeRemote == pppAuthPAP) {
			temps = htons(pppAuthPAP);
			rc = add_opt(machp -> inbound, AuthenticationType,
				4, (caddr_t)&temps, NULL);
			if (rc)
				return (rc);
		}
		if (conf -> pppAuthTypeLocal == pppAuthPAP) {
			machp -> linkp -> conf.pppLocAuthok = DO_PAP;
		}
	} else {
		if (conf -> pppAuthTypeRemote & DO_PAP) {
			temps = htons(pppAuthPAP);
			rc = add_opt(machp -> inbound, AuthenticationType,
				4, (caddr_t)&temps, NULL);
			if (rc)
				return (rc);
		}

		if (conf -> pppAuthTypeRemote & DO_CHAP) {
			chap_auth.prot = pppCHAP;
			chap_auth.algorithm = MD5;
			tempchap = hton_chap_op_t(chap_auth);
			rc = add_opt(machp->inbound, AuthenticationType, 5,
			    (caddr_t)&tempchap, inauth_type);

			if (rc)
				return (rc);
		}
		machp -> linkp -> conf.pppLocAuthok = conf-> pppAuthTypeLocal;
	}
	return (0);
}


/*
 * ppp_get_conf_lcp()
 *
 * Get the lcp configuration information.
 *
 * Returns 0
 */
int
ppp_get_conf_lcp(machp, conf)
	register pppMachine_t	*machp;
	pppLinkControlEntry_t *conf;
{
	conf -> pppLinkAllowMRU = machp -> allowneg[MaxReceiveUnit];
	conf -> pppLinkAllowPAComp = machp -> allowneg[ProtoFieldCompress];
	conf -> pppLinkAllowACC = machp -> allowneg[AsyncControlMap];
	conf -> pppLinkAllowMagic = machp -> allowneg[MagicNumber];
	conf -> pppLinkAllowQual = machp -> allowneg[LinkQualityMon];
	conf -> pppLinkAllowAuth = machp -> allowneg[AuthenticationType];
	return (0);
}


/*
 * ppp_set_conf_lcp()
 *
 * Set the lcp layer configuration information.
 *
 * Returns 0 on success, nonzero on error.
 */
int
ppp_set_conf_lcp(machp, conf)
	register pppMachine_t	*machp;
	pppLinkControlEntry_t *conf;
{
	register int			rc;
	u_short				temps;
	u_short				mymru;
	u_int				mymap;
	u_long				templ;
	u_long				magic_num;
	lqm_op_t			lqmdata;
	lqm_op_t			templqm;

	machp -> allowneg[MaxReceiveUnit] =
	    CONV_OPT(conf -> pppLinkAllowMRU, machp, MaxReceiveUnit);
	machp -> allowneg[ProtoFieldCompress] =
	    CONV_OPT(conf -> pppLinkAllowPAComp, machp, ProtoFieldCompress);
	machp -> allowneg[AddrCtrlCompress] =
	    CONV_OPT(conf -> pppLinkAllowPAComp, machp, AddrCtrlCompress);
	machp -> allowneg[AsyncControlMap] =
	    CONV_OPT(conf -> pppLinkAllowACC, machp, AsyncControlMap);
	machp -> allowneg[MagicNumber] =
	    CONV_OPT(conf -> pppLinkAllowMagic, machp, MagicNumber);
	machp -> allowneg[LinkQualityMon] =
	    CONV_OPT(conf -> pppLinkAllowQual, machp, LinkQualityMon);
	machp -> allowneg[AuthenticationType] =
	    CONV_OPT(conf -> pppLinkAllowAuth, machp, AuthenticationType);


	mymru = conf->pppLinkLocalMRU;
	if (mymru >= PPP_MIN_MRU || mymru >= PPP_MAX_MRU) {
		temps = htons(mymru);
		rc = add_opt(machp->inbound, MaxReceiveUnit,  4,
		    (caddr_t)&temps, nak_mru);
		if (rc)
			return (rc);
	}

	if (machp-> linkp-> conf.pppLinkMediaType == pppAsync &&
	    machp -> linkp-> conf.pppLinkLocalACCMap !=
	    (mymap = conf-> pppLinkLocalACCMap)) {
		templ = htonl(mymap);
		rc = add_opt(machp->inbound, AsyncControlMap,  6,
		    (caddr_t)&templ, NULL);
		if (rc)
			return (rc);

		machp -> linkp-> conf.pppLinkLocalACCMap = mymap;
	}

	if (machp-> linkp-> conf.pppLinkMediaType == pppAsync) {
		rc = add_opt(machp->inbound, ProtoFieldCompress,  2,
					NULL, NULL);
		if (rc)
			return (rc);
	}

	if (machp-> linkp-> conf.pppLinkMediaType == pppAsync) {
		rc = add_opt(machp->inbound, AddrCtrlCompress,	2,
						NULL, NULL);
		if (rc)
			return (rc);
	}

	lqmdata.proto = pppLQM_REPORT;
	lqmdata.rep_period = 3000;

	templqm = hton_lqm_op_t(lqmdata);
	rc = add_opt(machp -> inbound, LinkQualityMon, 8,
		(caddr_t)&templqm, NULL);

	if (rc)
		return (rc);

	magic_num = ppp_rand();

	machp -> linkp -> conf.pppLinkREQMagicNumber = magic_num;

	templ = htonl(magic_num);
	rc = add_opt(machp -> inbound, MagicNumber, 6,
	    (caddr_t)&templ, in_magic_number);

	if (rc)
		return (rc);

	return (0);

}


/*
 * alloc_lcp_machine()
 *
 * allocate the lcp machine structure.
 *
 * Returns a pointer to the structure on success, NULL otherwise
 */
pppMachine_t *
alloc_lcp_machine(readq, linkp)
queue_t		*readq;
pppLink_t	*linkp;

{
	register pppMachine_t	*machp;
	int			i;

	PPP_MEMDBG("alloc_machine: allocating machine type %x\n",
	    pppLCP);
	machp = kmem_zalloc(sizeof (pppMachine_t), KM_NOSLEEP);
	if (machp == NULL) {
		return (NULL);
	}

/*
 * Set initial link/network control protocol options
 */
	machp->protocol = pppLCP;

	machp->ntype = pppLCP & 0x0fff;

	machp->state = S0;	 /* Closed */

	machp->linkp = linkp;	 /* ptr to parent link */

	machp -> readq = readq;

	machp->optsz = PPP_MAX_LCP_OPTS;
	machp->imp = lcp_imp_opts;

	machp -> max_attempts = MAX_NEG_ATTEMPTS;
	machp -> attempts = MAX_NEG_ATTEMPTS;

	machp-> standard_allow[MaxReceiveUnit] = LOC_OPTIONAL | REM_OPTIONAL;
	machp-> standard_allow[ProtoFieldCompress] =
	    LOC_OPTIONAL | REM_OPTIONAL;
	machp-> standard_allow[AddrCtrlCompress] = LOC_OPTIONAL | REM_OPTIONAL;
	machp-> standard_allow[AsyncControlMap] = LOC_OPTIONAL | REM_OPTIONAL;
	machp-> standard_allow[MagicNumber] = LOC_OPTIONAL | REM_OPTIONAL;
	machp-> standard_allow[AuthenticationType] = LOC_MAND | REM_OPTIONAL;
	machp -> standard_allow[LinkQualityMon] = LOC_DISALLOW | REM_OPTIONAL;

	for (i = 0; i < PPP_MAX_LCP_OPTS; i++)	{
		machp -> allowneg[i] = machp -> standard_allow[i];
	}

	return (machp);
}

static lqm_op_t
ntoh_lqm_op_t(lqm_op_t temp)
{
	lqm_op_t	lqm;
	lqm.proto = ntohs(temp.proto);
	lqm.rep_period = ntohl(temp.rep_period);
	return (lqm);
}

static lqm_op_t
hton_lqm_op_t(lqm_op_t temp)
{
	lqm_op_t	lqm;
	lqm.proto = htons(temp.proto);
	lqm.rep_period = htonl(temp.rep_period);
	return (lqm);
}

static chap_op_t
ntoh_chap_op_t(chap_op_t cop)
{
	chap_op_t		newcop;

	newcop.prot = ntohs(cop.prot);
	newcop.algorithm = cop.algorithm;
	return (newcop);
}

static chap_op_t
hton_chap_op_t(chap_op_t cop)
{
	chap_op_t		newcop;

	newcop.prot = htons(cop.prot);
	newcop.algorithm = cop.algorithm;
	return (newcop);
}
