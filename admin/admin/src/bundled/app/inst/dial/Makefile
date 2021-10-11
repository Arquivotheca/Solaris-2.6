# Makefile for ttinstall 
#
#ident      "@(#)Makefile 1.12 94/11/14 SMI"
#

include ../../Makefile.cmd

SRCS= dial.c

CFLAGS	+= $(DEBUG_CFLAGS) -Xa
LDFLAGS	+= $(DEBUG_CFLAGS)

PROG=dial

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

include ../../Makefile.targ

.WAIT:
