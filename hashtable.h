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

void     InitHashTables();
void     UnSetPathFlag(MAZE *maze);
void     SetPathFlag(MAZE *maze);
void     ClearHashTable(MAZE *maze);
HASHENTRY *StoreHashTable(MAZE *maze, int down, int min_h,
		int area, int dl, int pen, int pathflag);
HASHENTRY *GetHashTable(MAZE *maze);
void     PSSetPathFlag(MAZE *maze);
void     PSUnSetPathFlag(MAZE *maze);
void     PSClearHashTable(MAZE *maze);
HASHENTRY *PSStoreHashTable(MAZE *maze, int down, int min_h, int pathflag);
HASHENTRY *PSGetHashTable(MAZE *maze);
HASHKEY  NormHashKey(MAZE *maze);
HASHKEY  DeadHashKey(MAZE *maze);
HASHKEY  UpdateHashKey( MAZE *maze, UNMOVE *move);

void GGStoreHashTable(HASHKEY hashkey);
int  GGGetHashTable(HASHKEY hashkey);

extern HASHKEY RandomTable[ 896 ];
extern HASHENTRY HashTableNorm[MAX_HASHENTRIES];
extern HASHENTRY HashTableDead[MAX_HASHENTRIES];
extern HASHENTRY HashTablePen[MAX_HASHENTRIES];

void InitRandom();
