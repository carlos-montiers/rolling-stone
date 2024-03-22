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

extern MAZE *DeadMaze;
extern MAZE *DInsertMaze;

int  DeadMoveOrdering(int depth, int number_moves);
void DeadDeactivateStones(MAZE *maze, BitString visible);
int  DeadMoveSuspected(MAZE *maze, MOVE *last_move);
PHYSID FindClosestPosMan(MAZE *maze, BitString squares, 
		      BitString already_visible);
PHYSID FindClosestPosStone(MAZE *maze, BitString squares, 
		      BitString already_visible);
void DeadMiniConflict(int minimize);

int  DeadIsGoalNode();
int  DeadMove(MAZE *maze, MOVE *last_move, int treedepth);
int  DeadStartIda();
int  DeadIda(int treedepth, int g);
int  DeadMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret, int targetpen);
int  DeadUnMakeMove(MAZE *maze, UNMOVE *unmove, int targetpen);
int  DeadLowerBound(MAZE *maze, int targetpen);
int  DeadUpdateLowerBound(MAZE *maze, PHYSID pos, int targetpen);
