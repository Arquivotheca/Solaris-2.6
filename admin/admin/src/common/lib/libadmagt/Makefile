#
#ident	"@(#)Makefile	1.19	94/10/25 SMI"	/* SunOS 5.0 */
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# lib/libadmagt/Makefile
#


LIBRARY   =	libadmagt.a
VERS	  =	.2
ADM_TEXT_DOMAIN = SYS_admfw
SYMFLAGS  =	-DADM_TEXT_DOMAIN=\"$(ADM_TEXT_DOMAIN)\"	\
		-DADM_DEBUG
SVR4FLAG  =	-D_SVR4_
PICFLAG   =	-K PIC

EXPHDRS   =	adm_amsl.h

IMPLHDRS  =	adm_amsl_impl.h	adm_amsl_proto.h

OBJECTS =	adm_amsl_verify.o	adm_amsl_dispatch.o	\
		adm_amsl_local.o	adm_amsl_invoke.o	\
		adm_amsl_args.o		adm_amsl_auth.o		\
		adm_amsl_buffer.o	adm_amsl_error.o	\
		adm_amsl_init.o		adm_amsl_log.o		\
		adm_amsl_parms.o	adm_amsl_pipes.o

include ../Makefile.lib

CLOBBERFILES =	$(DYNLIB)
TXTS =

# Following variables define where to find header files
LOCAL    =	.
RELATED  =	../libadmapm ../libadmsec ../libadmcom
GENERAL  =	
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(SVR4FLAG) $(SYMFLAGS) $(CPPFLAGS.master)

CFLAGS +=       $(DEBUG_CFLAGS)

DYNLIBLINK=	$(DYNLIB:%$(VERS)=%)

.KEEP_STATE:

all: 		static dynamic $(TXTS)

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

install:	all $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK)

install_h:	$(ROOTUSRINCLEXP)

lint:		$(LINTLIB)

include ../Makefile.targ
