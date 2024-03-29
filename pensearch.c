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
MAZE *PenMaze;
MAZE *InsertMaze;
MAZE *AreaMaze;

/* This stuff does not work yet since shadow stones are influencing the
 * decision about somehting being a deadlock or not, the result of the
 * PenMove search depends on the position of the shadow stones, which it
 * should not. */

int PenIsGoalNode(int g)
{
	if (IdaInfo->IdaMaze->number_stones == 0 || IdaInfo->IdaMaze->h == 0) {
		IdaInfo->CurrentSolutionDepth=g;
		SR(Debug(3,0,"PenIsGoalNode: no stones left\n"));
		return(1);
	}
	return(0);
}

void MarkReachPos(MAZE *maze, BitString reach, PHYSID manpos, int clear)
{
  /* recursive function to mark the fields that are reachable */

  static PHYSID stack[ENDPATH];
  PHYSID pos;
  int top;

  if (clear==YES) Set0BS(reach);

  stack[0] = manpos;
  top = 1;
  while( top ) {
    pos = stack[ --top ];
    if( IsBitSetBS( reach, pos) ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;

    SetBitBS( reach,pos );

    if( IsBitSetBS( maze->M[ 0 ], pos ) )
      stack[ top++ ] = pos + 1;
    if( IsBitSetBS( maze->M[ 1 ], pos ) )
      stack[ top++ ] = pos + YSIZE;
    if( IsBitSetBS( maze->M[ 2 ], pos ) )
      stack[ top++ ] = pos - 1;
    if( IsBitSetBS( maze->M[ 3 ], pos ) )
      stack[ top++ ] = pos - YSIZE;
  }
}

int FindFringeStones( MAZE *maze, BitString fs,
		      BitString no_reach, PHYSID move_to )
/* returns number of stones surrounding area */
/* ACHTUNG: watch out that we are not including stones on goal squares,
   PenSearch is not equiped to handle those! */
{
  int dir,marked;
  PHYSID pos;

  /* mark the area that is adjacent to the current move_to pos that is not
   * accessible to the man currently */
  Set0BS( no_reach );
  marked = 0;
  for (dir=0; dir<=NORTHWEST; dir++) {
      pos = move_to + DirToDiff[dir];
      if (   IsBitSetBS(maze->no_reach, pos) /* not reachable && not stone */
	  && !IsBitSetBS(no_reach, pos)) {   /* and not already included */
	MarkReachPos(maze, no_reach, pos, NO);
	marked++;
      }
  }
  if (marked == 0) return(0);		     /* no inaccesss area close by */

  /* now find the stones that suround that area and return that number */
  return(WhichStones(maze,fs,no_reach,YES));
}

int WhichStones(MAZE *maze, BitString fs, BitString no_reach, int clear)
/* add all the stones to fs that are touching no_reach area AND not
 * area, or are neighbour to one already part of the pattern, until no more
 * stones can be added */
{
  int s,dir,pos,pos_target,bool_no_reach,bool_fs,bool_else,this_marked;
  int marked = 0;
  if (clear==YES) Set0BS(fs);
  do {
      this_marked = 0;
      for (s=0; s<maze->number_stones; s++) {
	pos = maze->stones[s].loc;
	if (IsBitSetBS(fs,pos)) continue;
	if (IsBitSetBS(maze->goal,pos)) continue;
	    /* &&!IsBitSetBS(maze->stones_done,pos)) continue; */
	bool_no_reach = bool_fs = bool_else = NO;
	for (dir=0; dir<=NORTHWEST; dir++) {
	    pos_target = pos+DirToDiff[dir];
	    if (IsBitSetBS(fs, pos_target))		bool_fs       = YES;
	    else if (IsBitSetBS(no_reach, pos_target))	bool_no_reach = YES;
	    else if (  !IsBitSetBS(maze->stone, pos_target)
		     &&!IsBitSetBS(maze->wall, pos_target)) bool_else     = YES;
	    if (   bool_no_reach==YES
		&& (   bool_else==YES
		    || bool_fs==YES)) {
		SetBitBS(fs,pos);
		this_marked++;
		break;
	    }
	}
      }
      marked += this_marked;
  } while (this_marked);
  return(marked);
}

int AddMoreStones( MAZE *maze, BitString fs, BitString no_reach)
/* assuming fs contains the fringe stones and no_reach the currently selected
 * non_reachable area, try to add more non-reachable area (stones surounding
 * it. Return YES if successfully added, no if no such area exists */
{
  int    s,dir;
  PHYSID pos,pos_target;

  for (s=0; s<maze->number_stones; s++) {
      pos = maze->stones[s].loc;
      if (IsBitSetBS(maze->goal,pos)) continue;
	  /* &&!IsBitSetBS(maze->stones_done,pos)) continue; */
      if (IsBitSetBS(fs,pos)) {
	  for (dir=0; dir<=NORTHWEST; dir++) {
	      pos_target = pos + DirToDiff[dir];
	      /* if something is not reachable and not yet part of area */
	      if (   IsBitSetBS(maze->no_reach, pos_target)
		  && !IsBitSetBS(no_reach, pos_target)) {
		  MarkReachPos(maze, no_reach, pos_target, NO);
		  /* add all the stones and return with success */
		  WhichStones(maze,fs,no_reach,NO);
		  return(YES);
	      }
	  }
      }
  }
  return(NO);
}

int  AreaMove(MAZE *maze, MOVE *last_move, int treedepth, int targetpen)
/* Return True (YES) if targetpen was achieved */
{
	BitString	visible,no_reach,relevant;
	IDA		idainfo,*old_idainfo;
	int32_t		node_count;
	int		result;
	int		number_stones,old_number_stones;
	short		old_gm;
	int		max_pen;
	SR(int here_nodes = total_node_count;)

	Set0BS(visible);
	Set0BS(no_reach);
        number_stones = FindFringeStones(maze,visible,no_reach,last_move->to);
	if (number_stones==0) return(0);
	if (number_stones > maze->number_stones*2/3) return(0);

	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                 = &idainfo;
	IdaInfo->IdaMaze        = UpdateMaze(maze,AreaMaze);
	IdaInfo->AbortNodeCount = old_idainfo->pattern_node_limit;
	IdaInfo->pattern_node_limit = old_idainfo->pattern_node_limit
				     /PATTERNLIMITDIV;
	IdaInfo->goal_last_to   = last_move->to;
	IdaInfo->base_indent    = old_idainfo->base_indent + treedepth;
	IdaInfo->PrintPriority  = PENPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTablePen;

	IdaInfo->ThresholdInc   = 1;
	old_gm = Options.mc_gm;
	Options.mc_gm=0;
	Set1BS(IdaInfo->no_reach);
	Set0BS(relevant);
	SetBitBS(visible,last_move->to);
	node_count    = 0;
	max_pen	      = 0;

	SR(Debug(4,0,"AreaMove #### Start search\n"));
	for (;;) {
		SR(Debug(4,0,"AreaMove Iteration, stones: %i\n",number_stones));

		PenDeactivateStones(IdaInfo->IdaMaze,visible,targetpen);
		IdaInfo->node_count    = 0;
		result                 = DeadStartIda();
		BitOrEqBS(relevant,IdaInfo->IdaManSquares);
		if (result > IdaInfo->IdaMaze->pen) {
			if (max_pen<result) max_pen = result;
			PenMiniConflict(result,YES);
			node_count += IdaInfo->node_count;
			IdaInfo->node_count = 0;
		}
		if (AbortSearch()) {
			SR(Debug(4,0,"AreaMove: too many nodes: %i\n",
			      IdaInfo->node_count));
			goto END;
		}
		old_number_stones = number_stones;
	        number_stones = AddMoreStones(maze,visible,no_reach);
		if (old_number_stones >= number_stones) break;
		if (number_stones>=maze->number_stones-2) {
			SR(Debug(4,0,"AreaMove: too many stones: %i\n",
			      number_stones));
			break;
		}
	}
/* 	if (  number_stones>0) AddTestedPen(maze->conflicts,relevant,
				   old_idainfo->IdaMaze->stone,
				   IdaInfo->IdaMaze->manpos,last_move->to);
*/
END:
	node_count += IdaInfo->node_count;
	IdaInfo->node_count = 0;
	if (max_pen >= targetpen) {
		area_pos_nc += node_count;
		area_pos_sc ++;
	} else {
		area_neg_nc += node_count;
		area_neg_sc ++;
	}
	IdaInfo = old_idainfo;
	SR(Debug(4,0,"AreaMove ## End search - ALIVE (nodes: %" PRIi32 " stones: %i)\n",
		node_count,number_stones));
	Options.mc_gm=old_gm;
        GetPenalty(maze,targetpen);
        return(maze->pen >= targetpen);
}

/* Try to find out if we can move this stone still to a goal,
   include those that might be creating a deadlock,
   after finding that this is a deadlock, find the minimal set of
   stones belonging to a deadlock */
/* Return True (YES) if targetpen was achieved */

int ScanSearch(MAZE *maze)
/* At the beginning of the search, go and test for pre-existing penalties.
 * That should increase the initial lower bound and remove early iterations. */
/* As a welcome sideeffect we might find partially independend sts of stones
 * that allow us to use super macros */
{
	int        i,old_h;
	PHYSID     pos;
	MOVE	   fake_move;
	HASHENTRY *entry;

	if (Options.scan_srch==NO) return(0);
	old_h = IdaInfo->IdaMaze->h;
	entry = GetHashTable(maze);
	if (entry == NULL) {
		entry = StoreHashTable(IdaInfo->IdaMaze,0,
			IdaInfo->IdaMaze->h,1,1,1,0);
	}
	for (i=0; i<maze->number_stones; i++) {
		Mprintf(0,"ScanSearch: h %i, nodes used: %" PRIi32 "\n",
			IdaInfo->IdaMaze->h, total_node_count);
		pos = maze->stones[i].loc;
		/*if (IsBitSetBS(maze->stones_done,pos)) continue; */
		fake_move.to = pos;
		PenMove(maze,entry,&fake_move,0,ENDPATH);
	}
	Mprintf(0,"ScanSearch: h: %i->%i, nodes used: %" PRIi32 "\n",
		old_h, IdaInfo->IdaMaze->h, total_node_count);
	return(0);
}


/* ALL PEANLTIES ARE TOTAL PENALTIES, not relative to previous knowledge */
int  PenMove(MAZE *maze, HASHENTRY *entry, MOVE *last_move, int treedepth,
	int targetpen)
{
	BitString	visible,relevant;
	PHYSID		pos;
	IDA		idainfo,*old_idainfo;
	int32_t		node_count;
	int		result;
	int		number_stones;
	short		old_gm;
	int		max_pen;
	SR(int here_nodes = total_node_count;)

	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                = &idainfo;
	IdaInfo->IdaMaze        = UpdateMaze(maze,PenMaze);
	IdaInfo->AbortNodeCount = old_idainfo->pattern_node_limit;
	IdaInfo->pattern_node_limit = old_idainfo->pattern_node_limit
				     /PATTERNLIMITDIV;
	IdaInfo->goal_last_to   = last_move->to;
	IdaInfo->base_indent    = old_idainfo->base_indent + treedepth;
	IdaInfo->PrintPriority  = PENPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTablePen;
	IdaInfo->ThresholdInc   = 1;
	old_gm = Options.mc_gm;
	Options.mc_gm=0;
	Set1BS(IdaInfo->no_reach);
	Set0BS(visible); Set0BS(relevant);
	SetBitBS(visible,last_move->to);
	node_count    = 0;
	number_stones = 1;
	max_pen	      = 0;

	SR(Debug(4,0,"PenMove #### Start search\n"));
	for (;;) {
		SR(Debug(4,0,"PenMove Iteration, stones: %i\n", number_stones));
		PenDeactivateStones(IdaInfo->IdaMaze,visible,targetpen);
		BitAndNotAndNotBS(IdaInfo->shadow_stones,
			maze->stone,visible,maze->goal);
		IdaInfo->node_count    = 0;
		result                 = PenStartIda();
		BitOrEqBS(relevant,IdaInfo->IdaManSquares);
		if (result > IdaInfo->IdaMaze->pen) {
			if (max_pen<result) max_pen = result;
			PenMiniConflict(result,YES);
			node_count += IdaInfo->node_count;
			IdaInfo->node_count = 0;
		}

		if (AbortSearch()) {
			SR(Debug(4,0,"PenMove: too many nodes: %i\n",
			      IdaInfo->node_count));
			goto END;
		}
		/* Turn off all goal stones */
		BitAndNotEqBS(IdaInfo->IdaStoneSquares,IdaInfo->IdaMaze->goal);
		/* if a stone is blocked, try that block, else man blocks */
		pos = FindClosestPosStone(maze,
			IdaInfo->IdaStoneSquares, visible);
		if (pos > 0) SetBitBS(visible,pos);
		else {
			BitAndNotEqBS(IdaInfo->IdaManSquares,
				IdaInfo->IdaMaze->goal);
			pos = FindClosestPosMan(maze,
				IdaInfo->IdaManSquares, visible);
			if (pos > 0) SetBitBS(visible,pos);
			else {
				/* No more stones in the solution path */
				/* and goal areas with one entrance */
			    if (Isnt0BS(IdaInfo->IdaManSquares)) {
				AddTestedPen(maze->conflicts,
					IdaInfo->IdaManSquares,
			   		old_idainfo->IdaMaze->stone,
			   		IdaInfo->IdaMaze->manpos,last_move->to,
					GoodTested(IdaInfo->IdaMaze));
				/* make sure no other deadlock searches are performed */
				entry->pensearched = 1;
				entry->dlsearched = 1;
			    }
			    break;
			}
		}
		/* If we have too many stones in the works, abort */
		number_stones = NumberBitsBS(visible);
		if (number_stones>=maze->number_stones-2) {
			SR(Debug(4,0,"PenMove: too many stones: %i\n",
			      number_stones));
			break;
		}
	}
END:
	node_count += IdaInfo->node_count;
	IdaInfo->node_count = 0;
	if (max_pen >= targetpen) {
		pen_pos_nc += node_count;
		pen_pos_sc ++;
	} else {
		pen_neg_nc += node_count;
		pen_neg_sc ++;
	}
	IdaInfo = old_idainfo;
	SR(Debug(4,0,"PenMove ## End search - ALIVE (nodes: %" PRIi32 " stones: %i)\n",
		node_count,number_stones));
	Options.mc_gm=old_gm;
        GetPenalty(maze,targetpen);
        return(maze->pen >= targetpen);
}

int GoodTested(MAZE *maze)
/* Goal area used to push stones to is good: only one entrance and a goal
 * area with macros! */
{
	int i,goalloc,groomi;

	for (i=0; i<maze->number_stones; i++) {
		goalloc =  maze->goals[maze->lbtable[i].goalidx].loc;
		groomi  = maze->groom_index[goalloc];
		if (groomi < 0)
			return(0);
		else {
			if (maze->grooms[groomi].n>1) return(0);
		}
	}
	return(1);
}

void PenDeactivateStones(MAZE *maze, BitString visible, int targetpen)
{
	PHYSID    pos;

	maze->number_stones=0;
	CopyBS(maze->stone,visible);
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(visible,pos)) {
			maze->PHYSstone[pos] = maze->number_stones;
			maze->stones[maze->number_stones++].loc = pos;
		} else {
			maze->PHYSstone[pos] = -1;
		}
	}
	NormHashKey(maze);
	MarkReach(maze);
	PenLowerBound(maze,targetpen);
}

int PenMoveSuspected(MAZE *maze, MOVE *last_move)
{
/* 0 if we should search, >0 for already searched, <0 don't search */
/* return 0 if we suspect a deadlock search is beneficial, here should be
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

		/* only if stone/no_reach in front and not searched before */
		return(WasTestedPen(maze->conflicts,maze->stone,
				maze->reach,last_move->to));
	}

	return(-1);

}

int PenMiniConflict(int penalty, int minimize)
/* Called when penalty should be entered */
/* return the maximal penalty seen */
/* Work with total penalties, not relative */
{
	int        result;
	short      old_gm;
	IDA	   idainfo,*old_idainfo;
	int32_t    node_count;
	SAVEMAZE   savemaze;
	PHYSID     pos;
	BitString  already,visible;
	SR(int here_nodes = total_node_count);


	if (penalty>=ENDPATH) {
		DeadMiniConflict(minimize);
		return(ENDPATH);
	} else if (penalty<=0) {
		/* return on 0 penalties, because we might get empty patterns */
		return(0);
	}

	SR(Debug(4,0,"PenMiniConflict #### Start\n"));

	if (Options.minimize == YES && minimize == YES) {
		CopyBS(visible,IdaInfo->IdaMaze->stone);

		old_idainfo = IdaInfo;
		InitIDA(&idainfo);
		IdaInfo                 = &idainfo;
		IdaInfo->IdaMaze        = SaveMaze(old_idainfo->IdaMaze,
						   &savemaze);
		CopyBS(IdaInfo->no_reach,old_idainfo->no_reach);
		IdaInfo->ThresholdInc   = 1;
		IdaInfo->AbortNodeCount = old_idainfo->pattern_node_limit;
		IdaInfo->pattern_node_limit = old_idainfo->pattern_node_limit
						/PATTERNLIMITDIV;
		IdaInfo->goal_last_to   = old_idainfo->goal_last_to;
		IdaInfo->closest_confl  = 0;
		IdaInfo->base_indent   += 2;
		IdaInfo->PrintPriority  = PENPATTERNSEARCHPP;
		IdaInfo->HashTable      = HashTablePen;
		IdaInfo->MiniFlag	= YES;
		old_gm        = Options.mc_gm;
		Options.mc_gm = 0;
		node_count    = 0;
		Set0BS(already);
		for (;;) {
			pos = FindFarthestPosStone(IdaInfo->IdaMaze,
							visible,already);
			if (pos==0) break;
			SetBitBS(already,pos);
			SR(Debug(4,0,"PenMiniConflict #### Start iteration\n"));
			UnsetBitBS(visible,pos);
			PenDeactivateStones(IdaInfo->IdaMaze,visible,ENDPATH);
			node_count += IdaInfo->node_count;
			IdaInfo->node_count = 0;
			result = PenStartIda();
			mini_node_count += IdaInfo->node_count;
			if (result < penalty) {
				if( result > IdaInfo->IdaMaze->pen ) {
					/* There may be another penalty here */
					result = PenMiniConflict(result, YES);
					/* if larger penalty found note and
					 * do not insert that stone again */
					if (penalty < result) penalty = result;
					else SetBitBS(visible,pos);
				} else SetBitBS(visible,pos);
			} else {
				SR(Debug(4,0,"PenMiniConflict: Extra Stone\n"));
				/* increase penalty if possible */
				if (penalty < result) penalty = result;
			}
		}

		PenDeactivateStones(IdaInfo->IdaMaze,visible,ENDPATH);
		AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
			IdaInfo->IdaMaze->no_reach, penalty);
		Options.mc_gm=old_gm;
		RestoreMaze(IdaInfo->IdaMaze,&savemaze);
		IdaInfo = old_idainfo;
		IdaInfo->node_count += node_count;
	}  else { /* if minimization */
		AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
			IdaInfo->IdaMaze->no_reach, penalty);
	}

	SR(Debug(4,0,"PenMiniConflict #### End\n"));
	return(penalty);
}

PHYSID FindFarthestPosStone(MAZE *maze, BitString squares,
		      		  BitString already_visible)
/* Find the square (position) the in squares that has a stone on it
 * that is not already visible */
{
  int    stonei;
  DIST   dist = 0;
  PHYSID pos, p;

  pos = 0;
  for( stonei = 0; stonei < maze->number_stones; stonei++ ) {
    p = maze->stones[ stonei ].loc;
    if( !IsBitSetBS( already_visible, p ) &&
	IsBitSetBS( squares, p ) ) {
      if( dist < XDistStone( maze, maze->manpos, p ) ) {
	dist = XDistStone( maze, maze->manpos, p );
	pos = p;
      }
    }
  }
  return pos;
}

int PenStartIda() {
/* Sets up all data structures and repeatedly calls ida with increasing
   threshold to guarantee optimal solutions, returns 0 if solution found
   otherwise the smallest heuristic value seen at any leaf node if this is
   ENDPATH there is no solution - deadlock */
/* !!! returns the TOTAL penalty for this pattern of stones!!! */

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
	IdaInfo->Threshold = IdaInfo->IdaMaze->h;
/*PrintMaze(IdaInfo->IdaMaze);*/
	for(;;) {
		GetPenalty(IdaInfo->IdaMaze,ENDPATH);
		SR(Debug(3,0,"PenStartIda: Threshold %i (%i) [%i]\n",
			IdaInfo->Threshold,
			IdaInfo->IdaMaze->h,IdaInfo->node_count));
		GTVAny(GTVOpen(IdaInfo->Threshold,
			       GTVFen(IdaInfo->IdaMaze)));
		IdaInfo->IdaMaze->goal_sqto = -1;
		result = PenIda(0,0); /**********************************/
		GTVAny(GTVClose());
		if (   result == 0	/* solution found now */
		    || AbortSearch()	/* effort exhausted */
		    || IdaInfo->CurrentSolutionDepth!=ENDPATH) {
			break;
		}
		if (result>=ENDPATH) {
			IdaInfo->Threshold = ENDPATH +( IdaInfo->IdaMaze->h
						      - IdaInfo->IdaMaze->pen);
			break;
		}
		IdaInfo->Threshold += IdaInfo->ThresholdInc;
	        if (IdaInfo->CurrentSolutionDepth == IdaInfo->Threshold) {
			break;
		}
	}

	print_stats(4);

	result =   IdaInfo->Threshold
		- (IdaInfo->IdaMaze->h - IdaInfo->IdaMaze->pen);
	if (   Options.assumedead == 1
	    && AbortSearch())
	    result = ENDPATH;
	SR(Debug(3,0,"PenStartIda -> end, result: %i\n", result));
	return(result);
}

static int PenCompare(const void *m1, const void *m2) {
        return(((MOVE*)m1)->value - ((MOVE*)m2)->value);
}

int PenMoveOrdering(int depth, int number_moves)
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
                My_qsort(&(S->moves),number_moves,sizeof(MOVE),PenCompare);
        }
        return(number_moves);
}


int PenIda(int treedepth, int g) {
/* the procedure that does the work at one node. it returns
	X - the smallest h underneath this node */

	IDAARRAY  *S;
	HASHENTRY *entry;
	int 	   min_h,number_moves,result,i;
	int	   targetpen;
	int        dir;
	SR(int here_nodes = total_node_count);
	SR(int old_h = IdaInfo->IdaMaze->h- IdaInfo->IdaMaze->pen);

        SR(Debug(5,treedepth,"starting PenIda (h=%i) (%s) %" PRIi32 " %" PRIx64 "\n",
		IdaInfo->IdaMaze->h,
          	treedepth==0?"a1a1"
		      :PrintMove(IdaInfo->IdaArray[treedepth-1].currentmove),
		total_node_count,IdaInfo->IdaMaze->hashkey));
	S = &(IdaInfo->IdaArray[treedepth]);
	GTVAny(GTVNodeEnter(treedepth,g,0,GTVMove(treedepth
		?IdaInfo->IdaArray[treedepth-1].currentmove:DummyMove),0));
	if (AbortSearch()) {
		GTVAny(GTVNodeExit(treedepth,0,"Abort_Search"));
		return(IdaInfo->IdaMaze->h);
	}
	if (treedepth >=MAX_DEPTH) {
		SR(Debug(5,treedepth,"Too deep in the tree!(%i)\n",treedepth));
		GTVAny(GTVNodeExit(treedepth,0,"Too_deep_in_tree"));
		return(IdaInfo->IdaMaze->h);
	}
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
	if (PenIsGoalNode(g)) {
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
		if (IdaInfo->Threshold - g < (int) entry->down) {
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
	number_moves = PenMoveOrdering(treedepth,number_moves);
	if (number_moves==0) DeadMiniConflict(YES);

	/* foreach move call ida */
	targetpen = (  (IdaInfo->Threshold - g)
		     - (IdaInfo->IdaMaze->h - IdaInfo->IdaMaze->pen)) + 2;
	for (i=0; i<number_moves; i++) {
		if (ISDUMMYMOVE(S->moves[i])) continue;
		if (treedepth>0 && IdaInfo->IdaMaze->goal_sqto!=-1) {
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
		SR(Debug(6,treedepth,"PenMakeMove %s (%i of %i)\n",
			PrintMove(S->currentmove),i+1,number_moves));
		if (!PenMakeMove(IdaInfo->IdaMaze,&(S->currentmove),
				 &(S->unmove), targetpen)){
			continue;
		}
		result = PenIda(treedepth+1,g+S->unmove.move_dist);
		PenUnMakeMove(IdaInfo->IdaMaze,&(S->unmove), targetpen);
		SR(Assert(old_h==IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen,
			"PenUnMakeMove: old_h!=h!\n"));
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

int PenMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret, int targetpen)
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
		    <ManDist(maze,IdaInfo->goal_last_to,IdaInfo->closest_confl))
			IdaInfo->closest_confl = move->to;
	}

	/* touching a goal removes stone */
	if (maze->Phys[move->to].goal>=0) {

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
		PenLowerBound(maze,targetpen);
		ret->move_dist = old_h - maze->h;
	} else {
		UpdateHashKey(maze, ret);
		CopyBS(maze->old_no_reach,maze->no_reach);
		if( move->macro_id )
			  MarkReach( maze );
		else
			  UpdateReach( maze, ret->stoneto );
		PenUpdateLowerBound(maze, ret->stoneto, targetpen);
		ret->old_stoneid = -1;
	}

	SR(Assert(maze->manpos>0,"PenMakeMove: manpos < 0!\n"));
	return(1);
}

int PenUnMakeMove(MAZE *maze, UNMOVE *unmove, int targetpen)
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
		PenUpdateLowerBound(maze, unmove->stonefrom, targetpen);
	} else {
		NormHashKey(maze);
		PenLowerBound(maze, targetpen);
	}
	IdaInfo->closest_confl = unmove->old_closest_confl;

	if (  ((unmove->old_stoneid != -1)||(maze->goal_sqto==unmove->stoneto))
	    &&(maze->h - unmove->move_dist == new_h    /* optimal move */)) {
		/* This is either the start or a continuation of a goal move */
		maze->goal_sqto = unmove->stonefrom;
	} else maze->goal_sqto = -1;

	return(0);
}

int PenLowerBound(MAZE *maze, int targetpen)
{
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

int PenUpdateLowerBound(MAZE *maze, PHYSID stonepos, int targetpen)
/* Update lowerbound after move to square pos */
{
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


