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

extern int PosNr;

void ReadMaze(FILE *fp, MAZE *maze );
void PrintMaze(MAZE *maze);
char *PrintMove(MOVE move);
char *HumanMove(MOVE move);
void PrintBit2Maze(MAZE *maze,BitString marks);
void PrintBit3Maze(MAZE *maze,BitString marks,BitString mark2, PHYSID manpos);
