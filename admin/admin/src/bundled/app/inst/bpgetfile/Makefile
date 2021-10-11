#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident	"@(#)Makefile	1.14	95/03/14 SMI"
# bpgetfile makefile

include ../../Makefile.cmd

PROG =		bpgetfile

SRCS =		bpgetfile.c

OBJS = $(SRCS:%.c=${ARCH}/%.o)

TXTS =

CFLAGS += $(DEBUG_CFLAGS)

#LDLIBS = $(NEEDU) -Bdynamic -ldl -Bstatic -lsocket -lnsl -lintl -lrpcsvc -lc
#LDFLAGS += -dy
LDLIBS = -lsocket -lnsl -lintl -lrpcsvc
LDFLAGS +=
#
# We need to make these become externally available so
# that tcpip.so (ps) can find them at run time.
NEEDU=		-u t_optmgmt \
		-u sscanf \
		-u inet_netof \
		-u strpbrk \
		-u yp_get_default_domain \
		-u yp_first \
		-u strrchr \
		-u strncmp \
		-u strtok \
		-u t_alloc \
		-u memset \
		-u t_free \
		-u t_unbind

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all: $(ARCH) $(PROG) $(TXTS)

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

$(PROG):	$(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LDLIBS)
	cp $@ ${ARCH}
	$(POST_PROCESS)

install: all $(ROOTUSRSBINPROG)

lint:		lint_SRCS

clean:
	-rm -f core errs *.BAK ${OBJS} *.o

clobber: clean
	-rm -f ${PROG}
	-rm -rf ${ARCHSUPPORT}

include ../../Makefile.targ
