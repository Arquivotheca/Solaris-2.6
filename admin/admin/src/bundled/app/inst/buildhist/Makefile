#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident @(#)Makefile	1.25	96/02/20
# buildhist makefile

include ../../Makefile.cmd

SRCS		= buildhist.c

OBJS		= $(SRCS:%.c=${ARCH}/%.o)

HDRS		=

INCDIR1		= $(ROOTINCADMIN)
INCLUDES	= -I$(INCDIR1)

LIBDIR1		= $(ROOTADMINLIB)
LIBDIRS		= -L$(LIBDIR1)

LIBDEPS		= $(LIBDIR1)/libspmisoft.a \
		  $(LIBDIR1)/libspmicommon.a

CFLAGS		+= $(DEBUG_CFLAGS)
CPPFLAGS	+= $(INCLUDES)
LINTFLAGS	= ${INCLUDES} -umx
LDFLAGS		= $(DEBUG_CFLAGS) ${LIBDIRS}

LIBS		= -Bstatic -lspmisoft -lspmicommon \
		  -Bdynamic -lintl -lgen -ladm \
		  -ldl -lnsl


PROGRAM =	buildhist

SCRIPT	=	bldpkghist

FILES=$(PROGRAM) $(SCRIPT)

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all: ${ARCH} .WAIT $(FILES) .WAIT
	cp $(PROGRAM) $(ARCH)

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

$(PROGRAM):	$(OBJS) $(LIBDEPS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) 

install: all $(ROOTCDBUILDPROG)

msgs:

clean:
		$(RM) $(OBJS) *~ *.out Makefile.bak

clobber: clean
		$(RM) $(FILES)
		rm -rf ${ARCHSUPPORT}

lint:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		$(LINT.c) $(SRCS)

cstyle:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		cstsyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

include ../../Makefile.targ

.WAIT:
