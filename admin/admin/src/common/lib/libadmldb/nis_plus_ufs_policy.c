/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#pragma ident "@(#)nis_plus_ufs_policy.c	1.4 94/09/07 Sun Microsystems"

/* Contains code to create a new NIS+ table for Policy default values.      */
/* The table has a key, a value, and a comment.                             */
/* The owner of the new table is determined by looking up the owner of the  */
/* passwd table and using that value.  The initial values to fill the table */
/* are taken from the "/usr/snadm/etc/policy.defaults" file.                */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpcsvc/nis.h>
#include <admldb.h>
#include <admldb_impl.h>
#include <admldb_msgs.h>
#include "nis_plus_ufs_policy.h"


#define TIME_TO_LIVE     10800            /* 3 hours            */
#define ENTRY_TYPE       "policy record"  /* type of entry      */
#define NUM_POLICY_COLS  3                /* num cols in table  */
#define POLICY_SEPARATOR '='              /* char between cols  */
#define KEY_NAME         "Policy"         /* name of key column */
#define VAL_NAME         "Value"          /* name of value col  */
#define COMMENT_NAME     "Comment"        /* comment col name   */
#define KEY_INDEX        0                /* index of key col   */
#define VAL_INDEX        1                /* index of val col   */
#define COMMENT_INDEX    2                /* comment col index  */
#define BUFFSIZE         256              /* size of char buffs */

/* #define ACCESS_RIGHTS \
    (WORLD_DEFAULT                           | \
     ((NIS_READ_ACC + NIS_MODIFY_ACC) << 8)  | \
     OWNER_DEFAULT)                        tbl access rights  */


#define ACCESS_RIGHTS \
    (WORLD_DEFAULT        | \
     ((NIS_READ_ACC +\
       NIS_MODIFY_ACC +\
       NIS_CREATE_ACC +\
       NIS_DESTROY_ACC) << 8) | \
     OWNER_DEFAULT)                       /* tbl access rights  */

static char       name_buff[BUFFSIZE];
static char       owner_buff[BUFFSIZE];
static char       zo_name_buff[BUFFSIZE];
static char       zo_domain_buff[BUFFSIZE];
static char       key_buff[BUFFSIZE];
static char       val_buff[BUFFSIZE];
    

static entry_col  entry_col_buff[NUM_POLICY_COLS];
static table_col  table_col_buff[NUM_POLICY_COLS];


static int
split_entry(
    const char*  entry,
    char*        key,
    char*        value
)
{
    char* separator_pos = strchr(entry, '=');
    int key_length;
    
    if (separator_pos == NULL) {
	return 0;
    }
    
    key_length = separator_pos - entry;
    
    strncpy(key, entry, key_length);
    key[key_length] = '\0';
    
    strcpy(value, entry + key_length + 1);

    return 1;
}

static void
prune_possible_extra_dots(
    char* nis_name
)
{
    int len = strlen(nis_name);
    int i;

    for (i = len - 1; i > 0 && nis_name[i] == '.' && nis_name[i-1] == '.';
	--i);
    
    nis_name[i+1] = '\0';
}

	
static int
get_admin_group_name(
    const char* domain,
    char*       name_buff,
    Db_error**  db_err,
    int         clean_or_dirty
)
{
    nis_result* lookup_result;
    char buff[256];
    
    sprintf(buff, "org_dir.%s.", domain);
    prune_possible_extra_dots(buff);    

    lookup_result = nis_lookup(buff, 0);
    
    if (lookup_result->status != NIS_SUCCESS) {
	db_err_set(db_err, DB_ERR_IN_LIB, clean_or_dirty,
	    "create_new_policy_table", "get_admin_group_name",
	    nis_sperrno(lookup_result->status));
	nis_freeresult(lookup_result);
	return 0;
    }
    
    strcpy(name_buff, lookup_result->objects.objects_val->zo_group);
    nis_freeresult(lookup_result);    
    return 1;
}

    


int
add_policy_table(
    const char* owner,
    const char* domain,
    Db_error**  db_err
)
{
    nis_object new_table;
    table_obj* table_data;
    nis_result* add_result;
    int flag;
    char group_name[256];

    sprintf(name_buff, "Policy_defaults.org_dir.%s.", domain);
    prune_possible_extra_dots(name_buff);

    if (!get_admin_group_name(domain, group_name, db_err, ADM_FAILCLEAN)) {
	return 0;
    }
    
    new_table.zo_owner        = (char*)owner;
    new_table.zo_group        = group_name;
    new_table.zo_name         = zo_name_buff;
    new_table.zo_domain       = zo_domain_buff;    
    new_table.zo_access       = ACCESS_RIGHTS;
    new_table.zo_ttl          = TIME_TO_LIVE;
    new_table.zo_data.zo_type = TABLE_OBJ;
    
    table_data     = &new_table.zo_data.objdata_u.ta_data;
    
    table_data->ta_type                                  = (char*)ENTRY_TYPE;
    table_data->ta_maxcol                                = NUM_POLICY_COLS;
    table_data->ta_sep                                   = POLICY_SEPARATOR;
    table_data->ta_path                                  = "";
    table_data->ta_cols.ta_cols_len                      = NUM_POLICY_COLS;
    table_data->ta_cols.ta_cols_val                      = table_col_buff;
    table_data->ta_cols.ta_cols_val[KEY_INDEX].tc_name   = (char*)KEY_NAME;
    table_data->ta_cols.ta_cols_val[KEY_INDEX].tc_flags  = TA_SEARCHABLE;
    table_data->ta_cols.ta_cols_val[KEY_INDEX].tc_rights = 0;
    table_data->ta_cols.ta_cols_val[VAL_INDEX].tc_name   = (char*)VAL_NAME;
    table_data->ta_cols.ta_cols_val[VAL_INDEX].tc_flags  = TA_SEARCHABLE;
    table_data->ta_cols.ta_cols_val[VAL_INDEX].tc_rights = 0;
    table_data->ta_cols.ta_cols_val[COMMENT_INDEX].tc_name   
      = (char*)COMMENT_NAME;
    table_data->ta_cols.ta_cols_val[COMMENT_INDEX].tc_flags 
      = TA_SEARCHABLE;
    table_data->ta_cols.ta_cols_val[COMMENT_INDEX].tc_rights 
      = 0;

    add_result = nis_add(name_buff, &new_table);
    flag       = add_result->status;
    nis_freeresult(add_result);

    if (flag != NIS_SUCCESS) {
	db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
	"create_new_policy_table", "add_policy_table",
	    nis_sperrno(flag));
	return 0;
    }

    return 1;
}
    

int
add_to_policy_table(
    const char* domain,
    const char* entry,    /* <name>=<value> */
    Db_error**  db_err
)
{
    nis_object new_entry;
    entry_obj* entry_data;
    nis_result* add_result;
    int flag;
    char group_name[256];

    sprintf(name_buff, "Policy_defaults.org_dir.%s.", domain);
    prune_possible_extra_dots(name_buff);
    (void)split_entry(entry, key_buff, val_buff);

    if (!get_admin_group_name(domain, group_name, db_err, ADM_FAILDIRTY)) {
	return 0;
    }
    
    new_entry.zo_owner        = owner_buff;
    new_entry.zo_group        = group_name;
    new_entry.zo_name         = zo_name_buff;
    new_entry.zo_domain       = zo_domain_buff;    
    new_entry.zo_access       = ACCESS_RIGHTS;
    new_entry.zo_ttl          = TIME_TO_LIVE;
    new_entry.zo_data.zo_type = ENTRY_OBJ;
    
    entry_data                = &new_entry.zo_data.objdata_u.en_data;
    
    entry_data->en_type                                     = (char*)ENTRY_TYPE;
    entry_data->en_cols.en_cols_len                         = NUM_POLICY_COLS;
    entry_data->en_cols.en_cols_val                         = entry_col_buff;
    entry_data->en_cols.en_cols_val[KEY_INDEX].ec_flags     = 0;
    entry_data->en_cols.en_cols_val[KEY_INDEX].ec_value.ec_value_len 
      = strlen(key_buff) + 1;
    entry_data->en_cols.en_cols_val[KEY_INDEX].ec_value.ec_value_val 
      = key_buff;
    entry_data->en_cols.en_cols_val[VAL_INDEX].ec_flags     = 0;
    entry_data->en_cols.en_cols_val[VAL_INDEX].ec_value.ec_value_len 
      = strlen(val_buff) + 1;
    entry_data->en_cols.en_cols_val[VAL_INDEX].ec_value.ec_value_val 
      = val_buff;

    add_result = nis_add_entry(name_buff, &new_entry, 0);
    flag       = add_result->status; 

    nis_freeresult(add_result);

    if (flag != NIS_SUCCESS) {
	db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
	"create_new_policy_table", "add_to_policy_table",
	    nis_sperrno(flag));
	return 0;
    }

    return 1;
}

void 
find_master_name(
    const char* domain,
    char* buff
)
{
    nis_result* passwd_result;
    nis_object* passwd_table;
    
    sprintf(name_buff, "passwd.org_dir.%s.", domain);
    prune_possible_extra_dots(name_buff);
    
    passwd_result = nis_lookup(name_buff, 0);
    
    if (passwd_result->status != NIS_SUCCESS) {
	nis_freeresult(passwd_result);
	fprintf(stderr, "find_master_name: couldn't find %s\n", name_buff);
	exit(1);
    } else {
	passwd_table = passwd_result->objects.objects_val;
	strcpy(buff, passwd_table->zo_owner);
	nis_freeresult(passwd_result);
    }
}

int
create_new_policy_table(
    const char* domain,
    Db_error**  db_err
)
{
    char* buff = malloc(BUFFSIZE * sizeof(char));
    FILE* defaults;
    int add_result;
    
    find_master_name(domain, buff);
   
    if (!add_policy_table(buff, domain, db_err)) {
	free(buff);
	return 0;
    }

    defaults = fopen(POLICY_FILE_NAME, "r");

    if (!defaults) {
	db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
	    "create_new_policy_table", "fopen",
	    strerror(errno));
	free(buff);
	return 0;
    }
    
    while (fgets(buff, BUFFSIZE, defaults) != NULL) {
	buff[strlen(buff)-1] = '\0';
	if (!add_to_policy_table(domain, buff, db_err)) {
	    return 0;
	}
    }
    
    free(buff);
    return 1;
}

    
void
install_fallback_file(
    const char* db_file_name
)
{
    FILE* defaults;
    FILE* db_file;
    char* buff;
    char* key_buff;
    char* val_buff;

    if ((defaults = fopen(POLICY_FILE_NAME, "r")) == NULL) {
	return;
    }

    if ((db_file = fopen(db_file_name, "w")) == NULL) {
	fclose(defaults);
	return;
    }

    buff      = malloc(BUFFSIZE * sizeof(char));
    key_buff  = malloc(BUFFSIZE * sizeof(char));
    val_buff  = malloc(BUFFSIZE * sizeof(char));

    
    while (fgets(buff, BUFFSIZE, defaults) != NULL) {
	buff[strlen(buff)-1] = '\0';

	if (buff[0] == '\0') {
	    continue; 
	}
       
	if (split_entry(buff, key_buff, val_buff)) {
	    fputs(key_buff, db_file);
	    putc(' ', db_file);
	    fputs(val_buff, db_file);
	    putc('\n', db_file);
	} else {
	    fprintf(stderr, "bad policy entry: %s\n", buff); 
	}
    }
    
    fclose(defaults);
    fclose(db_file);
    free(buff);
    free(key_buff);
    free(val_buff);
    return;
}
