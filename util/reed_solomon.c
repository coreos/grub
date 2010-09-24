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
typedef unsigned int grub_size_t;
typedef unsigned char grub_uint8_t;
typedef unsigned short grub_uint16_t;
#define xmalloc malloc
#define grub_memset memset
#define grub_memcpy memcpy
#else
#include <grub/types.h>
#include <grub/reed_solomon.h>
#include <grub/util/misc.h>
#include <grub/misc.h>
#endif

#define GF_SIZE 8
typedef grub_uint8_t gf_single_t;
typedef grub_uint16_t gf_double_t;
const gf_single_t gf_polynomial = 0x1d;

#define SECTOR_SIZE 512
#define MAX_BLOCK_SIZE (200 * SECTOR_SIZE)

static gf_single_t
gf_reduce (gf_double_t a)
{
  int i;
  for (i = GF_SIZE - 1; i >= 0; i--)
    if (a & (1ULL << (i + GF_SIZE)))
      a ^= (((gf_double_t) gf_polynomial) << i);
  return a & ((1ULL << GF_SIZE) - 1);
}

static gf_single_t
gf_mul (gf_single_t a, gf_single_t b)
{
  gf_double_t res = 0;
  int i;
  for (i = 0; i < GF_SIZE; i++)
    if (b & (1 << i))
      res ^= ((gf_double_t) a) << i;
  return gf_reduce (res);
}

static int
bin_log2 (gf_double_t a)
{
  int i = 0;
  while (a)
    {
      a >>= 1;
      i++;
    }
  return i - 1;
}

static gf_single_t
gf_invert (gf_single_t a)
{
  /* We start with: */
  /* 1 * a + 0 * p = a */
  /* 0 * a + 1 * p = p */
  gf_double_t x1 = 1, y1 = 0, z1 = a, x2 = 0, y2 = 1;
  gf_double_t z2 = gf_polynomial | (1ULL << GF_SIZE), t;
  /* invariant: z1 < z2*/
  while (z1 != 0)
    {
      int k;
      k = bin_log2 (z2) - bin_log2 (z1);
      x2 ^= (x1 << k);
      y2 ^= (y1 << k);
      z2 ^= (z1 << k);

      if (z1 >= z2)
	{
	  t = x2;
	  x2 = x1;
	  x1 = t;
	  t = y2;
	  y2 = y1;
	  y1 = t;
	  t = z2;
	  z2 = z1;
	  z1 = t;
	}
    }

  return gf_reduce (x2);
}

static gf_single_t
pol_evaluate (gf_single_t *pol, grub_size_t degree, gf_single_t x)
{
  int i;
  gf_single_t xn = 1, s = 0;
  for (i = degree; i >= 0; i--)
    {
      s ^= gf_mul (pol[i], xn);
      xn = gf_mul (x, xn);
    }
  return s;
}

static void
rs_encode (gf_single_t *data, grub_size_t s, grub_size_t rs)
{
  gf_single_t *rs_polynomial, a = 1;
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
      if (a & (1 << (GF_SIZE - 1)))
	{
	  a <<= 1;
	  a ^= gf_polynomial;
	}
      else
	a <<= 1;
      for (i = 0; i < rs; i++)
	rs_polynomial[i] = rs_polynomial[i + 1] ^ gf_mul (a, rs_polynomial[i]);
      rs_polynomial[rs] = gf_mul (a, rs_polynomial[rs]);
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

static void
syndroms (gf_single_t *m, grub_size_t s, grub_size_t rs,
	  gf_single_t *sy)
{
  gf_single_t xn = 1;
  int i;
  for (i = 0; i < rs; i++)
    {
      if (xn & (1 << (GF_SIZE - 1)))
	{
	  xn <<= 1;
	  xn ^= gf_polynomial;
	}
      else
	xn <<= 1;
      sy[i] = pol_evaluate (m, s + rs - 1, xn);
    }
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

  sigma = xmalloc (rs2 * sizeof (gf_single_t));
  errpot = xmalloc (rs2 * sizeof (gf_single_t));
  errpos = xmalloc (rs2 * sizeof (int));
  sy = xmalloc (rs * sizeof (gf_single_t));

  syndroms (m, s, rs, sy);

  {
    gf_single_t *eq;
    int *chosen;

    eq = xmalloc (rs2 * (rs2 + 1) * sizeof (gf_single_t));
    chosen = xmalloc (rs2 * sizeof (int));

    for (i = 0; i < rs; i++)
      if (sy[i] != 0)
	break;

    /* No error detected.  */
    if (i == rs)
      return;

    for (i = 0; i < rs2; i++)
      for (j = 0; j < rs2 + 1; j++)
	eq[i * (rs2 + 1) + j] = sy[i+j];

    grub_memset (sigma, 0, rs2 * sizeof (gf_single_t));
    grub_memset (chosen, -1, rs2 * sizeof (int));

    for (i = 0 ; i < rs2; i++)
      {
	int nzidx;
	int k;
	gf_single_t r;
	for (nzidx = 0; nzidx < rs2 && (eq[i * (rs2 + 1) + nzidx] == 0);
	     nzidx++);
	if (nzidx == rs2)
	  {
	    break;
	  }
	chosen[i] = nzidx;
	r = gf_invert (eq[i * (rs2 + 1) + nzidx]);
	for (j = 0; j < rs2 + 1; j++)
	  eq[i * (rs2 + 1) + j] = gf_mul (eq[i * (rs2 + 1) + j], r);
	for (j = i + 1; j < rs2; j++)
	  {
	    gf_single_t rr = eq[j * (rs2 + 1) + nzidx];
	    for (k = 0; k < rs2 + 1; k++)
	      eq[j * (rs2 + 1) + k] ^= gf_mul (eq[i * (rs2 + 1) + k], rr);;
	  }
      }
    for (i = rs2 - 1; i >= 0; i--)
      {
	gf_single_t s = 0;
	if (chosen[i] == -1)
	  continue;
	for (j = 0; j < rs2; j++)
	  s ^= gf_mul (eq[i * (rs2 + 1) + j], sigma[j]);
	s ^= eq[i * (rs2 + 1) + rs2];
	sigma[chosen[i]] = s;
      }

    free (chosen);
    free (eq);
  } 

  {
    gf_single_t xn = 1, xx = gf_invert (2), yn = 1;
    int lp = 0;
    for (i = 0; i < rs + s; i++)
      {
	gf_single_t ev = (gf_mul (pol_evaluate (sigma, rs2 - 1, xn), xn) ^ 1);
	if (ev == 0)
	  {
	    errpot[errnum] = yn;
	    errpos[errnum++] = s + rs - i - 1;
	  }
	yn = gf_mul (yn, 2);
	xn = gf_mul (xn, xx);
      }
  }
  {
    gf_single_t *eq;
    int *chosen;
    gf_single_t *errvals;

    eq = xmalloc (rs * (errnum + 1) * sizeof (gf_single_t));
    chosen = xmalloc (rs * sizeof (int));
    errvals = xmalloc (errnum * sizeof (int));

    grub_memset (chosen, -1, rs * sizeof (int));
    grub_memset (errvals, 0, errnum * sizeof (gf_single_t));

    for (j = 0; j < errnum; j++)
      eq[j] = errpot[j];
    eq[errnum] = sy[0];
    for (i = 1; i < rs; i++)
      {
	for (j = 0; j < errnum; j++)
	  eq[(errnum + 1) * i + j] = gf_mul (errpot[j],
					     eq[(errnum + 1) * (i - 1) + j]);
	eq[(errnum + 1) * i + errnum] = sy[i];
      }
    for (i = 0 ; i < rs; i++)
      {
	int nzidx;
	int k;
	gf_single_t r;
	for (nzidx = 0; nzidx < errnum && (eq[i * (errnum + 1) + nzidx] == 0);
	     nzidx++);
	if (nzidx == errnum)
	  continue;
	chosen[i] = nzidx;
	r = gf_invert (eq[i * (errnum + 1) + nzidx]);
	for (j = 0; j < errnum + 1; j++)
	  eq[i * (errnum + 1) + j] = gf_mul (eq[i * (errnum + 1) + j], r);
	for (j = i + 1; j < rs; j++)
	  {
	    gf_single_t rr = eq[j * (errnum + 1) + nzidx];
	    for (k = 0; k < errnum + 1; k++)
	      eq[j * (errnum + 1) + k] ^= gf_mul (eq[i * (errnum + 1) + k], rr);
	  }
      }
    for (i = rs - 1; i >= 0; i--)
      {
	gf_single_t s = 0;
	if (chosen[i] == -1)
	  continue;
	for (j = 0; j < errnum; j++)
	  s ^= gf_mul (eq[i * (errnum + 1) + j], errvals[j]);
	s ^= eq[i * (errnum + 1) + errnum];
	errvals[chosen[i]] = s;
      }
    for (i = 0; i < errnum; i++)
      m[errpos[i]] ^= errvals[i];
  }
  free (sy);
}

static void
decode_block (gf_single_t *ptr, grub_size_t s,
	      gf_single_t *rptr, grub_size_t rs)
{
  grub_size_t ss;
  int i, j, k;
  for (i = 0; i < SECTOR_SIZE; i++)
    {
      grub_size_t ds = (s + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      grub_size_t rr = (rs + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      gf_single_t m[ds + rr];

      for (j = 0; j < ds; j++)
	m[j] = ptr[SECTOR_SIZE * j + i];
      for (j = 0; j < rr; j++)
	m[j + ds] = rptr[SECTOR_SIZE * j + i];

      rs_recover (m, ds, rr);

      for (j = 0; j < ds; j++)
	ptr[SECTOR_SIZE * j + i] = m[j];
    }
}

static void
encode_block (gf_single_t *ptr, grub_size_t s,
	      gf_single_t *rptr, grub_size_t rs)
{
  grub_size_t ss;
  int i, j;
  for (i = 0; i < SECTOR_SIZE; i++)
    {
      grub_size_t ds = (s + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      grub_size_t rr = (rs + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      gf_single_t m[ds + rr];
      for (j = 0; j < ds; j++)
	m[j] = ptr[SECTOR_SIZE * j + i];
      rs_encode (m, ds, rr);
      for (j = 0; j < rr; j++)      
	rptr[SECTOR_SIZE * j + i] = m[j + ds];
    }
}

void
grub_reed_solomon_add_redundancy (void *buffer, grub_size_t data_size,
				  grub_size_t redundancy)
{
  grub_size_t s = data_size;
  grub_size_t rs = redundancy;
  gf_single_t *ptr = buffer;
  gf_single_t *rptr = ptr + s;

  while (s > 0)
    {
      grub_size_t tt;
      grub_size_t cs, crs;
      cs = s;
      crs = rs;
      tt = cs + crs;
      if (tt > MAX_BLOCK_SIZE)
	{
	  cs = (cs * MAX_BLOCK_SIZE) / tt;
	  crs = (crs * MAX_BLOCK_SIZE) / tt;
	}
      encode_block (ptr, cs, rptr, crs);
      ptr += cs;
      rptr += crs;
      s -= cs;
      rs -= crs;
    }
}

void
grub_reed_solomon_recover (void *ptr, grub_size_t s, grub_size_t rs)
{
  gf_single_t *rptr = ptr + s;
  while (s > 0)
    {
      grub_size_t tt;
      grub_size_t cs, crs;
      cs = s;
      crs = rs;
      tt = cs + crs;
      if (tt > MAX_BLOCK_SIZE)
	{
	  cs = cs * MAX_BLOCK_SIZE / tt;
	  crs = crs * MAX_BLOCK_SIZE / tt;
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
  in = fopen ("tst.bin", "rb");
  if (!in)
    return 1;
  fseek (in, 0, SEEK_END);
  s = ftell (in);
  fseek (in, 0, SEEK_SET);
  rs = 1024 * ((s + MAX_BLOCK_SIZE - 1) / (MAX_BLOCK_SIZE - 1024));
  buf = xmalloc (s + rs + SECTOR_SIZE);
  fread (buf, 1, s, in);

  grub_reed_solomon_add_redundancy (buf, s, rs);

  out = fopen ("tst_rs.bin", "wb");
  fwrite (buf, 1, s + rs, out);
  fclose (out);

  grub_memset (buf + 512 * 15, 0, 512);

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
