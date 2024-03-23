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

int  VerifyConflict(CONFLICTS *c, BitString conflict, BitString reach, int pen);
void InitConflicts(CONFLICTS *c);
void DelConflicts(CONFLICTS *c);
void AddPenalties(CONFLICTS *c);
void InsertPenalty(CONFLICTS *c, int i, int p);
void InitPenalty(PENALTY *p, int penalty);
PENALTY *FindPenalty(CONFLICTS *c, int penalty);
void AddConflicts(PENALTY *p);
void InsertConflict(PENALTY *p, BitString c, BitString no_reach);
void AddConflict(CONFLICTS *c,BitString conflict,BitString no_reach,int penalty);

int  GetPriorPostPen(MAZE *maze, int penalty, int *prior, int *post);
int  GetPenalty(MAZE *maze, int targetpen);


void PrintConflicts(MAZE *maze, CONFLICTS *c);

void AddTestedPen(CONFLICTS *c, BitString relevant, BitString stones,
				PHYSID manpos, PHYSID stonepos, int goodtested);
void AddTestedDead(CONFLICTS *c, BitString relevant, BitString stones,
				PHYSID manpos, PHYSID stonepos);
int WasTestedPen(CONFLICTS *c, BitString stones, BitString reach,
				PHYSID stonepos);
int WasTestedDead(CONFLICTS *c, BitString stones, BitString reach,
				PHYSID stonepos);

void PrintConflict( CONFLICTS *c, int peni, int coni );
void PrintTested(MAZE *maze, int num);
void PrintMatches(MAZE *maze);

void RemoveConflict( CONFLICTS *c, int peni, int coni);
int  RemoveDuplicates( CONFLICTS *c, int penalty,
		      BitString pattern, BitString no_reach );
void RemoveWorst( CONFLICTS *c );

