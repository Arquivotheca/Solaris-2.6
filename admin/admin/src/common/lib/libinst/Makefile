#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident	"@(#)Makefile 1.89 95/10/31"
# install library makefile

LIBRARY	= libinstall.a
VERS	= .2

OBJECTS	= \
	disk.o		\
	disk_check.o	\
	disk_debug.o	\
	disk_dfltmnt.o	\
	disk_dflt.o	\
	disk_fdisk.o	\
	disk_filesys.o	\
	disk_find.o	\
	disk_load.o	\
	disk_profile.o	\
	disk_prom.o	\
	disk_sdisk.o	\
	disk_upg.o	\
	disk_util.o	\
	ibe_fileio.o	\
	ibe_mount.o	\
	ibe_print.o	\
	ibe_sm.o	\
	ibe_util.o	\
	install_prod.o	\
	install_setup.o	\
	setser.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= disk_lib.h ibe_lib.h disk_strings.h ibe_strings.h
EXPHDRS		= disk_api.h ibe_api.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

INCLUDE		= -I. -I../libsw
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xc ${CPPFLAGS}
LDFLAGS		+= -ldevinfo

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= disk_test prom_test *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIB

.KEEP_STATE:

all: $(HDRS) .WAIT static dynamic
	@true

static: $(LIBS)

dynamic: $(DYNLIB) .WAIT $(DYNLIBLINK)

install:	all .WAIT \
		$(ROOTADMINLIB) $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK) \
		msgs $(INSTMSGS)

install_test:	all .WAIT \
		$(ROOTADMINLIB) $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) $(INSTMSGS)

install_h:	$(ROOTUSRINCLEXP)

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

lint:  ${SRCS} ${HDRS}
	${LINT.c} ${SRCS}

cstyle:	$(SRCS) $(PRIVHDRS) $(PUBHDRS)
	cstyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

disk_test: objs/$(ARCH)/disk_test.o objs/$(ARCH)/$(LIBRARY)
	cc -g -o $@ -u print_disk objs/$(ARCH)/disk_test.o \
	    objs/$(ARCH)/$(LIBRARY) -ldl -ldevinfo -lnsl -ladm \
	    -lintl ../libsw/objs/$(ARCH)/libsw.a

disk_test.o: ${HDRS}

prom_test: disk_prom.c
	cc -g -c -DTEST disk_prom.c -I. -I../libsw 
	cc -g -o $@ disk_prom.o \
	    objs/$(ARCH)/$(LIBRARY) -ldl -ldevinfo -lnsl -ladm \
	    -lintl ../libsw/objs/$(ARCH)/libsw.a

include ../Makefile.targ
