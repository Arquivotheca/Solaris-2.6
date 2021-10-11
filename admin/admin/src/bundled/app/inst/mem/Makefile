#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.14 95/03/14 SMI"
# mem makefile

include ../../Makefile.cmd

SRCS= mem.c

CFLAGS	+= $(DEBUG_CFLAGS) -Xa
LDFLAGS	+= $(DEBUG_CFLAGS)

PROG=mem

OBJS=${SRCS:%.c=${ARCH}/%.o}

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all:	${ARCH} .WAIT ${PROG} 
	@true

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

${PROG}: ${OBJS}
	${LINK.c} -o $@ ${OBJS}
	cp $@ ${ARCH}
	$(POST_PROCESS)

install: all $(ROOTSBINPROG)

msgs msgs_test:
	@true

clean:
	-rm -f core errs *.o *.BAK ${OBJS}

clobber: clean
	-rm -f ${PROG}
	-rm -rf ${ARCHSUPPORT}

include ../../Makefile.targ

.WAIT:
