/*
 * Copyright (c) 1994, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)prom_node.c	1.20	96/03/13 SMI"

#include <sys/promif.h>
#include <sys/promimpl.h>

/*
 * Routines for walking the PROMs devinfo tree
 */
dnode_t
prom_nextnode(register dnode_t nodeid)
{
	cell_t ci[5];

	ci[0] = p1275_ptr2cell("peer");		/* Service name */
	ci[1] = (cell_t)1;			/* #argument cells */
	ci[2] = (cell_t)1;			/* #result cells */
	ci[3] = p1275_dnode2cell(nodeid);	/* Arg1: input phandle */
	ci[4] = p1275_dnode2cell(OBP_NONODE);	/* Res1: Prime result */

	promif_preprom();
	(void) p1275_cif_handler(&ci);
	promif_postprom();

	return (p1275_cell2dnode(ci[4]));	/* Res1: peer phandle */
}

#if	defined(__ppc)
/*
 * XXX: This belongs in the client program
 */
static int
ppc_OF_properties_supported(dnode_t nodeid)
{
	char name[8];
	int len;

	len = prom_getproplen(nodeid, "device_type");
	if ((len < 0) || (len >= 8))
		return (1);
	(void) prom_getprop(nodeid, "device_type", name);
	if (prom_strcmp(name, "isa") == 0)
		return (0);
	if (prom_strcmp(name, "scsi-2") == 0)
		return (0);
	return (1);
}
#endif

dnode_t
prom_childnode(register dnode_t nodeid)
{
	cell_t ci[5];

#if	defined(__ppc)
	if (!ppc_OF_properties_supported(nodeid))
		return (0);
#endif

	ci[0] = p1275_ptr2cell("child");	/* Service name */
	ci[1] = (cell_t)1;			/* #argument cells */
	ci[2] = (cell_t)1;			/* #result cells */
	ci[3] = p1275_dnode2cell(nodeid);	/* Arg1: input phandle */
	ci[4] = p1275_dnode2cell(OBP_NONODE);	/* Res1: Prime result */

	promif_preprom();
	(void) p1275_cif_handler(&ci);
	promif_postprom();

	return (p1275_cell2dnode(ci[4]));	/* Res1: child phandle */
}

/*
 * Create an object suitable for careening about prom trees
 */
pstack_t *
prom_stack_init(dnode_t *buf, size_t maxstack)
{
	pstack_t *p = (pstack_t *)buf;

	p->sp = p->minstack = buf + sizeof (pstack_t) / sizeof (dnode_t *);
	p->maxstack = buf + maxstack;

	return ((pstack_t *)p);
}

/*
 * Destroy the object
 */
void
prom_stack_fini(pstack_t *ps)
{
	ps->sp = (dnode_t *)0;
	ps->minstack = (dnode_t *)0;
	ps->maxstack = (dnode_t *)0;
	ps = (pstack_t *)0;
}

dnode_t
prom_findnode_bydevtype(dnode_t node, char *type, pstack_t *ps)
{
	int done = 0;

	do {
		while (node != OBP_BADNODE && node != OBP_NONODE) {
			*(ps->sp)++ = node;
			if (ps->sp > ps->maxstack)
				prom_panic(
			    "maxstack exceeded in prom_findnode_bydevtype");
			node = prom_childnode(node);
		}

		if (ps->sp > ps->minstack) {
			node = *(--ps->sp);
			if (prom_devicetype(node, type))
				return (node);
			node = prom_nextnode(node);
		} else
			done = 1;

	} while (!done);

	return (OBP_NONODE);
}

dnode_t
prom_findnode_byname(dnode_t node, char *name, pstack_t *ps)
{
	int done = 0;

	do {
		while (node != OBP_BADNODE && node != OBP_NONODE) {
			*(ps->sp)++ = node;
			if (ps->sp > ps->maxstack)
				prom_panic(
			    "maxstack exceeded in prom_findnode_byname");
			node = prom_childnode(node);
		}

		if (ps->sp > ps->minstack) {
			node = *(--ps->sp);
			if (prom_getnode_byname(node, name))
				return (node);
			node = prom_nextnode(node);
		} else
			done = 1;

	} while (!done);

	return (OBP_NONODE);
}

/*
 * Return the root nodeid.
 * Calling prom_nextnode(0) returns the root nodeid.
 */
dnode_t
prom_rootnode(void)
{
	static dnode_t rootnode;

	return (rootnode ? rootnode : (rootnode = prom_nextnode(OBP_NONODE)));
}

dnode_t
prom_parentnode(register dnode_t nodeid)
{
	cell_t ci[5];

	ci[0] = p1275_ptr2cell("parent");	/* Service name */
	ci[1] = (cell_t)1;			/* #argument cells */
	ci[2] = (cell_t)1;			/* #result cells */
	ci[3] = p1275_dnode2cell(nodeid);	/* Arg1: input phandle */
	ci[4] = p1275_dnode2cell(OBP_NONODE);	/* Res1: Prime result */

	promif_preprom();
	(void) p1275_cif_handler(&ci);
	promif_postprom();

	return (p1275_cell2dnode(ci[4]));	/* Res1: parent phandle */
}

dnode_t
prom_finddevice(char *path)
{
	cell_t ci[5];

	ci[0] = p1275_ptr2cell("finddevice");	/* Service name */
	ci[1] = (cell_t)1;			/* #argument cells */
	ci[2] = (cell_t)1;			/* #result cells */
	ci[3] = p1275_ptr2cell(path);		/* Arg1: pathname */
	ci[4] = p1275_dnode2cell(OBP_BADNODE);	/* Res1: Prime result */

	promif_preprom();
	(void) p1275_cif_handler(&ci);
	promif_postprom();

	return ((dnode_t)p1275_cell2dnode(ci[4])); /* Res1: phandle */
}

dnode_t
prom_chosennode(void)
{
	static dnode_t chosen;
	dnode_t	node;

	if (chosen)
		return (chosen);

	node = prom_finddevice("/chosen");

	if (node != OBP_BADNODE)
		return (chosen = node);

	prom_fatal_error("prom_chosennode: Can't find </chosen>\n");
	/*NOTREACHED*/
}

/*
 * Returns the nodeid of /aliases.
 * /aliases exists in OBP >= 2.4 and in Open Firmware.
 * Returns OBP_BADNODE if it doesn't exist.
 */
dnode_t
prom_alias_node(void)
{
	static dnode_t node;

	if (node == 0)
		node = prom_finddevice("/aliases");
	return (node);
}
