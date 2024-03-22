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

#define MAX_GTV_DEPTH 200

#ifdef GTV
#define GTVAny(x)	x
#else
#define GTVAny(x)
#endif

int  GTVOpen( int d, char fen[] );
void GTVNodeEnter( int d, int alpha, int beta, char move[], int type );
void GTVNodeExit( int d, int score, char *best_move );
void GTVClose( );
char *GTVFen(MAZE *maze);
char *GTVMove(MOVE move);
