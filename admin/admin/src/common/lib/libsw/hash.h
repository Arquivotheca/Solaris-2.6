/*
 *	Copyright (c) 1991, Brian Berliner and Jeff Polk
 *
 *	You may distribute under the terms of the GNU General Public License
 *	as specified in the README file that comes with the CVS 1.3 kit.
 */

#pragma ident "@(#)hash.h 1.5 93/10/22 SMI"

#ifndef SWM_HASH_H
#define	SWM_HASH_H

#include <sys/types.h>
/*
 * The number of buckets for the hash table contained
 * in each list.  This should probably be prime.
 */
#define	NULL 		0
#define	HASHSIZE	151

/*
 * Types of nodes
 */

enum ntype {
	HTYPE_UNKNOWN,
	HTYPE_HEADER
};
typedef enum ntype Ntype;

struct node {
	Ntype	type;
	struct node	*next;
	struct node	*prev;
	struct node	*hashnext;
	struct node *hashprev;
	char	*key;
	void	*data;
#ifdef __STDC__
	void	(*delproc)(struct node *);
#else
	void	(*delproc)();
#endif /* __STDC__ */
};
typedef struct node Node;

struct list {
	Node	*list;
	Node	*hasharray[HASHSIZE];
	struct list *next;
};
typedef struct list List;

struct entnode {
	char *version;
	char *timestamp;
	char *options;
	char *tag;
	char *date;
};
typedef struct entnode Entnode;

#ifdef __STDC__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern List	*getlist(void);
extern void	dellist(List **);
extern Node	*getnode(void);
extern void	delnode(Node *);
extern void	freenode(Node *);
extern int	addnode(List *, Node *);
extern Node	*findnode(List *, char *);
extern int	walklist(List *, int (*)(Node *, caddr_t), caddr_t);
extern void	sortlist(List *, int (*)(Node *, Node *));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#else
extern List	*getlist();	/* list = getlist(); */
extern void	dellist();	/* dellist(&list); */
extern Node	*getnode();	/* node = getnode(); */
extern void	delnode();	/* delnode(node); */
extern void	freenode();	/* freenode(node) */
extern int	addnode();	/* exist_err = addnode(list, node); */
extern Node	*findnode();	/* node = findnode(list, key); */
extern int	walklist();	/* errors = walklist(list, function); */
extern void	sortlist();	/* sortlist(list, compare); */
#endif /* __STDC__ */

#endif	/* !SWM_HASH_H */

