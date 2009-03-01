/* list.h - header for grub list */
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

#ifndef GRUB_LIST_HEADER
#define GRUB_LIST_HEADER 1

#include <grub/symbol.h>
#include <grub/types.h>

struct grub_list
{
  struct grub_list *next;
};
typedef struct grub_list *grub_list_t;

typedef int (*grub_list_hook_t) (grub_list_t item);

void EXPORT_FUNC(grub_list_push) (grub_list_t *head, grub_list_t item);
void * EXPORT_FUNC(grub_list_pop) (grub_list_t *head);
void EXPORT_FUNC(grub_list_remove) (grub_list_t *head, grub_list_t item);
void EXPORT_FUNC(grub_list_iterate) (grub_list_t head, grub_list_hook_t hook);

/* This function doesn't exist, so if assertion is false for some reason, the
   linker would fail.  */
extern void* grub_assert_fail (void);

#define GRUB_FIELD_MATCH(ptr, type, field) \
  ((char *) &(ptr)->field == (char *) &((type) (ptr))->field)

#define GRUB_AS_LIST(ptr) \
  (GRUB_FIELD_MATCH (ptr, grub_list_t, next) ? \
   (grub_list_t) ptr : grub_assert_fail ())

#define GRUB_AS_LIST_P(pptr) \
  (GRUB_FIELD_MATCH (*pptr, grub_list_t, next) ? \
   (grub_list_t *) (void *) pptr : grub_assert_fail ())

struct grub_named_list
{
  struct grub_named_list *next;
  const char *name;
};
typedef struct grub_named_list *grub_named_list_t;

void * EXPORT_FUNC(grub_named_list_find) (grub_named_list_t head,
					  const char *name);

#define GRUB_AS_NAMED_LIST(ptr) \
  ((GRUB_FIELD_MATCH (ptr, grub_named_list_t, next) && \
    GRUB_FIELD_MATCH (ptr, grub_named_list_t, name))? \
   (grub_named_list_t) ptr : grub_assert_fail ())

#endif /* ! GRUB_LIST_HEADER */
