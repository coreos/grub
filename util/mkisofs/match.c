/*
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include "fnmatch.h"

#include "match.h"

struct pattern
{
  char *str;
  struct pattern *next;
};

static struct pattern *patlist = NULL;
static struct pattern *i_patlist = NULL;	/* ISO9660/RR */
static struct pattern *j_patlist = NULL;	/* Joliet */

#define DECL_ADD_MATCH(function, list) \
void \
function (char *pattern) \
{ \
  struct pattern *new; \
  new = malloc (sizeof (*new)); \
  new->str = strdup (pattern); \
  new->next = list; \
  list = new; \
}

DECL_ADD_MATCH (add_match, patlist)
DECL_ADD_MATCH (i_add_match, i_patlist)
DECL_ADD_MATCH (j_add_match, j_patlist)

#define DECL_MATCHES(function, list) \
int \
function (char *str) \
{ \
  struct pattern *i; \
  for (i = list; i != NULL; i = i->next) \
    if (fnmatch (i->str, str, FNM_FILE_NAME) != FNM_NOMATCH) \
      return 1; \
  return 0; \
}

DECL_MATCHES (matches, patlist)
DECL_MATCHES (i_matches, i_patlist)
DECL_MATCHES (j_matches, j_patlist)

int
i_ishidden()
{
  return (i_patlist != NULL);
}


int j_ishidden()
{
  return (j_patlist != NULL);
}
