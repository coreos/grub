/*
 * 27-Mar-96: Jan-Piet Mens <jpm@mens.de>
 * added 'match' option (-m) to specify regular expressions NOT to be included
 * in the CD image.
 */

static char rcsid[] ="$Id: match.c,v 1.3 1999/03/02 03:41:25 eric Exp $";

#include "config.h"
#include <prototyp.h>
#include <stdio.h>
#ifndef VMS
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#endif
#include <string.h>
#include "match.h"

#define MAXMATCH 1000
static char *mat[MAXMATCH];

void add_match(fn)
char * fn;
{
  register int i;

  for (i=0; mat[i] && i<MAXMATCH; i++);
  if (i == MAXMATCH) {
    fprintf(stderr,"Can't exclude RE '%s' - too many entries in table\n",fn);
    return;
  }

 
  mat[i] = (char *) malloc(strlen(fn)+1);
  if (! mat[i]) {
    fprintf(stderr,"Can't allocate memory for excluded filename\n");
    return;
  }

  strcpy(mat[i],fn);
}

int matches(fn)
char * fn;
{
  /* very dumb search method ... */
  register int i;

  for (i=0; mat[i] && i<MAXMATCH; i++) {
    if (fnmatch(mat[i], fn, FNM_FILE_NAME) != FNM_NOMATCH) {
      return 1; /* found -> excluded filenmae */
    }
  }
  return 0; /* not found -> not excluded */
}

/* ISO9660/RR hide */

static char *i_mat[MAXMATCH];

void i_add_match(fn)
char * fn;
{
  register int i;

  for (i=0; i_mat[i] && i<MAXMATCH; i++);
  if (i == MAXMATCH) {
    fprintf(stderr,"Can't exclude RE '%s' - too many entries in table\n",fn);
    return;
  }

 
  i_mat[i] = (char *) malloc(strlen(fn)+1);
  if (! i_mat[i]) {
    fprintf(stderr,"Can't allocate memory for excluded filename\n");
    return;
  }

  strcpy(i_mat[i],fn);
}

int i_matches(fn)
char * fn;
{
  /* very dumb search method ... */
  register int i;

  for (i=0; i_mat[i] && i<MAXMATCH; i++) {
    if (fnmatch(i_mat[i], fn, FNM_FILE_NAME) != FNM_NOMATCH) {
      return 1; /* found -> excluded filenmae */
    }
  }
  return 0; /* not found -> not excluded */
}

int i_ishidden()
{
  return((int)i_mat[0]);
}

/* Joliet hide */

static char *j_mat[MAXMATCH];

void j_add_match(fn)
char * fn;
{
  register int i;

  for (i=0; j_mat[i] && i<MAXMATCH; i++);
  if (i == MAXMATCH) {
    fprintf(stderr,"Can't exclude RE '%s' - too many entries in table\n",fn);
    return;
  }

 
  j_mat[i] = (char *) malloc(strlen(fn)+1);
  if (! j_mat[i]) {
    fprintf(stderr,"Can't allocate memory for excluded filename\n");
    return;
  }

  strcpy(j_mat[i],fn);
}

int j_matches(fn)
char * fn;
{
  /* very dumb search method ... */
  register int i;

  for (i=0; j_mat[i] && i<MAXMATCH; i++) {
    if (fnmatch(j_mat[i], fn, FNM_FILE_NAME) != FNM_NOMATCH) {
      return 1; /* found -> excluded filenmae */
    }
  }
  return 0; /* not found -> not excluded */
}

int j_ishidden()
{
  return((int)j_mat[0]);
}

