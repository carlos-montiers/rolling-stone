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

#include "board.h"

void InitHist(HISTOGRAM *hist)
{
	hist->max_index   = 0;
	hist->max_count   = 0;
	hist->total_count = 0;
	hist->total_sum   = 0;
	memset(hist->count,0,sizeof(H_int)*MAXHIST);
}

void ResetHist(HISTOGRAM *hist)
/* Reset after use, be smart about how much of count to set to 0 */
{
	memset(hist->count,0,sizeof(H_int)*hist->max_index);
	hist->max_index   = 0;
	hist->max_count   = 0;
	hist->total_count = 0;
	hist->total_sum   = 0;
}

void IncCounter(HISTOGRAM *hist, int index)
{
	if (index >= MAXHIST) {
		Mprintf(0, "IncCounter: index too large! %i\n", index);
		return;
	}
	hist->total_count++;
	hist->total_sum += index;
	if (index > hist->max_index) {
		hist->max_index = index;
	}
	hist->count[index]++;
	if (hist->count[index] > hist->max_count) {
		hist->max_count = hist->count[index];
	}
}

float GetAvgHist(HISTOGRAM *hist)
{
	return(((float)hist->total_sum)/hist->total_count);
}

void PrintHist(HISTOGRAM *hist)
{

	int line,mark;
	int scale;

	/* scale everything such that we have maximally 32 lines */
	scale = hist->max_count>>5;
	if (scale == 0) scale = 1;

	for (line=hist->max_count/scale; line>0; line--) {
		for (mark=0; mark<hist->max_index; mark++) {
			if ((hist->count[mark])/scale>=line) Mprintf(0,"*");
			else Mprintf(0," ");
		}
		Mprintf(0,"\n");
	}
	for (mark=0; mark<=(((hist->max_index)/10)+1)*10; mark++) {
		if (mark%10 == 0) Mprintf(0,"|");
		else if (mark%5 == 0) Mprintf(0,"+");
		else Mprintf(0,"-");
	}
	Mprintf(0,"\n0",mark);
	for (mark=10; mark<=((hist->max_index)/10+1)*10; mark+=10) {
		Mprintf(0,"%10i",mark);
	}
	Mprintf(0,"\ntotal count: %4i, max count: %4i, max index %4i, avg index: %5.1f\n\n",
		hist->total_count, hist->max_count, hist->max_index,
		GetAvgHist(hist));
}

void PrintHist2(HISTOGRAM *hist, HISTOGRAM *hist2)
{

	int line,mark;
	int scale,max_mark;

	/* scale everything such that we have maximally 32 lines */
	scale = (max(hist2->max_count,hist->max_count))/20;
	max_mark = max(hist2->max_index,hist->max_index);

	for (line=hist->max_count/scale; line>0; line--) {
		for (mark=0; mark<max_mark; mark++) {
			if (  (hist->count[mark])/scale >= line
			    &&(hist2->count[mark])/scale >= line)
				Mprintf(0,"+");
			else if ((hist->count[mark])/scale>=line)
				Mprintf(0,"|");
			else if ((hist2->count[mark])/scale>=line)
				Mprintf(0,"-");
			else Mprintf(0," ");
		}
		Mprintf(0,"\n");
	}
	for (mark=0; mark<=((max_mark/10)+1)*10; mark++) {
		if (mark%10 == 0) Mprintf(0,"|");
		else if (mark%5 == 0) Mprintf(0,"+");
		else Mprintf(0,"-");
	}
	Mprintf(0,"\n0",mark);
	for (mark=10; mark<=((max_mark/10)+1)*10; mark+=10) {
		Mprintf(0,"%10i",mark);
	}
	Mprintf(0,"\n'|': total count: %4i, max count: %4i, max index: %4i, avg index: %5.1f\n",
		hist->total_count, hist->max_count, hist->max_index,
		GetAvgHist(hist));
	Mprintf(0,  "'-': total count: %4i, max count: %4i, max index: %4i, avg index: %5.1f\n\n",
		hist2->total_count, hist2->max_count, hist2->max_index,
		GetAvgHist(hist2));
}

