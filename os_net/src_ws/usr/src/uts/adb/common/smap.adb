#include <sys/param.h>
#include <sys/types.h>
#include <sys/t_lock.h>
#include <vm/seg.h>
#include <vm/seg_map.h>

smap
./"vnode"16t"off hi"16t"off lo"16t"bitmap"8t"refcnt"n{sm_vp,X}{sm_off,XX}{sm_bitmap,x}{sm_refcnt,x}
+/"hash"16t"next"16t"prev"n{sm_hash,X}{sm_next,X}{sm_prev,X}
