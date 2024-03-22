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

void  InitMaze(MAZE *maze);
void  ResetMaze(MAZE *maze);
void  DelMaze(MAZE *maze);
MAZE *CopyMaze(MAZE *maze); 
void  DelCopiedMaze(MAZE *maze);
MAZE *UpdateMaze(MAZE *maze, MAZE *target_maze);

MAZE *SaveMaze(MAZE *maze, SAVEMAZE *savemaze);
MAZE *RestoreMaze(MAZE *maze, SAVEMAZE *savemaze);
