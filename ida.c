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

void emitChar(char c) { /* BD */
    char string[2];
    string[0] = c;
    string[1] = '\0';

    Debug(0, -1, string);
}

void emitMove(int direction, bool isPush) { /* BD */
    const char moves[] = "urdlURDL";
    int index = direction + (isPush ? DIRECTION_COUNT : 0);
    emitChar(moves[index]);
}

void PrintPlayerPath(MAZE *maze, PHYSID from, PHYSID to, PHYSID parents[]) { /* BD */
    /* precondition: the parent squares have been calculated by calling
       MarkPlayersReachableSquares( maze, parents )
    */

    if (from == to) {
        return;
    }

    PHYSID parent = parents[to];

    if (parent == UNDEFINED && to != maze->manpos) {
        Debug(0, -1, "\nError: No path found from %d to %d\n", from, to);
        getchar();
        return;
    }

    PrintPlayerPath(maze, from, parent, parents);

    emitMove(DiffToDir(to - parent), false);
}

/************************************************************************/
/*									*/
/*	Classical IDA* routine, compares if g+h<threshold		*/
/*	before searching deeper. 					*/
/*									*/
/*									*/
/************************************************************************/

IDA *IdaInfo;

int StartIda(int nomacro) {
/* Sets up all data structures and repeatedly calls ida with increasing
   threshold to guarantee optimal solutions, returns 0 if solution found
   otherwise the smallest heuristic value seen at any leaf node if this is
   ENDPATH there is no solution - deadlock */

	int       result=ENDPATH;
	MAZE     *maze;
	PHYSID    pos;
	int       i;
	bool      valid_solution;

	/* initialize data structures */
	InitNodeCount();
	PenMaze = CopyMaze( IdaInfo->IdaMaze );
	DeadMaze = CopyMaze( IdaInfo->IdaMaze );
	AreaMaze = CopyMaze( IdaInfo->IdaMaze );
	DInsertMaze = CopyMaze( IdaInfo->IdaMaze );
	InsertMaze = CopyMaze( IdaInfo->IdaMaze );
	Set_Timer();
	init_stats();

	MainIdaInfo.StartTimeMS = GetTimeInMilliseconds();

START_IDA:
	InitHashTables();

	ScanSearch(IdaInfo->IdaMaze);
	scan_node_count = total_node_count;

        memset( &IdaInfo->IdaArray[ 0 ].solution, 0, sizeof( MOVE ) );
	area_pos_nc = area_pos_sc = area_neg_nc = area_neg_sc = 0;
	dl_pos_nc = dl_pos_sc = dl_neg_nc = dl_neg_sc = 0;
	pen_pos_nc = pen_pos_sc = pen_neg_nc = pen_neg_sc = 0;
	AvoidThisSquare = 0;
	Set0BS(IdaInfo->IdaManSquares);
	Set0BS(IdaInfo->IdaStoneSquares);
	IdaInfo->CurrentSolutionDepth = ENDPATH;
	for (IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	       (IdaInfo->CurrentSolutionDepth > IdaInfo->Threshold)
	     &&(  (IdaInfo->Threshold < (IdaInfo->IdaMaze->h<<1) )
		||(IdaInfo->Threshold < 100))
	     &&(!AbortSearch());
	     IdaInfo->Threshold += IdaInfo->ThresholdInc) {
		GetPenalty(IdaInfo->IdaMaze,ENDPATH);
		Debug(2,0,"Threshold %i (%i)\n",
			IdaInfo->Threshold, IdaInfo->IdaMaze->h);
		GTVAny(GTVOpen(IdaInfo->Threshold,
			       GTVFen(IdaInfo->IdaMaze)));
		IdaInfo->IdaMaze->goal_sqto = -1;

		result = Ida(0,0); /**********************************/
		GTVAny(GTVClose());
		print_stats(2);
		if (result>=ENDPATH)
			IdaInfo->Threshold = ENDPATH + IdaInfo->ThresholdInc;
	}
	IdaInfo->Threshold -= IdaInfo->ThresholdInc;
	if (result<ENDPATH) result = IdaInfo->Threshold - IdaInfo->IdaMaze->h;

	/* if we used goal macros and did not find a solution and did not
	 * exhaust the search effort (#38), rerun search without goal macros */
	if (MainIdaInfo.TimedOut == NO
	    && (   (result >= ENDPATH)
	        || (IdaInfo->Threshold+IdaInfo->ThresholdInc >= (IdaInfo->IdaMaze->h<<1)))
	    && nomacro==YES
	    && IdaInfo->IdaMaze->number_grooms > 0
	    && (  IdaInfo->node_count < IdaInfo->AbortNodeCount
		||IdaInfo->AbortNodeCount==-1)) {
		/* Turn off macros */
printf("removing goal macro\n");
		maze = IdaInfo->IdaMaze;
		for (pos = 0; pos < XSIZE*YSIZE; pos++) {
			maze->groom_index[pos] = -2;
			if (maze->macros[pos].type == 4)
				maze->macros[pos].type = 0;
		}
		for (i=0; i<maze->number_grooms; i++) {
			DelGMTree(maze->gmtrees[i]);
		}
		My_free(maze->gmtrees);
		maze->gmtrees = NULL;
		maze->number_grooms = 0;
		My_free(maze->grooms);
		maze->grooms = NULL;

		goto START_IDA;
	}

	Remove_Timer();

	MainIdaInfo.StopTimeMS = GetTimeInMilliseconds();

	if (MainIdaInfo.TimedOut == YES) {
		Debug( 0, -1, "\n%s\n\n", IdaInfo->IdaMaze->name );
		Debug( 0, -1, "Timeout!\n");
	} else {
		valid_solution = PrintSolution();
		if (valid_solution) {
			PrintSolutionUsingLURDNotation(); /* BD */
		}
	}

	uint64_t timeSeconds = CalculateElapsedTimeSeconds(MainIdaInfo.StartTimeMS, MainIdaInfo.StopTimeMS);
	Debug(0, -1, "\nTime: %llu second%s\n", timeSeconds, timeSeconds == 1 ? "" : "s");

	Debug( 0, -1, "\n");

	DelCopiedMaze( PenMaze );
	DelCopiedMaze( DeadMaze );
	DelCopiedMaze( AreaMaze );
	DelCopiedMaze( DInsertMaze );
	DelCopiedMaze( InsertMaze );
	return(result);
}

bool PrintSolution()
{
	MAZE     *maze;
	MOVE      lastmove;
	MOVE      solution[ENDPATH];
	UNMOVE    unmove;
	int       i,g;
	bool      valid_solution;

	Debug(0,-1,"Path: ");
	i = 0; g = 0;
	lastmove = DummyMove;
	maze = CopyMaze(IdaInfo->IdaMaze);
	while (IdaInfo->IdaArray[i].solution.from != 0 && maze->h>0) {
		solution[i]=IdaInfo->IdaArray[i].solution;
		if (lastmove.to==IdaInfo->IdaArray[i].solution.from) {
			Debug(0,-1,"%s",
				HumanMove(IdaInfo->IdaArray[i].solution)+2);
		} else {
			Debug(0,-1," %s",
				HumanMove(IdaInfo->IdaArray[i].solution));
		}
		lastmove = IdaInfo->IdaArray[i].solution;
		MakeMove(maze,&(IdaInfo->IdaArray[i].solution),
			 &unmove,ENDPATH);
		if (IdaInfo->IdaArray[i].unmove.move_dist>1) {
			Debug(0,-1,"*");
		}
		g += IdaInfo->IdaArray[i].unmove.move_dist;
		i++;
	}
	Debug(0,-1,"\n(moves: %i depth: %i)\n",g,i);
	solution[i] = DummyMove;
	DelCopiedMaze(maze);

	maze = CopyMaze(IdaInfo->IdaMaze);
	valid_solution = ValidSolution(maze,solution);
	Debug( 0, -1, "\n%s\n\n", maze->name );
	if (!valid_solution) {
		if (i == 0) {
			Debug(0, -1, "No solution found\n");
		} else {
			Debug(0, -1, "Invalid solution\n");
		}
	}
	DelCopiedMaze(maze);

	IdaInfo->IdaArray[i].solution.from = 0; /* set end-of-solution marker */ /* BD */
	return valid_solution;
}

void PrintSolutionUsingLURDNotation() { /* BD */
    /* Prints the Sokoban puzzle solution using LURD notation */
    MAZE *maze;
    PHYSID from, to, playerFromSquare, parents[MAZESIZE], boxPath[ENDPATH + 1];
    int i, j, k, boxPushDirection, pushCount;

    maze = CopyMaze(IdaInfo->IdaMaze);

    PrintMaze( maze, false );

    AvoidThisSquare = 0;
    MarkReach( maze );

    i = 0;
    Debug(0, -1, "Solution:\n");
    while (IdaInfo->IdaArray[i].solution.from != 0) {

        pushCount = calculateBoxPath(maze, IdaInfo->IdaArray[i].solution.from,
                                           IdaInfo->IdaArray[i].solution.to,
                                           IdaInfo->IdaArray[i].solution.last_over, /* box-from-square for the last of the pushes */
                                           boxPath);

        for (j = 0; j < pushCount; j++) { /* push the box one square at a time */
          from             = boxPath[ j ];
          to               = boxPath[ j + 1 ];
          boxPushDirection = DiffToDir(to - from);
          playerFromSquare = from - DirToDiff[ boxPushDirection ];

          MarkPlayersReachableSquares(maze, parents);                     /* calculate the player's path to "playerFromSquare" */
          PrintPlayerPath(maze, maze->manpos, playerFromSquare, parents); /* emit non-pushing player moves */
          emitMove(boxPushDirection, true);                               /* emit box push */

          MANTO(maze, from); /* do the push on the board */
          STONEFROMTO(maze, from, to );
          MarkReach( maze );
        }
        i++;
    }
    Debug( 0, -1, "\n"); /* emit a blank line to separate the puzzle solution from the next command */

    DelCopiedMaze(maze);
}

int IsGoalNodeNorm(int g)
{
	int i;

	if (IdaInfo->IdaMaze->h!=0) {
		return(0);
	} else {
		if (g < IdaInfo->CurrentSolutionDepth) {
			IdaInfo->CurrentSolutionDepth=g;
			for (i=0; i<g; i++) {
				IdaInfo->IdaArray[i].solution
					= IdaInfo->IdaArray[i].currentmove;
			}
		}
		if (g > IdaInfo->Threshold) {
			Debug(0,0,"Premature Goal Found! Depth: %d\n",g);
			return(0);
		} else return(1);
	}
}

int Ida(int treedepth, int g) {
/* the procedure that does the work at one node. */

	IDAARRAY  *S;
	HASHENTRY *entry;
	MOVE       *last_move;
	int 	   min_h,result,i;
        int        dir;
	int	   targetpen;
	int	   areasearched = 0, dlsearched = 0, pensearched = 0;
	int old_h = IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen;
	SR(int here_nodes = total_node_count;)

        SR(Debug(4,treedepth,"starting ida (h=%i) (%s) %d\n",
		IdaInfo->IdaMaze->h,
		treedepth==0?"a1a1":PrintMove(IdaInfo->IdaArray[treedepth-1].currentmove),IdaInfo->node_count));
	S = &(IdaInfo->IdaArray[treedepth]);

	GTVAny(GTVNodeEnter(treedepth,g,0,GTVMove(treedepth?IdaInfo->IdaArray[treedepth-1].currentmove:DummyMove),0));
	if (AbortSearch()) {
		GTVAny(GTVNodeExit(treedepth,0,"Abort_Search"));
		return(IdaInfo->IdaMaze->h);
	}
	IncNodeCount(treedepth);

	/* check for goal state, if yes return(0) */
	if (IsGoalNodeNorm(g)) {
		SR(Debug(4,treedepth,"Found goal (%i %i)******************\n",
			IdaInfo->IdaMaze->h, IdaInfo->IdaMaze->number_stones));
		GTVAny(GTVNodeExit(treedepth,0,"Goal_found"));
		return(0);
	}

	/* The following order is important, we might find a goal
	   an iteration earlier! */
	/* check for cutoff: is g+h > threshold => return(0) (fail) */
	if (g+IdaInfo->IdaMaze->h>IdaInfo->Threshold) {
		SR(Debug(4,treedepth,"Threshold cutoff (%i=%i+%i)\n",
			g+IdaInfo->IdaMaze->h,g,IdaInfo->IdaMaze->h));
		GTVAny(GTVNodeExit(treedepth,IdaInfo->IdaMaze->h,
			"Threshold_Cutoff"));
		return(IdaInfo->IdaMaze->h);
	}

	/* check trans table if searched already */
	entry = GetHashTable(IdaInfo->IdaMaze);
	if (entry != NULL) {
		if (entry->pathflag == 1) {
			SR(Debug(4,treedepth, "Cycle (TT)\n"));
			GTVAny(GTVNodeExit(treedepth, ENDPATH,"Cycle (TT)"));
			IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
			return(IdaInfo->IdaMaze->h);
		} else if (IdaInfo->Threshold - g <= (int) entry->down) {
			SR(Debug(4,treedepth, "Futil (TT) %i<=%i\n",
				IdaInfo->Threshold-g,entry->down));
			GTVAny(GTVNodeExit(treedepth,ENDPATH,"Futil (TT)"));
			IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
			return(entry->min_h);
		}
		SetPathFlag(IdaInfo->IdaMaze);
	} else {
		entry = StoreHashTable(IdaInfo->IdaMaze,0,
			IdaInfo->IdaMaze->h,areasearched,
			dlsearched,pensearched,1);
	}

	/* Only if was not found in TT,this search makes sense */
	targetpen = (  (IdaInfo->Threshold - g)
		     - (IdaInfo->IdaMaze->h - IdaInfo->IdaMaze->pen)) + 2;
	last_move = &(IdaInfo->IdaArray[treedepth-1].currentmove);
	if (treedepth>0) {
	    if (   Options.area_srch == 1
		&& entry->areasearched == 0) {
		areasearched = 1;
		if (AreaMove(IdaInfo->IdaMaze,last_move,treedepth,targetpen)) {
		    SR(Debug(4,treedepth,"Penalty detected (AreaMove)\n"));
		    GTVAny(GTVNodeExit(
			treedepth,entry->min_h,"AreaMove: penalty"));
		    StoreHashTable(IdaInfo->IdaMaze,IdaInfo->Threshold-g,
				IdaInfo->IdaMaze->h,areasearched,
				dlsearched,pensearched,0);
		    return(IdaInfo->IdaMaze->h);
		}
	    }
	    if (   Options.pen_srch == 1
		&& entry->pensearched == 0
		&& last_move->macro_id != 4
		&& PenMoveSuspected(IdaInfo->IdaMaze,last_move)==0) {
		pensearched = 1;
		if (PenMove(IdaInfo->IdaMaze,entry,last_move,treedepth,
			targetpen)) {
		    SR(Debug(4,treedepth,"Penalty detected (PenMove)\n"));
		    GTVAny(GTVNodeExit(
			treedepth,entry->min_h,"PenMove: penalty"));
		    StoreHashTable(IdaInfo->IdaMaze,IdaInfo->Threshold-g,
				IdaInfo->IdaMaze->h,areasearched,
				dlsearched,pensearched,0);
		    return(IdaInfo->IdaMaze->h);
		}
	    }
	    if (   Options.dl_srch == 1
		&& entry->dlsearched == 0
		&& last_move->macro_id != 4
		&& DeadMoveSuspected(IdaInfo->IdaMaze, last_move)) {
		dlsearched = 1;
		if (DeadMove(IdaInfo->IdaMaze,last_move,treedepth)) {
		    SR(Debug(4,treedepth,"Deadlock detected (DeadMove)\n"));
		    GTVAny(GTVNodeExit(
			    treedepth,entry->min_h,"DeadMove: deadlck"));
		    return(ENDPATH);
		}
	    }
	}
	if (treedepth >= IdaInfo->ForwDepthLimit) {
		SR(Debug(4,treedepth,"Too deep in the tree!(%i)\n",treedepth));
		GTVAny(GTVNodeExit(treedepth,0,"Too_deep_in_tree"));
		return(IdaInfo->IdaMaze->h);
	}

	min_h    = ENDPATH;

	S->number_moves = GenerateMoves(IdaInfo->IdaMaze,&(S->moves[0]));
	S->number_moves = (*IdaInfo->MoveOrdering) (treedepth,S->number_moves);
	if (S->number_moves == 0) {
		DeadMiniConflict(YES);
		return(IdaInfo->IdaMaze->h);
	}

	/* foreach move call ida */
	for (i=0; i<S->number_moves; i++) {
		if (ISDUMMYMOVE(S->moves[i])) continue;
		S->currentmove = S->moves[i];
		S->currentindex = i;

		if (   Options.cut_goal==1
		    && S->moves[i].macro_id!=4
		    && IdaInfo->IdaMaze->goal_sqto != -1 ) {
		    /*&& IdaInfo->IdaMaze->goal_sqto != -1
		    && IdaInfo->IdaMaze->goal_sqto != S->moves[i].from ) {*/
			SR(Debug(4,treedepth,"Goal Cut move\n"));
			continue;
		}
		if (   Options.local == 1
		    /* && i > 0 make sure moves to the goal are allowed? */
		    && RegisterMove(&(S->moves[i]),treedepth)) {
			SR(Debug(4,treedepth,"Local Cut move %s\n",
                                PrintMove(S->moves[i])));
                        continue;
		}
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
		SR(Debug(6,treedepth,"MakeMove %s (%i of %i)\n",
			PrintMove(S->currentmove),i+1,S->number_moves));
		if( !MakeMove( IdaInfo->IdaMaze, &S->currentmove,
			       &S->unmove, targetpen ) ) {
		  continue;
		}

		result = Ida(treedepth+1,g+S->unmove.move_dist);

		UnMakeMove(IdaInfo->IdaMaze,&(S->unmove), targetpen);
		if (old_h != IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen)
			BetterLowerBound(IdaInfo->IdaMaze);

		if (result < min_h) {
			min_h = result;
		}
		/* one solution found? */
		if (result == 0) {
			S->solution = S->currentmove;
			SetManStoneSquares(IdaInfo->IdaMaze,S->currentmove);
			goto END_IDA;
		}

		if (IdaInfo->IdaMaze->h + g > IdaInfo->Threshold)
			goto END_IDA;

		/* if a deadlock score came back, maybe we are already in a
			deadlock??? NOTE: A test for RevMove here could
			propagate the deadlock up  - did not work - why? */
	}

END_IDA:
	/* write Transposition table */
	StoreHashTable(IdaInfo->IdaMaze,IdaInfo->Threshold-g,min_h,
		areasearched,dlsearched,pensearched,0);
	SR(Debug(4,treedepth,"return from ida %i (%i)\n", treedepth, min_h));
	GTVAny(GTVNodeExit(treedepth,min_h,"Normal_Exit"));
	return(min_h);
}

void SetManStoneSquares(MAZE *maze, MOVE bestmove)
{
	BitString stone,man;
	int p,m;

	PushesMoves(maze,bestmove.from,bestmove.to,&p,&m,stone,man);
	BitOrEqBS(IdaInfo->IdaStoneSquares,stone);
	BitOrEqBS(IdaInfo->IdaManSquares,man);
}

int AbortSearch() {

	/* stop any search if its limit is reached */
	if (   MainIdaInfo.TimedOut == YES) return(1);
	if (   MainIdaInfo.AbortNodeCount >= 0
	    && total_node_count >= MainIdaInfo.AbortNodeCount) return(1);
	if (   IdaInfo->AbortNodeCount >= 0
	    && IdaInfo->node_count >= IdaInfo->AbortNodeCount) return(1);

	return(0);
}

void InitIDA(IDA *ida)
{
	Set0BS(ida->IdaManSquares);
	Set0BS(ida->IdaStoneSquares);
	ida->IdaMaze        = NULL;
	ida->Threshold      = 0;
	ida->ThresholdInc   = 2;
        /* ida->IdaArray[0].solution = (MOVE){0,0,0,0,0,0}; */
        memset( &ida->IdaArray[ 0 ].solution, 0, sizeof( MOVE ) );
	ida->AbortNodeCount = -1;
	ida->ForwDepthLimit = MAX_DEPTH-1;
	ida->base_indent    = 0;
	ida->MiniFlag       = 0;

	ida->CurrentSolutionDepth = ENDPATH;

	ida->HashTable	    = HashTableNorm;
	ida->MoveOrdering   = NewMoveOrdering;

	ida->goal_last_to   = 0;
	ida->closest_confl  = 0;
	Set0BS(ida->shadow_stones);
	Set0BS(ida->no_reach);
	ida->pattern_node_limit = 1000;

	ida->tt_hits = ida->tt_cols = ida->tt_reqs =
	ida->gmtt_hits = ida->gmtt_cols = ida->gmtt_reqs = ida->node_count = 0;
	ida->dcache_hits = 0;

	ida->PrintPriority  = 0;
	ida->TimeOut        = 0;
	ida->TimeOutType    = REAL;
	ida->TimedOut       = 0;

}

void SetLocalCut(int k, int m, int d)
/* any of k,m set to -1 will turn local cut off
   any of k,m set to  0 will auto set parameters
   else set as sent in */
{
	if (k < 0 || m < 0 || k>m) {
		Mprintf(0,"Strange local cut parameter setting: %i,%i.\n",k,m);
		Mprintf(0,"Turning local cut off!\n");
		Options.local_k = -1;
		Options.local_m = -1;
		Options.local_d = max(YSIZE,XSIZE);
		Options.local = 0;
	} else if ((k < 1 || m < 1) && Options.autolocal==1) {
		k = 1;
		XDistHist(IdaInfo->IdaMaze, &d, &m);
		d = max(6,d);
		m = min(10,m);
		Mprintf(0,"Auto Set local cut parameter: %i,%i, %i.\n",k,m,d);
		Options.local_k = (short) k;
		Options.local_m = (short) m;
		Options.local_d = (short) d;
	} else {
		Mprintf(0,"Set local cut parameter: %i,%i, %i.\n",k,m,d);
		Options.local_k = (short) k;
		Options.local_m = (short) m;
		Options.local_d = (short) d;
		Options.local = 1;
		Options.autolocal = 0;
	}
}

int DistantSquares(PHYSID s1, PHYSID s2, short crowding)
/* Return YES if the two squares are distant */
{
       return( (((int)XDistMan(IdaInfo->IdaMaze,s1,s2)>(Options.local_d+crowding)))
	     &&(((int)XDistMan(IdaInfo->IdaMaze,s2,s1)>(Options.local_d+crowding))));
}

int DistantMove(MAZE *maze, MOVE *last_move, MOVE *test_move)
/* Returns YES if test_move is considered a distant move to last_move */
{
	if ( DistantSquares(test_move->from,last_move->from,
		Crowding(maze,last_move->to))
	    && (test_move->macro_id != 4)
	    && (last_move->macro_id != 4))
		return(YES);
	return(NO);
}

short Crowding(MAZE *maze, PHYSID sq)
/* return a crowding number. The more stones in the vicinity, the larger the
 * crowding number */
/* for now, the crowding number is the number stones "local" to sq */
{
	short crowding;
	int i;
	static short tbl[MAXSTONES] = {0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1,
			      2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3, 4,4};
	crowding = 0;
	for (i=0; i<maze->number_stones; i++) {
		if (!DistantSquares(sq,maze->stones[i].loc,0)) crowding++;
	}
	return(tbl[crowding]);
}

int RegisterMove(MOVE *move, int depth)
/* Return YES if move should be cut off */
/* do two things, do not allow switches back into previously visited areas
          and,    do not allow too many switches between previous moves */
{
	int i,n;
	int firstswitch;


	/* if move was just made available, never cut! */
	if (IsBitSetBS(IdaInfo->IdaMaze->old_no_reach,move->man)) {
/*printf("New, accessible move!\n");*/
		return(NO);
	}

	if (   depth > 0
	    && DistantMove(IdaInfo->IdaMaze,
			   &(IdaInfo->IdaArray[depth-1].currentmove),
			   move)) {
		firstswitch = YES;
	} else {
		firstswitch = NO;
	}
	n = 0;
	/* last move was non-local, now see if we where in that area
	 * before, and if so, how many times. if we exceed k, cut */
	i = 1;
	while ( ( depth-i ) >= 0 && ( i < Options.local_m ) ) {
	   /* if we hit a goal macro move stop */
	   if ( IdaInfo->IdaArray[depth-i].currentmove.macro_id == 4 )
			break;
	   /* if move was local to ancestor, count */
	   /* AND not the same as previous stone */
	   if (   firstswitch == YES
	       && !DistantMove(IdaInfo->IdaMaze,
		   	       &(IdaInfo->IdaArray[depth-i].currentmove),
		   	       move)
	       && DistantMove(IdaInfo->IdaMaze,
			      &(IdaInfo->IdaArray[depth-i].currentmove),
			      &(IdaInfo->IdaArray[(depth-i)+1].currentmove))) {
			return( YES );
	   }

	   /* Count how many distant moves where made in the recent past */
	   if (DistantMove(IdaInfo->IdaMaze,
			   &(IdaInfo->IdaArray[depth-i].currentmove),
			   &(IdaInfo->IdaArray[(depth-i)+1].currentmove)))
		n++;
	   if ( n > Options.local_k )
		return( YES );
	   i++;
	}
	return( NO );
}

