/* regexp.c -- The regexp command.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/env.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <regex.h>

static grub_err_t
grub_cmd_regexp (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  int argn = 0;
  regex_t regex;
  int ret;
  grub_size_t s;
  char *comperr;
  grub_err_t err;
  regmatch_t *matches = 0;

  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "2 arguments expected");

  ret = regcomp (&regex, args[0], REG_EXTENDED);
  if (ret)
    goto fail;

  matches = grub_zalloc (sizeof (*matches) * (regex.re_nsub + 1));
  if (! matches)
    goto fail;

  ret = regexec (&regex, args[1], regex.re_nsub + 1, matches, 0);
  if (!ret)
    {
      int i;
      char ch;
      char buf[5 + sizeof (size_t) * 3];

      for (i = 0; i <= regex.re_nsub && matches[i].rm_so != -1; i++)
	{
	  ch = args[1][matches[i].rm_eo];
	  args[1][matches[i].rm_eo] = '\0';

	  grub_snprintf (buf, sizeof (buf), "%s%u", "match", i);
	  if (grub_env_set (buf, args[1] + matches[i].rm_so))
	    break;

	  args[1][matches[i].rm_eo] = ch;
	}

      regfree (&regex);
      grub_free (matches);
      return GRUB_ERR_NONE;
    }

 fail:
  grub_free (matches);
  s = regerror (ret, &regex, 0, 0);
  comperr = grub_malloc (s);
  if (!comperr)
    {
      regfree (&regex);
      return grub_errno;
    }
  regerror (ret, &regex, comperr, s);
  err = grub_error (GRUB_ERR_TEST_FAILURE, "%s", comperr);
  regfree (&regex);
  grub_free (comperr);
  return err;
}

static grub_command_t cmd;

GRUB_MOD_INIT(regexp)
{
  cmd = grub_register_command ("regexp", grub_cmd_regexp,
			       N_("REGEXP STRING"),
			       N_("Test if REGEXP matches STRING."));
}

GRUB_MOD_FINI(regexp)
{
  grub_unregister_command (cmd);
}
