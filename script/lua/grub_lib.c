/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "grub_lib.h"

#include <grub/env.h>
#include <grub/parser.h>
#include <grub/command.h>

static int
grub_lua_run (lua_State *state)
{
  int n;
  char **args;
  grub_err_t result;

  if (! lua_gettop(state))
    return 0;

  if ((! grub_parser_split_cmdline (lua_tostring (state, 1), 0, &n, &args))
      && (n >= 0))
    {
      grub_command_t cmd;

      cmd = grub_command_find (args[0]);
      if (cmd)
	(cmd->func) (cmd, n, &args[1]);
      else
	grub_error (GRUB_ERR_FILE_NOT_FOUND, "command not found");

      grub_free (args[0]);
      grub_free (args);
    }

  result = grub_errno;
  grub_errno = 0;

  lua_pushinteger(state, result);

  if (result)
    lua_pushstring (state, grub_errmsg);

  return (result) ? 2 : 1;
}

static int
grub_lua_getenv (lua_State *state)
{
  int n, i;

  n = lua_gettop(state);
  for (i = 1; i <= n; i++)
    {
      const char *name, *value;

      name = lua_tostring (state, i);
      value = grub_env_get (name);
      if (value)
	lua_pushstring (state, value);
      else
	lua_pushnil (state);
    }

  return n;
}

static int
grub_lua_setenv (lua_State *state)
{
  const char *name, *value;

  if (lua_gettop(state) != 2)
    return 0;

  name = lua_tostring (state, 1);
  value = lua_tostring (state, 2);

  if (name[0])
    grub_env_set (name, value);

  return 0;
}

luaL_Reg grub_lua_lib[] =
  {
    {"run", grub_lua_run},
    {"getenv", grub_lua_getenv},
    {"setenv", grub_lua_setenv},
    {0, 0}
  };
