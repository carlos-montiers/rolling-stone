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

#define MAXHIST 500

typedef int H_int;

typedef struct {
	long  total_sum;
	H_int total_count;
	int   max_index;
	H_int max_count;
	H_int count[MAXHIST];
} HISTOGRAM;

void  InitHist(HISTOGRAM *hist);
void  ResetHist(HISTOGRAM *hist);
void  IncCounter(HISTOGRAM *hist, int index);
float GetAvgHist(HISTOGRAM *hist);
void  PrintHist(HISTOGRAM *hist);
void  PrintHist2(HISTOGRAM *hist, HISTOGRAM *hist2);
