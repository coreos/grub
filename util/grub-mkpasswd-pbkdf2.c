/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1992-1999,2001,2003,2004,2005,2009 Free Software Foundation, Inc.
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

#include <config.h>

#include <grub/types.h>
#include <grub/crypto.h>
#include <grub/auth.h>
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/i18n.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE	1

#include <argp.h>

#include "progname.h"

static struct argp_option options[] = {
  {"iteration-count",  'c', N_("NUM"), 0, N_("Number of PBKDF2 iterations"), 0},
  {"buflen",  'l', N_("NUM"), 0, N_("Length of generated hash"), 0},
  {"salt",  's', N_("NUM"), 0, N_("Length of salt"), 0},
  { 0, 0, 0, 0, 0, 0 }
};

struct arguments
{
  unsigned int count;
  unsigned int buflen;
  unsigned int saltlen;
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  char *p;

  switch (key)
    {
    case 'c':
      arguments->count = strtoul (arg, NULL, 0);
      break;

    case 'l':
      arguments->buflen = strtoul (arg, NULL, 0);
      break;

    case 's':
      arguments->saltlen = strtoul (arg, NULL, 0);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[OPTIONS]"),
  N_("Generate PBKDF2 password hash."),
  NULL, NULL, NULL
};


static void
hexify (char *hex, grub_uint8_t *bin, grub_size_t n)
{
  while (n--)
    {
      if (((*bin & 0xf0) >> 4) < 10)
	*hex = ((*bin & 0xf0) >> 4) + '0';
      else
	*hex = ((*bin & 0xf0) >> 4) + 'A' - 10;
      hex++;

      if ((*bin & 0xf) < 10)
	*hex = (*bin & 0xf) + '0';
      else
	*hex = (*bin & 0xf) + 'A' - 10;
      hex++;
      bin++;
    }
  *hex = 0;
}

int
main (int argc, char *argv[])
{
  struct arguments arguments = {
    .count = 10000,
    .buflen = 64,
    .saltlen = 64
  };
  char *bufhex, *salthex;
  gcry_err_code_t gcry_err;
  grub_uint8_t *buf, *salt;
  ssize_t nr;
  char pass1[GRUB_AUTH_MAX_PASSLEN];
  char pass2[GRUB_AUTH_MAX_PASSLEN];

  set_program_name (argv[0]);

  grub_util_init_nls ();

  /* Check for options.  */
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  bufhex = malloc (arguments.buflen * 2 + 1);
  if (!bufhex)
    grub_util_error (_("out of memory"));
  buf = malloc (arguments.buflen);
  if (!buf)
    {
      free (bufhex);
      grub_util_error (_("out of memory"));
    }

  salt = malloc (arguments.saltlen);
  if (!salt)
    {
      free (bufhex);
      free (buf);
      grub_util_error (_("out of memory"));
    }
  salthex = malloc (arguments.saltlen * 2 + 1);
  if (!salthex)
    {
      free (salt);
      free (bufhex);
      free (buf);
      grub_util_error (_("out of memory"));
    }
  
  printf ("%s", _("Enter password: "));
  if (!grub_password_get (pass1, GRUB_AUTH_MAX_PASSLEN))
    {
      free (buf);
      free (bufhex);
      free (salthex);
      free (salt);
      grub_util_error (_("failure to read password"));
    }
  printf ("\n%s", _("Reenter password: "));
  if (!grub_password_get (pass2, GRUB_AUTH_MAX_PASSLEN))
    {
      free (buf);
      free (bufhex);
      free (salthex);
      free (salt);
      grub_util_error (_("failure to read password"));
    }

  if (strcmp (pass1, pass2) != 0)
    {
      memset (pass1, 0, sizeof (pass1));
      memset (pass2, 0, sizeof (pass2));
      free (buf);
      free (bufhex);
      free (salthex);
      free (salt);
      grub_util_error (_("passwords don't match"));
    }
  memset (pass2, 0, sizeof (pass2));

#if ! defined (__linux__) && ! defined (__FreeBSD__)
  printf ("%s", _("WARNING: your random generator isn't known to be secure\n"));
#endif

  {
    FILE *f;
    size_t rd;
    f = fopen ("/dev/urandom", "rb");
    if (!f)
      {
	memset (pass1, 0, sizeof (pass1));
	free (buf);
	free (bufhex);
	free (salthex);
	free (salt);
	fclose (f);
	grub_util_error (_("couldn't retrieve random data for salt"));
      }
    rd = fread (salt, 1, arguments.saltlen, f);
    if (rd != arguments.saltlen)
      {
	fclose (f);
	memset (pass1, 0, sizeof (pass1));
	free (buf);
	free (bufhex);
	free (salthex);
	free (salt);
	grub_util_error (_("couldn't retrieve random data for salt"));
      }
    fclose (f);
  }

  gcry_err = grub_crypto_pbkdf2 (GRUB_MD_SHA512,
				 (grub_uint8_t *) pass1, strlen (pass1),
				 salt, arguments.saltlen,
				 arguments.count, buf, arguments.buflen);
  memset (pass1, 0, sizeof (pass1));

  if (gcry_err)
    {
      memset (buf, 0, arguments.buflen);
      memset (bufhex, 0, 2 * arguments.buflen);
      free (buf);
      free (bufhex);
      memset (salt, 0, arguments.saltlen);
      memset (salthex, 0, 2 * arguments.saltlen);
      free (salt);
      free (salthex);
      grub_util_error (_("cryptographic error number %d"), gcry_err);
    }

  hexify (bufhex, buf, arguments.buflen);
  hexify (salthex, salt, arguments.saltlen);

  printf (_("Your PBKDF2 is grub.pbkdf2.sha512.%d.%s.%s\n"),
	  arguments.count, salthex, bufhex);
  memset (buf, 0, arguments.buflen);
  memset (bufhex, 0, 2 * arguments.buflen);
  free (buf);
  free (bufhex);
  memset (salt, 0, arguments.saltlen);
  memset (salthex, 0, 2 * arguments.saltlen);
  free (salt);
  free (salthex);

  return 0;
}
