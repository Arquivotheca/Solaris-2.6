/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident   "@(#)showrev.h 1.2     95/02/09 SMI"

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<cl_system_errs.h>
#include<sys/param.h>
#include<sys/utsname.h>
#include<sys/systeminfo.h>

/* headers */
void do_kernel();
void do_window();
void do_file_type();
void do_command_rev();
void do_file_perms();
void do_elf();
void do_checksum();
void do_c(char *filename);

