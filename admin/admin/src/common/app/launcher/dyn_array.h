#pragma	ident	"@(#)dyn_array.h	1.1	93/07/20 SMI"

/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

/*
 * dyn_array.h - General purpose dynamic array allocation
 */

typedef struct s_array {
	unsigned int	elt_size;
	unsigned int	inc_size;
	unsigned int	length;
	unsigned int	num;
	void	*array;
} Array;


Array	*array_create(unsigned int, unsigned int);
void	array_free(Array *);
int	array_add(Array *, const void *);
void	*array_get(Array *);
int	array_count(Array *);
