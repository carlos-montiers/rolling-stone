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

void Set_Timer();
void Remove_Timer();
uint64_t GetTimeInMilliseconds();
uint64_t CalculateElapsedTimeSeconds(uint64_t startTimeMS, uint64_t stopTimeMS);
