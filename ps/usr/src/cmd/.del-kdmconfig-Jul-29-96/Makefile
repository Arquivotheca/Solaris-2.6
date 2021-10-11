#
#ident @(#)Makefile	1.50	96/05/07 SMI
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#	Makefile for maintaining usr/src/cmd/kdmconfig
#

include $(SRC)/Makefile.master

# Override POST_PROCESS_O as used in .c.o rule in Makefile.master so
# that we don't stamp each object, yet still retain the method of setting
# OBJS seen below.
POST_PROCESS_O  =

# passed in from upper level makefile for -g or -O optimization 
OPTFLAG		= -g
MCSSTRING       = $(RELEASE_CM)

INCDIR1		= .
INCDIR2         = $(INCDESTDIR)
INCLUDES	= -I$(INCDIR1) -I$(INCDIR2)

LIBDIR1		= .
LIBDIR2         = $(LIBDESTDIR)
LIBDIRS		= -L$(LIBDIR1) -L$(LIBDIR2)

SCCSGET		= sccs get
CPPFLAGS	= ${INCLUDES}
CFLAGS		= ${OPTFLAG}
CCFLAGS		= ${OPTFLAG}
LINTFLAGS	= ${INCLUDES}
LDFLAGS		= ${LIBDIRS}

SUBDIRS         = help/howto help/reference help/topics

all:=           TARGET = all
install:=       TARGET = install
clean:=         TARGET = clean
sccs:=          TARGET = sccs

###
### Instructions:
###
### Fill in HDRS, SRCS, LIBS, and PROGRAM name to build the program.
### LIBS should use the -l<libname> form rather than explicit references
### to .a files, and allow the linker to find library files via the
### -L flags specified above.
###
PRIVHDRS	= except.h windvc.h ui.h silent.h exists.h kdmconfig.h \
              bootparam.h util.h
PUBHDRS		=
HDRS		= $(PRIVHDRS) $(PUBHDRS)

SRCS		= kdmconfig_msgs.c kdmconfig.c except.c doconfig.c dounconfig.c \
              ui.c silent.c windvc.c bootparam.c ui_objects.c util.c

OBJS		= $(SRCS:.c=.o)

LIBS            = -ltty -ldvc -lkvm -lgen -lelf -ltermlib

PROGRAM 	= kdmconfig

OWNER=root
GROUP=sys
FILEMODE=0755

INSDIR=$(ROOT)/sbin
INSTALLABLES=$(PROGRAM:%=$(INSDIR)/%)

# install rules
$(INSDIR)/%: %
	$(INS.file)

.KEEP_STATE:

${SUBDIRS}: FRC
		@(cd $@; pwd; $(MAKE) "SCCSGET=$(SCCSGET)" ${TARGET});

.PARALLEL: $(OBJS) $(SUBDIRS)

all:    $(PROGRAM)

# Degree of comment processing determined by assignments made to
# INTERNAL_RELEASE_BUILD and/or RELEASE_BUILD environment variables.
# See Makefile.master.

$(PROGRAM): 	$(OBJS) ${LIBDIR2}/libdvc.a
		$(LINK.c) -o $(PROGRAM) $(OBJS) $(LIBS)
		$(PROCESS_COMMENT) $(PROGRAM)

install:        all .WAIT $(INSDIR) $(INSTALLABLES) $(SUBDIRS)

$(INSDIR):
	-$(INS.dir)

sccs:           $(SUBDIRS)
		$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(HDRS)

clean:          $(SUBDIRS)
		$(RM) $(OBJS) $(TEXT_DOMAIN).[mp]o *~ \
		    Makefile.bak *.i kdmconfig core tags kdmconfig_msgs.?

clobber:        clean
		$(RM) $(PROGRAM)

sclean:         clean
		sccs clean

lint:		$(SRCS) $(PRIVHDRS) $(PUBHDRS) 
		$(LINT.c) $(SRCS) $(LIBDIRS) $(LIBS)

cstyle:		$(SRCS) $(PRIVHDRS) $(PUBHDRS) 
		cstyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

TEXT_DOMAIN=SUNW_INSTALL_KDMCONFIG

msgs: $(TEXT_DOMAIN).po
	@cp $(TEXT_DOMAIN).po ../messages

$(TEXT_DOMAIN).po kdmconfig_msgs.c:	msg_master_$(MACH) $(UI_SRCS) $(UI_HDRS)
	@echo "${LEVEL}" 'Generating .po file and kdmconfig_msgs.[ch] from msg_master_$(MACH)'
	@$(MKMSGS) -d $(TEXT_DOMAIN) < msg_master_$(MACH)

make-mo:
	sccs get make-mo
	chmod +x ./make-mo

mofile: $(TEXT_DOMAIN).po make-mo
	@echo 'Making dummy-de .mo file'
	@./make-mo de
	@msgfmt -o $(TEXT_DOMAIN).mo de.po
	@rm -f de.[pm]o

# 'makedepend' requires /usr/openwin/bin in path.
depend:		$(SRCS) $(HDRS)
		makedepend $(INCLUDES) $(SRCS)

test: test.o windvc.o silent.o
	$(LINK.c) -o test silent.o test.o windvc.o -L$(LIBDESTDIR) $(LIBS)

FRC:

# DO NOT DELETE THIS LINE -- make depend depends on it.
