#if defined (_WIN32) || defined (__CYGWIN__)
#include "random_windows.c"
#else
#include "random_unix.c"
#endif
