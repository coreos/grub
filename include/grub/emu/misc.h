#ifndef GRUB_EMU_MISC_H
#define GRUB_EMU_MISC_H 1

#include <grub/symbol.h>
#include <grub/types.h>

#ifdef __NetBSD__
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/grub"
#else
# define DEFAULT_DIRECTORY	"/boot/grub"
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

extern int verbosity;
extern const char *program_name;

void grub_init_all (void);
void grub_fini_all (void);

void * EXPORT_FUNC(xmalloc) (grub_size_t size);
void * EXPORT_FUNC(xrealloc) (void *ptr, grub_size_t size);
char * EXPORT_FUNC(xstrdup) (const char *str);
char * EXPORT_FUNC(xasprintf) (const char *fmt, ...);

void EXPORT_FUNC(grub_util_warn) (const char *fmt, ...);
void EXPORT_FUNC(grub_util_info) (const char *fmt, ...);
void EXPORT_FUNC(grub_util_error) (const char *fmt, ...) __attribute__ ((noreturn));

#endif /* GRUB_EMU_MISC_H */
