/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)keypad.c 1.1	95/12/22 SMI"

/*
 * keypad.c
 *
 * XCurses Library
 *
 * Copyright 1990, 1995 by Mortice Kern Systems Inc.  All rights reserved.
 *
 * This Software is unpublished, valuable, confidential property of
 * Mortice Kern Systems Inc.  Use is authorized only in accordance
 * with the terms and conditions of the source licence agreement
 * protecting this Software.  Any unauthorized use or disclosure of
 * this Software is strictly prohibited and will result in the
 * termination of the licence agreement.
 *
 * If you have any questions, please consult your supervisor.
 */

#ifdef M_RCSID
#ifndef lint
static char rcsID[] = "$Header: /rd/src/libc/xcurses/rcs/keypad.c 1.3 1995/05/24 19:43:46 ant Exp $";
#endif
#endif

#include <private.h>
#include <stdlib.h>

/*f
 * Add a function key string to the decode tree.
 * Return -1 on error, else the length of the key sequence.
 */
static int
decode_add(root, str, code)
t_decode **root;
const char *str;
short code;
{
	const char *start;
	t_decode *node, *saved;

	if (root == (t_decode **) 0)
		return -1;

	if (str == (char *) 0)
		return 0;

	start = str;
	saved = (t_decode *) 0;

	if (*root == (t_decode *) 0) {
		/* First node of tree. */
		node = (t_decode *) malloc(sizeof *node);
		if (node == (t_decode *) 0)
			return -1;

		*root = saved = node;

		node->child = node->sibling = (t_decode *) 0;
		node->ch = *str++;
		node->key = 0;
	} else {
		/* Find node to insert function key sequence into the tree. */
		for (node = *root; *str != '\0'; ++str, node = node->child) {
			while (node->ch != *str 
			&& node->sibling != (t_decode *)0)
				node = node->sibling;
	    
			if (node->ch != *str) {
				node->sibling = (t_decode *) malloc(
					sizeof *node
				);
				if (node->sibling == (t_decode *) 0)
					return -1;

				saved = node = node->sibling;
				node->child = node->sibling = (t_decode *) 0;
				node->ch = *str++;
				node->key = 0;
				break;
			}

			if (node->child == (t_decode *) 0)
				break;
		}
	}

	/* Insert string into the tree; node->child == null. */
	while (*str != '\0') {
		node->child = (t_decode *) malloc(sizeof *node);
		if (node->child == (t_decode *) 0) {
			__m_decode_free(&saved);
			return -1;
		}

		node = node->child;
		node->child = node->sibling = (t_decode *) 0;
		node->ch = *str++;
		node->key = 0;
	}

	node->key = code;

	return (int) (str - start);
}

void
__m_decode_free(tree)
t_decode **tree;
{
	if (*tree != (t_decode *) 0) {
		__m_decode_free(&(*tree)->sibling);
		__m_decode_free(&(*tree)->child);
		free(*tree);
		*tree = (t_decode *) 0;
	}
}

/*f
 * Initialise the function key decode tree.
 */
int
__m_decode_init(tree)
t_decode **tree;
{
	int max, len;
	short (*p)[2];

	*tree = (t_decode *) 0;

	for (max = -1, p = __m_keyindex; **p != -1; ++p) {
		len = decode_add(tree, cur_term->_str[**p], (*p)[1]);
		if (len < 0)
			return -1;
		if (max < len)
			max = len;
	}

	return max;
}

/*f
 * When true for a given window, then multibyte function key processing
 * is done for all input throough that window, see wgetch().
 */
int
keypad(WINDOW *w, bool bf)
{
#ifdef M_CURSES_TRACE
	__m_trace("keypad(%p, %d)", w, bf);
#endif

	w->_flags &= ~W_USE_KEYPAD;
	
	if (bf)
		w->_flags |= W_USE_KEYPAD;

	return __m_return_code("keypad", OK);
}

