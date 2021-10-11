
#ident "@(#)test.c 1.6 93/12/16"

#include "windvc.h"
#include "silent.h"
#include "exists.h"
#include <stdlib.h>
#include <stdio.h>

#define ATTRLIST

static void
usage_exit()
{
	fprintf(stderr,"kdmconfig test: Usage: test [-c | -e | -a  | -n | -s ]\n");
	exit(-1);
}

main(int argc, char **argv)
{
int x;
char **list1;
ATTRIB * list2;
NODE node;
int *num;
char *str;
int c;

	if (argc < 2) usage_exit();

	dvc_init();

	while ((c = getopt(argc, argv, "sncdea")) != EOF) 
	switch(c) {
	case 'd':
		list1 = get_category_list("display");
		for(x=0; list1[x]; x++)
			printf("%s\n", list1[x]);
		list1 = get_category_list("pointer");
		for(x=0; list1[x]; x++)
			printf("%s\n", list1[x]);
		list1 = get_category_list("keyboard");
		for(x=0; list1[x]; x++)
			printf("%s\n", list1[x]);
			break;

	case 'e':
		if (display_exists) printf("Display exists!\n");
		if (keyboard_exists) printf("Keyboard exists!\n");
		if (pointer_exists) printf("Pointer exists!\n");
		break;

	case 'a':
		node = get_device_info( "pointer", 1);
		list2 = get_attrib_name_list(node);
		for(x=0; list2[x]; x++) {
			printf("%s\n", (char *)get_attrib_title(node, list2[x]));
			if (get_attrib_type(node, list2[x]) == VAL_NUMERIC) {
				printf("NUMERIC ATTRIBUTE.\n");
				printf("%d\n", *(int*)get_attrib_value(node, list2[x]));
				while (num = (int*)get_attrib_value(node, NULL))
					printf("%d\n",*num);
			} else if (get_attrib_type(node, list2[x]) == VAL_STRING) {
				printf("STRING ATTRIBUTE.\n");
				printf("%s\n", (char *)get_attrib_value(node, list2[x]));
				while (str = (char *)get_attrib_value(node, NULL))
					printf("%s\n",str);
			} else {
				printf("Attribute %s not defined for node\n", list2[x]);
			}
		}
		break;

	case 'c':
		node = get_device_info( "pointer", 1);
		list2 = get_attrib_name_list(node);
		printf("%s\n", get_selected_attrib_value(node, list2[0]));
		set_attrib_value(node, list2[0], "/dev/tty01");
		printf("%s\n", get_selected_attrib_value(node, list2[0]));
		break;

	case 'n':
		node = get_node_by_name("pointer", "LOGI-S");
		printf("%s\n", node->title);
		node = get_node_by_name("pointer", "kdmouse");
		printf("%s\n", node->title);
		node = get_node_by_name("display", "fahr60");
		printf("%s\n", node->title);
		break;

	case 's':
		get_pointer_silent();
		break;

	default:
		fprintf(stderr,"kdcont test: option %c not recognized.\n",c);
		usage_exit();
		break;
	}
}

void ui_notice( char *text )
{

}

void ui_error_exit( char *text )
{

}
