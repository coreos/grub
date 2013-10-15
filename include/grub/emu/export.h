#ifdef GRUB_SYMBOL_GENERATOR
void EXPORT_FUNC (open64) (void);
void EXPORT_FUNC (close) (void);
void EXPORT_FUNC (read) (void);
void EXPORT_FUNC (write) (void);
void EXPORT_FUNC (ioctl) (void);
void EXPORT_FUNC (__errno_location) (void);
void EXPORT_FUNC (strerror) (void);
void EXPORT_FUNC (sysconf) (void);
void EXPORT_FUNC (times) (void);
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif
