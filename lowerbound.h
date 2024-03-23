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

int MinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove, int targetpen);
int BetterLowerBound(MAZE *maze);
int BetterUpdateLowerBound(MAZE *maze, UNMOVE *unmove, int targetpen);
int BetterUpdateLowerBound2(MAZE *maze, UNMOVE *unmove, int targetpen);

int PlainLowerBound(MAZE *maze);
int PlainMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove);
