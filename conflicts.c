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

#define CFLTS_PPLEVEL 6

void InitConflicts(CONFLICTS *c)
{
	SR(Debug(CFLTS_PPLEVEL,0,"InitConflicts\n"));
	c->number_patterns  = 0;
	c->number_removed   = 0;
	c->number_killed    = 0;
	c->number_penalties = 0;
	c->array_size_pen   = 0;
	c->pen              = NULL;

	c->number_deadtested 	 = 0;
	c->array_size_deadtested = 0;
	c->deadtested		 = NULL;

	c->number_pentested	 = 0;
	c->array_size_pentested  = 0;
	c->pentested		 = NULL;

	memset(c->penalty_hist,0,sizeof(long)*MAX_PENHIST);
	memset(c->penalty_depth,0,sizeof(long)*MAX_DEPTH);
}

void DelConflicts(CONFLICTS *c)
{
	int i;
	SR(Debug(CFLTS_PPLEVEL,0,"DelConflicts\n"));
	for (i=0; i<c->number_penalties; i++) My_free(c->pen[i].cflts);
	My_free(c->pen);
	My_free(c->deadtested);
	My_free(c->pentested);
	InitConflicts(c);
}

void AddPenalties(CONFLICTS *c)
/* just increase the size of the penalty array by CONFLICT_INC */
{
	SR(Debug(CFLTS_PPLEVEL,0,"AddPenalties: (%i to %i)\n",
		c->array_size_pen,c->array_size_pen+CONFLICT_INC));
	c->pen=My_realloc(c->pen,
			  sizeof(PENALTY)*(c->array_size_pen+CONFLICT_INC));
	c->array_size_pen += CONFLICT_INC;
}

void InsertPenalty(CONFLICTS *c, int i, int p)
/* Insert a new penalty struct at position i for penalty p */
{
	SR(Debug(CFLTS_PPLEVEL,0,"InsertPenalty: at %i for pen: %i\n",i,p));
	if (c->array_size_pen<=c->number_penalties) AddPenalties(c);
	memmove(c->pen+i+1,c->pen+i,
		sizeof(PENALTY)*(c->number_penalties-i));
	InitPenalty(c->pen+i,p);
	c->number_penalties++;
}

void InitPenalty(PENALTY *p, int penalty)
/* init penalty structure */
{
	SR(Debug(CFLTS_PPLEVEL,0,"InitPenalty: pen: %i\n",penalty));
	p->penalty = penalty;
	p->array_size = 0;
	p->number_conflicts = 0;
	p->cflts = NULL;
}

PENALTY *FindPenalty(CONFLICTS *c, int penalty)
/* Find a CFLTS in conflicts.pen do nothing if there otherwise insert one */
{
	int i;

	SR(Debug(CFLTS_PPLEVEL,0,"FindPenalty: pen: %i\n",penalty));
	SR(Assert((((penalty&1)==0) && penalty>=2) || penalty <=ENDPATH,
		"FindPenalty: Penalty wrong: %i", penalty));
	i=0;
	while (  (i < c->number_penalties)
	       &&(penalty < c->pen[i].penalty)) {
		i++;
	}
	if (i>=c->number_penalties || penalty != c->pen[i].penalty) {
		/* We have to create this entry by inserting it */
		InsertPenalty(c,i,penalty);
	}
	return(c->pen+i);
}

void AddConflicts(PENALTY *p)
{
	SR(Debug(CFLTS_PPLEVEL,0,"AddConflicts: (%i to %i)\n",
		p->array_size,p->array_size+CONFLICT_INC));
	p->cflts=My_realloc(p->cflts,sizeof(CFLT)*(p->array_size+CONFLICT_INC));
	p->array_size += CONFLICT_INC;
}

/* 3 if a == b, 1 if a is a subset of b, 2 if b is a subset of a,
   0 otherwise */
int SubsetBS( BitString a, BitString b )
{
  int ab = 1, ba = 2, i;


  for( i = 0; i < NUMBERINTS; i++ ) {
    if( a[ i ] & ~b[ i ] )
      ab = 0;
    if( b[ i ] & ~a[ i ] )
      ba = 0;
    if( !ab && !ba )
      return 0;
  }
  return ab | ba;
}

void RemoveConflict( CONFLICTS *c, int peni, int coni)
{
    if ( coni < c->pen[ peni ].number_conflicts ) {
        memcpy( &c->pen[ peni ].cflts[ coni ],
	    &c->pen[ peni ].cflts[ --c->pen[ peni ].number_conflicts ],
	    sizeof( CFLT ) );
    }
}

int RemoveDuplicates( CONFLICTS *c, int penalty,
		      BitString pattern, BitString no_reach )
{
  int peni, coni;

  for( peni = 0; peni < c->number_penalties; peni++ ) {
    for( coni = 0; coni < c->pen[ peni ].number_conflicts; coni++ ) {
	if( c->pen[ peni ].penalty < penalty ) {
	  if(    SubsetBS( c->pen[peni].cflts[coni].conflict, pattern ) > 1
	     && !LogAndNotBS( no_reach, c->pen[peni].cflts[coni].no_reach )) {
	    RemoveConflict( c, peni, coni);
	    c->number_removed++;
	    coni--;	/* make sure we look at the one moved here too */
	  }
	} else if( c->pen[ peni ].penalty == penalty ) {
	  switch( SubsetBS( c->pen[ peni ].cflts[ coni ].conflict,
			    pattern ) ) {
	  case 1:
	  case 3:
	     if (!LogAndNotBS( c->pen[peni].cflts[coni].no_reach, no_reach )) {
	    	c->number_removed++;
	    	return 0;
	     }
	     break;
	  case 2:
	     if (!LogAndNotBS( no_reach, c->pen[peni].cflts[coni].no_reach )) {
	        RemoveConflict( c, peni, coni);
	        c->number_removed++;
	        coni--;	/* make sure we look at the one moved here too */
	     }
	  }
	} else {
	  switch( SubsetBS( c->pen[peni].cflts[coni].conflict, pattern )) {
	  case 1:
	  case 3:
	     if (!LogAndNotBS( c->pen[peni].cflts[coni].no_reach, no_reach )) {
	         c->number_removed++;
	         return 0;
	     }
	  }
	}
    }
  }
  return 1;
}

void RemoveWorst( CONFLICTS *c )
/* Find worst pattern, least used, and remove it */
{
  int   peni, coni, w_peni, w_coni;
  long  worst_t_used;
  w_peni = 0;
  w_coni = 0;
  worst_t_used = total_node_count;

  if (Options.limit_pat==NO) return;
  for( peni = 0; peni < c->number_penalties; peni++ ) {
    for( coni = 0; coni < c->pen[ peni ].number_conflicts; coni++ ) {
	/* if this conflict is worse, remember */
	if (   worst_t_used > c->pen[peni].cflts[coni].t_used
	    && c->pen[peni].cflts[coni].n_used < MIN_USED
	    && c->pen[peni].cflts[coni].t_created<total_node_count-REM_DELAY) {
	  w_peni = peni;
	  w_coni = coni;
	  worst_t_used = c->pen[peni].cflts[coni].t_used;
	}
    }
  }
  /* PrintConflict( c, w_peni, w_coni ); */
  if (w_peni!=0 || w_coni!=0) {
	RemoveConflict( c, w_peni, w_coni );
	c->number_killed++;
	c->number_patterns--;
  }
}

void InsertConflict(PENALTY *p, BitString c, BitString no_reach)
/* Insert c and no_reach into the PEANLTY structure */
{
	SR(Debug(CFLTS_PPLEVEL,0,"InsertConflict\n"));

/*
printf("InsertConflict: penalty: %d\n",p->penalty);
PrintBit3Maze(IdaInfo->IdaMaze,c,no_reach,0);
*/

	if (p->array_size<=p->number_conflicts) AddConflicts(p);
	CopyBS(p->cflts[p->number_conflicts].conflict,c);
	CopyBS(p->cflts[p->number_conflicts].no_reach,no_reach);
	p->cflts[p->number_conflicts].n_used=0;
	p->cflts[p->number_conflicts].t_created=total_node_count;
	p->cflts[p->number_conflicts].t_used=0;
	p->cflts[p->number_conflicts].onestone = FindFirstSet( c );
	p->number_conflicts++;
}

void AddConflict(CONFLICTS *c,BitString conflict,BitString no_reach,int penalty)
/* Penalty is total penalty, not relative */
{
  int      n;

  SR(Debug(CFLTS_PPLEVEL,0,"AddConflict\n"));
  if (penalty==0) return;

  n = NumberBitsBS(conflict);
  /* return if all stones are included, basically minimization failed */
  if (n==IdaInfo->IdaMaze->number_goals) return;
  if (n<2) {
     Mprintf( 0, "ALARM: Pattern wrong!\n");
     PrintBit3Maze(IdaInfo->IdaMaze,conflict,no_reach,0);
  }
  if (RemoveDuplicates( c, penalty, conflict, no_reach ) ) {
	if (c->number_patterns >= PATTERN_LIMIT) RemoveWorst( c );
        InsertConflict(FindPenalty(c,penalty),conflict,no_reach);
	c->number_patterns++;
  }
}

int  GetPriorPostPen(MAZE *maze, int penalty, int *prior, int *post)
{
	int i;

	for (i=0;
	        i<maze->conflicts->number_penalties
	     && maze->conflicts->pen[i].penalty != penalty;
	     i++ ) ;

	if ( i >= maze->conflicts->number_penalties ) return(0);

	if ( i == 0 ) *prior = -1;
	else *prior = maze->conflicts->pen[ i - 1 ].penalty;

	if ( i == maze->conflicts->number_penalties - 1 ) *post = -1;
	else *post = maze->conflicts->pen[ i + 1 ].penalty;

	return( 1 );
}

void PrintConflicts(MAZE *maze, CONFLICTS *c)
{
	int	  peni;
	int       coni;
	PHYSID    old_manpos = maze->manpos;

	Mprintf( 0, "Number of patterns: %d\n", c->number_patterns);
	for (peni=0; peni<c->number_penalties; peni++) {
		Mprintf( 0, "Penalty: %i #confl: %d\n", c->pen[peni].penalty,
			c->pen[peni].number_conflicts);
		for (coni=0; coni<c->pen[peni].number_conflicts; coni++) {
			PrintConflict( c, peni, coni );
		}
	}
	Mprintf( 0, "Number of deadtested: %d\n", c->number_deadtested);
	for (peni=0; peni<c->number_deadtested; peni++) {
		maze->manpos = c->deadtested[peni].manpos;
		PrintBit3Maze(maze,c->deadtested[peni].stones,
				   c->deadtested[peni].relevant,0);
	}
	Mprintf( 0, "Number of pentested: %d\n", c->number_pentested);
	for (peni=0; peni<c->number_pentested; peni++) {
		maze->manpos = c->pentested[peni].manpos;
		PrintBit3Maze(maze,c->pentested[peni].stones,
				   c->pentested[peni].relevant,0);
	}
	Mprintf( 0, "penalty: count; ");
	for (peni=0; peni<MAX_PENHIST; peni+=2) {
		if (c->penalty_hist[peni]>0)
			Mprintf( 0, "%d:%ld, ", peni, c->penalty_hist[peni]);
	}
	Mprintf( 0, "\ndepth: count; ");
	for (peni=0; peni<MAX_DEPTH; peni++) {
		if (c->penalty_depth[peni]>0)
			Mprintf( 0, "%d:%ld, ", peni, c->penalty_depth[peni]);
	}
	Mprintf( 0, "\n");
	maze->manpos = old_manpos;
}

void AddTestedPen(CONFLICTS *c, BitString relevant, BitString stones,
				PHYSID manpos, PHYSID stonepos, int goodtested)
{
	TESTED *pen;
	if (Options.st_testd==0 || NumberBitsBS(relevant) == 0) return;
	if (c->number_pentested >= CONFLICT_RESET) {
		/* We have too many of these, remove them all, start over */
		c->number_pentested = 0;
	} else if (c->array_size_pentested <= c->number_pentested) {
		c->pentested=My_realloc(c->pentested,
			sizeof(CFLT)*(c->array_size_pentested+CONFLICT_INC));
		c->array_size_pentested += CONFLICT_INC;
	}
	pen = &(c->pentested[c->number_pentested]);
	pen->manpos = manpos;
	pen->stonepos = stonepos;
	pen->goodtested = goodtested;
	CopyBS(pen->relevant,relevant);
	CopyBS(pen->stones,stones);
	BitOrEqBS(pen->stones,IdaInfo->IdaMaze->goal);
	c->number_pentested++;
}

void AddTestedDead(CONFLICTS *c, BitString relevant, BitString stones,
				PHYSID manpos, PHYSID stonepos)
{
	TESTED *dead;
	if (Options.st_testd==0 || NumberBitsBS(relevant) == 0) return;
	if (c->number_deadtested >= CONFLICT_RESET) {
		/* We have too many of these, remove them all, start over */
		c->number_deadtested = 0;
	} else if (c->array_size_deadtested <= c->number_deadtested) {
		c->deadtested=My_realloc(c->deadtested,
			sizeof(CFLT)*(c->array_size_deadtested+CONFLICT_INC));
		c->array_size_deadtested += CONFLICT_INC;
	}
	dead = &(c->deadtested[c->number_deadtested]);
	dead->manpos = manpos; dead->stonepos = stonepos;
	CopyBS(dead->relevant,relevant);
	CopyBS(dead->stones,stones);
	c->number_deadtested++;
}

int WasTestedPen(CONFLICTS *c, BitString stones,
		 BitString reach, PHYSID stonepos)
{
/* was this or a similar position tested before? Yes if
	1) man square on same square AND
	2) stone on same square AND
	3) no stone is on relevant squares that was not before.
Play with what are relevant squares (touched for a solution? close to the
stones in the fianlpattern???) */
/* return position+1 of pattern tested */
/* 0 if was not searched before */

	int i;

	if (Options.st_testd==0) return(0);
	for (i=0; i<c->number_pentested; i++) {
		if (  (IsBitSetBS(reach,c->pentested[i].manpos))
		    &&(stonepos==c->pentested[i].stonepos)
		    &&(!LogAndAndNotBS(stones, c->pentested[i].relevant,
				c->pentested[i].stones))) {
			return(i+1);
		}
	}
	return(0);
}

int WasTestedDead(CONFLICTS *c, BitString stones,
		  BitString reach, PHYSID stonepos)
{
	int i;

	if (Options.st_testd==0) return(0);
	for (i=0; i<c->number_deadtested; i++) {
		if (  (stonepos==c->deadtested[i].stonepos)
		    &&(IsBitSetBS(reach,c->deadtested[i].manpos))
		    &&(!LogAndAndNotBS(stones, c->deadtested[i].relevant,
				c->deadtested[i].stones))) {
			return(i+1);
		}
	}
	return(0);
}

void PrintConflict( CONFLICTS *c, int peni, int coni )
{
	Mprintf( 0, "Pattern %d, pen: %d\n", coni, c->pen[peni].penalty);
	Mprintf( 0, "n_used: %ld, t_created: %ld, t_used: %ld\n",
		c->pen[peni].cflts[coni].n_used,
		c->pen[peni].cflts[coni].t_created,
		c->pen[peni].cflts[coni].t_used);
	PrintBit3Maze(IdaInfo->IdaMaze,
		c->pen[peni].cflts[coni].conflict,
		c->pen[peni].cflts[coni].no_reach,0);
}

void PrintTested(MAZE *maze, int num)
{
	PrintBit3Maze(maze,
		maze->conflicts->pentested[num].stones,
		maze->conflicts->pentested[num].relevant,
		maze->conflicts->pentested[num].manpos);
}

void  PrintMatches(MAZE *maze)
    /* This is the way we did GetPenalty first. Quick and dirty and we hope,
     * not very efficient, since we are trying to do it "better" */
{
	int	  peni, coni, penalty, count;
	CONFLICTS *c;
	BitString stones;
	PHYSID    manpos;

	penalty = 0;
	count = 0;
	manpos = maze->manpos;
	c      = maze->conflicts;
	CopyBS(stones,maze->stone);

	for (peni=0; peni<c->number_penalties; peni++) {
		for (coni=0; coni<c->pen[peni].number_conflicts; coni++) {
		    count++;
		    if (   IsBitSetBS(stones,c->pen[peni].cflts[coni].onestone)
			&& !IsBitSetBS( c->pen[ peni ].cflts[ coni ].no_reach,
				      manpos )
			&& AllBitsSetBS(stones,
				      c->pen[peni].cflts[coni].conflict) ) {
		      penalty += c->pen[peni].penalty;
printf( "GetPenalty hit: peni: %d, coni: %d, penalty: %d\n", peni,coni, c->pen[peni].penalty);
			PrintBit3Maze(IdaInfo->IdaMaze,
				c->pen[peni].cflts[coni].conflict,
				c->pen[peni].cflts[coni].no_reach,0);
			if (penalty>=ENDPATH) {
				SR(Debug(CFLTS_PPLEVEL,0,
					"GetPenalty: Deadlock found\n"));
				goto END_GETPEN;
			}
		    }
		}
	}
END_GETPEN:
	SR(Debug(CFLTS_PPLEVEL,0,"GetPenalty: pen found: %i (s:%i)\n",
		penalty,count));
}

#define BitType unsigned long
#define BitSize (sizeof(BitType)*8)
#define BitSet(bs,bitnumber)   		((bs)  |=  (((BitType)1)<<bitnumber))
#define BitSet0(bs)	      		((bs)   =  0)
#define BitUnSet(bs,bitnumber) 		((bs)  &= ~(((BitType)1)<<bitnumber))
#define BitsNot0(bs)	      		((bs)  !=  0)
#define BitCopy(bs1, bs2)		((bs1)  =  (bs2))
#define BitOrEq(bs1, bs2)		((bs1) |=  (bs2))
#define BitAndNot(bs1,bs2,bs3)		((bs1)  =  (bs2) &~ (bs3))
#define BitAndNotEq(bs1,bs2)		((bs1) &= ~(bs2))

int BitNext(BitType bs)
    /* returns -1 for no bits left */
    /* does not find "smallest" bitfirst!!! */
{
/*
    int i;
    char *a;
    a = (char*) &bs;

    for (i=0; i<sizeof(BitType); i++) {
	if ( BitFirst[(int)a[i]] > -1 )
	    return(BitFirst[(int)a[i]]+8*i);
    }
    return(-1);
*/
    unsigned long x;

    if( (x = (bs & 0x000000ff)) != 0 )
	    return( BitFirst[ x >> 0 ] + 0 );
    if( (x = (bs & 0x0000ff00)) != 0 )
	    return( BitFirst[ x >> 8 ] + 8 );
    if( (x = (bs & 0x00ff0000)) != 0 )
	    return( BitFirst[ x >> 16 ] + 16);
    if( (x = (bs & 0xff000000)) != 0 )
	    return( BitFirst[ x >> 24 ] + 24);
    return( -1 );
}

void FindTransitiveClosure(int pattern, BitType *patterns,
	BitType *conflict_table)
{
    BitType	tested, to_test;

    BitSet0(*patterns);
    BitSet0(tested);
    BitSet0(to_test);
    BitSet(to_test,pattern);
    do {
	BitSet(tested,pattern);
	BitOrEq(*patterns,conflict_table[pattern]);
	BitAndNot(to_test,*patterns,tested);
	pattern = BitNext(to_test);
    } while (pattern != -1);
}

int Maximize(BitType open, BitType *used,
	     BitType *conflict_table, int *penalties)
    /* This routine tries to maximize the set covering problem of the patterns
     * entered and returns the maximal penalty we can obtain
     * open: the patterns that can still be included without conflict */
{
    /* works like a tree search in which every node represents certain
     * patterns included in the penalty set */
    int max_penalty, penalty;
    int pattern;
    BitType new_open,tmp_used;

    max_penalty = 0;
    pattern = BitNext(open);
    tmp_used = 0;
    while (pattern != -1) {
	    /* for each pattern left */
	    BitAndNot(new_open,open,conflict_table[pattern]);
	    penalty = Maximize(new_open,&tmp_used,conflict_table,penalties)
		    + penalties[pattern];
	    if (penalty > max_penalty) {
		max_penalty = penalty;
		*used = tmp_used | pattern;
	    }

	    BitUnSet(open,pattern);
	    pattern = BitNext(open);
    }
    return( max_penalty );
}

int  GetPenalty(MAZE *maze, int targetpen)
{
	int	  peni;
	int       coni;
	int 	  total_penalty, hoverestim;
	int	  pattern;		/* pattern index */
	BitType   patterns_all;		/* all the patterns in a bitstring */
	BitType   patterns_left;	/* all the patterns in a bitstring */
	BitType   bpatterns;		/* tmp variable */
	BitType   used,tmp_used;	/* which are used? points */

	int	  match_count;		/* number of patterns matched */
	CFLT     *cflts[BitSize];	/* all the matching patterns */
	int	  penalties[BitSize];	/* penalties to corresponding pattern */
	CONFLICTS *c = maze->conflicts;

	char	  conflict_count[BitSize];
					/* How many patterns conflict with
					 * this one */
	BitType	  conflict_table[BitSize];
					/* Bit pattern representing with which
					 * this pattern conflicts. */
	BitString stones;

	/* For overestimation */
	BitString used_stones;
	PHYSID    pos;
	float     penalty,this_pen,p;


	if (Options.lazy_max==NO) targetpen = ENDPATH;
	SR(Debug(CFLTS_PPLEVEL,0, "GetPenalty: target: %i\n",targetpen));

	BitAndNotButOrBS(stones,maze->stone,maze->goal,maze->stones_done);

	/* If there is a stone on a non-goal fixed, return deadlock */
	if (LogAndNotAndNotBS(maze->stones_done,maze->goal,maze->out)) {
		maze->h -= maze->pen;
		maze->pen = ENDPATH;
		maze->h += maze->pen;
		return(ENDPATH);
	}

	if (Options.lb_cf==0) {
		maze->h -= maze->pen;
		maze->pen = 0;
		return(0);
	}

	/* find all the patterns, save the stones bitmap in the patterns and the
	 * penalties in the penalties array. */
	match_count = 0;
	BitSet0(patterns_all);
	Set0BS(used_stones);
	for (peni=0; peni<c->number_penalties; peni++) {
		for (coni=0; coni<c->pen[peni].number_conflicts; coni++) {
		    if ( IsBitSetBS( stones,
				     c->pen[peni].cflts[coni].onestone ) &&
			 !IsBitSetBS( c->pen[ peni ].cflts[ coni ].no_reach,
				      maze->manpos ) &&
			 AllBitsSetBS(stones,
				      c->pen[peni].cflts[coni].conflict) ) {
			if (c->pen[peni].penalty>=targetpen) {
				c->pen[peni].cflts[coni].n_used++;
				c->pen[peni].cflts[coni].t_used=
					total_node_count;
				SR(Debug(CFLTS_PPLEVEL,0,
				     "GetPenalty: early pen found: %i (s:%i)\n",
				     c->pen[peni].penalty,match_count+1));
				maze->h   -= maze->pen;
				maze->pen  = c->pen[peni].penalty;
				maze->h   += maze->pen;
				return(maze->pen);
			}
			penalties[match_count] = c->pen[peni].penalty;
			cflts[match_count]  = &(c->pen[peni].cflts[coni]);
			BitSet(patterns_all,match_count);
			match_count++;
			BitOrEqBS(used_stones,
				c->pen[peni].cflts[coni].conflict);
			if (match_count >= BitSize) goto ENDLOOP;
		    }
		}
	}
ENDLOOP:
	/* Handle the trivia */
	BitSet0(used);
	if (match_count == 0) {
		total_penalty = 0;
	} else if (match_count == 1) {
		total_penalty = penalties[0];
		BitSet(used,0);
	} else if (match_count == 2) {
	    if (LogAndBS(cflts[0]->conflict,cflts[1]->conflict)) {
		if (penalties[0] > penalties[1]) {
			total_penalty = penalties[0];
			BitSet(used,0);
		} else {
			total_penalty = penalties[1];
			BitSet(used,1);
		}
	    } else {
		total_penalty = penalties[0]+penalties[1];
		BitSet(used,0);
		BitSet(used,1);
	    }
	} else {

	    /* Prepare the conflict arrays, Zero only what is needed */
	    memset(conflict_count,0,sizeof(char)*match_count);
	    memset(conflict_table,0,sizeof(BitType)*match_count);
	    for (peni=0; peni<match_count; peni++) {
		/* Set the pattern conflicting with itself in bit pattern */
		BitSet(conflict_table[peni],peni);
		for (coni=peni+1; coni<match_count; coni++) {
			/* See if those two conflict */
		        if (LogAndBS(cflts[peni]->conflict,
				     cflts[coni]->conflict)) {
			    /* They conflict */
			    BitSet(conflict_table[peni],coni);
			    BitSet(conflict_table[coni],peni);
			    conflict_count[peni]++;
			    conflict_count[coni]++;
			}
		}
	    }

	    /* Repeatedly, find transitive closure of stone packs,
	     * remember those already included to avoid circular references,
	     * start from beginning, pass those on to the maximizer */

	    total_penalty = 0;
	    BitCopy(patterns_left,patterns_all);
	    BitAndNotEq(patterns_left,used);
	    while (BitsNot0(patterns_left)) {
		/* find next conflict */
		pattern = BitNext(patterns_left);
		if (conflict_count[pattern] == 0) {
		    /* simple penalty */
		    total_penalty += penalties[pattern];
		    BitUnSet(patterns_left,pattern);
		    used |= pattern;
		} else {
		    /* conflict pack */
		    FindTransitiveClosure(pattern,&bpatterns,conflict_table);
		    tmp_used = 0;
		    total_penalty += Maximize(bpatterns, &tmp_used,
					      conflict_table, penalties);
		    BitAndNotEq(patterns_left,bpatterns);
		    used |= tmp_used;
		}
		if (total_penalty >= targetpen) {
			break;
		}
	    }
	}

	if (total_penalty >= targetpen) {
		pattern = 0;
		while (BitsNot0(used)) {
			if (used&1) {
			    cflts[pattern]->n_used++;
			    cflts[pattern]->t_used=total_node_count;
			}
			pattern++;
			used >>= 1;
		}
	} else if (   Options.overestim != 0.0
		   && IdaInfo == &MainIdaInfo ) {
	    /* only if we did not reach targetpen, try to overestimate */
	    /* overestimation here works with giving the max  penalty to each
	     * stone for each pattern it is involved in, the penalty is the
	     * maximum of all the penalties/stones */

		penalty = 0;
		for (peni=0; peni<maze->number_stones; peni++) {
		    pos = maze->stones[peni].loc;
		    if (IsBitSetBS(used_stones,pos)) {
			this_pen = 0;
			for (coni=0; coni<match_count; coni++) {
			    if (IsBitSetBS(cflts[coni]->conflict,pos)) {
				p = ((float)penalties[coni])/
					NumberBitsBS(cflts[coni]->conflict);
				if (this_pen<p) this_pen=p;
			    }
			}
			penalty += this_pen;
		    }
		}
		/* penalty -= total_penalty;*/
		penalty *= Options.overestim;
		total_penalty = penalty;
	}

	SR(Debug(CFLTS_PPLEVEL,0,"GetPenalty: pen found: %i (s:%i)\n",
		total_penalty,match_count));
	if (total_penalty&1 && total_penalty!=ENDPATH) total_penalty++;

	maze->h   -= maze->pen;
	if (   Options.hoverestim > 1.0
	    && IdaInfo == &MainIdaInfo
	    && maze->h < ENDPATH) {
		hoverestim = maze->h * (Options.hoverestim - 1.0);
		if (hoverestim&1) hoverestim++;
	} else hoverestim = 0;
	maze->pen  = total_penalty + hoverestim;
	maze->h   += maze->pen;

	return(total_penalty);
}

