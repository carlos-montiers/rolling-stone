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

/* This is a test if the freeze_Dead routine works better than the deadlock
 * table, so this code is an adaptation of the code in soko500.c */

int       opdir[2] = {1, 0};
BitString stones, walls, dead;
int       notongoal;

int Frozen(MAZE *maze, int dir_in, PHYSID pos)
{
    int ret,pos1,pos2,dir;
    /* is the stone frozen in one direction */
    if (!IsBitSetBS(maze->goal,pos)) notongoal = YES;
    ret = NO;
    dir = opdir[dir_in];
    pos1 = pos + DirToDiff[dir];
    pos2 = pos - DirToDiff[dir];

    if (IsBitSetBS(walls,pos1) || IsBitSetBS(walls,pos2)) return(YES);
    if (IsBitSetBS(dead,pos1) && IsBitSetBS(dead,pos2)) return(YES);

    SetBitBS(walls,pos);
    if (IsBitSetBS(stones,pos1) && Frozen(maze,dir,pos1)) ret = YES;
    else if (IsBitSetBS(stones,pos2) && Frozen(maze,dir,pos2)) ret = YES;
    UnsetBitBS(walls,pos);
    
    return(ret);
}

int DeadLock(MAZE *maze, MOVE *move) {
/* We are using global variables for this routine, defined above! */
/* This does not all deadlocks because it might not see a stone not on goal
 * that is frozen. In those cases, we have to make the move and then when we
 * add to stones_done, we see that that happend and return with deadlock */

    CopyBS(stones,maze->stone);
    CopyBS(walls,maze->out);
/* NEW, add as wall what is a stone and stones_done (pattern searches) */
    BitOrAndEqBS(walls,maze->stones_done,maze->stone);

    CopyBS(dead,maze->dead);
    notongoal = NO;
    UnsetBitBS(stones,move->from);
    SetBitBS(stones,move->to);

    if (   Frozen(maze,0,move->to)
	&& Frozen(maze,1,move->to)
        && notongoal == YES)
	    return(YES);
    return(DeadLock2(maze,move));
}


int DeadLockOld(MAZE *maze, MOVE move) {
/* Does not catch this pattern:
    $#
    #
*/

	PHYSID frontdiff,sidediff;

	if (Options.dl_mg==0) return(0);
	if (maze->Phys[move.to].goal >= 0) return(0);
	frontdiff = move.to - move.from;
	if (abs(frontdiff)==1) sidediff=YSIZE;
	else sidediff=1;

	/* check two-wall block */
	if (  (IsBitSetBS(maze->out,move.to+frontdiff))
	    &&(  (IsBitSetBS(maze->out,move.to+sidediff))
	       ||(IsBitSetBS(maze->out,move.to-sidediff))))
		return(1);

	/* check the four-block */
	if (  (IsBitSetBS(maze->out,move.to+frontdiff))
	    ||(maze->PHYSstone[move.to+frontdiff]>=0)) {
		/* check one side */
		if (   (  (IsBitSetBS(maze->out,move.to+sidediff))
			||(maze->PHYSstone[move.to+sidediff]>=0))
		    && (  (IsBitSetBS(maze->out,move.to+sidediff+frontdiff))
			||(maze->PHYSstone[move.to+sidediff+frontdiff]>=0))) 
			return(1);
		sidediff = -sidediff;
		if (   (  (IsBitSetBS(maze->out,move.to+sidediff))
			||(maze->PHYSstone[move.to+sidediff]>=0))
		    && (  (IsBitSetBS(maze->out,move.to+sidediff+frontdiff))
			||(maze->PHYSstone[move.to+sidediff+frontdiff]>=0))) 
			return(1);
	}
	return(0);
}

int DeadLock2(MAZE *maze, MOVE *move) {
	int     dir,dirloop;

	if (Options.dl2_mg==0) return(0);
	if (AvoidThisSquare!=0) return(0);
	if (ISDUMMYMOVE(*move)) return(0);
	if (maze->Phys[move->to].goal >= 0) return(0);
	if (IsBitSetBS(maze->one_way,move->to)) {
		/* the stone lands on a one way square, check if it is
		 * permanently dead */
		/* check for each direction it could be pushed to (using the
		 * for OneWayWeight pointers to determine where the man can
		 * go), 1) if SX is set, 2) if no stone is there 3) no
		 * deadlock is created */
		dir = DiffToDir(move->last_over - move->to);
                if (dir == NODIR) return(0);
		/* If one move can be generated, return NO deadlock */
		for (dirloop=NORTH; dirloop<=WEST; dirloop++) {
			if (   (ConnectedDir(maze,move->to,dir,dirloop))
		            && (IsBitSetBS(maze->S[OppDir[dirloop]],move->to))
		            && ( (move->last_over==move->to-DirToDiff[dirloop])
		                ||(  (maze->PHYSstone[move->to
						     -DirToDiff[dirloop]]<0)
		                   &&(!DeadTree(maze,
						move->to-DirToDiff[dirloop],
						OppDir[dirloop]))))) {
				/* move is possible */
				return(0);
			}
		}
		/* deadlock */
		return(1);
	} 
	return(0);
}

