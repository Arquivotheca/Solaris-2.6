# 
# Makefile for swmtool.
#
# The _stubs, and _ui files are built from Guide .G files.
# Any changes made to them here will potentially be lost.
#
#ident  "@(#)Makefile 1.28 94/02/11"
#
ARCH:sh=uname -p
LEVEL="=="
TEXT_DOMAIN = SUNW_INSTALL_SWM

PROGRAM = ${ARCH}/swmtool

SOURCES.guide = Base_ui.c Base_stubs.c \
	Client_ui.c Client_stubs.c \
	Cmd_ui.c Cmd_stubs.c \
	File_ui.c File_stubs.c \
	Meter_ui.c Meter_stubs.c \
	Pkg_ui.c Pkg_stubs.c \
	Props_ui.c Props_stubs.c \
	swmtool.c

SOURCES.xview = Base.c Client.c Notice.c Pkg.c Props.c \
	Modules.c Levels.c group.c group_data.c \
	xv_admin.c xv_category.c xv_host.c xv_init.c \
	xv_load.c xv_meter.c xv_subr.c xv_tty.c

SOURCES.misc = admin.c file.c host.c mount.c pkgexec.c \
	select.c space.c subr.c stubs.c

HEADERS.misc = admin.h defs.h host.h ui.h xview.h

SOURCES = $(SOURCES.guide) $(SOURCES.xview) $(SOURCES.misc)
OBJECTS = $(SOURCES:%.c=${ARCH}/%.o)

DEBUG_OBJECTS	= $(SOURCES:%.c=${ARCH}/debug/%.o)
DEBUGDIR	= ${ARCH}/debug

#
# For dependency checking

LIBSW = ../../lib/${ARCH}/libsw.a

# Compiler flags.
# passed in from upper level makefile for -G or -O optimization

MCSSTRING=@\(\#\)SunOS\ 5.0\ FCS\ June\ 1992

ANSI		= -Xa
OPTFLAG		= -g
MCSSTRING	= @\(\#\)Solaris\ 2.1\ FCS\ November\ 1992

INCLUDES	= -I. -I../../include -I$(OPENWINHOME)/include

LIBDIRS		= -L. -L../../lib/${ARCH} -L$(OPENWINHOME)/lib -R/usr/openwin/lib

SCCSGET		= sccs get
DEFS		= -DMAIN -DSVR4 -DXVIEW -DTEXT_DOMAIN=\"$(TEXT_DOMAIN)\"

CFLAGS		= ${OPTFLAG} ${ANSI}
CPPFLAGS	= ${INCLUDES} ${DEFS}

LDFLAGS		= ${CFLAGS} ${LIBDIRS}
LDLIBS		= -lsw -ladm -lsocket -lnsl -lxview -lolgx -lX11 -lintl
PURIFYOPTS	=
PURIFY		= purify ${PURIFYOPTS}

${ARCH}/debug/%.o: %.c
	${COMPILE.c} -o $@ $<

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

# Standard targets.

demo	:=	CPPFLAGS += -DDEMO
debug	:=	OPTFLAG = -g

all:	${ARCH} .WAIT $(PROGRAM)
	@true

demo:	 ${ARCH} .WAIT $(PROGRAM)
	@true

install:	all
	@true

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

$(PROGRAM): $(OBJECTS) $(LIBSW)
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)

$(PROGRAM).pure: $(DEBUGDIR) .WAIT $(DEBUG_OBJECTS) $(LIBSW)
	$(PURIFY) $(LINK.c) -o $(PROGRAM).pure $(DEBUG_OBJECTS) $(LDLIBS)

debug: $(DEBUGDIR) .WAIT $(DEBUG_OBJECTS) $(LIBSW)
	$(LINK.c) -o $(PROGRAM) $(DEBUG_OBJECTS) $(LDLIBS)

${DEBUGDIR}:
	@[ -d ${DEBUGDIR} ] || (rm -f ${DEBUGDIR} ;mkdir -p ${DEBUGDIR})

strip:
	@str=`mcs -p $(PROGRAM) | egrep "@" | line`; \
	if [ "$$str" != ${MCSSTRING} ]; then \
		echo "${LEVEL} strip $(PROGRAM)"; \
		strip $(PROGRAM); \
		mcs -d $(PROGRAM); \
		mcs -a ${MCSSTRING} $(PROGRAM); \
	fi

cstyle:
	cstyle $(HEADERS.misc) $(SOURCES.misc) $(SOURCES.xview)
 
lint:	$(SOURCES)
	$(LINT.c) $(SOURCES) $(TARGETS.c)

clean:
	$(RM) $(PROGRAM) $(OBJECTS) $(DEBUG_OBJECTS) core *.po *.o

xgettext:	$(TEXT_DOMAIN).po

msgs: $(TEXT_DOMAIN).po
	mv $(TEXT_DOMAIN).po ../messages

$(TEXT_DOMAIN).po:	$(SOURCES)
	xgettext -s -c "i18n:" -m "" $(SOURCES)
	sed	-e "/^# msg/d" \
		-e "/^domain[ 	][ 	]*\"messages\"/d" \
		-e "s?^#[ 	][ 	]*[/ ]\*/*?#?" \
		-e "s?[ 	][ 	]*\*/??" \
		$(TEXT_DOMAIN).po messages.po > $(TEXT_DOMAIN).po.sed
	mv $(TEXT_DOMAIN).po.sed $(TEXT_DOMAIN).po
	rm -f messages.po

.WAIT:
