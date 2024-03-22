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

void MarkReachDist(MAZE *maze,PHYSID manpos,int stonepos,BitString obstacles)
  /* recursive function to mark the fields that are reachable if obstacles
   * (stones and walls and out squares) are in the maze */
{
  static PHYSID stack[ENDPATH];
  PHYSID        pos;
  int           top;

  Set0BS(maze->reach);
  SetBitBS(obstacles,stonepos);

  stack[0] = manpos;
  top = 1;
  while( top ) {
    pos = stack[ --top ];
    if( IsBitSetBS( maze->reach, pos) ) continue;

    SetBitBS( maze->reach,pos );
    if( !IsBitSetBS( obstacles, pos + 1 ) )     stack[ top++ ] = pos + 1;
    if( !IsBitSetBS( obstacles, pos - 1 ) )     stack[ top++ ] = pos - 1;
    if( !IsBitSetBS( obstacles, pos + YSIZE ) ) stack[ top++ ] = pos + YSIZE;
    if( !IsBitSetBS( obstacles, pos - YSIZE ) ) stack[ top++ ] = pos - YSIZE;
  }
  UnsetBitBS(obstacles,stonepos);
}

void SetStoneDistances(MAZE *maze)
/* Set all stone distances for the maze, for all squares, with
   stones on the board being fixed (stones_done) */
/* Initialize dists! */
/* We start backwards from all squares to get the distance *from* all other
 * squares */
/* Assumes that one_way (connected) is set already */
{
	int i,dir,fdir;
	int next_in, next_out;
	PHYSID man,pos,to;
	DIST dist;
	static DIST   dist_st[ENDPATH];
	static PHYSID pos_st[ENDPATH];
	static PHYSID man_st[ENDPATH];
	static int    dir_st[ENDPATH];
	BitString     obstacles;
	STNDIST      *dists;

	dists = maze->s_distances = (STNDIST*)My_malloc(sizeof(STNDIST));
	for (i=0; i<XSIZE*YSIZE*XSIZE*YSIZE*4; i++)
		((DIST*)dists)[i] = MAXDIST;
	BitOrBS(obstacles,maze->stones_done,maze->out);

	for (to=0; to<XSIZE*YSIZE; to++) {
		if (IsBitSetBS(obstacles,to)) continue;
		next_in  = 0;
		next_out = 0;
		/* position the man in all four directions, if possible */
		for (dir=NORTH; dir<=WEST; dir++) {
			pos = to+DirToDiff[dir];
			(*dists)[dir][to][to] = 0;
			if (!IsBitSetBS(obstacles,pos)) {
				if (!IsBitSetBS(obstacles,pos+DirToDiff[dir])){
					pos_st[next_in]  = pos;
					man_st[next_in]  = pos+DirToDiff[dir];
					dir_st[next_in]  = dir;
					dist_st[next_in] = 1;
					next_in++;
				}
			}
		}

		/* each value tripple on the stack means the
		 * position of a stone/man/dist to a goal */
		while (next_out < next_in) {
			pos  = pos_st[next_out];
			man  = man_st[next_out];
			fdir = dir_st[next_out];
			dist = dist_st[next_out];
			next_out++;

			if (!IsBitSetBS(maze->one_way,pos)) {

			    /* simple in the case of no one_way */
			    if ( (*dists)[NORTH][pos][to] <= dist ) continue;
			    (*dists)[NORTH][pos][to] = dist;
			    (*dists)[EAST] [pos][to] = dist;
			    (*dists)[SOUTH][pos][to] = dist;
			    (*dists)[WEST] [pos][to] = dist;

			    for (dir=NORTH; dir<=WEST; dir++) {
				/* fdir can be NODIR, watch array index! */
			  	if (   fdir != OppDir[dir]
				    &&!IsBitSetBS(obstacles,
						  pos+DirToDiff[dir])
			  	    &&!IsBitSetBS(obstacles,
						  pos+2*DirToDiff[dir])) {
				    pos_st[next_in] = pos +   DirToDiff[dir];
				    man_st[next_in] = pos + 2*DirToDiff[dir];
				    dir_st[next_in] = dir;
				    dist_st[next_in] = dist+1;
				    next_in++;
				}
			    }
			} else {
			    MarkReachDist(maze,man,pos,obstacles);

			    /* look at each direction to see if it is connected
			     * and if so, if the man could be on that side after
			     * the last push. Add possible moves after that. */
			    for (dir=NORTH; dir<=WEST; dir++) {
			       if (  !IsBitSetBS(maze->reach,pos+DirToDiff[dir])
			           || (*dists)[dir][pos][to] <= dist)
				    continue;

			       /* we found a cheaper way to get here */
			       (*dists)[dir][pos][to] = dist;

			       /* So far, this was if the man could be here,
			        * could we have pushed the stone from here? */
			       if(IsBitSetBS(maze->reach,pos+2*DirToDiff[dir])){
				    pos_st[next_in] = pos +   DirToDiff[dir];
				    man_st[next_in] = pos + 2*DirToDiff[dir];
				    dir_st[next_in] = NODIR;
				    dist_st[next_in] = dist+1;
				    next_in++;
			       }
			    }
			}
		}
	}
	MarkReach(maze);
}

void SetDistDistances(MAZE *maze)
/* This function sets the pseudo distance, capturing the idea of influence
 * of the squares. Use breadth first to minimize retraversal.*/
{
	#define STONEPEN 2
	#define MANPEN 1
	int    from_dir,dir,tunnel,i;
	DIST   add,scew,weight;
	PHYSID curr;
	int    next_in, next_out;
	PHYSID start;
	MANDIST *dists;

	static PHYSID s_curr[ENDPATH];
	static DIST   s_weight[ENDPATH];
	static int    s_from_dir[ENDPATH];

	dists = maze->d_distances    = (MANDIST*)My_malloc(sizeof(MANDIST));
	for (i=0; i<XSIZE*YSIZE*XSIZE*YSIZE; i++)
		((DIST*)dists)[i] = MAXDIST;

	for (start=0; start<XSIZE*YSIZE; start++) {
	    if (IsBitSetBS(maze->out,start)) continue;

	    s_curr[0] = start;
	    s_weight[0] = 0;
	    s_from_dir[0] = NODIR;
	    next_in = 1;
	    next_out = 0;

	    while (next_out < next_in) {
		curr = s_curr[next_out];
		weight = s_weight[next_out];
		from_dir = s_from_dir[next_out++];

		/* is this in a tunnel? */
		tunnel = maze->Phys[curr].free == 2;

		NewAddScew(maze,&add,&scew,start,curr,from_dir);
		if ((add>0) && scew==YES) add >>= 1;
		weight += add;

        	if (weight >= DistDist(maze,start,curr)) continue;
		DistDist(maze,start,curr) = weight;

		for (dir=NORTH; dir<=WEST; dir++) {
			if (from_dir == dir) continue;
			if (IsBitSetBS(maze->S[dir],curr)) {
				s_curr[next_in] = curr+DirToDiff[dir];
				s_weight[next_in] = weight+(tunnel?0:MANPEN);
				s_from_dir[next_in++] = OppDir[dir];
        		} else if (IsBitSetBS(maze->M[dir],curr)) {
				s_curr[next_in] = curr+DirToDiff[dir];
				s_weight[next_in] = weight+(tunnel?0:STONEPEN);
				s_from_dir[next_in++] = OppDir[dir];
			}
		}
	    }
	}
}

void DistHist(MAZE *maze)
/* Count Histogram for Xdists and print it out */
{
	PHYSID    from,to;
	HISTOGRAM h,hg;

	InitHist(&h);	
	InitHist(&hg);	
	for (from = 0; from < XSIZE*YSIZE; from++) {
		if (IsBitSetBS(maze->out,from)) continue;
		if (IsBitSetBS(maze->dead,from)) continue;
		if (maze->groom_index[from]>=0) continue;
		for (to = 0; to < XSIZE*YSIZE; to++) {
			if (IsBitSetBS(maze->out,to)) continue;
			if (IsBitSetBS(maze->dead,to)) continue;
			if (maze->groom_index[to]>=0) continue;
			IncCounter(&h,ManDist(maze,from,to));
			if (GetScew(maze,from,to))
				IncCounter(&hg,ManDist(maze,from,to));
		}
	}
	PrintHist2(&h,&hg);	
}

void SDistHist(MAZE *maze)
/* Count Histogram for Xdists and print it out */
{
	PHYSID    from,to;
	HISTOGRAM h,hg;

	InitHist(&h);	
	InitHist(&hg);	
	for (from = 0; from < XSIZE*YSIZE; from++) {
		if (IsBitSetBS(maze->out,from)) continue;
		if (IsBitSetBS(maze->dead,from)) continue;
		if (maze->groom_index[from]>=0) continue;
		for (to = 0; to < XSIZE*YSIZE; to++) {
			if (IsBitSetBS(maze->out,to)) continue;
			if (IsBitSetBS(maze->dead,to)) continue;
			if (maze->groom_index[to]>=0) continue;
			if (GetOptDist(maze,from,to,NODIR) >= ENDPATH)
				continue;
			IncCounter(&h,GetOptDist(maze,from,to,NODIR));
			if (GetScew(maze,from,to))
			       IncCounter(&hg,GetOptDist(maze,from,to,NODIR));
		}
	}
	PrintHist2(&h,&hg);	
}

void XDistHist(MAZE *maze, int *all, int *scew)
/* Count Histogram for Xdists and print it out */
/* if all or scew are not NULL return the respective averages and only print 
 * histogram if either is NULL */
{
	PHYSID    from,to;
	HISTOGRAM h,hg, sqto, sqfrom;

	InitHist(&h);	
	InitHist(&hg);	
	InitHist(&sqto);
	InitHist(&sqfrom);
	for (from = 0; from < XSIZE*YSIZE; from++) {
		if (IsBitSetBS(maze->out,from)) continue;
		if (IsBitSetBS(maze->dead,from)) continue;
		if (maze->groom_index[from]>=0) continue;
		ResetHist(&sqto);
		ResetHist(&sqfrom);
		for (to = 0; to < XSIZE*YSIZE; to++) {
			if (IsBitSetBS(maze->out,to)) continue;
			if (IsBitSetBS(maze->dead,to)) continue;
			if (maze->groom_index[to]>=0) continue;
			IncCounter(&h,DistDist(maze,from,to));
			IncCounter(&sqto,DistDist(maze,from,to));
			IncCounter(&sqfrom,DistDist(maze,to,from));
			if (GetScew(maze,from,to))
				IncCounter(&hg,DistDist(maze,from,to));
		}
	}
	if ( all != NULL)  *all = (int) (GetAvgHist(&h)*0.5) + 1;
	if (scew != NULL) *scew = (int) GetAvgHist(&hg) + 1;
	if (scew == NULL || all == NULL) PrintHist2(&h,&hg);	
}

void SetManDistances(MAZE *maze) 
/* set dists with the distances for the man, given the current stones_done */
{
	int dir;
	DIST dist;
	int next_in, next_out;
	PHYSID to,pos;
	static DIST   dist_st[ENDPATH];
	static PHYSID pos_st[ENDPATH];
	BitString     obstacles;
	MANDIST      *dists;

	dists = maze->m_distances    = (MANDIST*)My_malloc(sizeof(MANDIST));
	for (dir=0; dir<XSIZE*YSIZE*XSIZE*YSIZE; dir++)
		((DIST*)dists)[dir] = MAXDIST;
	BitOrBS(obstacles,maze->stones_done,maze->out);

	for (to=0; to<XSIZE*YSIZE; to++) {
		if (IsBitSetBS(obstacles,to)) continue;
		pos_st[0]  = to;
		dist_st[0] = 0;
		next_in    = 1;
		next_out   = 0;
		while (next_out < next_in) {
			pos  = pos_st[next_out];
			dist = dist_st[next_out];
			next_out++;

			if (ManDist(maze,pos,to) <= dist) continue;
			ManDist(maze,pos,to) = dist;

			for (dir=NORTH; dir<=WEST; dir++) {
			   if (!IsBitSetBS(obstacles,pos+DirToDiff[dir])) {
				pos_st[next_in]  = pos + DirToDiff[dir];
				dist_st[next_in] = dist+1;
				next_in++;
			   }
			}
		}
	}
}

void InitDCache(MAZE *maze)
/* First time initialization */
{
	maze->number_d_cache = maze->size_d_cache = 0;
	maze->d_cache = NULL;
}

void FreeDCache(MAZE *maze)
/* free all memory */
{
	int i;
	for (i=0; i<maze->number_d_cache; i++) {
		My_free(maze->d_cache[i].s_distances);
		My_free(maze->d_cache[i].m_distances);
		My_free(maze->d_cache[i].connected);
	}
	My_free(maze->d_cache);
	InitDCache(maze);
}

DCACHE *InDCache(MAZE *maze, BitString stones_done)
/* find cached distances and return it if found, otherwise return empty */
{
	int i;

	/* find the match or run out */
	i = 0;
	while (   (i<maze->number_d_cache)
	       && (!EqualBS(maze->d_cache[i].stones_done,stones_done))) i++;

	/* not found, initialize to NULL */
	if (i==maze->number_d_cache) {
		if (i==maze->size_d_cache) {
			maze->d_cache = My_realloc(maze->d_cache,
				sizeof(DCACHE)*(maze->size_d_cache+10));
			maze->size_d_cache += 10;
		}
		CopyBS(maze->d_cache[i].stones_done,maze->stones_done);
		maze->d_cache[i].s_distances=NULL;
		maze->number_d_cache++;
	} else {
		MainIdaInfo.dcache_hits++;
	}
	return(maze->d_cache+i);
}

void GetNewDistances(MAZE *maze, UNMOVE *ret)
/* According to the stones_done, set new distances, but check cache first */
/* save old distances in ret -> assumes to be called from MakeMove */
{
	DCACHE *c;

	if (ret != NULL) {
		ret->old_s_distances = maze->s_distances;
		ret->old_m_distances = maze->m_distances;
		ret->old_connected   = maze->connected;
		CopyBS(ret->old_one_way,maze->one_way);
	}

	/* if too many already, just return here */
	if (maze->number_d_cache>=100) return;
		
	/* Check cache, empty if s_distances==NULL */
	if ((c = InDCache(maze,maze->stones_done))->s_distances==NULL) {
		/* not there, recalculate */
		MarkOneConnected(maze);
		SetManDistances(maze);
		SetStoneDistances(maze);
		c->s_distances = maze->s_distances;
		c->m_distances = maze->m_distances;
		c->connected   = maze->connected;
		CopyBS(c->one_way,maze->one_way);
	} else {
		/* found in cache, just reuse info */
		maze->s_distances = c->s_distances;
		maze->m_distances = c->m_distances;
		maze->connected   = c->connected;
		CopyBS(maze->one_way,c->one_way);
	}
}

void SetDistDead(MAZE *maze)
/* Some squares are dead that we can only detect because non of the goals
 * is reachable */
{
	PHYSID  start,goal;
	int     dir;

	for (start=0; start<XSIZE*YSIZE; start++) {
		if (IsBitSetBS(maze->out,start)) continue;
		if (IsBitSetBS(maze->dead,start)) continue;
		if (IsBitSetBS(maze->one_way,start)) continue;
		for (goal=0; goal<maze->number_goals; goal++) {
			if (StoneDist(maze,start,maze->goals[goal].loc)<MAXDIST)
				break;
		}
		if (goal>=maze->number_goals) {
			SetBitBS(maze->dead,start);
			for (dir=NORTH; dir<=WEST; dir++) {
				UnsetBitBS(maze->S[OppDir[dir]],
					   start+DirToDiff[dir]);
			}
		}
	}
}

int GetManDir(MAZE *maze, PHYSID pos, PHYSID manpos)
{
	int  dir;
	DIST dist;

	if (!IsBitSetBS(maze->one_way,pos)) return(NORTH);
	dist = ManDist(maze,pos,manpos);
	if     (dist>ManDist(maze,pos+1,manpos)) 	dir = NORTH;
        else if(dist>ManDist(maze,pos+YSIZE,manpos)) 	dir = EAST;
        else if(dist>ManDist(maze,pos-1,manpos)) 	dir = SOUTH;
        else if(dist>ManDist(maze,pos-YSIZE,manpos)) 	dir = WEST;
	else {
		dir = NODIR;
		SR(Assert(1,"GetManDir: where are you?\n"));
	}
	return(dir);
}

DIST GetShortestDist(MAZE *maze, PHYSID goal, PHYSID start) 
/* this is not critical, just for plain lower bound */
{
	DIST   min_w,w;
	int    dir;
	
	min_w = MAXDIST;
	for (dir = EAST; dir <= WEST; dir++) {
		w = StoneDistDir(maze,start,goal,dir);
		if (w < min_w) min_w = w;
	}
	return(min_w);
}

DIST GetOptDist(MAZE *maze, PHYSID start, PHYSID goal, int dir) 
{
	DIST w;
	
	if (dir!=NODIR) {
	   	return(StoneDistDir(maze,start,goal,dir));
	} else {
	    	w = MAXDIST;
		if(   IsBitSetBS(maze->M[NORTH],start)
		   && w>StoneDistDir(maze,start,goal,NORTH))
			w=StoneDistDir(maze,start,goal,NORTH);
		if(   IsBitSetBS(maze->M[EAST],start)
		   && w>StoneDistDir(maze,start,goal,EAST))
			w=StoneDistDir(maze,start,goal,EAST);
		if(   IsBitSetBS(maze->M[SOUTH],start)
		   && w>StoneDistDir(maze,start,goal,SOUTH))
			w=StoneDistDir(maze,start,goal,SOUTH);
		if(   IsBitSetBS(maze->M[WEST],start)
		   && w>StoneDistDir(maze,start,goal,WEST))
			w=StoneDistDir(maze,start,goal,WEST);
	        return(w);
	}
}

int GetScew(MAZE *maze, PHYSID from, PHYSID via)
/* Returns YES if via is on an optimal path to any of the goals */
{
	DIST   scew,detour;
	int    goali;

	for (goali=0; goali<maze->number_goals; goali++) {
		scew = GetOptDist(maze,from,maze->goals[goali].loc,NODIR);
		if (scew >= ENDPATH) continue;
		
		detour = GetOptDist(maze,from,via,NODIR)
	       		+ GetOptDist(maze,via,maze->goals[goali].loc,NODIR);
		/* there is no path using via */
		if (detour >= ENDPATH) continue;
		if (detour - scew == 0) return( YES );
	}
	return( NO );
}

void NewAddScew(MAZE *maze, DIST *add, DIST *scew,
		PHYSID start, PHYSID curr, int from_dir)
{
	int    free,s_free;

	/* calc penalty for this square */
	free = maze->Phys[curr].free;
	s_free = maze->Phys[curr].s_free;

	/* remove entrance freedom */
	if (from_dir != NODIR) {
	    if (IsBitSetBS(maze->S[from_dir],curr)) {
		s_free--;
		free--;
	    } else free--;
	}

	/* remove exit freedom - highest */
	if (s_free>0) {
		s_free--;
		free--;
	} else if (free>0) free--;

	*add = STONEPEN * s_free + MANPEN * (free - s_free);

	/* scew towards goal, penalizing squares off course, find min */
	*scew = GetScew(maze,start,curr);
}

DIST XDistMan(MAZE *maze, PHYSID from, PHYSID to)
/* if xdist is set, uses d_weight, otherwise m_weight */
{
	if (Options.xdist == 1)
		return(DistDist(maze,from,to));
	else
		return(ManDist(maze,from,to));
}

DIST XDistStone(MAZE *maze, PHYSID from, PHYSID to)
/* if xdist is set, uses d_weight, otherwise m_weight */
{
	if (Options.xdist == 1)
		return(DistDist(maze,from,to));
	else
		return(StoneDist(maze,from,to));
}

void PrintMazeDist(MAZE *maze, PHYSID to, PHYSID manpos)
{
	int x,y,num_empty,pos,dist;
	char buff[XSIZE*YSIZE*5+2];

	if (maze == NULL) return;
	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"????");
			else if(IsBitSetBS(maze->wall,pos)) strcat(buff,"####");
			else if (  maze->manpos==pos
				 &&maze->Phys[pos].goal>=0) strcat(buff,"++++");
			else if (manpos==pos) strcat(buff,"@@@@");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff,"    ");
				num_empty++;
			}
			else {
				dist = StoneDist(maze,pos,to);
				if (dist!=ENDPATH)
					sprintf(&buff[strlen(buff)],"%4x",dist);
				else
					strcat(buff,"    ");
			}
		}
		Mprintf( 0, "%s\n",buff);
	}
	Mprintf( 0, "%s\n",buff);
}


