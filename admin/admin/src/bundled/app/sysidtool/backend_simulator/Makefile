#}
#
#ident "@(#)Makefile 1.4	94/12/06 SMI"
#
# Makefile for sysidtool backend simulator
#
# include global definitions
include ../../Makefile.cmd
#
ROOTUSRINCL=	$(ROOT)/usr/include
ROOTUSRINCLADM=	$(ROOT)/usr/include/admin

SIMULATOR_LIB=		libsysidsim.so
SIMPARSETEST_PROG=	simparse_test
SIMPARSETEST_PROG_ND=	simparse_test_nodyn
SIMNSTEST_PROG=		simns_test
SIMNETTEST_PROG=	simnet_test
SIMHOSTTEST_PROG=	simhost_test
SIMGLUE_OBJ=		simglue.o
OPENWINHOME=		/usr/openwin
MOTIFHOME=		/opt/dt
DERIVED_FILES=		y.tab.c lex.yy.c
LIBCCDIR=		/usr/dist/local/share/sunpro,v2.0.1/sol-2.${ARCH}/lib

AR=		ar
LORDER=		lorder
TSORT=		tsort

SIMULATOR_OBJS= y.tab.o simhost.o simnet.o simns.o simstate.o ${SIMGLUE_OBJ}
SIMPARSETEST_OBJS= simparse_test.o
SIMNSTEST_OBJS = simns_test.o
SIMNETTEST_OBJS = simnet_test.o
SIMHOSTTEST_OBJS = simhost_test.o

SIMULATOR_SRCS=		$(NETINIT_OBJS:.o=.c)
HEADERS=	

DYNLIBS = ${SIMULATOR_LIB}
TESTPROGS = $(SIMPARSETEST_PROG) $(SIMNSTEST_PROG) \
    $(SIMNETTEST_PROG) $(SIMHOSTTEST_PROG)


#
OBJECTS=$(SIMULATOR_OBJS) $(SIMPARSETEST_OBJS) $(SIMNSTEST_OBJS) \
	$(SIMNETTEST_OBJS) $(SIMHOSTTEST_OBJS)
SRCS = $(OBJECTS:.o=.c)

CLOBBERFILES=	$(PROGS) $(TESTPROGS) $(DYNLIBS)

# No RELINC includes (check on admdb.h)
WSINC =		$(OPENWINHOME)/include $(MOTIFHOME)/include
GENINC = . ..
GLUEINC = $(GENINC) $(ROOTUSRINCL) $(ROOTUSRINCLADM)


OPTFLAG =
GCPPFLAGS = ${DEBUG_CFLAGS} -DSVR4 -DTEXT_DOMAIN=\"${TEXTDOMAIN}\" \
	$(GENINC:%=-I%)

GLUECPPFLAGS = ${DEBUG_CFLAGS} -DSVR4 -DTEXT_DOMAIN=\"${TEXTDOMAIN}\" \
	$(GLUEINC:%=-I%)

GLDFLAGS = -R . -L . 
CFLAGS +=	$(DEBUG_CFLAGS) -Xa
CCFLAGS +=	$(DEBUG_CCFLAGS)

LCPPFLAGS=	$(OPTFLAG) ${GCPPFLAGS}

LLDFLAGS=	-G -h ${SIMULATOR_LIB} -L${LIBCCDIR}

LINTFLAGS=	-ux
NATIVECC=	$(CC)

# `Cause it blows up with $(TARGET_ARCH)
LINT.c=		$(LINT) $(LINTFLAGS) $(CPPFLAGS)

LLIBS=		

$(TESTPROGS)   :=   CPPFLAGS = $(GCPPFLAGS)
$(TESTPROGS)   :=   LDFLAGS = $(GLDFLAGS)
$(TESTPROGS)   :=   LDLIBS = -lsysidsim
$(SIMPARSETEST_PROG) := OBJS = $(SIMPARSETEST_OBJS)
$(SIMPARSETEST_PROG_ND) := OBJS = $(SIMPARSETEST_OBJS) $(SIMULATOR_OBJS)
$(SIMNETTEST_PROG)   := OBJS = $(SIMNETTEST_OBJS)
$(SIMNSTEST_PROG)    := OBJS = $(SIMNSTEST_OBJS)
$(SIMHOSTTEST_PROG)  := OBJS = $(SIMHOSTTEST_OBJS)

$(SIMULATOR_LIB)     :=  CPPFLAGS = $(LCPPFLAGS)
$(SIMULATOR_LIB)     :=  LDFLAGS = $(LLDFLAGS)
$(SIMULATOR_LIB)     :=  CFLAGS += -K pic
$(SIMULATOR_LIB)     :=  CCFLAGS += -pic
$(SIMULATOR_LIB)     :=  OBJS = $(SIMULATOR_OBJS)
$(SIMULATOR_LIB)     :=  LDLIBS = -lC -lnsl -ldl
$(SIMGLUE_OBJ)       :=  CPPFLAGS = $(GLUECPPFLAGS)
lint_SIMULATOR_LIB   :=  CPPFLAGS = $(LCPPFLAGS)
lint_SIMULATOR_LIB   :=  LINTFLAGS = -muxy


all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

.KEEP_STATE:

all:	$(DYNLIBS) 

testprogs: $(DYNLIBS) $(TESTPROGS)

$(TESTPROGS):	$$(OBJS) 
	$(LINK.cc) -o $@ $(OBJS) $(LDLIBS)
	$(POST_PROCESS)

$(SIMPARSETEST_PROG_ND): $$(OBJS)
	$(LINK.cc) -o $@ $(OBJS) $(LDLIBS)

$(DYNLIBS):	$$(OBJS)
	$(LINK.cc) -o $@ $(OBJS) $(LDLIBS)
	$(POST_PROCESS)

install: all 
	echo "Nothing to install here"

clobber: clean 
	$(RM) $(TESTPROGS) $(CLOBBERFILES)

clean: 
	$(RM) $(OBJECTS) ${DERIVED_FILES}

lint_PROG:
	$(LINT.c) $(PROG).c

lint_LIBS: lint_UILIB lint_SYSIDLIB lint_TTYLIB

lint_SIMULATORLIB:  $(SIMULATOR_SRCS)
	$(LINT.c) $(SIMULATOR_SRCS)

lex.yy.c: sim_parser.l
	${LEX} sim_parser.l

y.tab.c: sim_parser.y lex.yy.c
	$(YACC) -t sim_parser.y

y.tab.o: y.tab.c
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

FRC:
