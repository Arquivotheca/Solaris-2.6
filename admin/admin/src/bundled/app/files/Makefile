#
#      "@(#)Makefile 1.43 96/09/27"
#
# Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved.
#

# Makefile for miscellaneous files
#
# FILES are those things that are just used as is

include ../Makefile.cmd

FILES= inittab Xdefaults .sysIDtool.state \
	transfer_list openwin-init .root.copy \
	base_sibi.pkg shadow system i.template dial

# SCRIPTS are .sh files, use the default rules
CD_SBIN_SCRIPTS=rcS suninstall sysconfig \
	sulogin rc2 startup

CD_TOOL_SCRIPTS=add_install_client rm_install_client \
	setup_install_server dial

CD_SCRIPTS=rcS.stub stubboot profind chkprobe \
	re-preinstall parse_dynamic_clustertoc .root.copy

CD_BUILD_SCRIPTS=faspac i.template repac base_sibi.pkg \
	.mini_root.copy create_cd_root

INITD_SCRIPTS=autoinstall sysid.net sysid.sys

ROOTUSRSBIN_SCRIPTS=server_upgrade

SCRIPTS= $(CD_SBIN_SCRIPTS) $(CD_TOOL_SCRIPTS) \
	$(CD_SCRIPTS) $(INITD_SCRIPTS) $(CD_BUILD_SCRIPTS) \
	$(ROOTUSRSBIN_SCRIPTS)

OPENWINFILES=install-openwin-menu.gui

WINMISC=Xinitrc $(OPENWINFILES)

INSTFILES= Xinitrc $(CD_SCRIPTS) Xdefaults openwin-init

ETCFILES= inittab .sysIDtool.state transfer_list shadow system

VARSADM_FILES = README

ROOTINSTPROG	= $(INSTFILES:%=$(ROOTINSTALLD)/%)

ROOTCDPROG	= $(CD_TOOL_SCRIPTS:%=$(ROOTTOOLS)/%)

ROOTSBINPROG	= $(CD_SBIN_SCRIPTS:%=$(ROOTSBIN)/%)

ROOTETCPROG	= $(ETCFILES:%=$(ROOTETC)/%)

ROOTMENU	= $(OPENWINFILES:%=$(ROOTUSRMENU)/%)

ROOTINITDPROG	= $(INITD_SCRIPTS:%=$(ROOTINITD)/%)

ROOTCDBUILDPROG = $(CD_BUILD_SCRIPTS:%=$(ROOTCDBUILD)/%)

ROOTVARRM	= $(VARSADM_FILES:%=$(ROOTVARSADM)/%)

ROOTUSRSBINPROG	= $(ROOTUSRSBIN_SCRIPTS:%=$(ROOTUSRSBIN)/%)

MSG_DOMAIN=SUNW_INSTALL_SCRIPTS

.KEEP_STATE:

all: ${FILES} ${SCRIPTS}

install: all $(ROOTCDPROG) $(ROOTSBINPROG) $(ROOTINSTPROG) $(ROOTMENU) \
	$(ROOTETCPROG) $(ROOTINITDPROG) $(ROOTCDBUILDPROG) \
	$(ROOTUSRSBINPROG) msgs $(INSTMSGS) $(ROOTVARRM)

clean:

clobber:
	-rm -f ${SCRIPTS} *.po *.mo

msgs: ${MSG_DOMAIN}.po

${MSG_DOMAIN}.po: ${SCRIPTS}
	@$(ROOTADMINBIN)/xgetsh -d $(MSG_DOMAIN) ${SCRIPTS} ${WINMISC}

$(ROOTVARSADM)/%: %
	$(INS.file)

.root.copy: root.copy
	@${COMPILE.c} -E root.copy 2>/dev/null | sed -e "/^$$/d" -e "/^#/d" -e "/^[ 	]/d"  > $@

.mini_root.copy: mini_root.copy
	@${COMPILE.c} -E mini_root.copy 2>/dev/null | sed -e "/^$$/d" -e "/^#/d" -e "/^[ 	]/d"  > $@

base_sibi.pkg: base_sibi.pkg.sh
	${COMPILE.c} -E base_sibi.pkg.sh 2>/dev/null | sed -e "/^$$/d" -e "/^#/d" -e "/^[ 	]/d"  > $@

.sysIDtool.state: sysIDtool.state
	@rm -f $@
	@cp sysIDtool.state $@

include ../Makefile.targ
