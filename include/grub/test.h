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

extern grub_test_t EXPORT_VAR (grub_test_list);

void EXPORT_FUNC (grub_test_register) (const char *name, void (*test) (void));

void EXPORT_FUNC (grub_test_unregister) (const char *name);

/* Execute a test and print results.  */
int grub_test_run (grub_test_t test);

/* Test `cond' for nonzero; log failure otherwise.  */
void grub_test_nonzero (int cond, const char *file,
			const char *func, grub_uint32_t line,
			const char *fmt, ...)
  __attribute__ ((format (printf, 5, 6)));

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif
#else
#define __func__ "<unknown>"
#endif

/* Macro to fill in location details and an optional error message.  */
#define grub_test_assert(cond, ...)			\
  grub_test_nonzero(cond, __FILE__, __func__, __LINE__, \
		    ## __VA_ARGS__,			\
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

/* Functions that are defined differently for unit and functional tests.  */
void *grub_test_malloc (grub_size_t size);
void grub_test_free (void *ptr);
char *grub_test_strdup (const char *str);
int grub_test_vsprintf (char *str, const char *fmt, va_list args);
int grub_test_printf (const char *fmt, ...)
  __attribute__ ((format (printf, 1, 2)));

#endif /* ! GRUB_TEST_HEADER */
