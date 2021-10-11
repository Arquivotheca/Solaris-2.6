#
#ident	"@(#)Makefile	1.36	94/10/25 SMI"
#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#
# lib/libadmapm/Makefile
#

LIBRARY     =	libadmapm.a
VERS	    =	.2
MSGMASTER   =   adm_err.msgs
MSG         =   adm_err_msgs
MSG_DOMAIN  = SUNW_ADM_FW
NETMGT_TEXT_DOMAIN = SYS_admcom
SVR4FLAG    =	-D_SVR4_
SYMFLAGS    =   -DADM_TEXT_DOMAIN=\"$(MSG_DOMAIN)\"		\
		-DNETMGT_TEXT_DOMAIN=\"$(NETMGT_TEXT_DOMAIN)\"		\
		-DADM_DEBUG
PICFLAG	    =	-K PIC

EXPHDRS   =	adm_fw.h	adm_amcl.h	adm_args.h 	\
		adm_err.h	adm_reqID.h	adm_diag.h	\
		adm_log.h	adm_method.h	adm_om_proto.h	\
		adm_err_msgs.h	adm_cache.h	adm_sec.h	\
		adm_local.h

IMPLHDRS  =	adm_fw_impl.h	adm_amsl_impl.h	adm_args_impl.h  \
		adm_err_impl.h	adm_reqID_impl.h adm_lock_impl.h \
		adm_log_impl.h	adm_om_impl.h	adm_messages.h	\
		adm_sec_impl.h

MSGPO	  =	$(MSG_DOMAIN).po
MSGOBJ    =     $(MSG).o
SRCMSGS	  =	$(MSG).c $(MSG).h $(MSGPO)

OBJECTS =	$(MSGOBJ)						\
		adm_fw.o	adm_fw_impl.o		adm_fw_glob.o	\
		adm_amcl.o						\
		adm_args.o	adm_args_impl.o				\
		adm_err.o	adm_err_impl.o				\
		adm_reqID.o	adm_reqID_impl.o			\
				adm_lock_impl.o				\
		adm_diag.o	adm_diag_impl.o				\
		adm_log.o	adm_log_impl.o		adm_log_glob.o	\
		adm_find_method.o			adm_om_glob.o	\
		adm_sec.o	adm_sec_impl.o				\
		adm_cache.o	local_args.o

SRCS      =	$(OBJECTS:.o=.c)

NATIVECC  =	$(CC)

include ../Makefile.lib

CLOBBERFILES=	$(DYNLIB) $(SRCMSGS)
TXTS =

# Following variables define where to find header files
LOCAL    =	.
RELATED  =	../libadmagt ../libadmsec
GENERAL  =	../libadmcom

CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(SVR4FLAG) $(SYMFLAGS) $(CPPFLAGS.master)

CFLAGS +=       $(DEBUG_CFLAGS)

.KEEP_STATE:

$(MSGOBJ):	$(SRCMSGS)
		$(COMPILE.c) $(MSG).c

$(SRCMSGS):	$(MSGMASTER)
		$(MKMSGS) -d $(MSG_DOMAIN) < $(MSGMASTER)

all: 		static dynamic $(TXTS)

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

install:	all $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK) \
		$(INSTMSGS)

install_h:	$(ROOTUSRINCLEXP)

lint:		$(LINTLIB)

include ../Makefile.targ
