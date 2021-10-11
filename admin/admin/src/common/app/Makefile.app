#
#ident	"@(#)Makefile.app	1.18	95/01/19 SMI"
#
# Copyright (c) 1992-1994 by Sun Microsystems, Inc.
#
#	app/Makefile.app
#	Definitions common to command source.
#
# include global definitions; SRC should be defined in the shell.
include $(SRC)/Makefile.master

FILEMODE=	2555
LIBFILEMODE=	0444
ROOTBIN=	$(ROOT)/usr/bin
ROOTLIB=	$(ROOTUSRLIB)
ROOTSBIN=	$(ROOT)/sbin
ROOTADMINBIN = 	$(ROOTADMIN)/bin
ROOTAPPDEFAULTS = $(ROOT)/usr/dt/lib/app-defaults

ROOTPROG=	$(PROG:%=$(ROOTBIN)/%)
ROOTSHFILES=	$(SHFILES:%=$(ROOTBIN)/%)
ROOTLIBPROG=	$(PROG:%=$(ROOTLIB)/%)
ROOTSBINPROG=	$(PROG:%=$(ROOTSBIN)/%)
ROOTUSRSBINPROG=$(PROG:%=$(ROOTUSRSBIN)/%)
ROOTETCPROG=	$(PROG:%=$(ROOTETC)/%)
ROOTADMINBINPROG = $(PROG:%=$(ROOTADMINBIN)/%)
ROOTAPPDEFAULTSPROG = $(PROG:%=$(ROOTAPPDEFAULTS)/%)

# QA partner support
QAPINCPATH11	= -I/net/rmtc/usr/rmtc/QApartner/qap_1.1/partner/include
QAPLIBPATH11	= -L/net/rmtc/usr/rmtc/QApartner/qap_1.1/partner/lib
QAPINCPATH20	= -I/net/rmtc/usr/rmtc/QApartner/qap_2.0/partner/include
QAPLIBPATH20	= -L/net/rmtc/usr/rmtc/QApartner/qap_2.0/partner/lib

X_CFLAGS        = -I$(OPENWINHOME)/include
MOTIF_CFLAGS    = -I$(MOTIFHOME)/include
NIHINC          = -I$(ROOT)/usr/include/nihcl
ADMININC        = -I../../lib/libadmobjs
SNAGINC		= -I$(ROOT)/usr/include/admin

X_LIBPATH       = -L$(OPENWINHOME)/lib
MOTIF_LIBPATH   = -L$(MOTIFHOME)/lib
NIHLIB          = -L$(ROOT)/usr/lib
ADMLIB          = -L$(ROOT)/usr/lib
SNAGLIB         = -L$(ROOT)/usr/snadm/classes/lib

RLINK_PATH	= -R/usr/snadm/lib:/usr/lib:/usr/openwin/lib:/usr/dt/lib

MOTIFLIB_NAME	= Xm

$(ROOTBIN)/%: %
	$(INS.file)

$(ROOTLIB)/%: %
	$(INS.file)

$(ROOTSBIN)/%: %
	$(INS.file)

$(ROOTUSRSBIN)/%: %
	$(INS.file)

$(ROOTETC)/%: %
	$(INS.file)

$(ROOTADMINBIN)/%: %
	$(INS.file)

$(ROOTADMINETC)/%: %
	$(INS.file)

$(ROOTAPPDEFAULTS)/%: %
	$(INS.file)
