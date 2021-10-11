
/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 *	Object Manager Cache
 *
 *	Table is array of links.  Each link can point to acm_cache_entry
 *	Entry holds
 *		key		class_name or class_name/method_name
 *		signature	Hash of key: may save strcmp
 *		busy		Has this been used recently?  This is a
 *				count maintained by cache routines: not time
 *		val[]		Array of pointers to values
 *		TTL[]		Array of Time To Live values - uses clock
 *		next		Pointer to other entries which hash to
 *					same slot in table
 *
 *	To check the cache, (adm_cache_check) we have a _key_ and a _type_
 *		1) Hash entry
 *		2) Step down linked list, checking for items with same sig.
 *		3) When found, we compare strings
 *		4) If match, check that val[type] is non-null and TTL < time
 *		5) If we have a hit, reset busy, and return value
 *	We age each item by reducing busy as we check it.
 *	If we find key, but val is empty or stale, we return pointer to key, so
 *		that insert can use this location.
 *
 *	Insertion (adm_cache_insert) takes _key_, _type_, and _value_
 *	We first call check_cache, which return a null pointer or
 *	a pointer to a node with the right key value.
 *	If the node is null, we insert at front of list.
 *	If the node is there, we replace the item of this type (if any)
 *	    with the new item, and reset the item's TTL.
 *	Insertion checks the total number of elements in the cache.
 *	If it exceeds the run-time limit on the cache size, we
 *	    refuse the insertion and call clean_cache to remove old items
 *
 *	Relation of OLD (busy counter) to STALE (TTL)
 *	STALE is more accurate, but makes a system call.  Thus we
 *	use BUSY more often.
 */

#pragma	ident	"@(#)adm_cache.c	1.21	92/09/04 SMI"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdarg.h>

		/* used for rand: only for debugging */
#include <stdlib.h>

#include "adm_cache.h"
#include "adm_om_impl.h"	/* Needs to know default sizes.  Bug? XXX */
#include "adm_fw.h"
#include "adm_fw_impl.h"	/* Defines ADM_DBG */

		/* Local definitions and macros */
#define	VERBOSE		0	/* How chatty are we? */
#define	ADM_CACHE_MUTEX_READLOCK	0
#define	ADM_CACHE_MUTEX_WRITELOCK	1
#define ADM_CACHE_DEBUG_ON		0	/* Are we debugging cache? */

#define	CACHE_MISS	0
#define CACHE_HIT	1
#define CACHE_STALE	2

#define	EMPTY(p)	(p == (struct adm_cache_entry *)NULL)
#define	OLD(p)		(p->busy == 0)
#define	STALE(p, type)	(p->TTL[type] < time((time_t *)NULL))
#define	P(s)		((s == (char *)NULL) ? "." : s)

extern int adm_caching_on;	/* -1 = unint, 0 = off, 1 = on */

/*
 *	Local prototypes
 */

static int adm_clean_cache(adm_cache *cache);
static int adm_hash(char *s);
static int adm_dispose(struct adm_cache_entry **probe, adm_cache *cache);
static void adm_print_cache(adm_cache *cache);

/*
 *	Local variable
 */

static lock_count = 0;	/* XXX used to detect too many locks... */

/*
 *	adm_cache_debug
 *
 *	Print debug statments in common format
 */
static void
adm_cache_debug (char *msg_format, ...)
{
	va_list ap;

	printf("(%ld) ", (long) getpid());

	va_start(ap, msg_format);
	vprintf(msg_format, ap);
	va_end(ap);
}

/*
 *	adm_cache_logging_on
 *
 *	Turn on counting for logging
 */
int
adm_cache_logging(adm_cache *cache, int on)
{
	int was_on;

	was_on = cache->cache_count_on;
	if (on)
		cache->cache_count_on = 1;
	else
		cache->cache_count_on = 0;

	return (was_on);
}

/*
 *	adm_cache_mutex_lock
 *
 *	Make sure no-one else is mucking with your item
 *	XXX These should be maintained w/ MT Locks, when they arrive
 *	Are these on a per-cache or on a per item level?
 *	These chould be at cache[pos] level.
 */
int
adm_cache_mutex_lock(adm_cache *cache, int type)
{
	if (lock_count != 0)	{
		ADM_DBG("c", ("Cache:   lock: count = %d\n", lock_count));
		return (1);
	}
	lock_count++;

	return (0);
}

/*
 *	adm_cache_mutex_unlock
 *
 *	Release your lock
 *	XXX These should be maintained w/ MT Locks, when they arrive
 */
int
adm_cache_mutex_unlock(adm_cache *cache)
{
	if (lock_count != 1)	{
		ADM_DBG("c", ("Cache:   unlock: count %d\n", lock_count));
	}
	lock_count--;

	return (0);
}

/*
 *	adm_flush_cache
 *
 *	clear the decks - remove all items in the cache
 */
int
adm_flush_cache(adm_cache *cache)
{
	int i;

	ADM_DBG("c", ("Cache:  Flush the cache"));

	for (i = ADM_CACHE_SIZE-1; i >= 0; i--)	{
		while (!EMPTY(cache->cache[i]))	{
			adm_dispose(&(cache->cache[i]), cache);
		}
	}

	if (cache->cache_count_on)
		cache->cstat_flush++;
	return (0);
}

/*
 *	adm_initialize_cache
 *
 *	Set up an initial cache
 */
int
adm_initialize_cache (adm_cache *cache, int max_cache_size, int path_ttl,
    int super_ttl, int auth_ttl, int domain_ttl)
{
	int i;

	if (adm_caching_on == 0) {
		ADM_DBG("c", ("Cache:  Turn off the cache"));
		return (1);
	}

	ADM_DBG("c", ("Cache:  Turn on the cache"));
	adm_caching_on = 1;

	cache->cstat_lookups	= 0,
	cache->cstat_hits	= 0,
	cache->cstat_probes	= 0,
	cache->cstat_inserttry	= 0,
	cache->cstat_inserts	= 0,
	cache->cstat_clean	= 0,
	cache->cstat_flush	= 0;
	cache->cache_size = 0;

	if (max_cache_size < 0)
		cache->max_cache    = OM_CACHE_SIZE;
	else
		cache->max_cache    = max_cache_size;


	if (path_ttl < 0)
		cache->ttl_delta[0] = OM_PATH_TTL;
	else
		cache->ttl_delta[0] = path_ttl;

	if (super_ttl < 0)
		cache->ttl_delta[1] = OM_SUPER_TTL;
	else
		cache->ttl_delta[1] = super_ttl;

	if (auth_ttl < 0)
		cache->ttl_delta[2] = OM_ACL_TTL;
	else
		cache->ttl_delta[2] = auth_ttl;

	if (domain_ttl < 0)
		cache->ttl_delta[3] = OM_DOMAIN_TTL;
	else
		cache->ttl_delta[3] = domain_ttl;

	for (i = ADM_CACHE_SIZE-1; i >= 0; i--)	{
		cache->cache[i] = (struct adm_cache_entry *) NULL;
	}
	cache->last_victim = ADM_CACHE_SIZE - 1;

	return (0);
}

/*
 *	adm_cache_off
 *
 *	A simple interface to turn off a cache
 */
void
adm_cache_off (adm_cache *cache)
{
	adm_caching_on = 0;
	cache->cache_count_on = 0;
	adm_flush_cache(cache);
	cache->max_cache = 0;
}

/*
 *	adm_cache_on
 *
 *	A simple interface to turn on a cache
 */
void
adm_cache_on (adm_cache *cache)
{
	adm_caching_on = 1;
	adm_initialize_cache (cache, -1, -1, -1, -1, -1);
	cache->cache_count_on = 1;
}

/*
 *	adm_cache_stats
 *
 *	Fill a buffer with status information
 */
int
adm_cache_stats(adm_cache *cache, char buf[], int *len)
{
	ADM_DBG("c", ("Cache:  Adm_cache_stats"));

	sprintf(buf, "OM Cache Stats: Lookups %u Hits %u Ratio %u%%\n",
	    cache->cstat_lookups, cache->cstat_hits,
	    ((cache->cstat_lookups == 0) ? 0 :
		(100*cache->cstat_hits)/cache->cstat_lookups));

	cache->cstat_lookups	= 0;
	cache->cstat_hits	= 0;
	cache->cstat_probes	= 0;
	cache->cstat_inserttry	= 0;
	cache->cstat_inserts	= 0;
	cache->cstat_clean	= 0;
	cache->cstat_flush	= 0;

	return(ADM_SUCCESS);
}

/*
 *	adm_print_cache_hits
 *
 *	Monitoring code to track cache hits, usage, etc.
 *	XXX Remove from production version
 */
void
adm_print_cache_hits(adm_cache *cache)
{
	ADM_DBG("c", ("Cache:  Adm_print_cache_hits"));

	printf("\n\tCache Stats\n\n");
	printf("Lookups\t%u\tProbes\t%u\tHits\t%u\n", cache->cstat_lookups,
	    cache->cstat_probes, cache->cstat_hits);

	printf("Hits per lookup: %u%%\n", ((cache->cstat_lookups == 0) ? 0 :
	    (100*cache->cstat_hits)/cache->cstat_lookups));

	cache->cstat_lookups   = 0;
        cache->cstat_hits      = 0;
	cache->cstat_probes	= 0;
	cache->cstat_inserttry	= 0;
	cache->cstat_inserts	= 0;
	cache->cstat_clean	= 0;
	cache->cstat_flush	= 0;
}

/*
 *	adm_print_cache
 *
 *	Debugging code to print the cache
 *	XXX Remove from production version
 */
void
adm_print_cache(adm_cache *cache)
{
	int i;
	struct adm_cache_entry *p;
	int found;

	printf ("\n\n\tTable\n\n");

	for (i=0; i<ADM_CACHE_SIZE; i++)	{
		found = 0;
		p = cache->cache[i];
		while (!EMPTY(p))	{
			printf("%d\t%s\t%s\t%s\t%s\t%d\n", i, p->key,
			    P(p->val[0]), P(p->val[1]), P(p->val[3]), p->busy);
			p = p->next;
			found = 1;
		}
		if (found)
			printf ("\n");
	}
}

/*
 *	adm_print_cache_stats
 *
 *	Collect and print stats about cache usage
 */
void
adm_print_cache_stats(adm_cache *cache, int verbose)
{
	int i, j;
	struct adm_cache_entry *p;
	int	count = 0,
		size[ADM_NUMCACHETYPES],
		stale[ADM_NUMCACHETYPES],
		type_count[ADM_NUMCACHETYPES];

	if (!verbose)	{
		adm_print_cache_hits(cache);
		return;
	}

	for (j=0; j<ADM_NUMCACHETYPES; j++)	{
		size[j] = 0;
		stale[j] = 0;
		type_count[j] = 0;
	}
	for (i=0; i<ADM_CACHE_SIZE; i++)	{
		p = cache->cache[i];
		while (!EMPTY(p))	{
			count++;
			for (j=0; j<ADM_NUMCACHETYPES; j++)	{
				if (p->val[j] != (char *)NULL)	{
					type_count[j]++;
					if (p->TTL[j] < time((time_t *)NULL))
						stale[j]++;
					size[j] = size[j] + p->len[j];
				}
			}
			p = p->next;
		}
	}

	printf ("Number in cache: %d\n", count);

	if (count != cache->cache_size)	{
		printf ("Cache_size wrong: assumed: %d, true = %d\n",
		    cache->cache_size, count);
		cache->cache_size = count;
		adm_print_cache(cache);
	}
	printf ("What is stored in each type?\n");
	printf ("# of entries\t# stale\tSize in bytes\n");
	for (j=0; j<ADM_NUMCACHETYPES; j++)	{
		printf ("%d\t%d\t%d\t%d\n", j, type_count[j], stale[j],
		    size[j]);
	}
	adm_print_cache(cache);
	adm_print_cache_hits(cache);
}


/*
 *	Adm_clean_cache()
 *
 *	Remove dead entries from the cache
 */
static int
adm_clean_cache(adm_cache *cache)
{
	struct adm_cache_entry *prev;
	int i, v_type;
	int dispose_count = 0, free_count = 0;

	ADM_DBG("c", ("Cache:  Adm_clean_cache"));

	for (i = ADM_CACHE_SIZE-1; i >= 0; i--)	{
		while (!EMPTY(cache->cache[i]) && OLD(cache->cache[i])) {
			adm_dispose(&(cache->cache[i]), cache);
			dispose_count++;
		}
		if (EMPTY(cache->cache[i]))	{
			continue;
		}
		cache->cache[i]->busy = cache->cache[i]->busy >> 1;
		prev = cache->cache[i];
		while (!EMPTY(prev->next))	{
			for (v_type = 0; v_type < ADM_NUMCACHETYPES; v_type++) {
				if (prev->TTL[v_type] < time((time_t *)NULL)) {
					/* Note that we can wind up with */
					/* entry with key and no values */

					free(prev->val[v_type]);
					free_count++;
					prev->val[v_type] = (char *)NULL;
				}
			}

			prev->next->busy = prev->next->busy >> 1;
			if OLD(prev->next)	{
				adm_dispose(&prev->next, cache);
				dispose_count++;
			} else {
				prev = prev->next;
			}
		}
	}
	if (VERBOSE)	{
		adm_print_cache(cache);
	}
	cache->cstat_clean++;

	while (dispose_count == 0)	{	/* We need to pick a victim */
		cache->last_victim++;
		if (cache->last_victim == ADM_CACHE_SIZE)
			cache->last_victim = 0;

		if (!EMPTY(cache->cache[cache->last_victim]))	{
			adm_dispose(&(cache->cache[cache->last_victim]), cache);
			dispose_count++;
		}
	}

	return (0);
}

/*
 *	Hash
 *
 *	Takes a character string and returns an int.
 *	Algorithm shifts left and XORs new character
 */

static int
adm_hash(char *s)
{
	char *p;
	int val = 0;

	for (p = s; *p != '\0'; p++)	{
		val = (val << 1) ^ *s;
	}
	return (val);
}

/*
 *	adm_cache_check
 *
 *	This routine looks up items and returns their value
 *	Use by adm_cache_lookup and adm_cache_insert (which is why
 *	we return the pos and signature of key)
 *	Datastructure: array of pointers which point to nodes.
 *	Nodes have key, signature, busy field, and pointers to byte strings
 *	indexed by type (superclass, path, acl, ...)
 *	Each bytestring has a private TTL counter, so we can make path
 *	info age slower than security info
 *	Nodes are kept in a linked list.  We traverse list, looking for
 *	node with the same sig.  When found, we compare keys.
 *	If they match, we check date and the
 *	There are two ways to age: busy field becomes 0 or ttl dates
 *	Insertion for new nodes happens at front of list
 *
 *	Bugs: Items near front of list age more rapidly than later items
 *	When reclaiming, we don't check to see if items are stale:
 *	just if they aren't busy (see macros OLD vs STALE)
 *	This is more efficient, but not as accurate
 */

struct adm_cache_entry *
adm_cache_check(char *key, int *pos, int *sig, int type, char value[], int len,
    adm_cache *cache, int *return_code)
{
	struct adm_cache_entry *status, *probe, *prev;

	*return_code = CACHE_MISS;

	if (ADM_CACHE_DEBUG_ON)
	 	ADM_DBG("c", ("Cache:  Cache_check key %s type %d", key, type));

	if (cache->cache_count_on)
		cache->cstat_lookups++;

	*sig = adm_hash(key);
	*pos = *sig % ADM_CACHE_SIZE;

	value[0] = '\0';	/* Return value */

		/* For saftey, include redundant check on adm_caching_on */
	if ((!adm_caching_on) || (cache->max_cache == 0))	{
		ADM_DBG("c", ("Cache:  Adm_cache_check: cache is off"));
		return ((struct adm_cache_entry *)NULL);
	}

	status = (struct adm_cache_entry *)NULL;
	probe = cache->cache[*pos];
	prev = (struct adm_cache_entry *)NULL;

	if (cache->cache_count_on)
		cache->cstat_probes++;
	while (!EMPTY(probe))	{	/* and not found */
		/*
		 * ADM_DBG("c", ("Cache:  cache_check: probe is %d", *pos));
		 */
	
		if ((probe->signature == *sig) &&
		    (!strcmp(probe->key, key)))	{

			if (ADM_CACHE_DEBUG_ON)
				ADM_DBG("c", ("Cache:  Hit key!"));

			status = probe;
			probe->busy = ADM_MAXCACHEBUSY;

			/* Is there anything to look at? */
			if (probe->val[type] == (char *)NULL) {

				if (ADM_CACHE_DEBUG_ON)
					ADM_DBG("c", ("Cache:  Empty value"));

				break;
			}

			if (probe->TTL[type] < time((time_t *)NULL)) {
				ADM_DBG("c", ("Cache:  Hit stale value"));

				*return_code = CACHE_STALE;

				if (probe->val[type] != (char *)NULL) {
					free(probe->val[type]);
					probe->val[type] = (char *)NULL;
				}
				break;
			}

			if ((probe->val[type] != (char *)NULL) &&
			    (probe->len[type] <= len))	{

				if (ADM_CACHE_DEBUG_ON)
					ADM_DBG("c", ("Cache:  Hit value!"));

				*return_code = CACHE_HIT;

				memcpy(value, probe->val[type],
				    probe->len[type]);
			}
			break;
		}
			/* Age the entry */
		probe->busy = probe->busy >> 1;
		if OLD(probe)	{
			/* Note that adm_dispose will advance prev->next */
			/* XXX I changed this: added else to backet assign */
			/* XXX and added re-assignment of probe. */
			if (EMPTY(prev)) {
				adm_dispose(&(cache->cache[*pos]), cache);
				probe = cache->cache[*pos];
			} else {
				adm_dispose(&prev->next, cache);
				probe = prev->next;
			}
		} else {
			prev = probe;
			probe = probe->next;
		}
		if (cache->cache_count_on)
			cache->cstat_probes++;
	}

	if (cache->cache_count_on)	{
		if (!EMPTY(status))
			cache->cstat_hits++;
	}

	return (status);
}

/*
 *	adm_cache_lookup
 *
 *	This provides an interface to adm_cache_check that does not
 *	need to know about adm_cache_entry *
 *	Insert, however, needs more from adm_cache_check: must keep position
 */
int
adm_cache_lookup(char *key, int type, char value[], int len, adm_cache *cache)
{
	int temp, temp2;
	int status = 0;

	if (ADM_CACHE_DEBUG_ON)
		ADM_DBG("c", 
		    ("Cache:  Cache_lookup for key %s, type %d", key, type));

	if ((!adm_caching_on) || (cache->max_cache == 0))	{
		if (ADM_CACHE_DEBUG_ON)
			ADM_DBG("c", 
			    ("Cache:  Adm_cache_lookup: cache is off"));
		return (1);
	}

	/* Seems counter-intuitive to lock: but cache_check deletes */
	if (adm_cache_mutex_lock(cache, ADM_CACHE_MUTEX_WRITELOCK)) {
		ADM_DBG("c", ("Cache:  Adm_cache_lookup: could not lock"));
		status = 1;		 /* Could not lock */
	} else {
		adm_cache_check(key, &temp, &temp2, type, value, len,
		    cache, &status);

		if (status == CACHE_HIT) {
			status = 0;
		} else {
			status = 1;
		}
		adm_cache_mutex_unlock(cache);
	}
	return (status);
}

/*
 *	dispose(ptr)
 *
 *	Private routine to dispose of a node in the hash table
 *	Frees storage and updates cachesize pointer
 *	Note that dispose advances the pointer ptr to next spot
 */

static int
adm_dispose(struct adm_cache_entry **probe, adm_cache *cache)
{
	struct adm_cache_entry *temp;
	int i;

	temp = *probe;	/* The one to delete */
	*probe = temp->next;

	if (temp->key != (char *)NULL)	{
		free(temp->key);
		temp->key = (char *)NULL;
	}
	for (i=0; i <ADM_NUMCACHETYPES; i++)	{
		if (temp->val[i] != (char *)NULL)	{
			free(temp->val[i]);
			temp->val[i] = (char *)NULL;
		}
	}
	temp->next = (struct adm_cache_entry *)NULL;

	free((char *)temp);
	cache->cache_size--;
	return (0);
}

/*
 *	adm_cache_insert
 *
 *	Try to insert a value into the cache
 *	Call adm_cache_check to locate a spot.  Mutex lock, and
 *	try to insert
 */

int
adm_cache_insert(char *key, int type, char value[], int length,
    adm_cache *cache)
{
	struct adm_cache_entry *probe;
	int status = 0;
	int cache_status = 0;
	int sig, pos, i;
	char trash[100];

	if (ADM_CACHE_DEBUG_ON)
		ADM_DBG("C", ("Cache:  Memory on entry %x", sbrk(0)));

	ADM_DBG("c", ("Cache:  Insert key %s value %s type %d in cache", 
	    P(key), P(value), type));

	if ((!adm_caching_on) || (cache->max_cache == 0))	{
		ADM_DBG("c", ("Cache:  Adm_cache_insert: cache is off"));
		return (1);	/* quick exit for degenerate cases */
	}

	if (value == (char *)NULL) {
		ADM_DBG("c", ("Cache:  Attempt to insert empty value"));
		return (1);
	}

	if (cache->cache_count_on)
		cache->cstat_inserttry++;

	if (adm_cache_mutex_lock(cache, ADM_CACHE_MUTEX_WRITELOCK)) {
		status = 1;
	} else {			 /* try to make room */
		if (cache->cache_size >= cache->max_cache)	{

			if (ADM_CACHE_DEBUG_ON)
				ADM_DBG("C", ("Cache:  Insert 1 calls clean"));

			adm_clean_cache(cache);
			if (cache->cache_size >= cache->max_cache)
				status = 1;
		}
		probe = adm_cache_check(key, &pos, &sig, type, trash, 100,
		    cache, &cache_status);

		if (EMPTY(probe))	{
			if (ADM_CACHE_DEBUG_ON)
				ADM_DBG("C", ("Cache:  new entry"));
			probe = (struct adm_cache_entry *)
			    malloc (sizeof (adm_cache_entry));
			if (EMPTY(probe))	{
				if (ADM_CACHE_DEBUG_ON)
					ADM_DBG("C", 
					    ("Cache:  Insert 2 calls clean"));
				adm_clean_cache(cache);
				status = 1;
				goto insert_fail;
			}

			/* Probe is not empty.  Start insert into list */
			/* Not commited until key is in place */
			probe->signature = sig;
			probe->next = cache->cache[pos];
			probe->busy = ADM_MAXCACHEBUSY;
			for (i = 0; i < ADM_NUMCACHETYPES; i++)	{
				probe->val[i] = (char *)NULL;
				probe->TTL[i] = 0;
			}

			probe->key = (char *)malloc(strlen(key) + 1);
			if (probe->key == (char *)NULL)	{
				if (ADM_CACHE_DEBUG_ON)
					ADM_DBG("C", 
					    ("Cache:  Insert 3 calls clean"));
				adm_clean_cache(cache);
				free(probe);
				status = 1;
				goto insert_fail;
			}
			strcpy(probe->key, key);
			cache->cache[pos] = probe;
			++cache->cache_size;
			if (ADM_CACHE_DEBUG_ON)
			 	ADM_DBG("c", 
			 	    ("Cache:  Adm_cache_insert key %s", key));

		}		/* end if EMPTY(probe) - new node in place */

		if (status == 0)	{

			/* probe points to a valid node in the hash table */
			if (probe->val[type] != (char *)NULL)	{
				free(probe->val[type]);
				probe->val[type] = (char *)NULL;
			}

			probe->val[type] = (char *)malloc(length);
			if (probe->val[type] == (char *)NULL)	{
				if (ADM_CACHE_DEBUG_ON)
					ADM_DBG("C", 
					    ("Cache:  Insert 4 calls clean"));
				adm_clean_cache(cache);
				status = 1;
			}
		}

		if (status == 0)	{
			memcpy(probe->val[type], value, length);
			probe->len[type] = length;
			probe->TTL[type] = time((time_t *)NULL) +
			    cache->ttl_delta[type];
		
			if (ADM_CACHE_DEBUG_ON)
				ADM_DBG("c", ("Cache:  cache inserted value"));

			if (cache->cache_count_on)
				cache->cstat_inserts++;
		}
		insert_fail:
			adm_cache_mutex_unlock(cache);
		if (status != 0)
			ADM_DBG("c", ("Cache:  Adm_cache_insert failed"));
	}

	if (ADM_CACHE_DEBUG_ON)
		ADM_DBG("C", ("Cache:  Memory on exit  %x", sbrk(0)));

	return (status);
}
