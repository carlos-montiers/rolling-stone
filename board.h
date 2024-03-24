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

#include <signal.h>
#include <time.h>
#include <malloc.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define CONFLICT_INC 1000	/* lumb increment of tested array */
#define CONFLICT_RESET 5000	/* number of conflicts after which we set to 0*/
#define PATTERN_LIMIT 800	/* Maximum number of patterns allowed */
#define REM_DELAY     50000	/* Dealy that allows a pattern to prove itself*/
#define MIN_USED      800	/* patterns with that count won't be removed */

#define PATTERNLIMITDIV 4
extern int PATTERNRATIO;
#define RANDOM_MAX 2147483647

#define PENPATTERNSEARCHPP 0
#define DEADPATTERNSEARCHPP 0

#ifndef TRUE
  #define TRUE 		1
  #define FALSE 	0
#endif
#define YES  		1
#define NO    		0


#define	VIRTUAL		0
#define REAL		1

#define MAX(x,y)	((x)>(y)?(x):(y))
#define MIN(x,y)	((x)<(y)?(x):(y))
#define INI		0x7f
#define MAXGOALS	34
#define MAXSTONES	34
#define MAX_MOVES 	70 /* JS */
#define MAX_DEPTH 	700
#define MAX_PENHIST	41
#define MAX_HASHENTRIES 4*2*32768
#define HASHMASK        (MAX_HASHENTRIES -1)
#define GMMAX_HASHENTRIES 8192
#define GMHASHMASK      (GMMAX_HASHENTRIES -1)
#define MAXDIFFICULTY	0x1fff /* This is sort of a sane upper bound */
#define DONEDIFFICULTY	0x7fff /* This is to indicate that we tried before */
#define MAX_SQUARES     25
#define MAX_LOCATIONS   MAXSTONES
#define XY2ID(x,y)	(x*YSIZE+y)
#define ENDPATH		2047
#define MAXDIST         255
#define XSIZE		20
#define YSIZE		16 /* JS */
#define BASETYPE   	long
#define PRINTBASETYPE(a) Mprintf( 0, "%08lx",a);

#define NUMBERBITS 	XSIZE*YSIZE
#define BYTEPERINT 	sizeof(BASETYPE)
#define NUMBERINTS 	(NUMBERBITS/(BYTEPERINT*8)+\
				((NUMBERBITS%(BYTEPERINT*8))?1:0))

#ifdef PC
	#define         DL1PATHFILE "screens/DL.1.pc"
	#define         DL2PATHFILE "screens/DL.2.pc"
#else
	#define         DL1PATHFILE "screens/DL.1"
	#define         DL2PATHFILE "screens/DL.2"
#endif


#define min(a,b)	(((a)<(b))?a:b)
#define max(a,b)	(((a)>(b))?a:b)

#define EMPTY 	0	/* Nothing on square */
#define NONE 	0	/*  */
#define NODIR 	 -1	/* No direction */
#define NORTH	  0	/* Possible to go there? */
#define EAST	  1	/* Possible to go there? */
#define SOUTH	  2	/* Possible to go there? */
#define WEST	  3	/* Possible to go there? */
#define NORTHEAST 4
#define SOUTHEAST 5
#define SOUTHWEST 6
#define NORTHWEST 7

typedef unsigned short USHORT;
typedef          short PHYSID;
/* typedef 	 short DIST;  JS */
typedef unsigned char  DIST;
typedef unsigned long long HASHKEY;

typedef DIST STNDIST[4][XSIZE*YSIZE][XSIZE*YSIZE];	/* dir, from, to */
typedef DIST MANDIST[XSIZE*YSIZE][XSIZE*YSIZE];
typedef char CONN[4][YSIZE*XSIZE];			/* which of the 4
							 * dirs are connected
							 * at this position */

typedef struct {
	char stoneidx; /* JS */	/* referenced by goalidx */
	char goalidx;  /* JS */ /* referenced by stoneidx */
	DIST distance; /* JS */ /* referenced by stoneidx */
} LBENTRY;

typedef BASETYPE BitString[NUMBERINTS];

typedef struct {		/* defines one low level square */
	USHORT  tunnel:6;
	USHORT  min_dim:6;
	USHORT  free:3;
	USHORT  s_free:3;
	signed char   goal;	/* stone idx into stones table */
} PHYS;

typedef struct {
	PHYSID loc;
} STN;

typedef struct {
	PHYSID man;
	PHYSID from;
	PHYSID last_over;
	PHYSID to;
	/* short  macro_id; JS */
	char   macro_id;
	DIST   move_dist;
	short  value;
} MOVE;

#define EQMOVE(m1,m2)	((m1).from==(m2).from&&(m1).to==(m2).to)
#define ISDUMMYMOVE(m)	((m).from==ENDPATH&&(m).to==ENDPATH)

typedef struct {
	STNDIST   *old_s_distances;
	MANDIST   *old_m_distances;
	CONN      *old_connected;
	BitString  old_one_way;
	BitString  old_stones_done;

	PHYSID manfrom;
	PHYSID stonefrom;
	PHYSID stoneto;
	/* short  macro_id; JS */
	char   macro_id;
	DIST   move_dist;
	PHYSID old_closest_confl;
	signed char    old_stoneid; /* JS */
	BitString save_old_no_reach;
        BitString save_no_reach;
        BitString save_reach;
	struct UGLY_GMNODE  *old_GMTree;
} UNMOVE;

typedef struct {
	HASHKEY lock;
	struct UGLY_GMNODE *gmnode;
} GMHASHENTRY;

typedef struct {
	HASHKEY lock;
	unsigned short man:9;
	unsigned short goal_sqto:9;
	unsigned short down:14;	        /* how deeply was it searched */
	unsigned short min_h:14;	/* minimum h seen in this subtree */
	unsigned short areasearched:1;	/* area search done already */
	unsigned short pensearched:1;	/* penalty search done already */
	unsigned short dlsearched:1;	/* deadlock search done already */
	unsigned short pathflag:1;	/* this is a flag indicating part
					   of current path (== cycle)*/
} HASHENTRY;

typedef struct {
	PHYSID loc;
	int    tried;
} GOL;

typedef struct {
	PHYSID from;
	PHYSID to;
	PHYSID last_over;
} MACRO;

typedef struct {
	int    type;
	short  number_macros;
	MACRO  *macros;
} MACROS;

typedef struct {
	PHYSID fw;
	PHYSID bw;
} TOGOAL;

typedef struct {
	short  n;
	short  maninout;
	short  index;
	short  number_stones;		/* all stones, regardless where */
	short  number_stonesongoal;	/* stones on a goal square */
	short  number_goals;
	short  number_squares;
	short  deadentrances;
	HASHKEY hashkey;
	PHYSID locations[MAX_LOCATIONS];
	PHYSID entrances[MAX_LOCATIONS];
	BitString goals;
	BitString squares;
} GROOM;

typedef struct UGLY_GMNODE {
	char       references;
	char       number_entries;
	HASHKEY    hashkey;
	struct UGLY_GMENTRY *entries;
} GMNODE;

typedef struct UGLY_GMENTRY {
	unsigned short new_distances:1;
	unsigned short goal_loc:9;
	unsigned short entrance_loc:9;
	GMNODE  *next;
} GMENTRY;

typedef struct {
	BitString conflict;
	BitString no_reach;
	/* long      n_matched; JS */
	long      n_used;
	long      t_created;
	/* long      t_matched; JS */
	long      t_used;
        PHYSID onestone;
} CFLT;

typedef struct {
	int   number_conflicts;
	int   array_size;
	int   penalty;
	CFLT *cflts;
} PENALTY;

typedef struct {
	int	goodtested;
	PHYSID manpos;
	PHYSID stonepos;
	BitString relevant;
	BitString stones;
} TESTED;

typedef struct {
	BitString    stones_done;
	BitString    one_way;
	STNDIST     *s_distances;
	MANDIST     *m_distances;
	CONN	    *connected;
} DCACHE;

typedef struct {
	int	  number_patterns;
	int	  number_removed; 	/* removed because of dublicates */
	int	  number_killed; 	/* removed because too many patterns */
	int	  number_penalties;
	int 	  array_size_pen;
	PENALTY  *pen;

	int	  number_deadtested;
	int	  array_size_deadtested;
	TESTED   *deadtested;

	int	  number_pentested;
	int	  array_size_pentested;
	TESTED   *pentested;

	long	  penalty_hist[MAX_PENHIST];
	long	  penalty_depth[MAX_DEPTH];
} CONFLICTS;

typedef struct {
		/* first put all the fluent things for fast partial copies */
	BitString    stone;
	BitString    goal;
	BitString    reach;
	BitString    no_reach;
	BitString    old_no_reach;
	BitString    stones_done;	/* stones in goal area for good */
	PHYSID       manpos;
	int          h;
	int          pen;
	int	     g;
	char 	     number_stones;
	HASHKEY      hashkey;
	int	     currentmovenumber;	/* basically index into IDAARRAY */
	signed char  PHYSstone[ XSIZE * YSIZE ]; /* stone idx into stone table*/

	LBENTRY     *lbtable;
	STN         *stones;

		/* changes when stones get fixed on goal squares */
	BitString    one_way;		/* things that change with stones    */
	STNDIST     *s_distances;	/* on goal squares that are fixed    */
	MANDIST     *m_distances;	/* restricting movement.             */
					/* Copies of those pointers are kept */
	CONN	    *connected;		/* in gmnodes for ease of comp.      */
					/* s_distances has distance in
					 * NORTH, if not one_way */

		/* this is all dead weight, will not change during livetime */
	MANDIST     *d_distances;	/* heuristic influence squares */
	BitString    out;
	BitString    wall;
	BitString    dead;
	BitString    M[4];		/* Can the man go there? */
	BitString    S[4];		/* Can the stone move there? */

	GMNODE     **gmtrees;
	short        groom_index[XSIZE*YSIZE];
	MACROS       macros[XSIZE*YSIZE];
	PHYS 	     Phys[XSIZE*YSIZE];
	char         number_grooms;
	char         number_goals;
	GROOM       *grooms;
	GOL         *goals;
	int 	     number_d_cache;
	int 	     size_d_cache;
	PHYSID       goal_manpos;
	DCACHE      *d_cache;
	CONFLICTS   *conflicts;
		/* For the goal-push cutoff, if we could push a stone to a goal
		   using macro we cut all alternatives to pushing that stone */
	PHYSID	     goal_sqto;
} MAZE;

typedef struct {
		/* first put all the fluent things for fast partial copies */
	BitString    stone;
	BitString    goal;
	BitString    reach;
	BitString    no_reach;
	BitString    old_no_reach;
	BitString    stones_done;	/* stones in goal area for good */
	PHYSID       manpos;
	int          h;
	int          pen;
	int	     g;
	short	     number_stones;
	HASHKEY      hashkey;
	int	     currentmovenumber;	/* basically index into IDAARRAY */
	signed char PHYSstone[ XSIZE * YSIZE ]; /* stone idx into stone table*/

		/* now all things that point to fluents */
	LBENTRY     lbtable[MAXSTONES];
	STN         stones[MAXSTONES];
} SAVEMAZE;

typedef struct {
	MOVE   moves[MAX_MOVES];
	MOVE   solution;
	MOVE   currentmove;
	UNMOVE unmove;
	int    number_moves;
	int    currentindex;
	short  distant;
} IDAARRAY;

typedef struct {
	MAZE      *IdaMaze;
	BitString  IdaManSquares;
	BitString  IdaStoneSquares;
	int        Threshold;
	int        ThresholdInc;
	IDAARRAY   IdaArray[MAX_DEPTH];
	long       AbortNodeCount;
	int        ForwDepthLimit;	/* primarily used for   */
	int        base_indent;
	int	   MiniFlag;		/* set to YES in PenMiniConflict */

	int        CurrentSolutionDepth;

	/* functions and datastructures to call/use accoring to what-search */
	HASHENTRY *HashTable;
	int 	  (*MoveOrdering) ();

	/* DeadSearch: "last_move".to location and shortest conflict location */
	PHYSID       goal_last_to;
	PHYSID       closest_confl;
	BitString    shadow_stones;
	BitString    no_reach;
	int	     pattern_node_limit;

	/* stats stuff */
	long	   node_count;	/* number nodes search during this ida */
				/* total_node_count is only reset when a
				 * normal search is setup, therefore
				 * contains ALL nodes searched (real) */
	long       nodes_depth[MAX_DEPTH];

	long       tt_hits;
	long       tt_cols;
	long       tt_reqs;
	long       gmtt_hits;
	long       gmtt_cols;
	long       gmtt_reqs;
	int        dcache_hits;

	int 	   PrintPriority;
	int	   TimeOut;
	int	   TimeOutType;
	int	   TimedOut;
} IDA;

/************ exports ******************/


#include "deadlock.h"
#include "debug.h"
#include "hashtable.h"
#include "ida.h"
#include "init.h"
#include "io.h"
#include "lowerbound.h"
#include "mark.h"
#include "moves.h"
#include "mymem.h"
#include "stats.h"
#include "weights.h"
#include "dl.h"
#include "tree.h"
#include "menu.h"
#include "macro.h"
#include "gtv.h"
#include "bitstring.h"
#include "conflicts.h"
#include "deadsearch.h"
#include "pensearch.h"
#include "histogram.h"
#include "time.h"

extern MOVE DummyMove;
extern int  PP;
extern long area_pos_nc, area_neg_nc;	/* node counts for pos/neg searches */
extern int  area_pos_sc, area_neg_sc;	/* search count for pos/neg */
extern long dl_pos_nc, dl_neg_nc;	/* node counts for pos/neg searches */
extern int  dl_pos_sc, dl_neg_sc;	/* search count for pos/neg */
extern long pen_pos_nc, pen_neg_nc;	/* node counts for pos/neg searches */
extern int  pen_pos_sc, pen_neg_sc;	/* search count for pos/neg */
