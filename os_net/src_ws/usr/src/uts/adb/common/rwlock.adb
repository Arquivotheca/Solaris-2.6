#include <sys/machtypes.h>
#include <sys/t_lock.h>
#include <sys/mutex_impl.h>
#include <sys/rwlock_impl.h>

_rwlock_impl
#ifdef i386
./"type"8t"waiters"8t"wr_want"8t"holdcnt"8t"owner"n{type,B}{un.rw.waiters,x}{writewanted,d}{un.rw.holdcnt,d}{owner,X}{END}
#else
./"type"8t"waiters"8t"wr_want"8t"holdcnt"8t"owner"n{type,B}{waiters,x}{un.rw.writewanted,d}{un.rw.holdcnt,d}{owner,X}{END}
#endif
