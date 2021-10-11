#
# Copyright (c) 1996, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.128 96/10/10"
# ttinstall makefile

include ../../Makefile.cmd

PROG=ttinstall

SRCS = c_progress.c \
	disk_fs_util.c \
	evlog.c \
	inst_alt_lang.c \
	inst_bootdisk.c \
	inst_check.c \
	inst_client_arch.c \
	inst_disk_edit.c \
	inst_disk_use.c \
	inst_disk_prep.c \
	inst_dsr_al.c \
	inst_dsr_analyze.c \
	inst_dsr_fsredist.c \
	inst_dsr_fssummary.c \
	inst_dsr_media.c \
	inst_dsr_space_req.c \
	inst_fs_auto.c \
	inst_fs_preserve.c \
	inst_fs_summary.c \
	inst_intro.c \
	inst_parade.c \
	inst_progressbar.c \
	inst_rfs.c \
	inst_space.c \
	inst_space_meter.c \
	inst_alloc_client.c \
	inst_summary.c \
	inst_sw_choice.c \
	inst_sw_edit.c \
	inst_systype.c \
	tty_pfc.c \
	rfs_util.c \
	summary_util.c \
	upg_summary.c \
	pfgtutor.c \
	pfgprocess.c \
	v_pfg_lfs.c \
	v_pfg_disks.c \
	v_check.c \
	v_fdisk.c \
	v_lfs.c \
	v_misc.c \
	v_rfs.c \
	v_sdisk.c \
	v_sw.c \
	v_upgrade.c

OBJS =	$(SRCS:%.c=${ARCH}/%.o)

# pick up all current .h files
HDRS	:sh= ls *.h

SUBDIRS = HelpFiles

all:=		TARGET= all
install:=	TARGET= install
clean:=		TARGET= clean
clobber:=	TARGET= clobber

LIBDIR		= $(ROOTADMINLIB)
LIBDIRS         = -L. -L$(LIBDIR)

LIBS		= \
		$(LIBDIR)/libspmitty.a \
		$(LIBDIR)/libspmiapp.a \
		$(LIBDIR)/libspmisvc.a \
		$(LIBDIR)/libspmistore.a \
		$(LIBDIR)/libspmisoft.a \
		$(LIBDIR)/libspmicommon.a

INCLUDES        = -I$(ROOTINCADMIN)

CPPFLAGS	+= ${INCLUDES}
CFLAGS		+= $(DEBUG_CFLAGS)
LDFLAGS		+= $(DEBUG_CFLAGS) -R$(ROOTADMINLIB:$(ROOT)%=%) ${LIBDIRS}

# lint
LINT_ERR = lint.errs
LINT_OPTS = -umx
LINTFLAGS  = ${LINT_OPTS}

# cstyle
CSTYLE_ERR = cstyle.errs

PURIFYOPTS      = -logfile="/tmp/errs.%p"
PURIFY          = purify $(PURIFYOPTS)

MSG_DOMAIN	= SUNW_INSTALL_TTINSTALL

TERMLIB = -ltermlib
APPLIBS = -lspmitty -lspmiapp -lspmisvc -lspmisoft -lspmistore -lspmicommon

LDLIBS +=	${APPLIBS} \
		${TERMLIB} \
		-lcurses -lsocket -ldl -ldevinfo -lnsl -ladm -lkvm -lelf

DYNAMICLDLIBS = ${TERMLIB} \
		-lcurses -lsocket -ldl -ldevinfo -lnsl -ladm -lkvm -lelf

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all: 	$(ARCH) .WAIT $(PROG) .WAIT
	@true

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

pure:   $(PROG).pure
	@true

install:	all $(ROOTUSRSBINPROG) msgs $(INSTMSGS) $(SUBDIRS)

$(PROG):	${ARCH} .WAIT $(OBJS) $(DYNLIBS)
		$(LINK.c) -o $@ $(OBJS) \
		$(LDLIBS)
		cp $@ ${ARCH}
		$(POST_PROCESS)

$(PROG).static:	${ARCH} .WAIT $(OBJS) $(DYNLIBS)
		$(LINK.c) -o $@ $(OBJS) \
		-ustart_pkgadd -uend_pkgadd -uprogress_init -uprogress_done \
		-uprogress_cleanup -uinteractive_pkgrm -uinteractive_pkgadd \
		-Bstatic ${APPLIBS} -Bdynamic $(DYNAMICLDLIBS)
		cp $@ ${ARCH}
		$(POST_PROCESS)


$(PROG).pure:	$(OBJS) $(DYNLIBS)
		$(PURIFY) $(LINK.c) -o $@ $(OBJS) \
		-ustart_pkgadd -uend_pkgadd -uprogress_init -uprogress_done \
		-uprogress_cleanup -uinteractive_pkgrm -uinteractive_pkgadd \
		$(LDLIBS) -lw

lint:
	${LINT.c} ${SRCS} > ${LINT_ERR} 2>&1

cstyle:
	@echo Running cstyle... See ${CSTYLE_ERR} for errors
	@rm -f ${CSTYLE_ERR}
	@for i in ${SRCS} ${HDRS}; do \
		echo "------------ $${i} ------------" >> ${CSTYLE_ERR}; \
		cstyle $${i} >> ${CSTYLE_ERR} 2>&1; \
		echo "" >> ${CSTYLE_ERR}; \
		echo "" >> ${CSTYLE_ERR}; \
        done;

depend:
	makedepend $(INCLUDES) $(SRCS)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

#
# Create a message file for domain "install".
#
# Since the app may use strings exported from the libraries,
# we could actually end up generating 'extraneous' .po files here
# (e.g. SUNW_INSTALL_LIBAPP.po).
# We delete all but the app's .po here.
#
msgs:		${MSG_DOMAIN}.po

${MSG_DOMAIN}.po:	${SRCS} ${HDRS}
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
msgs_test: ${SRCS} ${HDRS}
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
	-rm -f *.i *.po *.mo

clean:
	-rm -f core errs *.o *.BAK *.$(LINTERR) *.ln \
		$(UI_LIB) *~ *% .*~ .*% ${OBJS}

clobber: clean clean_msgs
	@-rm -f $(PROG)
	rm -rf ${ARCHSUPPORT}

include ../../Makefile.targ

.WAIT:
# DO NOT DELETE
# makedepend dependencies modified using:
# /usr/rmtc/misc/spmi/bin/deps.pl, Version: 1.2
# on Thu Jul 11 17:07:30 US/Mountain 1996

${ARCH}/c_progress.o: pf.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/c_progress.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/c_progress.o: inst_parade.h c_progress.h v_types.h v_sw.h v_misc.h
${ARCH}/disk_fs_util.o: pf.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/disk_fs_util.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/disk_fs_util.o: inst_parade.h v_types.h v_disk.h v_lfs.h v_misc.h
${ARCH}/disk_fs_util.o: disk_fs_util.h
${ARCH}/evlog.o: evlog.h
${ARCH}/inst_alt_lang.o: pf.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_alt_lang.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_alt_lang.o: inst_parade.h tty_pfc.h inst_msgs.h v_types.h v_sw.h
${ARCH}/inst_bootdisk.o: pf.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_bootdisk.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_bootdisk.o: inst_parade.h tty_pfc.h v_types.h v_check.h v_disk.h v_lfs.h
${ARCH}/inst_bootdisk.o: v_sw.h v_misc.h inst_msgs.h disk_fs_util.h
${ARCH}/inst_check.o: pf.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_check.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_check.o: inst_parade.h tty_pfc.h inst_msgs.h v_types.h v_check.h v_sw.h
${ARCH}/inst_check.o: v_disk.h
${ARCH}/inst_client_arch.o: pf.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_client_arch.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_client_arch.o: inst_parade.h tty_pfc.h inst_msgs.h v_types.h v_sw.h
${ARCH}/inst_disk_edit.o: pf.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_disk_edit.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_disk_edit.o: inst_parade.h tty_pfc.h v_types.h v_check.h v_disk.h
${ARCH}/inst_disk_edit.o: v_lfs.h v_sw.h inst_msgs.h
${ARCH}/inst_disk_use.o: pf.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_disk_use.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_disk_use.o: inst_parade.h tty_pfc.h v_types.h v_check.h v_disk.h v_lfs.h
${ARCH}/inst_disk_use.o: v_sw.h v_misc.h inst_msgs.h disk_fs_util.h
${ARCH}/inst_disk_prep.o: pf.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_disk_prep.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_disk_prep.o: inst_parade.h tty_pfc.h v_types.h v_disk.h inst_msgs.h
${ARCH}/inst_dsr_al.o: pf.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_dsr_al.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_dsr_al.o: inst_parade.h inst_progressbar.h inst_msgs.h
${ARCH}/inst_dsr_analyze.o: pf.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_dsr_analyze.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_dsr_analyze.o: inst_parade.h tty_pfc.h inst_msgs.h inst_progressbar.h
${ARCH}/inst_dsr_fsredist.o: pf.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_dsr_fsredist.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_dsr_fsredist.o: inst_parade.h tty_pfc.h inst_msgs.h v_types.h v_misc.h
${ARCH}/inst_dsr_fsredist.o: v_sw.h
${ARCH}/inst_dsr_fssummary.o: pf.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_dsr_fssummary.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_dsr_fssummary.o: inst_parade.h tty_pfc.h inst_msgs.h
${ARCH}/inst_dsr_media.o: pf.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_dsr_media.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_dsr_media.o: inst_parade.h tty_pfc.h inst_msgs.h v_types.h v_misc.h
${ARCH}/inst_dsr_media.o: v_sw.h
${ARCH}/inst_dsr_space_req.o: pf.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_dsr_space_req.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_dsr_space_req.o: inst_parade.h tty_pfc.h inst_msgs.h
${ARCH}/inst_fs_auto.o: pf.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_fs_auto.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_fs_auto.o: inst_parade.h tty_pfc.h v_types.h v_check.h v_disk.h v_lfs.h
${ARCH}/inst_fs_auto.o: v_sw.h v_misc.h inst_msgs.h disk_fs_util.h
${ARCH}/inst_fs_preserve.o: pf.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_fs_preserve.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_fs_preserve.o: inst_parade.h tty_pfc.h inst_msgs.h disk_fs_util.h
${ARCH}/inst_fs_preserve.o: v_types.h v_check.h v_disk.h v_lfs.h v_sw.h v_misc.h
${ARCH}/inst_fs_summary.o: pf.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_fs_summary.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_fs_summary.o: inst_parade.h tty_pfc.h v_types.h v_disk.h v_lfs.h v_sw.h
${ARCH}/inst_fs_summary.o: inst_msgs.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_intro.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_intro.o: tty_pfc.h pf.h inst_parade.h inst_msgs.h
${ARCH}/inst_parade.o: pf.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_parade.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_parade.o: inst_parade.h tty_pfc.h inst_msgs.h v_check.h v_types.h
${ARCH}/inst_parade.o: v_lfs.h v_disk.h v_sw.h v_misc.h v_upgrade.h
${ARCH}/inst_progressbar.o: pf.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_progressbar.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_progressbar.o: inst_parade.h inst_progressbar.h
${ARCH}/inst_rfs.o: pf.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_rfs.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_rfs.o: inst_parade.h tty_pfc.h inst_msgs.h rfs_util.h v_types.h v_rfs.h
${ARCH}/inst_rfs.o: v_sw.h
${ARCH}/inst_space.o: pf.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_space.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_space.o: inst_parade.h inst_msgs.h v_types.h v_disk.h v_lfs.h
${ARCH}/inst_space_meter.o: pf.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_space_meter.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_space_meter.o: inst_parade.h inst_msgs.h disk_fs_util.h v_types.h
${ARCH}/inst_space_meter.o: v_disk.h v_lfs.h
${ARCH}/inst_alloc_client.o: pf.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_alloc_client.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_alloc_client.o: inst_parade.h tty_pfc.h inst_msgs.h v_types.h v_misc.h
${ARCH}/inst_summary.o: pf.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_summary.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_summary.o: inst_parade.h tty_pfc.h inst_msgs.h summary_util.h v_types.h
${ARCH}/inst_summary.o: v_check.h v_rfs.h v_lfs.h v_sw.h v_disk.h v_misc.h
${ARCH}/inst_sw_choice.o: pf.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_sw_choice.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_sw_choice.o: inst_parade.h tty_pfc.h v_types.h v_disk.h v_lfs.h v_sw.h
${ARCH}/inst_sw_choice.o: inst_msgs.h
${ARCH}/inst_sw_edit.o: pf.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_sw_edit.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_sw_edit.o: inst_parade.h inst_msgs.h v_check.h v_types.h v_disk.h v_sw.h
${ARCH}/inst_systype.o: pf.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/inst_systype.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/inst_systype.o: inst_parade.h tty_pfc.h inst_msgs.h v_types.h v_misc.h v_sw.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_pfc.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_pfc.o: tty_pfc.h
${ARCH}/rfs_util.o: pf.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/rfs_util.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/rfs_util.o: inst_parade.h tty_pfc.h inst_msgs.h rfs_util.h v_types.h
${ARCH}/rfs_util.o: v_check.h v_rfs.h v_sw.h
${ARCH}/summary_util.o: pf.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/summary_util.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/summary_util.o: inst_parade.h inst_msgs.h summary_util.h v_types.h v_rfs.h
${ARCH}/summary_util.o: v_lfs.h v_sw.h v_disk.h v_misc.h
${ARCH}/upg_summary.o: pf.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/upg_summary.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/upg_summary.o: inst_parade.h tty_pfc.h pfgprocess.h inst_msgs.h
${ARCH}/upg_summary.o: summary_util.h v_types.h v_disk.h v_misc.h v_sw.h v_upgrade.h
${ARCH}/upg_summary.o: inst_progressbar.h
${ARCH}/pfgtutor.o: pf.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgtutor.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgtutor.o: inst_parade.h pfgprocess.h
${ARCH}/pfgprocess.o: pf.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/pfgprocess.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/pfgprocess.o: inst_parade.h inst_msgs.h summary_util.h v_check.h v_types.h
${ARCH}/pfgprocess.o: v_misc.h v_upgrade.h
${ARCH}/v_pfg_lfs.o: pf.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_pfg_lfs.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_pfg_lfs.o: inst_parade.h
${ARCH}/v_pfg_disks.o: pf.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_pfg_disks.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_pfg_disks.o: inst_parade.h pferror.h inst_msgs.h v_types.h v_disk.h v_lfs.h
${ARCH}/v_check.o: pf.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_check.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_check.o: inst_parade.h v_types.h v_disk.h v_check.h v_misc.h v_sw.h v_lfs.h
${ARCH}/v_check.o: v_rfs.h
${ARCH}/v_fdisk.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_fdisk.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_fdisk.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_fdisk.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_fdisk.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_fdisk.o: v_types.h v_disk.h v_disk_private.h
${ARCH}/v_lfs.o: pf.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_lfs.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_lfs.o: inst_parade.h v_types.h v_disk.h v_misc.h v_lfs.h v_upgrade.h
${ARCH}/v_misc.o: pf.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_misc.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_misc.o: inst_parade.h v_types.h v_misc.h v_disk.h v_lfs.h v_rfs.h v_sw.h
${ARCH}/v_rfs.o: pf.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_rfs.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_rfs.o: inst_parade.h v_types.h v_rfs.h v_misc.h
${ARCH}/v_sdisk.o: pf.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_sdisk.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_sdisk.o: inst_parade.h v_types.h v_misc.h v_disk.h v_lfs.h v_disk_private.h
${ARCH}/v_sw.o: pf.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_sw.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_sw.o: inst_parade.h v_types.h v_misc.h v_sw.h v_lfs.h v_upgrade.h
${ARCH}/v_upgrade.o: pf.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmitty_api.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmisvc_api.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/v_upgrade.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/v_upgrade.o: inst_parade.h inst_msgs.h v_types.h v_misc.h v_sw.h v_lfs.h
${ARCH}/v_upgrade.o: v_disk.h v_upgrade.h
