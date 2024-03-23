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

void Mprintf( int priority, char *format, ... )
{
   va_list argptr;
   static char msg[2000];

   if (priority <= IdaInfo->PrintPriority) {
   	va_start( argptr, format );
   	vsprintf( msg, format, argptr );
   	va_end( argptr );

        printf( msg );		/* This is the only printf in the entire
				 * program!!!!!! */
        fflush(stdout);
   }
}

void Debug( int level, int indent, char *format, ... )
/*
	0: Highest priority, prints exit messages and error stuff
	2: Highest level stuff, like loading and saving mazes
	4: High level search stuff, calls to mea
	6: Lower level stuff, path searcher routines
	8: all the rest
*/
{

   va_list argptr;
   char msg[2000];
   #define xxx " | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |"
   char xxx2[800];

      /*
         The level of this debug call is less or equal to current debug level,
         therefore debug messages are printed.
      */
   if (IdaInfo->PrintPriority >= level) {
      if ( format != NULL ) {
         va_start( argptr, format );
         vsprintf( msg, format, argptr );
         va_end( argptr );
      }
      indent += IdaInfo->base_indent;
      if (indent>=0) {
      	strncpy(xxx2,xxx,indent);
      	xxx2[indent] = '\0';
      	Mprintf( level, ". %3d %s ", indent, xxx2);
      }
      Mprintf( level, msg );
   }
}

void Assert(int cond, char *format, ...) {

   va_list argptr;
   char msg[2000];


   if (!cond) {
	if ( format != NULL ) {
        	va_start( argptr, format );
        	vsprintf( msg, format, argptr );
        	va_end( argptr );
	}
	My_exit(1,msg);
   }
}
