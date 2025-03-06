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

extern IDA *IdaInfo;

int   StartIda(int nomacro);
bool  PrintSolution();
void  PrintSolutionUsingLURDNotation();

int   IsGoalNodeNorm(int g);
int   Ida(int depth, int g);
void  SetManStoneSquares(MAZE *maze, MOVE bestmove);
int   AbortSearch();
void  InitIDA(IDA *ida);

int   DistantSquares(PHYSID s1, PHYSID s2, short crowding);
int   DistantMove(MAZE *maze, MOVE *last_move, MOVE *test_move);
void  SetLocalCut(int k, int m, int d);
int   RegisterMove(MOVE *move, int depth);
short Crowding(MAZE *maze, PHYSID sq);
