#
#ident "@(#)Makefile 1.11 96/10/09"
#
# Copyright (c) 1996, by Sun Microsystems, Inc.
# All Rights Reserved.
#

include $(SRC)/Makefile.master

# These definitions should be moved into an equivalent of ON's Makefile.psm
# located in $(SRC).
PLATFORM:sh=             uname -m

PSM_BOOT_DIR		 = $(ROOT)/platform/$(PLATFORM)/boot
PSM_BOOT_SOL_DIR	 = $(PSM_BOOT_DIR)/solaris

PSM_BOOT_DRV_DIR	 = $(PSM_BOOT_SOL_DIR)/drivers
PSM_BOOT_DRV_ISA025_DIR	 = $(PSM_BOOT_DRV_DIR)/isa.025
PSM_BOOT_DRV_ISA125_DIR	 = $(PSM_BOOT_DRV_DIR)/isa.125
PSM_BOOT_DRV_ISA150_DIR	 = $(PSM_BOOT_DRV_DIR)/isa.150
PSM_BOOT_DRV_ISA175_DIR	 = $(PSM_BOOT_DRV_DIR)/isa.175
PSM_BOOT_DRV_NOTISA010_DIR = $(PSM_BOOT_DRV_DIR)/notisa.010
PSM_BOOT_DRV_NOTISA020_DIR = $(PSM_BOOT_DRV_DIR)/notisa.020

PSM_BOOT_DB_DIR		 = $(PSM_BOOT_SOL_DIR)/devicedb
PSM_BOOT_MCA_DIR	 = $(PSM_BOOT_SOL_DIR)/mca_adf
PSM_BOOT_MACH_DIR	 = $(PSM_BOOT_SOL_DIR)/machines

PSM_BOOT_DESTDIR = $(PSM_BOOT_DIR) $(PSM_BOOT_SOL_DIR) $(PSM_BOOT_DRV_DIR) \
	$(PSM_BOOT_DRV_ISA025_DIR) $(PSM_BOOT_DRV_ISA125_DIR) \
	$(PSM_BOOT_DRV_ISA150_DIR) $(PSM_BOOT_DRV_ISA175_DIR) \
	$(PSM_BOOT_DRV_NOTISA010_DIR) $(PSM_BOOT_DRV_NOTISA020_DIR) \
	$(PSM_BOOT_DB_DIR) $(PSM_BOOT_MCA_DIR) $(PSM_BOOT_MACH_DIR)

USR_PLAT_DIR		 = $(ROOT)/usr/platform
USR_PSM_DIR		 = $(USR_PLAT_DIR)/$(PLATFORM)
USR_PSM_LIB_DIR		 = $(USR_PSM_DIR)/lib
USR_PSM_LIB_FS_DIR       = $(USR_PSM_LIB_DIR)/fs

#
#  XXX- boot.bin also needs to be here!  It may need to come from a different
#	source, though!
#
PSM_BOOT_SOL_FILES = boot.rc bootargs.rc bootconf.exe bootconf.txt \
	bootenv.rc strap.rc

PSM_BOOT_DRV_ISA025_FILES = ata.bef com.bef fdc.bef joyst.bef key.bef \
	logi.bef lpt.bef msm.bef ps2ms.bef
PSM_BOOT_DRV_ISA125_FILES = aha1540.bef aic.bef blogic.bef dpt.bef eepro.bef \
	el.bef elink.bef elx.bef pe3.bef smc.bef trantor.bef
PSM_BOOT_DRV_ISA150_FILES = esa.bef iee.bef nei.bef pcn.bef tr.bef 
PSM_BOOT_DRV_ISA175_FILES = sbpro.bef
PSM_BOOT_DRV_NOTISA010_FILES = adp.bef aha1740.bef iss.bef mlx.bef ncrs.bef \
	pcscsi.bef dnet.bef ieef.bef nee.bef smce.bef smceu.bef smcf.bef
#
# Comment out iprb.bef temporarily
#
#PSM_BOOT_DRV_NOTISA020_FILES = nfe.bef riles.bef corvette.bef mcis.bef \
#	iprb.bef chs.bef
PSM_BOOT_DRV_NOTISA020_FILES = nfe.bef riles.bef corvette.bef mcis.bef \
	chs.bef

PSM_BOOT_DB_FILES = master config.db

PSM_BOOT_MCA_FILES = @0708.adf @6042.adf @61c8.adf @61c9.adf @627c.adf \
	@627d.adf @6e6c.adf @6fc0.adf @6fc1.adf @6fc2.adf @80ec.adf @8efc.adf \
	@8efe.adf @8eff.adf @8f82.adf @8fbb.adf @8fda.adf @8fdb.adf @90ee.adf \
	@917b.adf @deff.adf @e001.adf @eee7.adf @eeff.adf @efef.adf @effd.adf \
	pdefe.adf pdffe.adf pe0fe.adf pe1fe.adf pe4fe.adf pff6f.adf pff9f.adf \
	@60e9.adf @6127.adf @8fa2.adf @8fa8.adf @8fd6.adf pfe80.adf pfebe.adf \
	pfee7.adf

DATAFILES +=
REALMODE_BOOT = bootblk mboot pboot
REALMODE_COM = gluecode.com smc.com

REALMODE_PROTO = $(REALMODE_ROOT)/proto
REALMODE_SOL = $(REALMODE_PROTO)/bootpart/solaris
REALMODE_DB = $(REALMODE_SOL)/devicedb
REALMODE_MCA = $(REALMODE_SOL)/mca_adf

BOOT_DESTDIR = $(USR_PSM_LIB_FS_DIR)/ufs
COM_DESTDIR = $(USR_PSM_LIB_FS_DIR)/nfs

INST_BOOT = $(REALMODE_BOOT:%=$(BOOT_DESTDIR)/%)
INST_COM  = $(REALMODE_COM:%=$(COM_DESTDIR)/%)
INST_SOL  = $(PSM_BOOT_SOL_FILES:%=$(PSM_BOOT_SOL_DIR)/%)
INST_MCA  = $(PSM_BOOT_MCA_FILES:%=$(PSM_BOOT_MCA_DIR)/%)
INST_DB   = $(PSM_BOOT_DB_FILES:%=$(PSM_BOOT_DB_DIR)/%)
INST_ISA25 = $(PSM_BOOT_DRV_ISA025_FILES:%=$(PSM_BOOT_DRV_ISA025_DIR)/%)
INST_ISA125 = $(PSM_BOOT_DRV_ISA125_FILES:%=$(PSM_BOOT_DRV_ISA125_DIR)/%)
INST_ISA150 = $(PSM_BOOT_DRV_ISA150_FILES:%=$(PSM_BOOT_DRV_ISA150_DIR)/%)
INST_ISA175 = $(PSM_BOOT_DRV_ISA175_FILES:%=$(PSM_BOOT_DRV_ISA175_DIR)/%)
INST_NOISA10 = $(PSM_BOOT_DRV_NOTISA010_FILES:%=$(PSM_BOOT_DRV_NOTISA010_DIR)/%)
INST_NOISA20 = $(PSM_BOOT_DRV_NOTISA020_FILES:%=$(PSM_BOOT_DRV_NOTISA020_DIR)/%)

.KEEP_STATE:

all: $(BOOT_DESTDIR) $(COM_DESTDIR) $(PSM_BOOT_DESTDIR) \
	$(INST_BOOT) $(INST_COM) $(INST_SOL) $(INST_MCA) \
	$(INST_DB) $(INST_ISA25) \
	$(INST_ISA125) $(INST_ISA150) $(INST_ISA175) \
	$(INST_NOISA10) $(INST_NOISA20)

install: all

clobber:

$(BOOT_DESTDIR) $(COM_DESTDIR) $(PSM_BOOT_DESTDIR):
	$(INS.dir)

$(PSM_BOOT_SOL_DIR)/%:	$(REALMODE_SOL)/%
	$(INS.file)

$(PSM_BOOT_DRV_ISA025_DIR)/%:	$(REALMODE_PROTO)/%
	$(INS.file)

$(PSM_BOOT_DRV_ISA125_DIR)/%:	$(REALMODE_PROTO)/%
	$(INS.file)

$(PSM_BOOT_DRV_ISA150_DIR)/%:	$(REALMODE_PROTO)/%
	$(INS.file)

$(PSM_BOOT_DRV_ISA175_DIR)/%:	$(REALMODE_PROTO)/%
	$(INS.file)

$(PSM_BOOT_DRV_NOTISA010_DIR)/%: $(REALMODE_PROTO)/%
	$(INS.file)

$(PSM_BOOT_DRV_NOTISA020_DIR)/%: $(REALMODE_PROTO)/%
	$(INS.file)

$(BOOT_DESTDIR)/%:  $(REALMODE_PROTO)/%
	$(INS.file)

$(COM_DESTDIR)/gluecode.com:    $(REALMODE_PROTO)/gluecode.com
	$(RM) $@; $(INS) -s -m $(FILEMODE) -f $(@D) $(REALMODE_PROTO)/gluecode.com

# This is BS; smc.bef and smc.com are not really doing anything during the
# boot process. Smc.com is here only for placeholding purposes.
$(COM_DESTDIR)/smc.com:    $(REALMODE_PROTO)/smc.bef
	$(RM) $@; $(INS) -s -m $(FILEMODE) -f $(@D) $(REALMODE_PROTO)/smc.bef
	$(MV) $(@D)/smc.bef $@
