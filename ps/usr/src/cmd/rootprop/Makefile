#
#ident @(#)Makefile	1.18	96/05/07 SMI
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#       Makefile for maintaining usr/src/cmd/rootprop
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
LIBDIR2         = $(LIBDESTDIR)/lib
LIBDIRS		= -L$(LIBDIR1) -L$(LIBDIR2)

SCCSGET		= sccs get
CPPFLAGS	= ${INCLUDES} -D_KERNEL
CFLAGS		= ${OPTFLAG} -Xa
CCFLAGS		= ${OPTFLAG}
LINTFLAGS	= ${INCLUDES}
LDFLAGS		= ${LIBDIRS}

MODE		= 664
INSTALL		= /usr/sbin/install
INSTALLFLAGS	= -s -m $(MODE)

# Comment section processing - rootprop is a special case, so leave null.
POST_PROCESS_O  =

###
### Instructions:
###
### Fill in HDRS, SRCS, LIBS, and PROGRAM name to build the program.
### LIBS should use the -l<libname> form rather than explicit references
### to .a files, and allow the linker to find library files via the
### -L flags specified above.
###

PUBHDRS		= rootprop_io.h
PRIVHDRS	= rootprop_reg.h

HDRS		= $(PUBHDRS) $(PRIVHDRS)
INSTHDRS	= $(PUBHDRS:%.h=$(INCDESTDIR)/%.h)

SRCS		= $(PROGRAM).c

OBJS		= $(SRCS:.c=.o)

PROGRAM		= rootprop
# FILES are those things that are just used as is
FILES           = rootprop.conf

LIBS =

OWNER=root
GROUP=sys

INSDIR=$(ROOT)/kernel/drv
INSTALLABLES=$(PROGRAM:%=$(INSDIR)/%) $(FILES:%=$(INSDIR)/%)

$(INSDIR)/$(FILES):=   FILEMODE=0644
$(INSDIR)/$(PROGRAM):=  FILEMODE=0755

# install rules
$(INSDIR)/%: %
	$(INS.file)

.KEEP_STATE:

# Unless building for an external release, RELEASE_BUILD equates to '#'
# and strip rule is not executed.
all: $(PROGRAM) $(INSTHDRS) $(RELEASE_BUILD) strip

$(PROGRAM):	$(OBJS)
		$(LD) -r -o $@ $(OBJS)

install:        all $(INSDIR) $(INSTALLABLES)

$(INSDIR):
	-$(INS.dir)

strip:
	@str=`mcs -p $(PROGRAM) | egrep "@" | line`; \
		if [ "$$str" != ${MCSSTRING} ]; then \
			mcs -c -a ${MCSSTRING} $(PROGRAM); \
		fi

sccs:
		$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(HDRS)

clean:
		$(RM) $(PROGRAM) $(OBJS) *~ Makefile.bak core

clobber:        clean
		$(RM) $(PROGRAM)

sclean:		clean
		sccs clean

lint:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		$(LINT.c) $(SRCS)

cstyle:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		cstsyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)


$(INCDESTDIR)/%.h:	%.h
	$(INSTALL) $(INSTALLFLAGS) -f $(INCDESTDIR) $<

msgs:

# 'makedepend' requires /usr/openwin/bin in path.
depend:		$(SRCS) $(HDRS)
		makedepend $(INCLUDES) $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
