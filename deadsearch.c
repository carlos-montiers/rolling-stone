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
MAZE *DeadMaze;
MAZE *DInsertMaze;

/* This stuff does not work yet since shadow stones are influencing the
 * decision about somehting being a deadlock or not, the result of the
 * DeadMove search depends on the position of the shadow stones, which it
 * should not. */

int DeadIsGoalNode(int g)
{
	if (IdaInfo->IdaMaze->number_stones == 0 || IdaInfo->IdaMaze->h==0) {
		IdaInfo->CurrentSolutionDepth=g;
		SR(Debug(3,0,"DeadIsGoalNode: no stones left\n"));
		return(1);
	}
	if (  (NumberBitsBS(IdaInfo->IdaMaze->no_reach)==0)
	    &&(IdaInfo->closest_confl != 0)) {
		IdaInfo->CurrentSolutionDepth=g;
		SR(Debug(3,0,"DeadIsGoalNode: all reachable\n"));
		return(1);
	}
	return(0);
}

/* Try to find out if we can move this stone still to a goal,
   include those that might be creating a deadlock,
   after finding that this is a deadlock, find the minimal set of
   stones belonging to a deadlock */
int  DeadMove(MAZE *maze, MOVE *last_move, int treedepth)
{
	BitString	visible,relevant;
	PHYSID		pos;
	IDA		idainfo,*old_idainfo;
	int32_t 	node_count;
	int		result;
	int		number_stones;
	unsigned short	old_gm;
	SR(int here_nodes = total_node_count);

	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                 = &idainfo;
	IdaInfo->IdaMaze        = UpdateMaze(maze,DeadMaze);
	IdaInfo->ThresholdInc   = 2;
	IdaInfo->AbortNodeCount = old_idainfo->pattern_node_limit;
	IdaInfo->pattern_node_limit = old_idainfo->pattern_node_limit
				     /PATTERNLIMITDIV;
	IdaInfo->goal_last_to   = last_move->to;
	IdaInfo->closest_confl  = 0;
	IdaInfo->base_indent    = old_idainfo->base_indent + treedepth;
	IdaInfo->PrintPriority  = DEADPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTableDead;
	old_gm = Options.mc_gm;
	Options.mc_gm=0;
	BitNotAndNotBS(IdaInfo->no_reach,maze->reach,maze->out);
	Set0BS(visible); Set0BS(relevant);
	SetBitBS(visible,last_move->to);
	node_count    = 0;
	number_stones = 1;

	SR(Debug(4,0,"DeadMove #### Start search\n"));
	for (;;) {
		SR(Debug(4,0,"DeadMove Iteration, stones: %i\n",
			number_stones));
		DeadDeactivateStones(IdaInfo->IdaMaze,visible);
		BitAndNotAndNotBS(IdaInfo->shadow_stones,
			maze->stone,visible,maze->goal);
		IdaInfo->node_count    = 0;
		result                 = DeadStartIda();
		node_count            += IdaInfo->node_count;
		if (AbortSearch()) {
			SR(Debug(4,0,"DeadMove: too many nodes: %i\n",
			      IdaInfo->node_count));
			goto END;
		}
		BitOrEqBS(relevant,IdaInfo->IdaManSquares);
		if (result>=ENDPATH) {
			DeadMiniConflict(YES);
			dl_pos_nc += IdaInfo->node_count;
			dl_pos_sc ++;
			IdaInfo = old_idainfo;
			SR(Debug(4,0,"DeadMove ## End search - DEAD (nodes:"));
			SR(Debug(4,0," %" PRIi32 ", stones: %i, result: %i)\n",
				node_count, number_stones, result));
			Options.mc_gm=old_gm;
			return(1);
		}
		/* Turn off all goal stones */
		/* BitAndNotButOrEqBS(IdaInfo->IdaStoneSquares,
				IdaInfo->IdaMaze->goal,
				IdaInfo->IdaMaze->stones_done); */
		BitAndNotEqBS(IdaInfo->IdaStoneSquares,IdaInfo->IdaMaze->goal);
		/* if a stone is blocked, try that block, else man blocks */
		pos = FindClosestPosStone(maze,
			IdaInfo->IdaStoneSquares, visible);
		if (pos > 0) SetBitBS(visible,pos);
		else {
			/* BitAndNotButOrEqBS(IdaInfo->IdaManSquares,
				IdaInfo->IdaMaze->goal,
				IdaInfo->IdaMaze->stones_done); */
			BitAndNotEqBS(IdaInfo->IdaManSquares,
				IdaInfo->IdaMaze->goal);
			pos = FindClosestPosMan(maze,
				IdaInfo->IdaManSquares, visible);
			if (pos > 0) SetBitBS(visible,pos);
			else {
				/* AddTestedDead(maze->conflicts,relevant,
			   		old_idainfo->IdaMaze->stone,
			   		IdaInfo->IdaMaze->manpos,last_move->to);
				*/
			   break;
			}
		}
		/* If we have too many stones in the works, abort */
		number_stones = NumberBitsBS(visible);
		if (number_stones>=maze->number_stones-2) {
			SR(Debug(4,0,"DeadMove: too many stones: %i\n",
			      number_stones));
			break;
		}
	}
END:
	dl_neg_nc += IdaInfo->node_count;
	dl_neg_sc ++;
	IdaInfo = old_idainfo;
	SR(Debug(4,0,"DeadMove ## End search - ALIVE (nodes: %" PRIi32 " stones: %i)\n",
		node_count,number_stones));
	Options.mc_gm=old_gm;
	return(0);
}

void DeadDeactivateStones(MAZE *maze, BitString visible)
{
	PHYSID    pos;

	maze->number_stones=0;
	CopyBS(maze->stone,visible);
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(visible,pos)) {
			maze->PHYSstone[ pos ] = maze->number_stones;
			maze->stones[maze->number_stones++].loc = pos;
		} else {
			maze->PHYSstone[ pos ] = -1;
		}
	}
	NormHashKey(maze);
	MarkReach(maze);
	DeadLowerBound(maze,ENDPATH);
}

int DeadMoveSuspected(MAZE *maze, MOVE *last_move)
{
/* return 1 if we suspect a deadlock search is beneficial, here should be
 * all the heuristic stuff that we hope will basically have a good guess at
 * the final outcome of the proove search - sort of like move ordering in
 * alpha-beta */

	int diff,odiff;
	BitString test;

	diff = last_move->to - last_move->last_over;
	if (diff==1 || diff==-1) odiff = YSIZE;
	else odiff = 1;

	/* check all 5 forward and side directions for stones and/or
	 * no_reach area */
	Set0BS(test);
	SetBitBS(test,last_move->to + diff);
	SetBitBS(test,last_move->to + diff + odiff);
	SetBitBS(test,last_move->to + diff - odiff);
	SetBitBS(test,last_move->to + odiff);
	SetBitBS(test,last_move->to - odiff);

	if (  LogAndBS(test,maze->stone)
	    ||LogAndBS(test,maze->no_reach)) {
		/*if (WasTestedDead(maze->conflicts,maze->stone,
				maze->reach,last_move->to))
			return(0);
		else */
			return(1);
	}

	return(0);

}

PHYSID FindClosestPosStone(MAZE *maze, BitString squares,
		      		  BitString already_visible)
/* Find the square (position) the in squares that has a stone on it
 * that is not already visible */
{
	int    stonei;
	DIST   dist = MAXDIST;
	PHYSID pos  = 0,p;

	for (stonei=0; stonei<maze->number_stones; stonei++) {
		p = maze->stones[stonei].loc;
		if (  !IsBitSetBS(already_visible,p)
		    && IsBitSetBS(squares,p)) {
                        if (  dist > XDistStone(maze,maze->manpos,p)) {
                                dist = XDistStone(maze,maze->manpos,p);
				pos  = p;
			}
		}
	}
	return(pos);
}

PHYSID FindClosestPosMan(MAZE *maze, BitString squares,
		      		  BitString already_visible)
/* Find the square (position) the in squares that has a stone on it
 * that is not already visible */
{
	int    stonei;
	DIST   dist = MAXDIST;
	PHYSID pos  = 0,p;

	for (stonei=0; stonei<maze->number_stones; stonei++) {
		p = maze->stones[stonei].loc;
		if (  !IsBitSetBS(already_visible,p)
		    && IsBitSetBS(squares,p)) {
			if (  dist > XDistMan(maze,maze->manpos,p)) {
				dist = XDistMan(maze,maze->manpos,p);
				pos  = p;
			}
		}
	}
	return(pos);
}

void DeadMiniConflict(int minimize)
/* stones marked in visible are a deadlock set, try and minimize the set by
 * removing one stone at a time, finding its necessity in the deadlock */
/* Make sure all stones that were initially on are when we get out */
{
	int        r,result;
	unsigned short old_gm;
	IDA	   idainfo, *old_idainfo;
	int32_t    node_count;
	PHYSID     pos;
	SAVEMAZE   savemaze;
	BitString  already, visible;
	SR(int here_nodes = total_node_count);

	/* we get called from searches when we meet 0 move positions */
	if (Options.dl_srch == NO) return;

	SR(Debug(4,0,"DeadMiniConflict #### Start\n"));

	if (Options.minimize == YES && minimize == YES) {
		CopyBS(visible,IdaInfo->IdaMaze->stone);
		old_idainfo = IdaInfo;
		InitIDA(&idainfo);
		IdaInfo                 = &idainfo;
		IdaInfo->IdaMaze        = SaveMaze(old_idainfo->IdaMaze,
						   &savemaze);
		CopyBS(IdaInfo->no_reach,old_idainfo->no_reach);
		IdaInfo->ThresholdInc   = 2;
		IdaInfo->AbortNodeCount = old_idainfo->pattern_node_limit;
		IdaInfo->pattern_node_limit = old_idainfo->pattern_node_limit
						/PATTERNLIMITDIV;
		IdaInfo->goal_last_to   = old_idainfo->goal_last_to;
		IdaInfo->closest_confl  = 0;
		IdaInfo->base_indent   += 2;
		IdaInfo->PrintPriority  = DEADPATTERNSEARCHPP;
		IdaInfo->HashTable      = HashTableDead;
		IdaInfo->MiniFlag	= YES;
		old_gm        = Options.mc_gm;
		Options.mc_gm = 0;
		node_count    = 0;
		Set0BS(already);
		for (;;) {
			pos = FindFarthestPosStone(IdaInfo->IdaMaze,visible,already);
			if (pos==0) break;
			SetBitBS(already,pos);
			SR(Debug(4,0,"DeadMiniConflict ### Start iteration\n"));
			UnsetBitBS(visible,pos);
			DeadDeactivateStones(IdaInfo->IdaMaze,visible);
			node_count += IdaInfo->node_count;
			IdaInfo->node_count = 0;
			result = DeadStartIda();
			mini_node_count += IdaInfo->node_count;
			if (result<ENDPATH) {
				if( result > IdaInfo->IdaMaze->pen ) {
					IdaInfo->ThresholdInc   = 1;
					r = PenStartIda();
					/* subtract one because of the ThresholdInc==2 */
					result = max(r,result-1);
					PenMiniConflict(result, YES);
					IdaInfo->ThresholdInc   = 2;
				}
				SetBitBS(visible,pos);
			  SR(}else{Debug(4,0,"DeadMiniConflict Extra Stone\n"));
			}
		}

		DeadDeactivateStones(IdaInfo->IdaMaze,visible);
		AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
			IdaInfo->IdaMaze->no_reach, ENDPATH);
		Options.mc_gm=old_gm;
		RestoreMaze(IdaInfo->IdaMaze,&savemaze);
		IdaInfo = old_idainfo;
		IdaInfo->node_count += node_count;
	} else { /*if minimization */
		AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
			IdaInfo->IdaMaze->no_reach, ENDPATH);
	}

	SR(Debug(4,0,"DeadMiniConflict #### End\n"));
}

int DeadStartIda()
/* Sets up all data structures and repeatedly calls ida with increasing
   threshold to guarantee optimal solutions, returns 0 if solution found
   otherwise the increase of maze->h by Threshold, if this is
   ENDPATH there is no solution - deadlock */
{

	int       result=ENDPATH;
	SR(int here_nodes = total_node_count);
	if (AbortSearch()) return(0);
	/* initialize data structures */
	AvoidThisSquare = 0;
	init_stats();
	Set0BS(IdaInfo->IdaManSquares);
	Set0BS(IdaInfo->IdaStoneSquares);
	IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	IdaInfo->CurrentSolutionDepth = ENDPATH;
	BitNotAndNotBS(IdaInfo->no_reach,IdaInfo->IdaMaze->reach,
					 IdaInfo->IdaMaze->out);
	GetPenalty(IdaInfo->IdaMaze,ENDPATH);
	for (IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	       (IdaInfo->CurrentSolutionDepth > IdaInfo->Threshold)
	     &&(result != 0)
	     &&(!AbortSearch());
	     IdaInfo->Threshold += IdaInfo->ThresholdInc) {
		GetPenalty(IdaInfo->IdaMaze,ENDPATH);
		SR(Debug(2,0,"DeadStartIda: Threshold %i (%i) [%i]\n",
			IdaInfo->Threshold,
			IdaInfo->IdaMaze->h,IdaInfo->node_count));
		GTVAny(GTVOpen(IdaInfo->Threshold,
			       GTVFen(IdaInfo->IdaMaze)));
		IdaInfo->IdaMaze->goal_sqto = -1;
		result = DeadIda(0,0); /**********************************/
		GTVAny(GTVClose());
		print_stats(4);
		if (result>=ENDPATH) IdaInfo->Threshold = ENDPATH;
	}
	if (  (IdaInfo->CurrentSolutionDepth<ENDPATH)
	    ||(result==0)
	    ||(AbortSearch()))
		IdaInfo->Threshold -= IdaInfo->ThresholdInc;
	if (result<ENDPATH) result = IdaInfo->Threshold - IdaInfo->IdaMaze->h;
	return(result);
}

static int DeadCompare(const void *m1, const void *m2) {
        return(((MOVE*)m1)->value - ((MOVE*)m2)->value);
}

int DeadMoveOrdering(int depth, int number_moves)
{
        int  i,diff;
        IDAARRAY *S = &(IdaInfo->IdaArray[depth]);
        MAZE *maze = IdaInfo->IdaMaze;
        MOVE *m,*lmove;
        PHYSID goalpos;

        if (number_moves>1) {
                lmove = depth?&(IdaInfo->IdaArray[depth-1].currentmove)
                                :&DummyMove;
                for (i=0; i<number_moves; i++) {
                        m = &(S->moves[i]);
                        if (ISDUMMYMOVE(*m)) {
                                m->value = -ENDPATH;
                                continue;
                        }
                        goalpos = maze->goals[
                                    maze->lbtable[
                                      maze->PHYSstone[m->from]].goalidx].loc;
                        m->value =
                              maze->lbtable[maze->PHYSstone[m->from]].distance;
                        if (m->macro_id != 4) diff = m->value -
                              StoneDistManpos(maze,m->to,goalpos,m->from);
                        else diff = m->value;
                        if (diff>0) {
                                if (lmove->to == m->from)
                                        m->value -= ENDPATH;
                        } else {
                                m->value -= diff*100;
                        }
                }
                My_qsort(&(S->moves),number_moves,sizeof(MOVE),DeadCompare);
        }
        return(number_moves);
}


int DeadIda(int treedepth, int g) {
/* the procedure that does the work at one node. it returns
	X - the smallest h underneath this node */

	IDAARRAY  *S;
	HASHENTRY *entry;
	int 	   min_h,number_moves,result,i;
	int        dir, targetpen;


	SR(GMNODE **old_gmnode = IdaInfo->IdaMaze->gmtrees);
	SR(int here_nodes = total_node_count);
	SR(int old_h = IdaInfo->IdaMaze->h- IdaInfo->IdaMaze->pen);
/*PrintMaze(IdaInfo->IdaMaze, true);*/

        SR(Debug(5,treedepth,"starting ida (h=%i) (%s) %" PRIi32 " %" PRIx64 "\n",
		IdaInfo->IdaMaze->h,
          	treedepth==0?"a1a1"
		      :PrintMove(IdaInfo->IdaArray[treedepth-1].currentmove),
		total_node_count,IdaInfo->IdaMaze->hashkey));
	GTVAny(GTVNodeEnter(treedepth,g,0,GTVMove(treedepth
		?IdaInfo->IdaArray[treedepth-1].currentmove:DummyMove),0));
	if (AbortSearch()) {
		GTVAny(GTVNodeExit(treedepth,0,"Abort_Search"));
		return(IdaInfo->IdaMaze->h);
	}
	SR(if (treedepth >=MAX_DEPTH) {
		SR(Debug(5,treedepth,"Too deep in the tree!(%i)\n",treedepth));
		GTVAny(GTVNodeExit(treedepth,0,"Too_deep_in_tree"));
		return(IdaInfo->IdaMaze->h);
	})

	S = &(IdaInfo->IdaArray[treedepth]);
	IncNodeCount(treedepth);

	/* The following order is important, otherwise we might find a goal
	   an iteration earlier! */
	/* check for cutoff: is g+h > threshold => return(0) (fail) */
	if (g+IdaInfo->IdaMaze->h>IdaInfo->Threshold) {
		SR(Debug(5,treedepth,"Threshold cutoff (%i=%i+%i)\n",
			g+IdaInfo->IdaMaze->h,g,IdaInfo->IdaMaze->h));
		GTVAny(GTVNodeExit(treedepth,
				   IdaInfo->IdaMaze->h,"Threshold_Cutoff"));
		return(IdaInfo->IdaMaze->h);
	}
	/* check for goal state, if yes return(1) */
	if (DeadIsGoalNode(g)) {
		SR(Debug(5,treedepth,"Found goal (%i %i)******************\n",
			IdaInfo->IdaMaze->h, IdaInfo->IdaMaze->number_stones));
		GTVAny(GTVNodeExit(treedepth,0,"Goal_found"));
		return(0);
	}

	/* check trans table if searched already */
	entry = PSGetHashTable(IdaInfo->IdaMaze);
	if (entry != NULL) {
		if (entry->pathflag == 1) {
			SR(Debug(5,treedepth, "Cycle (TT)\n"));
			GTVAny(GTVNodeExit(treedepth, ENDPATH,"Cycle (TT)"));
			IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
			return(IdaInfo->IdaMaze->h);
		}
		if ((IdaInfo->Threshold - g) < (int)entry->down) {
			/* This entry was searched deeper than we need now,
			 * if no goal was found we won't either, cut */
			if (entry->min_h != 0) {
				SR(Debug(5,treedepth, "Futil (TT) %i < %i\n",
					IdaInfo->Threshold-g,entry->down));
				GTVAny(GTVNodeExit(treedepth,
					ENDPATH,"Futil (TT)"));
				IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
				return(entry->min_h);
			}
		} else if (IdaInfo->Threshold - g == entry->down) {
			/* This node was already searched as deep as we
			 * want. We need to make sure we find a conflict if
			 * not in Minimization. If we have a conflict
			 * already, just quit. */
			if (   entry->min_h != 0
			    || IdaInfo->closest_confl != 0
			    || IdaInfo->MiniFlag==YES) {
				SR(Debug(5,treedepth, "Futil (TT) %i == %i\n",
					IdaInfo->Threshold-g,entry->down));
				GTVAny(GTVNodeExit(treedepth,
					ENDPATH,"Futil (TT)"));
				IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
				return(entry->min_h);
			}
		} else {
			/* This node was searched shallower than we need,
			 * only if there was a goal. Can we ever get here? */
			if (entry->min_h==0 && IdaInfo->MiniFlag==YES) {
				SR(Debug(5,treedepth,
					"Futil (TT) %i > %i (0!!)\n",
					IdaInfo->Threshold-g,entry->down));
				GTVAny(GTVNodeExit(treedepth,
					ENDPATH,"Futil (TT)"));
				IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
				return(0);
			}
		}
		PSSetPathFlag(IdaInfo->IdaMaze);
	} else {
		PSStoreHashTable(IdaInfo->IdaMaze,0,IdaInfo->IdaMaze->h,1);
	}
	min_h    = ENDPATH;

	number_moves = GenerateMoves(IdaInfo->IdaMaze,&(S->moves[0]));
	number_moves = DeadMoveOrdering(treedepth,number_moves);
	if (number_moves==0 && treedepth!=0) DeadMiniConflict(YES);

	/* foreach move call ida */
	targetpen = (  (IdaInfo->Threshold - g)
		     - (IdaInfo->IdaMaze->h - IdaInfo->IdaMaze->pen)) + 2;
	for (i=0; i<number_moves; i++) {
		if (ISDUMMYMOVE(S->moves[i])) continue;
		if (IdaInfo->IdaMaze->goal_sqto!=-1) {
			SR(Debug(5,treedepth,"Goal Cut move\n"));
			continue;
		}
		S->currentmove = S->moves[i];
		/* check for deadlock */
		dir = DiffToDir(S->currentmove.to-S->currentmove.last_over);
		if (dir!=NODIR) {
			if (DeadTree(IdaInfo->IdaMaze,S->currentmove.to,dir)) {
				SR(Debug(6,treedepth,
					"DeadTree fd deadlock (%i-%i)\n",
					S->currentmove.from,
					S->currentmove.to));
				continue;
			}
		}
		SR(Debug(6,treedepth,"DeadMakeMove %s (%i of %i)\n",
			PrintMove(S->currentmove),i+1,number_moves));
		if (!DeadMakeMove(IdaInfo->IdaMaze,&(S->currentmove),
				  &(S->unmove),targetpen)){
			continue;
		}
		result = DeadIda(treedepth+1,g+S->unmove.move_dist);
		DeadUnMakeMove(IdaInfo->IdaMaze,&(S->unmove),targetpen);
		SR(Assert(old_h==IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen,
			"DeadUnMakeMove: old_h!=h!\n"));
		SR(Assert(old_gmnode==IdaInfo->IdaMaze->gmtrees,
			"DeadIda: old_gmtrees[0] changed!!\n"));
		if (result < min_h) {
			min_h = result;
		}
		/* one solution found? */
		if (result == 0) {
			S->solution = S->currentmove;
			SetManStoneSquares(IdaInfo->IdaMaze,S->currentmove);
			goto END_IDA;
		}
	}

END_IDA:
	/* write Transposition table */
	if (!AbortSearch() || min_h == 0)
		PSStoreHashTable(IdaInfo->IdaMaze,IdaInfo->Threshold-g,min_h,0);
	else
		PSUnSetPathFlag(IdaInfo->IdaMaze);
	SR(Debug(5,treedepth,"return from ida %i (%i)\n", treedepth, min_h));
	GTVAny(GTVNodeExit(treedepth,min_h,"Normal_Exit"));
	return(min_h);
}

int DeadMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret, int targetpen)
/* this is a routine that makes a STONE move, not merely a man move like
 * DoMove. It will just put the man to the new location */
{
	int old_h;

	ret->manfrom  = maze->manpos;
	ret->stonefrom= move->from;
	ret->stoneto  = move->to;
	ret->macro_id = move->macro_id;
	ret->move_dist= StoneDist(maze,move->from,move->to);
	maze->goal_sqto = -1;

	CopyBS( ret->save_reach, maze->reach );
	CopyBS( ret->save_no_reach, maze->no_reach );

	MANTO(maze,move->last_over);
	STONEFROMTO(maze,ret->stonefrom,ret->stoneto);

	ret->old_closest_confl = IdaInfo->closest_confl;
	if (IsBitSetBS(IdaInfo->shadow_stones,move->to)) {
		if ( ManDist(maze,IdaInfo->goal_last_to,move->to)
		    <ManDist(maze,IdaInfo->goal_last_to,
				    IdaInfo->closest_confl))
			IdaInfo->closest_confl = move->to;
	}
	/* either touching a goal or an area unreachable before, removes
	 * stone */
	if (  (maze->Phys[move->to].goal>=0)
	    ||(  !IsBitSetBS(IdaInfo->no_reach,ret->stoneto)
	       && IdaInfo->closest_confl != 0)) {
		/* remove stone */
		ret->old_stoneid = maze->PHYSstone[ret->stoneto];
		maze->PHYSstone[ret->stoneto] = -1;
		maze->number_stones--;
		if (maze->number_stones>ret->old_stoneid) {
			/* relocate stone if necessary */
			maze->stones[ret->old_stoneid]
				= maze->stones[maze->number_stones];
			maze->PHYSstone[maze->stones[ret->old_stoneid].loc]
				= ret->old_stoneid;
		}
		UnsetBitBS(maze->stone,ret->stoneto);
		NormHashKey(maze);
		CopyBS(maze->old_no_reach,maze->no_reach);
		MarkReachQuick( maze, move->from );

		/* make sure we deepen the tree to cut off bad moves later */
		old_h = maze->h;
		DeadLowerBound(maze,targetpen);
		ret->move_dist = old_h - maze->h;
	} else {
		UpdateHashKey(maze, ret);
		CopyBS(maze->old_no_reach,maze->no_reach);
		MarkReach(maze);
		DeadUpdateLowerBound(maze, ret->stoneto, targetpen);
		ret->old_stoneid = -1;
	}
	SR(Assert(maze->manpos>0,"DeadMakeMove: manpos < 0!\n"));
	return(1);
}

int DeadUnMakeMove(MAZE *maze, UNMOVE *unmove, int targetpen)
{
	int	 new_h;

	new_h = maze->h;
	if (unmove->old_stoneid != -1) {
		/* recreate stone */
		if (maze->number_stones>unmove->old_stoneid) {
			/* relocate stone, if was before */
			maze->stones[maze->number_stones]
				= maze->stones[unmove->old_stoneid];
			maze->PHYSstone[maze->stones[unmove->old_stoneid].loc]
				= maze->number_stones;
		}
		maze->number_stones++;
		maze->stones[unmove->old_stoneid].loc = unmove->stoneto;
		maze->PHYSstone[unmove->stoneto] = unmove->old_stoneid;
		SetBitBS(maze->stone,unmove->stoneto);
	}
	MANTO(maze,unmove->manfrom);
	STONEFROMTO(maze,unmove->stoneto,unmove->stonefrom);

	CopyBS( maze->reach, unmove->save_reach );
	CopyBS( maze->no_reach, unmove->save_no_reach );
	if (unmove->old_stoneid == -1) {
		UpdateHashKey(maze, unmove);
		DeadUpdateLowerBound(maze, unmove->stonefrom, targetpen);
	} else {
		NormHashKey(maze);
		DeadLowerBound(maze,targetpen);
	}
	IdaInfo->closest_confl = unmove->old_closest_confl;

	if (  ((unmove->old_stoneid != -1)||(maze->goal_sqto==unmove->stoneto))
	    &&(maze->h - unmove->move_dist == new_h    /* optimal move */)) {
		/* This is either the start or a continuation of a goal move */
		maze->goal_sqto = unmove->stonefrom;
	} else maze->goal_sqto = -1;

	return(0);
}

int DeadLowerBound(MAZE *maze, int targetpen) {
/* This is a fast and bad lower bound - just the closest goal - since we
remove stones when we hit goals */
	char   stonei,goali;
	PHYSID stonepos, goalpos;
	DIST   dist;

	/* initialize table with just any matching */
	maze->pen = maze->h = 0;
	GetPenalty(maze,targetpen);
	for (stonei=0; stonei<maze->number_stones; stonei++) {
		stonepos = maze->stones[stonei].loc;
		if (IsBitSetBS(maze->stones_done,stonepos)) {
			maze->lbtable[stonei].distance = 0;
			maze->lbtable[stonei].goalidx
				= maze->Phys[stonepos].goal;
			continue;
		}
		maze->lbtable[stonei].distance = MAXDIST;
		for (goali=0; goali<maze->number_goals; goali++) {
			goalpos = maze->goals[goali].loc;
			if (   stonepos!=goalpos
			    && IsBitSetBS(maze->stone,goalpos))
				continue;
			dist = StoneDist(maze,stonepos,goalpos);
			if (dist < maze->lbtable[stonei].distance) {
				maze->lbtable[stonei].distance=dist;
				maze->lbtable[stonei].goalidx=goali;
			}
		}
		maze->h += maze->lbtable[stonei].distance;
	}
	return(maze->h);
}

int DeadUpdateLowerBound(MAZE *maze, PHYSID stonepos, int targetpen) {
/* Update lowerbound after move to square pos */
	char   stonei,goali;
	PHYSID goalpos;
	DIST   dist;

	GetPenalty(maze,targetpen);
	stonei   = maze->PHYSstone[stonepos];
	maze->h -= maze->lbtable[stonei].distance;
	maze->lbtable[stonei].distance = MAXDIST;
	for (goali=0; goali<maze->number_goals; goali++) {
		goalpos = maze->goals[goali].loc;
		if (   stonepos!=goalpos
		    && IsBitSetBS(maze->stone,goalpos))
			continue;
		dist = StoneDist(maze,stonepos,goalpos);
		if (dist < maze->lbtable[stonei].distance) {
			maze->lbtable[stonei].distance=dist;
			maze->lbtable[stonei].goalidx=goali;
		}
	}
	maze->h += maze->lbtable[stonei].distance;
	return(maze->h);
}

