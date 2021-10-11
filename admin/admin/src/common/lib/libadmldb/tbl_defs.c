/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)tbl_defs.c	1.29	96/08/02 SMI"

#include <admldb.h>
#include <admldb_impl.h>
#include <admldb_msgs.h>
#include <valid.h>

extern int compare_ufs_col0();
extern int compare_ufs_col1();
extern int compare_nisplus_col0();
extern int compare_nisplus_col0_ci();
extern int compare_nisplus_col1();
extern int compare_nisplus_col1_ci();
extern int compare_nisplus_aliased();
extern int compare_nisplus_services();
extern int list_table_impl();
extern int set_table_impl();
extern int remove_table_impl();

struct tbl_trans_data auto_home_trans = {
	DB_AUTO_HOME_TBL,
	{ "/etc/auto_home", "auto.home", "auto_home.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 1, -1, &compare_ufs_col0, 2,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_PATH_PAR },
			}
		},
		{ 1, 1, -1, &compare_ufs_col0, 2,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_PATH_PAR },
			}
		},
		{ 0, -1, -1, &compare_nisplus_col0, 2,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_PATH_PAR },
			}
		}
	},
	{ 1, { DB_OLD_USERNAME_PAR, DB_USERNAME_PAR, 1, { 0, 0, 0 } } },
	{ 2, { { DB_USERNAME_PAR, valid_uname, DB_ERR_BAD_USERNAME }, 
	     { DB_PATH_PAR, valid_auto_home_path, DB_ERR_BAD_PATH } } },
	{
		{ &list_table_impl, DB_LIST_AUTO_HOME_MTHD },
		{ &list_table_impl, DB_GET_AUTO_HOME_MTHD },
		{ &set_table_impl, DB_SET_AUTO_HOME_MTHD },
		{ &remove_table_impl, DB_REMOVE_AUTO_HOME_MTHD }
	},
};

struct tbl_trans_data bootparams_trans = {
	DB_BOOTPARAMS_TBL,
	{ "/etc/bootparams", "bootparams", "bootparams.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 1, -1, &compare_ufs_col0, 2,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_FS_PAR },
			}
		},
		{ 1, 1, -1, &compare_ufs_col0, 2,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_FS_PAR },
			}
		},
		{ 0, -1, -1, &compare_nisplus_col0, 2,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_FS_PAR },
			}
		}
	},
	{ 1, { DB_OLD_HOSTNAME_PAR, DB_HOSTNAME_PAR, 1, { 0, 0, 0 } } },
	{ 2, { { DB_HOSTNAME_PAR, valid_bootparams_key, DB_ERR_BAD_HOSTNAME }, 
	     { DB_FS_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_BOOTPARAMS_MTHD },
		{ &list_table_impl, DB_GET_BOOTPARAMS_MTHD },
		{ &set_table_impl, DB_SET_BOOTPARAMS_MTHD },
		{ &remove_table_impl, DB_REMOVE_BOOTPARAMS_MTHD }
	},
};

struct tbl_trans_data cred_trans = {
	DB_CRED_TBL,
	{ NULL, NULL, "cred.org_dir",
	  ":", NULL, 0 },
	{
		{ 0 },
		{ 0 },
		{ 0, -1, -1, &compare_nisplus_col0, 5,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_CREDNAME_PAR, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_AUTH_TYPE_PAR },
				{ 0, DB_CASE_INSENSITIVE, DB_AUTH_NAME_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_PUBLIC_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_PRIVATE_PAR },
			}
		}
	},
	{ 3, { { DB_OLD_CREDNAME_PAR, DB_CREDNAME_PAR,   1, { 0, 0, 0 } }, 
	       { DB_OLD_AUTH_TYPE_PAR, DB_AUTH_TYPE_PAR, 0, { 1, 1, 1 } }, 
	       { DB_OLD_AUTH_NAME_PAR, DB_AUTH_NAME_PAR, 0, { 2, 2, 2 } }
	} },
	{ 5, { { DB_CREDNAME_PAR, NULL, 0 }, { DB_AUTH_TYPE_PAR, NULL, 0 }, 
	     { DB_AUTH_NAME_PAR, NULL, 0 }, { DB_PUBLIC_PAR, NULL, 0 },
	     { DB_PRIVATE_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_CRED_MTHD },
		{ &list_table_impl, DB_GET_CRED_MTHD },
		{ &set_table_impl, DB_SET_CRED_MTHD },
		{ &remove_table_impl, DB_REMOVE_CRED_MTHD }
	},
};

struct tbl_trans_data ethers_trans = {
	DB_ETHERS_TBL,
	{ "/etc/ethers", "ethers.byaddr", "ethers.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, -1, 2, &compare_ufs_col1, 3,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_ETHERADDR_PAR, 1, 0, 0 },
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 1, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_ufs_col1, 3,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_ETHERADDR_PAR, 1, 0, 0 },
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 1, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_nisplus_col1_ci, 3,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_ETHERADDR_PAR, 1 },
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 2, { { DB_OLD_ETHERADDR_PAR, DB_ETHERADDR_PAR, 0, { 0, 0, 0 } }, 
	  { DB_OLD_HOSTNAME_PAR, DB_HOSTNAME_PAR, 1, { 1, 1, 1 } } } },
	{ 3, { { DB_ETHERADDR_PAR, valid_host_ether_addr, DB_ERR_BAD_ETHER_ADDR }, 
	     { DB_HOSTNAME_PAR, valid_hostname, DB_ERR_BAD_HOSTNAME }, 
	     { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_ETHERS_MTHD },
		{ &list_table_impl, DB_GET_ETHERS_MTHD },
		{ &set_table_impl, DB_SET_ETHERS_MTHD },
		{ &remove_table_impl, DB_REMOVE_ETHERS_MTHD }
	},
};

struct tbl_trans_data group_trans = {
	DB_GROUP_TBL,
	{ "/etc/group", "group.bygid", "group.org_dir",
	  ":", NULL, 1 },
	{
		{ 0, -1, -1, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_SENSITIVE, DB_GROUPNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_PASSWD_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_GID_PAR, 1, 2, 2 },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
			}
		},
		{ 0, -1, -1, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_SENSITIVE, DB_GROUPNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_PASSWD_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_GID_PAR, 1, 2, 2 },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
			}
		},
		{ 0, -1, -1, &compare_nisplus_col0, 4,
			{
				{ 1, DB_CASE_SENSITIVE, DB_GROUPNAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_PASSWD_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_GID_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
			}
		}
	},
	{ 2, { { DB_OLD_GROUPNAME_PAR, DB_GROUPNAME_PAR, 0, { 0, 0, 0 } }, 
	  { DB_OLD_GID_PAR, DB_GID_PAR, 1, { 2, 2, 2 } } } },
	{ 4, { { DB_GROUPNAME_PAR, valid_gname, DB_ERR_BAD_GROUPNAME }, 
	     { DB_PASSWD_PAR, valid_passwd, DB_ERR_BAD_PASSWD }, 
	     { DB_GID_PAR, valid_gid, DB_ERR_BAD_GID },
	     { DB_MEMBERS_PAR, valid_group_members, DB_ERR_BAD_MEMBERS } } },
	{
		{ &list_table_impl, DB_LIST_GROUP_MTHD },
		{ &list_table_impl, DB_GET_GROUP_MTHD },
		{ &set_table_impl, DB_SET_GROUP_MTHD },
		{ &remove_table_impl, DB_REMOVE_GROUP_MTHD }
	},
};

struct tbl_trans_data hosts_trans = {
	DB_HOSTS_TBL,
	{ "/etc/inet/hosts", "hosts.byaddr", "hosts.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 2, 3, &compare_ufs_col1, 4,
			{
				{ 1, DB_CASE_SENSITIVE, DB_IPADDR_PAR, 1, 0, 0 },
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 1, 2 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, 2, 3, &compare_ufs_col1, 4,
			{
				{ 1, DB_CASE_SENSITIVE, DB_IPADDR_PAR, 1, 0, 0 },
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 1, 2 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, 1, 3, &compare_nisplus_aliased, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_IPADDR_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 2, { { DB_OLD_IPADDR_PAR, DB_IPADDR_PAR, 0, { 0, 0, 2 } }, 
	  { DB_OLD_HOSTNAME_PAR, DB_HOSTNAME_PAR, 1, { 1, 1, 1 } } } },
	{ 4, { { DB_IPADDR_PAR, valid_host_ip_addr, DB_ERR_BAD_IP_ADDR }, 
	     { DB_HOSTNAME_PAR, valid_hostname, DB_ERR_BAD_HOSTNAME }, 
	     { DB_ALIASES_PAR, NULL, 0 }, { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_HOSTS_MTHD },
		{ &list_table_impl, DB_GET_HOSTS_MTHD },
		{ &set_table_impl, DB_SET_HOSTS_MTHD },
		{ &remove_table_impl, DB_REMOVE_HOSTS_MTHD }
	},
};

struct tbl_trans_data locale_trans = {
	DB_LOCALE_TBL,
	{ "/etc/locale", "locale.byname", "locale.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, -1, 2, &compare_ufs_col0, 3, 
			{ 
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_LOCALE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		 },
		{ 0, -1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_LOCALE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_nisplus_col0_ci, 3,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_LOCALE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 1, { DB_OLD_HOSTNAME_PAR, DB_HOSTNAME_PAR, 1, { 0, 0, 0 } } },
	{ 3, { { DB_HOSTNAME_PAR, valid_hostname, DB_ERR_BAD_HOSTNAME }, 
	     { DB_LOCALE_PAR, NULL, 0 }, { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_LOCALE_MTHD },
		{ &list_table_impl, DB_GET_LOCALE_MTHD },
		{ &set_table_impl, DB_SET_LOCALE_MTHD },
		{ &remove_table_impl, DB_REMOVE_LOCALE_MTHD }
	},
};

struct tbl_trans_data mail_aliases_trans = {
	DB_MAIL_ALIASES_TBL,
	{ "/etc/mail/aliases", "mail.aliases", "mail_aliases.org_dir",
	  ":", DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_ALIAS_NAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_EXPANSION_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, -1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_ALIAS_NAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_EXPANSION_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_nisplus_col0, 4,
			{
				{ 1, DB_CASE_SENSITIVE, DB_ALIAS_NAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_EXPANSION_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_OPTIONS_PAR },
			}
		}
	},
	{ 1, { DB_OLD_ALIAS_NAME_PAR, DB_ALIAS_NAME_PAR, 1, { 0, 0, 0 } } },
	{ 4, { { DB_ALIAS_NAME_PAR, valid_mail_alias, DB_ERR_BAD_MAIL_ALIAS }, 
	     { DB_EXPANSION_PAR, NULL, 0 }, 
	     { DB_COMMENT_PAR, NULL, 0 }, { DB_OPTIONS_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_ALIASES_MTHD },
		{ &list_table_impl, DB_GET_ALIASES_MTHD },
		{ &set_table_impl, DB_SET_ALIASES_MTHD },
		{ &remove_table_impl, DB_REMOVE_ALIASES_MTHD }
	},
};

struct tbl_trans_data netgroup_trans = {
	DB_NETGROUP_TBL,
	{ "/etc/netgroup", "netgroup", "netgroup.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_NETGROUP_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, 1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_NETGROUP_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, 1, 5, &compare_nisplus_col0, 6,
			{
				{ 1, DB_CASE_SENSITIVE, DB_NETGROUP_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MEMBERS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 1, { DB_OLD_NETGROUP_PAR, DB_NETGROUP_PAR, 1, { 0, 0, 0 } } },
	{ 3, { { DB_NETGROUP_PAR, NULL, 0 }, { DB_MEMBERS_PAR, NULL, 0 }, 
	     { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_NETGROUP_MTHD },
		{ &list_table_impl, DB_GET_NETGROUP_MTHD },
		{ &set_table_impl, DB_SET_NETGROUP_MTHD },
		{ &remove_table_impl, DB_REMOVE_NETGROUP_MTHD }
	},
};

struct tbl_trans_data netmasks_trans = {
	DB_NETMASKS_TBL,
	{ "/etc/inet/netmasks", "netmasks.byaddr", "netmasks.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, -1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_NETNUM_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_NETMASK_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, -1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_NETNUM_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_NETMASK_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_nisplus_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_NETNUM_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_NETMASK_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 1, { DB_OLD_NETNUM_PAR, DB_NETNUM_PAR, 1, { 0, 0, 0 } } },
	{ 3, { { DB_NETNUM_PAR, valid_ip_netnum, DB_ERR_BAD_IP_NETNUM }, 
	     { DB_NETMASK_PAR, valid_ip_netmask, DB_ERR_BAD_IP_NETMASK }, 
	     { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_NETMASKS_MTHD },
		{ &list_table_impl, DB_GET_NETMASKS_MTHD },
		{ &set_table_impl, DB_SET_NETMASKS_MTHD },
		{ &remove_table_impl, DB_REMOVE_NETMASKS_MTHD }
	},
};

struct tbl_trans_data networks_trans = {
	DB_NETWORKS_TBL,
	{ "/etc/inet/networks", "networks.byaddr", "networks.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_NETNAME_PAR, 1, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_NETNUM_PAR, 1, 1, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_NETNAME_PAR, 1, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_NETNUM_PAR, 1, 1, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, 1, 3, &compare_nisplus_aliased, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_NETNAME_PAR, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_NETNUM_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 2, { { DB_OLD_NETNAME_PAR, DB_NETNAME_PAR, 1, { 0, 0, 1 } }, 
	  { DB_OLD_NETNUM_PAR, DB_NETNUM_PAR, 0, { 1, 1, 2 } } } },
	{ 4, { { DB_NETNAME_PAR, valid_netname, DB_ERR_BAD_NETNAME }, 
	     { DB_NETNUM_PAR, valid_ip_netnum, DB_ERR_BAD_IP_NETNUM }, 
	     { DB_ALIASES_PAR, NULL, 0 }, { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_NETWORKS_MTHD },
		{ &list_table_impl, DB_GET_NETWORKS_MTHD },
		{ &set_table_impl, DB_SET_NETWORKS_MTHD },
		{ &remove_table_impl, DB_REMOVE_NETWORKS_MTHD }
	},
};

struct tbl_trans_data passwd_trans = {
	DB_PASSWD_TBL,
	{ "/etc/passwd", "passwd.byname", "passwd.org_dir",
	  ":", NULL, 1 },
	{
		{ 0, -1, -1, &compare_ufs_col0, 7,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_NULL_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_UID_PAR, 0, 2, 2 },
				{ 0, DB_CASE_SENSITIVE, DB_GID_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_GCOS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_PATH_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_SHELL_PAR },
			}
		},
		{ 0, -1, -1, &compare_ufs_col0, 7,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_PASSWD_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_UID_PAR, 0, 2, 2 },
				{ 0, DB_CASE_SENSITIVE, DB_GID_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_GCOS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_PATH_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_SHELL_PAR },
			}
		},
		{ 0, -1, -1, &compare_nisplus_col0, 8,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_PASSWD_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_UID_PAR, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_GID_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_GCOS_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_PATH_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_SHELL_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_SHADOW_PAR },
			}
		}
	},
	{ 1, { { DB_OLD_USERNAME_PAR, DB_USERNAME_PAR, 1, { 0, 0, 0 } } 

	       /* ,  
The passwd file should only be keyed on username, uid need not be unique
									  
	  { DB_OLD_UID_PAR, DB_UID_PAR, 0, { 2, 2, 2 } }
*/

 } }, 
	{ 14, { { DB_USERNAME_PAR, valid_uname, DB_ERR_BAD_USERNAME }, 
	      { DB_PASSWD_PAR, valid_passwd, DB_ERR_BAD_PASSWD }, 
	      { DB_UID_PAR, valid_uid, DB_ERR_BAD_UID }, 
	      { DB_GID_PAR, valid_gid, DB_ERR_BAD_GID }, 
	      { DB_GCOS_PAR, valid_gcos, DB_ERR_BAD_GCOS }, 
	      { DB_PATH_PAR, valid_home_path, DB_ERR_BAD_PATH },
	      { DB_SHELL_PAR, valid_shell, DB_ERR_BAD_SHELL },
	      { DB_LASTCHANGED_PAR, valid_int, DB_ERR_NOT_INTEGER },
	      { DB_MINIMUM_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER },
	      { DB_MAXIMUM_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER },
	      { DB_WARN_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER },
	      { DB_INACTIVE_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER },
	      { DB_EXPIRE_PAR, valid_int, DB_ERR_NOT_INTEGER },
	      { DB_FLAG_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_PASSWD_MTHD },
		{ &list_table_impl, DB_GET_PASSWD_MTHD },
		{ &set_table_impl, DB_SET_PASSWD_MTHD },
		{ &remove_table_impl, DB_REMOVE_PASSWD_MTHD }
	},
};

struct tbl_trans_data protocols_trans = {
	DB_PROTOCOLS_TBL,
	{ "/etc/inet/protocols", "protocols.bynumber", "protocols.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_PROTOCOL_NAME_PAR, 1, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_PROTOCOL_NUM_PAR, 1, 1, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_PROTOCOL_NAME_PAR, 1, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_PROTOCOL_NUM_PAR, 1, 1, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, 1, 3, &compare_nisplus_aliased, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_PROTOCOL_NAME_PAR, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_PROTOCOL_NUM_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 2, { { DB_OLD_PROTOCOL_NAME_PAR, DB_PROTOCOL_NAME_PAR, 0, { 0, 0, 1 } }, 
	  { DB_OLD_PROTOCOL_NUM_PAR, DB_PROTOCOL_NUM_PAR, 1, { 1, 1, 2 } } } },
	{ 4, { { DB_PROTOCOL_NAME_PAR, valid_proto_name, DB_ERR_BAD_PROTO_NAME }, 
	     { DB_PROTOCOL_NUM_PAR, valid_proto_num, DB_ERR_BAD_PROTO_NUM },
	     { DB_ALIASES_PAR, NULL, 0 }, { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_PROTOCOLS_MTHD },
		{ &list_table_impl, DB_GET_PROTOCOLS_MTHD },
		{ &set_table_impl, DB_SET_PROTOCOLS_MTHD },
		{ &remove_table_impl, DB_REMOVE_PROTOCOLS_MTHD }
	},
};

struct tbl_trans_data rpc_trans = {
	DB_RPC_TBL,
	{ "/etc/rpc", "rpc.bynumber", "rpc.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_RPC_NAME_PAR, 1, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_RPC_NUM_PAR, 1, 1, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_RPC_NAME_PAR, 1, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_RPC_NUM_PAR, 1, 1, 1},
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, 1, 3, &compare_nisplus_aliased, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_RPC_NAME_PAR, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 1, DB_CASE_SENSITIVE, DB_RPC_NUM_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 2, {  { DB_OLD_RPC_NAME_PAR, DB_RPC_NAME_PAR, 0, { 0, 0, 1 } }, 
	  { DB_OLD_RPC_NUM_PAR, DB_RPC_NUM_PAR, 1, { 1, 1, 2 } } } },
	{ 4, { { DB_RPC_NAME_PAR, valid_rpc_name, DB_ERR_BAD_RPC_NAME },
	     { DB_RPC_NUM_PAR, valid_rpc_num, DB_ERR_BAD_RPC_NUM }, 
	     { DB_ALIASES_PAR, NULL, 0 }, { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_RPC_MTHD },
		{ &list_table_impl, DB_GET_RPC_MTHD },
		{ &set_table_impl, DB_SET_RPC_MTHD },
		{ &remove_table_impl, DB_REMOVE_RPC_MTHD }
	},
};

struct tbl_trans_data services_trans = {
	DB_SERVICES_TBL,
	{ "/etc/inet/services", "services.byname", "services.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_SERVICE_NAME_PAR, 0, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_SERVICE_PORT_PAR, 1, 1, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, 2, 3, &compare_ufs_col0, 4,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_SERVICE_NAME_PAR, 0, 0, 2 },
				{ 1, DB_CASE_SENSITIVE, DB_SERVICE_PORT_PAR, 1, 1, 1 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 1, 1, 4, &compare_nisplus_services, 5,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_SERVICE_NAME_PAR, 0 },
				{ 0, DB_CASE_INSENSITIVE, DB_ALIASES_PAR },
				{ 1, DB_CASE_INSENSITIVE, DB_PROTOCOL_NAME_PAR, 0 },
				{ 1, DB_CASE_SENSITIVE, DB_SERVICE_PORT_PAR, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 3, { { DB_OLD_SERVICE_NAME_PAR, DB_SERVICE_NAME_PAR, 0, { 0, 0, 1 } }, 
	  { DB_OLD_SERVICE_PORT_PAR, DB_SERVICE_PORT_PAR, 1, { 1, 1, 3 } },
	  { DB_OLD_PROTOCOL_NAME_PAR, DB_PROTOCOL_NAME_PAR, 1, { 1, 1, 2 } } } },
	{ 5, { { DB_SERVICE_NAME_PAR, valid_service_name, DB_ERR_BAD_SERV_NAME }, 
	     { DB_SERVICE_PORT_PAR, valid_port_num, DB_ERR_BAD_PORT_NUM }, 
	     { DB_PROTOCOL_NAME_PAR, valid_proto_name, DB_ERR_BAD_PROTO_NAME },
	     { DB_ALIASES_PAR, NULL, 0 }, { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_SERVICES_MTHD },
		{ &list_table_impl, DB_GET_SERVICES_MTHD },
		{ &set_table_impl, DB_SET_SERVICES_MTHD },
		{ &remove_table_impl, DB_REMOVE_SERVICES_MTHD }
	},
};

struct tbl_trans_data shadow_trans = {
	DB_SHADOW_TBL,
	{ "/etc/shadow", "shadow.byname", NULL,
	  ":", NULL, 0 },
	{
		{ 0, -1, -1, &compare_ufs_col0, 9,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_PASSWD_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_LASTCHANGED_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MINIMUM_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MAXIMUM_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_WARN_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_INACTIVE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_EXPIRE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_FLAG_PAR },
			}
		},
		{ 0, -1, -1, &compare_ufs_col0, 9,
			{
				{ 1, DB_CASE_SENSITIVE, DB_USERNAME_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_PASSWD_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_LASTCHANGED_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MINIMUM_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MAXIMUM_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_WARN_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_INACTIVE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_EXPIRE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_FLAG_PAR },
			}
		},
		{ 0, -1, -1, &compare_ufs_col0, 7,
			{
				{ 0, DB_CASE_SENSITIVE, DB_LASTCHANGED_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MINIMUM_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_MAXIMUM_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_WARN_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_INACTIVE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_EXPIRE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_FLAG_PAR },
			}
		}
	},
	{ 1, { DB_OLD_USERNAME_PAR, DB_USERNAME_PAR, 0, { 0, 0, 0 } } },
	{ 9, { { DB_USERNAME_PAR, valid_uname, DB_ERR_BAD_USERNAME }, 
	     { DB_PASSWD_PAR, valid_passwd, DB_ERR_BAD_PASSWD }, 
	     { DB_LASTCHANGED_PAR, valid_int, DB_ERR_NOT_INTEGER }, 
	     { DB_MINIMUM_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER }, 
	     { DB_MAXIMUM_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER }, 
	     { DB_WARN_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER }, 
	     { DB_INACTIVE_PAR, valid_unsigned_int, DB_ERR_NOT_INTEGER }, 
	     { DB_EXPIRE_PAR, valid_int, DB_ERR_NOT_INTEGER }, 
	     { DB_FLAG_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_PASSWD_MTHD },
		{ &list_table_impl, DB_GET_PASSWD_MTHD },
		{ &set_table_impl, DB_SET_PASSWD_MTHD },
		{ &remove_table_impl, DB_REMOVE_PASSWD_MTHD }
	},
};

struct tbl_trans_data timezone_trans = {
	DB_TIMEZONE_TBL,
	{ "/etc/timezone", "timezone.byname", "timezone.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, -1, 2, &compare_ufs_col0, 3,
			{
				{ 0, DB_CASE_SENSITIVE, DB_TIMEZONE_PAR },
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 1, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_ufs_col0, 3,
			{
				{ 0, DB_CASE_SENSITIVE, DB_TIMEZONE_PAR },
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1, 1, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_nisplus_col0, 3,
			{
				{ 1, DB_CASE_INSENSITIVE, DB_HOSTNAME_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_TIMEZONE_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 1, { DB_OLD_HOSTNAME_PAR, DB_HOSTNAME_PAR, 1, { 1, 1, 0 } } },
	{ 3, { { DB_HOSTNAME_PAR, valid_hostname, DB_ERR_BAD_HOSTNAME }, 
	     { DB_TIMEZONE_PAR, valid_timezone, DB_ERR_BAD_TIMEZONE }, 
	     { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_TIMEZONE_MTHD },
		{ &list_table_impl, DB_GET_TIMEZONE_MTHD },
		{ &set_table_impl, DB_SET_TIMEZONE_MTHD },
		{ &remove_table_impl, DB_REMOVE_TIMEZONE_MTHD }
	},
};

struct tbl_trans_data policy_trans = {
	DB_POLICY_TBL,
	{ "/etc/Policy_defaults", "Policy_defaults.byname", "Policy_defaults.org_dir",
	  DEFAULT_COLUMN_SEP, DEFAULT_COMMENT_SEP, 0 },
	{
		{ 0, -1, 2, &compare_ufs_col0, 3, 
			{ 
				{ 1, DB_CASE_SENSITIVE, DB_POLICY_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_POLICY_VAL_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		 },
		{ 0, -1, 2, &compare_ufs_col0, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_POLICY_PAR, 1, 0, 0 },
				{ 0, DB_CASE_SENSITIVE, DB_POLICY_VAL_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		},
		{ 0, -1, 2, &compare_nisplus_col0_ci, 3,
			{
				{ 1, DB_CASE_SENSITIVE, DB_POLICY_PAR, 1 },
				{ 0, DB_CASE_SENSITIVE, DB_POLICY_VAL_PAR },
				{ 0, DB_CASE_SENSITIVE, DB_COMMENT_PAR },
			}
		}
	},
	{ 1, { DB_OLD_POLICY_PAR, DB_POLICY_PAR, 1, { 0, 0, 0 } } },
	{ 3, { { DB_POLICY_PAR, valid_policy, DB_ERR_BAD_POLICY }, 
	     { DB_POLICY_VAL_PAR, NULL, 0 }, { DB_COMMENT_PAR, NULL, 0 } } },
	{
		{ &list_table_impl, DB_LIST_POLICY_MTHD },
		{ &list_table_impl, DB_GET_POLICY_MTHD },
		{ &set_table_impl, DB_SET_POLICY_MTHD },
		{ &remove_table_impl, DB_REMOVE_POLICY_MTHD }
	},
};

struct tbl_trans_data *adm_tbl_trans[] = {
        &auto_home_trans,
        &bootparams_trans,
        &cred_trans,
        &ethers_trans,
        &group_trans,
        &hosts_trans,
        &locale_trans,
        &mail_aliases_trans,
        &netgroup_trans,
        &netmasks_trans,
        &networks_trans,
        &passwd_trans,
	&policy_trans,
        &protocols_trans,
        &rpc_trans,
        &services_trans,
        &shadow_trans,
        &timezone_trans };
