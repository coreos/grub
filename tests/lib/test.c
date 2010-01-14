#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/test.h>

struct grub_test_failure
{
  /* The next failure.  */
  struct grub_test_failure *next;

  /* The test source file name.  */
  char *file;

  /* The test function name.  */
  char *funp;

  /* The test call line number.  */
  grub_uint32_t line;

  /* The test failure message.  */
  char *message;
};
typedef struct grub_test_failure *grub_test_failure_t;

grub_test_t grub_test_list;
static grub_test_failure_t failure_list;

static void
add_failure (const char *file,
	     const char *funp,
	     grub_uint32_t line, const char *fmt, va_list args)
{
  char buf[1024];
  grub_test_failure_t failure;

  failure = (grub_test_failure_t) grub_malloc (sizeof (*failure));
  if (!failure)
    return;

  grub_vsprintf (buf, fmt, args);

  failure->file = grub_strdup (file ? : "<unknown_file>");
  failure->funp = grub_strdup (funp ? : "<unknown_function>");
  failure->line = line;
  failure->message = grub_strdup (buf);

  grub_list_push (GRUB_AS_LIST_P (&failure_list), GRUB_AS_LIST (failure));
}

static void
free_failures (void)
{
  grub_test_failure_t item;

  while ((item = grub_list_pop (GRUB_AS_LIST_P (&failure_list))) != 0)
    {
      if (item->message)
	grub_free (item->message);

      if (item->funp)
	grub_free (item->funp);

      if (item->file)
	grub_free (item->file);

      grub_free (item);
    }
  failure_list = 0;
}

void
grub_test_nonzero (int cond,
		   const char *file,
		   const char *funp, grub_uint32_t line, const char *fmt, ...)
{
  va_list ap;

  if (cond)
    return;

  va_start (ap, fmt);
  add_failure (file, funp, line, fmt, ap);
  va_end (ap);
}

void
grub_test_register (const char *name, void (*test_main) (void))
{
  grub_test_t test;

  test = (grub_test_t) grub_malloc (sizeof (*test));
  if (!test)
    return;

  test->name = grub_strdup (name);
  test->main = test_main;

  grub_list_push (GRUB_AS_LIST_P (&grub_test_list), GRUB_AS_LIST (test));
}

void
grub_test_unregister (const char *name)
{
  grub_test_t test;

  test = (grub_test_t) grub_named_list_find
    (GRUB_AS_NAMED_LIST (grub_test_list), name);

  if (test)
    {
      grub_list_remove (GRUB_AS_LIST_P (&grub_test_list), GRUB_AS_LIST (test));

      if (test->name)
	grub_free (test->name);

      grub_free (test);
    }
}

int
grub_test_run (grub_test_t test)
{
  auto int print_failure (grub_test_failure_t item);
  int print_failure (grub_test_failure_t item)
  {
    grub_test_failure_t failure = (grub_test_failure_t) item;

    grub_printf (" %s:%s:%u: %s\n",
		 (failure->file ? : "<unknown_file>"),
		 (failure->funp ? : "<unknown_function>"),
		 failure->line, (failure->message ? : "<no message>"));
    return 0;
  }

  test->main ();

  grub_printf ("%s:\n", test->name);
  grub_list_iterate (GRUB_AS_LIST (failure_list),
		     (grub_list_hook_t) print_failure);
  if (!failure_list)
    grub_printf ("%s: PASS\n", test->name);
  else
    grub_printf ("%s: FAIL\n", test->name);

  free_failures ();
  return GRUB_ERR_NONE;
}
