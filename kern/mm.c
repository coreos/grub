/* mm.c - functions for memory manager */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  PUPA is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <pupa/mm.h>
#include <pupa/misc.h>
#include <pupa/err.h>
#include <pupa/types.h>
#include <pupa/disk.h>

/* Magic words.  */
#define PUPA_MM_FREE_MAGIC	0x2d3c2808
#define PUPA_MM_ALLOC_MAGIC	0x6db08fa4

typedef struct pupa_mm_header
{
  struct pupa_mm_header *next;
  pupa_size_t size;
  pupa_size_t magic;
#if PUPA_CPU_SIZEOF_VOID_P == 4
  char padding[4];
#elif PUPA_CPU_SIZEOF_VOID_P == 8
  char padding[8];
#else
# error "unknown word size"
#endif
}
*pupa_mm_header_t;

#if PUPA_CPU_SIZEOF_VOID_P == 4
# define PUPA_MM_ALIGN_LOG2	4
#elif PUPA_CPU_SIZEOF_VOID_P == 8
# define PUPA_MM_ALIGN_LOG2	8
#endif

#define PUPA_MM_ALIGN	(1 << PUPA_MM_ALIGN_LOG2)

typedef struct pupa_mm_region
{
  struct pupa_mm_header *first;
  struct pupa_mm_region *next;
  pupa_addr_t addr;
  pupa_size_t size;
}
*pupa_mm_region_t;



static pupa_mm_region_t base;

/* Get a header from the pointer PTR, and set *P and *R to a pointer
   to the header and a pointer to its region, respectively. PTR must
   be allocated.  */
static void
get_header_from_pointer (void *ptr, pupa_mm_header_t *p, pupa_mm_region_t *r)
{
  if ((unsigned) ptr & (PUPA_MM_ALIGN - 1))
    pupa_fatal ("unaligned pointer %p", ptr);

  for (*r = base; *r; *r = (*r)->next)
    if ((unsigned) ptr > (*r)->addr
	&& (unsigned) ptr <= (*r)->addr + (*r)->size)
      break;

  if (! *r)
    pupa_fatal ("out of range pointer %p", ptr);
  
  *p = (pupa_mm_header_t) ptr - 1;
  if ((*p)->magic != PUPA_MM_ALLOC_MAGIC)
    pupa_fatal ("alloc magic is broken at %p", *p);
}

/* Initialize a region starting from ADDR and whose size is SIZE,
   to use it as free space.  */
void
pupa_mm_init_region (void *addr, pupa_size_t size)
{
  pupa_mm_header_t h;
  pupa_mm_region_t r, *p, q;

  /* If this region is too small, ignore it.  */
  if (size < PUPA_MM_ALIGN * 2)
    return;

  /* Allocate a region from the head.  */
  r = (pupa_mm_region_t) (((pupa_addr_t) addr + PUPA_MM_ALIGN - 1)
			  & (~(PUPA_MM_ALIGN - 1)));
  size -= (char *) r - (char *) addr + sizeof (*r);
  
  h = (pupa_mm_header_t) ((char *) r + PUPA_MM_ALIGN);
  h->next = h;
  h->magic = PUPA_MM_FREE_MAGIC;
  h->size = (size >> PUPA_MM_ALIGN_LOG2);

  r->first = h;
  r->addr = (pupa_addr_t) h;
  r->size = (h->size << PUPA_MM_ALIGN_LOG2);

  /* Find where to insert this region. Put a smaller one before bigger ones,
     to prevent fragmentations.  */
  for (p = &base, q = *p; q; p = &(q->next), q = *p)
    if (q->size > r->size)
      break;
  
  *p = r;
  r->next = q;
}

/* Allocate the number of units N with the alignment ALIGN from the ring
   buffer starting from *FIRST. ALIGN must be a power of two. Return a
   non-NULL if successful, otherwise return NULL.  */
static void *
pupa_real_malloc (pupa_mm_header_t *first, pupa_size_t n, pupa_size_t align)
{
  pupa_mm_header_t p, q;
  
  if ((*first)->magic == PUPA_MM_ALLOC_MAGIC)
    return 0;

  for (q = *first, p = q->next; ; q = p, p = p->next)
    {
      pupa_off_t extra;

      extra = ((pupa_addr_t) (p + 1) >> PUPA_MM_ALIGN_LOG2) % align;
      if (extra)
	extra = align - extra;

      if (! p)
	pupa_fatal ("null in the ring");

      if (p->magic != PUPA_MM_FREE_MAGIC)
	pupa_fatal ("free magic is broken at %p", p);

      if (p->size >= n + extra)
	{
	  if (extra == 0 && p->size == n)
	    {
	      q->next = p->next;
	      p->magic = PUPA_MM_ALLOC_MAGIC;
	    }
	  else if (extra == 0 || p->size == n + extra)
	    {
	      p->size -= n;
	      p += p->size;
	      p->size = n;
	      p->magic = PUPA_MM_ALLOC_MAGIC;
	    }
	  else
	    {
	      pupa_mm_header_t r;

	      r = p + extra + n;
	      r->magic = PUPA_MM_FREE_MAGIC;
	      r->size = p->size - extra - n;
	      r->next = p->next;
	      
	      p->size = extra;
	      p->next = r;
	      p += extra;
	      p->size = n;
	      p->magic = PUPA_MM_ALLOC_MAGIC;
	    }

	  *first = q;
	  return p + 1;
	}

      if (p == *first)
	break;
    }

  return 0;
}

/* Allocate SIZE bytes with the alignment ALIGN and return the pointer.  */
void *
pupa_memalign (pupa_size_t align, pupa_size_t size)
{
  pupa_mm_region_t r;
  pupa_size_t n = ((size + PUPA_MM_ALIGN - 1) >> PUPA_MM_ALIGN_LOG2) + 1;
  int first = 1;
  
  align = (align >> PUPA_MM_ALIGN_LOG2);
  if (align == 0)
    align = 1;

 again:
  
  for (r = base; r; r = r->next)
    {
      void *p;
      
      p = pupa_real_malloc (&(r->first), n, align);
      if (p)
	return p;
    }

  /* If failed, invalidate disk caches to increase free memory.  */
  if (first)
    {
      pupa_disk_cache_invalidate_all ();
      first = 0;
      goto again;
    }

  pupa_error (PUPA_ERR_OUT_OF_MEMORY, "out of memory");
  return 0;
}

/* Allocate SIZE bytes and return the pointer.  */
void *
pupa_malloc (pupa_size_t size)
{
  return pupa_memalign (0, size);
}

/* Deallocate the pointer PTR.  */
void
pupa_free (void *ptr)
{
  pupa_mm_header_t p;
  pupa_mm_region_t r;

  if (! ptr)
    return;

  get_header_from_pointer (ptr, &p, &r);

  if (p == r->first)
    {
      p->magic = PUPA_MM_FREE_MAGIC;
      p->next = p;
    }
  else
    {
      pupa_mm_header_t q;
      
      for (q = r->first; q >= p || q->next <= p; q = q->next)
	{
	  if (q->magic != PUPA_MM_FREE_MAGIC)
	    pupa_fatal ("free magic is broken at %p", q);
	  
	  if (q >= q->next && (q < p || q->next > p))
	    break;
	}

      p->magic = PUPA_MM_FREE_MAGIC;
      p->next = q->next;
      q->next = p;
      
      if (p + p->size == p->next)
	{
	  p->next->magic = 0;
	  p->size += p->next->size;
	  p->next = p->next->next;
	}
      
      if (q + q->size == p)
	{
	  p->magic = 0;
	  q->size += p->size;
	  q->next = p->next;
	}

      r->first = q;
    }
}

/* Reallocate SIZE bytes and return the pointer. The contents will be
   the same as that of PTR.  */
void *
pupa_realloc (void *ptr, pupa_size_t size)
{
  pupa_mm_header_t p;
  pupa_mm_region_t r;
  void *q;
  pupa_size_t n;
  
  if (! ptr)
    return pupa_malloc (size);

  if (! size)
    {
      pupa_free (ptr);
      return 0;
    }

  /* FIXME: Not optimal.  */
  n = ((size + PUPA_MM_ALIGN - 1) >> PUPA_MM_ALIGN_LOG2) + 1;
  get_header_from_pointer (ptr, &p, &r);
  
  if (p->size >= n)
    return p;
  
  q = pupa_malloc (size);
  if (! q)
    return q;
  
  pupa_memcpy (q, ptr, size);
  pupa_free (ptr);
  return q;
}

#if MM_DEBUG
void
pupa_mm_dump (unsigned lineno)
{
  pupa_mm_region_t r;

  pupa_printf ("called at line %u\n", lineno);
  for (r = base; r; r = r->next)
    {
      pupa_mm_header_t p;
      
      for (p = (pupa_mm_header_t) ((r->addr + PUPA_MM_ALIGN - 1)
				   & (~(PUPA_MM_ALIGN - 1)));
	   (pupa_addr_t) p < r->addr + r->size;
	   p++)
	{
	  switch (p->magic)
	    {
	    case PUPA_MM_FREE_MAGIC:
	      pupa_printf ("F:%p:%u:%p\n",
			   p, p->size << PUPA_MM_ALIGN_LOG2, p->next);
	      break;
	    case PUPA_MM_ALLOC_MAGIC:
	      pupa_printf ("A:%p:%u\n", p, p->size << PUPA_MM_ALIGN_LOG2);
	      break;
	    }
	}
    }

  pupa_printf ("\n");
}
#endif /* MM_DEBUG */
