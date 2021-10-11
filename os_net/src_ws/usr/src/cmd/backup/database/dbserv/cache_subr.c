#ident	"@(#)cache_subr.c 1.4 91/12/20"

/*
 * Copyright (c) 1990,1991,1992 by Sun Microsystems, Inc.
 */

#include "defs.h"
#include <assert.h>

#define	NULL_FREELIST(h)	((struct cache_block *)&h->freelist)
#define	NULL_LRU(h)		((struct cache_block *)&h->lru)

#define	HASHSZ	8192
#define	HASH(blknum)	(blknum & (HASHSZ-1))

#ifdef __STDC__
static struct cache_block *hash_lookup(struct block_head *, u_long, int *);
static void hash_insert(struct block_head *, struct cache_block *);
static void hash_remove(struct block_head *, struct cache_block *);
static void remlist(struct cache_block *);
static void inslist(struct cache_block *, struct cache_block *);
#else
static struct cache_block *hash_lookup();
static void hash_insert();
static void hash_remove();
static void remlist();
static void inslist();
#endif

/*
 * initialize a cache
 */
struct cache_header *
cache_init(data, nitems, datasize, cp)
	caddr_t	data;		/* cache blocks */
	int nitems, datasize;	/* # of blocks and size of each */
	struct cache_header *cp;	/* reusing a cache? */
{
	register struct block_head *p;
	register struct cache_block *b;
	register int i;

	if (cp == NULL_CACHE_HEADER) {
		/*
		 * a new cache.  Allocate needed data structures:
		 *
		 *	- cache header
		 *	- hash buckets
		 *	- block structures
		 */
		if ((cp = (struct cache_header *)
				malloc(sizeof (struct cache_header))) == NULL) {
			(void) fprintf(stderr, gettext(
				"%s: out of memory\n"), "cache_init");
			return (NULL_CACHE_HEADER);
		}
		if ((cp->hb = (struct block_head *) malloc(
				sizeof (struct block_head)*HASHSZ)) == NULL) {
			free((char *)cp);
			(void) fprintf(stderr, gettext(
				"%s: out of memory\n"), "cache_init");
			return (NULL_CACHE_HEADER);
		}
		if ((b = (struct cache_block *)malloc((unsigned)
			(nitems*sizeof (struct cache_block)))) == NULL) {
			free((char *)cp->hb);
			free((char *)cp);
			(void) fprintf(stderr, gettext(
				"%s: out of memory\n"), "cache_init");
			return (NULL_CACHE_HEADER);
		}
		cp->blkdescrip = b;
		cp->cache_blksize = datasize;
		cp->cache_size = nitems;
	} else {
		/*
		 * re-use an old cache header provided that all parameters
		 * match.
		 */
		if (cp->cache_blksize != datasize)
			return (NULL_CACHE_HEADER);
		if (cp->cache_size != nitems)
			return (NULL_CACHE_HEADER);
		b = cp->blkdescrip;
	}
	/*
	 * initialize buckets (all empty)
	 */
	for (i = 0, p = cp->hb; i < HASHSZ; i++, p++)
		p->link = p->rlink = (struct cache_block *)p;
	cp->lru.nxt = cp->lru.prv = (struct cache_block *)&cp->lru;
	cp->freelist.nxt = cp->freelist.prv =
			(struct cache_block *)&cp->freelist;
	/*
	 * initialize freelist
	 */
	for (i = 0; i < nitems; i++, b++, data += datasize) {
		b->data = data;
		b->nxt = b->prv = b->link = b->rlink = NULL;
		inslist(b, cp->freelist.nxt);
	}
	return (cp);
}

/*
 * get the specified block.  If it's already in the cache (and valid)
 * just return it.  Otherwise, recycle a block and read the requested
 * data into it.
 */
struct cache_block *
cache_getblock(h, blknum)
	struct cache_header *h;
	u_long blknum;
{
	struct cache_block *b;
	int search_cnt = 0;

	if (blknum == NONEXISTENT_BLOCK)
		return (NULL_CACHE_BLOCK);

	if (b = hash_lookup(h->hb, blknum, &search_cnt)) {
		remlist(b);		/* remove from LRU list */
		inslist(b, h->lru.prv);	/* re-insert at end of LRU */
		if (b->flags & CACHE_ENT_VALID) {
			return (b);
		} else {
			(void) fprintf(stderr, gettext(
				"%s: in tree but not valid!\n"), "getblock");
			return (b);
		}
	} else {
		return (NULL_CACHE_BLOCK);
	}
}

/*
 * allocate a new cache block
 * If the block is currently dirty, the caller is responsible for seeing
 * that it is written to disk...
 * Also, caller must see that this block gets hashed after it is
 * filled in.
 */
struct cache_block *
cache_alloc_block(h)
	struct cache_header *h;
{
	struct cache_block *b;

	if ((b = h->freelist.nxt) != NULL_FREELIST(h)) {
		remlist(b);			/* take off freelist */
		inslist(b, h->lru.prv);		/* back of lru list */
		b->flags = 0;
		return (b);
	}
	b = h->lru.nxt;
	if (b == NULL_LRU(h)) {
		(void) fprintf(stderr, gettext(
			"%s: cannot get freelist or lru"), "alloc_block");
		exit(1);
	}

	if (b->flags & CACHE_ENT_VALID) {
		hash_remove(h->hb, b);
	}
	remlist(b);
	inslist(b, h->lru.prv);
	return (b);
}

void
cache_insert(h, bp)
	struct cache_header *h;
	struct cache_block *bp;
{
	bp->flags |= CACHE_ENT_VALID;
	hash_insert(h->hb, bp);
}

/*
 * mark a cache block dirty
 */
void
cache_dirtyblock(h, bnum)
	struct cache_header *h;
	u_long	bnum;
{
	struct cache_block *bp;
	int cnt;

	bp = hash_lookup(h->hb, bnum, &cnt);
	if (bp == NULL_CACHE_BLOCK) {
		(void) fprintf(stderr, gettext(
			"%s: cannot find block %lu\n"), "dirtyblock", bnum);
		return;
	}
	bp->flags |= CACHE_ENT_DIRTY;
}

/*ARGSUSED*/
static struct cache_block *
hash_lookup(hb, blknum, cnt)
	struct block_head *hb;
	u_long	blknum;
	int *cnt;
{
	register struct block_head *bucket;
	register struct cache_block *p;

	bucket = &hb[HASH(blknum)];
	for (p = bucket->link; p != (struct cache_block *)bucket; p = p->link) {
		if (p->blknum == blknum)
			return (p);
	}
	return (NULL_CACHE_BLOCK);
}

static void
hash_insert(hb, bp)
	struct block_head *hb;
	struct cache_block *bp;
{
	struct block_head *bucket;

	bucket = &hb[HASH(bp->blknum)];

	bp->link = bucket->link;
	bp->rlink = (struct cache_block *)bucket;
	bucket->link->rlink = bp;
	bucket->link = bp;

}

/*ARGSUSED*/
static void
hash_remove(hb, bp)
	struct block_head *hb;
	struct cache_block *bp;
{
	bp->rlink->link = bp->link;
	bp->link->rlink = bp->rlink;
}

static void
remlist(b)
	struct cache_block *b;
{
	b->prv->nxt = b->nxt;
	b->nxt->prv = b->prv;
}

static void
inslist(b2, b1)
	struct cache_block *b2, *b1;
{
	register struct cache_block *b3;

	b3 = b1->nxt;
	b1->nxt = b2;
	b2->nxt = b3;
	b3->prv = b2;
	b2->prv = b1;
}
