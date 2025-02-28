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

/*******************************************************************************/
/* 									       */
/* 	MOVE will now include only stone pushes and is centered on that notion.*/
/*	This way from and to in MOVE and UNMOVE will have a different meaning  */
/* 	than in PATH							       */
/* 									       */
/*******************************************************************************/

#define SETMOVE(move,v,ifrom,ito,manp)\
	(move).move_dist =1; (move).from =ifrom; (move).last_over =ifrom;\
	(move).macro_id =0; (move).to =ito; (move).value =v; (move).man=manp;

int DirToDiff[8] = {1,YSIZE,-1,-YSIZE,1+YSIZE,-1+YSIZE,-1-YSIZE,1-YSIZE};
int  OppDir[8] = {2,3,0,1,6,7,4,5};
int NextDir[8] = {1,2,3,0,5,6,7,4};
int PrevDir[8] = {3,0,1,2,7,4,5,6};

int GenerateMoves(MAZE *maze, MOVE *moves)
{
	PHYSID pos;
	int i,dir;
	int number_moves = 0;
	BitString stones_done;

	CopyBS(stones_done,maze->stones_done);
	for (i=0; i<maze->number_stones; i++) {
		pos = maze->stones[i].loc;
		if (  (maze->PHYSstone[pos]<0)
		    ||(IsBitSetBS(stones_done,pos)))
			continue;
		SetBitBS(stones_done,pos);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (   (IsBitSetBS( maze->reach,pos+DirToDiff[dir]))
			    && (IsBitSetBS(maze->S[OppDir[dir]],pos))
			    && (maze->PHYSstone[pos-DirToDiff[dir]]<0)) {
				SETMOVE(moves[number_moves],1,
				        pos,pos-DirToDiff[dir],
					pos+DirToDiff[dir]);
				if (!DeadLock(maze,moves+number_moves)) {
					if (SubMacro(maze,moves,&number_moves))
						goto END;
					number_moves++;
/* JS: Check this does not exceed MAX_MOVES */
				}
			}
		}
	}
END:
	moves[number_moves].to = ENDPATH;
	moves[number_moves].from = ENDPATH;
	return(number_moves);
}

static int compare_val(const void *m1, const void *m2) {
	return(((MOVE*)m2)->value - ((MOVE*)m1)->value);
}

static int compare_negval(const void *m1, const void *m2) {
/* secondary ordering by man move distance */
/* if( ( (MOVE *)m1 )->value == ( (MOVE *)m2 )->value && */
/*     !ISDUMMYMOVE( *(MOVE *)m1 ) && !ISDUMMYMOVE( *( MOVE *)m2 ) ) */
/*   return IdaInfo->IdaMaze->m_weights[ ( (MOVE *)m1 )->last_over ]->w */
/*     [ IdaInfo->IdaMaze->manpos ] - */
/*     IdaInfo->IdaMaze->m_weights[ ( (MOVE *)m2 )->last_over ]->w */
/*     [ IdaInfo->IdaMaze->manpos ]; */
	return(((MOVE*)m1)->value - ((MOVE*)m2)->value);
}

int NoMoveOrdering(int depth, int number_moves)
{
	return(number_moves);
}

int NewMoveOrdering(int depth, int number_moves)
{
	IDAARRAY *S;
	MAZE  *maze;
	MOVE  *m,*lmove;
	PHYSID goalpos;
	GROOM *groom;
	int    e;
	int    i,diff;
	DIST   dist,d;

	if (number_moves>1) {
 		lmove = depth?&(IdaInfo->IdaArray[depth-1].currentmove)
				:&DummyMove;
		S = &(IdaInfo->IdaArray[depth]);
		maze = IdaInfo->IdaMaze;
		for (i=0; i<number_moves; i++) {
			m = &(S->moves[i]);
			if (ISDUMMYMOVE(*m)) {
				m->value = -ENDPATH;
				continue;
			}
			/* Use the closest entrance to the goalll area the
			 * stone is in (if) as goalpos */
			goalpos = maze->goals[
				    maze->lbtable[
				      maze->PHYSstone[m->from]].goalidx].loc;
			if (maze->groom_index[m->from] >= 0 ) {
			    dist  = StoneDist(maze,m->from,goalpos);
			} else if (maze->groom_index[goalpos] >= 0 ) {
			    groom = maze->grooms + maze->groom_index[goalpos];
			    dist  = StoneDist(maze,m->from,goalpos);
			    for (e = 0; e < groom->n; e++) {
				d = StoneDist(maze,m->from,groom->locations[e]);
				if (dist > d) {
					dist = d;
					goalpos = groom->locations[e];
				}
			    }
			} else {
			    dist = MAXDIST;
			    for (e = 0; e < maze->number_goals; e++) {
				d = StoneDist(maze,m->from,maze->goals[e].loc);
				if (dist > d) {
					dist = d;
					goalpos = maze->goals[e].loc;
				}
			    }
			}
			m->value = dist;

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
		My_qsort(&(S->moves),number_moves,sizeof(MOVE),compare_negval);
	}
	return(number_moves);
}

int ManDistMoveOrdering(int depth, int number_moves)
{
	if (number_moves>1) {
		My_qsort(&(IdaInfo->IdaArray[depth].moves),
			number_moves,sizeof(MOVE),compare_val);
	}
	return(number_moves);
}

int MakeMove(MAZE *maze, MOVE *move, UNMOVE *ret, int targetpen)
/* this is a routine that makes a STONE move, not merely a man move like
 * DoMove. It will just put the man to the new location */
{
	int i;
	int recalculate;	/* should we recalculate the distances */
	int fixed;		/* stone moved into goal is fixed? */
	GMNODE **gmtree=NULL;

	ret->manfrom  = maze->manpos;
	ret->stonefrom= move->from;
	ret->stoneto  = move->to;
	ret->macro_id = move->macro_id;
	ret->move_dist= move->move_dist;
	CopyBS( ret->save_old_no_reach, maze->old_no_reach);
	CopyBS( ret->save_reach, maze->reach );
	CopyBS( ret->save_no_reach, maze->no_reach );
	ret->old_GMTree = NULL;
	maze->goal_sqto = -1;
	ret->old_s_distances = NULL;

	recalculate = YES;
	fixed	    = NO;
	CopyBS(ret->old_stones_done,maze->stones_done);
	if (maze->groom_index[move->to]>=0 && Options.mc_gm==1) {
		gmtree = &(maze->gmtrees[maze->groom_index[move->to]]);
		if (move->macro_id == 4) {
			/* this is the case when the goal macro worked */
			ret->old_GMTree = *gmtree;

			fixed = YES;
			/* set GMTree to next level down */
			i = 0;
			while (   (i<(*gmtree)->number_entries)
		       	&& ((*gmtree)->entries[i].goal_loc!=move->to)) i++;
			if (i<(*gmtree)->number_entries) {
				if ((*gmtree)->entries[i].new_distances==NO) {
					recalculate = NO;
				}

				(*gmtree) = (*gmtree)->entries[i].next;
			} else {
				(*gmtree) = NULL;
			}
		}
	}

	MANTO(maze,move->last_over);
	STONEFROMTO(maze,ret->stonefrom,ret->stoneto);
	UpdateHashKey(maze, ret);
	CopyBS(maze->old_no_reach,maze->no_reach);
        MarkReach( maze );
	if (IsBitSetBS(maze->goal,move->to)) {
	    /* Moving onto a goal, see if we should mark stones_done */
	    if (fixed || IsStoneDone(maze,move->to)) {
		    PropStonesDone(maze,move->to);
	    } else recalculate = NO;
	    /* Only if we know better from the gm-pre-calc, don't recalculate */
	    if (recalculate == YES && Options.lb_dd == YES) {
		GetNewDistances(maze,ret);
	    }
	}
	/* make sure we recalculate the lower bound for the new distances */
	if (ret->old_s_distances == NULL) {
		BetterUpdateLowerBound(maze, ret, targetpen);
	} else {
		BetterLowerBound(maze);
	}

	SR(Assert(maze->manpos>0,"MakeMove: manpos < 0!\n"));
	maze->currentmovenumber++;
	maze->g += ret->move_dist;
	return(1);
}

int UnMakeMove(MAZE *maze, UNMOVE *unmove, int targetpen)
{
	GMNODE **gmtree;
	int	 new_h;

	new_h = maze->h;
	MANTO(maze,unmove->manfrom);
	STONEFROMTO(maze,unmove->stoneto,unmove->stonefrom);

	if (unmove->old_GMTree != NULL) {
		Options.mc_gm = 1;
		gmtree = &(maze->gmtrees[maze->groom_index[unmove->stoneto]]);
		(*gmtree) = unmove->old_GMTree;
	}

	if (IsBitSetBS(maze->goal,unmove->stoneto)) {
		CopyBS(maze->stones_done,unmove->old_stones_done);
	}
	if (unmove->old_s_distances != NULL) {
		/* Make sure we restore the old weight tables */
		maze->s_distances = unmove->old_s_distances;
		maze->m_distances = unmove->old_m_distances;
		maze->connected   = unmove->old_connected;
		CopyBS(maze->one_way,unmove->old_one_way);
		BetterLowerBound(maze);
	} else {
		BetterUpdateLowerBound2(maze, unmove, targetpen);
	}

	UpdateHashKey(maze, unmove);
	CopyBS( maze->reach, unmove->save_reach );
	CopyBS( maze->no_reach, unmove->save_no_reach );
	CopyBS( maze->old_no_reach, unmove->save_old_no_reach);

	if (  ((unmove->macro_id==4) || (maze->goal_sqto==unmove->stoneto))
	    &&(maze->h - unmove->move_dist == new_h    /* optimal move */)) {
		/* This is either the start or a continuation of a goal move */
		maze->goal_sqto = unmove->stonefrom;
	} else maze->goal_sqto = -1;

	maze->currentmovenumber--;
	maze->g -= unmove->move_dist;
	return(0);
}

int DistToGoal(MAZE *maze, PHYSID start, PHYSID goal, PHYSID *last_over) {
/* pseudo-recursive function to see how many pushes are needed to get to
 * goal */

        static PHYSID stack[XSIZE*YSIZE];
        static PHYSID from[XSIZE*YSIZE];
        static PHYSID dist[XSIZE*YSIZE];
	static int s_visited[4][XSIZE*YSIZE];
        PHYSID pos,fro,old_man;
	char   old_stone;
        int    goal_dist, next_in, next_out, dir, dis;

	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		for (dir=NORTH; dir<=WEST; dir++) {
			s_visited[dir][pos]=-1;
		}
	}

	/* set initial from to a position immediate to start on right side */
	for (dir=NORTH; dir<=WEST; dir++) {
		if (IsBitSetBS(maze->reach,start+DirToDiff[dir])) {
			from[0]  = start+DirToDiff[dir];
			break;
		}
	}
	old_man  = maze->manpos;
        stack[0] = start;
	dist[0]  = 0;
	old_stone= maze->PHYSstone[start];
	maze->PHYSstone[start]=-1;
        next_in  = 1;
        next_out = 0;
	goal_dist = -1;
        while (next_out < next_in) {
		fro = from[next_out];
                pos = stack[next_out];
                dis = dist[next_out++];
                if (maze->PHYSstone[pos] >= 0)
			continue;
		if (pos==goal) {
			*last_over = fro;
			goal_dist = dis;
			break;
		}
		dir = DiffToDir(fro-pos);
		if (s_visited[dir][pos] > 0) continue;

		/* set maze up to make the reach analysis for the man */
		MANTO(maze,fro);
		AvoidThisSquare = pos;
		MarkReach(maze);

		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->reach,pos+DirToDiff[dir]))
				s_visited[dir][pos] = dis;
		}
		for (dir=NORTH; dir<=WEST; dir++) {
                	if (   IsBitSetBS(maze->S[dir],pos)
			    && IsBitSetBS(maze->reach, pos-DirToDiff[dir])) {
				from[next_in] = pos;
				dist[next_in] = dis + 1;
				stack[next_in++] = pos+DirToDiff[dir];
			}
		}
		dis = -1;
        }
	MANTO(maze,old_man);
	maze->PHYSstone[start]=old_stone;
	AvoidThisSquare = 0;
	MarkReach(maze);
	return(goal_dist);
}

void Moves(MAZE *maze, PHYSID *from, signed char *reach )
{
  static PHYSID stack[ ENDPATH ];
  static PHYSID f[ ENDPATH ];
  PHYSID pos;
  int next_in, next_out, dir;

  memset( reach, -1, XSIZE * YSIZE );
  stack[ 0 ] = maze->manpos;
  f[ 0 ] = 0;
  from[ 0 ] = -1;
  next_in  = 1;
  next_out = -1;
  while( ++next_out < next_in ) {
    pos = stack[ next_out ];
    if( reach[ pos ] >= 0 ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;
    if( AvoidThisSquare == pos ) continue;
    reach[ pos ] = reach[ from[ pos ] = f[ next_out ] ] + 1;
    for( dir = NORTH; dir <= WEST; dir++ )
      if( IsBitSetBS( maze->M[ dir ], pos ) ) {
	f[ next_in ] = pos;
	stack[ next_in++ ] = pos + DirToDiff[ dir ];
      }
  }
}

/* generate a path that does not touch any of the squares in shadows */
void Moves2(MAZE *maze, PHYSID *from, signed char *reach, BitString shadows )
{
  static PHYSID stack[ ENDPATH ];
  static PHYSID f[ ENDPATH ];
  PHYSID pos;
  int next_in, next_out, dir;

  memset( reach, -1, XSIZE * YSIZE );
  stack[ 0 ] = maze->manpos;
  f[ 0 ] = 0;
  from[ 0 ] = -1;
  next_in  = 1;
  next_out = -1;
  while( ++next_out < next_in ) {
    pos = stack[ next_out ];
    if( reach[ pos ] >= 0 ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;
    if( AvoidThisSquare == pos ) continue;
    if( IsBitSetBS( shadows, pos ) ) continue;
    reach[ pos ] = reach[ from[ pos ] = f[ next_out ] ] + 1;
    for( dir = NORTH; dir <= WEST; dir++ )
      if( IsBitSetBS( maze->M[ dir ], pos ) ) {
	f[ next_in ] = pos;
	stack[ next_in++ ] = pos + DirToDiff[ dir ];
      }
  }
}

void GenAllSquares( PHYSID pos, PHYSID *from, BitString all_squares )
{
  Set0BS( all_squares );
  SetBitBS( all_squares, pos );
  while( from[ pos ] > 0 ) {
    pos = from[ pos ];
    SetBitBS( all_squares, pos );
  }
}

void PushesMoves(MAZE *maze, PHYSID start, PHYSID goal,
		 int *pushes, int *moves,
		 BitString stone_squares, BitString man_squares)
{

  static PHYSID stack[XSIZE*YSIZE];
  static PHYSID from[XSIZE*YSIZE];
  static PHYSID mand[XSIZE*YSIZE];
  static PHYSID n_pu[XSIZE*YSIZE];
  static PHYSID n_mo[XSIZE*YSIZE];
  static PHYSID movefrom[ XSIZE * YSIZE ];
  static BitString s_squares[XSIZE*YSIZE];
  static BitString m_squares[XSIZE*YSIZE];
  static BitString all_square;
  static int s_visited[XSIZE*YSIZE];
  static signed char reach[ XSIZE * YSIZE ];
  PHYSID pos,fro,old_man;
  char   old_stone;
  int    next_in, next_out, dir;

  memset( s_visited, 0, sizeof( int ) * XSIZE * YSIZE );
  memset( n_pu, 0, sizeof( PHYSID ) * XSIZE * YSIZE );
  memset( n_mo, 0, sizeof( PHYSID ) * XSIZE * YSIZE );

  old_man  = maze->manpos;
  from[0]  = maze->manpos;
  n_pu[maze->manpos]  = -1;
  n_mo[maze->manpos]  = -1;
  stack[0] = start;
  mand[0]  = 0;
  Set0BS(s_squares[0]);
  old_stone= maze->PHYSstone[start];
  maze->PHYSstone[start]=-1;
  next_in  = 1;
  next_out = 0;
  AvoidThisSquare = start;
  while (next_out < next_in) {
    fro = from[next_out];
    pos = stack[next_out++];
    if (maze->PHYSstone[pos] >= 0)
      continue;
    if (s_visited[pos] ) continue;
    s_visited[pos] = 1;
    n_pu[pos]      = n_pu[fro]+1;
    n_mo[pos]      = n_mo[fro]+ mand[next_out-1] +1;
    if (pos==goal) break;

    /* set maze up to make the reach analysis for the man */
    MANTO(maze,fro);
    AvoidThisSquare = pos;
    Moves(maze,movefrom,reach);

    for (dir=NORTH; dir<=WEST; dir++) {
      if (   IsBitSetBS(maze->S[dir],pos)
	     && reach[pos-DirToDiff[dir]]>=0) {
	mand[next_in] = reach[pos-DirToDiff[dir]];
	CopyBS(m_squares[next_in], m_squares[next_out-1]);
	GenAllSquares( pos - DirToDiff[ dir ], movefrom, all_square );
	BitOrEqBS( m_squares[next_in], all_square );
	from[next_in] = pos;
	CopyBS(s_squares[next_in], s_squares[next_out-1]);
	SetBitBS(s_squares[next_in],pos);
	stack[next_in++] = pos +DirToDiff[dir];
      }
    }
  }
  MANTO(maze,old_man);
  maze->PHYSstone[start]=old_stone;
  AvoidThisSquare = 0;
  MarkReach(maze);
  *pushes = n_pu[goal];
  *moves  = n_mo[goal];
  CopyBS(stone_squares,s_squares[next_out-1]);
  SetBitBS(stone_squares,pos);
  CopyBS(man_squares,m_squares[next_out-1]);
  SetBitBS(man_squares,fro);
}

void PushesMoves2(MAZE *maze, PHYSID start, PHYSID goal,
		 int *pushes, int *moves,
		 BitString stone_squares, BitString man_squares)
{
  static PHYSID stack[XSIZE*YSIZE];
  static PHYSID from[XSIZE*YSIZE];
  static PHYSID mand[XSIZE*YSIZE];
  static PHYSID n_pu[XSIZE*YSIZE];
  static PHYSID n_mo[XSIZE*YSIZE];
  static PHYSID movefrom[ XSIZE * YSIZE ];
  static BitString s_squares[XSIZE*YSIZE];
  static BitString m_squares[XSIZE*YSIZE];
  static BitString all_square;
  static int s_visited[XSIZE*YSIZE];
  static signed char reach[ XSIZE * YSIZE ];
  PHYSID pos,fro,old_man;
  char   old_stone;
  int    next_in, next_out, dir;

  memset( s_visited, 0, sizeof( int ) * XSIZE * YSIZE );
  memset( n_pu, 0, sizeof( PHYSID ) * XSIZE * YSIZE );
  memset( n_mo, 0, sizeof( PHYSID ) * XSIZE * YSIZE );

  old_man = maze->manpos;
  from[ 0 ] = maze->manpos;
  n_pu[ maze->manpos ] = -1;
  n_mo[ maze->manpos ] = -1;
  stack[ 0 ] = start;
  mand[ 0 ] = 0;
  Set0BS( s_squares[ 0 ] );
  old_stone = maze->PHYSstone[ start ];
  maze->PHYSstone[ start ] = -1;
  next_in = 1;
  next_out = 0;
  AvoidThisSquare = start;
  while( next_out < next_in ) {
    fro = from[ next_out ];
    pos = stack[ next_out++ ];
    if( maze->PHYSstone[ pos ] >= 0 )
      continue;
    if( s_visited[ pos ] ) continue;
    s_visited[ pos ] = 1;
    n_pu[ pos ] = n_pu[ fro ] + 1;
    n_mo[ pos ] = n_mo[ fro ] + mand[ next_out - 1 ] + 1;
    if( pos == goal )
      break;

    /* set maze up to make the reach analysis for the man */
    MANTO( maze, fro );
    AvoidThisSquare = pos;
    Moves2( maze, movefrom, reach, IdaInfo->shadow_stones );

    /* try to avoid shadow stones */
    for( dir = NORTH; dir <= WEST; dir++ ) {
      if( IsBitSetBS( maze->S[ dir ], pos ) &&
	  reach[ pos - DirToDiff[ dir ] ] >=0 &&
	  !s_visited[ pos + DirToDiff[ dir ] ] ) {
	mand[ next_in ] = reach[ pos - DirToDiff[ dir ] ];
	CopyBS( m_squares[ next_in ], m_squares[ next_out - 1 ] );
	GenAllSquares( pos - DirToDiff[ dir ], movefrom, all_square );
	BitOrEqBS( m_squares[ next_in ], all_square );
	from[ next_in ] = pos;
	CopyBS( s_squares[ next_in ], s_squares[ next_out - 1 ] );
	SetBitBS( s_squares[ next_in ], pos );
	stack[ next_in++ ] = pos + DirToDiff[ dir ];
	s_visited[ pos + DirToDiff[ dir ] ] = 1;
      }
    }
  }
  MANTO( maze, old_man );
  maze->PHYSstone[ start ] = old_stone;
  AvoidThisSquare = 0;
  MarkReach( maze );
  *pushes = n_pu[ goal ];
  *moves = n_mo[ goal ];
  CopyBS( stone_squares, s_squares[ next_out - 1 ] );
  SetBitBS( stone_squares, pos );
  CopyBS( man_squares, m_squares[ next_out - 1 ] );
  SetBitBS( man_squares, fro );
}

int ValidSolution(MAZE *maze, MOVE *solution) {

	int number_pushes, number_moves, i;
	int p,m;
	UNMOVE unmove;
	BitString stone,man;

	number_pushes = number_moves = 0;
	i = 0;
	while (!ISDUMMYMOVE(solution[i])) {
		p = m = 0;
		PushesMoves(maze,solution[i].from,solution[i].to,
			    &p,&m,stone,man);
		number_pushes += p;
		number_moves  += m;
		MakeMove(maze,&(solution[i]),&unmove, ENDPATH);
		i++;
	}
	Mprintf(0,"Moves: %i, Pushes: %i, Treedepth: %i\n",
		number_moves,number_pushes,i);
	if (maze->h!=0)
		return(0);
	else return(number_pushes);

}


int DiffToDir(int diff)
{
	switch (diff) {
	case -1:
		return(SOUTH);
	case  1:
		return(NORTH);
	case -YSIZE:
		return(WEST);
	case YSIZE:
		return(EAST);
	case +1+YSIZE:
		return(NORTHEAST);
	case -1+YSIZE:
		return(SOUTHEAST);
	case -1-YSIZE:
		return(SOUTHWEST);
	case +1-YSIZE:
		return(NORTHWEST);
	default:
		return(NODIR);
	}
}

int calculateBoxPath(MAZE *maze, PHYSID start, PHYSID goal, PHYSID lastFrom, PHYSID path[]) { /* BD */
    /* calculates a box path from "start" to "goal" ending with the player on "lastFrom".
       preconditions:
       * the player's reachable squares have been calculated.
       * "path[]" is large enough to contain a path of maximum length + 1.
    */
    static PHYSID queueFrom[MAZESIZE];
    static PHYSID queueTo[MAZESIZE];
    static PHYSID queueParent[DIRECTION_COUNT][MAZESIZE];
    signed char originalBoxIndex;
    PHYSID square, from, to, originalPlayerSquare, temporary;
    int head, tail, direction, pushCount, index;

    for (square = 0; square < MAZESIZE; square++) {
        for (direction = NORTH; direction <= WEST; direction++) {
            queueParent[direction][square] = UNDEFINED;
        }
    }

    /* set initial "from" to a player-reachable square next to the box */
    queueFrom[0] = UNDEFINED;
    for (direction = NORTH; direction <= WEST; direction++) {
        if (IsBitSetBS(maze->reach, start + DirToDiff[direction])) {
            queueFrom[0] = start + DirToDiff[direction];
            break;
        }
    }
    if (queueFrom[0] == UNDEFINED) {
       return 0;
    }

    originalPlayerSquare = maze->manpos;
    originalBoxIndex = maze->PHYSstone[start];
    maze->PHYSstone[start] = UNDEFINED;
    queueTo[0] = start;
    head = -1;
    tail = 0;

    /* BFS (breadth-first search) */
    while ((++head) <= tail) {
        from = queueFrom[head];
        to = queueTo[head];

        if (maze->PHYSstone[ to ] > UNDEFINED) continue;

        if ((to == goal) && (from == lastFrom)) {
           break;
        }

        MANTO(maze, from);
        AvoidThisSquare = to;
        MarkReach(maze);

        for (direction = NORTH; direction <= WEST; direction++) {
            square = to + DirToDiff[direction];
            if (IsBitSetBS(maze->S[direction], to) &&
                IsBitSetBS(maze->reach, to - DirToDiff[direction]) &&
                (queueParent[direction][square] == UNDEFINED)) {
                queueParent[direction][square] = head;
                queueFrom[++tail] = to;
                queueTo[tail] = square;
            }
        }
    } // end while

    /* restore game state */
    MANTO(maze, originalPlayerSquare);
    maze->PHYSstone[start] = originalBoxIndex;
    AvoidThisSquare = 0;
    MarkReach(maze);

    /* path reconstruction */
    pushCount = 0;

    if (head <= tail) { /* true: found a path */
      index = head;
      do {
        from = queueFrom [ index ];
        to = queueTo[ index ];
        path[ pushCount++ ] = to;
        index = queueParent[ DiffToDir( to - from ) ] [ to ];
      } while ( index > 0 );

      path[ pushCount++ ] = start; /* append the start square */

      // reverse the path. it was built backwards
      for (index = 0; index < pushCount / 2; index++) {
        temporary = path[index];
        path[index] = path[pushCount - 1 - index];
        path[pushCount - 1 - index] = temporary;
      }

     pushCount--; /* discount the start square */
    }

    return pushCount; /* return the number of pushes */
}
