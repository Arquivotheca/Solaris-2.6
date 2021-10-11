#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.12 95/12/12 SMI"
# rootdev makefile

include ../../Makefile.cmd

PROG		= rootdev

SRCS		= rootdev.c

OBJS		=${SRCS:%.c=%.o}

LIBDIR		= $(ROOTADMINLIB)
LIBDIRS		= -L${LIBDIR}
LIBDEPS		= $(LIBDIR)/libspmistore.a $(LIBDIR)/libspmicommon.a
LIBS		= -lspmistore -lspmicommon

INCLUDES	= -I$(ROOTINCADMIN)

CFLAGS		+= ${DEBUG_CFLAGS} -Xa
CPPFLAGS	= ${INCLUDES}
LDLIBS		= $(LIBDIRS) -R$(ROOTADMINLIB:$(ROOT)%=%) $(LIBS) \
		  -ldl -ldevinfo -lnsl -lintl -lgen -ladm

LINTERR		= lint_errors

LINTFILES	=${SRCS:%.c=${ARCH}/%.ln}

.KEEP_STATE:

all:	${PROG} 
	@true

${PROG}: ${OBJS} ${LIBDEPS}
	${LINK.c} -o $@ ${OBJS} ${LDLIBS}
	$(POST_PROCESS)

msgs msgs_test:
	@true

lint:
	lint -x ${INCLUDES} ${SRCS}

install:	all $(ROOTINSTALLDPROG)

cstyle:
	cstyle ${SRCS}

clean_msgs:
	-rm -f *.i *.po *.mo

clean: clean_msgs
	-rm -f core errs *.o *.BAK ${OBJS}

clobber: clean
	-rm -f ${PROG}
	-rm -rf ${ARCHSUPPORT}

include ../../Makefile.targ
