#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.8 95/03/14 SMI"
# get_root makefile

include ../../Makefile.cmd

SRCS		= get_root.c prt.c

OBJS		= $(SRCS:%.c=${ARCH}/%.o)

HDRS		= prt.h

INCDIR1		= .
INCDIR2		= ../../include
INCLUDES	= -I$(INCDIR1) -I$(INCDIR2)

LIBDIR1		= .
LIBDIR2		= ../../lib/${ARCH}
LIBDIRS		= -L$(LIBDIR1) -L$(LIBDIR2)

CFLAGS		+= $(DEBUG_CFLAGS) $(INCLUDES)
LINTFLAGS	+= ${INCLUDES}
LDFLAGS		+= $(DEBUG_CFLAGS) ${LIBDIRS}

LIBS =

DEPLIBS =

PROG =	get_root

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all: ${ARCH} .WAIT $(PROG)

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

$(PROG):	$(OBJS)
		$(LINK.c) -o $@ $(OBJS) $(LIBS)
		cp $@ ${ARCH}
		$(POST_PROCESS)

$(OBJS):	$(HDRS)

install:	all $(ROOTSBINPROG)

msgs:
	@true

sccs:
	$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(HDRS)

clean:
	-rm -f core errs *.BAK ${OBJS} *.o

clobber: clean
	-rm -f ${PROG}
	-rm -rf ${ARCHSUPPORT}

lint:		$(SRCS) $(HDRS)
		$(LINT.c) $(SRCS)

cstyle:		$(SRCS) $(HDRS)
		cstsyle $(SRCS) $(HDRS)

include ../../Makefile.targ

.WAIT:
