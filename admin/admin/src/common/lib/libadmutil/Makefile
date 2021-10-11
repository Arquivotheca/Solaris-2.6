#
#ident  "@(#)Makefile 1.48     95/05/17 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# classes/lib/libadmutil/Makefile
#

LIBRARY   =	libadmutil.a
VERS	  =	.2

MSGMASTER =	
MSG	  =	
TEXTDOMAIN =    $(MSG)
MSGPO	  =	$(TEXTDOMAIN).po
MSGOBJ	=	pics/$(MSG).o
MSGLIB	=	lib$(MSG).so.1

EXPHDRS   =	db_entry.h valid.h string_op.h admutil.h cl_util_errs.h 

IMPLHDRS  =	

OBJECTS =	\
		config_nsswitch.o \
		db_entry.o \
		get_domain.o \
		get_net_if_ip_addr.o \
		get_net_if_names.o \
		get_nodename.o \
		is_local_host.o \
		modify.o \
		remove_lb_ntoa_entry.o \
		set_domain.o \
		set_lb_ntoa_entry.o \
		set_net_if_ip_netmask.o \
		set_net_if_status.o \
		set_nodename.o \
		set_run_level.o \
		set_timezone.o \
		string_op.o \
		valid.o

SRCS      =	$(OBJECTS:.o=.c)
SRCMSGS	=	$(MSG).c $(MSG).h

include ../Makefile.lib

CLEANFILES=	valid_test.o $(MSGSRCS)

CLOBBERFILES=	$(DYNLIB) $(DYNLIBLINK) valid_test *% $(MSGPO)

# Following variables define where to find header files
LOCAL    =	.
RELATED  =
GENERAL  =	$(ROOTINCADMIN)
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(CPPFLAGS.master)

CFLAGS +=	$(DEBUG_CFLAGS)

.KEEP_STATE:

all: 		static dynamic

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

install:	all \
		$(ROOTADMINLIB) $(ROOTADMINLIBS) \
		$(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK)

valid_test:= LDFLAGS =
valid_test:	dynamic valid_test.o
		$(LINK.c) -o $@ valid_test.o  -R. -L. -ladmutil -lsocket -lgen
		
# build xxx_message.o file for inclusion in links of individual methods
$(MSGOBJ):	$(SRCMSGS)
 
# build xxx_message.c and xxx_message.h files
#       also build the xxx_message.po file that will be used as input to the
#       localization process -- not sure where to put it or how to distribute it
$(SRCMSGS):	$(MSGMASTER)
		$(MKMSGS) -d $(TEXTDOMAIN) < $(MSGMASTER)

install_h:	$(ROOTUSRINCLEXP)

lint:		$(LINTLIB)

# EXPORT DELETE START
# Special target to clean up the source tree for export distribution
# Warning: This target changes the source tree
EXPORT_SRC:
	sed -e "/^# EXPORT DELETE START/,/^# EXPORT DELETE END/d" \
		< Makefile > Makefile+
	$(MV) Makefile+ Makefile
	$(CHMOD) 444 Makefile

# EXPORT DELETE END

include ../Makefile.targ

