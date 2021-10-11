#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.66 96/05/14 SMI"
# pfinstall makefile

include ../../Makefile.cmd

SRCS =	pf_disk.c \
	pf_fdisk.c \
	pf_main.c \
	pf_parse.c \
	pf_software.c \
	pf_util.c

OBJS =	${SRCS:%.c=${ARCH}/%.o}

HDRS =	profile.h pf_strings.h

LIBDIR  = $(ROOTADMINLIB)
LIBDIRS = -L${LIBDIR}

INCLUDEDIR = -I. -I$(ROOTINCADMIN)

CPPFLAGS  += $(INCLUDEDIR)
CFLAGS	  += $(DEBUG_CFLAGS) -Xa
LINTFLAGS  = -umx ${CPPFLAGS}

LDFLAGS	 += $(DEBUG_CFLAGS) -R$(ROOTADMINLIB:$(ROOT)%=%) $(LIBDIRS)
APPLIBS  += -lspmiapp -lspmisvc -lspmistore -lspmisoft -lspmicommon
LIBS	 += -ldl -ldevinfo -lnsl -ladm
LDLIBS	 += ${APPLIBS} ${LIBS}
STATICLDLIBS= -Bstatic ${APPLIBS} -Bdynamic ${LIBS}

LIBDEP	  = \
	$(LIBDIR)/libspmiapp.so \
	$(LIBDIR)/libspmisvc.so \
	$(LIBDIR)/libspmistore.so \
	$(LIBDIR)/libspmisoft.so \
	$(LIBDIR)/libspmicommon.so
STATICLIBDEP = \
	$(LIBDIR)/libspmiapp.a \
	$(LIBDIR)/libspmisvc.a \
	$(LIBDIR)/libspmistore.a \
	$(LIBDIR)/libspmisoft.a \
	$(LIBDIR)/libspmicommon.a

MSG_DOMAIN = SUNW_INSTALL_PFINSTALL

PROG	   = pfinstall

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE: 

all: ${ARCH} .WAIT $(PROG)
	@true

${ARCH}:
	@[ -d ${@} ] || (${RM} ${@} ;mkdir -p ${@})

$(PROG):  $(OBJS) $(HDRS) .WAIT $(LIBDEP)
	$(LINK.c) -o $@ $(OBJS) $(LDLIBS)
	cp $@ ${ARCH}
	$(POST_PROCESS)

$(PROG).static:	$(OBJS) $(HDRS) .WAIT $(STATICLIBDEP)
	$(LINK.c) -o $@ $(OBJS) $(STATICLDLIBS)
	cp $@ ${ARCH}
	$(POST_PROCESS)

${OBJS}: $(HDRS)

lint: ${SRCS} ${HDRS}
	${LINT.c} ${SRCS}

install:  all $(ROOTINSTALLDPROG) msgs $(INSTMSGS)

msgs: ${MSG_DOMAIN}.po

${MSG_DOMAIN}.po: ${SRCS} ${HDRS}
	@echo "Making messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} -s \
			-c "i18n:" - >/dev/null 2>&1

# Create a message file with "translated" test strings.
# Turn the po file into an mo file ready for testing.
msgs_test: ${SRCS} ${HDRS}
	@echo "Making test messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} -s \
			-c "i18n:" -m "öýê!" - >/dev/null 2>&1
	@msgfmt -o ${MSG_DOMAIN}.mo ${MSG_DOMAIN}.po

clean_msgs:
	-${RM} *.po *.mo

clean:
	-${RM} core errs *.BAK ${OBJS} *.o
	cd tests; pwd; $(MAKE) $@

clobber: clean clean_msgs
	-${RM} ${PROG} ${PROG}.static
	-${RM} -r ${ARCHSUPPORT}
	cd tests; pwd; $(MAKE) $@

include ../../Makefile.targ

.WAIT:
