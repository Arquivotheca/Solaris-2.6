/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef _VALID_H
#define _VALID_H

#pragma	ident	"@(#)valid.h	1.14	95/06/05 SMI"

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef __STDC__
extern int valid_host_ip_addr(const char *);
extern int valid_ip_netmask(const char *);
extern int valid_ip_netnum(const char *);
extern int valid_host_ether_addr(const char *);
extern int valid_domainname(const char *);
extern int valid_hostname(const char *);
extern int valid_printer_name(const char *);
extern int valid_description(const char *);
extern int valid_printerport(const char *);
extern int valid_printertype(const char *);
extern int valid_timezone(const char *);
extern int valid_policy(const char *);
extern int valid_gname(const char *);
extern int valid_gid(const char *);
extern int valid_uname(const char *);
extern int valid_home_path(const char *);
extern int valid_gcos(const char *);
extern int valid_int(const char *);
extern int valid_unsigned_int(const char *);
extern int valid_proto_name(const char *);
extern int valid_uid(const char *);
extern int valid_shell(const char *);
extern int valid_proto_num(const char *);
extern int valid_rpc_num(const char *);
extern int valid_rpc_name(const char *);
extern int valid_port_num(const char *);
extern int valid_service_name(const char *);
extern int valid_path(const char *);
extern int valid_auto_home_path(const char *);
extern int valid_bootparams_key(const char *);
extern int valid_printer_allow_list( const char *);
extern int valid_group_members(const char *);
extern int valid_mail_alias(const char *);
extern int valid_netname(const char *);
extern int valid_passwd(const char *);
extern char *normalize_ether(const char *);
extern char *normalize_ip(const char *);
extern char *normalize_gid(const char *);
extern char *normalize_uid(const char *);
#else
extern int valid_host_ip_addr();
extern int valid_ip_netmask();
extern int valid_ip_netnum();
extern int valid_host_ether_addr();
extern int valid_domainname();
extern int valid_hostname();
extern int valid_printer_name();
extern int valid_description();
extern int valid_printerport();
extern int valid_printertype();
extern int valid_timezone();
extern int valid_policy();
extern int valid_gname();
extern int valid_gid();
extern int valid_uname();
extern int valid_home_path();
extern int valid_gcos();
extern int valid_int();
extern int valid_unsigned_int();
extern int valid_proto_name();
extern int valid_uid();
extern int valid_shell();
extern int valid_proto_num();
extern int valid_rpc_num();
extern int valid_rpc_name();
extern int valid_port_num();
extern int valid_service_name();
extern int valid_path();
extern int valid_auto_home_path();
extern int valid_bootparams_key();
extern int valid_printer_allow_list();
extern int valid_group_members();
extern int valid_mail_alias();
extern int valid_netname();
extern int valid_passwd();
extern char *normalize_ether();
extern char *normalize_ip();
extern char *normalize_gid();
extern char *normalize_uid();
#endif /* __STDC__ */

#ifdef  __cplusplus
}
#endif

#endif	/* !_VALID_H */

