#if defined (_WIN32) || defined (__CYGWIN__)
#include "windows/random.c"
#else
#include "unix/random.c"
#endif
