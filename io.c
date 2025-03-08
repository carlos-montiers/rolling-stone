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

int PosNr=0;

void PrintMaze(MAZE *maze, bool printGoalRooms /* BD */) {
	int x,y,num_empty,pos;
	char buff[XSIZE*2+4]; /* BD */

	if (maze == NULL) return;
	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		if (printGoalRooms) { /* BD */
		   /* make it an unreadable puzzle */
		   strcat(buff,"; "); /* BD */
		}
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"?");
			else if (IsBitSetBS(maze->wall,pos)) strcat(buff,"#");
			else if ( maze->PHYSstone[pos]>=0
					  && maze->Phys[pos].goal>=0) strcat(buff,"*");
			else if (maze->PHYSstone[pos]>=0) strcat(buff,"$");
			else if ( maze->manpos==pos
					  && maze->Phys[pos].goal>=0) strcat(buff,"+");
			else if (maze->manpos==pos) strcat(buff,"@");
			else if (maze->Phys[pos].goal>=0) strcat(buff,".");
			else if (IsBitSetBS(maze->out,pos)) {
					strcat(buff," ");
					num_empty++;
			}
			else if ((printGoalRooms) /* BD */
					 && (maze->groom_index[x*YSIZE+y]>=0)) {
					sprintf(&buff[strlen(buff)],"%i",
							maze->groom_index[x*YSIZE+y]%10);
			}
			else strcat(buff," ");
		}
		Mprintf( 0, "%s\n",buff);
	}
	if (printGoalRooms) { /* BD */
		Mprintf( 0, "\nmanpos: %i h: %i pen: %i search nodes: %" PRIi32 " patterns: %d total nodes: %" PRIi32 "\n",
				 maze->manpos,maze->h,maze->pen,
				 IdaInfo->node_count, maze->conflicts->number_patterns, total_node_count );
	}
}

void ReadMaze(FILE *fp, MAZE *maze ) {
	int sq,x,y,pos;

	x = 0;
	y = YSIZE-1;
	ResetMaze(maze);
	while ( (sq = getc(fp)) != EOF) {
		pos = XY2ID(x,y);
		if (sq=='\n' && x==0) goto END_INPUT;
		if (y<0) {
			My_exit(1, "Maze too large for YSIZE, recompile with larger YSIZE!\n");
		}
		if (x == 0) {
			UnsetBitBS(maze->M[WEST],pos);
			UnsetBitBS(maze->S[WEST],pos);
		}
		if (x == XSIZE-1) {
			UnsetBitBS(maze->M[EAST],pos);
			UnsetBitBS(maze->S[EAST],pos);
		}
		if (y == 0) {
			UnsetBitBS(maze->M[SOUTH],pos);
			UnsetBitBS(maze->S[SOUTH],pos);
		}
		if (y == YSIZE-1) {
			UnsetBitBS(maze->M[NORTH],pos);
			UnsetBitBS(maze->S[NORTH],pos);
		}
		switch (sq) {
		case '\n':
			if (x==0) goto END_INPUT;
			if (x>XSIZE) {
				My_exit(1, "Maze too large for XSIZE, recompile with larger XSIZE!\n");
			}
			for (;x<XSIZE;x++) SetBitBS(maze->out,pos);
			y--;
			x = 0;
			break;
		case '#':
			SetBitBS(maze->wall,pos);

			if (IsBitSetBS(maze->M[NORTH],pos)) {
				UnsetBitBS(maze->M[SOUTH],pos+1);
				UnsetBitBS(maze->S[SOUTH],pos+1);
				UnsetBitBS(maze->S[NORTH],pos+1);
			}
			if (IsBitSetBS(maze->M[EAST],pos)) {
				UnsetBitBS(maze->M[WEST],pos+YSIZE);
				UnsetBitBS(maze->S[WEST],pos+YSIZE);
				UnsetBitBS(maze->S[EAST],pos+YSIZE);
			}
			if (x>0) {
				UnsetBitBS(maze->M[EAST],pos-YSIZE);
				UnsetBitBS(maze->S[EAST],pos-YSIZE);
				UnsetBitBS(maze->S[WEST],pos-YSIZE);
			}
			if (y>0) {
				UnsetBitBS(maze->M[NORTH],pos-1);
				UnsetBitBS(maze->S[NORTH],pos-1);
				UnsetBitBS(maze->S[SOUTH],pos-1);
			}
			UnsetBitBS(maze->M[NORTH],pos);
			UnsetBitBS(maze->M[EAST],pos);
			UnsetBitBS(maze->M[WEST],pos);
			UnsetBitBS(maze->M[SOUTH],pos);
			UnsetBitBS(maze->S[NORTH],pos);
			UnsetBitBS(maze->S[EAST],pos);
			UnsetBitBS(maze->S[WEST],pos);
			UnsetBitBS(maze->S[SOUTH],pos);
			x++;
			break;
		case '+':
		case '%':
		case '@':
			maze->manpos = XY2ID(x,y);
                        if (sq=='@') x++;
			else goto GOALSQUARE;
                        break;
		case '*': /* Stone on Goal, jump next break!!!!! */
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case '$':
			maze->PHYSstone[pos] = maze->number_stones++;
			SR(Assert(MAXSTONES>maze->number_stones,
				"Too many stones!\n"));
			maze->stones
			   =(STN*)My_realloc(maze->stones,
				sizeof(STN)*maze->number_stones);
			maze->stones[maze->number_stones-1].loc=XY2ID(x,y);
			SetBitBS(maze->stone,XY2ID(x,y));
                        if (sq=='$' || (sq>='A'&&sq<='Z')) {
                        	x++;
				break;
			}
			/* fall throuh in case it is a stone on a goal */
GOALSQUARE:
		case '.':
			maze->Phys[pos].goal = maze->number_goals++;;
			SR(Assert(MAXGOALS>maze->number_goals,
				"Too many goals!\n"));
			maze->goals
			   =(GOL*)My_realloc(maze->goals,
				sizeof(GOL)*maze->number_goals);
			maze->goals[maze->number_goals-1].loc=XY2ID(x,y);
			maze->goals[maze->number_goals-1].tried=0;
			SetBitBS(maze->goal,XY2ID(x,y));
                        x++;
                        break;
		case '=':
		case ' ':
                        x++;
                        break;
		default:
			My_exit(1, "unknown maze square: \"%c\"\n",sq);
		}
	}
END_INPUT:
	maze->goal_manpos = maze->manpos;
	/* sanity check for some trivial numbers */
	if (maze->number_goals < maze->number_stones) {
		My_exit(1,
			"Too few goals (%i) for all stones (%i) in the maze\n",
			maze->number_goals, maze->number_stones);
	}

        maze->lbtable= (LBENTRY*)My_realloc(maze->lbtable,
			sizeof(LBENTRY)*maze->number_goals);
	MarkAll(maze);
	if (maze->number_stones==maze->number_goals) {
		BetterLowerBound(maze);
	} else {
		Debug(0,0,"ReadMaze: DeadLowerBound called, stones<goals!\n");
		DeadLowerBound(maze,ENDPATH);
	}
	NormHashKey(maze);
	FindMacros(maze);

	if (Options.autolocal == 1) SetLocalCut(0,0,0);
}

char *PrintMove(MOVE move) {

	static char buff[32];

	sprintf(buff,"(%02i:)%02i-%02i",move.man,move.from,move.to);
	return(buff);
}

char *HumanMove(MOVE move) {
	static char buff[50];

	if (ISDUMMYMOVE(move)) {
		/*strcpy(buff,"AA-AA DummyMove");*/
		strcpy(buff,"Aa-Aa");
	} else {
		buff[0] = 'A'+move.from/YSIZE;
		buff[1] = 'a'+(YSIZE - move.from%YSIZE -1);
		buff[2] = '-';
		buff[3] = 'A'+move.to/YSIZE;
		buff[4] = 'a'+(YSIZE - move.to%YSIZE -1);
		buff[5] = '\0';
	}
	return(buff);
}

void PrintBit2Maze(MAZE *maze,BitString marks) {
	int x,y,num_empty,pos;
	char buff[XSIZE*2+4]; /* BD */

	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		strcat(buff, "; "); /* BD */
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"?");
			else if (IsBitSetBS(maze->wall,pos)) strcat(buff,"#");
			else if (IsBitSetBS(marks,pos)) strcat(buff,"0");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff," ");
				num_empty++;
			}
			else strcat(buff," ");
		}
		Mprintf( 0, "%s\n",buff);
	}
}

void PrintBit3Maze(MAZE *maze,BitString marks,BitString mark2, PHYSID manpos) {
	int x,y,num_empty,pos;
	char buff[XSIZE*2+4]; /* BD */

	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		strcat(buff, "; "); /* BD */
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"?");
			else if (pos == manpos) strcat(buff,"@");
			else if (IsBitSetBS(maze->wall,pos)) strcat(buff,"#");
			else if (IsBitSetBS(marks,pos)) strcat(buff,"*");
			else if (IsBitSetBS(mark2,pos)) strcat(buff,"+");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff," ");
				num_empty++;
			}
			else strcat(buff," ");
		}
		Mprintf( 0, "%s\n",buff);
	}
}

