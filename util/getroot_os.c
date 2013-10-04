#ifdef __linux__
#include "getroot_linux.c"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "getroot_freebsd.c"
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#include "getroot_bsd.c"
#elif defined(__APPLE__)
#include "getroot_apple.c"
#elif defined(__sun__)
#include "getroot_sun.c"
#elif defined(__GNU__)
#include "getroot_hurd.c"
#elif defined(__CYGWIN__) || defined (__MINGW32__)
#include "getroot_windows.c"
#elif defined(__AROS__)
#include "getroot_aros.c"
#else
# warning "No getroot OS-specific functions is available for your system. Device detection may not work properly."
#include "getroot_basic.c"
#endif
