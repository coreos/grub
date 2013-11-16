/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008,2009,2010,2011  Free Software Foundation, Inc.
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

#include <config-util.h>
#include <config.h>

#include <grub/misc.h>
#include <unistd.h>
#include <grub/emu/exec.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>
#include <grub/util/misc.h>
#include <grub/disk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

int
grub_util_exec (const char *const *argv)
{
  pid_t pid;
  int status = -1;
  char *str, *pstr;
  const char *const *ptr;
  grub_size_t strl = 0;
  for (ptr = argv; *ptr; ptr++)
    strl += grub_strlen (*ptr) + 1;
  pstr = str = xmalloc (strl);
  for (ptr = argv; *ptr; ptr++)
    {
      pstr = grub_stpcpy (pstr, *ptr);
      *pstr++ = ' ';
    }
  if (pstr > str)
    pstr--;
  *pstr = '\0';

  grub_util_info ("executing %s", str);
  grub_free (str);

  pid = fork ();
  if (pid < 0)
    grub_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (pid == 0)
    {
      /* Child.  */

      /* Close fd's.  */
#ifdef GRUB_UTIL
      grub_util_devmapper_cleanup ();
      grub_diskfilter_fini ();
#endif

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      execvp ((char *) argv[0], (char **) argv);
      exit (127);
    }

  waitpid (pid, &status, 0);
  if (!WIFEXITED (status))
    return -1;
  return WEXITSTATUS (status);
}

int
grub_util_exec_redirect (const char *const *argv, const char *stdin_file,
			 const char *stdout_file)
{
  pid_t mdadm_pid;
  int status = -1;
  char *str, *pstr;
  const char *const *ptr;
  grub_size_t strl = 0;
  for (ptr = argv; *ptr; ptr++)
    strl += grub_strlen (*ptr) + 1;
  strl += grub_strlen (stdin_file) + 2;
  strl += grub_strlen (stdout_file) + 2;

  pstr = str = xmalloc (strl);
  for (ptr = argv; *ptr; ptr++)
    {
      pstr = grub_stpcpy (pstr, *ptr);
      *pstr++ = ' ';
    }
  *pstr++ = '<';
  pstr = grub_stpcpy (pstr, stdin_file);
  *pstr++ = ' ';
  *pstr++ = '>';
  pstr = grub_stpcpy (pstr, stdout_file);
  *pstr = '\0';

  grub_util_info ("executing %s", str);
  grub_free (str);

  mdadm_pid = fork ();
  if (mdadm_pid < 0)
    grub_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (mdadm_pid == 0)
    {
      int in, out;
      /* Child.  */
      
      /* Close fd's.  */
#ifdef GRUB_UTIL
      grub_util_devmapper_cleanup ();
      grub_diskfilter_fini ();
#endif

      in = open (stdin_file, O_RDONLY);
      if (in < 0)
	exit (127);
      dup2 (in, STDIN_FILENO);
      close (in);

      out = open (stdout_file, O_WRONLY | O_CREAT, 0700);
      dup2 (out, STDOUT_FILENO);
      close (out);

      if (out < 0)
	exit (127);

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      execvp ((char *) argv[0], (char **) argv);
      exit (127);
    }
  waitpid (mdadm_pid, &status, 0);
  if (!WIFEXITED (status))
    return -1;
  return WEXITSTATUS (status);
}

int
grub_util_exec_redirect_null (const char *const *argv)
{
  return grub_util_exec_redirect (argv, "/dev/null", "/dev/null");
}

pid_t
grub_util_exec_pipe (const char *const *argv, int *fd)
{
  int mdadm_pipe[2];
  pid_t mdadm_pid;

  *fd = 0;

  if (pipe (mdadm_pipe) < 0)
    {
      grub_util_warn (_("Unable to create pipe: %s"),
		      strerror (errno));
      return 0;
    }
  mdadm_pid = fork ();
  if (mdadm_pid < 0)
    grub_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (mdadm_pid == 0)
    {
      /* Child.  */

      /* Close fd's.  */
#ifdef GRUB_UTIL
      grub_util_devmapper_cleanup ();
      grub_diskfilter_fini ();
#endif

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      close (mdadm_pipe[0]);
      dup2 (mdadm_pipe[1], STDOUT_FILENO);
      close (mdadm_pipe[1]);

      execvp ((char *) argv[0], (char **) argv);
      exit (127);
    }
  else
    {
      close (mdadm_pipe[1]);
      *fd = mdadm_pipe[0];
      return mdadm_pid;
    }
}

pid_t
grub_util_exec_pipe_stderr (const char *const *argv, int *fd)
{
  int mdadm_pipe[2];
  pid_t mdadm_pid;

  *fd = 0;

  if (pipe (mdadm_pipe) < 0)
    {
      grub_util_warn (_("Unable to create pipe: %s"),
		      strerror (errno));
      return 0;
    }
  mdadm_pid = fork ();
  if (mdadm_pid < 0)
    grub_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (mdadm_pid == 0)
    {
      /* Child.  */

      /* Close fd's.  */
#ifdef GRUB_UTIL
      grub_util_devmapper_cleanup ();
      grub_diskfilter_fini ();
#endif

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      close (mdadm_pipe[0]);
      dup2 (mdadm_pipe[1], STDOUT_FILENO);
      dup2 (mdadm_pipe[1], STDERR_FILENO);
      close (mdadm_pipe[1]);

      execvp ((char *) argv[0], (char **) argv);
      exit (127);
    }
  else
    {
      close (mdadm_pipe[1]);
      *fd = mdadm_pipe[0];
      return mdadm_pid;
    }
}
