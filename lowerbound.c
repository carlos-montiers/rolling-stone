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

/**********************************************************************
 *
 *	This is a better lower bound estimator (well, routines that
 *	implements it). It uses minimum flow, maximum matching to solve
 *	which stone goes to which goal without conflicts. If no matching
 *	exists - deadlock!
 *
 **********************************************************************/

int BetterLowerBound(MAZE *maze)
/* called after distances where changed to make sure we get the stones_done
 * onto their squares */
{
	char stonei,goali;
	int  taken[MAXSTONES];

	/* initialize table with just any matching */
	maze->h = 0;
	maze->pen=0;
	memset(taken,0,sizeof(int)*maze->number_stones);

	/* first get all the done stones in */
	for (stonei=0; stonei<maze->number_stones; stonei++) {
		maze->lbtable[(int)stonei].distance = MAXDIST;
		if (  IsBitSetBS(maze->stones_done,maze->stones[stonei].loc)
		    &&IsBitSetBS(maze->goal,maze->stones[stonei].loc)) {
			/* make sure this guy gets home */
			goali = maze->Phys[maze->stones[stonei].loc].goal;
			maze->lbtable[(int)goali].stoneidx  = stonei;
			maze->lbtable[(int)stonei].goalidx  = goali;
			maze->lbtable[(int)stonei].distance = 0;
			taken[(int)goali] = YES;
		}
	}
	/* now look for any matching for the once not done yet */
	for (stonei=0; stonei<maze->number_stones; stonei++) {
		if (maze->lbtable[(int)stonei].distance == MAXDIST) {
			for (goali=0; goali<maze->number_goals; goali++) {
				if (taken[(int)goali] == NO) {
					maze->lbtable[(int)goali].stoneidx  = stonei;
					maze->lbtable[(int)stonei].goalidx  = goali;
					maze->lbtable[(int)stonei].distance =
					    StoneDist(maze,
						maze->stones[(int)stonei].loc,
						maze->goals[(int)goali].loc);
					maze->h +=
						maze->lbtable[(int)stonei].distance;
					taken[(int)goali] = YES;
					break;
				}
			}
		}
	}
	MinMatch(maze,0,NULL,ENDPATH);
	return(maze->h);

}

int MinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove, int targetpen)
/*********************************************************************
   if moveto == 0 then we need to force a full lb calculation, otherwise
   go for optimizations using the fact that only one stone was moved.
*********************************************************************/
{
	int  i,cost,cost_swap, min_h, next_in, next_out, heur_diff, dist_diff;
	char heur_stone, dist_stone;
	char stonei,goali,stonej,goalj;
	DIST disti, distj, distk;
	static short levels[MAXSTONES];
	static int stack[MAXSTONES*MAXSTONES];
	int stone_done;

	if (moveto!=0) {
	    min_h  = maze->h-unmove->move_dist;
	    stonei = maze->PHYSstone[moveto];
	    goali  = maze->lbtable[(int)stonei].goalidx;
	    disti  = StoneDist(maze, maze->stones[stonei].loc,
				     maze->goals[goali].loc);
	    i      = disti - maze->lbtable[(int)stonei].distance;
	    maze->lbtable[(int)stonei].distance = disti;
	    maze->h += i;
	    stone_done = IsBitSetBS(maze->stones_done,maze->stones[stonei].loc);
	    if (maze->h==min_h && (!stone_done || disti==0)) goto END;
	    stack[0] = stonei;
	    next_in  = 1;
	} else {
	    for (i=0; i<maze->number_stones; i++) stack[i]=i;
	    next_in  = maze->number_stones;
	    min_h    = 0;
	}
	memset(levels,0,sizeof(short)*maze->number_stones);
	next_out = 0;
	do {
	    /* only the stone that move and then that one that was traded is
	     * of interest, we don't have to to loop over all stones! */
	    stonei = stack[next_out++];
	    if (  IsBitSetBS(maze->stones_done,maze->stones[stonei].loc)
		&&IsBitSetBS(maze->goal,maze->stones[stonei].loc))
		    stone_done = YES;
	    else
		    stone_done = NO;
	    disti  = maze->lbtable[(int)stonei].distance;

	    /* if this is a done stone, skip */
	    if (stone_done && disti==0) continue;

	    goali  = maze->lbtable[(int)stonei].goalidx;
	    heur_diff = 0;  /* is negative, minimized!!!! */
	    heur_stone= MAXSTONES;
	    dist_diff = 0;  /* is negative, closer goal found */
	    dist_stone= MAXSTONES;
	    /* Loop through the goals to find the closest */
	    for (goalj=0; goalj<maze->number_goals; goalj++) {
		stonej = maze->lbtable[(int)goalj].stoneidx;
		if (   stonei == stonej
		    || IsBitSetBS(maze->stones_done,maze->stones[stonej].loc)) {
			continue;
		}
		/* find out if we should swop goals or not */
		/* cost as it is right now */
		cost = (distk = StoneDist(maze, maze->stones[stonej].loc,
					  maze->goals[goalj].loc)) + disti;
		/* cost after swap */
		distj = StoneDist(maze,maze->stones[stonei].loc,
				  maze->goals[goalj].loc);
		cost_swap
		     = StoneDist(maze, maze->stones[stonej].loc,
				 maze->goals[goali].loc) + distj;
		if (   stone_done
		    && distj == 0) {
			heur_diff = (cost_swap - cost);
			heur_stone = stonej;
			break;
		} else if (heur_diff >= (cost_swap - cost)) {
			/* lower cost matching found */
			heur_diff = (cost_swap - cost);
			heur_stone = stonej;
			/* Take this shortcut only if the stone is not moved
			 * to final square */
			if (  maze->h+heur_diff==min_h
			    &&!stone_done) break;
		} else if (  (cost_swap==cost)
			   &&(disti!=0)
			   &&(distk!=0)
			   &&(dist_diff > ((int)distj-(int)disti))
			   &&(levels[(int)stonei] >= levels[(int)stonej])) {
				/* stonei can be matched to a closer goal */
				dist_diff = distj - disti;
				dist_stone = stonej;
		}
	    }
	    if (heur_diff<0) {
		/* decrease in heuristic value found */
	        maze->h += heur_diff;
		stonej = heur_stone;
	    } else if (dist_diff<0) {
		/* closer goal found for stone that was moved */
		stonej = dist_stone;
	    } else if (!stone_done)
		continue;
	    goalj = maze->lbtable[(int)stonej].goalidx;
	    maze->lbtable[(int)goali].stoneidx = stonej;
	    maze->lbtable[(int)goalj].stoneidx = stonei;
	    maze->lbtable[(int)stonei].goalidx = goalj;
	    maze->lbtable[(int)stonej].goalidx = goali;
	    disti = maze->lbtable[(int)stonei].distance;
	    maze->lbtable[(int)stonei].distance =
		StoneDist(maze,maze->stones[stonei].loc,
			       maze->goals[goalj].loc);
	    distj = maze->lbtable[(int)stonej].distance;
	    maze->lbtable[(int)stonej].distance =
		StoneDist(maze,maze->stones[stonej].loc,maze->goals[goali].loc);
	    stack[next_in++]=stonej;
	    stack[next_in++]=stonei;
	    if (disti>maze->lbtable[(int)stonei].distance) levels[(int)stonei]++;
	    if (distj>maze->lbtable[(int)stonej].distance) levels[(int)stonej]++;
	} while (   next_out < next_in
		 && maze->h > min_h
		 && next_in < (MAXSTONES*MAXSTONES-2));
	SR(Assert(maze->h>=0,"MinMatch: heuristc < 0!\n"));
END:
	GetPenalty(maze,targetpen);
	return(maze->h);
}

int BetterUpdateLowerBound(MAZE *maze, UNMOVE *unmove, int targetpen)
/* Asumtion: Move is already made */
{
	return(MinMatch(maze,unmove->stoneto,unmove,targetpen));
}

int BetterUpdateLowerBound2(MAZE *maze, UNMOVE *unmove, int targetpen)
/* Asumtion: Move is already un-made */
{
	MinMatch(maze,unmove->stonefrom,unmove, targetpen);
	return(maze->h);
}

int PlainLowerBound(MAZE *maze)
{
	char i;

	/* initialize table with just any matching */
	maze->h = 0;
	for (i=0; i<maze->number_stones; i++) {
		maze->lbtable[(int)i].stoneidx = i;
		maze->lbtable[(int)i].goalidx  = i;
		maze->lbtable[(int)i].distance =
			GetShortestDist(maze,maze->goals[i].loc,maze->stones[i].loc);
		maze->h += maze->lbtable[(int)i].distance;
	}
	maze->pen=0;
	PlainMinMatch(maze,0,NULL);
	return(maze->h);
}

int PlainMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove)
{
/*********************************************************************
   if moveto == 0 then we need to force a full lb calculation, otherwise
   go for optimizations using the fact that only one stone was moved.
*********************************************************************/
	int i,cost,cost_swap, min_h, next_in, next_out, heur_diff, dist_diff;
	char heur_stone, dist_stone;
	char stonei,goali,stonej,goalj;
	DIST disti, distj, distk;
	static short levels[MAXSTONES];
	static int stack[MAXSTONES*MAXSTONES];

	if (moveto!=0) {
	    min_h  = maze->h-unmove->move_dist;
	    stonei = maze->PHYSstone[moveto];
	    goali  = maze->lbtable[(int)stonei].goalidx;
	    disti  = GetShortestDist(maze,maze->goals[goali].loc,
		   		maze->stones[stonei].loc);
	    i      = disti - maze->lbtable[(int)stonei].distance;
	    maze->lbtable[(int)stonei].distance = disti;
	    maze->h += i;
	    if (maze->h==min_h) {
		return(maze->h);
	    }
	    stack[0] = stonei;
	    next_in  = 1;
	} else {
	    for (i=0; i<maze->number_stones; i++) stack[i]=i;
	    next_in  = maze->number_stones;
	    min_h    = 0;
	}
	memset(levels,0,sizeof(short)*maze->number_stones);
	next_out = 0;
	do {
	    /* only the stone that move and then that one that was traded is
	     * of interest, we don't have to to loop over all stones! */
	    stonei = stack[next_out++];
	    goali  = maze->lbtable[(int)stonei].goalidx;
	    disti  = maze->lbtable[(int)stonei].distance;
	    heur_diff = 0;  /* is negative, minimized!!!! */
	    heur_stone= MAXSTONES;
	    dist_diff = 0;  /* is negative, closer goal found */
	    dist_stone= MAXSTONES;
	    /* Loop through the goals to find the closest */
	    for (goalj=0; goalj<maze->number_goals; goalj++) {
		stonej = maze->lbtable[(int)goalj].stoneidx;
		if (stonei == stonej) {
			continue;
		}
		/* find out if we should swop goals or not */
		/* cost as it is right now */
		cost = (distk = GetShortestDist(maze,maze->goals[goalj].loc,
			      maze->stones[stonej].loc)) + disti;
		/* cost after swap */
		distj = GetShortestDist(maze,maze->goals[goalj].loc,
                              maze->stones[stonei].loc);
		cost_swap
		     = GetShortestDist(maze,maze->goals[goali].loc,
                              maze->stones[stonej].loc) + distj;
		if (heur_diff > (cost_swap - cost)) {
			/* lower cost matching found */
			heur_diff = (cost_swap - cost);
			heur_stone = stonej;
			if (maze->h+heur_diff==min_h) break;
		} else if (  (cost_swap==cost)
			   &&(distk*disti!=0)
			   &&(dist_diff > ((int)distj-(int)disti))
			   &&(levels[(int)stonei] >= levels[(int)stonej])) {
				/* stonei can be matched to a closer goal */
				dist_diff = distj - disti;
				dist_stone = stonej;
		}
	    }
	    if (heur_diff<0) {
		/* decrease in heuristic value found */
	        maze->h += heur_diff;
		stonej = heur_stone;
	    } else if (dist_diff<0) {
		/* closer goal found for stone that was moved */
		stonej = dist_stone;
	    } else continue;
	    goalj = maze->lbtable[(int)stonej].goalidx;
	    maze->lbtable[(int)goali].stoneidx = stonej;
	    maze->lbtable[(int)goalj].stoneidx = stonei;
	    maze->lbtable[(int)stonei].goalidx = goalj;
	    maze->lbtable[(int)stonej].goalidx = goali;
	    disti = maze->lbtable[(int)stonei].distance;
	    maze->lbtable[(int)stonei].distance =
		GetShortestDist(maze,maze->goals[goalj].loc,
			maze->stones[stonei].loc);
	    distj = maze->lbtable[(int)stonej].distance;
	    maze->lbtable[(int)stonej].distance =
		GetShortestDist(maze,maze->goals[goali].loc,
			maze->stones[stonej].loc);
	    stack[next_in++]=stonej;
	    stack[next_in++]=stonei;
	    if (disti>maze->lbtable[(int)stonei].distance) levels[(int)stonei]++;
	    if (distj>maze->lbtable[(int)stonej].distance) levels[(int)stonej]++;
	    SR(Assert(next_in<MAXSTONES*MAXSTONES,"PlainMinMatch: Stack to small!\n"));
	} while (next_out < next_in && maze->h>min_h );
	SR(Assert(maze->h>=0,"PlainMinMatch: heuristc < 0!\n"));
	return(maze->h);
}

