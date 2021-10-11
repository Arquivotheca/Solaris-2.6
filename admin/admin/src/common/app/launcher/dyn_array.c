#pragma	ident	"@(#)dyn_array.c	1.2	93/10/18 SMI"

/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

/*
 * dyn_array.c - General purpose dynamic array allocation
 */

#include <stdlib.h>
#include <string.h>
#include "dyn_array.h"


/*
 * array_create
 *
 * Create a grow-able array.
 *
 * input:
 *	int		elt_size;	size of the elements to be added
 *	int		inc_size;	grow array in chunks of "inc_size"
 *
 * returns:
 *	returns a handle on a dynamically grow-able array if
 *	successful, returns NULL on error.  The only error
 *	condition is an out of memory malloc failure.
 *
 * algorithm:
 *	Allocate the maintenance structure for the array, fill
 *	in the element size (elt_size) and array increment size
 *	(inc_size) fields, allocate space for the first "inc_size"
 *	elements, save the current length of the allocated space
 *	in "length", and return.
 */

Array	*
array_create(unsigned int elt_size, unsigned int inc_size)
{

	Array	*ret;


	ret = (Array *)malloc((unsigned)sizeof (Array));

	if (ret) {

		ret->array = malloc((unsigned)(elt_size * inc_size));
		if (ret->array) {
			ret->elt_size = elt_size;
			ret->inc_size = inc_size;
			ret->length = inc_size;
			ret->num = 0;
		} else {
			free((void *)ret);
			ret = (Array *)NULL;
		}
	}

	return ret;
}


/*
 * array_free
 *
 * Free the maintenance structure for a dynamic array.
 *
 * input:
 *	Array		*array;		the array handle, from array_create()
 *
 * returns:
 *	no return value.  free()'s its argument as a side effect.
 *
 * algorithm:
 *	Free the maintenance structure for a dynamic array.  It
 *	is assumed that the caller has already extracted the
 *	size and/or array from the structure via array_count()
 *	and/or array_get() before calling array_free().
 */

void
array_free(Array *array)
{
	if (array)
		free((void *)array);
}


/*
 * array_add
 *
 * Add an element to a grow-able array.
 *
 * input:
 *	Array		*array;		the array handle, from array_create()
 *	const void	*elt;		pointer to the element to be added
 *
 * returns:
 *	If "elt" is successfully added to the array, the number
 *	of elements in the array is returned, otherwise return -1.
 *
 * algorithm:
 *	Check the current length of the array, and the number
 *	of elements that have been added; if they're equal, then
 *	the array is full, and needs to be reallocated.  Save
 *	the current array, so that if the realloc fails we don't
 *	loose the current pointer on a NULL return from realloc.
 *	If realloc successful, or the array wasn't full, add the
 *	new element and increment the number of elements in the
 *	array.
 */

int
array_add(Array *array, const void *elt)
{

	void		*save;
	unsigned int	bytes;


	if (! array)
		return -1;

	/*
	 * We have a malloc'd array big enough to hold "length"
	 * elements, it currently has "num" elements in it, and
	 * we want to add another one.  If there's room, copy
	 * it in and increment "num"; if not, realloc, copy it
	 * in, and increment.
	 */

	if (array->num == array->length) {

		/* we'll overflow, need to realloc */

		save = array->array;

		array->length += array->inc_size;
		bytes = array->length * array->elt_size;

		array->array = realloc(array->array, bytes);

		if (array->array == NULL) {

			/*
			 * reallocation failed; put things back the
			 * way they were and return error condition
			 * to the caller.
			 */

			array->array = save;
			array->length -= array->inc_size;

			return -1;
		}
	}

	/*
	 * Success, add the new element and increment
	 * the count of elements in the array.
	 */

	if (elt != NULL) {
		(void) memcpy((char *)array->array +
			      (array->num * array->elt_size),
			      elt, (size_t)array->elt_size);
	} else {
		(void) memset((char *)array->array +
			      (array->num * array->elt_size),
			      0, (size_t)array->elt_size);
	}

	return (++array->num);
}


/*
 * array_get
 *
 * Return the "real" array from an Array handle
 *
 * input:
 *	Array		*array;		the array handle, from array_create()
 *
 * returns:
 *	Returns the dynamically allocated array associated with
 *	the input handle.
 *
 * algorithm:
 *	Check for valid input, then return the array field of
 *	the input Array handle.
 */

void	*
array_get(Array *array)
{
	if (array)
		return array->array;
	else
		return NULL;
}


/*
 * array_count
 *
 * Return the number of elements currently in a dynamic array.
 *
 * input:
 *	Array		*array;		the array handle, from array_create()
 *
 * returns:
 *	Returns the number of elements that have been successfully
 *	added to a dynamic array.
 *
 * algorithm:
 *	Check for valid input, then return the num field of
 *	the input Array handle.
 */

int
array_count(Array *array)
{
	if (array)
		return array->num;
	else
		return -1;
}


/*
 * Main test driver -- compile with -DMAIN to run test.
 */

#ifdef MAIN

struct test {
	char	character;
	int	integer;
};

int
main(int argc, char *argv[])
{

	int		i;
	char		c;
	int		cnt;
	int		*int_array;
	struct test	test_struct;
	struct test	*test_struct_array;
	struct test	*test_struct_ptr;
	struct test	**test_struct_ptr_array;
	Array		*array;


	array = array_create(sizeof (int), 10);

	for (i = 0; i <= 101; i++) {
		(void) array_add(array, (void *)&i);
	}

	cnt = array_count(array);
	int_array = (int *)array_get(array);
	array_free(array);

	printf("Got array with %d elts\n", cnt);
	for (i = 0; i < cnt; i++) {
		printf("array[%d] is %d\n", i, int_array[i]);
	}

	/* test adding structures */

	array = array_create(sizeof (struct test), 5);

	for (c = 'a'; c <= 'z'; c++) {
		test_struct.character = c;
		test_struct.integer = c;
		(void) array_add(array, (void *)&test_struct);
	}

	cnt = array_count(array);
	test_struct_array = (struct test *)array_get(array);
	array_free(array);

	printf("Got array with %d elts\n", cnt);
	for (i = 0; i < cnt; i++) {
		printf("array[%d].char is %c, .int is %d\n", i,
		       test_struct_array[i].character,
		       test_struct_array[i].integer);
	}

	/* test adding pointers to structures */

	array = array_create(sizeof (struct test *), 50);

	for (c = 'a'; c <= 'z'; c++) {
		test_struct_ptr = (struct test *)malloc(sizeof (struct test));
		test_struct_ptr->character = c;
		test_struct_ptr->integer = c;
		(void) array_add(array, (void *)&test_struct_ptr);
	}

	cnt = array_count(array);
	test_struct_ptr_array = (struct test **)array_get(array);
	array_free(array);

	printf("Got array with %d elts\n", cnt);
	for (i = 0; i < cnt; i++) {
		printf("array[%d]->char is %c, ->int is %d\n", i,
		       test_struct_ptr_array[i]->character,
		       test_struct_ptr_array[i]->integer);
	}

#ifdef lint
	return 0;
#endif /*lint*/
}
#endif /*MAIN*/
