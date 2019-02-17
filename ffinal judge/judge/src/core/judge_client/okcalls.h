
#include <sys/syscall.h>
#define HOJ_MAX_LIMIT -1
#ifdef __i386
   #include "okcalls32.h"
#else
   #include "okcalls64.h"
#endif
