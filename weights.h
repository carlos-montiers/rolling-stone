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

#define ConnectedDir(maze,pos,dir1,dir2) \
		(((dir1)==NODIR||(dir2)==NODIR)?(0): \
		(((*(maze)->connected)[(dir1)][(pos)]) & ( 1<<(dir2) )))

#define StoneDist(maze,pos,goal) \
	(IsBitSetBS(maze->one_way,pos)? \
	((*(maze)->s_distances)[WhereMan(maze,pos)][pos][goal]): \
	((*(maze)->s_distances)[0][pos][goal]))
#define StoneDistDir(maze,pos,goal,dir) \
				 ((*(maze)->s_distances)[dir][pos][goal])
#define ManDist(maze,pos,goal)   ((*(maze)->m_distances)[pos][goal])
#define DistDist(maze,pos,goal)  ((*(maze)->d_distances)[pos][goal])

#define StoneDistManpos(maze,start,goal,manpos) \
	((*maze->s_distances)[GetManDir(maze,start,manpos)][start][goal])

void SetDistDead(MAZE *maze);
void MarkReachDist(MAZE *maze,PHYSID manpos,int stonepos,BitString obstacles);
void SetManDistances(MAZE *maze);
void SetDistDistances(MAZE *maze);
void SetStoneDistances(MAZE *maze);
void InitDCache(MAZE *maze);
void FreeDCache(MAZE *maze);
DCACHE *InDCache(MAZE *maze, BitString stones_done);
void GetNewDistances(MAZE *maze, UNMOVE *ret);

DIST GetOptDist(MAZE *maze, PHYSID start, PHYSID goal, int dir);
DIST GetShortestDist(MAZE *maze, PHYSID goal, PHYSID start);

#define WhereMan(maze,pos) GetManDir((maze),(pos),(maze)->manpos)
int    GetManDir(MAZE *maze, PHYSID curr, PHYSID goal);
int    GetScew(MAZE *maze, PHYSID from, PHYSID via);
void   NewAddScew(MAZE *maze, DIST *add, DIST *scew,
		PHYSID start, PHYSID curr, int from_dir);

DIST XDistMan(MAZE *maze, PHYSID from, PHYSID to);
DIST XDistStone(MAZE *maze, PHYSID from, PHYSID to);

void  DistHist(MAZE *maze);
void SDistHist(MAZE *maze);
void XDistHist(MAZE *maze, int *all, int *scew);
void PrintMazeDist(MAZE *maze, PHYSID to, PHYSID manpos);


