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

void    PickUpEntrances(MAZE *maze, int gridx);
int     SubMacro(MAZE *maze, MOVE *moves, int *move_number);
void    RemoveGRoom(MAZE *maze, int gridx);
void    FindMacros(MAZE *maze);

PHYSID  FindEndTunnel(MAZE *maze, PHYSID pos, int dir, PHYSID *last_over);
void    AddMacro(MAZE *maze, PHYSID pos, int type, PHYSID from,
		PHYSID last_over, PHYSID to);

int 	StartBuildGMTree(MAZE *start_maze, GROOM *groom);
GMNODE *BuildGMTree(MAZE *maze, GROOM *groom,
		int depth, int allow_null);
void    DelGMTree(GMNODE *gmnode);
int     GoalReach(MAZE *maze, GROOM *groom, int values[MAXGOALS], PHYSID start,
		PHYSID manpos, int index, int optimal, int mark);
void    GetProperties(MAZE *maze, GROOM *groom, int properties[MAXGOALS]);
int     DeadGoal(MAZE *maze, PHYSID pos);
void    FixedGoals(MAZE *maze, int fixed[], int groom_index);
void    Communicating(MAZE *maze, GROOM *groom, int com[MAX_LOCATIONS]);
int     GetBaseProps(MAZE *maze, GROOM *groom, int values[MAXGOALS]);
int     CreatePropGMacro(MAZE *maze, GROOM *groom, GMNODE *gmnode, int depth,
		int values[MAXGOALS], int before[MAXGOALS], int entri,
		int allow_null,BitString considered,int prop,int *sat_prop);
#define CPGM(prop) \
	CreatePropGMacro(maze,groom,gmnode,depth,values,before, \
		entri,allow_null,considered,(prop),&sat_prop)

void    CreateGMacro(MAZE *maze, GROOM *groom, int entri, int goali,
		GMNODE *gmnode, GMNODE *ret_gmnode, int before[MAXGOALS]);
int     ValueSquare(MAZE *maze, GROOM *groom, int goal_index,
		int before[MAXGOALS], int after[MAXGOALS],
		int com_before[MAX_LOCATIONS], int com_after[MAX_LOCATIONS]);
int     ValueSquareEntr(int entri, int reach, int n);
void    PrintMazeValue(MAZE *maze, int index, int values[MAXGOALS]);
void    PrintGoalMacro(MAZE *maze, GROOM *groom, int setpos, int entri,
		int goali, int values[MAXGOALS], int before[MAXGOALS]);
int     FindBestSquare(MAZE *maze, GROOM *groom, int sat_prop, int entri,
		int values[MAXGOALS], int before[MAXGOALS]);
void    PrintSquareProp(int prop);

void InitGRoom(GROOM *groom, int gridx);
void GroomExcPos(MAZE *maze, PHYSID pos, GROOM *groom);
void GroomIncPos(MAZE *maze, PHYSID pos, GROOM *groom);
int  EvaluateGroom(GROOM *groom);
void AsimGoals(MAZE *maze, PHYSID pos, GROOM *groom);
void GrowDFS(MAZE *maze, GROOM *groom, int g);
int  GrowRoom(MAZE *maze, PHYSID pos, GROOM *groom);

extern GMHASHENTRY GMHashTable[GMMAX_HASHENTRIES];

void GMInitHashTable();
void GMStoreHashTable(MAZE *maze, GMNODE *n);
int  GMGetHashTable(HASHKEY key, GMNODE **n);
void GMDelHashEntry(HASHKEY key);

#define AddStone(maze,pos,stonei) \
	stonei = maze->number_stones; maze->stones[stonei].loc = pos; \
	maze->PHYSstone[pos] = stonei; SetBitBS(maze->stone,pos); \
	maze->number_stones++;

#define RemoveStone(maze,pos,stonei) \
	UnsetBitBS(maze->stone,	maze->stones[stonei].loc); \
	maze->PHYSstone[pos] = -1; maze->stones[stonei].loc = 0; \
	maze->number_stones--;

/* a property entry will contain static entries and entrance dep entries,
 * bit 0: fixed stone?		- indep of entrances
 * bit 1: need now distances?	- indep of entrances?
 * bit 2-6: entrance dependent reachability props
 * shifting is only needed of the entrance dep bits.
*/
#define PROP_FIXED      1
#define PROP_NEWDIST    2
#define PROP_NONOBST    4
#define PROP_DEAD       8
#define PROP_NONREACH  16
/* --------------------------- */
#define PROP_STRICT    32
#define PROP_OPTIMAL   64
#define PROP_INSIDE   128
#define PROP_LOOSEST  256
#define PROP_CLOSEST  512
#define PROP_ESHIFT     5

#define PROP_REACH (PROP_STRICT|PROP_OPTIMAL|PROP_INSIDE|PROP_LOOSEST)
#define PROP_REACH_ALL ( PROP_REACH | \
	(PROP_REACH<<PROP_ESHIFT*1) | (PROP_REACH<<PROP_ESHIFT*2) | \
	(PROP_REACH<<PROP_ESHIFT*3))
#define PROP_STRICT_ALL ( PROP_STRICT | \
	(PROP_STRICT<<PROP_ESHIFT*1) | (PROP_STRICT<<PROP_ESHIFT*2) | \
	(PROP_STRICT<<PROP_ESHIFT*3))
#define PROP_CLOSEST_ALL ( PROP_CLOSEST | \
	(PROP_CLOSEST<<PROP_ESHIFT*1) | (PROP_CLOSEST<<PROP_ESHIFT*2) | \
	(PROP_CLOSEST<<PROP_ESHIFT*3))
#define PROP_NONOPT (PROP_INSIDE|PROP_LOOSEST)
#define PROP_NONOPT_ALL ( PROP_NONOPT | \
	(PROP_NONOPT<<PROP_ESHIFT*1) | (PROP_NONOPT<<PROP_ESHIFT*2) | \
	(PROP_NONOPT<<PROP_ESHIFT*3))

#define PROP_0000 ( 0 )
#define PROP_0001 (               PROP_CLOSEST<<(PROP_ESHIFT*entri))
#define PROP_0010 ( PROP_OPTIMAL              <<(PROP_ESHIFT*entri))
#define PROP_0011 ((PROP_OPTIMAL|PROP_CLOSEST)<<(PROP_ESHIFT*entri))
#define PROP_0100 ( PROP_INSIDE               <<(PROP_ESHIFT*entri))
#define PROP_0101 ((PROP_INSIDE |PROP_CLOSEST)<<(PROP_ESHIFT*entri))
#define PROP_0110 ( PROP_STRICT               <<(PROP_ESHIFT*entri))
#define PROP_0111 ((PROP_STRICT |PROP_CLOSEST)<<(PROP_ESHIFT*entri))
#define PROP_1000 (PROP_NONOBST)
#define PROP_1001 (PROP_NONOBST|(               PROP_CLOSEST<<(PROP_ESHIFT*entri)))
#define PROP_1010 (PROP_NONOBST|( PROP_OPTIMAL              <<(PROP_ESHIFT*entri)))
#define PROP_1011 (PROP_NONOBST|((PROP_OPTIMAL|PROP_CLOSEST)<<(PROP_ESHIFT*entri)))
#define PROP_1100 (PROP_NONOBST|( PROP_INSIDE               <<(PROP_ESHIFT*entri)))
#define PROP_1101 (PROP_NONOBST|((PROP_INSIDE |PROP_CLOSEST)<<(PROP_ESHIFT*entri)))
#define PROP_1110 (PROP_NONOBST|( PROP_STRICT               <<(PROP_ESHIFT*entri)))
#define PROP_1111 (PROP_NONOBST|((PROP_STRICT |PROP_CLOSEST)<<(PROP_ESHIFT*entri)))

#define PEN_COM_COUNT       1  /* pen for each communication lost */
#define PEN_STRICT_COUNT    2  /* pen for each square lost under most strict
				  conditions */
#define PEN_LOOSE_COUNT    10  /* pen for each square lost under loosest
				  conditions */
#define PEN_OPTIM_COUNT     5  /* pen for each square lost under optimal
				  distance conditions */
#define PEN_INSIDE_COUNT    5  /* penalty for each square lost under inside
				  use conditions */
#define PEN_DEAD_COUNT  10000  /* pen for dead square (unreachable) */
#define PEN_ENTR_REACH     10  /* pen for a square that should be filled
				  from another entrance - this is entrance
				  specific */
#define PEN_NOTCLOSEST     10  /* This square is better reached from other
				  entrance */
#define PEN_NOTFIXED       10  /* if stone is not fixed on goal */
#define PEN_0_LIMIT	    3   /* square penalty for which we return, even
				   if no tree below was produced. That is, a
				   square with penalty 0 is a secure bet,
				   make it and return */
#define PEN_1_LIMIT        10	/* square penalty limit to which only 1
				   macro is created */
#define PEN_2_LIMIT        40	/* square penalty limit to which only 2
				   macro is created */
#define PEN_3_LIMIT       100	/* square penalty limit to which only 3
				   macro is created */
#define PEN_4_LIMIT       250	/* square penalty limit to which only 4
				   macro is created */

#define CHECKEND if (sat_prop == PROP_1111) continue
