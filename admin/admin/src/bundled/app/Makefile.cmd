#
# Real Makefile SID -- change $-signs to %-signs in the line below
# to use this as a real Makefile, delete the space between '#' and
# ident, and put a space between '#' and ident for the template SID.
#
#ident @(#)Makefile.cmd   1.10     96/02/13
#

include $(SRC)/Makefile.master

FILEMODE = 0755

ROOTINSTALLD	= $(ROOT)/usr/sbin/install.d
ROOTRESD	= $(ROOT)/usr/openwin/lib/locale/C/app-defaults
ROOTHELPDIR	= $(ROOT)/usr/openwin/lib/locale/C/help/install.help
ROOTHDIR	= ${ROOTHELPDIR}/${HELPTYPE}
ROOTGUIHELPDIR	= $(ROOT)/usr/openwin/lib/locale/C/help/installtool.help
ROOTGUIHDIR	= ${ROOTGUIHELPDIR}/${HELPTYPE}
ROOTAI		= $(ROOT)/Misc/jumpstart_sample
ROOTAI86	= $(ROOT)/Misc/jumpstart_sample/x86-begin.conf
ROOTTOOLS	= $(ROOT)/Tools
ROOTUSRMENU	= $(ROOT)/usr/lib/locale/C/LC_MESSAGES
ROOTETC		= $(ROOT)/etc
ROOTINITD	= $(ROOT)/etc/init.d
ROOTCDBUILD	= $(ROOT)/cdbuild
ROOTADMINBIN	= $(ROOT)/usr/snadm/bin
ROOTVARSADM	= $(ROOT)/var/sadm
ROOTSBIN	= $(ROOT)/sbin
ROOTUSRBIN	= $(ROOT)/usr/bin

ROOTPROG	= $(PROG:%=$(ROOT)/.%)
ROOTINSTALLDPROG= $(PROG:%=$(ROOTINSTALLD)/%)
ROOTHELPFILES	= $(HELPFILES:%=${ROOTHDIR}/%)
ROOTGUIHELPFILES= $(HELPFILES:%=${ROOTGUIHDIR}/%)
ROOTAIPROG	= $(PROG:%=$(ROOTAI)/%)
ROOTAI86FILES	= $(FILES:%=$(ROOTAI86)/%)
ROOTCDBUILDPROG	= $(FILES:%=$(ROOTCDBUILD)/%)
ROOTUSRBINPROG	= $(PROG:%=$(ROOTUSRBIN)/%)

$(ROOTHELPFILES) :=     FILEMODE = 0444
$(ROOTHELPFILES) :=     OWNER = root
$(ROOTHELPFILES) :=     GROUP = bin

# QA partner support
QAPINCPATH11	= -I/net/rmtc/usr/rmtc/QApartner/qap_1.1/partner/include
QAPLIBPATH11	= -L/net/rmtc/usr/rmtc/QApartner/qap_1.1/partner/lib
QAPINCPATH20	= -I/net/rmtc/usr/rmtc/QApartner/qap_2.0/partner/include
QAPLIBPATH20	= -L/net/rmtc/usr/rmtc/QApartner/qap_2.0/partner/lib

X_CFLAGS        = -I$(OPENWINHOME)/include
MOTIF_CFLAGS    = -I$(MOTIFHOME)/include

X_LIBPATH       = -L$(OPENWINHOME)/lib
MOTIF_LIBPATH   = -L$(MOTIFHOME)/lib

RLINK_PATH	= -R/usr/snadm/lib:/usr/lib:/usr/openwin/lib:/usr/dt/lib
