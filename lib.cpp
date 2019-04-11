/****************************************************************
Copyright (C) Lucent Technologies 1997
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name Lucent Technologies or any of
its entities not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
****************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include "awk.h"
#include "ytab.h"
#include <awkerr.h>

FILE*   infile = NULL;
int     recsize = RECSIZE;

Cell**  fldtab;  /* pointers to Cells */
char    inputFS[100] = " ";

#define  MAXFLD  2
int     nfields = MAXFLD;  /* last allocated slot for $i */

int     donefld;  /* 1 = implies rec broken into fields */
int     donerec;  /* 1 = record is valid (no flds have changed) */

int     lastfld = 0;  /* last used field */
extern  Awkfloat *ARGC;

static Cell dollar0 = { OCELL, CFLD, NULL, NULL, 0.0, REC | STR };
static Cell dollar1 = { OCELL, CFLD, NULL, NULL, 0.0, FLD | STR };

static int	refldbld (const char *, const char *);

void recinit ()
{
  recsize = RECSIZE;
  nfields = MAXFLD;
  if ((fldtab = (Cell **)malloc ((nfields + 1) * sizeof (Cell *))) == NULL
   || (fldtab[0] = (Cell *)malloc (sizeof (Cell))) == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space for $0 and fields");
  *fldtab[0] = dollar0;
  fldtab[0]->sval = (char *)calloc (recsize, sizeof(char));
  fldtab[0]->nval = tostring ("0");
  makefields (1, nfields);
}

/// Create $n1..$n2 inclusive
void makefields (int n1, int n2)
{
  char temp[50];
  int i;

  for (i = n1; i <= n2; i++)
  {
    fldtab[i] = (Cell *)malloc (sizeof (struct Cell));
    if (fldtab[i] == NULL)
      FATAL (AWK_ERR_NOMEM, "out of space in makefields %d", i);
    *fldtab[i] = dollar1;
    fldtab[i]->sval = strdup ("");
    sprintf (temp, "%d", i);
    fldtab[i]->nval = tostring (temp);
  }
}

/// Release all allocated fields
void freefields (void)
{
  int i;
  for (i = 0; i < nfields; i++)
    freecell (fldtab[i]);
  free (fldtab[0]);
  xfree (fldtab);
  // 'record' was freed by $0
}

/// Find first filename argument and open the file
void initgetrec (void)
{
  int i;
  const char *p;

  infile = NULL;
  for (i = 1; i < *ARGC; i++)
  {
    p = getargv (i); /* find 1st real filename */
    if (p == NULL || *p == '\0')
    {  /* deleted or zapped */
      interp->argno++;
      continue;
    }
    if (isclvar (p))
      setclvar (p);  /* a command line assignment before filename */
    else
      return; 
    interp->argno++;
  }
  infile = stdin;    /* no filenames, so use stdin */
}

/// Quick and dirty character quoting can quote a few short character strings
char *quote (char* in)
{
  static char buf[256];
  static char *beg = buf;
  char *out, *ret;
  if (beg > buf + sizeof (buf)-50)
    beg = buf;
  out = beg;
  while (*in)
  {
    switch (*in)
    {
    case '\n':
      *out++ = '\\';
      *out++ = 'n';
      break;
    case '\r':
      *out++ = '\\';
      *out++ = 'r';
      break;
    case '\t':
      *out++ = '\\';
      *out++ = 't';
      break;
    default:
      *out++ = *in;
    }
    in++;
  }
  *out++ = 0;
  ret = beg;
  beg = out;
  return ret;
}

/// Get next input record
int getrec (char **pbuf, int *pbufsize, int isrecord)
{
  /* note: cares whether buf == record */
  int c;
  const char* file = 0;

  dprintf ("RS=<%s>, FS=<%s>, ARGC=%g, FILENAME=%s\n",
    quote(*RS), quote(*FS), *ARGC, *FILENAME);
  if (isrecord)
  {
    donefld = 0;
    donerec = 1;
  }
  *pbuf[0] = 0;
  while (interp->argno < *ARGC || infile == stdin)
  {
    dprintf ("argno=%d, file=|%s|\n", interp->argno, NN(file));
    if (infile == NULL)
    {
      /* have to open a new file */
      file = getargv (interp->argno);
      if (file == NULL || *file == '\0')
      {
        /* deleted or zapped */
        interp->argno++;
        continue;
      }
      if (isclvar (file))
      {
        /* a var=value arg */
        setclvar (file);
        interp->argno++;
        continue;
      }
      xfree (*FILENAME);
      *FILENAME = strdup(file);
      dprintf ("opening file %s\n", file);
      if (*file == '-' && *(file + 1) == '\0')
        infile = stdin;
      else if ((infile = fopen (file, "r")) == NULL)
        FATAL (AWK_ERR_INFILE, "can't open file %s", file);
      setfval (fnrloc, 0.0);
    }
    c = readrec (pbuf, pbufsize, infile);
    if (c != 0 || *pbuf[0] != '\0')
    {
      /* normal record */
      if (isrecord)
      {
        fldtab[0]->tval = REC | STR;
        if (is_number (fldtab[0]->sval))
        {
          fldtab[0]->fval = atof (fldtab[0]->sval);
          fldtab[0]->tval |= NUM;
        }
      }
      setfval (nrloc, nrloc->fval + 1);
      setfval (fnrloc, fnrloc->fval + 1);
      return 1;
    }
    /* EOF arrived on this file; set up next */
    if (infile != stdin)
      fclose (infile);
    infile = NULL;
    *FILENAME = 0;
    interp->argno++;
  }
  return 0;  /* true end of file */
}

void nextfile (void)
{
  if (infile != NULL && infile != stdin)
    fclose (infile);
  infile = NULL;
  interp->argno++;
}

/// Get input char from input file or from input redirection function
int awkgetc (FILE *inf)
{
  int c = (inf == interp->files[0].fp && interp->inredir) ? interp->inredir() : getc (inf);
  return c;
}

// write string to output file or send it to output redirection function
int awkputs (const char *str, FILE *fp)
{
  if (fp == interp->files[1].fp && interp->outredir)
    return interp->outredir (str, strlen (str));
  else
    return fputs (str, fp);
}

/// Read one record into buf
int readrec (char **pbuf, int *pbufsize, FILE *inf)
{
  int sep, c;
  char *rr, *buf = *pbuf;
  int bufsize = *pbufsize;

  if (strlen (*FS) >= sizeof (inputFS))
    FATAL (AWK_ERR_LIMIT, "field separator %.10s... is too long", *FS);
  strcpy (inputFS, *FS);  /* for subsequent field splitting */
  if ((sep = **RS) == 0)
  {
    sep = '\n';
    while ((c = awkgetc (inf)) == '\n' && c != EOF)  /* skip leading \n's */
      ;
    if (c != EOF)
      ungetc (c, inf);
  }
  for (rr = buf; ; )
  {
    for (; (c = awkgetc (inf)) != sep && c != EOF; )
    {
      if (rr - buf + 1 > bufsize)
        if (!adjbuf (&buf, &bufsize, 1 + rr - buf, recsize, &rr))
          FATAL (AWK_ERR_NOMEM, "input record `%.30s...' too long", buf);
      *rr++ = c;
    }
    if (**RS == sep || c == EOF)
      break;

    /*
      **RS != sep and sep is '\n' and c == '\n'
      This is the case where RS = 0 and records are separated by two
      consecutive \n
    */
    if ((c = awkgetc (inf)) == '\n' || c == EOF) /* 2 in a row */
      break;
    if (!adjbuf (&buf, &bufsize, 2 + rr - buf, recsize, &rr))
      FATAL (AWK_ERR_NOMEM, "input record `%.30s...' too long", buf);
    *rr++ = '\n';
    *rr++ = c;
  }
#if 0
  //Not needed; buffer has been adjusted inside the loop
  if (!adjbuf (&buf, &bufsize, 1 + rr - buf, recsize, &rr))
    FATAL ("input record `%.30s...' too long", buf);
#endif
  *rr = 0;
  dprintf ("readrec saw <%s>, returns %d\n", buf, c == EOF && rr == buf ? 0 : 1);
  *pbuf = buf;
  *pbufsize = bufsize;
  return c == EOF && rr == buf ? 0 : 1;
}

/// Get ARGV[n]
const char *getargv (int n)
{
  Cell *x;
  const char *s;
  char temp[50];
  extern Array *ARGVtab;

  sprintf (temp, "%d", n);
  if (lookup (temp, ARGVtab) == NULL)
    return NULL;
  x = setsymtab (temp, "", 0.0, STR, ARGVtab);
  s = getsval (x);
  dprintf ("getargv(%d) returns |%s|\n", n, s);
  return s;
}

/*!
  Command line variable.
  Set var=value from s

  Assumes input string has correct format.
*/
void setclvar (const char *s)
{
  const char *p;
  Cell *q;

  for (p = s; *p != '='; p++)
    ;
  p = qstring (p+1, '\0');
  s = qstring (s, '=');
  q = setsymtab (s, p, 0.0, STR, symtab);
  setsval (q, p);
  if (is_number (q->sval))
  {
    q->fval = atof (q->sval);
    q->tval |= NUM;
  }
  dprintf ("command line set %s to |%s|\n", s, p);
}

/// Create fields from current record
void fldbld (void)
{
  char *fields, *fb, *fe, sep;
  Cell *p;
  int i, j, n;

  if (donefld)
    return;

  fields = strdup (getsval (fldtab[0]));
  n = strlen (fields);
  i = 0;  /* number of fields accumulated here */
  strcpy (inputFS, *FS);
  fb = fields;        //beginning of field
  if (strlen (inputFS) > 1)
  {
    /* it's a regular expression */
    i = refldbld (fields, inputFS);
  }
  else if ((sep = *inputFS) == ' ')
  {
    /* default whitespace */
    for (i = 0; ; )
    {
      while (*fb && (*fb == ' ' || *fb == '\t' || *fb == '\n'))
        fb++;
      if (!*fb)
        break;
      fe = fb;        //end of field
      while (*fe && *fe != ' ' && *fe != '\t' && *fe != '\n')
        fe++;
      if (*fe)
        *fe++ = 0;
      if (++i > nfields)
        growfldtab (i);
      xfree (fldtab[i]->sval);
      fldtab[i]->sval = strdup(fb);
      fldtab[i]->tval = FLD | STR;
      fb = fe;
    }
  }
  else if ((sep = *inputFS) == 0)
  {
    /* new: FS="" => 1 char/field */
    for (i = 0; *fb != 0; fb++)
    {
      char buf[2];
      if (++i > nfields)
        growfldtab (i);
      buf[0] = *fb;
      buf[1] = 0;
      xfree (fldtab[i]->sval);
      fldtab[i]->sval = strdup (buf);
      fldtab[i]->tval = FLD | STR;
    }
  }
  else if (*fb != 0)
  {
    /* if 0, it's a null field */

 /* subtlecase : if length(FS) == 1 && length(RS > 0)
  * \n is NOT a field separator (cf awk book 61,84).
  * this variable is tested in the inner while loop.
  */
    int rtest = '\n';  /* normal case */
    int end_seen = 0;
    if (strlen (*RS) > 0)
      rtest = '\0';
    fe = fb;
    for (i = 0; !end_seen; )
    {
      fb = fe;
      while (*fe && *fe != sep && *fe != rtest)
        fe++;
      if (*fe)
        *fe++ = 0;
      else
        end_seen = 1;

      if (++i > nfields)
        growfldtab (i);
      xfree (fldtab[i]->sval);
      fldtab[i]->sval = strdup(fb);
      fldtab[i]->tval = FLD | STR;
    }
  }
  cleanfld (i + 1, lastfld);  /* clean out junk from previous record */
  lastfld = i;
  donefld = 1;
  for (j = 1; j <= lastfld; j++)
  {
    p = fldtab[j];
    if (is_number (p->sval))
    {
      p->fval = atof (p->sval);
      p->tval |= NUM;
    }
  }
  setfval (nfloc, (Awkfloat)lastfld);
  donerec = 1; /* restore */
  free (fields);
#ifndef NDEBUG
  if (dbg)
  {
    for (j = 0; j <= lastfld; j++)
    {
      p = fldtab[j];
      dprintf ("field %d (%s): |%s|\n", j, p->nval, p->sval);
    }
  }
#endif
}

/// Clean out fields n1 .. n2 inclusive
void cleanfld (int n1, int n2)
{
  /* nvals remain intact */
  Cell *p;
  int i;

  for (i = n1; i <= n2; i++)
  {
    p = fldtab[i];
    xfree (p->sval);
    p->tval = FLD | STR;
  }
}

/// Add field n after end of existing lastfld
void newfld (int n)
{
  if (n > nfields)
    growfldtab (n);
  cleanfld (lastfld + 1, n);
  lastfld = n;
  setfval (nfloc, (Awkfloat)n);
}

/// Set lastfld cleaning fldtab cells if necessary
void setlastfld (int n)
{
  if (n > nfields)
    growfldtab (n);

  if (lastfld < n)
    cleanfld (lastfld + 1, n);
  else
    cleanfld (n + 1, lastfld);

  lastfld = n;
}

/// Get nth field
Cell *fieldadr (int n)
{
  if (n < 0)
    FATAL (AWK_ERR_ARG, "trying to access out of range field %d", n);
  if (n > nfields)  /* fields after NF are empty */
    growfldtab (n);  /* but does not increase NF */
  return(fldtab[n]);
}

/// Make new fields up to at least $n
void growfldtab (int n)
{
  int nf = 2 * nfields;
  size_t s;

  if (n > nf)
    nf = n;
  s = (nf + 1) * (sizeof (struct Cell *));  /* freebsd: how much do we need? */
  if (s / sizeof (struct Cell *) - 1 == nf) /* didn't overflow */
    fldtab = (Cell **)realloc (fldtab, s);
  else          /* overflow sizeof int */
    xfree (fldtab);  /* make it null */
  if (fldtab == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space creating %d fields", nf);
  makefields (nfields + 1, nf);
  nfields = nf;
}

/// Build fields from reg expr in FS
int refldbld (const char *rec, const char *fs)
{
  /* this relies on having fields[] the same length as $0 */
  /* the fields are all stored in this one array with \0's */
  char *fr, *fields;
  int i, tempstat, n;
  fa *pfa;

  n = strlen (rec);
  fields = strdup (rec);
  fr = fields;
  *fr = '\0';
  if (*rec == '\0')
    return 0;
  pfa = makedfa (fs, 1);
  dprintf ("into refldbld, rec = <%s>, pat = <%s>\n", rec, fs);
  tempstat = pfa->initstat;
  for (i = 1; ; i++)
  {
    if (i > nfields)
      growfldtab (i);
    xfree (fldtab[i]->sval);
    fldtab[i]->tval = FLD | STR;
    fldtab[i]->sval = strdup(fr);
    dprintf ("refldbld: i=%d\n", i);
    if (nematch (pfa, rec))
    {
      pfa->initstat = 2;  /* horrible coupling to b.c */
      dprintf ("match %s (%d chars)\n", patbeg, patlen);
      strncpy (fr, rec, patbeg - rec);
      fr += patbeg - rec + 1;
      *(fr - 1) = '\0';
      rec = patbeg + patlen;
    }
    else
    {
      dprintf ("no match %s\n", rec);
      strcpy (fr, rec);
      pfa->initstat = tempstat;
      break;
    }
  }
  free (fields);
  return i;
}

/// Create $0 from $1..$NF if necessary
void recbld (void)
{
  int i;
  char *r;
  const char *p;
  char **prec;

  if (donerec == 1)
    return;
  r = fldtab[0]->sval;
  prec = &fldtab[0]->sval;
  for (i = 1; i <= *NF; i++)
  {
    p = getsval (fldtab[i]);
    if (!adjbuf (prec, &recsize, 2 + strlen (p) + strlen (*OFS) + r - *prec, recsize, &r))
      FATAL (AWK_ERR_NOMEM, "created $0 `%.30s...' too long", *prec);
    while ((*r = *p++) != 0)
      r++;
    if (i < *NF)
    {
      for (p = *OFS; (*r = *p++) != 0; )
        r++;
    }
  }
  *r = '\0';
  dprintf ("in recbld inputFS=%s, fldtab[0]=%p\n", inputFS, (void*)fldtab[0]);
  dprintf ("recbld = |%s|\n", *prec);
  donerec = 1;
}

int  errorflag = 0;


double errcheck (double x, const char *s)
{
  if (errno == EDOM)
  {
    errno = 0;
    WARNING ("%s argument out of domain", s);
    x = 1;
  }
  else if (errno == ERANGE)
  {
    errno = 0;
    WARNING ("%s result out of range", s);
    x = 1;
  }
  return x;
}

/*!
  Checks if s is a command line variable assignment 
  of the form var=something
*/
int isclvar (const char *s)
{
  const char *os = s;

  if (!isalpha (*s) && *s != '_')
    return 0;
  for (; *s; s++)
    if (!(isalnum (*s) || *s == '_'))
      break;
  return *s == '=' && s > os && *(s + 1) != '=';
}

/* strtod is supposed to be a proper test of what's a valid number */
/* appears to be broken in gcc on linux: thinks 0x123 is a valid FP number */
/* wrong: violates 4.10.1.4 of ansi C standard */

#include <math.h>
int is_number (const char *s)
{
  double r;
  char *ep;

  if (!s)
    return 0;

  errno = 0;
  r = strtod (s, &ep);
  if (ep == s || r == HUGE_VAL || errno == ERANGE)
    return 0;
  while (*ep == ' ' || *ep == '\t' || *ep == '\n')
    ep++;
  if (*ep == '\0')
    return 1;
  else
    return 0;
}
