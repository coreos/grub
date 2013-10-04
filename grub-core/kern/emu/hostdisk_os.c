#ifdef __linux__
#include "hostdisk_linux.c"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "hostdisk_freebsd.c"
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#include "hostdisk_bsd.c"
#elif defined(__APPLE__)
#include "hostdisk_apple.c"
#elif defined(__sun__)
#include "hostdisk_sun.c"
#elif defined(__GNU__)
#include "hostdisk_hurd.c"
#elif defined(__CYGWIN__) || defined(__MINGW32__)
#include "hostdisk_windows.c"
#elif defined(__AROS__)
#include "hostdisk_aros.c"
#else
# warning "No hostdisk OS-specific functions is available for your system. Device detection may not work properly."
#include "hostdisk_basic.c"
#endif
