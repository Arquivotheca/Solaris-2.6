# Copyright 1995, Sun Microsystems
#
# Makefile for volume management guis
#
# "@(#)Makefile	1.8	95/02/01 SMI"
#
# /master/avocet/volmgt/494_int/gui/Makefile

RM=             rm -f
ECHO=           echo
MCS=            mcs
XGETTEXT= 	xgettext
OWH:sh=		echo ${OPENWINHOME-/usr/openwin}

# Make sure we don't confuse with things in our environment
RELEASE_BUILD=
DEBUG_FLAGS =

# Define the majority text domain in this directory.
TEXT_DOMAIN= SUNWvolg
DTEXTDOM=-DTEXT_DOMAIN=\"$(TEXT_DOMAIN)\"       # For messaging.
DOMAIN_DIR = /usr/lib/locale
DDOMDIR=-DDOMAIN_DIR=\"$(DOMAIN_DIR)\"       # For messaging.

#
# Uncomment this for a release build
#
RELEASE_BUILD:sh=       /usr/bin/echo \\043


$(RELEASE_BUILD)DEBUG_FLAGS = -g -DDEBUG

CFLAGS:sh =if [ "`uname -p`" = "sparc" ]; then \
                echo "-O -xcg89 -Wa,-cg92" ; \
          else  echo "-O"; \
          fi

$(RELEASE_BUILD)CFLAGS:sh =if [ "`uname -p`" = "sparc" ]; then \
                echo "-xcg89 -Wa,-cg92" ; \
          else  echo ""; \
          fi

LDFLAGS =	-L$(OWH)/lib -s
$(RELEASE_BUILD)LDFLAGS =	-L$(OWH)/lib

CPPFLAGS = $(DTEXTDOM) $(DDOMDIR) -DOW_I18N
COMPILE.c=      $(CC) $(CFLAGS) $(CPPFLAGS) -c $(DEBUG_FLAGS)
COMPILE.cpp= 	$(CC) -E -C $(CFLAGS) $(CPPFLAGS)
LINK.c=         $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DEBUG_FLAGS)
PURIFY.c=       purify $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DEBUG_FLAGS)

#
# For source message catalogue
#

.SUFFIXES: $(SUFFIXES) .po


# 
# For non release builds we generate a bogus portable message file so
# we can easily run it through the pig latin and kanji scripts.
#

XGETFLAGS= -c TRANSLATION_NOTE -d $(TEXT_DOMAIN) -j
$(RELEASE_BUILD)XGETFLAGS= -c TRANSLATION_NOTE -m "" -d $(TEXT_DOMAIN) -j
BUILD.po= $(XGETTEXT) $(XGETFLAGS) $<.i ;\
	$(RM) $@ messages.po $<.i

# Rules (normally from make.rules) and macros which are used for post
# processing files. Normally, these do stripping of the comment section
# automatically.
#    RELEASE_CM:	Should be editted to reflect the release.
#    POST_PROCESS_O:	Post-processing for `.o' files.
#    POST_PROCESS_A:	Post-processing for `.a' files (currently null).
#    POST_PROCESS_SO:	Post-processing for `.so' files.
#    POST_PROCESS:	Post-processing for executable files (no suffix).
# Note that these macros are not completely generalized as they are to be
# used with the file name to be processed following.
#
# It is left as an exercise to Release Engineering to embellish the generation
# of the release comment string.
#
#	If this is a standard development build:
#		compress the comment section (mcs -c)
#		add the standard comment (mcs -a $(RELEASE_CM))
#		add the development specific comment (mcs -a $(DEV_CM))
#
#	If this is an installation build:
#		delete the comment section (mcs -d)
#		add the standard comment (mcs -a $(RELEASE_CM))
#		add the development specific comment (mcs -a $(DEV_CM))
#
#	If this is an release build:
#		delete the comment section (mcs -d)
#		add the standard comment (mcs -a $(RELEASE_CM))
#
#	Sigh,... The declaration POUND_SIGN is always '#'. This is just needed
#	to get around the make **bug** that '#' is always a comment delimiter,
#	even when escaped or quoted.
#
# The following list of macros are used in the definition of RELEASE_CM
# which is used to label all binaries in the build:
#
# 	RELEASE		Specific release of the build, eg: 5.2
#	VERSION		Version of the build (alpha, beta, Generic)
#	PATCHID		If this is a patch this value should contain
#			the patchid value (eg: "Generic 100832-01"), otherwise
#			it will be set to $(VERSION)
#	RELEASE_DATE	Date of the Release Build
#	PATCH_DATE	Date the patch was created, if this is blank it
#			will default to the RELEASE_DATE
#
POUND_SIGN:sh=  echo \\043
RELEASE=        5.5
VERSION=        Generic
RELEASE_DATE:sh= date "+%B %Y"
PATCH_DATE=     $(RELEASE_DATE)
PATCHID=        $(VERSION)
RELEASE_CM=     "@($(POUND_SIGN))SunOS $(RELEASE) $(PATCHID) $(PATCH_DATE)"
DEV_CM=         "@($(POUND_SIGN))SunOS Internal Development: `$(ECHO) $$LOGNAME` `date +%D` `$(ECHO) [\`basename $$CODEMGR_WS\`]`"

PROCESS_COMMENT=        @?mcs -d -a $(RELEASE_CM)
$(RELEASE_BUILD)PROCESS_COMMENT=        @?mcs -d -a $(RELEASE_CM) -a $(DEV_CM)

POST_PROCESS=           $(PROCESS_COMMENT) $@

#
# Lint flags used by volmgt
#

LINTFLAGS=	-uxn
CSTYLE=cstyle

OBJS=		volmissing_popup.o eject_popup.o 
SRCS=		$(OBJS:%.o=%.c)
POFILES=        $(OBJS:%.o=%.po)

XINCDIRS= -I$(OWH)/include

LDLIBS +=		-lvolmgt -ladm
XLDLIBS= -lxview -lolgx -lX11 -lintl -lw
RXLDLIBS= -R/usr/openwin/lib

LPROG=	volmissing_popup eject_popup

# Where the programs will live.
#
# /usr/lib/vold

.KEEP_STATE:

all: ${LPROG} ${POFILES}

volmissing_popup:	volmissing_popup.o
	${LINK.c} -o $@ volmissing_popup.o ${LDLIBS} ${RXLDLIBS} ${XLDLIBS}
#	${PURIFY.c} -o $@ volmissing_popup.o ${LDLIBS} ${RXLDLIBS} ${XLDLIBS}
	$(POST_PROCESS)

volmissing_popup.o:	volmissing_popup.h volmissing_popup.c
	${COMPILE.c} ${XINCDIRS} volmissing_popup.c
	$(POST_PROCESS)

eject_popup:	eject_popup.o
	${LINK.c} -o $@ eject_popup.o ${LDLIBS} ${RXLDLIBS} ${XLDLIBS}
	$(POST_PROCESS)

eject_popup.o:	eject_popup.c
	${COMPILE.c} ${XINCDIRS} eject_popup.c
	$(POST_PROCESS)

install: all

cstyle:
	${CSTYLE} ${SRCS}

lint:
	${LINT.c} ${XINCDIRS} ${SRCS}

#
# Rules to create message catalogue files from .c files
#

.c.po:
	$(COMPILE.cpp) ${XINCDIRS} $<  > $<.i
	$(BUILD.po)

clean:
	${RM} ${OBJS} *.o a.out core

clobber: clean
	${RM} ${LPROG} *.po

