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

#define MANTO(maze,to) \
	maze->manpos = to

#define STONEFROMTO(maze,from,to) {\
	signed char stonei      = maze->PHYSstone[from]; \
	maze->PHYSstone[from]   = -1; \
	maze->PHYSstone[to]     = stonei; \
	maze->stones[stonei].loc = to; \
	UnsetBitBS(maze->stone,from); \
	SetBitBS(maze->stone,to); \
	}

extern int DirToDiff[8];
extern int  OppDir[8];
extern int NextDir[8];
extern int PrevDir[8];

int  GenerateMoves(MAZE *maze, MOVE *moves);
int  MakeMove(MAZE *maze, MOVE *move, UNMOVE *unmove, int targetpen);
int  UnMakeMove(MAZE *maze, UNMOVE *unmove, int targetpen);
int  DistToGoal(MAZE *maze, PHYSID start, PHYSID goal, PHYSID *last_over);
void Moves(MAZE *maze, PHYSID *from, signed char *reach);
void GenAllSquares( PHYSID pos, PHYSID *from, BitString all_squares );
void PushesMoves(MAZE *maze, PHYSID start, PHYSID goal,
		 int *pushes, int *moves,
		 BitString stone_squares, BitString man_squares);
void PushesMoves2(MAZE *maze, PHYSID start, PHYSID goal,
		 int *pushes, int *moves,
		 BitString stone_squares, BitString man_squares);
bool ValidSolution(MAZE *maze, MOVE *solution);
int DiffToDir(int diff);

/* all the move ordering functions: */
int NoMoveOrdering(int depth, int number_moves);
int NewMoveOrdering(int depth, int number_moves);
int ManDistMoveOrdering(int depth, int number_moves);

int calculateBoxPath(MAZE *maze, PHYSID start, PHYSID goal, PHYSID lastFrom, PHYSID path[]);
