#include <config.h>
#include <config-util.h>

#include <sys/times.h>
#include <unistd.h>
#include <grub/emu/misc.h>

grub_uint64_t
grub_util_get_cpu_time_ms (void)
{
  struct tms tm;

  times (&tm); 
  return (tm.tms_utime * 1000ULL) / sysconf(_SC_CLK_TCK);
}
