/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009, 2010  Free Software Foundation, Inc.
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

#include <grub/relocator.h>
#include <grub/relocator_private.h>
#include <grub/mm_private.h>
#include <grub/misc.h>
#include <grub/cache.h>

/* FIXME: check memory map.  */ 

struct grub_relocator
{
  struct grub_relocator_chunk *chunks;
  grub_addr_t postchunks;
  grub_addr_t highestaddr;
  grub_addr_t highestnonpostaddr;
  grub_size_t relocators_size;
};

struct grub_relocator_subchunk
{
  enum {CHUNK_TYPE_IN_REGION, CHUNK_TYPE_REGION_START,
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
	CHUNK_TYPE_FIRMWARE
#endif
  } type;
  grub_addr_t host_start;
  grub_addr_t start;
  grub_size_t size;
};

struct grub_relocator_chunk
{
  struct grub_relocator_chunk *next;
  grub_addr_t src;
  grub_addr_t target;
  grub_size_t size;
  struct grub_relocator_subchunk *subchunks;
  unsigned nsubchunks;
};

struct grub_relocator_extra_block
{
  struct grub_relocator_extra_block *next;
  grub_addr_t start;
  grub_addr_t end;
};

struct grub_relocator_extra_block *extra_blocks;

struct grub_relocator *
grub_relocator_new (void)
{
  struct grub_relocator *ret;

  grub_cpu_relocator_init ();

  ret = grub_zalloc (sizeof (struct grub_relocator));
  if (!ret)
    return NULL;
    
  ret->postchunks = ~(grub_addr_t) 0;
  ret->relocators_size = grub_relocator_jumper_size;
  grub_dprintf ("relocator", "relocators_size=%lu\n",
		(unsigned long) ret->relocators_size);
  return ret;
}

#define DIGITSORT_BITS 8
#define DIGITSORT_MASK ((1 << DIGITSORT_BITS) - 1)
#define BITS_IN_BYTE 8

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

static inline int
is_start (int type)
{
  return !(type & 1) && (type != COLLISION_START);
}

static void
allocate_regstart (grub_addr_t addr, grub_size_t size, grub_mm_region_t rb,
		   grub_mm_region_t *regancestor, grub_mm_header_t hancestor)
{
  grub_addr_t newreg_start, newreg_raw_start = addr + size;
  grub_addr_t newreg_size, newreg_presize;
  grub_mm_header_t new_header;
  grub_mm_header_t hb = (grub_mm_header_t) (rb + 1);
  
  grub_dprintf ("relocator", "ra = %p, rb = %p\n", regancestor, rb);

  newreg_start = ALIGN_UP (newreg_raw_start, GRUB_MM_ALIGN);
  newreg_presize = newreg_start - newreg_raw_start;
  newreg_size = rb->size - (newreg_start - (grub_addr_t) rb);
  if ((hb->size << GRUB_MM_ALIGN_LOG2) >= newreg_start
      - (grub_addr_t) rb)
    {
      grub_mm_header_t newhnext = hb->next;
      grub_size_t newhsize = ((hb->size << GRUB_MM_ALIGN_LOG2)
			      - (newreg_start
				 - (grub_addr_t) rb)) >> GRUB_MM_ALIGN_LOG2;
      new_header = (void *) (newreg_start + sizeof (*rb));
      if (newhnext == hb)
	newhnext = new_header;
      new_header->next = newhnext;
      new_header->size = newhsize;
      new_header->magic = GRUB_MM_FREE_MAGIC;
    }
  else
    {
      new_header = hb->next;
      if (new_header == hb)
	new_header = (void *) (newreg_start + sizeof (*rb));	    
    }
  {
    struct grub_mm_header *newregfirst = rb->first;
    struct grub_mm_region *newregnext = rb->next;
    struct grub_mm_region *newreg = (void *) newreg_start;
    hancestor->next = new_header;
    if (newregfirst == hb)
      newregfirst = new_header;
    newreg->first = newregfirst;
    newreg->next = newregnext;
    newreg->pre_size = newreg_presize;
    newreg->size = newreg_size;
    *regancestor = newreg;
    {
      grub_mm_header_t h = newreg->first, hp = NULL;
      do
	{
	  if ((void *) h < (void *) (newreg + 1))
	    grub_fatal ("Failed to adjust memory region: %p, %p, %p, %p, %p",
			newreg, newreg->first, h, hp, hb);
	  if ((void *) h == (void *) (newreg + 1))
	    grub_dprintf ("relocator",
			  "Free start memory region: %p, %p, %p, %p, %p",
			  newreg, newreg->first, h, hp, hb);

	  hp = h;
	  h = h->next;
	}
      while (h != newreg->first);
    }
  }

}

static void
allocate_inreg (grub_addr_t addr, grub_size_t size,
		grub_mm_header_t hb, grub_mm_header_t hbp,
		grub_mm_region_t rb)
{
  struct grub_mm_header *foll = NULL;
    
  if (ALIGN_UP (addr + size, GRUB_MM_ALIGN) + GRUB_MM_ALIGN
      <= (grub_addr_t) (hb + hb->size))
    {
      foll = (void *) ALIGN_UP (addr + size, GRUB_MM_ALIGN);
      foll->magic = GRUB_MM_FREE_MAGIC;
      foll->size = hb->size - (foll - hb);
    }

  if (addr - (grub_addr_t) hb >= sizeof (*hb))
    {
      hb->size = ((addr - (grub_addr_t) hb) >> GRUB_MM_ALIGN_LOG2);
      if (foll)
	{
	  foll->next = hb;
	  hbp->next = foll;
	  if (rb->first == hb)
	    rb->first = foll;
	}
    }
  else
    {
      if (foll)
	foll->next = hb->next;
      else
	foll = hb->next;
	
      hbp->next = foll;
      if (rb->first == hb)
	rb->first = foll;
      if (rb->first == hb)
	rb->first = (void *) (rb + 1);
    }
}

static int
malloc_in_range (struct grub_relocator *rel,
		 grub_addr_t start, grub_addr_t end, grub_addr_t align,
		 grub_size_t size, struct grub_relocator_chunk *res,
		 int from_low_priv, int collisioncheck)
{
  grub_mm_region_t r, *ra, base_saved;
  struct grub_relocator_mmap_event *events = NULL, *eventt = NULL, *t;
  unsigned maxevents = 2;
  grub_mm_header_t p, pa;
  unsigned *counter;
  int nallocs = 0;
  unsigned i, j, N = 0;
  grub_addr_t target = 0;

  grub_dprintf ("relocator",
		"trying to allocate in %x-%x aligned %x size %x\n",
		start, end, align, size);

  start = ALIGN_UP (start, align);
  end = ALIGN_DOWN (end - size, align) + size;
  grub_dprintf ("relocator",
		"trying to allocate in %x-%x aligned %x size %x\n",
		start, end, align, size);

  if (end < start + size)
    return 0;

  /* We have to avoid any allocations when filling scanline events. 
     Hence 2-stages.
   */
  for (r = grub_mm_base; r; r = r->next)
    {
      p = r->first;
      do
	{
	  maxevents += 2;
	  p = p->next;
	}
      while (p != r->first);
      maxevents += 4;
    }
  if (collisioncheck && rel)
    {
      struct grub_relocator_chunk *chunk;
      for (chunk = rel->chunks; chunk; chunk = chunk->next)
	maxevents += 2;
    }

#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
  {
    struct grub_relocator_extra_block *cur;
    for (cur = extra_blocks; cur; cur = cur->next)
      maxevents += 2;
  }

  maxevents += grub_relocator_firmware_get_max_events ();
#endif

  events = grub_malloc (maxevents * sizeof (events[0]));
  eventt = grub_malloc (maxevents * sizeof (events[0]));
  counter = grub_malloc ((DIGITSORT_MASK + 2) * sizeof (counter[0]));
  if (!events || !eventt || !counter)
    {
      grub_dprintf ("relocator", "events or counter allocation failed %d\n",
		    maxevents);
      grub_free (events);
      grub_free (eventt);
      grub_free (counter);
      return 0;
    }

  if (collisioncheck && rel)
    {
      struct grub_relocator_chunk *chunk;
      for (chunk = rel->chunks; chunk; chunk = chunk->next)
	{
	  events[N].type = COLLISION_START;
	  events[N].pos = chunk->target;
	  N++;
	  events[N].type = COLLISION_END;
	  events[N].pos = chunk->target + chunk->size;
	  N++;
	}
    }

#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
  for (r = grub_mm_base; r; r = r->next)
    {
      grub_dprintf ("relocator", "Blocking at 0x%x-0x%x\n",
		    (grub_addr_t) r - r->pre_size, 
		    (grub_addr_t) (r + 1) + r->size);
      events[N].type = FIRMWARE_BLOCK_START;
      events[N].pos = (grub_addr_t) r - r->pre_size;
      N++;
      events[N].type = FIRMWARE_BLOCK_END;
      events[N].pos = (grub_addr_t) (r + 1) + r->size;
      N++;
    }
  {
    struct grub_relocator_extra_block *cur;
    for (cur = extra_blocks; cur; cur = cur->next)
      {
	grub_dprintf ("relocator", "Blocking at 0x%x-0x%x\n",
		      cur->start, cur->end);
	events[N].type = FIRMWARE_BLOCK_START;
	events[N].pos = cur->start;
	N++;
	events[N].type = FIRMWARE_BLOCK_END;
	events[N].pos = cur->end;
	N++;
      }
  }
#endif

  /* No malloc from this point.  */
  base_saved = grub_mm_base;
  grub_mm_base = NULL;

  for (ra = &base_saved, r = *ra; r; ra = &(r->next), r = *ra)
    {
      int pre_added = 0;
      pa = r->first;
      p = pa->next;
      do 
	{	  
	  if (p == (grub_mm_header_t) (r + 1))
	    {
	      pre_added = 1;
	      events[N].type = REG_BEG_START;
	      events[N].pos = (grub_addr_t) r - r->pre_size;
	      events[N].reg = r;
	      events[N].regancestor = ra;
	      events[N].head = p;
	      events[N].hancestor = pa;
	      N++;
	      events[N].type = REG_BEG_END;
	      events[N].pos = (grub_addr_t) (p + p->size) - sizeof (*r);
	      N++;
	    }
	  else
	    {
	      events[N].type = IN_REG_START;
	      events[N].pos = (grub_addr_t) p;
	      events[N].head = p;
	      events[N].hancestor = pa;
	      events[N].reg = r;
	      N++;
	      events[N].type = IN_REG_END;
	      events[N].pos = (grub_addr_t) (p + p->size);
	      N++;
	    }
	  pa = p;
	  p = pa->next;
	}
      while (pa != r->first);
      /* FIXME */
      if (0)//if (!pre_added)
	{
	  events[N].type = REG_BEG_START;
	  events[N].pos = (grub_addr_t) r - r->pre_size;
	  events[N].reg = r;
	  N++;
	  events[N].type = REG_BEG_END;
	  events[N].pos = (grub_addr_t) r;
	  N++;
	}
    }

#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
  N += grub_relocator_firmware_fill_events (events + N);
#endif

  /* Put ending events after starting events.  */
  {
    int st = 0, e = N / 2;
    for (j = 0; j < N; j++)
      if (is_start (events[j].type) || events[j].type == COLLISION_START)
	eventt[st++] = events[j];
      else
	eventt[e++] = events[j];
    t = eventt;
    eventt = events;
    events = t;
  }
  for (i = 0; i < (BITS_IN_BYTE * sizeof (grub_addr_t) / DIGITSORT_BITS);
       i++)
    {
      memset (counter, 0, (1 + (1 << DIGITSORT_BITS)) * sizeof (counter[0]));
      for (j = 0; j < N; j++)
	counter[((events[j].pos >> (DIGITSORT_BITS * i)) 
		 & DIGITSORT_MASK) + 1]++;
      for (j = 0; j <= DIGITSORT_MASK; j++)
	counter[j+1] += counter[j];
      for (j = 0; j < N; j++)
	eventt[counter[((events[j].pos >> (DIGITSORT_BITS * i)) 
			& DIGITSORT_MASK)]++] = events[j];
      t = eventt;
      eventt = events;
      events = t;
    }

#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
 retry:
#endif

  /* Now events are nicely sorted.  */
  {
    int nstarted = 0, ncollisions = 0, nstartedfw = 0, nblockfw = 0;
    grub_addr_t starta = 0;
    int numstarted;
    for (j = from_low_priv ? 0 : N - 1; from_low_priv ? j < N : (j + 1); 
	 from_low_priv ? j++ : j--)
      {
	int isinsidebefore, isinsideafter;
	isinsidebefore = (!ncollisions 
			  && (nstarted || (nstartedfw && !nblockfw)));
	switch (events[j].type)
	  {
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
	  case REG_FIRMWARE_START:
	    nstartedfw++;
	    break;

	  case REG_FIRMWARE_END:
	    nstartedfw--;
	    break;

	  case FIRMWARE_BLOCK_START:
	    nblockfw++;
	    break;

	  case FIRMWARE_BLOCK_END:
	    nblockfw--;
	    break;
#endif

	  case COLLISION_START:
	    ncollisions++;
	    break;

	  case COLLISION_END:
	    ncollisions--;
	    break;

	  case IN_REG_START:
	  case REG_BEG_START:
	    nstarted++;
	    break;

	  case IN_REG_END:
	  case REG_BEG_END:
	    nstarted--;
	    break;
	  }
	isinsideafter = (!ncollisions 
			 && (nstarted || (nstartedfw && !nblockfw)));
	if (!isinsidebefore && isinsideafter)
	  {
	    starta = from_low_priv ? ALIGN_UP (events[j].pos, align)
	      : ALIGN_DOWN (events[j].pos - size, align) + size;
	    numstarted = j;
	  }
	if (isinsidebefore && !isinsideafter && from_low_priv)
	  {
	    target = starta;
	    if (target < start)
	      target = start;
	    if (target + size <= end && target + size <= events[j].pos)
	      /* Found an usable address.  */
	      goto found;
	  }
	if (isinsidebefore && !isinsideafter && !from_low_priv)
	  {
	    target = starta - size;
	    if (target > end - size)
	      target = end - size;
	    if (target >= start && target >= events[j].pos)
	      goto found;
	  }
      }
  }

  grub_mm_base = base_saved;
  grub_free (events);
  grub_free (eventt);
  grub_free (counter);
  return 0;

 found:
  {
    int inreg = 0, regbeg = 0, ncol = 0;
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
    int fwin = 0, fwb = 0;
#endif
    int last_start = 0;
    for (j = 0; j < N; j++)
      {
	int typepre;
	if (ncol)
	  typepre = -1;
	else if (regbeg)
	  typepre = CHUNK_TYPE_REGION_START;
	else if (inreg)
	  typepre = CHUNK_TYPE_IN_REGION;
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
	else if (fwin && !fwb)
	  typepre = CHUNK_TYPE_FIRMWARE;
#endif
	else
	  typepre = -1;

	if (j != 0 && events[j - 1].pos != events[j].pos)
	  {
	    grub_addr_t alloc_start, alloc_end;
	    alloc_start = max (events[j - 1].pos, target);
	    alloc_end = min (events[j].pos, target + size);
	    if (alloc_end > alloc_start)
	      {
		switch (typepre)
		  {
		  case CHUNK_TYPE_REGION_START:
		    allocate_regstart (alloc_start, alloc_end - alloc_start,
				       events[last_start].reg,
				       events[last_start].regancestor,
				       events[last_start].hancestor);
		    break;
		  case CHUNK_TYPE_IN_REGION:
		    allocate_inreg (alloc_start, alloc_end - alloc_start,
				    events[last_start].head,
				    events[last_start].hancestor,
				    events[last_start].reg);
		    break;
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
		  case CHUNK_TYPE_FIRMWARE:
		    /* The failure here can be very expensive.  */
		    if (!grub_relocator_firmware_alloc_region (alloc_start, 
							       alloc_end - alloc_start))
		      {
			grub_dprintf ("relocator",
				      "firmware allocation 0x%x-0x%x failed.\n",
				      alloc_start, alloc_end);
			if (from_low_priv)
			  start = alloc_end;
			else
			  end = alloc_start;
			goto retry;
		      }
		    break;
#endif
		  }
		nallocs++;
	      }
	  }
	  
	switch (events[j].type)
	  {
	  case REG_BEG_START:
	  case IN_REG_START:
	    if (events[j].type == REG_BEG_START &&
		(grub_addr_t) (events[j].reg + 1) > target)
	      regbeg++;
	    else
	      inreg++;
	    last_start = j;
	    break;

	  case REG_BEG_END:
	  case IN_REG_END:
	    inreg = regbeg = 0;
	    break;

#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
	  case REG_FIRMWARE_START:
	    fwin++;
	    break;

	  case REG_FIRMWARE_END:
	    fwin--;
	    break;

	  case FIRMWARE_BLOCK_START:
	    fwb++;
	    break;

	  case FIRMWARE_BLOCK_END:
	    fwb--;
	    break;
#endif
	  case COLLISION_START:
	    ncol++;
	    break;
	  case COLLISION_END:
	    ncol--;
	    break;
	  }

      }
  }

  grub_memset ((void *) target, 0, size);
  grub_dprintf ("relocator", "baseptr = %p\n", &base_saved);
  for (r = base_saved; r; r = r->next)
    {
      p = r->first;
      do
	{
	  if (!p)
	    grub_fatal ("null in the ring %p %p\n", r, p);
	  p = p->next;
	}
      while (p != r->first);
    }

  /* Malloc is available again.  */
  grub_mm_base = base_saved;

  /* FIXME: react on out of memory.  */
  res->subchunks = grub_malloc (sizeof (res->subchunks[0]) * nallocs);
  res->nsubchunks = nallocs;

  {
    int last_start = 0;
    int inreg = 0, regbeg = 0, ncol = 0;
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
    int fwin = 0, fwb = 0;
#endif
    int cural = 0;
    for (j = 0; j < N; j++)
      {
	int typepre;
	if (ncol)
	  typepre = -1;
	else if (regbeg)
	  typepre = CHUNK_TYPE_REGION_START;
	else if (inreg)
	  typepre = CHUNK_TYPE_IN_REGION;
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
	else if (fwin && !fwb)
	  typepre = CHUNK_TYPE_FIRMWARE;
#endif
	else
	  typepre = -1;

	if (j != 0 && events[j - 1].pos != events[j].pos)
	  {
	    grub_addr_t alloc_start, alloc_end;
	    alloc_start = max (events[j - 1].pos, target);
	    alloc_end = min (events[j].pos, target + size);
	    if (alloc_end > alloc_start)
	      {
		grub_dprintf ("relocator", "subchunk 0x%x-0x%x, %d\n",
			      alloc_start, alloc_end, typepre);
		res->subchunks[cural].type = typepre;
		if (typepre == CHUNK_TYPE_REGION_START)
		  {
		    res->subchunks[cural].host_start
		      = (grub_addr_t) events[last_start].reg;
		  }
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
		if (typepre == CHUNK_TYPE_REGION_START
		    || typepre == CHUNK_TYPE_FIRMWARE)
		  {
		    /* FIXME: react on out of memory.  */
		    struct grub_relocator_extra_block *ne;
		    ne = grub_malloc (sizeof (*ne));
		    ne->start = alloc_start;
		    ne->end = alloc_end;
		    ne->next = extra_blocks;
		    extra_blocks = ne;
		  }
#endif
		cural++;
	      }
	  }
	  
	switch (events[j].type)
	  {
	  case REG_BEG_START:
	  case IN_REG_START:
	    if (events[j].type == REG_BEG_START &&
		(grub_addr_t) (events[j].reg + 1) > target)
	      regbeg++;
	    else
	      inreg++;
	    last_start = j;
	    break;

	  case REG_BEG_END:
	  case IN_REG_END:
	    inreg = regbeg = 0;
	    break;

#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
	  case REG_FIRMWARE_START:
	    fwin++;
	    break;

	  case REG_FIRMWARE_END:
	    fwin--;
	    break;

	  case FIRMWARE_BLOCK_START:
	    fwb++;
	    break;

	  case FIRMWARE_BLOCK_END:
	    fwb--;
	    break;
#endif
	  case COLLISION_START:
	    ncol++;
	    break;
	  case COLLISION_END:
	    ncol--;
	    break;
	  }

      }
  }

  res->src = target;
  res->size = size;
  grub_dprintf ("relocator", "allocated: %x %x\n", target, size);
  return 1;
}

static void
adjust_limits (struct grub_relocator *rel, 
	       grub_addr_t *min_addr, grub_addr_t *max_addr,
	       grub_addr_t in_min, grub_addr_t in_max)
{
  struct grub_relocator_chunk *chunk;

  *min_addr = 0;
  *max_addr = rel->postchunks;

  /* Keep chunks in memory in the same order as they'll be after relocation.  */
  for (chunk = rel->chunks; chunk; chunk = chunk->next)
    {
      if (chunk->target > in_max && chunk->src < *max_addr
	  && chunk->src < rel->postchunks)
	*max_addr = chunk->src;
      if (chunk->target + chunk->size <= in_min
	  && chunk->src + chunk->size > *min_addr
	  && chunk->src < rel->postchunks)
	*min_addr = chunk->src + chunk->size;
    }
}

grub_err_t
grub_relocator_alloc_chunk_addr (struct grub_relocator *rel, void **src,
				 grub_addr_t target, grub_size_t size)
{
  struct grub_relocator_chunk *chunk;
  grub_addr_t min_addr = 0, max_addr;

  if (target > ~size)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "address is out of range");

  adjust_limits (rel, &min_addr, &max_addr, target, target);

  for (chunk = rel->chunks; chunk; chunk = chunk->next)
    if ((chunk->target <= target && target < chunk->target + chunk->size)
	|| (target <= chunk->target && chunk->target < target + size))
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "overlap detected");


  chunk = grub_malloc (sizeof (struct grub_relocator_chunk));
  if (!chunk)
    return grub_errno;

  grub_dprintf ("relocator",
		"min_addr = 0x%llx, max_addr = 0x%llx, target = 0x%llx\n",
		(unsigned long long) min_addr, (unsigned long long) max_addr,
		(unsigned long long) target);

  do
    {
      /* A trick to improve Linux allocation.  */
#if defined (__i386__) || defined (__x86_64__)
      if (target < 0x100000)
	if (malloc_in_range (rel, rel->highestnonpostaddr, ~(grub_addr_t)0, 1,
			     size, chunk, 0, 1))
	  {
	    if (rel->postchunks > chunk->src)
	      rel->postchunks = chunk->src;
	    break;
	  }
#endif
      if (malloc_in_range (rel, target, max_addr, 1, size, chunk, 1, 0))
	break;

      if (malloc_in_range (rel, min_addr, target, 1, size, chunk, 0, 0))
	break;

      if (malloc_in_range (rel, rel->highestnonpostaddr, ~(grub_addr_t)0, 1,
			   size, chunk, 0, 1))
	{
	  if (rel->postchunks > chunk->src)
	    rel->postchunks = chunk->src;
	  break;
	}

      grub_dprintf ("relocator", "not allocated\n");
      grub_free (chunk);
      return grub_error (GRUB_ERR_OUT_OF_MEMORY, "out of memory");
    }
  while (0);

  grub_dprintf ("relocator", "allocated 0x%llx/0x%llx\n",
		(unsigned long long) chunk->src, (unsigned long long) target);

  if (rel->highestaddr < target + size)
    rel->highestaddr = target + size;

  if (rel->highestaddr < chunk->src + size)
    rel->highestaddr = chunk->src + size;

  if (chunk->src < rel->postchunks)
    {
      if (rel->highestnonpostaddr < target + size)
	rel->highestnonpostaddr = target + size;
      
      if (rel->highestnonpostaddr < chunk->src + size)
	rel->highestnonpostaddr = chunk->src + size;  
    }

  grub_dprintf ("relocator", "relocators_size=%ld\n",
		(unsigned long) rel->relocators_size);

  if (chunk->src < target)
    rel->relocators_size += grub_relocator_backward_size;
  if (chunk->src > target)
    rel->relocators_size += grub_relocator_forward_size;

  grub_dprintf ("relocator", "relocators_size=%ld\n",
		(unsigned long) rel->relocators_size);

  chunk->target = target;
  chunk->size = size;
  chunk->next = rel->chunks;
  rel->chunks = chunk;
  grub_dprintf ("relocator", "cur = %p, next = %p\n", rel->chunks,
		rel->chunks->next);

  *src = (void *) chunk->src;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_relocator_alloc_chunk_align (struct grub_relocator *rel, void **src,
				  grub_addr_t *target,
				  grub_addr_t min_addr, grub_addr_t max_addr,
				  grub_size_t size, grub_size_t align,
				  int preference)
{
  grub_addr_t min_addr2 = 0, max_addr2;
  struct grub_relocator_chunk *chunk;

  if (max_addr > ~size)
    max_addr = ~size;

#ifdef GRUB_MACHINE_PCBIOS
  if (min_addr < 0x1000)
    min_addr = 0x1000;
#endif

  grub_dprintf ("relocator", "chunks = %p\n", rel->chunks);

  chunk = grub_malloc (sizeof (struct grub_relocator_chunk));
  if (!chunk)
    return grub_errno;

  if (malloc_in_range (rel, min_addr, max_addr, align,
		       size, chunk,
		       preference != GRUB_RELOCATOR_PREFERENCE_HIGH, 1))
    {
      grub_dprintf ("relocator", "allocated 0x%llx/0x%llx\n",
		    (unsigned long long) chunk->src,
		    (unsigned long long) chunk->src);
      grub_dprintf ("relocator", "chunks = %p\n", rel->chunks);
      chunk->target = chunk->src;
      chunk->size = size;
      chunk->next = rel->chunks;
      rel->chunks = chunk;
      *src = (void *) chunk->src;
      *target = chunk->target;
      return GRUB_ERR_NONE;
    }

  adjust_limits (rel, &min_addr2, &max_addr2, min_addr, max_addr);
  grub_dprintf ("relocator", "Adjusted limits from %lx-%lx to %lx-%lx\n",
		(unsigned long) min_addr, (unsigned long) max_addr,
		(unsigned long) min_addr2, (unsigned long) max_addr2);

  do
    {
      if (malloc_in_range (rel, min_addr2, max_addr2, align,
			   size, chunk, 1, 1))
	break;

      if (malloc_in_range (rel, rel->highestnonpostaddr, ~(grub_addr_t)0, 1,
			   size, chunk, 0, 1))
	{
	  if (rel->postchunks > chunk->src)
	    rel->postchunks = chunk->src;
	  break;
	}

      return grub_error (GRUB_ERR_OUT_OF_MEMORY, "out of memory");
    }
  while (0);

  /* FIXME: check memory map.  */
  if (preference == GRUB_RELOCATOR_PREFERENCE_HIGH)
    chunk->target = ALIGN_DOWN (max_addr, align);
  else
    chunk->target = ALIGN_UP (min_addr, align);    
  while (1)
    {
      struct grub_relocator_chunk *chunk2;
      for (chunk2 = rel->chunks; chunk2; chunk2 = chunk2->next)
	if ((chunk2->target <= chunk->target
	     && chunk->target < chunk2->target + chunk2->size)
	    || (chunk2->target <= chunk->target + size
		&& chunk->target + size < chunk2->target + chunk2->size)
	    || (chunk->target <= chunk2->target && chunk2->target
		< chunk->target + size)
	    || (chunk->target <= chunk2->target + chunk2->size
		&& chunk2->target + chunk2->size < chunk->target + size))
	  {
	    if (preference == GRUB_RELOCATOR_PREFERENCE_HIGH)
	      chunk->target = ALIGN_DOWN (chunk2->target, align);
	    else
	      chunk->target = ALIGN_UP (chunk2->target + chunk2->size, align);
	    break;
	  }
      if (!chunk2)
	break;
    }

  if (chunk->src < chunk->target)
    rel->relocators_size += grub_relocator_backward_size;
  if (chunk->src > chunk->target)
    rel->relocators_size += grub_relocator_forward_size;

  chunk->size = size;
  chunk->next = rel->chunks;
  rel->chunks = chunk;
  grub_dprintf ("relocator", "cur = %p, next = %p\n", rel->chunks,
		rel->chunks->next);
  *src = (void *) chunk->src;
  *target = chunk->target;
  return GRUB_ERR_NONE;
}

/* FIXME: remove extra blocks.  */
void
grub_relocator_unload (struct grub_relocator *rel)
{
  struct grub_relocator_chunk *chunk, *next;
  if (!rel)
    return;
  for (chunk = rel->chunks; chunk; chunk = next)
    {
      unsigned i;
      for (i = 0; i < chunk->nsubchunks; i++) 
	switch (chunk->subchunks[i].type)
	  {
	  case CHUNK_TYPE_REGION_START:
	    {
	      grub_mm_region_t r1, r2, *rp;
	      grub_mm_header_t h;
	      grub_size_t pre_size;
	      r1 = (grub_mm_region_t) ALIGN_UP (chunk->subchunks[i].start 
						+ chunk->subchunks[i].size,
						GRUB_MM_ALIGN);
	      r2 = (grub_mm_region_t) ALIGN_UP (chunk->subchunks[i].host_start,
						GRUB_MM_ALIGN);
	      for (rp = &grub_mm_base; *rp && *rp != r2; rp = &((*rp)->next));
	      /* FIXME  */
	      if (!*rp)
		grub_fatal ("Anomaly in region alocations detected. "
			    "Simultaneous relocators?");
	      pre_size = ALIGN_UP (chunk->subchunks[i].host_start, 
				   GRUB_MM_ALIGN)
		- chunk->subchunks[i].host_start;
	      r2->first = r1->first;
	      r2->next = r1->next;
	      r2->pre_size = pre_size;
	      r2->size = r1->size + (r2 - r1) * sizeof (*r2);
	      *rp = r1;
	      h = (grub_mm_header_t) (r1 + 1);
	      h->next = h;
	      h->magic = GRUB_MM_ALLOC_MAGIC;
	      h->size = (r2 - r1);
	      grub_free (h + 1);
	      break;
	    }
	  case CHUNK_TYPE_IN_REGION:
	    {
	      grub_mm_header_t h 
		= (grub_mm_header_t) ALIGN_DOWN (chunk->subchunks[i].start,
						 GRUB_MM_ALIGN);
	      h->size = (chunk->subchunks[i].start / GRUB_MM_ALIGN)
		- ((chunk->subchunks[i].start + chunk->subchunks[i].size 
		    + GRUB_MM_ALIGN - 1) / GRUB_MM_ALIGN);
	      h->next = h;
	      h->magic = GRUB_MM_ALLOC_MAGIC;
	      grub_free (h + 1);
	      break;
	    }
#if GRUB_RELOCATOR_HAVE_FIRMWARE_REQUESTS
	  case CHUNK_TYPE_FIRMWARE:
	    grub_relocator_firmware_free_region (chunk->subchunks[i].start,
						 chunk->subchunks[i].size);
	    break;
#endif
	  }
      next = chunk->next;
      grub_free (chunk->subchunks);
      grub_free (chunk);
    }
}

grub_err_t
grub_relocator_prepare_relocs (struct grub_relocator *rel, grub_addr_t addr,
			       grub_addr_t *relstart, grub_size_t *relsize)
{
  grub_addr_t rels;
  grub_addr_t rels0;
  struct grub_relocator_chunk *sorted;
  grub_size_t nchunks = 0;
  unsigned j;
  struct grub_relocator_chunk movers_chunk;

  grub_dprintf ("relocator", "Preparing relocs (size=%ld)\n",
		(unsigned long) rel->relocators_size);

  if (!malloc_in_range (rel, 0, ~(grub_addr_t)0 - rel->relocators_size + 1,
			grub_relocator_align,
			rel->relocators_size, &movers_chunk, 1, 1))
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "out of memory");
  rels = rels0 = movers_chunk.src;

  if (relsize)
    *relsize = rel->relocators_size;

  grub_dprintf ("relocator", "Relocs allocated\n");
  
  {
    unsigned i;
    grub_size_t count[257];
    struct grub_relocator_chunk *from, *to, *tmp;

    grub_memset (count, 0, sizeof (count));

    {
        struct grub_relocator_chunk *chunk;
	for (chunk = rel->chunks; chunk; chunk = chunk->next)
	  {
	    grub_dprintf ("relocator", "chunk %p->%p, 0x%lx\n", 
			  (void *) chunk->src, (void *) chunk->target,
			  (unsigned long) chunk->size);
	    nchunks++;
	    count[(chunk->src & 0xff) + 1]++;
	  }
    }
    from = grub_malloc (nchunks * sizeof (sorted[0]));
    to = grub_malloc (nchunks * sizeof (sorted[0]));
    if (!from || !to)
      {
	grub_free (from);
	grub_free (to);
	return grub_errno;
      }

    for (j = 0; j < 256; j++)
      count[j+1] += count[j];

    {
      struct grub_relocator_chunk *chunk;
      for (chunk = rel->chunks; chunk; chunk = chunk->next)
	from[count[chunk->src & 0xff]++] = *chunk;
    }

    for (i = 1; i < GRUB_CPU_SIZEOF_VOID_P; i++)
      {
	grub_memset (count, 0, sizeof (count));
	for (j = 0; j < nchunks; j++)
	  count[((from[j].src >> (8 * i)) & 0xff) + 1]++;
	for (j = 0; j < 256; j++)
	  count[j+1] += count[j];
	for (j = 0; j < nchunks; j++)
	  to[count[(from[j].src >> (8 * i)) & 0xff]++] = from[j];
	tmp = to;
	to = from;
	from = tmp;
      }
    sorted = from;
    grub_free (to);
  }

  for (j = 0; j < nchunks; j++)
    {
      grub_dprintf ("relocator", "sorted chunk %p->%p, 0x%lx\n", 
		    (void *) sorted[j].src, (void *) sorted[j].target,
		    (unsigned long) sorted[j].size);
      if (sorted[j].src < sorted[j].target)
	{
	  grub_cpu_relocator_backward ((void *) rels,
				       (void *) sorted[j].src,
				       (void *) sorted[j].target,
				       sorted[j].size);
	  rels += grub_relocator_backward_size;
	}
      if (sorted[j].src > sorted[j].target)
	{
	  grub_cpu_relocator_forward ((void *) rels,
				      (void *) sorted[j].src,
				      (void *) sorted[j].target,
				      sorted[j].size);
	  rels += grub_relocator_forward_size;
	}
      if (sorted[j].src == sorted[j].target)
	grub_arch_sync_caches ((void *) sorted[j].src, sorted[j].size);
    }
  grub_cpu_relocator_jumper ((void *) rels, addr);
  *relstart = rels0;
  grub_free (sorted);
  return GRUB_ERR_NONE;
}
