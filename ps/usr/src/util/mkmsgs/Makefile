#
# Real Makefile SID -- change $-signs to %-signs in the line below
# to use this as a real Makefile, delete the space between '#' and
# ident, and put a space between '#' and ident for the template SID.
#
#ident "@(#)Makefile 1.7 94/10/25"
#
# ident  Derived from template: @(#)Makefile.program	1.5	92/09/24
#

include ../../Makefile.master

.KEEP_STATE:

# passed in from upper level makefile for -g or -O optimization 
OPTFLAG		= -g

INCDIR1		=
INCDIR2		= 
INCLUDES	=

LIBDIR1		=
LIBDIR2		=
LIBDIRS		=

SCCSGET		= sccs get
CPPFLAGS	= ${INCLUDES}
CFLAGS		= ${OPTFLAG}
CCFLAGS		= ${OPTFLAG}
LINTFLAGS	= ${INCLUDES}
LDFLAGS		= ${LIBDIRS}

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

SRCS		= mkmsgs.c
OBJS		= mkmsgs.o

LIBS            = 

PROGRAM 	= mkmsgs

INSDIR=$(MKMSGDIR)
INSTALLABLES=$(INSDIR)/$(PROGRAM)

# install here too, since mkmsgs is not a delivered binary, but used
# during the build
all:    $(PROGRAM) $(INSDIR) $(INSTALLABLES)
	@true

# Degree of comment processing determined by assignments made to
# INTERNAL_RELEASE_BUILD and/or RELEASE_BUILD environment variables.
# See Makefile.master.
$(PROGRAM): $(SRCS)
	$(NATIVECC) $(SRCS) -o $(PROGRAM)
	$(PROCESS_COMMENT) $(PROGRAM)

OWNER=root
GROUP=sys
FILEMODE=0755

# installing is really done in "all" above

# install rules
$(INSDIR)/%:%
	$(INS.file)

install: all
	@true

$(INSDIR):
	-$(INS.dir)

sccs:
		$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(HDRS)

clean:
		$(RM) $(OBJS)  *~ \
		    Makefile.bak *.i core

clobber:        clean
		$(RM) -r $(PROGRAM) $(INSDIR)

sclean:         clean
		sccs clean

lint:		$(SRCS) $(PRIVHDRS) $(PUBHDRS) $(UI_SRCS) $(UI_HDRS)
		$(LINT.c) $(SRCS) $(UI_SRCS) $(LIBDIRS) $(LIBS)

cstyle:		$(SRCS) $(PRIVHDRS) $(PUBHDRS) $(UI_SRCS) $(UI_HDRS)
		cstyle $(SRCS) $(PRIVHDRS) $(PUBHDRS) $(UI_SRCS) $(UI_HDRS)

# 'makedepend' requires /usr/openwin/bin in path.
depend:		$(SRCS) $(HDRS) $(UI_SRCS) $(UI_HDRS)
		makedepend $(INCLUDES) $(SRCS) $(UI_SRCS)

FRC:

# DO NOT DELETE THIS LINE -- make depend depends on it.
