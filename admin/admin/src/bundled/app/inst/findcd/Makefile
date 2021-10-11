#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.16 95/03/14 SMI"
# findcd makefile

include ../../Makefile.cmd

PROG=findcd

SRCS= findcd.c

OBJS=${SRCS:%.c=${ARCH}/%.o}

CFLAGS	+=	$(DEBUG_CFLAGS) -Xa
LDFLAGS	+= 	$(DEBUG_CFLAGS)
LIBS= -ladm

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all:	${ARCH} .WAIT ${PROG} 
	@true

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

${PROG}: ${OBJS}
	${LINK.c} -o $@ ${OBJS} ${LIBS}
	cp $@ ${ARCH}
	$(POST_PROCESS)

install:	all $(ROOTINSTALLDPROG)

msgs:
	@true

clean:
	-rm -f core errs *.o *.BAK ${OBJS}

clobber: clean
	-rm -f ${PROG}
	-rm -rf ${ARCHSUPPORT}

include ../../Makefile.targ

.WAIT:
