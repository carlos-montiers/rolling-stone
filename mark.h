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

extern PHYSID AvoidThisSquare;

void MarkAll(MAZE *maze);
void MarkOut(MAZE *maze, PHYSID pos);
void MarkDead(MAZE *maze);
void MarkOneConnected(MAZE *maze);
void MarkTun(MAZE *maze);
void MarkTG(MAZE *maze);

void MarkReach(MAZE *maze);
void CleanReach(MAZE *maze);
void UpdateReach(MAZE *maze, PHYSID stonepos);
void MarkReachNoUnreach(MAZE *maze);
void MarkReachQuick(MAZE *maze, PHYSID from);
void MarkPlayersReachableSquares(MAZE *maze, PHYSID parents[]);

int  IsStoneDone(MAZE *maze, PHYSID moveto);
void PropStonesDone(MAZE *maze, PHYSID pos);
void SetAllStonesDone(MAZE *maze);


