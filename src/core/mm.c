#include "../core.h"

u64 allocB; // currently allocated number of bytes

#if MM==0
  #include "../opt/mm_malloc.c"
#elif MM==1
  #include "../opt/mm_buddy.c"
#elif MM==2
  #include "../opt/mm_2buddy.c"
#else
  #error bad MM value
#endif
