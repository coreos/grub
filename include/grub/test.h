#ifndef GRUB_TEST_HEADER
#define GRUB_TEST_HEADER

#include <grub/dl.h>
#include <grub/list.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/symbol.h>

struct grub_test
{
  /* The next test.  */
  struct grub_test *next;

  /* The test name.  */
  char *name;

  /* The test main function.  */
  void (*main) (void);
};
typedef struct grub_test *grub_test_t;

extern grub_test_t grub_test_list;

void grub_test_register   (const char *name, void (*test) (void));
void grub_test_unregister (const char *name);

/* Execute a test and print results.  */
int grub_test_run (grub_test_t test);

/* Test `cond' for nonzero; log failure otherwise.  */
void grub_test_nonzero (int cond, const char *file,
			const char *func, grub_uint32_t line,
			const char *fmt, ...)
  __attribute__ ((format (printf, 5, 6)));

/* Macro to fill in location details and an optional error message.  */
#define grub_test_assert(cond, ...)				\
  grub_test_nonzero(cond, __FILE__, __FUNCTION__, __LINE__,	\
		    ## __VA_ARGS__,				\
		    "assert failed: %s", #cond)

/* Macro to define a unit test.  */
#define GRUB_UNIT_TEST(name, funp)		\
  void grub_unit_test_init (void)		\
  {						\
    grub_test_register (name, funp);		\
  }						\
						\
  void grub_unit_test_fini (void)		\
  {						\
    grub_test_unregister (name);		\
  }

/* Macro to define a functional test.  */
#define GRUB_FUNCTIONAL_TEST(name, funp)	\
  GRUB_MOD_INIT(functional_test_##funp)		\
  {						\
    grub_test_register (name, funp);		\
  }						\
						\
  GRUB_MOD_FINI(functional_test_##funp)		\
  {						\
    grub_test_unregister (name);		\
  }

#endif /* ! GRUB_TEST_HEADER */
