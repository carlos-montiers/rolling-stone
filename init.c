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

void InitMaze(MAZE *maze)  {
	PHYSID pos;
	int dir;

	maze->lbtable	     = NULL;
	memset(&(maze->Phys),0,sizeof(PHYS)*XSIZE*YSIZE);
	maze->number_grooms  = 0;
	maze->number_goals   = 0;
	maze->number_stones  = 0;
	maze->grooms         = NULL;
	maze->gmtrees        = NULL;
	maze->stones         = NULL;
	maze->goals          = NULL;

	/* Do not initialize those, seting routines do that */
	InitDCache(maze);
	maze->s_distances    = NULL;
	maze->m_distances    = NULL;
	maze->d_distances    = NULL;
	maze->connected	     = NULL;

	maze->h		     = 0;
	maze->pen	     = 0;
	maze->g		     = 0;
	maze->hashkey	     = 0;
	maze->conflicts      = My_malloc(sizeof(CONFLICTS));
	InitConflicts(maze->conflicts);
	maze->goal_sqto      = -1;
	maze->currentmovenumber = 0;
	for (dir=NORTH; dir<=WEST; dir++) {
		Set1BS(maze->S[dir]);
		Set1BS(maze->M[dir]);
	}
	Set0BS(maze->wall);
	Set0BS(maze->dead);
	Set1BS(maze->out);
	Set0BS(maze->stone);
	Set0BS(maze->goal);
	Set0BS(maze->reach);
	Set0BS(maze->no_reach);
	Set0BS(maze->old_no_reach);
	Set0BS(maze->stones_done);
	
	for (pos = 0; pos < XSIZE*YSIZE; pos++) {
		maze->Phys[pos].tunnel    =  1;
		maze->Phys[pos].min_dim   =  0;
		maze->Phys[pos].free      =  0;
		maze->Phys[pos].s_free    =  0;
		maze->PHYSstone[pos]      = -1;
		maze->Phys[pos].goal      = -1;
		maze->groom_index[pos] = -2;
		maze->macros[pos].type = 0;
		maze->macros[pos].number_macros = 0;
		maze->macros[pos].macros = NULL;
	}
}

void ResetMaze(MAZE *maze)  {
	PHYSID pos,dir;
	int i;

	memset(&(maze->Phys),0,sizeof(PHYS)*XSIZE*YSIZE);
	Set0BS( maze->reach );
	maze->number_goals   = 0;
	maze->number_stones  = 0;
	maze->h		     = 0;
	maze->pen	     = 0;
	maze->g		     = 0;
	maze->hashkey	     = 0;
	maze->goal_sqto      = -1;
	maze->currentmovenumber = 0;
	DelConflicts(maze->conflicts);
	for (dir=NORTH; dir<=WEST; dir++) {
		Set1BS(maze->S[dir]);
		Set1BS(maze->M[dir]);
	}
	Set0BS(maze->wall);
	Set0BS(maze->dead);
	Set1BS(maze->out);
	Set0BS(maze->stone);	
	Set0BS(maze->goal);
	Set0BS(maze->reach);
	Set0BS(maze->no_reach);
	Set0BS(maze->old_no_reach);
	Set0BS(maze->stones_done);

	for (pos = 0; pos < XSIZE*YSIZE; pos++) {
		maze->Phys[pos].tunnel    =  1;
		maze->Phys[pos].min_dim   =  0;
		maze->Phys[pos].free      =  0;
		maze->Phys[pos].s_free    =  0;
		maze->PHYSstone[pos]     = -1;
		maze->Phys[pos].goal      = -1;
		maze->groom_index[pos] = -2;
		if (maze->macros[pos].type!=4) 
			My_free(maze->macros[pos].macros);
		maze->macros[pos].type = 0;
		maze->macros[pos].number_macros = 0;
		maze->macros[pos].macros = NULL;
	}
	for (i=0; i<maze->number_grooms; i++) {
		DelGMTree(maze->gmtrees[i]);
	}
	My_free(maze->gmtrees);
	maze->gmtrees = NULL;

	FreeDCache(maze);
	maze->s_distances = NULL;
	maze->m_distances = NULL;
	maze->connected	  = NULL;

	My_free(maze->d_distances);
	maze->d_distances = NULL;

	maze->number_grooms = 0;
	My_free(maze->grooms);
	maze->grooms = NULL;
}

void DelMaze(MAZE *maze) {
	PHYSID pos;
	int    i;

	My_free(maze->lbtable);
	My_free(maze->stones);
	My_free(maze->goals);
	maze->stones  = NULL;
	maze->goals   = NULL;
	maze->number_goals   = 0;
	maze->number_stones  = 0;
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (maze->macros[pos].type!=4) 
			My_free(maze->macros[pos].macros);
		maze->macros[pos].macros = NULL;
	}
	for (i=0; i<maze->number_grooms; i++) {
		DelGMTree(maze->gmtrees[i]);
	}
	My_free(maze->gmtrees);
	maze->gmtrees = NULL;

	My_free(maze->d_distances);
	maze->d_distances = NULL;

	FreeDCache(maze);
	DelConflicts(maze->conflicts);
	My_free(maze->conflicts);

	maze->number_grooms = 0;
	My_free(maze->grooms);
	maze->grooms = NULL;
}

MAZE *SaveMaze(MAZE *maze, SAVEMAZE *savemaze)
/* Save important into from maze into savemaze for future restoration */
{

    memcpy(savemaze,maze,((char*)(&maze->lbtable))-(char*)maze);
    memcpy(savemaze->stones,maze->stones,sizeof(STN)*maze->number_stones);
    memcpy(savemaze->lbtable,maze->lbtable,sizeof(LBENTRY)*maze->number_goals);
    return(maze);
}

MAZE *RestoreMaze(MAZE *maze, SAVEMAZE *savemaze)
/* restore maze from savemaze */
{

    memcpy(maze,savemaze,((char*)(&maze->lbtable))-(char*)maze);
    memcpy(maze->stones,savemaze->stones,sizeof(STN)*maze->number_stones);
    memcpy(maze->lbtable,savemaze->lbtable,sizeof(LBENTRY)*maze->number_goals);
    return(maze);
}

MAZE *UpdateMaze(MAZE *maze, MAZE *target_maze)
/* Update an already copied maze, copy only things that change during search */
/* This is specifically for penalty and deadlock searches, so don't bother
   copying stuff not needed there */
{

    memcpy(target_maze,maze,((char*)(&maze->lbtable))-(char*)maze);
    memcpy(target_maze->stones,maze->stones,sizeof(STN)*maze->number_stones);
    memcpy(target_maze->lbtable,maze->lbtable,
		sizeof(LBENTRY)*maze->number_goals);
    return(target_maze);
}


MAZE *CopyMaze(MAZE *maze) {

	MAZE *ret_maze;

	ret_maze = 
		(MAZE *)My_malloc(sizeof(MAZE));
	memcpy(ret_maze,maze,sizeof(MAZE));

	/* now copy all pointers to structures */
	ret_maze->lbtable=
		(LBENTRY*) My_malloc(sizeof(LBENTRY)*maze->number_goals);
	memcpy(ret_maze->lbtable,maze->lbtable,
		sizeof(LBENTRY)*maze->number_goals);
	ret_maze->gmtrees=
		(GMNODE**) My_malloc(sizeof(GMNODE*)*maze->number_grooms);
	memcpy(ret_maze->gmtrees,maze->gmtrees,
		sizeof(GMNODE*)*maze->number_grooms);
	ret_maze->stones=
		(STN*)My_malloc(sizeof(STN)*maze->number_stones);
	memcpy(ret_maze->stones,maze->stones,sizeof(STN)*maze->number_stones);
	ret_maze->goals=
		(GOL*)My_malloc(sizeof(GOL)*maze->number_goals);
	memcpy(ret_maze->goals,maze->goals,sizeof(GOL)*maze->number_goals);

	return(ret_maze);
}

void DelCopiedMaze(MAZE *maze) {

	if (maze == NULL) return;
	My_free(maze->lbtable);
	My_free(maze->gmtrees);
	My_free(maze->stones);
	My_free(maze->goals);
	My_free(maze);
}

