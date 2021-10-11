#pragma ident	"@(#)_Q_mul.c	1.2	92/07/20 SMI" 

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_get_rp_rd(), _Q_set_exception();

#define	FUNC	mul

QUAD 
_Q_mul(x,y)
	QUAD x,y;
{
	unpacked	px,py,pz;
	QUAD		z;
	_fp_current_exceptions = 0;
	_Q_get_rp_rd();
	_fp_unpack(&px,&x,fp_op_extended);
	_fp_unpack(&py,&y,fp_op_extended);
	_fp_mul(&px,&py,&pz);
	_fp_pack(&pz,&z,fp_op_extended);
	_Q_set_exception(_fp_current_exceptions);
	return z;
}
