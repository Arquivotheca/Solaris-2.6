#! /bin/sh
#
# ident	"@(#)prep_partition.sh	1.1	95/05/08 SMI"
#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
# 
# NOTICE: THIS SCRIPT IS NOT PUBLIC.
#

trap "rm -f /tmp/$$.fdisk /tmp/$$.awk" 0

usage() {
	echo usage: $0 "/dev/rdsk/c?t?d?p0" 1>&2
	exit 255
}

case $# in
1)
	;;
*)
	usage
	;;
esac

case "$1" in
-t)
	cat > /tmp/$$.fdisk
	get_table="true"
	;;
-*)
	usage
	;;
*)
	get_table="fdisk -W /tmp/$$.fdisk $1"
	;;
esac

cat << '/' > /tmp/$$.awk
BEGIN {
    DOSOS12 = 1;
    PCIXOS = 2;
    DOSOS16 = 4;
    EXTDOS = 5;
    DOSBIG = 6;
    PPCBOOT = 65;
    DOSDATA = 86;
    OTHEROS = 98;
    UNIXOS = 99;
    SUNIXOS = 130;

    hex41_size = 2047;
    fat12_size = 6144;
    min_solaris_size = 40*1024;	# 20M at 512/sector, for sanity
    start_sector = 1;
}

$1=="*" && $3=="sectors/track" { sec_per_track = $2; }
$1=="*" && $3=="tracks/cylinder" { track_per_cyl = $2; }
$1=="*" && $3=="cylinders" { cyls = $2; }
$1==PPCBOOT { existing_hex41 = $10; }
$1==DOSOS12 { existing_fat12 = $10; }
$1==SUNIXOS { existing_solaris = $10; }
$1>0 { existing++; }

END {
	if (existing_hex41 >= hex41size \
	&& existing_fat12 >= fat12_size \
	&& existing_solaris >= min_solaris_size) {
		# Recommend using existing partitioning
		exit 2;
	}

	printf "* Id    Act  Bhead  Bsect  Bcyl    Ehead  Esect  Ecyl    Rsect    Numsect\n";
	total = sec_per_track * track_per_cyl * cyls;
	fmt= "  %-5d %-4d %-6d %-6d %-7d %-6d %-6d %-7d %-7d %-8d\n";
	sect = start_sector;
	printf fmt, PPCBOOT,   0, 0,0,0, 0,0,0, sect, hex41_size;
	sect += hex41_size;
	printf fmt, DOSOS12, 128, 0,0,0, 0,0,0, sect, fat12_size;
	sect += fat12_size;
	printf fmt, SUNIXOS,   0, 0,0,0, 0,0,0, sect, total - sect;

	if (existing) exit 1;
	exit 0;
}

# * /dev/rdsk/c0t6d0p0 default fdisk table
# * Dimensions:
# *     512 bytes/sector
# *      87 sectors/track
# *       7 tracks/cylinder
# *     1750 cylinders
# *
# * systid:
# *   1:  DOSOS12
# *   2:  PCIXOS
# *   4:  DOSOS16
# *   5:  EXTDOS
# *   6:  DOSBIG
# *   65: PPCBOOT
# *   86: DOSDATA
# *   98: OTHEROS
# *   99: UNIXOS
# *  130: SUNIXOS
# *
# 
# * Id    Act  Bhead  Bsect  Bcyl    Ehead  Esect  Ecyl    Rsect    Numsect
#   65    0    0      2      0       16     2      1       1        2047    
#   1     128  16     3      1       13     8      7       2048     6144    
#   130   0    13     9      7       2      32     1011    8192     1057558 
# 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456
# 
/

if $get_table
then
    awk -f /tmp/$$.awk /tmp/$$.fdisk
    case $? in
    0)
	# It recommended something safe
	exit 0
	;;
    1)
	# It recommended something destructive
	exit 1
	;;
    2)
	# It recommended using existing partitions
	cat /tmp/$$.fdisk
	exit 0
	;;
    esac
else
    # fdisk will have printed something
    echo $0:  fdisk failed 1>&2
    exit 255
fi

exit 2
