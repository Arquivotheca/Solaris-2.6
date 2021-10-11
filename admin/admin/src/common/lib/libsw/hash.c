/* LINTLIB */
/*
 *
 *    Copyright (c) 1991, Brian Berliner and Jeff Polk
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.3 kit.
 *
 *    Polk's hash list manager.  So cool.
 */
#ident  "@(#)hash.c 1.1 92/06/22"

/*	"$SunId: @(#)hash.c 1.3 92/04/03 SMI [RMTC] $"	*/

#include <stdio.h>
#include <string.h>
#include <memory.h>
#ifndef SVR4
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include "hash.h"
#include "sw_lib.h"

/* global caches */
static List *listcache = (List *)0;
static Node *nodecache = (Node *)0;

#ifdef __STDC__
static int hashp(char *);
static void freenode_mem(Node *);
#else
static int hashp();
static void freenode_mem();
#endif

/* hash function */
static int
hashp(key)
	char *key;
{
	register char *p;
	register int n = 0;

	for (p = key; *p; p++)
		n += (u_char)*p;

	return (n % HASHSIZE);
}

/*
 * create a new list (or get an old one from the cache)
 */
List *
#ifdef __STDC__
getlist(void)
#else
getlist()
#endif
{
	int  i;
	List *list;
	Node *node;

	if (listcache != (List *)0)
	{
		/* get a list from the cache and clear it */
		list = listcache;
		listcache = listcache->next;
		list->next = (List *)0;
		for (i = 0; i < HASHSIZE; i++)
			list->hasharray[i] = (Node *)0;
	}
	else
	{
		/* make a new list from scratch */
		list = (List *)malloc(sizeof (List));
		if (list != (List *)0) {
			(void) memset((char *)list, 0, sizeof (List));
			node = getnode();
			list->list = node;
			node->type = HTYPE_HEADER;
			node->next = node->prev = node;
		}
	}
	return (list);
}

/*
 * free up a list
 */
void
dellist(listp)
	List **listp;
{
	int  i;
	Node *p;

	if (*listp == (List *)0)
		return;

	p = (*listp)->list;

	/* free each node in the list (except header) */
	while (p->next != p)
		delnode(p->next);

	/* free any list-private data, without freeing the actual header */
	freenode_mem(p);

	/* free up the header nodes for hash lists (if any) */
	for (i = 0; i < HASHSIZE; i++)
	{
		if ((p = (*listp)->hasharray[i]) != (Node *)0)
		{
			/* put the nodes into the cache */
			p->type = HTYPE_UNKNOWN;
			p->next = nodecache;
			nodecache = p;
		}
	}

	/* put it on the cache */
	(*listp)->next = listcache;
	listcache = *listp;
	*listp = (List *)0;
}

/*
 * get a new list node
 */
Node *
#ifdef __STDC__
getnode(void)
#else
getnode()
#endif
{
	Node *p;

	if (nodecache != (Node *)0)
	{
		/* get one from the cache */
		p = nodecache;
		nodecache = p->next;
	}
	else
	{
		/* make a new one */
		p = (Node *)malloc(sizeof (Node));
	}

	if (p != (Node *)0) {
		/* always make it clean */
		(void) memset((char *)p, 0, sizeof (Node));
		p->type = HTYPE_UNKNOWN;
	}

	return (p);
}

/*
 * remove a node from its list (maybe hash list too) and free it
 */
void
delnode(p)
	Node *p;
{
	if (p == (Node *)0)
		return;

	/* take it out of the list */
	if(p->next != NULL)
		p->next->prev = p->prev;
	if(p->prev != NULL)
		p->prev->next = p->next;

	/* if it was hashed, remove it from there too */
	if (p->hashnext != (Node *)0)
	{
		p->hashnext->hashprev = p->hashprev;
		p->hashprev->hashnext = p->hashnext;
	}

	/* free up the storage */
	freenode(p);
}

/*
 * free up the storage associated with a node
 */
static void
freenode_mem(p)
	Node *p;
{
	if (p->delproc != (void (*)())0)
		p->delproc(p); 		/* call the specified delproc */
	else
	{
		if (p->data != (void *)0) /* otherwise free() it if necessary */
			free(p->data);
	}
	if (p->key != (char *)0)	/* free the key if necessary */
		free((void *)p->key);

	/* to be safe, re-initialize these */
	p->key = (char *)0;
	p->data = (void *)0;
#ifdef __STDC__
	p->delproc = (void (*)(Node *))0;
#else
	p->delproc = (void (*)())0;
#endif
}

/*
 * free up the storage associated with a node
 * and recycle it
 */
void
freenode(p)
	Node *p;
{
	/* first free the memory */
	freenode_mem(p);

	/* then put it in the cache */
	p->type = HTYPE_UNKNOWN;
	p->next = nodecache;
	nodecache = p;
}

/*
 * insert item p at end of list "list" (maybe hash it too)
 *   if hashing and it already exists, return -1 and don't actually
 *   put it in the list
 *
 *   return 0 on success
 */
int
addnode(list, p)
	List *list;
	Node *p;
{
	int hashval;
	Node *q;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("addnode");
#endif

	if (p->key != (char *)0)			/* hash it too? */
	{
		hashval = hashp(p->key);
		if (list->hasharray[hashval] == (Node *)0)
		{
			/* make header for list? */
			q = getnode();
			q->type = HTYPE_HEADER;
			list->hasharray[hashval] =
			    q->hashnext = q->hashprev = q;
		}

		/* put it into the hash list if it's not already there */
		for (q = list->hasharray[hashval]->hashnext;
		    q != list->hasharray[hashval]; q = q->hashnext)
		{
			if (strcmp(p->key, q->key) == 0)
			return (-1);
		}
		q = list->hasharray[hashval];
		p->hashprev = q->hashprev;
		p->hashnext = q;
		p->hashprev->hashnext = p;
		q->hashprev = p;
	}

	/* put it into the regular list */
	p->prev = list->list->prev;
	p->next = list->list;
	list->list->prev->next = p;
	list->list->prev = p;

	return (0);
}

/*
 * look up an entry in hash list table
 */
Node *
findnode(list, key)
	List *list;
	char *key;
{
	Node *head, *p;

	if (list == (List *)0)
		return ((Node *)0);

	head = list->hasharray[hashp(key)];
	if (head == (Node *)0)
		return ((Node *)0);

	for (p = head->hashnext; p != head; p = p->hashnext)
		if (strcmp(p->key, key) == 0)
			return (p);
	return ((Node *)0);
}

/*
 * walk a list with a specific proc
 */
int
walklist(list, proc, data)
	List *list;
#ifdef __STDC__
	int (*proc)(Node *, caddr_t);
#else
	int (*proc)();
#endif
	caddr_t data;
{
	Node *head, *p;
	int errs = 0;

	if (list == NULL)
		return (0);

	head = list->list;
	for (p = head->next; p != head; p = p->next)
		if (proc(p, data) != 0)
			errs++;
	return (errs);
}

/*
 * sort the elements of a list (in place)
 */
void
sortlist(list, comp)
	List *list;
#ifdef __STDC__
	int (*comp)(Node *, Node *);
#else
	int (*comp)();
#endif
{
	Node *head, *remain, *p, *q;

	/* save the old first element of the list */
	head = list->list;
	remain = head->next;

	/* make the header node into a null list of it's own */
	head->next = head->prev = head;

	/* while there are nodes remaining, do insert sort */
	while (remain != head)
	{
		/* take one from the list */
		p = remain;
		remain = remain->next;

		/*
		 * traverse the sorted list looking
		 * for the place to insert it
		 */
		for (q = head->next; q != head; q = q->next)
		{
			if (comp(p, q) < 0)
			{
				/* p comes before q */
				p->next = q;
				p->prev = q->prev;
				p->prev->next = p;
				q->prev = p;
				break;
			}
		}
		if (q == head)
		{
			/* it belongs at the end of the list */
			p->next = head;
			p->prev = head->prev;
			p->prev->next = p;
			head->prev = p;
		}
	}
}
