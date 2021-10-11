#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# "@(#)Makefile 1.6 96/06/29 SMI"
#
# SPMI storage library makefile
#

LIBRARY	= libspmistore.a
VERS	= .1

OBJECTS	= \
	store_boot.o \
	store_bootobj.o \
	store_check.o \
	store_common.o \
	store_debug.o \
	store_disk.o \
	store_fdisk.o \
	store_initdisk.o \
	store_rwdisklist.o \
	store_sdisk.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= spmistore_lib.h store_strings.h
EXPHDRS		= spmistore_api.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

INCLUDE		 = -I../libspmicommon
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xa ${CPPFLAGS}
LDFLAGS		+=

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIBSTORE

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

driver:	objs/$(ARCH)/driver.o objs/$(ARCH)/$(LIBRARY)
		$(CC) -o objs/$(ARCH)/$@ -DMODULE_TEST $@.c $(INCLUDE) \
			-u print_disk \
			objs/$(ARCH)/$(LIBRARY) \
			-ldl -ldevinfo -lnsl -ladm \
			-Bstatic -lspmicommon

store_initdisk:	objs/$(ARCH)/$(LIBRARY)
		$(CC) -o objs/$(ARCH)/$@ -DMODULE_TEST $@.c $(INCLUDE) \
			-u print_disk \
			objs/$(ARCH)/store_initdisk.o \
			objs/$(ARCH)/$(LIBRARY) \
			-ldl -ldevinfo -lnsl -ladm \
			-Bstatic -lspmicommon

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

include ../Makefile.targ
