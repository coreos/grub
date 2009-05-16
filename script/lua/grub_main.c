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

#include <grub/dl.h>
#include <grub/parser.h>

static lua_State *state;

static grub_err_t
grub_lua_parse_line (char *line, grub_reader_getline_t getline)
{
  int r;
  char *old_line = 0;

  lua_settop(state, 0);
  while (1)
    {
      r = luaL_loadbuffer (state, line, grub_strlen (line), "grub");
      if (! r)
	{
	  r = lua_pcall (state, 0, 0, 0);
	  if (r)
	    grub_error (GRUB_ERR_BAD_ARGUMENT, "lua command fails");
	  else
	    {
	      grub_free (old_line);
	      return grub_errno;
	    }
	  break;
	}

      if (r == LUA_ERRSYNTAX)
	{
	  int lmsg;
	  const char *msg = lua_tolstring(state, -1, &lmsg);
	  const char *tp = msg + lmsg - (sizeof(LUA_QL("<eof>")) - 1);
	  if (grub_strstr(msg, LUA_QL("<eof>")) == tp)
	    {
	      char *n, *t;
	      int len;

	      lua_pop(state, 1);
	      if ((getline (&n, 1)) || (! n))
		{
		  grub_error (GRUB_ERR_BAD_ARGUMENT, "incomplete command");
		  break;
		}

	      len = grub_strlen (line);
	      t = grub_malloc (len + grub_strlen (n) + 2);
	      if (! t)
		break;

	      grub_strcpy (t, line);
	      t[len] = '\n';
	      grub_strcpy (t + len + 1, n);
	      grub_free (old_line);
	      line = old_line = t;
	      continue;
	    }
	}

      break;
    }

  grub_free (old_line);
  lua_gc (state, LUA_GCCOLLECT, 0);

  return grub_errno;
}

static struct grub_parser grub_lua_parser =
  {
    .name = "lua",
    .parse_line = grub_lua_parse_line
  };

GRUB_MOD_INIT(lua)
{
  (void) mod;

  state = lua_open ();
  if (state)
    {
      lua_gc(state, LUA_GCSTOP, 0);
      luaL_openlibs(state);
      luaL_register(state, "grub", grub_lua_lib);
      lua_gc(state, LUA_GCRESTART, 0);
      grub_parser_register ("lua", &grub_lua_parser);
    }
}

GRUB_MOD_FINI(lua)
{
  if (state)
    {
      grub_parser_unregister (&grub_lua_parser);
      lua_close(state);
    }
}
