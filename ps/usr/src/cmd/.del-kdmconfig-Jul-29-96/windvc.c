/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 *
 * windvc.c: API layer over base dvc library to facilitate windows config.
 *
 * Description:
 *  This set of routines provide a layer between the standard dvc routines,
 *  and the OpenWindows config program (kdmconfig). It allows simple retrival of
 *  lists, and of deivce info nodes for the selected keyboard, mouse, display.
 *  It also provides for management of the attribute lists of each of the
 *  node types, and the ability to select and commit the configured devices.
 *
 * A list of all public routines implemented here are prototyped in windvc.h.
 * See that file for a complete interface description of each public
 * routine as well.
 *
 */

#pragma ident "@(#)windvc.c 1.27 95/10/04 SMI"

#include "windvc.h"
#include "exists.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/utsname.h>

/* Internal data structures */

typedef struct {
	char **list;
	int	idx;
	NODE	selected;
} _Configuration;

static _Configuration dis = { NULL, -1, (NODE)NULL };
static _Configuration kbd = { NULL, -1, (NODE)NULL };
static _Configuration ptr = { NULL, -1, (NODE)NULL };
static _Configuration mon = { NULL, -1, (NODE)NULL };

typedef struct _ins {
	struct _ins * next;
	NODE  node;
	ATTRIB	* attrib_list;
	char	** attrib_title;
	char	** attrib_tkey;
	char	** attrib_ptype;
} _ins_t;
/* FYI: ins = internal node stuff */

static _ins_t * _nlist_head = (_ins_t *)NULL;

/* how many items in a growing list to malloc at a time */
#define	LIST_BLOCK_SIZE 10

/*
 * ------------------------------------
 *   Internal (non-exported) routines
 * ------------------------------------
 */

/*
 * This routine allows the construction of internal lists of the
 * categories we are interested in.
 */

static void
_make_cat_list(char ***l)
{
	char **titles, *dev_title;
	int n;
	/* Make space for 10 entries initially */
	titles = (char **)calloc(LIST_BLOCK_SIZE, sizeof (char *));
	for (n = 0; dev_title = next_cat_dev_title(); n++) {
		titles[n] = strdup(dev_title);
		if (((n+1)%LIST_BLOCK_SIZE) == 0)  {
			titles = (char **)realloc(titles,
				(n+LIST_BLOCK_SIZE+1) * sizeof (char *));
			(void) memset((void *)&(titles[n+1]), 0,
					LIST_BLOCK_SIZE * sizeof (char *));
		}
	}
	*l = titles;
	return;

}

static int
_cat_index(cat_t catname)
{
	if (strcmp(catname, "display") == NULL)
		return (dis.idx);
	if (strcmp(catname, "keyboard") == NULL)
		return (kbd.idx);
	if (strcmp(catname, "pointer") == NULL)
		return (ptr.idx);
	if (strcmp(catname, "monitor") == NULL)
		return (mon.idx);

	return (-1);
}

static NODE
_match_node(NODE other)
{
	_ins_t *n;

	for (n = _nlist_head; n != (_ins_t *)NULL; n = n->next) {
		if (strcmp(other->name, n->node->name) == NULL)
			return (n->node);
		}
	return (NULL);
}

/*
 * This routines fills the internal node struct fields, allowing
 * easier access to a node's internals. It keeps track of the node
 * so parsing is not done each time. Nodes are kept in MRC order.
 */

static _ins_t *
_parse_node(NODE  newnode)
{
	_ins_t * n;
	int x, y;
	attr_list_t *att;

	/* Look to see if this node has already been parsed */

	for (n = _nlist_head; n != (_ins_t *)NULL; n = n->next) {
		if ((void *)(n->node) == (void *)newnode) {
			break;
		}
	}

	/* found it! */
	if (n)
		return (n);

	/* Otherwise we need to parse it */
	n = (_ins_t *)calloc(1, sizeof (_ins_t));
	n->node = newnode;
	n->next = _nlist_head;
	_nlist_head = n; /* Put things in MRC(Most Recently Created) order */

	/*	 count attributes (Max possible) */
	for (x = 0, att = newnode->typ_alist; att; att = att->next, x++)
		;

	if (x) {
		n->attrib_list = (ATTRIB *)calloc(x, sizeof (ATTRIB));
		n->attrib_title = (char **)calloc(x, sizeof (char *));
		n->attrib_tkey = (char **)calloc(x, sizeof (char *));
		n->attrib_ptype = (char **)calloc(x, sizeof (char *));
		y = 0;
		for (x = 0, att = newnode->typ_alist; att;
		    att = att->next, x++){
			val_list_t *val;

			for (val = att->vlist; val; val = val->next){
			if (match(val->val.string, VAR_STRING)) {
				attr_list_t *nattr;
				n->attrib_list[y] =
					strchr(val->val.string, ',');
				n->attrib_list[y]++;
				n->attrib_title[y] =
					expand_abbr(n->attrib_list[y]);
				n->attrib_tkey[y] = att->name;
				for (nattr = newnode->typ_alist; nattr;
				    nattr = nattr->next)
					if (!strcmp(nattr->name,
					    n->attrib_list[y])){
						n->attrib_ptype[y] =
						    nattr->vlist->val.string;
					}
				y++;
			} /* end if  */
			} /* end for */
		}
	}
	return (n);
}

/*
 * These routines return singular aspects of an attribute using
 * an associatace lookup scheme.
 */
static char *
_attrib_tkey(_ins_t * h, ATTRIB attrib)
{
	int x;

	for (x = 0; h->attrib_list[x]; x++){
		if (!strcmp(h->attrib_list[x], attrib))
			return (h->attrib_tkey[x]);
	}
	return (attrib);
}

static char *
_attrib_ptype(_ins_t * h, ATTRIB attrib)
{
	int x;

	for (x = 0; h->attrib_list[x]; x++)
		if (!strcmp(h->attrib_list[x], attrib))
			return (h->attrib_ptype[x]);
	return (attrib);
}

static attr_list_t *
_attrib_value(_ins_t * h, ATTRIB attrib)
{
	attr_list_t *alist = h->node->dev_alist;

	attrib = _attrib_tkey(h, attrib);
	while (alist && strcmp(attrib, alist->name))
		alist = alist->next;
	return (alist);
}

static val_list_t *
_attrib_val_list(_ins_t * h, ATTRIB attrib)
{
	ATTRIB attrib2;
	attr_list_t *alist  = h->node->dev_alist;
	attr_list_t *alist2 = h->node->typ_alist;
	val_list_t  *vlist1, *vlist2;

	attrib2 = _attrib_tkey(h, attrib);

	while (alist && strcmp(attrib2, alist->name))
		alist = alist->next;
	if (alist){
		/*
		 * find corresponding name of the type list then
		 * find the corresponding vlist entries on both the
		 * dev and typ list to find the correct vlist entry to
		 * modify
		 */
		while (alist2 && strcmp(attrib2, alist2->name))
			alist2 = alist2->next;
		for (vlist1 = alist->vlist, vlist2 = alist2->vlist; vlist1;
		    vlist1 = vlist1->next, vlist2 = vlist2->next){
			if (vlist2->val_type = VAL_STRING) {
				if (!strcmp(vlist2->val.string +
				    sizeof (VAR_STRING) - 1, attrib))
					return (vlist1);
			}
		}
	}
	return ((val_list_t *)NULL);
}


static attr_list_t*
_attrib_value_typ(_ins_t * h, ATTRIB attrib)
{
	attr_list_t *alist = h->node->typ_alist;

	while (alist && strcmp(attrib, alist->name))
		alist = alist->next;
	return (alist);
}

/*
 * These routines provide common prototype for invocation of
 * the dvc next_* routines, avoiding a compiler warning.
 */

static void
_next_string_wrapper(char ** attr, char **res)
{
	*res = next_string(attr);
}

static void
_next_numeric_wrapper(char ** attr, char **res)
{
	int usint;

	next_numeric(attr, (int*)res, &usint);


}

static void
_reconfigure()
{
DIR *dp;
struct dirent *de;
char cmd[MAXPATHLEN];
int reconf = 0;
struct utsname uinfo;
char *krn, *plt, *usr, *deflt, *dir;
char *path1, *path2, *path3, *path4;
attr_list_t *install_map, *alist;
char *path[4] = {0, 0, 0, 0};
int i;


	uname(&uinfo);

	install_map = find_typ_info("install_map");

	for (alist = install_map; alist; alist = alist->next)
		if (streq(KRN_ATTR, alist->name))
			krn = alist->vlist->val.string;
		else if (streq(PLTFRM_ATTR, alist->name))
			plt = alist->vlist->val.string;
		else if (streq(USR_ATTR, alist->name))
			usr = alist->vlist->val.string;
		else if (streq(alist->name, DEFAULT_ATTR))
			deflt = alist->vlist->val.string;



	path[0] = strcats(usr, plt, "/", uinfo.machine, krn, NULL);
	path[1] = strcats(plt, "/", uinfo.machine, krn, NULL);
	path[2] = strcats(usr, krn, NULL);
	path[3] = strcats(krn, NULL);
	path[4] = (char *) NULL;


	/*
	 * first take the selected modules and copy them to /tmp/kernel.
	 * If there are none, no problem.
	 */

	for (i = 0; i < sizeof (path)/sizeof (char *); i++){
	    dir = strcats("/", deflt, path[i], NULL);
	    dp = opendir(dir);
	    while ((dp != NULL) && (de = readdir(dp)) != NULL) {
		if (!de->d_name[0]) break;
		if (de->d_name[0] == '.') continue;
		/* only interested in ones with a dot in them */
		if (!strchr(de->d_name, '.')) continue;
		/* If this is not a .conf file, go on */
		if (strcmp(".conf", strrchr(de->d_name, '.'))) continue;

#ifdef NOT
		/* copy the associated module */
		sprintf(cmd, "cp %s/%s %s", dir, path[i],
			strtok(de->d_name, "."));
		system(cmd);
#else
			strtok(de->d_name, "."); /* remove .conf */
#endif
		reconf++;
		printf("Reconfiguring for driver:%s\n", de->d_name);
		sprintf(cmd, "cd /tmp; /usr/sbin/drvconfig -i %s -p"
		    " /tmp/root/etc/path_to_inst -r devices", de->d_name);
		fflush(stdout);
		system(cmd);
	    }
	    if (dp != NULL)
		closedir(dp);
	    xfree(dir);
	}

	if (reconf) {
		printf("Updating Kernel Device Configuration...");
		fflush(stdout);
		/* now, rerun the config commands */
		system("/usr/sbin/devlinks");
		printf("Done.\n");
		fflush(stdout);
	}
}

/*
 * This routine REMOVES the nodes loaded during fetch_device_info()
 * until we can be sure that if a device is loaded, it has been
 * completely configured correctly. When that is the case, we will
 * copy the loaded device to the selected device as well as remove it,
 * and it will get added back during the commit.
 */

static void
find_loaded()
{
	while (keyboard_exists) {
		remove_dev_node(cat_get_node_index(KEYBOARD_CAT));
	}
	while (pointer_exists) {
		remove_dev_node(cat_get_node_index(POINTER_CAT));
	}
	while (display_exists) {
		printf("Display Exists\n");
		remove_dev_node(cat_get_node_index(DISPLAY_CAT));
	}
	while (monitor_exists) {
		remove_dev_node(cat_get_node_index(MONITOR_CAT));
	}
}




/*
 * ---------------------------------
 *   Public (exported) routines.
 *   Elaborate descriptions of these
 *   functions can be found in the
 *   windvc.h include file.
 * ---------------------------------
 */

/*
 * SYSTEM INITIALIZATION Function
 */

void
dvc_init(void)
{
	int i;
	char *c_title;

	fetch_cat_info();

	/* Make the device lists */
	for (i = 0; c_title = next_cat_title(); i++) {
		set_cat_idx(i);
		if (!strncmp(c_title, "Video", 5)) {
			dis.idx = i;
			_make_cat_list(&(dis.list));
		} else if (!strncmp(c_title, "Keybo", 5)) {
			kbd.idx = i;
			_make_cat_list(&(kbd.list));
		} else if (!strncmp(c_title, "Point", 5)) {
			ptr.idx = i;
			_make_cat_list(&(ptr.list));
		} else if (!strncmp(c_title, "Monit", 5)) {
			mon.idx = i;
			_make_cat_list(&(mon.list));
		}
	}

	find_loaded();
}

/*
 * CATEGORY/NODE MANAGEMENT Functions
 */

/*
 * This is done as a macro since a,b,and c vary in their type
 * It just makes the code below a little easierto read.
 */

#define	SELECT_ON_CAT(x, a, b, c, d) \
	int ci = _cat_index(x); \
	if (ci == dis.idx) \
		return (a); \
	else if (ci == ptr.idx) \
		return (b); \
	else if (ci == kbd.idx) \
		return (c); \
	else if (ci == mon.idx) \
		return (d);

char **
get_category_list(cat_t catname)
{
	SELECT_ON_CAT(catname, dis.list, ptr.list, kbd.list, mon.list);
	return ((char **)NULL);
}

NODE
get_device_info(cat_t catname, int idx)
{
NODE node, existing;
	int ind;

	ind = _cat_index(catname);
	set_cat_idx(ind);
	node =  make_dev_node(idx);
	/* See if this node already exists somewhere */
	if ((existing = _match_node(node)) != (NODE)NULL) {
		free_dev_node(node);
		node = existing;
	}
	return (node);
}

/*
 * CONFIGURED DEVICES MANAGEMENT Functions
 */

NODE
get_selected_device(cat_t catname)
{
	SELECT_ON_CAT(catname, dis.selected, ptr.selected,
			kbd.selected, mon.selected);
	return ((NODE)NULL);
}

void
set_selected_device(NODE  node)
{
	cat_t catname;
	int ci;

	if (!node) return;

	catname = get_dev_cat(node);
	ci = _cat_index(catname);

	if (ci == dis.idx)
		dis.selected = node;
	else if (ci == kbd.idx)
		kbd.selected = node;
	else if (ci == ptr.idx)
		ptr.selected = node;
	else if (ci == mon.idx)
		mon.selected = node;
}

void
dvc_commit(void)
{
	void store_mon(NODE dev);

	if (dis.selected)
		add_dev_node(dis.selected);

	if (kbd.selected)
		add_dev_node(kbd.selected);

	if (ptr.selected)
		add_dev_node(ptr.selected);

	if (mon.selected)
		if (get_xin()){
			add_dev_node(mon.selected);
			store_mon(mon.selected);
		}
	if (modified_conf()) {
		update_conf();
		_reconfigure();
	}
}

/*
 * ATTRIBUTE MANAGEMENT Functions
 */

ATTRIB *
get_attrib_name_list(NODE  node)
{
	_ins_t * h;

	h = _parse_node(node);

	return (h->attrib_list);
}

char *
get_attrib_title(NODE  node, ATTRIB attrib)
{
	_ins_t *h;
	int x;

	h = _parse_node(node);
	for (x = 0; h->attrib_list[x]; x++)
		if (!strcmp(h->attrib_list[x], attrib))
			return (h->attrib_title[x]);
	return (NULL);
}

val_t
get_attrib_type(NODE  node, ATTRIB attrib)
{
	_ins_t *h;
	attr_list_t *att;

	h = _parse_node(node);
	attrib = _attrib_tkey(h, attrib);
	for (att = node->dev_alist; att; att = att->next)
		if (!strcmp(att->name, attrib))
			return (att->vlist->val_type);
	return (VAL_ERROR);
}

VAL
get_attrib_value(NODE node, ATTRIB attrib)
{
	static char * attr = NULL;
	static int x = 0;
	static int ct = 0;
	static VAL * retv;
	static VAL * retv2;
	static int retint;
	static char *retchar;
	static void (*next_func)(char **, char **);
	static _ins_t *h;
	char *ptr;

	if (!attrib) { /* Use existing values */
		if (x >= (ct-1))
			return (NULL); /* all of them found */
		x++;
	} else { /* Set up new parameters */
		/* Only set the new node struct when passing an attrib. */
		h = _parse_node(node);
		/* Set up attr to point to prototype string */
		attr = _attrib_ptype(h, attrib);
		if (get_attrib_type(node, attrib) == VAL_STRING) {
			ct = count_string(attr);
			retv = (VAL *)&retchar;
			retv2 = retv;
			next_func = _next_string_wrapper;
		} else {
			ct = count_numeric(attr);
			retv = (VAL *)&retint;
			retv2 = (VAL *)&retv;
			next_func = _next_numeric_wrapper;
		}
		x = 0;
	}
	(*next_func)(&attr, (char **)retv);
	return ((*retv != (VAL)NULL) ? *retv2 : NULL);
}

VAL
get_selected_attrib_value(NODE node, ATTRIB attrib)
{
	_ins_t *h;
	attr_list_t *value;
	val_list_t  *val;

	h = _parse_node(node);

	/* Set up attr to point to prototype string */
	value = _attrib_value(h, attrib);
	val   = _attrib_val_list(h, attrib);

	if (!value) return (VAL)NULL; /* Unknown attribute encountered */

	if (val->val_type == VAL_STRING)
		return ((VAL)(val->val.string));
	else if (val->val_type == VAL_NUMERIC)
		return ((VAL)&(val->val.integer));
	else if (val->val_type == VAL_UNUMERIC)
		return ((VAL)&(val->val.uinteger));
}

void
_free_node(_ins_t *h)
{
	_ins_t *n, *prev;

	free(h->attrib_list);
	free(h->attrib_title);
	free(h->attrib_tkey);
	free(h->attrib_ptype);
	/* Look to see if this node has already been parsed */

	for (n = prev = _nlist_head; n != (_ins_t *)NULL;
	    prev = n, n = n->next) {
		if ((void *)(n->node) == (void *)h->node) {
			break;
		}
	}

	if (n){
		if (n != _nlist_head)
		    prev->next = h->next;
		else
		    _nlist_head = _nlist_head->next;
		free(h);
	}
}


void*
set_attrib_value_typ(NODE node, ATTRIB attrib, VAL value)
{
	_ins_t *h;
	attr_list_t *vs;
	attr_list_t *vs2;
	char *depth;

	h = _parse_node(node);

	vs = _attrib_value(h, attrib);
	if (get_xin() && !strcmp(attrib, "__depth__")){
		depth = get_depth(node, value);
		vs2 = _attrib_value_typ(h, "__depth__");
		if (vs2->vlist->val_type == VAL_STRING){
			free(vs2->vlist->val.string);
			vs2->vlist->val.string = strdup((char *)depth);
		}
		_free_node(h);
		h = _parse_node(node);
	}
}

void
set_attrib_value(NODE node, ATTRIB attrib, VAL value)
{
	_ins_t *h;
	attr_list_t *vs;
	attr_list_t *vs2;
	val_list_t  *val;

	char *res;
	char *desk;
	static char *bmemsav = (char *)NULL;

	h = _parse_node(node);

	vs = _attrib_value(h, attrib);


	if (!vs) return; /* Unknown attribute encountered */
	if (get_xin() && !strcmp(attrib, "__depth__")){
		get_resdepth(node, value, &res, &desk);
		vs2 = _attrib_value_typ(h, "__resolution__");
		if (vs2->vlist->val_type == VAL_STRING){
			free(vs2->vlist->val.string);
			vs2->vlist->val.string = strdup((char *)res);
		}
		_free_node(h);
		h = _parse_node(node);
		vs2 = _attrib_value_typ(h, "__desktop__");
		if ( vs2 != NULL && vs2->vlist->val_type == VAL_STRING){
			free(vs2->vlist->val.string);
			vs2->vlist->val.string = strdup((char *)desk);
		}
		_free_node(h);
		h = _parse_node(node);
	}
	if (!strncmp(attrib, "__btype__", sizeof ("__btype__"))){
		if (!strncmp(value, "PCI", sizeof ("PCI"))){
			vs2 = _attrib_value_typ(h, "reg");

			if (vs2->vlist->val_type == VAL_STRING){
			    val_list_t *bmem_val;

			    bmemsav = vs2->vlist->next->val.string;

			    /* find "__bmemaddr__" */
			    for (bmem_val = vs2->vlist;
				bmem_val; bmem_val = bmem_val->next)
			    {
				if ((bmem_val->val_type == VAL_STRING)
					&& streq("var,__bmemaddr__",
					    bmem_val->val.string))
				    break;
			    }
			    if (bmem_val) {
				    /* bmemsav = vs2->vlist->next->val.string; */
				    bmem_val->val.string =
					xstrdup("numeric,0xA0000000");
			    }

			}
			_free_node(h);
			h = _parse_node(node);
			vs = _attrib_value(h, attrib);
		} else if (!strncmp(value, "VLB", sizeof ("VLB")) &&
				    bmemsav != NULL){
			vs2 = _attrib_value_typ(h, "reg");
			if (vs2->vlist->val_type == VAL_STRING){
				val_list_t *bmem_val;

				bmemsav = vs2->vlist->next->val.string;

				/* find "__bmemaddr__" */
				for (bmem_val = vs2->vlist;
				    bmem_val; bmem_val = bmem_val->next)
				{
				    if ((bmem_val->val_type == VAL_STRING)
					    && streq("var,__bmemaddr__",
						bmem_val->val.string))
					break;
				}
				if (bmem_val) {
					free(bmem_val->val.string);
					bmem_val->val.string = bmemsav;
				}
				bmemsav = (char *)NULL;
			}
			_free_node(h);
			h = _parse_node(node);
			vs = _attrib_value(h, attrib);
		}


	}

	val = _attrib_val_list(h, attrib);
	if (val == NULL)
		exit(0);
	if (vs->vlist->val_type == VAL_STRING) {
		free(val->val.string);
		val->val.string = strdup((char *)value);
	} else if (vs->vlist->val_type == VAL_NUMERIC)
		val->val.integer = (int)value;
	else if (vs->vlist->val_type == VAL_UNUMERIC)
		val->val.uinteger = (unsigned int)value;
}


int
is_xinside(NODE dev)
{
	return (is_xinside_attr(dev->typ_alist));
}

void
store_mon(NODE dev)
{
	store_mn_dev(dev);
}

cat_exists_t
cat_exists(cat_t catname)
{
	int i;
	NODE dp;
	cat_t dev_cat;

	for (i = 0; dp = get_dev_node(i); i++) {
		dev_cat = get_dev_cat(dp);
		if (!strcmp(dev_cat, catname))
			return (CAT_EXISTS_YES);
	}
	return (CAT_EXISTS_NO);
}

static NODE
cat_get_node(cat_t catname)
{
	int i;
	NODE dp;

	for (i = 0; dp = get_dev_node(i); i++) {
		if (!strcmp(get_dev_cat(dp), catname))
			return (dp);
	}
	return ((NODE)NULL);
}

static int
cat_get_node_index(cat_t catname)
{
	int i;
	NODE dp;

	for (i = 0; dp = get_dev_node(i); i++) {
		if (!strcmp(get_dev_cat(dp), catname))
			return (i);
	}
	return (-1);
}

NODE
get_node_by_name(cat_t catname, char * nodename)
{
	int x = 0;
	char **list;
	NODE n;

	set_cat_idx(_cat_index(catname));
	list = get_category_list(catname);
	for (x = 0; list[x]; x++) {
		n = make_dev_node(x);
		if (!strcmp(n->name, nodename))
			return (n);
		free_dev_node(n);
	}
	return ((NODE)NULL);
}
