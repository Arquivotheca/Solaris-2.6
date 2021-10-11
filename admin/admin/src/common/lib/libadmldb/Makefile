#
#ident	"@(#)Makefile	1.9	94/10/25 SMI"
#
# Copyright (c) 1992 by Sun Microsystems, Inc.
#

LIBRARY   =	libadmldb.a
VERS	  =	.2
MSGMASTER =	msg_master
MSG	=	admldb_msgs
MSG_DOMAIN =	SUNW_ADM_LIBLDB
MSGPO	=	$(MSG_DOMAIN).po

EXPHDRS   =	cl_database_parms.h admldb.h admldb_impl.h $(MSG).h

OBJECTS =	$(MSG).o admldb_impl.o admdb_tbl.o admldb_nis.o admldb_ufs.o \
		tbl_defs.o admldb.o nis_plus_ufs_policy.o

SRCS      =	$(OBJECTS:.o=.c)

MSGSRCS	=	$(MSG).c $(MSG).h

include ../Makefile.lib

CLEANFILES=	$(MSGSRCS)

CLOBBERFILES=	$(MSGPO) $(DYNLIB) $(DYNLIBLINK)

TXTS =

# Following variables define where to find header files
LOCAL    =	.
RELATED  =
GENERAL  =	$(ROOTINCADMIN)
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(CPPFLAGS.master)
# CFLAGS +=	$(DEBUG_CFLAGS) -DNIS_DEBUG
CFLAGS +=	$(DEBUG_CFLAGS)

.KEEP_STATE:

all: 		static dynamic $(TXTS)

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)


install:	all $(ROOTADMINLIB) $(ROOTADMINLIBS) \
		$(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK) \
		$(INSTMSGS)

# build xxx_message.c and xxx_message.h files
#	also build the xxx_message.po file that will be used as input to the
#	localization process -- not sure where to put it or how to distribute it
$(MSGSRCS):	$(MSGMASTER)
		$(MKMSGS) -d $(MSG_DOMAIN) < $(MSGMASTER)

install_h:	$(ROOTUSRINCLEXP)

lint:		$(LINTLIB)

include ../Makefile.targ

