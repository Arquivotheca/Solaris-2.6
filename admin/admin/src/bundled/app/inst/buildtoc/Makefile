#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident @(#)Makefile	1.30	96/02/28
# buildtoc makefile

include ../../Makefile.cmd

SRCS		= buildtoc.c

OBJS		= $(SRCS:%.c=${ARCH}/%.o)

HDRS		=

MAN			= buildtoc.4

LIBDIR1		= $(ROOTADMINLIB)
LIBDIRS		= -L$(LIBDIR1)

INCDIR1		= $(ROOTINCADMIN)
INCLUDES	= -I$(INCDIR1)

LIBDEPS		= $(LIBDIR1)/libspmisoft.a \
		  $(LIBDIR1)/libspmicommon.a

CFLAGS		+= $(DEBUG_CFLAGS)
CPPFLAGS	= -Xa ${INCLUDES}
LINTFLAGS	= -umx
LDFLAGS		+= -Xa $(DEBUG_CFLAGS) ${LIBDIRS}

LIBS = 		-Bstatic -lspmisoft -lspmicommon -Bdynamic -lintl \
		-ladm -ldl -lnsl

PROG =	buildtoc

FILES = ${PROG} ${MAN}

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all: ${ARCH} ${DESTDIR} .WAIT $(HDRS) $(PROG) $(MAN)
	cp $(PROG) $(ARCH)

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

$(PROG):	$(OBJS) $(LIBDEPS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) 

install: all $(ROOTCDBUILDPROG)

msgs:
	@true

clean:
		$(RM) $(OBJS) *~ *.out Makefile.bak *.o

clobber: clean
		$(RM) $(PROG)
		rm -rf ${ARCHSUPPORT}

lint:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		$(LINT.c) $(SRCS)

cstyle:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		cstsyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

include ../../Makefile.targ

.WAIT:
