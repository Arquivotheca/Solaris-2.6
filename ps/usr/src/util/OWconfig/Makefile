#
# Real Makefile SID -- change $-signs to %-signs in the line below
# to use this as a real Makefile, delete the space between '#' and
# ident, and put a space between '#' and ident for the template SID.
#
#ident "@(#)Makefile 1.3 94/05/12"
#
# ident  Derived from template: @(#)Makefile.program	1.5	92/09/24
#

include ../../Makefile.master

# passed in from upper level makefile for -g or -O optimization 
OPTFLAG		= -g
MCSSTRING       = $(RELEASE_CM)

INCDIR1		= .
INCDIR2		= ../../include
INCLUDES	= -I$(INCDIR1) -I$(INCDIR2)

LIBDIR1		= .
LIBDIR2		= ../../lib
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
PRIVHDRS	=
PUBHDRS		=
HDRS		= $(PRIVHDRS) $(PUBHDRS)

SRCS		=

OBJS		=

LIBS        =

PROGRAM 	= OWconfig

OWNER=root
GROUP=sys
FILEMODE=0755

INSDIR=$(ROOT)/etc/openwin/server/etc
INSTALLABLES=$(PROGRAM:%=$(INSDIR)/%)

# install rules
$(INSDIR)/%: %
	$(INS.file)

.KEEP_STATE:

${SUBDIRS}: FRC

all:    $(PROGRAM)
	@true

$(PROGRAM):	

install:        all $(INSDIR) $(INSTALLABLES)
	@true

$(INSDIR):
	-$(INS.dir)

strip:
	

sccs:           $(SUBDIRS)
		$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(HDRS)

clean: FRC

clobber:        clean

sclean:         clean
		sccs clean

lint: FRC

cstyle: FRC

msgs: FRC

$(TEXT_DOMAIN).po: FRC

make-mo: FRC

mofile: FRC

# 'makedepend' requires /usr/openwin/bin in path.
depend:		$(SRCS) $(HDRS) FRC

FRC:

# DO NOT DELETE THIS LINE -- make depend depends on it.
