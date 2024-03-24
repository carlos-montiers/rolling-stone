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

#include <stdio.h>
#include <string.h>
#include "board.h"

static FILE *fpTree;
static long TreeNodes;
static struct {
   long node_no;
   int  branching;
   int  type;
   int  alpha;
   int  beta;
   char move[8];
} Tree[MAX_GTV_DEPTH];


int GTVOpen( int depth, char fen[] )
{
   int  d;
   char name[15];

   TreeNodes = 0L;
   for ( d=0 ; d<MAX_GTV_DEPTH ; d++ ) {
      Tree[d].node_no = 0L;
      Tree[d].branching = 0;
      Tree[d].type = 0;
      Tree[d].alpha = 0;
      Tree[d].beta = 0;
   }
   sprintf( name, "tree%02d.gtv", depth );\
   fpTree = fopen( name, "w" );
   if ( fpTree != NULL  ) {
      fprintf( fpTree, "0 0 %s\n", fen );
   }

   return ( fpTree == NULL );
}


void GTVNodeEnter( int d, int alpha, int beta, char *move, int type )
{
   Tree[d].node_no = ++TreeNodes;
   Tree[d].alpha = alpha;
   Tree[d].beta = beta;
   strcpy( Tree[d].move, move );
   Tree[d].type = type;
}


void GTVNodeExit( int d, int score, char *move )
{
  char *type_chr = "PNQ";

  if (d) {
     Tree[d-1].branching++;
  }

  fprintf( fpTree,
           "%6ld %6ld %2d %s %6d %c %c %08lx%08lx %6d %6d %s\n",
           Tree[d].node_no,
           (d) ? Tree[d-1].node_no : 0,
           Tree[d].branching,
           Tree[d].move,
           score,
           type_chr[Tree[d].type],
           '-',
           0L,
           0L,
           Tree[d].alpha,
           Tree[d].beta,
           move );

  Tree[d].node_no = 0L;
  Tree[d].branching = 0;
  Tree[d].type = 0;
  Tree[d].alpha = 0;
  Tree[d].beta = 0;
  strcpy( Tree[d].move, "" );
}

void GTVClose( )
{
   fclose( fpTree );
}


void GTVEval()
{
}

/* GTV can find a position in the tree */
/* Set nullmove */

char *GTVFen(MAZE *maze) {
        int x,y,num_empty;
        static char buff[XSIZE*YSIZE*2];
	char e_buff[16];
	char this;

        num_empty=0;
        buff[0]='\0';
        for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
                num_empty = 0;
                for (x = 0; x<XSIZE; x++) {
			this = '\0';
                        if (   maze->PHYSstone[XY2ID(x,y)]>=0
                            && maze->Phys[XY2ID(x,y)].goal>=0) {
				this = '*';
			}
                        else if (maze->PHYSstone[XY2ID(x,y)]>=0) {
				this = '$';
			}
                        else if (maze->manpos == XY2ID(x,y)) {
				this = '@';
			}
                        else if (maze->Phys[XY2ID(x,y)].goal>=0) {
				this = '.';
			}
                        else if (IsBitSetBS(maze->wall,XY2ID(x,y))) {
				this = '#';
			}
                        else {
                                num_empty++;
			}
			if (this != '\0') {
				if (num_empty) {
					while (num_empty>9) {
						strcat(buff,"9");
						num_empty -= 9;
					}
					sprintf(e_buff,"%i%c",num_empty,this);
					strcat(buff,e_buff);
					num_empty=0;
				} else {
					sprintf(e_buff,"%c",this);
					strcat(buff,e_buff);
				}
			}
                }
		if (num_empty<XSIZE) strcat(buff,"/");
        }
	buff[strlen(buff)-1] = '\0';
	return(buff);
}

char *GTVMove(MOVE move) {
	static char buff[50];

	if (ISDUMMYMOVE(move)) {
		/*strcpy(buff,"AA-AA DummyMove");*/
		strcpy(buff,"AA-AA");
	} else {
		buff[0] = 'A'+move.from/YSIZE;
		buff[1] = 'A'+(YSIZE - move.from%YSIZE -1);
		buff[2] = '-';
		buff[3] = 'A'+move.to/YSIZE;
		buff[4] = 'A'+(YSIZE - move.to%YSIZE -1);
		buff[5] = ' ';
		buff[6] = '\0';
	}
	return(buff);
}

