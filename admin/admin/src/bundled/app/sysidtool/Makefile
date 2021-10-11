#
#
#pragma ident   "@(#)Makefile 1.65 96/08/28"
#  Copyright (c) 1991-1996 Sun Microsystems, Inc. All rights reserved.
#
# Makefile for sysidtool
#
# include global definitions
include ../Makefile.cmd
#
ROOTUSRINCL=	$(ROOT)/usr/include

TEST_PROG=			preconfig
NETINIT_PROG=		sysidnet
NISINIT_PROG=		sysidnis
SYSINIT_PROG=		sysidsys
ROOTINIT_PROG=		sysidroot
SYSIDUI_PROG=		sysid_ui
CONFIG_PROG=		sysidconfig
SYSUNCONFIG_PROG=	sys-unconfig
XMLIB=			sysidxm
XMLIB_QA=		sysidqa
TTYLIB=			sysidtty
UILIB=			sysiduilib
SYSIDLIB=		sysidlib
DERIVED_FILES=		nis_svc.h

# The location of the compile time libraries
# OPENWINHOME and MOTIFHOME are set in src/ENV or src/build.proto
# The location of the runtime libraries
OPENWINHOME_RUNTIME=	/usr/openwin
MOTIFHOME_RUNTIME=	/usr/dt

MSG_DOMAIN=	SUNW_INSTALL_SYSID
AR=		ar
LORDER=		lorder
TSORT=		tsort
SUBDIRS=	ttyhelp xmhelp
ALLSUBDIRS=	${SUBDIRS} backend_simulator


DYNLIBS =	$(TTYLIB) $(UILIB) $(SYSIDLIB) $(XMLIB)

PROTOCOL_DIR=	$(ROOT)/usr/include/rpcsvc

NETINIT_OBJS =	sysidnet.o ws_is_cons.o	$(PRECONFIG_OBJS)

PRECONFIG_OBJS =	config.o sysid_preconfig.o sysid_valid.o

NISINIT_OBJS =	sysidnis.o $(PRECONFIG_OBJS)

TESTNIS =	test_nisinit.o

SYSINIT_OBJS =	sysidsys.o $(PRECONFIG_OBJS)

ROOTINIT_OBJS =	sysidroot.o db_passwd.o $(PRECONFIG_OBJS)

SYSUNCONFIG_OBJS = sys-unconfig.o findconf.o db_passwd.o

CONFIG_OBJS = sysidconfig.o findconf.o

TEST_OBJS = $(PRECONFIG_OBJS)

PROG_OBJS = $(ROOTINIT_OBJS) $(SYSINIT_OBJS) $(NISINIT_OBJS) $(NETINIT_OBJS) \
	$(SYSUNCONFIG_OBJS) $(CONFIG_OBJS) $(PRECONFIG_OBJS)

SYSID_UI_OBJS =	sysid_ui.o

SYSIDLIB_OBJS = locale.o		\
		sys_method_wrappers.o	\
		db_method_wrappers.o	\
		get_net_if.o		\
		name_service.o		\
		state.o			\
		simhooks.o


UILIB_OBJS =	prompt.o		\
		message.o		\
		ui_display.o		\
		ui_format.o		\
		ui_host.o		\
		ui_date.o		\
		ui_memory.o		\
		ui_name_service.o	\
		ui_net.o		\
		ui_netmask.o		\
		ui_reply.o		\
		ui_locale.o		\
		ui_timezone.o		\
		ui_utils.o		\
		ui_dlib.o		

TTYLIB_OBJS =	tty_color.o		\
		tty_form.o		\
		tty_help.o		\
		tty_intro.o		\
		tty_locale.o		\
		tty_menu.o		\
		tty_password.o		\
		tty_subr.o		\
		tty_terminal.o		\
		tty_text.o		\
		tty_timezone.o		\
		tty_utils.o

XMLIB_OBJS =	xm_locale.o		\
		xm_form.o		\
		xm_help.o		\
		xm_text.o		\
		xm_subr.o		\
		xm_timezone.o		\
		xm_utils.o

TEST_SRCS=		$(TEST_OBJS:.o=.c)
NETINIT_SRCS=		$(NETINIT_OBJS:.o=.c)
NISINIT_SRCS=		$(NISINIT_OBJS:.o=.c)
SYSINIT_SRCS=		$(SYSINIT_OBJS:.o=.c)
ROOTINIT_SRCS=		$(ROOTINIT_OBJS:.o=.c)
SYSUNCONFIG_SRCS=	$(SYSUNCONFIG_OBJS:.o=.c)
CONFIG_SRCS=		$(CONFIG_OBJS:.o=.c)
UILIB_SRCS=		$(UILIB_OBJS:.o=.c)
SYSIDLIB_SRCS=		$(SYSIDLIB_OBJS:.o=.c)
PROG_SRCS=		$(PROG_OBJS:.o=.c) 
PROGALL_SRCS=		$(PROG_OBJS:.o=.c) $(SYSIDLIB_OBJS:.o=.c)
XMLIB_SRCS=		$(XMLIB_OBJS:.o=.c)
TTYLIB_SRCS=		$(TTYLIB_OBJS:.o=.c)
HEADERS=	dl_defs.h \
		message.h \
		nis_proc.h \
		nis_svc.h \
		prompt.h \
		sysid_msgs.h \
		sysid_ui.h \
		sysidtool.h \
		tty_color.h \
		tty_defs.h \
		tty_form.h \
		tty_help.h \
		tty_msgs.h \
		tty_utils.h \
		ui_form.h \
		xm_form.h \
		xm_help.h \
		xm_defs.h \
		xm_msgs.h

UIPROGS = $(NETINIT_PROG) $(NISINIT_PROG) $(SYSINIT_PROG) $(ROOTINIT_PROG)
PROGS=	$(UIPROGS) $(SYSUNCONFIG_PROG) $(SYSIDUI_PROG) \
        $(CONFIG_PROG) $(TEST_PROG)
FILES= Sysidtool

UISBINPROGS=	$(UIPROGS:%=$(ROOTUSRSBIN)/%) $(ROOTUSRSBIN)/$(CONFIG_PROG)
SUSBINPROGS=	$(SYSUNCONFIG_PROG:%=$(ROOTUSRSBIN)/%)
CUSBINPROGS=	$(CONFIG_PROG:%=$(ROOTUSRSBIN)/%)
SBINLIBS=	$(DYNLIBS:%=$(ROOTUSRSBIN)/%)
ROOTUSRSBINPROGS= $(UISBINPROGS) $(SUSBINPROGS) $(CUSBINPROGS)
ROOTUSRSBINLIBS= $(SBINLIBS)

ROOTRESDFILES= $(FILES:%=$(ROOTRESD)/%)

OBJECTS= $(NETINIT_OBJS) $(NISINIT_OBJS) $(SYSINIT_OBJS) $(ROOTINIT_OBJS) \
	$(SYSUNCONFIG_OBJS) $(UILIB_OBJS) $(SYSIDLIB_OBJS) \
	$(TTYLIB_OBJS) $(XMLIB_OBJS) $(CONFIG_OBJS)

SRCS = $(OBJECTS:.o=.c)

CLOBBERFILES=	$(PROGS) $(DYNLIBS) *.po *.mo

LOCINC = 	.
# No RELINC includes (check on admdb.h)
GENINC =	$(ROOTUSRINCL) $(ROOTINCADMIN)
RELINC =	
WSINC =		$(OPENWINHOME)/include $(MOTIFHOME)/include


OPTFLAG =
GCPPFLAGS = ${DEBUG_CPPFLAGS} -DSVR4 -DTEXT_DOMAIN=\"${MSG_DOMAIN}\" \
	$(GENINC:%=-I%)

CFLAGS +=	$(DEBUG_CFLAGS) -Xa

NCPPFLAGS=	$(OPTFLAG) ${GCPPFLAGS} -I $(PROTOCOL_DIR) 
TTYCPPFLAGS=	$(OPTFLAG) ${GCPPFLAGS}
UICPPFLAGS=	$(OPTFLAG) ${GCPPFLAGS}
SLCPPFLAGS=	$(NCPPFLAGS)
XCPPFLAGS=	$(OPTFLAG) ${GCPPFLAGS} $(WSINC:%=-I%)

WSLIBDIR= $(MOTIFHOME)/lib $(OPENWINHOME)/lib

NLDFLAGS=	-R/usr/sbin $(ROOTADMINLIB:%=-L%)
TTYLDFLAGS=	-G -h $(TTYLIB) -dy
UILDFLAGS=	-G -h $(UILIB) -R/usr/sbin -dy
SLLDFLAGS=	$(ROOTADMINLIB:%=-L%) -G -h $(SYSIDLIB) \
		-R/usr/snadm/lib -dy
XLDFLAGS=	$(ROOTADMINLIB:%=-L%) $(WSLIBDIR:%=-L%) -G -h $(XMLIB) \
		-R$(MOTIFHOME_RUNTIME)/lib:$(OPENWINHOME_RUNTIME)/lib -dy

LINTFLAGS=	-ux
NATIVECC=	$(CC)

# `Cause it blows up with $(TARGET_ARCH)
LINT.c=		$(LINT) $(LINTFLAGS) $(CPPFLAGS)

XMLIBS=		-lintl -lXm -lXt -lX11 -lgen
TTYLIBS=	-ltermlib
UILIBS=		

$(PROGS)   :=   CPPFLAGS = $(NCPPFLAGS)
$(PROGS)   :=   LDFLAGS = $(NLDFLAGS)

$(SYSIDUI_PROG) := 	OBJS = $(SYSID_UI_OBJS) $(SYSIDLIB) $(UILIB)
$(SYSIDUI_PROG) := 	LDLIBS = -lintl

$(NETINIT_PROG) :=	OBJS = $(NETINIT_OBJS) $(SYSIDLIB) $(UILIB)
$(NETINIT_PROG) :=	LDLIBS = -lintl

$(NISINIT_PROG) :=	OBJS = $(NISINIT_OBJS) $(SYSIDLIB) $(UILIB)
$(NISINIT_PROG) :=	LDLIBS = -lsocket -lnsl -lintl

$(SYSINIT_PROG) :=	OBJS = $(SYSINIT_OBJS) $(SYSIDLIB) $(UILIB)
$(SYSINIT_PROG) :=   	LDLIBS = -lsocket -lnsl -lintl

$(ROOTINIT_PROG) :=	OBJS = $(ROOTINIT_OBJS) $(SYSIDLIB) $(UILIB)
$(ROOTINIT_PROG) :=  	LDLIBS = -R/usr/snadm/lib -ladmldb -lnsl -lintl

$(SYSUNCONFIG_PROG) :=	OBJS = $(SYSUNCONFIG_OBJS) $(SYSIDLIB) $(UILIB)
$(SYSUNCONFIG_PROG) :=	LDLIBS = -R/usr/snadm/lib -lintl -ladmldb -ladmutil

$(TEST_PROG) :=	OBJS = $(TEST_OBJS) $(SYSIDLIB) $(UILIB) 
$(TEST_PROG) := CPPFLAGS = -DTEST_PROG $(GCPPFLAGS)

$(CONFIG_PROG) :=	OBJS = $(CONFIG_OBJS) $(SYSIDLIB) $(UILIB)
$(CONFIG_PROG) :=	LDLIBS = -lintl -lgen

$(XMLIB)     :=  CPPFLAGS = $(XCPPFLAGS)
$(XMLIB)     :=  LDFLAGS = $(XLDFLAGS)
$(XMLIB)     :=  CFLAGS += -K pic
$(XMLIB)     :=  OBJS = $(XMLIB_OBJS)
$(XMLIB)     :=  LDLIBS = $(XMLIBS)
lint_XMLIB   :=  CPPFLAGS = $(XCPPFLAGS)
lint_XMLIB   :=  LINTFLAGS = -muxy

$(XMLIB_QA)     :=  CPPFLAGS = -DQA_PARTNER $(XCPPFLAGS)
$(XMLIB_QA)     :=  LDFLAGS = $(XLDFLAGS) -L.
$(XMLIB_QA)     :=  CFLAGS += -K pic
$(XMLIB_QA)     :=  OBJS = $(XMLIB_OBJS)
$(XMLIB_QA)     :=  LDLIBS = $(XMLIBS) -lpartner

$(TTYLIB)     :=  CPPFLAGS = $(TTYCPPFLAGS)
$(TTYLIB)     :=  LDFLAGS = $(TTYLDFLAGS)
$(TTYLIB)     :=  CFLAGS += -K pic
$(TTYLIB)     :=  OBJS = $(TTYLIB_OBJS)
$(TTYLIB)     :=  LDLIBS = $(TTYLIBS)
lint_TTYLIB   :=  CPPFLAGS = $(TTYCPPFLAGS)
lint_TTYLIB   :=  LINTFLAGS = -muxy

$(UILIB)     :=  CPPFLAGS = $(UICPPFLAGS)
$(UILIB)     :=  LDFLAGS = $(UILDFLAGS)
$(UILIB)     :=  CFLAGS += -K pic
$(UILIB)     :=  OBJS = $(UILIB_OBJS)
$(UILIB)     :=  LDLIBS = $(UILIBS)
lint_UILIB   :=  CPPFLAGS = $(UICPPFLAGS)
lint_UILIB   :=  LINTFLAGS = -muxy

$(SYSIDLIB)     :=  CPPFLAGS = $(SLCPPFLAGS)
$(SYSIDLIB)     :=  LDFLAGS = $(SLLDFLAGS)
$(SYSIDLIB)     :=  CFLAGS += -K pic
$(SYSIDLIB)     :=  OBJS = $(SYSIDLIB_OBJS)
$(SYSIDLIB)     :=  LDLIBS = -ladmutil \
			 -lsocket -lnsl -lrpcsvc -lkvm -lelf -lgen -ldl -lw -lmp
lint_SYSIDLIB   :=  CPPFLAGS = $(SLCPPFLAGS)
lint_SYSIDLIB   :=  LINTFLAGS = -muxy
lint_PROG   	:=  CPPFLAGS = $(NCPPFLAGS)
lint_PROG   	:=  LINTFLAGS = -muxy
lint_PROGALL 	:=  CPPFLAGS = $(NCPPFLAGS) $(SLCPPFLAGS)
lint_PROGALL	:=  LINTFLAGS = -muxy

$(UISBINPROGS) :=	FILEMODE = 0711
$(UISBINPROGS) :=	OWNER = root
$(UISBINPROGS) :=	GROUP = sys

$(SUSBINPROGS) :=	FILEMODE=755
$(SUSBINPROGS) :=	OWNER = bin
$(SUSBINPROGS) :=	GROUP = bin

$(PSBINPROGS) :=	FILEMODE=755
$(PSBINPROGS) :=	OWNER = bin
$(PSBINPROGS) :=	GROUP = bin

$(SBINLIBS)    := 	FILEMODE=755
$(SBINLIBS)    :=	OWNER = root
$(SBINLIBS)    :=	GROUP = sys

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

.KEEP_STATE:

all:	$(DYNLIBS) $(PROGS)

backsim: 
	cd backend_simulator;  pwd; $(MAKE) all

backsimclean: 
	cd backend_simulator;  pwd; $(MAKE) clean

$(PROGS):	$$(OBJS) 
	$(PURIFY) $(LINK.c) -o $@ $(OBJS) $(LDLIBS)
	$(POST_PROCESS)

$(DYNLIBS):	$$(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LDLIBS)
	$(POST_PROCESS)

$(XMLIB_QA):	$$(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LDLIBS) -L. -lpartner
	$(POST_PROCESS)

tst_locale:	locale.c
	$(COMPILE.c) -o tst_locale.o -DTEST $(SLCPPFLAGS) locale.c
	$(LINK.c) -o $@ -lintl tst_locale.o

install: all $(ROOTUSRSBINPROGS) $(ROOTUSRSBINLIBS) $(ROOTSBINPROG) \
	$(ROOTRESDFILES) msgs $(INSTMSGS) ${SUBDIRS}

clobber: clean 
	$(RM) $(PROG) $(CLOBBERFILES)

clean:
	$(RM) $(OBJECTS) core

lint_PROG: ${PROG_SRCS}
	$(LINT.c) $(PROG_SRCS)

lint: lint_PROGALL

lint_PROGALL: $(PROGALL_SRCS)
	$(LINT.c) $(PROGALL_SRCS)

lint_LIBS: lint_UILIB lint_SYSIDLIB lint_TTYLIB lint_XMLIB

lint_UILIB:   $(UILIB_SRCS)
	$(LINT.c) $(UILIB_SRCS)

lint_SYSIDLIB:        $(SYSIDLIB_SRCS)
	$(LINT.c) $(SYSIDLIB_SRCS)

lint_TTYLIB:  $(TTYLIB_SRCS)
	$(LINT.c) $(TTYLIB_SRCS)

lint_XMLIB:  $(XMLIB_SRCS)
	$(LINT.c) $(XMLIB_SRCS)

name_service.o: $(DERIVED_FILES) name_service.c

nis_svc.h:  $(PROTOCOL_DIR)/nis.x $(PROTOCOL_DIR)/nis_object.x
	rpcgen -C -h $(PROTOCOL_DIR)/nis.x |\
	sed -n -e '/EDIT_START/,$$ p' |\
	sed -e 's/_2_svc/_svc/' |\
	sed -e 's/_3_svc/_svc/' |\
	sed -e 's/_3/_clnt/' |\
	sed -e 's/_2/_clnt/' >nis_svc.h

msgs: $(MSG_DOMAIN).po

$(MSG_DOMAIN).po:	$(SRCS) $(HEADERS)
	@for i in $(SRCS) $(HEADERS); do \
		echo "xgettext $$i"; \
		sed 's/#define//' <$$i >messages.sed; \
		xgettext -j -c "i18n:" -m "" messages.sed; \
	done
	@cat $(MSG_DOMAIN).po messages.po | \
	    sed -e "/^# msg/d" \
		-e "/^domain[ 	][ 	]*\"messages\"/d" \
		-e "s?^#[ 	][ 	]*[/ ]\*/*?#?" \
		-e "s?[ 	][ 	]*\*/??" \
		> $(MSG_DOMAIN).po.sed
	@mv $(MSG_DOMAIN).po.sed $(MSG_DOMAIN).po
	@rm -f messages.po messages.sed

simulator: FRC
	cd backend_simulator; pwd; $(MAKE) all

$(ALLSUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

include ../Makefile.targ
