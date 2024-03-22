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

/************************* global variables *************************/

int BestScore;
BitString BestGoalArea;
int min_dist[XSIZE*YSIZE];
int bits[MAX_LOCATIONS];	/* the bits that are set if a square is
				   reachable from this entrance */

/******************************* Tunnel Macro Stuff **********************/

PHYSID  FindEndTunnel(MAZE *maze, PHYSID pos, int diff, PHYSID *last_over) {

	PHYSID next;
	int    from_dir,next_dir,waystogo,dir;

	next = pos + diff;

	next_dir = NODIR;
	/* is it a one way square and does no goal macro start here */
	if (   IsBitSetBS(maze->one_way,next)
	    && !IsBitSetBS(maze->goal,next)
	    && maze->macros[next].type!=4) {
		/* go on with tunnel */
		/* find direction of continuing at next */
		from_dir = DiffToDir(-diff);
		waystogo = 0;
		for (dir=NORTH; dir<=WEST; dir++) {
			if (   IsBitSetBS(maze->S[dir],next) 
		    	    && ConnectedDir(maze,next,from_dir,OppDir[dir])) {
				waystogo++; next_dir = DirToDiff[dir];
			}
		}
		if (waystogo==0) {
			/* DeadEnd, Don't go here */
			return(0); 
		} else if (waystogo==1) {
			/* still in tunnel, go on */
			return(FindEndTunnel(maze,next,next_dir,last_over));
		}
		/* more then one way, NOTE: could be checked if one
		 * of the possible ways is DeadEnd... return as
		 * last_over square */
		/* fall through to end of tunnel */
	}
	/* next is end of tunnel */
	*last_over = pos;
	return(next);
}

void FindStartEndTunnel(MAZE *maze, int diff, 
			PHYSID pos, PHYSID *start, PHYSID *end)
{
	int        dir;
	int	   odiff;

	dir = DiffToDir(diff);
	odiff = (diff==YSIZE?1:YSIZE);
	*end   = pos;
	*start = pos;

	while (    IsBitSetBS(maze->S[dir],*end)
	       && !IsBitSetBS(maze->goal,*end+diff)
	       && !IsBitSetBS(maze->out,*end+2*diff)
	       &&  IsBitSetBS(maze->out,*end+diff+odiff)
	       &&  IsBitSetBS(maze->out,*end+diff-odiff)
	       &&  maze->macros[*end + diff].type==0) {
		*end += diff;
	}
	dir = OppDir[dir];
	while (    IsBitSetBS(maze->S[dir],*start)
	       && !IsBitSetBS(maze->goal,*start-diff)
	       && !IsBitSetBS(maze->out,*start-2*diff)
	       &&  IsBitSetBS(maze->out,*start-diff-odiff)
	       &&  IsBitSetBS(maze->out,*start-diff+odiff)
	       &&  maze->macros[*start - diff].type==0) {
		*start -= diff;
	}
}

/******************************* Goal Room Stuff **********************/

void InitGRoom(GROOM *groom, int gridx)
{
	groom->n              = 0;
	groom->index          = (short) gridx;
	groom->number_stonesongoal  = 0;
	groom->number_stones  = 0;
	groom->number_goals   = 0;
	groom->number_squares = 0;
	groom->deadentrances  = 0;
	groom->hashkey        = 0;
	Set0BS(groom->goals);
	Set0BS(groom->squares);
}

void RemoveGRoom(MAZE *maze, int gridx)
{
	int j;

	for (j=0; j<XSIZE*YSIZE; j++) {
		if (maze->groom_index[j]==gridx)
				maze->groom_index[j]=-1;
	}
	maze->gmtrees[gridx] = NULL;
	maze->number_grooms--;
}

void AddMacro(MAZE *maze, PHYSID pos, int type, PHYSID from, 
			    PHYSID last_over, PHYSID to) {

	int n;
	n = maze->macros[pos].number_macros++;

	maze->macros[pos].macros = (MACRO*)
		My_realloc( maze->macros[pos].macros,
	        sizeof(MACRO)*maze->macros[pos].number_macros);
	maze->macros[pos].type			=type;
	maze->macros[pos].macros[n].from	=from;
	maze->macros[pos].macros[n].last_over 	=last_over;
	maze->macros[pos].macros[n].to  	=to;
}

void GroomIncPos(MAZE *maze, PHYSID pos, GROOM *groom)
/* Simply includes pos into groom */
{
	if (maze->Phys[pos].goal>=0) {
		if (maze->PHYSstone[pos]>=0) groom->number_stonesongoal++;
		groom->number_goals++;
		SetBitBS(groom->goals,pos);
	}
	if (maze->PHYSstone[pos]>=0) groom->number_stones++;
	maze->groom_index[pos] = groom->index;
	groom->number_squares++;
	SetBitBS(groom->squares,pos);
	groom->hashkey ^= RandomTable[pos];
}

void GroomExcPos(MAZE *maze, PHYSID pos, GROOM *groom)
/* Simply excludes pos from groom */
{
	if (maze->Phys[pos].goal>=0) {
		if (maze->PHYSstone[pos]>=0) groom->number_stonesongoal--;
		groom->number_goals--;
		UnsetBitBS(groom->goals,pos);
	}
	if (maze->PHYSstone[pos]>=0) groom->number_stones--;
	maze->groom_index[pos] = -1;
	groom->number_squares--;
	UnsetBitBS(groom->squares,pos);
	groom->hashkey ^= RandomTable[pos];
}

int EvaluateGroom(GROOM *groom)
{
	int stones;
	if (groom->n==0 || groom->deadentrances==1) return( 0 );
	stones = groom->number_stones-groom->number_stonesongoal;
	if (stones >3 ) return(0);
	return(  1000*(20-groom->n)
		+   5*groom->number_squares
		- 100*groom->maninout
		- 500*stones );
}

void AsimGoals(MAZE *maze, PHYSID pos, GROOM *groom)
/* This is to assimilate all goals into the groom that are adjasent to one
 * another */
{
	int dir;

	if (IsBitSetBS(maze->out,pos)) return;
	if (maze->groom_index[pos] >=0 ) return;
	/*if (maze->PHYSstone[pos]>=0) return; */
	if (maze->Phys[pos].goal>=0) {
		GroomIncPos(maze,pos,groom);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->M[dir],pos)) 
				AsimGoals(maze,pos+DirToDiff[dir],groom);
		}
		return;
	}
	return;
}

void SetMinDist(MAZE *maze)
{
	int goali,dist;
	PHYSID pos;

	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(maze->out ,pos)) continue;
		min_dist[pos] = ENDPATH;
		for (goali=0; goali<maze->number_goals; goali++) {
		     dist = ManDist(maze,maze->goals[goali].loc,pos);
		     if (min_dist[pos] > dist) min_dist[pos] = dist; 
		}		
		if (maze->PHYSstone[pos]>=0) min_dist[pos] = min_dist[pos]+1;
	}
}

static int EntrComp(const void *e1, const void *e2) {
	/*if (IsBitSetBS(MainIdaInfo->IdaMaze->stone,*(PHYSID*)e1)) */
        return(min_dist[*(PHYSID*)e1] - min_dist[*(PHYSID*)e2]);
}

void StoneReach(MAZE *maze, BitString v, PHYSID entr, PHYSID location)
{
/* pseudo-recursive function to mark the fields in v from which location is
 * reachable via entr */
/* v contains all bits set for locations of squares that can be served */
/* by this entr. */

/* The man is not allowed to touch any goal area squares of ANY goal area.
 * That is not completely correct, since we might not even know yet what
 * will be a goal area in the future, but we have to live with this for now. */

        static PHYSID stack[ENDPATH];
        static PHYSID from[ENDPATH];
        PHYSID pos,fro;
        int next_in, next_out, dir, curr_dir;
	static short     touched_from[XSIZE*YSIZE];
	static short     dir2bit[4] = {1,2,4,8};

	from[0]  = entr;
        stack[0] = location;
        next_in  = 1;
        next_out = 0;
	Set0BS(v);
	memset(touched_from,0,sizeof(short)*XSIZE*YSIZE);
        while (next_out < next_in) {
		fro = from[next_out];
                pos = stack[next_out++];
		if (IsBitSetBS(maze->one_way,pos)) {
			/* if we touched that square from every dir, we are
			 * done with it */
			curr_dir = DiffToDir(fro-pos);
			if (touched_from[pos] & dir2bit[curr_dir]) continue;
			for (dir=NORTH; dir<=WEST; dir++) {
				if (  (!IsBitSetBS(maze->M[dir],pos))
				    ||(ConnectedDir(maze,pos,curr_dir,dir))) {
					touched_from[pos] |= dir2bit[dir];
				}
			}
		} else {
			if (IsBitSetBS(v,pos)) continue;
		}
		SetBitBS(v,pos);

		for (dir=NORTH; dir<=WEST; dir++) {
                	if(  IsBitSetBS(maze->M[dir],pos)
			   &&IsBitSetBS(maze->S[OppDir[dir]],pos+DirToDiff[dir])
		    	   &&maze->groom_index[pos+DirToDiff[dir]] < 0) {
				from[next_in] = pos;
				stack[next_in++] = pos+DirToDiff[dir];
			}
		}
        }
}

void ValidateEntrances(MAZE *maze, GROOM *groom)
/* This routine tries to find out if there are useless entrances and if all
 * stones can make it to an entrance */
{
	int i;
	PHYSID pos;
	BitString reach,tmp,all_reach;

	Set0BS(all_reach);
	for (i=0; i<groom->n; i++) {
		/* Which stones can make it to this entrance */
		StoneReach(maze,reach,groom->entrances[i],groom->locations[i]);
		BitOrEqBS(all_reach,reach);
	}
	/* If there are stones that can't reach any of the entrances, see if
	 * they could make it at all, even through goal areas. If they can't
	 * reach any of the entrances, fine, if they could, set n=0, since
	 * this is not a good goal area then */
	/* exception is when a stone is in the goal area already */
	BitAndNotBS(tmp,maze->stone,all_reach);
	if (Isnt0BS(tmp)) {
		for (pos=0; pos<XSIZE*YSIZE; pos++) {
			if (!IsBitSetBS(tmp,pos)) continue;
			/* this is a position of a stone not reaching any
			 * entrance, make an exception if we are in the goal
			 * area already */
			if (IsBitSetBS(groom->squares,pos)) continue;
			/* Can this stone make it at all? Then no goos! */
			for (i=0; i<groom->n; i++) {
			  if (  GetOptDist(maze,pos,groom->locations[i],NODIR)
			      < ENDPATH) {
				groom->deadentrances = 1;
				return;
			  }
			}
		}
	}
}

void GrowDFS(MAZE *maze, GROOM *groom, int g)
/* receives a groom, Entrances not set yet, no eval done and tries to
 * improve on it and over Bound */
{
	int i,n,dir;
	int score;
	PHYSID pos;
	PHYSID entrances[MAX_LOCATIONS];
	BitString tmp;

	/* We assume that if we found a BestGoalArea, it can only be grown,
	 * so throw everything away that is smaller than BestGoalArea */
	IncNodeCount(g);
	if (IdaInfo->node_count > 10000)
	    return;
	if (groom->n > 20) return;

	BitAndNotBS(tmp,BestGoalArea,groom->squares);
	if (Isnt0BS(tmp)) return;
	
	/* check if already looked at */
	if (GGGetHashTable(groom->hashkey)) return;
	GGStoreHashTable(groom->hashkey);
	PickUpEntrances(maze,groom->index);
	score = EvaluateGroom(groom);
	/*if (score < (20000 - (20000 - BestScore)*2)) return;*/
	if (BestScore > 19000 && score < 18000) return;
	if (BestScore > score + 5000) return;
	if (BestScore > 18500 && groom->maninout + groom->n > 10) return;
	if (score > BestScore) {
		BestScore = score;
		CopyBS(BestGoalArea,groom->squares);
	}

	/* sort entrances so near squares are first added */
	n = groom->n;
	memcpy(entrances,groom->entrances,sizeof(PHYSID)*n);
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(groom->squares,pos)) {
			for (dir=NORTH; dir<=WEST; dir++) {
                		if(  IsBitSetBS(maze->M[dir],pos)
			       /*&&!IsBitSetBS(maze->stone,pos+DirToDiff[dir])*/
				   /* no need to check for real entr-> TT! */
		    	   	   &&maze->groom_index[pos+DirToDiff[dir]]<0) {
				   entrances[n] = pos+DirToDiff[dir];
				   n++;
				   if (n>=MAX_LOCATIONS) return;
				}
			}
		}
	}
	My_qsort(entrances,n,sizeof(PHYSID),EntrComp);

	for (i=0; i<n; i++) {
		/* foreach entrance do */
		pos = entrances[i];
		if (/*  (maze->PHYSstone[pos]>=0)
		    ||*/(IsBitSetBS(groom->squares,pos)))
			continue;
		GroomIncPos(maze,pos,groom);
		GrowDFS(maze,groom,g+(IsBitSetBS(maze->dead,pos)?1:0));
		GroomExcPos(maze,pos,groom);
	}
}

int GrowRoom(MAZE *maze, PHYSID pos, GROOM *groom)
/* run a DFS Branch and Bound through the goal area search space to find 
 * the "best" goal area topology, least entrances and then most squares. */
{
	PHYSID p;

	/* setup distance array */
	SetMinDist(maze);
	IdaInfo->node_count = 0;

	/* Asimilate all goals connected to pos */
	AsimGoals(maze,pos,groom);
if (groom->number_goals < 3) return(0);
	PickUpEntrances(maze,groom->index);
	BestScore = EvaluateGroom(groom);
	CopyBS(BestGoalArea,groom->squares);
	GrowDFS(maze,groom,0);
	for (p=0; p<XSIZE*YSIZE; p++) {
		if (IsBitSetBS(groom->squares,p))
			GroomExcPos(maze,p,groom);
		if (IsBitSetBS(BestGoalArea,p))
			GroomIncPos(maze,p,groom);
	}
PrintBit2Maze(maze,maze->grooms[0].squares);
	return(1);
}

void PickUpEntrances(MAZE *maze, int index) {
	PHYSID pos;
	int dir,i;
	PHYSID manioentr[MAX_LOCATIONS];
	PHYSID manioloca[MAX_LOCATIONS];
	
	maze->grooms[index].n = 0;
	maze->grooms[index].maninout = 0;
	maze->grooms[index].deadentrances = 0;
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (  IsBitSetBS(maze->out,pos)
		    ||maze->groom_index[pos] < 0)
			continue;
		if (index != maze->groom_index[pos]) continue;
		for (dir=NORTH; dir<=WEST; dir++) {
			if (  maze->groom_index[pos+DirToDiff[dir]]<0) {
			    if (IsBitSetBS(maze->S[OppDir[dir]],pos+DirToDiff[dir])) {
				maze->grooms[index].
					locations[maze->grooms[index].n] 
					= pos;
				maze->grooms[index].
					entrances[maze->grooms[index].n]
					= pos+DirToDiff[dir];
				maze->grooms[index].n++;
				if (maze->grooms[index].n==MAX_LOCATIONS) {
					maze->grooms[index].n = 0;
					return;
				}
			    } else if (IsBitSetBS(maze->M[OppDir[dir]],
					pos+DirToDiff[dir])) {
				/* count the man only entrances */
				manioloca[maze->grooms[index].maninout]
					= pos;
				manioentr[maze->grooms[index].maninout]
					= pos+DirToDiff[dir];
				maze->grooms[index].maninout++;
			    }
			}
		}
	}
	/* add the man entrances at the end of the stone entrances */
	for (dir=0,i=maze->grooms[index].n;
	     dir<maze->grooms[index].maninout;
	     dir++, i++) {
		maze->grooms[index].locations[i] = manioloca[dir];
		maze->grooms[index].entrances[i] = manioentr[dir];
	}

	ValidateEntrances(maze,maze->grooms+index);
}

int SubMacro(MAZE *maze, MOVE *moves, int *move_number) {
/* Substitute a move onto a macro square with a macro move */
/* return 1 only if the macro should be the only move(s) to try in this
 * position */
	int    i,num_matched;
	BitString s;
	MACRO  *macro;
	GMNODE *gmnode;
	GROOM  *groom;
	MOVE    m = moves[*move_number];
	MOVE   *o_m;
	int     o_n;
	DIST    min_dist;
	int     dist, allow_cut;

	if (maze->macros[m.to].type != 0) {
	   switch (maze->macros[m.to].type) {
	   case 1:
	   case 3:
		break;
	   case 2:
		if (Options.mc_tu==0) break;
		o_m = moves + *move_number;
		/* This is the One way Tunnel Macro */
		for (i=0; i<maze->macros[m.to].number_macros; i++) {
			macro = &(maze->macros[m.to].macros[i]);
			if (m.last_over==macro->from) {
				/* Check for stones in Tunnel first */
				dist = DistToGoal(maze,m.from,
					       macro->to,&(o_m->last_over));
				if (dist!=-1) {
				       o_m->to=macro->to;
				       o_m->macro_id  = 2;
				       o_m->move_dist = (DIST) dist;
				       return(SubMacro(maze,moves,move_number));
				} else {
				       /* Stone was in the way, so it creates a
				        * deadlock moving this way! */
				       *o_m = DummyMove;
				       return(0);
				}
			}
		}
		break;
	   case 4:
		if (Options.mc_gm==0) break;
		gmnode = maze->gmtrees[maze->groom_index[m.to]];
		if (gmnode == NULL) return(0);
		o_m = moves;
		o_n = 0;
		num_matched = 0;
		/* is turned on, only after a successful goal macro was created,
		 * that is, a goal macro inside was optimally found, or one that
		 * was planned to be non-optimal was created */
		allow_cut = NO;
		for (i=0; i<gmnode->number_entries; i++) {
			if (gmnode->entries[i].entrance_loc==m.last_over){
				/* check if Macro move is possible if more
				 * then one entrance to the goal exist */
			      num_matched = 0;
			      dist = DistToGoal(maze,m.from,
				        gmnode->entries[i].goal_loc,
					&(o_m->last_over));
			      if (dist==-1) continue;
			      o_m->move_dist = (DIST) dist;
			      o_m->from      = m.from;
			      o_m->to        = gmnode->entries[i].goal_loc;
			      o_m->macro_id  = 4;
			      o_m++;
			      o_n++;
			}
		}
		if (o_n > 0) {
			*move_number = o_n;
			return(1);
		} else {
			/* If there are macros, but not from this entrance,
			   don't do this macro, and do not single step.
			   Chances are that we just have to wait until
			   stones are put into other places from other
			   entrances */
			if (  gmnode->number_entries>0
			    &&num_matched==0) {
				(*move_number)--;
				return(0);
			}

			/* we could not find a valid macro move from this
			 * square. Now, if other entrances exist, and
			 * moveable stones are inside (!stones_done), don't make
			 * this move at all, but single step return(0), but
			 * if other entrances exist, we might be able to
			 * change things inside, then don't execute that
			 * move, number_moves--; return(0) */
			groom = &maze->grooms[maze->groom_index[m.to]];
			if (  groom->n + groom->maninout>1
			    ||maze->Phys[m.from].s_free>2) {
				BitAndAndNotBS(s,groom->squares,
					 maze->stone,maze->stones_done);
				if (Isnt0BS(s)) (*move_number)--;
			}

			return(0);
		}
	   }
	} else if ( maze->groom_index[m.to] >= 0 ) {
	    /* this is a stone in the goal area, find a place to set it to */
	    if (Options.mc_gm==0) return(0);
	    gmnode = maze->gmtrees[maze->groom_index[m.to]];
	    if (gmnode == NULL) return(0);
	    o_m = moves;
	    o_n = 0;
	    min_dist = MAXDIST;
	    /* find the closest square and dump the stone there */
	    for (i=0; i<gmnode->number_entries; i++) {
		    /* check if Macro move is possible if more
		     * then one entrance to the goal exist */
		  dist = DistToGoal(maze,m.from,gmnode->entries[i].goal_loc,
			  &(o_m->last_over));
		  if (dist==-1) continue;
		  else if (dist < (int)min_dist) {
		      min_dist       = (DIST)dist;
		      o_m->move_dist = (DIST)dist;
		      o_m->from      = m.from;
		      o_m->to        = gmnode->entries[i].goal_loc;
		      o_m->macro_id  = 4;
		      o_n            = 1;
		  }
	    }
	    if (o_n > 0) {
		*move_number = o_n;
		return(1);
	    } else {
		/* the stone in the goal area could not get to a final goal
		 * square, just try pushing it around and lets see what
		 * happens */
		return(0);
	    }
	}
	return(0);
}

void DelGMTree(GMNODE *gmnode) {
	int     i;
	if (gmnode==NULL) return;
	gmnode->references--;
	if (gmnode->references>0) return;
	GMDelHashEntry(gmnode->hashkey);
	for (i=0; i<gmnode->number_entries; i++) {
		DelGMTree(gmnode->entries[i].next);
	}
	My_free(gmnode->entries);
	My_free(gmnode);
}

GMNODE *GetGMTree() {
	GMNODE *gmnode;

	gmnode = (GMNODE*)My_malloc(sizeof(GMNODE));
	gmnode->references	= 1;
	gmnode->number_entries	= 0;
	gmnode->hashkey		= 0;
	gmnode->entries		= NULL;
	return(gmnode);
}
	
void GetProperties(MAZE *maze, GROOM *groom, int properties[MAXGOALS])
{

	int entri, min_dist, goali;
	int dist, closest; 
	int min_d[MAX_LOCATIONS];
	int    changed_fixed;
	PHYSID pos;
	BitString outstone;

	GetBaseProps(maze,groom,properties);
	BitOrBS(outstone,maze->out,maze->stone);
	do {
		changed_fixed=NO;
		for (goali=0; goali<maze->number_goals; goali++) {
			pos = maze->goals[goali].loc;
			if (   (groom->index != maze->groom_index[pos])
			    || (properties[goali] & PROP_FIXED))
				continue;
			if (   (   (   IsBitSetBS(maze->out,pos+1)
				    || IsBitSetBS(maze->out,pos-1))
			        && (   IsBitSetBS(maze->out,pos+YSIZE)
				    || IsBitSetBS(maze->out,pos-YSIZE)))
			    || (   IsBitSetBS(outstone,pos+YSIZE+1)
				&& IsBitSetBS(outstone,pos+YSIZE)
				&& IsBitSetBS(outstone,pos+1))
			    || (   IsBitSetBS(outstone,pos+YSIZE-1)
				&& IsBitSetBS(outstone,pos+YSIZE)
				&& IsBitSetBS(outstone,pos-1))
			    || (   IsBitSetBS(outstone,pos-YSIZE+1)
				&& IsBitSetBS(outstone,pos-YSIZE)
				&& IsBitSetBS(outstone,pos+1))
			    || (   IsBitSetBS(outstone,pos-YSIZE-1)
				&& IsBitSetBS(outstone,pos-YSIZE)
				&& IsBitSetBS(outstone,pos-1)) ) {
				changed_fixed=YES;
				properties[goali] |= PROP_FIXED;
			}
		}
	} while (changed_fixed==YES);

	/* find closest entrance(s) to each square in goal area */
	/* We have to subtract the distance of the entrance to the closest
	 * goal square to equal different distances of entrances */
	/* set all the bits one entrance could set in bits[] and
	 * prime min_d with the distance to closest goal square */
	dist = 0;
	for (entri=0; entri<groom->n; entri++) {
		min_d[entri] = ENDPATH;
		for (goali=0; goali<maze->number_goals; goali++) {
			pos = maze->goals[goali].loc;
			if (maze->PHYSstone[pos] >= 0) continue;
			dist = StoneDistManpos(maze, groom->locations[entri],
			     pos, groom->entrances[entri]);
			if (min_d[entri] > dist) min_d[entri] = dist;
		}
	}

	/* find which are the closest squares to each entrance */
	for (goali=0; goali<maze->number_goals; goali++) {
		pos = maze->goals[goali].loc;
		if (maze->PHYSstone[pos] >= 0) continue;
		if (maze->groom_index[pos]!=groom->index) continue;
		closest=0;
		min_dist = ENDPATH;
		for (entri=0; entri<groom->n; entri++) {
			dist = StoneDistManpos(maze,groom->locations[entri],
			       maze->goals[goali].loc,groom->entrances[entri])
			     - min_d[entri];
			if (dist < min_dist) {
				closest  = PROP_CLOSEST << (PROP_ESHIFT*entri);
				min_dist = dist;
			} else if (dist == min_dist) {
				closest = 0;
			}
		}
		properties[goali] |= closest;
	}
}

void GetSquareValues(MAZE *maze, GROOM *groom,
		int values[MAXGOALS], int before[MAXGOALS])
{
	int     after[MAXGOALS]; 		/* values after stone is in   */
	int	com_before[MAX_LOCATIONS];      /* which to which entrance    */
	int	com_after[MAX_LOCATIONS];       /* can the man go inside maze */
						/* from is index into table,
						 * bit is to which entrance   */
	char    goali,stonei;
	PHYSID  pos;

	Communicating(maze,groom,com_before);
	GetProperties(maze,groom,before);
	for (goali=0; goali<maze->number_goals; goali++) {

		pos = maze->goals[goali].loc;
		if (groom->index != maze->groom_index[pos]) continue;
		if (maze->PHYSstone[pos] >= 0) continue;

		AddStone(maze,pos,stonei);
		GetBaseProps(maze,groom,after);
		Communicating(maze,groom,com_after);
		values[goali] = ValueSquare(maze,groom,goali,
					before,after,com_before,com_after);
		RemoveStone(maze,pos,stonei);
	}
}

void MarkReachGRoom(MAZE *maze, int index) {
/* recursive function to mark the fields that are reachable */
	
	static PHYSID stack[ENDPATH];
	PHYSID pos;
	int next_in, next_out,dir;
	CleanReach(maze);

	stack[0] = maze->manpos;
	next_in  = 1;
	next_out = 0;
	while (next_out < next_in) {
		pos = stack[next_out++];
		if (IsBitSetBS(maze->reach, pos) ) continue;
		if (maze->PHYSstone[pos] >= 0 && pos!=maze->manpos) continue;
		if (   index >=0
			/* allow the man on all dead squares since they cant
			   be blocked by stones */
		    && !IsBitSetBS(maze->dead,pos)
		    && pos != stack[0]
		    && maze->groom_index[pos] != index) continue;
		/* The following is used to make local move generation
		 * possible */
		if (AvoidThisSquare==pos) continue;
		SetBitBS(maze->reach,pos);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->M[dir],pos)) 
				stack[next_in++] = pos +DirToDiff[dir];
		}
	}
	CopyBS(maze->old_no_reach,maze->no_reach);
	BitNotAndNotBS(maze->no_reach,maze->reach,maze->out);
	BitAndNotEqBS(maze->no_reach,maze->stone);
}

int GoalReach(MAZE *maze, GROOM *groom, int values[MAXGOALS], PHYSID start,
		PHYSID manpos, int index, int optimal, int mark)
{
/* pseudo-recursive function to mark the fields that are reachable */
/* v contains all bits set for locations of goals that are reachable */
/* index is the goal room index, 
   1.) if index < 0 then allow the man to go all over the place 
   2.) if optimal==YES then allow only to reach goals the optimal way */
/* return if there is any square not reachable == dead */

        static PHYSID stack[ENDPATH];
        static PHYSID from[ENDPATH];
        static DIST   dist[ENDPATH];
        PHYSID pos,fro,old_man;
        int next_in, next_out, dir, number;
	DIST dis;
	BitString s_visited[4];

	memset(s_visited,0,sizeof(BitString)*4);

	from[0]  = manpos;
	old_man  = maze->manpos;
        stack[0] = start;
	dist[0]  = 0;
        next_in  = 1;
        next_out = 0;
	number   = 0;			/* count the number reachable */
        while (next_out < next_in) {
		fro = from[next_out];
		dis = dist[next_out];
                pos = stack[next_out++];
                if (maze->PHYSstone[pos] >= 0) 
			continue;
		if (   optimal
		    && !IsBitSetBS(maze->one_way,pos)
		    && dis > StoneDistManpos(maze,start,pos,old_man))
			continue;
		dir = DiffToDir(fro-pos);
		if (IsBitSetBS(s_visited[dir],pos)) continue;

		/* set maze up to make the reach analysis for the man */
		MANTO(maze,fro);
		AvoidThisSquare = pos;
		MarkReachGRoom(maze,index);

		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->reach,pos+DirToDiff[dir]))
				SetBitBS(s_visited[dir],pos);
		}
		if (maze->Phys[pos].goal>=0) {
			values[maze->Phys[pos].goal] |= mark;
			number++;
		}

		for (dir=NORTH; dir<=WEST; dir++) {
                	if (   IsBitSetBS(maze->S[dir],pos)
		    	    && maze->groom_index[pos+DirToDiff[dir]]>=0
		    	    && IsBitSetBS(maze->reach, pos-DirToDiff[dir] ))  {
				from[next_in] = pos;
				dist[next_in] = dis + 1;
				stack[next_in++] = pos+DirToDiff[dir];
			}
		}
        }
	MANTO(maze,old_man);
	AvoidThisSquare = 0;
	return( number <  groom->number_goals - maze->number_stones );

}


void Communicating(MAZE *maze, GROOM *groom, int com[MAX_LOCATIONS])
/* enter into com which entrances communicate with which, that is stone and
 * maninout! */
{
	int    entri,toi;
	PHYSID from,to,old_manpos;
	/* put man on all entrance squares, MarkReachGRoom, and check... */

	AvoidThisSquare = 0;
	old_manpos = maze->manpos;
	for (entri=0; entri<groom->n+groom->maninout; entri++) {
		from = groom->locations[entri];
		com[entri] = 0;
		if (maze->PHYSstone[from]<0) {
			MANTO(maze,from);
			MarkReachGRoom(maze,groom->index);
			for (toi=0; toi<groom->n+groom->maninout; toi++) {
				to = groom->locations[toi];
				if (IsBitSetBS(maze->reach,to)) {
					com[entri] |= 1<<toi;
				}
			}
		}
	}
	MANTO(maze,old_manpos);
}

int  GetBaseProps(MAZE *maze, GROOM *groom, int values[MAXGOALS])
/* enter into values, which squares can be reached from which entrances
 * under which restrictions */
/* We cheat here to make this tractable, by only looking at one restriction
 * at a time, not all combinations. This is going to be a heuristic ordering
 * anyways, so let's not wast too many cycles here. */
/* return YES if dead squares exist */
{
	int goali,entri,mark,optimal,index;
	int dead;
	
	for (goali=0; goali<maze->number_goals; goali++) {
		values[goali] = 0;
	}
	dead = NO;
	optimal = YES;
	index   = groom->index;
	for (entri=0, mark=PROP_STRICT|PROP_INSIDE|PROP_OPTIMAL|PROP_LOOSEST;
	     entri<groom->n;
	     entri++, mark<<=PROP_ESHIFT) {
		dead |= GoalReach(maze, groom, values,
			  groom->locations[entri],
			  groom->entrances[entri], index, optimal, mark);
	}
	/* Under the strictest of all conditions, we can still reach
	 * everything, be happy and exit */
	if ( dead == NO ) return( NO );

	dead    = NO;
	optimal = NO;
	index   = -1;
	for (entri=0, mark=PROP_LOOSEST;
	     entri<groom->n;
	     entri++, mark<<=PROP_ESHIFT) {
		dead |= GoalReach(maze, groom, values,
			  groom->locations[entri],
			  groom->entrances[entri], index, optimal, mark);
	}
	/* under the loosest of all conditions we can't reach all, so we
	 * have a dead square or parking */
	/* if ( dead == YES ) return( YES );*/

	optimal = YES;
	index   = -1;
	for (entri=0, mark=PROP_OPTIMAL|PROP_LOOSEST;
	     entri<groom->n;
	     entri++, mark<<=PROP_ESHIFT) {
		GoalReach(maze, groom, values,
			  groom->locations[entri],
			  groom->entrances[entri], index, optimal, mark);
	}
	optimal = NO;
	index   = groom->index;
	for (entri=0, mark=PROP_INSIDE|PROP_LOOSEST;
	     entri<groom->n;
	     entri++, mark<<=PROP_ESHIFT) {
		GoalReach(maze, groom, values,
			  groom->locations[entri],
			  groom->entrances[entri], index, optimal, mark);
	}
	return( NO );
}

int  ValueSquare(MAZE *maze, GROOM *groom, int goal_index,
	int before[MAXGOALS], int after[MAXGOALS],
	int com_before[MAX_LOCATIONS], int com_after[MAX_LOCATIONS])
/* compare values and coms to see what the value of pos-square is */
/* The best square would not influence any of the other squares reachability
 * under the most strict circumstances */
/* This routine first count how many communication channels are blocked
 * (counting each twice), then counts how many squares are lost for the
 * different restrictions. In the end, a magic weighting puts them together
 * to return one penalty number */
{
	int diff, entri, goali;
	int com_count, strict_count, loose_count,
	    optim_count, inside_count, dead_count;

	com_count = strict_count = loose_count = 0;
	optim_count = inside_count = dead_count = 0;

	/* compare communication between entrances and penalize */
	for (entri=0; entri<groom->n+groom->maninout; entri++) {
		diff = (com_before[entri] ^ com_after[entri]);
		while (diff) {
			com_count += BitNumber[(unsigned char) diff&0xff];
			diff >>= 8;
		}
	}

	/* compare accessibility of squares before and after stone was put */
	for (goali=0; goali<maze->number_goals; goali++) {
		if (goali == goal_index) continue;
		diff = (before[goali] & ~after[goali]) & PROP_REACH_ALL;
		if (diff) {
		    if (    (before[goali] & PROP_REACH_ALL)
			&& !(after[goali]  & PROP_REACH_ALL)) {
			dead_count++;
		    } else {
			strict_count += NumberBitsInt(diff&PROP_STRICT_ALL);

			diff >>= 1;
			optim_count  += NumberBitsInt(diff&PROP_STRICT_ALL);

			diff >>= 1;
			inside_count += NumberBitsInt(diff&PROP_STRICT_ALL);

			diff >>= 1;
			loose_count  += NumberBitsInt(diff&PROP_STRICT_ALL);

		    }
		}
	}
	
	/* check out if we should calculate new distances */
	if (   com_count    > 1 /* discount self references */
	    || strict_count > 0
	    || loose_count  > 0
	    || optim_count  > 0)
		before[goal_index] |= PROP_NEWDIST;
	else 
		before[goal_index] &= ~PROP_NEWDIST;
	
	/* check out if any goal square is obsctructed */
	if (   inside_count == 0
	    && strict_count == 0
	    && loose_count  == 0
	    && optim_count  == 0)
		before[goal_index] |= PROP_NONOBST;
	else 
		before[goal_index] &= ~PROP_NONOBST;

	if (loose_count == 0) before[goal_index] &= ~PROP_NONREACH;
	else before[goal_index] |= PROP_NONREACH;

	return(   PEN_COM_COUNT    * com_count	   /* comunications lost */
		+ PEN_STRICT_COUNT * strict_count 
		+ PEN_LOOSE_COUNT  * loose_count
		+ PEN_OPTIM_COUNT  * optim_count
		+ PEN_INSIDE_COUNT * inside_count
		+ PEN_DEAD_COUNT   * dead_count
		+ PEN_NOTFIXED     * ((before[goal_index]&PROP_FIXED)?0:1)
	      );
}

int ValueSquareEntr(int entri, int reach, int n)
/* Also look at if that square is closest to this entrance */
/* Serve entrances first, that are reachable by this entrance only (or
 * better), since other entrances might not be able to get here */
{
        int penalty,i,self,alt;

        self = alt = 0;
        for (i=0; i<n; i++) {
                if (reach & (PROP_STRICT<<(PROP_ESHIFT*i))) {
                        if (i==entri) {
                                self=1;
                        } else {
                                alt=1;
                        }
                }
        }
        if (self>0) penalty = 0;
        else penalty = PEN_ENTR_REACH*alt;

	/* penalise if not the closest from THIS entry */
	penalty +=(reach & (PROP_CLOSEST<<(PROP_ESHIFT*entri))) ? 0: PEN_NOTCLOSEST;

	/* penalize if closest from some other entry */
	penalty +=(reach & (PROP_CLOSEST_ALL&~(PROP_CLOSEST<<(PROP_ESHIFT*entri)))) ? PEN_NOTCLOSEST : 0;

        return(penalty);
}

int FindBestSquare(MAZE *maze, GROOM *groom,
	int target_prop, int entri, int values[MAXGOALS], int before[MAXGOALS])
{
	int best_index, value, goali, best_value;
	PHYSID pos;

	best_value  = 65000;
	best_index  = -1;
	for (goali=0; goali<maze->number_goals; goali++) {
		pos = maze->goals[goali].loc;
		if (groom->index != maze->groom_index[pos]) continue;
		if (maze->PHYSstone[pos] >= 0) continue;
		if ((before[goali]&target_prop) != target_prop) continue;
		if (before[goali]&PROP_DEAD) continue;

		/* check if we can reach this goal square */
		if (!(before[goali] & bits[entri])) continue;

		value = values[goali]
		      + ValueSquareEntr(entri,before[goali],groom->n);
		if (value >= PEN_DEAD_COUNT) continue;
	
		if (best_value > value) {
			best_value  = value;
			best_index  = goali;
		} else if (best_value == value) {
			if (best_value != 0) {
			   /* if they are the same, find closer, if!=0 */
			   if (  StoneDistManpos(maze,
					groom->locations[entri],
					maze->goals[best_index].loc,
					groom->entrances[entri])
			        >StoneDistManpos(maze,
					groom->locations[entri],
					maze->goals[goali].loc,
					groom->entrances[entri])) {
				best_index  = goali;
			   }
			} else {
			   /* if best_value == 0, use furthest away */
			   if (  StoneDistManpos(maze,
					groom->locations[entri],
					maze->goals[best_index].loc,
					groom->entrances[entri])
			        <StoneDistManpos(maze,
					groom->locations[entri],
					maze->goals[goali].loc,
				groom->entrances[entri])) {
				best_index  = goali;
			    }
			}
		}
	}
	return(best_index);
}

void CreateGMacro(MAZE *maze, GROOM *groom, int entri, int goali,
		GMNODE *gmnode, GMNODE *ret_gmnode, int before[MAXGOALS])
{
	int index; 

	index = gmnode->number_entries++;
	gmnode->entries = (GMENTRY*)My_realloc(
		gmnode->entries,sizeof(GMENTRY)*
		gmnode->number_entries);
	if (  (Options.lb_dd==YES)
	    &&(before[goali]&PROP_NEWDIST)) {
		gmnode->entries[index].new_distances = YES;
	} else {
		gmnode->entries[index].new_distances = NO;
	}
	gmnode->entries[index].goal_loc     = maze->goals[goali].loc;
	gmnode->entries[index].entrance_loc = groom->entrances[entri];
	gmnode->entries[index].next         = ret_gmnode;
}

int CreateAllGMacro(MAZE *maze, GROOM *groom, GMNODE *gmnode, int depth,
		int values[MAXGOALS], int before[MAXGOALS], int entri,
		int allow_null)
{
   int number_found;
   char goali;
   unsigned char stonei;
   GMNODE *ret_gmnode;
   PHYSID pos;

   number_found = 0;
   for (goali=0; goali<maze->number_goals; goali++) {
	pos = maze->goals[goali].loc;
	if (groom->index != maze->groom_index[pos]) continue;
	if ((before[goali] & (PROP_REACH<<(PROP_ESHIFT*entri))) == 0) continue;
	if (values[goali] >= PEN_DEAD_COUNT) continue;

	AddStone(maze,pos,stonei);
/* SR(printf("All: cut point\n");
PrintGoalMacro(maze,groom,pos,entri,goali,values,before);)*/
	ret_gmnode=BuildGMTree(maze,groom,depth+1,allow_null);
	if (ret_gmnode!=NULL) {
		CreateGMacro(maze,groom,entri,goali,gmnode,ret_gmnode,before);
		number_found++;
	} else {
		before[goali] &= PROP_DEAD;
		values[goali] += PEN_DEAD_COUNT;
	}
	RemoveStone(maze,pos,stonei);
   }
   return(number_found);
}

int CreatePropGMacro(MAZE *maze, GROOM *groom, GMNODE *gmnode, int depth,
		int values[MAXGOALS], int before[MAXGOALS], int entri,
		int allow_null,BitString considered,int prop,int *sat_prop)
/* return succes (0/1), success is also, if a square that was already
 * considered turns out to satisfy the current properties */
{
   int number_found, best_index;
   unsigned char stonei;
   GMNODE *ret_gmnode;
   PHYSID pos;

   number_found = 0;
   do {	
	best_index=FindBestSquare(maze,groom,prop,entri,values,before);

	if (best_index == -1) break;
	pos = maze->goals[best_index].loc;
	/* we have this one already generated - done, new props? */
	if (IsBitSetBS(considered,pos)) {
		*sat_prop |= prop;
		break;
	}

	SetBitBS(considered,pos);

	AddStone(maze,pos,stonei);
/* SR(printf("prop: %x\n",prop);
PrintGoalMacro(maze,groom,pos,entri,best_index,values,before);)*/
	ret_gmnode=BuildGMTree(maze,groom,depth+1,allow_null);
	if (ret_gmnode!=NULL) {
		*sat_prop |= prop;
		CreateGMacro(maze,groom,entri,best_index,
			gmnode,ret_gmnode,before);
		number_found++;
	} else {
		before[best_index] &= PROP_DEAD;
		values[best_index] += PEN_DEAD_COUNT;
	}
	RemoveStone(maze,pos,stonei);
   } while (number_found==0);
   return(number_found);
}


int StartBuildGMTree(MAZE *start_maze, GROOM *groom) {
/* starts the building of the GMtree, copies working maze */
/* no stone is in goal area */

	MAZE	   *maze;
	IDA         idainfo,*old_idainfo;
	int 	    i;

	if (groom->n > 4) {
		Debug(2,0,"No Macro: Too many entries (%i)\n",groom->n);
		return(0);
	}

	/* setup some global stuff */
	GMInitHashTable();
	for (i=0; i<groom->n; i++) {
		bits[i] = 
		   (PROP_STRICT|PROP_INSIDE|PROP_OPTIMAL|PROP_LOOSEST)
		 <<(i*PROP_ESHIFT);
	}

	/* setup IdaInfo */
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo			= &idainfo;
	idainfo.IdaMaze		= maze = CopyMaze(start_maze);

	/* setup work maze */
	/* remove all stones */
	for (i=0; i<maze->number_stones; i++ ) {
		UnsetBitBS(maze->stone,maze->stones[i].loc);
		maze->PHYSstone[maze->stones[i].loc]=-1;
	}
	maze->number_stones=0;

	IdaInfo->node_count = 0;
	start_maze->gmtrees[groom->index] = BuildGMTree(maze,groom,0,0);
	if (start_maze->gmtrees[groom->index] == NULL) {
		Debug(0,0,"%s MACRO, nodes: %li n: %i -> restart:\n",
			(start_maze->gmtrees[groom->index] != NULL)?"YES":"NO",
			IdaInfo->node_count,groom->n);
		GMInitHashTable();
		IdaInfo->node_count = 0;
		start_maze->gmtrees[groom->index] =
			BuildGMTree(maze,groom,0,1);
	}
	Debug(0,0,"%s MACRO, nodes: %li n: %i\n",
		(start_maze->gmtrees[groom->index] != NULL)?"YES":"NO",
		IdaInfo->node_count,groom->n);

	IdaInfo = old_idainfo;
	DelCopiedMaze(maze);

	if (start_maze->gmtrees[groom->index] != NULL) {
	    for (i=0; i<groom->n; i++ ) {
	   	start_maze->macros[groom->locations[i]].type = 4;
	      	start_maze->macros[groom->locations[i]].number_macros = 1;
	      	start_maze->macros[groom->locations[i]].macros = NULL;
	    }
	    return(1);
	}
	return(0);
}

GMNODE *BuildGMTree(MAZE *maze, GROOM *groom, int depth, int allow_null)
/* This routine is responsible to create and fill up the gmnode for the
   current IdaMaze position, it will create at least one macro for the
   situation that we can't use the outside.
*/
{
	GMNODE *gmnode;
	int     before[MAXGOALS];		/* values before stones       */
	int	values[MAXGOALS];		/* heuristic values for filling
						   each square on its own */
	int	goali, entri, number, n, prop, sat_prop;
	BitString considered;   /* was this entrance already considered for
				 * the current entrance */	

	SR(Debug(4,4,"Enter GMBuilt\n"));
	if (IdaInfo->node_count>10000) {
		SR(Debug(4,4,"node_count limit exceeded\n"));
		return(NULL);
	}
	IdaInfo->node_count++;

	/* check for a transposition */
	if (GMGetHashTable(NormHashKey(maze),&gmnode)) {
		if (gmnode) gmnode->references++;
		SR(Debug(4,4,"found in GM TT\n"));
		return(gmnode);
	} else gmnode = GetGMTree();

	if (maze->number_stones==groom->number_goals) {
		SR(Debug(4,4,"'GOAL' found\n"));
		return(gmnode);
	}

	/* evaluate all squares, also sets the PROPS in before */
	GetSquareValues(maze,groom,values,before);

	/* create macros for each entri seperatly */
	for (entri=0; entri<groom->n; entri++) {
	   for (goali=0; goali<maze->number_goals; goali++) {
		/* is reachable form this entry */
		if (  (before[goali] & (PROP_REACH<<(PROP_ESHIFT*entri)))
		    &&((before[goali] & PROP_NONREACH) == 0))
			break;
	   }
	   if (goali == maze->number_goals) {
		/* Wow, this is when all reachable squares from this
		 * entrance in the goal area are restricting reachability 
		 * of any entrance, we have to generate all goal macros to
		 * be save */
		number = CreateAllGMacro(maze, groom, gmnode, depth,
				values, before, entri, allow_null);
		continue; /* skip the rest of the generation */
	   }

	   Set0BS(considered);
	   number = 0;
	   sat_prop = 0;

	   prop = PROP_1111; number += CPGM(prop);
	   if (number == 0) {
	      prop = PROP_1011; number += CPGM(prop);
	      if (number==0) {
	         prop = PROP_0111; n  = CPGM(prop);
	         prop = PROP_1101; n += CPGM(prop);
	         prop = PROP_1110; n += CPGM(prop);
		 number += n;
		 if (n==0) {
		    prop = PROP_1010; n  = CPGM(prop);
	            prop = PROP_1001; n += CPGM(prop);
	            prop = PROP_0011; n += CPGM(prop);
		    number += n;
		    if (n==0) {
	               prop = PROP_0101; n  = CPGM(prop);
	               prop = PROP_0110; n += CPGM(prop);
		       prop = PROP_1100; n += CPGM(prop);
		       number += n;
		       if (n==0) {
	                  prop = PROP_1000; n  = CPGM(prop);
	                  prop = PROP_0100; n += CPGM(prop);
	                  prop = PROP_0010; n += CPGM(prop);
	                  prop = PROP_0001; n += CPGM(prop);
		          number += n;
		          if (n==0) {
	                     prop = PROP_0000; n = CPGM(prop);
		             number += n;
		          }
		       }
		    } else {
		       SR(Assert(gmnode->number_entries>0,"GMNODE no entries!\n"));
	               prop = PROP_0101; n  = CPGM(prop);
	               prop = PROP_0110; n += CPGM(prop);
		       prop = PROP_1100; n += CPGM(prop);
		       number += n;
		       if (n==0) {
	                  prop = PROP_0100; n = CPGM(prop);
		          number += n;
		       }
		    }
		 }
	      } else {
	         SR(Assert(gmnode->number_entries>0,"GMNODE no entries!\n"));
	         prop = PROP_0111; n  = CPGM(prop);
	         prop = PROP_1101; n += CPGM(prop);
	         prop = PROP_1110; n += CPGM(prop);
		 number += n;
		 if (n==0) {
	            prop = PROP_0101; n  = CPGM(prop);
	            prop = PROP_0110; n += CPGM(prop);
	            prop = PROP_1100; n += CPGM(prop);
		    number += n;
		    if (n==0) {
	               prop = PROP_0100; n += CPGM(prop);
		       number += n;
		    }
		 }
	      }
	   }
	   CHECKEND;

	   /* get the next bunch with 1 turnoff only */
	   if (sat_prop & PROP_1000 == 0) {
		   prop = PROP_1011; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1101; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1110; number += (n = CPGM(prop)); CHECKEND;
	   }
	   if (sat_prop & PROP_0100 == 0) {
		   prop = PROP_0111; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1101; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1110; number += (n = CPGM(prop)); CHECKEND;
	   }
	   if (sat_prop & PROP_0010 == 0) {
		   prop = PROP_0111; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1011; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1110; number += (n = CPGM(prop)); CHECKEND;
	   }
	   if (sat_prop & PROP_0001 == 0) {
		   prop = PROP_0111; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1011; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1101; number += (n = CPGM(prop)); CHECKEND;
	   }

	   /* get the next bunch with 2 turnoffs */
	   if (sat_prop & PROP_1000 == 0) {
		   prop = PROP_1100; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1010; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_1001; number += (n = CPGM(prop)); CHECKEND;
	   }

	   if (sat_prop & PROP_0100 == 0) {
		   prop = PROP_1100; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_0110; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_0101; number += (n = CPGM(prop)); CHECKEND;
	   }
	   if (sat_prop & PROP_0010 == 0) {
		   prop = PROP_1010; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_0110; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_0011; number += (n = CPGM(prop)); CHECKEND;
	   }
	   if (sat_prop & PROP_0001 == 0) {
		   prop = PROP_1001; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_0101; number += (n = CPGM(prop)); CHECKEND;
		   prop = PROP_0011; number += (n = CPGM(prop)); CHECKEND;
	   }

	   /* get the next bunch with 3 turnoffs */
	   if (sat_prop & PROP_1000 == 0) {
		   prop = PROP_1000; number += (n = CPGM(prop)); CHECKEND;
	   } if (sat_prop & PROP_0100 == 0) {
		   prop = PROP_0100; number += (n = CPGM(prop)); CHECKEND;
	   }
	   if (sat_prop & PROP_0010 == 0) {
		   prop = PROP_0010; number += (n = CPGM(prop)); CHECKEND;
	   }
	   if (sat_prop & PROP_0001 == 0) {
		   prop = PROP_0001; number += (n = CPGM(prop)); CHECKEND;
	   }

	   /* OK, hopeless case, how about no PROPS? */
	   prop = PROP_0000; number += (n = CPGM(prop));

	}

	/* if we found no entries, delete only if we do not allow null
	 * meaning, the last placement did not obstruct any square in the
	 * goal room */
	NormHashKey(maze);
	if (gmnode->number_entries == 0 && !allow_null) {
		SR(Debug(4,4,"No solution!\n"));
		DelGMTree(gmnode);
		GMStoreHashTable(maze,NULL);
		return(NULL);
	} else {
		SR(Debug(4,4,"normal exit, entries: %i\n",
			gmnode->number_entries));
		GMStoreHashTable(maze,gmnode);
		return(gmnode);
	}
}

/********************************* GM Hashtable stuff ***********************/

GMHASHENTRY GMHashTable[GMMAX_HASHENTRIES];

void GMInitHashTable() {
	memset(GMHashTable,0,sizeof(GMHASHENTRY)*GMMAX_HASHENTRIES);
}

void GMStoreHashTable(MAZE *maze, GMNODE *n)
{
	HASHKEY gmhashkey = maze->hashkey&GMHASHMASK;
	if (   GMHashTable[gmhashkey].lock != 0
	    && GMHashTable[gmhashkey].lock != maze->hashkey) 
		IdaInfo->gmtt_cols++;
	GMHashTable[gmhashkey].lock       = maze->hashkey;
	GMHashTable[gmhashkey].gmnode     = n;
	if (n) n->hashkey = maze->hashkey;
}

int GMGetHashTable(HASHKEY key, GMNODE **n)
{
	HASHKEY gmhashkey = key&GMHASHMASK;
	IdaInfo->gmtt_reqs++;
	if (   GMHashTable[gmhashkey].lock == key ) {
		IdaInfo->gmtt_hits++;
		*n = GMHashTable[gmhashkey].gmnode;
		return(1);
	}
	return(0);
}

void GMDelHashEntry(HASHKEY key) {

	HASHKEY gmhashkey = key&GMHASHMASK;
	if (   GMHashTable[gmhashkey].lock == key ) {
		GMHashTable[gmhashkey].lock = 0;
		GMHashTable[gmhashkey].gmnode = NULL;
	}
}

/********************************* Printing ******************************/

void PrintMazeValue(MAZE *maze, int index, int values[MAXGOALS])
{
	int x,y,num_empty,pos;
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
			else if (maze->manpos==pos) strcat(buff,"@@@@");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff,"    ");
				num_empty++;
			}
			else if (   maze->groom_index[pos]==index
				 && maze->Phys[pos].goal>=0) {
				sprintf(&buff[strlen(buff)],"%4x",
					values[maze->Phys[pos].goal]&0xffff);
			}
			else if (maze->Phys[pos].goal>=0) strcat(buff,"....");
			else if (maze->groom_index[pos]>=0) 
				sprintf(&buff[strlen(buff)],"%4x",
					maze->groom_index[pos]%0xffff);
			else strcat(buff,"    ");
		}
		Mprintf( 0, "%s\n",buff);
	}
	Mprintf( 0, "%s\n",buff);
	Mprintf( 0, "manpos: %i h: %i pen: %i search nodes: %li patterns: %d total nodes: %li\n",
		 maze->manpos,maze->h,maze->pen,
		 IdaInfo->node_count,maze->conflicts->number_patterns,
		 total_node_count );
}

void PrintGoalMacro(MAZE *maze, GROOM *groom, int setpos, int entri,
		int goali, int values[MAXGOALS], int before[MAXGOALS])
{
	int x,y,num_empty,pos;
	char buff[XSIZE*YSIZE*5+2];

	if (maze == NULL) return;
	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"????");
			else if (pos==setpos) strcat(buff," >< ");
			else if (pos==groom->entrances[entri])
				strcat(buff," <> ");
			else if(IsBitSetBS(maze->stone,pos)) strcat(buff,"$$$$");
			else if(IsBitSetBS(maze->wall,pos)) strcat(buff,"####");
			else if (  maze->manpos==pos
				 &&maze->Phys[pos].goal>=0) strcat(buff,"++++");
			else if (maze->manpos==pos) strcat(buff,"@@@@");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff,"    ");
				num_empty++;
			}
			else if (   maze->groom_index[pos]==groom->index
				 && maze->Phys[pos].goal>=0) {
				sprintf(&buff[strlen(buff)],"%4i",
					 values[maze->Phys[pos].goal]
					+ValueSquareEntr(entri,
						before[goali],groom->n));
			}
			else if (maze->Phys[pos].goal>=0) strcat(buff,"....");
			else if (maze->groom_index[pos]==groom->index) 
				sprintf(&buff[strlen(buff)],"%4i",
					maze->groom_index[pos]%0xffff);
			else strcat(buff,"    ");
		}
		Mprintf( 0, "%s\n",buff);
	}
	if (goali>=0) Mprintf(0, "%i\n", values[goali]
				+ValueSquareEntr(entri,before[goali],groom->n));
}

/****************************** Called from the outside ******************/

void FindMacros(MAZE *maze)
{
	int i,gridx;
	int diff;
	PHYSID last,last_over,pos,start,end;
	/* find macros that shoot stones to goals */
	gridx = 0;
	/* init hashtables, that are used in GrowRoom() */
	InitHashTables();
	for (i=0; i<maze->number_goals; i++) {
		/* init was to -2, if is only -1 we tried goal already */
		if (maze->groom_index[maze->goals[i].loc]<-1) {
			gridx = maze->number_grooms;
			maze->gmtrees = (GMNODE**)
				My_realloc(maze->gmtrees,
				sizeof(GMNODE*) *(maze->number_grooms+1));
			maze->grooms=(GROOM*)
				My_realloc(maze->grooms,
				sizeof(GROOM) *(maze->number_grooms+1));

			maze->number_grooms++;
			InitGRoom(&(maze->grooms[gridx]),gridx);
			if (!GrowRoom(maze,maze->goals[i].loc,
					&(maze->grooms[gridx]))) {
				RemoveGRoom(maze,gridx);
				continue;
			}
			PickUpEntrances(maze,gridx);
			if (!StartBuildGMTree(maze,maze->grooms+gridx)) {
				RemoveGRoom(maze,gridx);
			}
		}
	}
	/* Get the Tunnel macros */
	if (Options.mc_tu==NO) return;
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		/* if macro here already, go on */
		if (maze->macros[pos].number_macros>0) continue;
		/* if in goal area, don't create tunnel macros */
		if (IsBitSetBS(maze->goal,pos)) continue;
		if (maze->groom_index[pos]>=0) continue;
		/* We go from lower left to upper right, use this fact to
		 * check if we must have looked at this square already */
		if (maze->Phys[pos].tunnel==1) {
		    if (IsBitSetBS(maze->one_way,pos)) {
			/* Get the Oneway Tunnel macros */
			/* Found one, check if left/lower one is already
			 * been found, if no go on, otherwise next */
			if (maze->Phys[pos].min_dim==1) diff=YSIZE;
			else diff=1;
			last = FindEndTunnel(maze,pos,diff,&last_over);
			if (last != 0) 
				AddMacro(maze,pos,2,pos-diff,last_over,last);
			last = FindEndTunnel(maze,pos,-diff,&last_over);
			if (last != 0) 
				AddMacro(maze,pos,2,pos+diff,last_over,last);
		    } else if (!IsBitSetBS(maze->out,pos)) {
			/* Get the two-way Tunnel macros */
			if (maze->Phys[pos].min_dim==1) diff=YSIZE;
			else diff=1;
			FindStartEndTunnel(maze,diff,pos,&start,&end);
			if (start!=end && maze->macros[start+diff].type == 0) {
				AddMacro(maze,start+diff,2,start,end,end+diff);
				AddMacro(maze,end-diff,2,end,start,start-diff);
			}
		    }
		}
	}
}

void PrintSquareProp(int prop)
{
	int n;

	printf("prop: 0x%08x\n",prop);
	if (prop & PROP_FIXED)
		printf("FIXED  ");
	else
		printf("fixed  ");
	if (prop & PROP_NEWDIST)
		printf("NEWDIST  ");
	else
		printf("newdist  ");
	if (prop & PROP_NONOBST)
		printf("NONOBST  ");
	else
		printf("nonobst  ");
	if (prop & PROP_DEAD)
		printf("DEAD  ");
	else
		printf("dead  ");
	if (prop & PROP_NONREACH)
		printf("NONREACH\n");
	else
		printf("nonreach\n");
	for (n=0; n<4; n++) {
		printf("entrance %i: ",n);
		if (prop&(PROP_STRICT<<(PROP_ESHIFT*n))) printf("STRICT  ");
		else printf("strict  ");
		if (prop&(PROP_OPTIMAL<<(PROP_ESHIFT*n))) printf("OPTIMAL  ");
		else printf("optimal  ");
		if (prop&(PROP_INSIDE<<(PROP_ESHIFT*n))) printf("INSIDE  ");
		else printf("inside  ");
		if (prop&(PROP_LOOSEST<<(PROP_ESHIFT*n))) printf("LOOSEST  : ");
		else printf("loosest  : ");
		if (prop&(PROP_CLOSEST<<(PROP_ESHIFT*n))) printf("CLOSEST");
		else printf("closest");
		printf("\n");
	}
}
