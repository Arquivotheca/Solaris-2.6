#
# Copyright (c) 1994-1996, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident ""@(#)Makefile 1.114 96/10/04""
# installtool makefile

include ../../Makefile.cmd



# ------------------------------------------------------------------------------
# QAP is based on <bundled/app/admintool>
# Set up for QA Partner.
# If QAPCH is "#", then QAP is commented out
# If QAPCH is "", then QAP is enabled.
$(QAPCH)QAPFLAG =       -DPARTNER
$(QAPCH)QAPINCPATH      =       $(QAPINCPATH11)
$(QAPCH)QAPLIBPATH      =       $(QAPLIBPATH11)
$(QAPCH)QAPLIB          =       -lpartner


PROG=installtool
MSG_DOMAIN=	SUNW_INSTALL_INSTALL

# application sources -------------------------------------------------

# TeleUSE generated sources
TU_SRCS = \
	tu_runtime.c \
	pfgAllocateSvcQuery.c \
	pfgAutolayout.c \
	pfgAutoquery.c \
	pfgBootDiskSelect.c \
	pfgClients.c \
	pfgCyl.c \
	pfgDisks.c \
	pfgFilesys.c \
	pfgInstallClients.c \
	pfgIntro.c \
	pfgLayout.c \
	pfgLocale.c \
	pfgMeta.c \
	pfgOs.c \
	pfgPrequery.c \
	pfgPreserve.c \
	pfgProgress.c \
	pfgProgressBar.c \
	pfgRemote.c \
	pfgRemquery.c \
	pfgDSRFSRedist.c \
	pfgDSRFSSummary.c \
	pfgDSRMedia.c \
	pfgDSRSpaceReq.c \
	pfgSoftware.c \
	pfgSolarCust.c \
	pfgSolarPart.c \
	pfgSummary.c \
	pfgSwQuery.c \
	pfgUpgrade.c \
	pfgUseDisks.c

PFG_SRCS = \
	pferror.c \
	pfgallocatesvcquery.c \
	pfgautolayout.c \
	pfgautoquery.c \
	pfgbootdiskselect.c \
	pfgclient_setup.c \
	pfgclients.c \
	pfgcyl.c \
	pfgdisks.c \
	pfgdsr_al.c \
	pfgdsr_analyze.c \
	pfgdsr_fsredist.c \
	pfgdsr_fssummary.c \
	pfgdsr_media.c \
	pfgdsr_spacereq.c \
	pfgfilesys.c \
	pfgintro.c \
	pfglayout.c \
	pfglocale.c \
	pfgmain.c \
	pfgmeta.c \
	pfgos.c \
	pfgprequery.c \
	pfgpreserve.c \
	pfgprocess.c \
	pfgprogress.c \
	pfgprogressbar.c \
	pfgremote.c \
	pfgremquery.c \
	pfgsoftware.c \
	pfgsolarcust.c \
	pfgsolarispart.c \
	pfgsummary.c \
	pfgswquery.c \
	pfgtoplevel.c \
	pfgtutor.c \
	pfgupgrade.c \
	pfgusedisks.c \
	pfgutil.c \
	v_pfg_disks.c \
	v_pfg_fdisk.c \
	v_pfg_lfs.c \
	v_pfg_rfs.c \
	v_pfg_sw.c

SRCS = \
	${TU_SRCS} \
	${PFG_SRCS}

FILES =	Installtool \
	Installtool_lowres

OBJS	= ${SRCS:%.c=${ARCH}/%.o}

# pick up all current .h files
# excluding the TeleUse generated files
HDRS	:sh= ls *.h | grep -v "pfg[A-Z].*_ui\.h" | grep -v tu_runtime

SUBDIRS	= adminhelp

install:=	TARGET= install

# unbundled product homes ---------------------------------------------
# The location of compile time libraries
OPENWINHOME		:sh= echo "${OPENWINHOME:-/usr/openwin}"
# The location of run time libraries
OPENWINHOME_RUNTIME	:sh= echo "${OPENWINHOME:-/usr/openwin}"
# The location of compile time MOTIF libraries
MOTIFHOME		:sh= echo "${MOTIFHOME:-/usr/dt}"
# The location of run time MOTIF libraries
MOTIFHOME_RUNTIME	:sh= echo "${MOTIFHOME_RUNTIME:-/usr/dt}"

# graphics sources ----------------------------------------------------
GINCLUDEDIR=	-I$(MOTIFHOME)/include -I$(OPENWINHOME)/include -I/usr/dt/include
GLIBDIR= 	-L$(MOTIFHOME)/lib -L$(OPENWINHOME)/lib $(QAPLIBPATH)\
		-R$(MOTIFHOME_RUNTIME)/lib:$(OPENWINHOME_RUNTIME)/lib
GLIBS=		-lMrm -lXm -lXt -lX11 $(QAPLIB)

# suninstall sources
APPINCLUDEDIR = -I$(ROOTINCADMIN)
APPLIBDIR  = -L$(ROOTADMINLIB)
APPLIBS    = -lspmixm -lspmiapp -lspmisvc -lspmistore -lspmisoft -lspmicommon
APPDEPLIBS = \
	$(ROOTADMINLIB)/libspmicommon.so \
	$(ROOTADMINLIB)/libspmistore.so \
	$(ROOTADMINLIB)/libspmisoft.so \
	$(ROOTADMINLIB)/libspmisvc.so \
	$(ROOTADMINLIB)/libspmiapp.so
STATICAPPDEPLIBS = \
	$(ROOTADMINLIB)/libspmicommon.a \
	$(ROOTADMINLIB)/libspmistore.a \
	$(ROOTADMINLIB)/libspmisoft.a \
	$(ROOTADMINLIB)/libspmisvc.a \
	$(ROOTADMINLIB)/libspmiapp.a

# generic sources
LIBS = -ldl -ldevinfo -lnsl -ladm -lkvm -lelf

# compiler -------------------------------------------------------------
CPPFLAGS  += ${GINCLUDEDIR} ${APPINCLUDEDIR} $(QAPINCPATH)
CFLAGS    += $(DEBUG_CFLAGS) -Xa -DUIL -DSYSV $(QAPFLAG)

# linker
LDFLAGS  += $(DEBUG_CFLAGS) -R$(ROOTADMINLIB:$(ROOT)%=%)
LDLIBS   += ${GLIBDIR} ${APPLIBDIR} ${GLIBS} ${APPLIBS} ${LIBS}
STATICLDLIBS += ${GLIBDIR} ${APPLIBDIR} ${GLIBS} \
		-Bstatic ${APPLIBS} -Bdynamic ${LIBS}

# lint
LINT_ERR = lint.errs
LINT_OPTS = -umx
LINTFLAGS  = ${LINT_OPTS}

# cstyle
CSTYLE_ERR = cstyle.errs

depend:
	makedepend -- ${CPPFLAGS} ${CFLAGS} -- ${PFG_SRCS}

# local purify options
PURIFYOPTS  += -cache-dir=./cache

ROOTRESDFILES	= $(FILES:%=$(ROOTRESD)/%)

# special targets -------------------------------------------------------
${ARCH}/%.o: %.c
	$(PURIFY) ${COMPILE.c} -o $@ $<

.KEEP_STATE:

# targets --------------------------------------------------------------

all: $(ARCH) .WAIT $(PROG)
	@true

${ARCH}:
	@[ -d $@ ] || (rm -f $@ ;mkdir -p $@)

$(PROG): ${ARCH} .WAIT $(OBJS) .WAIT $(APPDEPLIBS)
	$(PURIFY) $(LINK.c) -o $@ $(OBJS) $(LDLIBS)
	cp $@ ${ARCH}
	$(POST_PROCESS)

$(PROG).static: ${ARCH} .WAIT $(OBJS) .WAIT $(STATICAPPDEPLIBS)
	$(LINK.c) -o $@ $(OBJS) $(STATICLDLIBS)
	cp $@ ${ARCH}
	$(POST_PROCESS)

lint:
	${LINT.c} ${PFG_SRCS} > ${LINT_ERR} 2>&1

cstyle:
	@echo Running cstyle... See ${CSTYLE_ERR} for errors
	@rm -f ${CSTYLE_ERR}
	@for i in ${PFG_SRCS} ${HDRS}; do \
		echo "------------ $${i} ------------" >> ${CSTYLE_ERR}; \
		cstyle $${i} >> ${CSTYLE_ERR} 2>&1; \
		echo "" >> ${CSTYLE_ERR}; \
		echo "" >> ${CSTYLE_ERR}; \
        done;

install: all $(ROOTUSRSBINPROG) $(ROOTINSTALLDFILES) msgs \
		$(INSTMSGS) $(SUBDIRS)

install: all $(ROOTUSRSBINPROG) $(ROOTRESDFILES) msgs $(INSTMSGS) $(SUBDIRS)

${SUBDIRS}: FRC
	cd $@; pwd; $(MAKE) $(TARGET)

#
# Create a message file for domain "install".
#
# Since the app may use strings exported from the libraries,
# we could actually end up generating 'extraneous' .po files here
# (e.g. SUNW_INSTALL_LIBAPP.po).
# We delete all but the app's .po here.
#
msgs:	$(MSG_DOMAIN).po

${MSG_DOMAIN}.po: ${SRCS} ${HDRS}
	@echo "Making messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} \
			-c "i18n:" - >/dev/null 2>&1
	@for i in *.po; do \
		if test $${i} != ${MSG_DOMAIN}.po; then \
			echo "deleting $${i}"; \
			rm -f $${i}; \
		fi; \
	done;

# Create a message file with "translated" test strings.
# Turn the po file into an mo file ready for testing.
msgs_test:	${SRCS} ${HDRS}
	@echo "Making test messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} \
			-c "i18n:" -m "öýê!" - >/dev/null 2>&1
	@for i in *.po; do \
		if test $${i} != ${MSG_DOMAIN}.po; then \
			echo "deleting $${i}"; \
			rm -f $${i}; \
		fi; \
	done;
	@msgfmt -o ${MSG_DOMAIN}.mo ${MSG_DOMAIN}.po

clean_msgs:
	-rm -f *.po *.mo

clean:
	-rm -f core errs *.BAK ${OBJS} *.o

clobber: clean clean_msgs
	-rm -f ${PROG} ${PROG}.static
	-rm -rf ${ARCHSUPPORT}

FRC:

include ../../Makefile.targ

.WAIT:

#
# The following dependencies were actually generated using an
# install internal script (/usr/rmtc/misc/install/bin/deps.pl).
# First a make depend is done, and then certain dependencies are
# stripped out altogether (like X11, Xm, and some /usr/include things).
# Then the remaining dependencies had the hard-coded paths removed
# and replaced with environment variables references.
#

# DO NOT DELETE THIS LINE -- make depend depends on it.
# makedepend dependencies modified using:
# /usr/rmtc/misc/spmi/bin/deps.pl, Version: 1.2
# on Mon Jul 15 17:06:56 US/Mountain 1996

${ARCH}/pferror.o: pf.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pferror.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pferror.o: pferror.h pfg.h
${ARCH}/pferror.o: pfg_strings.h pfg_labels.h
${ARCH}/pfgallocatesvcquery.o: pf.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgallocatesvcquery.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgallocatesvcquery.o: pferror.h pfg.h
${ARCH}/pfgallocatesvcquery.o: pfg_strings.h pfg_labels.h pfgAllocateSvcQuery_ui.h
${ARCH}/pfgallocatesvcquery.o: tu_runtime.h
${ARCH}/pfgautolayout.o: pf.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgautolayout.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgautolayout.o: pferror.h pfg.h
${ARCH}/pfgautolayout.o: pfg_strings.h pfg_labels.h pfgAutolayout_ui.h tu_runtime.h
${ARCH}/pfgautoquery.o: pf.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgautoquery.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgautoquery.o: pferror.h pfg.h
${ARCH}/pfgautoquery.o: pfg_strings.h pfg_labels.h pfgAutoquery_ui.h tu_runtime.h
${ARCH}/pfgbootdiskselect.o: pf.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgbootdiskselect.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgbootdiskselect.o: pferror.h pfg.h
${ARCH}/pfgbootdiskselect.o: pfg_strings.h pfg_labels.h pfgBootDiskSelect_ui.h
${ARCH}/pfgbootdiskselect.o: tu_runtime.h
${ARCH}/pfgclient_setup.o: pf.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgclient_setup.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgclient_setup.o: pferror.h pfg.h
${ARCH}/pfgclient_setup.o: pfg_strings.h pfg_labels.h pfgInstallClients_ui.h
${ARCH}/pfgclient_setup.o: tu_runtime.h
${ARCH}/pfgclients.o: pf.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgclients.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgclients.o: pferror.h pfg.h
${ARCH}/pfgclients.o: pfg_strings.h pfg_labels.h pfgClients_ui.h tu_runtime.h
${ARCH}/pfgcyl.o: pf.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgcyl.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgcyl.o: pferror.h pfg.h
${ARCH}/pfgcyl.o: pfg_strings.h pfg_labels.h pfgdisks.h pfgCyl_ui.h tu_runtime.h
${ARCH}/pfgdisks.o: pf.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgdisks.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgdisks.o: pferror.h pfg.h
${ARCH}/pfgdisks.o: pfg_strings.h pfg_labels.h pfgdisks.h cyl.bmp pfgDisks_ui.h
${ARCH}/pfgdisks.o: tu_runtime.h
${ARCH}/pfgdsr_al.o: pfg.h
${ARCH}/pfgdsr_al.o: pf.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgdsr_al.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgdsr_al.o: pferror.h pfg_strings.h pfg_labels.h pfgprogressbar.h
${ARCH}/pfgdsr_analyze.o: pfg.h
${ARCH}/pfgdsr_analyze.o: pf.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgdsr_analyze.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgdsr_analyze.o: pferror.h pfg_strings.h pfg_labels.h pfgprogressbar.h
${ARCH}/pfgdsr_fsredist.o: pfg.h
${ARCH}/pfgdsr_fsredist.o: pf.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgdsr_fsredist.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgdsr_fsredist.o: pferror.h pfg_strings.h pfg_labels.h pfgDSRFSRedist_ui.h
${ARCH}/pfgdsr_fsredist.o: tu_runtime.h
${ARCH}/pfgdsr_fssummary.o: pfg.h
${ARCH}/pfgdsr_fssummary.o: pf.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgdsr_fssummary.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgdsr_fssummary.o: pferror.h pfg_strings.h pfg_labels.h pfgDSRFSSummary_ui.h
${ARCH}/pfgdsr_fssummary.o: tu_runtime.h
${ARCH}/pfgdsr_media.o: pfg.h
${ARCH}/pfgdsr_media.o: pf.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgdsr_media.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgdsr_media.o: pferror.h pfg_strings.h pfg_labels.h pfgDSRMedia_ui.h
${ARCH}/pfgdsr_media.o: tu_runtime.h
${ARCH}/pfgdsr_spacereq.o: pfg.h
${ARCH}/pfgdsr_spacereq.o: pf.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgdsr_spacereq.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgdsr_spacereq.o: pferror.h pfg_strings.h pfg_labels.h pfgDSRSpaceReq_ui.h
${ARCH}/pfgdsr_spacereq.o: tu_runtime.h
${ARCH}/pfgfilesys.o: pf.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgfilesys.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgfilesys.o: pferror.h pfg.h
${ARCH}/pfgfilesys.o: pfg_strings.h pfg_labels.h pfgFilesys_ui.h tu_runtime.h
${ARCH}/pfgintro.o: pf.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgintro.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgintro.o: pferror.h pfg.h
${ARCH}/pfgintro.o: pfg_strings.h pfg_labels.h pfgIntro_ui.h tu_runtime.h
${ARCH}/pfglayout.o: pf.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfglayout.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfglayout.o: pferror.h pfg.h
${ARCH}/pfglayout.o: pfg_strings.h pfg_labels.h pfgLayout_ui.h tu_runtime.h
${ARCH}/pfglocale.o: pf.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfglocale.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfglocale.o: pferror.h pfg.h
${ARCH}/pfglocale.o: pfg_strings.h pfg_labels.h pfglocales.h pfgLocale_ui.h
${ARCH}/pfglocale.o: tu_runtime.h
${ARCH}/pfgmain.o: pf.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgmain.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgmain.o: pferror.h pfg.h
${ARCH}/pfgmain.o: pfg_strings.h pfg_labels.h
${ARCH}/pfgmeta.o: pf.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgmeta.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgmeta.o: pferror.h pfg.h
${ARCH}/pfgmeta.o: pfg_strings.h pfg_labels.h pfgMeta_ui.h tu_runtime.h
${ARCH}/pfgos.o: pf.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgos.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgos.o: pferror.h pfg.h
${ARCH}/pfgos.o: pfg_strings.h pfg_labels.h pfgOs_ui.h tu_runtime.h
${ARCH}/pfgprequery.o: pf.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgprequery.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgprequery.o: pferror.h pfg.h
${ARCH}/pfgprequery.o: pfg_strings.h pfg_labels.h pfglocales.h pfgPrequery_ui.h
${ARCH}/pfgprequery.o: tu_runtime.h
${ARCH}/pfgpreserve.o: pf.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgpreserve.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgpreserve.o: pferror.h pfg.h
${ARCH}/pfgpreserve.o: pfg_strings.h pfg_labels.h pfgpreserve.h pfgPreserve_ui.h
${ARCH}/pfgpreserve.o: tu_runtime.h
${ARCH}/pfgprocess.o: pf.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgprocess.o: pferror.h pfg.h
${ARCH}/pfgprocess.o: pfg_strings.h pfg_labels.h
${ARCH}/pfgprogress.o: pfg.h
${ARCH}/pfgprogress.o: pf.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgprogress.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgprogress.o: pferror.h pfg_strings.h pfg_labels.h pfgProgress_ui.h
${ARCH}/pfgprogress.o: tu_runtime.h
${ARCH}/pfgprogressbar.o: pfg.h
${ARCH}/pfgprogressbar.o: pf.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgprogressbar.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgprogressbar.o: pferror.h pfg_strings.h pfg_labels.h pfgprogressbar.h
${ARCH}/pfgprogressbar.o: pfgProgressBar_ui.h tu_runtime.h
${ARCH}/pfgremote.o: pf.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgremote.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgremote.o: pferror.h pfg.h
${ARCH}/pfgremote.o: pfg_strings.h pfg_labels.h pfgRemote_ui.h tu_runtime.h
${ARCH}/pfgremquery.o: pf.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgremquery.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgremquery.o: pferror.h pfg.h
${ARCH}/pfgremquery.o: pfg_strings.h pfg_labels.h pfgRemquery_ui.h tu_runtime.h
${ARCH}/pfgsoftware.o: pf.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgsoftware.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgsoftware.o: pferror.h pfg.h
${ARCH}/pfgsoftware.o: pfg_strings.h pfg_labels.h pfgsoftware.h pfgSoftware_ui.h
${ARCH}/pfgsoftware.o: tu_runtime.h
${ARCH}/pfgsolarcust.o: pf.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgsolarcust.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgsolarcust.o: pferror.h pfg.h
${ARCH}/pfgsolarcust.o: pfg_strings.h pfg_labels.h pfgSolarCust_ui.h tu_runtime.h
${ARCH}/pfgsolarispart.o: pf.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgsolarispart.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgsolarispart.o: pferror.h pfg.h
${ARCH}/pfgsolarispart.o: pfg_strings.h pfg_labels.h pfgSolarPart_ui.h tu_runtime.h
${ARCH}/pfgsummary.o: pf.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgsummary.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgsummary.o: pferror.h pfg.h
${ARCH}/pfgsummary.o: pfg_strings.h pfg_labels.h pfgSummary_ui.h tu_runtime.h
${ARCH}/pfgswquery.o: pf.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgswquery.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgswquery.o: pferror.h pfg.h
${ARCH}/pfgswquery.o: pfg_strings.h pfg_labels.h pfgSwQuery_ui.h tu_runtime.h
${ARCH}/pfgtoplevel.o: pf.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgtoplevel.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgtoplevel.o: pferror.h pfg.h
${ARCH}/pfgtoplevel.o: pfg_strings.h pfg_labels.h pfgprocess.h Installtool.ad.h
${ARCH}/pfgtoplevel.o: Installtool_lowres.ad.h
${ARCH}/pfgtutor.o: pf.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgtutor.o: pferror.h pfg.h
${ARCH}/pfgtutor.o: pfg_strings.h pfg_labels.h pfgprocess.h
${ARCH}/pfgupgrade.o: pfg.h
${ARCH}/pfgupgrade.o: pf.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgupgrade.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgupgrade.o: pferror.h pfg_strings.h pfg_labels.h pfgprogressbar.h
${ARCH}/pfgupgrade.o: pfgUpgrade_ui.h tu_runtime.h
${ARCH}/pfgusedisks.o: pf.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgusedisks.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgusedisks.o: pferror.h pfg.h
${ARCH}/pfgusedisks.o: pfg_strings.h pfg_labels.h pfglocales.h pfgUseDisks_ui.h
${ARCH}/pfgusedisks.o: tu_runtime.h
${ARCH}/pfgutil.o: pf.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgutil.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/pfgutil.o: pferror.h pfg.h
${ARCH}/pfgutil.o: pfg_strings.h pfg_labels.h
${ARCH}/v_pfg_disks.o: pf.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/v_pfg_disks.o: pferror.h pfg.h
${ARCH}/v_pfg_disks.o: pfg_strings.h pfg_labels.h pfgdisks.h
${ARCH}/v_pfg_fdisk.o: pf.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_pfg_fdisk.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/v_pfg_fdisk.o: pferror.h pfg.h
${ARCH}/v_pfg_fdisk.o: pfg_strings.h pfg_labels.h pfgdisks.h
${ARCH}/v_pfg_lfs.o: pf.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/v_pfg_lfs.o: pferror.h
${ARCH}/v_pfg_rfs.o: pf.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_pfg_rfs.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/v_pfg_rfs.o: pferror.h
${ARCH}/v_pfg_sw.o: pf.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_pfg_sw.o: ${ROOTINCADMIN}/spmixm_api.h
${ARCH}/v_pfg_sw.o: pferror.h pfg.h
${ARCH}/v_pfg_sw.o: pfg_strings.h pfg_labels.h
