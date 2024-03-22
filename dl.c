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

#include "board.h"

#define STARTP 27
#define ENDP   60

void sigint () {

	Mprintf( 0, "\n");
	PrintMaze(IdaInfo->IdaMaze);
	print_stats(0);
	signal(SIGINT,sigint);
}

int main() {

	char   *ev;
	int    i;

	InitRandom();
	init_opts();
	InitBS();
	InitIDA(&MainIdaInfo);
	IdaInfo = &MainIdaInfo;

	ev = getenv("PP");
	if (ev!=NULL) IdaInfo->PrintPriority=atoi(ev);
	else IdaInfo->PrintPriority=2;
	Mprintf(2, "PrintPriority: %i\n", IdaInfo->PrintPriority);

	i = InitTree(DlSup1);
	LoadTree(i,DL1PATHFILE);
	i = InitTree(DlSup2);
	LoadTree(i,DL2PATHFILE);
/*
*/
	signal(SIGINT,sigint);

	MainMenu();

	My_exit(0,"");
	return(0);
}

void TestX(MAZE *maze, PHYSID from, PHYSID to)
{
	MOVE move;
	UNMOVE unmove;
	int  r;

	move.from = from;
	move.to   = to;
	move.last_over = from;
	move.macro_id  = 0;
	move.move_dist = 1;

	MakeMove(maze,&move,&unmove, ENDPATH);

	r = DeadMove(maze,&move,1);
	
	Mprintf( 0, "Return value from DeadMove = %i\n", r);
}
