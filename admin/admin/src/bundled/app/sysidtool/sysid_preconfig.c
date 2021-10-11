
/*
 *  Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * sysid_preconfig - support functions for accessing config data
 *
 * These functions are called by each of the sysid modules to read
 * in the configuration file and provide selected data from that
 * file.
 */

#pragma	ident	"@(#)sysid_preconfig.c	1.3	96/06/14 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "sysidtool.h"
#include "sysid_preconfig.h"

/* data structures */
typedef struct config_node {
	int keyword;
	char value[MAXPATHLEN];
	struct config_node *next_attribute;
	struct config_node *next_config;
} Config;

/* prototypes */
static Config *get_config_entry(int keyword,char *value);
static Config *get_config_attribute_entry(Config *keyword_node,
			int attribute_keyword);


/* static storage */

static Config *config_list = NULL;
static Config *end_config_list = NULL;

/*
 * dump_preconfig()
 * 
 * Print the sysid configuration item data structure.
 * This is a debug only function.
 *
 * Parameters:
 *  none
 *
 * Returns:
 * none
 *
 */
void
dump_preconfig()
{
	Config *config_ptr;
	Config *attr_ptr;

	config_ptr = config_list;
	fprintf(stderr," Sysid Preconfiguration Elements\n");
	while (config_ptr != NULL) {
		fprintf(stderr,"    Keyword = %d    Value = %s\n",
			config_ptr->keyword, config_ptr->value);
		attr_ptr = config_ptr->next_attribute;
		while (attr_ptr != NULL) {
			fprintf(stderr,"        Modifier = %d    Value = %s\n",
				attr_ptr->keyword, attr_ptr->value);
			attr_ptr = attr_ptr->next_attribute;
		}

		config_ptr = config_ptr->next_config;
	}
}

/*
 * get_config_entry()
 * 
 * Return a pointer to the node containing the specified 
 * keyword/value pair.  If value is NULL then return the
 * first keyword node that matches.
 *
 * Parameters:
 *  keyword  - int representing the desired keyword
 *  value - pointer to a value or NULL
 *
 * Returns:
 * pointer to the configuration node or NULL
 *
 */
static Config * 
get_config_entry(int keyword, char *value)
{
	Config *node;

	node = config_list;
	while (node != NULL) {
		if (node->keyword == keyword &&
			(value == NULL || strcmp(node->value, value) == 0))
			break;

		node = node->next_config;	
	}

	return(node);
}

/*
 * get_config_attribute_entry()
 * 
 * Return a pointer to the node containing the specified 
 * keyword/value pair from keyword attribute list.
 *
 * Parameters:
 *  keyword_node - Ptr to a Config node 
 *  attribute_keyword - keyword to search for
 *
 * Returns:
 * pointer to the configuration attribute node or NULL
 *
 */
static Config * 
get_config_attribute_entry(Config *keyword_node, int attribute_keyword)
{
	Config *node;

	node = keyword_node;
	while (node != NULL) {
		if (node->keyword == attribute_keyword)
			break;

		node = node->next_attribute;	
	}

	return(node);
}

/*
 * config_entries_exist()
 * 
 * Determine whether any configuration entries have been
 * specified.
 *
 * Parameters:
 *  none
 *
 * Returns:
 * 1 - if there are configuration entries
 * 0 - otherwise
 *
 */
 int
 config_entries_exist(void)
 {
	if (config_list != NULL)
		return(1);
	else
		return(0);
 }

/*
 * create_config_entry()
 * 
 * Create the specified configuration entry and add it to the 
 * configuration item list.
 *
 * Parameters:
 *  keyword 
 *  value
 *
 * Returns:
 * none
 *
 */
void 
create_config_entry(int keyword, char *value)
{
	Config *node;

	/* allocate a configuration node */
	node = (Config *)malloc(sizeof(Config));

	/*#  How do we deal with malloc failures #*/
	if (node != NULL) {
		node->keyword = keyword;
		strcpy(node->value,value);
		node->next_attribute = NULL;
		node->next_config = NULL;

		/* put new nodes at the end of the list */
		if (config_list == NULL) {
			config_list = node;
		}
		else {
			end_config_list->next_config = node;
		}

		end_config_list = node;
	}
}

/*
 * create_config_attribute()
 * 
 * Create the specified configuration attribute entry and add it to the 
 * configuration item list.
 *
 * Parameters:
 *  keyword 
 *  value
 *  attr_keyword
 *  attr_value
 *
 * Returns:
 * none
 *
 */
void 
create_config_attribute(int keyword, char *value, 
			int attr_keyword, char *attr_value)
{
	Config *keyword_node;
	Config *attribute_node;
	Config *next_attr;

	/* Find the keyword node */
	if ((keyword_node = get_config_entry(keyword,value)) == NULL)
		return;

	/* allocate a configuration node */
	attribute_node = (Config *)malloc(sizeof(Config));

	if (attribute_node != NULL) {
		attribute_node->keyword = attr_keyword;
		strcpy(attribute_node->value,attr_value);
		attribute_node->next_config = NULL;
		attribute_node->next_attribute = NULL;

		/* put the new nodes at the end of the list */
		if (keyword_node->next_attribute == NULL) {
			keyword_node->next_attribute = attribute_node;
		}
		else {
			next_attr = keyword_node->next_attribute;
			while (next_attr->next_attribute != NULL) {
				next_attr = next_attr->next_attribute;
			}
			next_attr->next_attribute = attribute_node;
		}
	}
}


/*
 * get_preconfig_value()
 * 
 * Look for the value associated with the keyword
 * or attribute keyword and return its value.
 *
 * If looking for the keyword value then the call will look like:
 *    get_preconfig_value(<keyword>,NULL,NULL);
 *
 * If looking for an attribute value then the call will look like:
 *    get_preconfig_value(<keyword>,<value>,<attribute keyword>);
 *
 * Parameters:
 *  keyword - pointer to keyword
 *  value - pointer to value or NULL if looking for keyword value
 *	    value can also be passed in as CFG_DEFAULT_VALUE which
 *          specifies that the default value (1st one found) is
 *          the value that should be used when searching for 
 *          an attribute value
 *  attribute keyword - pointer to keyword or NULL
 *
 * Return:
 *  a pointer to the requested value or NULL if that value 
 *  does not exist
 *
 */
char * 
get_preconfig_value(int keyword, char *value, int attribute_keyword)
{
	Config *keyword_node;
	Config *attribute_node;

	keyword_node = get_config_entry(keyword,value);

	if (keyword_node == NULL)
		return(NULL);

	if (value == NULL && attribute_keyword == NULL)
		return(keyword_node->value);

	attribute_node = get_config_attribute_entry(keyword_node,
		attribute_keyword);
	
	if (attribute_node == NULL)
		return(NULL);

	return(attribute_node->value);
	
}

