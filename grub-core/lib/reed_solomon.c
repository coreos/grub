/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifdef TEST
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define xmalloc malloc
#define grub_memset memset
#define grub_memcpy memcpy
#endif

#ifndef STANDALONE
#include <assert.h>
#endif

#ifndef STANDALONE
#ifdef TEST
typedef unsigned int grub_size_t;
typedef unsigned char grub_uint8_t;
#else
#include <grub/types.h>
#include <grub/reed_solomon.h>
#include <grub/util/misc.h>
#include <grub/misc.h>
#endif
#endif

#ifdef STANDALONE
#ifdef TEST
typedef unsigned int grub_size_t;
typedef unsigned char grub_uint8_t;
#else
#include <grub/types.h>
#include <grub/misc.h>
#endif
void
grub_reed_solomon_recover (void *ptr_, grub_size_t s, grub_size_t rs);
#endif

#define GF_SIZE 8
typedef grub_uint8_t gf_single_t;
#define GF_POLYNOMIAL 0x1d
#define GF_INVERT2 0x8e
#if defined (STANDALONE) && !defined (TEST)
static gf_single_t * const gf_powx __attribute__ ((section(".text"))) = (void *) 0x100000;
static gf_single_t * const gf_powx_inv __attribute__ ((section(".text"))) = (void *) 0x100200;

static char *scratch __attribute__ ((section(".text"))) = (void *) 0x100300;
#else
#if defined (STANDALONE)
static char *scratch;
#endif
static gf_single_t gf_powx[255 * 2];
static gf_single_t gf_powx_inv[256];
#endif

#define SECTOR_SIZE 512
#define MAX_BLOCK_SIZE (200 * SECTOR_SIZE)

static gf_single_t
gf_mul (gf_single_t a, gf_single_t b)
{
  if (a == 0 || b == 0)
    return 0;
  return gf_powx[(int) gf_powx_inv[a] + (int) gf_powx_inv[b]];
}

static inline gf_single_t
gf_invert (gf_single_t a)
{
  return gf_powx[255 - (int) gf_powx_inv[a]];
}

static void
init_powx (void)
{
  int i;
  grub_uint8_t cur = 1;

  gf_powx_inv[0] = 0;
  for (i = 0; i < 255; i++)
    {
      gf_powx[i] = cur;
      gf_powx[i + 255] = cur;
      gf_powx_inv[cur] = i;
      if (cur & (1ULL << (GF_SIZE - 1)))
	cur = (cur << 1) ^ GF_POLYNOMIAL;
      else
	cur <<= 1;
    }
}

static gf_single_t
pol_evaluate (gf_single_t *pol, grub_size_t degree, gf_single_t x)
{
  int i;
  gf_single_t s = 0;
  int log_xn = 0, log_x;
  if (x == 0)
    return pol[0];
  log_x = gf_powx_inv[x];
  for (i = degree; i >= 0; i--)
    {
      if (pol[i])
	s ^= gf_powx[(int) gf_powx_inv[pol[i]] + log_xn];
      log_xn += log_x;
      if (log_xn >= ((1 << GF_SIZE) - 1))
	log_xn -= ((1 << GF_SIZE) - 1);
    }
  return s;
}

#if !defined (STANDALONE)
static void
rs_encode (gf_single_t *data, grub_size_t s, grub_size_t rs)
{
  gf_single_t *rs_polynomial;
  int i, j;
  gf_single_t *m;
  m = xmalloc ((s + rs) * sizeof (gf_single_t));
  grub_memcpy (m, data, s * sizeof (gf_single_t));
  grub_memset (m + s, 0, rs * sizeof (gf_single_t));
  rs_polynomial = xmalloc ((rs + 1) * sizeof (gf_single_t));
  grub_memset (rs_polynomial, 0, (rs + 1) * sizeof (gf_single_t));
  rs_polynomial[rs] = 1;
  /* Multiply with X - a^r */
  for (j = 0; j < rs; j++)
    {
      for (i = 0; i < rs; i++)
	if (rs_polynomial[i])
	  rs_polynomial[i] = (rs_polynomial[i + 1]
			      ^ gf_powx[j + (int) gf_powx_inv[rs_polynomial[i]]]);
	else
	  rs_polynomial[i] = rs_polynomial[i + 1];
      if (rs_polynomial[rs])
	rs_polynomial[rs] = gf_powx[j + (int) gf_powx_inv[rs_polynomial[rs]]];
    }
  for (j = 0; j < s; j++)
    if (m[j])
      {
	gf_single_t f = m[j];
	for (i = 0; i <= rs; i++)
	  m[i+j] ^= gf_mul (rs_polynomial[i], f);
      }
  free (rs_polynomial);
  grub_memcpy (data + s, m + s, rs * sizeof (gf_single_t));
  free (m);
}
#endif

static void
syndroms (gf_single_t *m, grub_size_t s, grub_size_t rs,
	  gf_single_t *sy)
{
  gf_single_t xn = 1;
  unsigned i;
  sy[0] = pol_evaluate (m, s + rs - 1, xn);
  for (i = 1; i < rs; i++)
    {
      if (xn & (1 << (GF_SIZE - 1)))
	{
	  xn <<= 1;
	  xn ^= GF_POLYNOMIAL;
	}
      else
	xn <<= 1;
      sy[i] = pol_evaluate (m, s + rs - 1, xn);
    }
}

static void
gauss_eliminate (gf_single_t *eq, int n, int m, int *chosen)
{
  int i, j;

  for (i = 0 ; i < n; i++)
    {
      int nzidx;
      int k;
      gf_single_t r;
      for (nzidx = 0; nzidx < m && (eq[i * (m + 1) + nzidx] == 0);
	   nzidx++);
      if (nzidx == m)
	continue;
      chosen[i] = nzidx;
      r = gf_invert (eq[i * (m + 1) + nzidx]);
      for (j = 0; j < m + 1; j++)
	eq[i * (m + 1) + j] = gf_mul (eq[i * (m + 1) + j], r);
      for (j = i + 1; j < n; j++)
	{
	  gf_single_t rr = eq[j * (m + 1) + nzidx];
	  for (k = 0; k < m + 1; k++)
	    eq[j * (m + 1) + k] ^= gf_mul (eq[i * (m + 1) + k], rr);
	}
    }
}

static void
gauss_solve (gf_single_t *eq, int n, int m, gf_single_t *sol)
{
  int *chosen;
  int i, j;

#ifndef STANDALONE
  chosen = xmalloc (n * sizeof (int));
#else
  chosen = (void *) scratch;
  scratch += n * sizeof (int);
#endif
  for (i = 0; i < n; i++)
    chosen[i] = -1;
  for (i = 0; i < m; i++)
    sol[i] = 0;
  gauss_eliminate (eq, n, m, chosen);
  for (i = n - 1; i >= 0; i--)
    {
      gf_single_t s = 0;
      if (chosen[i] == -1)
	continue;
      for (j = 0; j < m; j++)
	s ^= gf_mul (eq[i * (m + 1) + j], sol[j]);
      s ^= eq[i * (m + 1) + m];
      sol[chosen[i]] = s;
    }
#ifndef STANDALONE
  free (chosen);
#else
  scratch -= n * sizeof (int);
#endif
}

static void
rs_recover (gf_single_t *m, grub_size_t s, grub_size_t rs)
{
  grub_size_t rs2 = rs / 2;
  gf_single_t *sigma;
  gf_single_t *errpot;
  int *errpos;
  gf_single_t *sy;
  int errnum = 0;
  int i, j;

#ifndef STANDALONE
  sigma = xmalloc (rs2 * sizeof (gf_single_t));
  errpot = xmalloc (rs2 * sizeof (gf_single_t));
  errpos = xmalloc (rs2 * sizeof (int));
  sy = xmalloc (rs * sizeof (gf_single_t));
#else
  sigma = (void *) scratch;
  scratch += rs2 * sizeof (gf_single_t);
  errpot = (void *) scratch;
  scratch += rs2 * sizeof (gf_single_t);
  errpos = (void *) scratch;
  scratch += rs2 * sizeof (int);
  sy = (void *) scratch;
  scratch += rs * sizeof (gf_single_t);
#endif

  syndroms (m, s, rs, sy);

  for (i = 0; i < (int) rs; i++)
    if (sy[i] != 0)
      break;

  /* No error detected.  */
  if (i == (int) rs)
    {
#ifndef STANDALONE
      free (sigma);
      free (errpot);
      free (errpos);
      free (sy);
#else
      scratch -= rs2 * sizeof (gf_single_t);
      scratch -= rs2 * sizeof (gf_single_t);
      scratch -= rs2 * sizeof (int);
      scratch -= rs * sizeof (gf_single_t);
#endif
      return;
    }

  {
    gf_single_t *eq;

#ifndef STANDALONE
    eq = xmalloc (rs2 * (rs2 + 1) * sizeof (gf_single_t));
#else
    eq = (void *) scratch;
    scratch += rs2 * (rs2 + 1) * sizeof (gf_single_t);
#endif

    for (i = 0; i < (int) rs2; i++)
      for (j = 0; j < (int) rs2 + 1; j++)
	eq[i * (rs2 + 1) + j] = sy[i+j];

    for (i = 0; i < (int) rs2; i++)
      sigma[i] = 0;

    gauss_solve (eq, rs2, rs2, sigma);

#ifndef STANDALONE
    free (eq);
#else
    scratch -= rs2 * (rs2 + 1) * sizeof (gf_single_t);
#endif
  } 

  {
    gf_single_t xn = 1, yn = 1;
    for (i = 0; i < (int) (rs + s); i++)
      {
	gf_single_t ev = (gf_mul (pol_evaluate (sigma, rs2 - 1, xn), xn) ^ 1);
	if (ev == 0)
	  {
	    errpot[errnum] = yn;
	    errpos[errnum++] = s + rs - i - 1;
	  }
	yn = gf_mul (yn, 2);
	xn = gf_mul (xn, GF_INVERT2);
      }
  }
  {
    gf_single_t *errvals;
    gf_single_t *eq;

#ifndef STANDALONE
    eq = xmalloc (rs * (errnum + 1) * sizeof (gf_single_t));
    errvals = xmalloc (errnum * sizeof (int));
#else
    eq = (void *) scratch;
    scratch += rs * (errnum + 1) * sizeof (gf_single_t);
    errvals = (void *) scratch;
    scratch += errnum * sizeof (int);
#endif

    for (j = 0; j < errnum; j++)
      eq[j] = 1;
    eq[errnum] = sy[0];
    for (i = 1; i < (int) rs; i++)
      {
	for (j = 0; j < (int) errnum; j++)
	  eq[(errnum + 1) * i + j] = gf_mul (errpot[j],
					     eq[(errnum + 1) * (i - 1) + j]);
	eq[(errnum + 1) * i + errnum] = sy[i];
      }

    gauss_solve (eq, rs, errnum, errvals);

    for (i = 0; i < (int) errnum; i++)
      m[errpos[i]] ^= errvals[i];
#ifndef STANDALONE
    free (eq);
    free (errvals);
#else
    scratch -= rs * (errnum + 1) * sizeof (gf_single_t);
    scratch -= errnum * sizeof (int);
#endif
  }
#ifndef STANDALONE
  free (sigma);
  free (errpot);
  free (errpos);
  free (sy);
#else
  scratch -= rs2 * sizeof (gf_single_t);
  scratch -= rs2 * sizeof (gf_single_t);
  scratch -= rs2 * sizeof (int);
  scratch -= rs * sizeof (gf_single_t);
#endif
}

static void
decode_block (gf_single_t *ptr, grub_size_t s,
	      gf_single_t *rptr, grub_size_t rs)
{
  int i, j;
  for (i = 0; i < SECTOR_SIZE; i++)
    {
      grub_size_t ds = (s + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      grub_size_t rr = (rs + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      gf_single_t *m;

      /* Nothing to do.  */
      if (!ds || !rr)
	continue;

#ifndef STANDALONE
      m = xmalloc (ds + rr);
#else
      m = (gf_single_t *) scratch;
      scratch += ds + rr;
#endif

      for (j = 0; j < (int) ds; j++)
	m[j] = ptr[SECTOR_SIZE * j + i];
      for (j = 0; j < (int) rr; j++)
	m[j + ds] = rptr[SECTOR_SIZE * j + i];

      rs_recover (m, ds, rr);

      for (j = 0; j < (int) ds; j++)
	ptr[SECTOR_SIZE * j + i] = m[j];

#ifndef STANDALONE
      free (m);
#else
      scratch -= ds + rr;
#endif
    }
}

#if !defined (STANDALONE)
static void
encode_block (gf_single_t *ptr, grub_size_t s,
	      gf_single_t *rptr, grub_size_t rs)
{
  int i, j;
  for (i = 0; i < SECTOR_SIZE; i++)
    {
      grub_size_t ds = (s + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      grub_size_t rr = (rs + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      gf_single_t *m;

      if (!ds || !rr)
	continue;

      m = xmalloc (ds + rr);
      for (j = 0; j < ds; j++)
	m[j] = ptr[SECTOR_SIZE * j + i];
      rs_encode (m, ds, rr);
      for (j = 0; j < rr; j++)      
	rptr[SECTOR_SIZE * j + i] = m[j + ds];
      free (m);
    }
}
#endif

#if !defined (STANDALONE)
void
grub_reed_solomon_add_redundancy (void *buffer, grub_size_t data_size,
				  grub_size_t redundancy)
{
  grub_size_t s = data_size;
  grub_size_t rs = redundancy;
  gf_single_t *ptr = buffer;
  gf_single_t *rptr = ptr + s;
  void *tmp;

  tmp = xmalloc (data_size);
  grub_memcpy (tmp, buffer, data_size);

  /* Nothing to do.  */
  if (!rs)
    return;

  init_powx ();

  while (s > 0)
    {
      grub_size_t tt;
      grub_size_t cs, crs;
      cs = s;
      crs = rs;
      tt = cs + crs;
      if (tt > MAX_BLOCK_SIZE)
	{
	  cs = ((cs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	  crs = ((crs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	}
      encode_block (ptr, cs, rptr, crs);
      ptr += cs;
      rptr += crs;
      s -= cs;
      rs -= crs;
    }

  assert (grub_memcmp (tmp, buffer, data_size) == 0);
  free (tmp);
}
#endif

void
grub_reed_solomon_recover (void *ptr_, grub_size_t s, grub_size_t rs)
{
  gf_single_t *ptr = ptr_;
  gf_single_t *rptr = ptr + s;

  /* Nothing to do.  */
  if (!rs)
    return;

  init_powx ();

  while (s > 0)
    {
      grub_size_t tt;
      grub_size_t cs, crs;
      cs = s;
      crs = rs;
      tt = cs + crs;
      if (tt > MAX_BLOCK_SIZE)
	{
	  cs = ((cs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	  crs = ((crs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	}
      decode_block (ptr, cs, rptr, crs);
      ptr += cs;
      rptr += crs;
      s -= cs;
      rs -= crs;
    }
}

#ifdef TEST
int
main (int argc, char **argv)
{
  FILE *in, *out;
  grub_size_t s, rs;
  char *buf;

  grub_memset (gf_powx, 0xee, sizeof (gf_powx));
  grub_memset (gf_powx_inv, 0xdd, sizeof (gf_powx_inv));

#ifdef STANDALONE
  scratch = xmalloc (1048576);
#endif

#ifndef STANDALONE
  init_powx ();
#endif

  in = fopen ("tst.bin", "rb");
  if (!in)
    return 1;
  fseek (in, 0, SEEK_END);
  s = ftell (in);
  fseek (in, 0, SEEK_SET);
  rs = 0x7007;
  buf = xmalloc (s + rs + SECTOR_SIZE);
  fread (buf, 1, s, in);
  fclose (in);

  grub_reed_solomon_add_redundancy (buf, s, rs);

  out = fopen ("tst_rs.bin", "wb");
  fwrite (buf, 1, s + rs, out);
  fclose (out);
#if 1
  grub_memset (buf + 512 * 15, 0, 512);
#endif

  out = fopen ("tst_dam.bin", "wb");
  fwrite (buf, 1, s + rs, out);
  fclose (out);
  grub_reed_solomon_recover (buf, s, rs);

  out = fopen ("tst_rec.bin", "wb");
  fwrite (buf, 1, s, out);
  fclose (out);

  return 0;
}
#endif
