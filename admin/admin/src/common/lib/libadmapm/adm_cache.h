
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *
 *
 *	This file contains general definitions for the cache for
 *	the administrative framework.
 *
 *
 */

#ifndef _adm_cache_h
#define	_adm_cache_h

#pragma	ident	"@(#)adm_cache.h	1.6	93/05/18 SMI"

#include <sys/types.h>
#include <sys/time.h>

/*
 *
 * General constants
 *
 */

#define	ADM_CACHE_SIZE	31	/* The size of hash table.  Should be prime */
#define	MAXBUF		1024
#define	ADM_MAXCACHEBUSY 15
#define	ADM_NUMCACHETYPES 4	/* Path and superclass */
#define	ADM_PATH_CTYPE 	0	/* index to pointer to path */
#define	ADM_SUPER_CTYPE	1	/* index to pointer to superclass */
#define	ADM_AUTH_CTYPE	2	/* index to pointer to authorization info */
#define	ADM_DOMAIN_CTYPE 3	/* index to pointer to domain info */

/*
 *
 * Typedefs
 *
 */
typedef struct adm_cache_entry {
	int signature;			/* Hash value to avoid strcmp */
	time_t TTL[ADM_NUMCACHETYPES];	/* Time to live */
	int len[ADM_NUMCACHETYPES];	/* Length of each entry */
	char *key;			/* Is this what we want? */
	char *val[ADM_NUMCACHETYPES];
	short busy;			/* Good victim? */
	struct adm_cache_entry *next;
} adm_cache_entry;

typedef struct adm_cache {
	adm_cache_entry *cache[ADM_CACHE_SIZE];
	u_int cache_size;			/* How many keys now? */
	u_int max_cache;			/* Max number of keys */
	u_int ttl_delta[ADM_NUMCACHETYPES];	/* What is each type's ttl? */
	int last_victim;		/* Where was last victim?	*/
	int cache_count_on;		/* Should we log stats? */
	u_int cstat_lookups,		/* How many seeks since last stats? */
	cstat_hits,			/* ... successes?		*/
	cstat_probes,			/* ... items looked at?		*/
					/* Do we need the remainders?	*/
	cstat_inserttry,		/* ... attempts to insert?	*/
	cstat_inserts,			/* ... successful inserts?	*/
	cstat_clean,			/* ... attempts to free space?	*/ 
	cstat_flush;			/* ... resets of cache?		*/
} adm_cache;

/*
 *
 * Global variables.
 *
 */

extern adm_cache adm_om_cache;

/*
 *
 * Exported interfaces
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

int adm_flush_cache(adm_cache *cache);
int adm_initialize_cache (adm_cache *cache, int max_cache_size, int path_ttl, 
    int super_ttl, int auth_ttl, int domain_ttl);
void adm_cache_off(adm_cache *cache);
void adm_cache_on(adm_cache *cache);
int adm_cache_lookup(char *key, int type, char value[], int length,
    adm_cache *cache);
int adm_cache_insert(char *key, int type, char value[], int length,
    adm_cache *cache);
int adm_cache_stats(adm_cache *cache, char buf[], int *len);
void adm_print_cache_stats(adm_cache *cache, int verbose);
int adm_cache_logging(adm_cache *cache, int on);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_cache_h */
