#ifndef lint
#ident   "@(#)soft_templates.c 1.3 96/06/07 SMI"
#endif

/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

char *platgrp_softinfo[] = {
"PLATFORM_GROUP=@ISA@:@PLATGRP@",
"",
"DryRun",
"PLATFORM_GROUP=@ISA@:@PLATGRP@",
""
};


char *platmember_softinfo[] = {
"PLATFORM_MEMBER=@PLAT@",
"",
"DryRun",
"PLATFORM_MEMBER=@PLAT@",
""
};

char *end_platform_file[] = {
"EOF",
"	logprogress @SEQ@ none",
"fi",
"",
"DryRun",
"EOF",
""
};

char *start_platform[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"rm -f ${base}@ROOT@/var/sadm/system/admin/.platform",
"touch ${base}@ROOT@/var/sadm/system/admin/.platform",
"chmod 644 ${base}@ROOT@/var/sadm/system/admin/.platform",
"cat >> ${base}@ROOT@/var/sadm/system/admin/.platform << EOF",
"",
"DryRun",
"cat >> ${base}@ROOT@/var/sadm/system/admin/.platform << EOF",
""
};

char *generic[] = {
"@LINE@",
"",
"DryRun",
"@LINE@",
""
};
