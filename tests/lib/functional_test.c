#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/test.h>

void *
grub_test_malloc (grub_size_t size)
{
  return grub_malloc (size);
}

void
grub_test_free (void *ptr)
{
  grub_free (ptr);
}

int
grub_test_vsprintf (char *str, const char *fmt, va_list args)
{
  return grub_vsprintf (str, fmt, args);
}

char *
grub_test_strdup (const char *str)
{
  return grub_strdup (str);
}

int
grub_test_printf (const char *fmt, ...)
{
  int r;
  va_list ap;

  va_start (ap, fmt);
  r = grub_vprintf (fmt, ap);
  va_end (ap);

  return r;
}

static grub_err_t
grub_functional_test (struct grub_extcmd *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char **args __attribute__ ((unused)))
{
  auto int run_test (grub_test_t test);
  int run_test (grub_test_t test)
  {
    grub_test_run (test);
    return 0;
  }

  grub_list_iterate (GRUB_AS_LIST (grub_test_list),
		     (grub_list_hook_t) run_test);
  return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT (functional_test)
{
  cmd = grub_register_extcmd ("functional_test", grub_functional_test,
			      GRUB_COMMAND_FLAG_CMDLINE, 0,
			      "Run all functional tests.", 0);
}

GRUB_MOD_FINI (functional_test)
{
  grub_unregister_extcmd (cmd);
}
