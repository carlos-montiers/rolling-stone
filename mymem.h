/*
** Copyright (C) 1999 by Andreas Junghanns.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

void *My_realloc(void *p, int s);
void *My_malloc(int s);
int My_free(void *p);

extern void My_qsort(void *base, int nel, int width,
          int (*compar) (const void *, const void *));

void My_exit(int code, char *format, ... );
