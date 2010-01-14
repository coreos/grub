/* All tests need to include test.h for GRUB testing framework.  */
#include <grub/test.h>

/* Functional test main method.  */
static void
example_test (void)
{
  /* Check if 1st argument is true and report with default error message.  */
  grub_test_assert (1 == 1);

  /* Check if 1st argument is true and report with custom error message.  */
  grub_test_assert (2 == 2, "2 equal 2 expected");
  grub_test_assert (2 == 3, "2 is not equal to %d", 3);
}

/* Register example_test method as a functional test.  */
GRUB_FUNCTIONAL_TEST ("example_functional_test", example_test);
