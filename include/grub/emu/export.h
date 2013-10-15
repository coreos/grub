#ifdef GRUB_SYMBOL_GENERATOR
void EXPORT_FUNC (sysconf) (void);
void EXPORT_FUNC (times) (void);
#else
#include <sys/times.h>
#include <unistd.h>
#endif
