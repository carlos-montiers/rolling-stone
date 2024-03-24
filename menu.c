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

/*=============================================================================
|
| This file includes User Interface Functions for the DEADLOCK program.
|
=============================================================================*/
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "board.h"

/*   Menu structures.
*/
CMDMENU cmdMainMenu[] = {
   { "S", CmdSolve,		"S nam|num kind      Solve Maze" },
   { "A", CmdAbort,		"A                   Set Abort Node Count" },
   { "T", CmdTimer,		"T sec [REAL|VIRT]   Set Abort time" },
   { "C", CmdTestAll,		"C a b kind          Test a to b in screen" },
   { "P", CmdPrint,		"P nam|num kind      Print Maze" },
   { "L", CmdBounds,		"L a b kind          All 90 Lower Bounds" },
   { "N", CmdPosNr,		"N num               Set PosNr with num" },
   { "M", CmdMove,		"M num-num|XX-YY     move from num to num" },
   { "X", CmdTestX,		"X [lrud]*           move center for xdist" },
   { "Z", CmdShow,		"Z                   Show Menu" },
   { "O", CmdOptions,		"O                   Options Menu" },
   { "Q", CmdQuit,		"Q                   Quit Program" },
   { NULL } };

 CMDMENU cmdShowMenu[] = {
   { "D", CmdShowMDist,		"D                   Hist of man-distances"},
   { "S", CmdShowSDist,		"S                   Hist of stone-distances"},
   { "X", CmdShowXDist,		"X                   Hist of X-distances"},
   { "C", CmdShowConfl,		"C                   Print Conflicts prev search"},
   { NULL } };

 CMDMENU cmdOptionsMenu[] = {
   { "E", CmdOptionEX,   "E             Examine all settings" },
   { "H", CmdOptionTT,   "H [on/off]    HashTable on/off" },
   { "D", CmdOptionDL,   "D [on/off]    deadlock det. movegen on/off" },
   { "Z", CmdOptionDZ,   "Z [on/off]    deadlock2 det. movegen on/off" },
   { "R", CmdOptionAS,   "R [on/off]    areasearch on/off" },
   { "S", CmdOptionDS,   "S [on/off]    deadsearch on/off" },
   { "N", CmdOptionPS,   "N [on/off]    pensearch on/off" },
   { "W", CmdOptionCS,   "W [on/off]    scan search on/off" },
   { "J", CmdOptionPL,   "J number      node limit for pattern searches" },
   { "K", CmdOptionMM,   "K [on/off]    minimization on/off" },
   { "Y", CmdOptionLP,   "Y [on/off]    limit patterns on/off" },
   { "m", CmdOptionLM,   "m [on/off]    lazy maximization on/off" },
   { "X", CmdOptionST,   "X [on/off]    store tested on/off" },
   { "P", CmdOptionPT,   "P number      Switch Pattern DBs on/off (0-7)" },
   { "M", CmdOptionMP,   "M [on/off]    LB manpos on/off" },
   { "C", CmdOptionCF,   "C [on/off]    LB conflict on/off" },
   { "d", CmdOptionDD,   "d [on/off]    Dynamic distances on/off" },
   { "T", CmdOptionTM,   "T [on/off]    Tunnel Macro on/off" },
   { "G", CmdOptionGM,   "G [on/off]    Goal Macro on/off" },
   { "U", CmdOptionCG,   "U [on/off]    Cut Goal Macro on/off" },
   { "A", CmdOptionXD,   "A [on/off]    eXtended Distance on/off" },
   { "L", CmdOptionLC,   "L k m d       Local Cut (k,m,d), -1 -1 turns off" },
   { "B", CmdOptionLA,   "B [on/off]    Auto Set Local Cut Parameter" },
   { "V", CmdOptionOE,   "V [s] [h]     Overestimation scaling factor and h-scaling" },
   { "F", CmdOptionAD,   "F [on/off]    Assume dead on/off" },
   { "O", CmdOptionMO,   "O number      Set Move order index (0-off)" },
   { NULL } };

int  Cur_Maze_Number;
IDA  MainIdaInfo;
MAZE Maze;

void SimpleMakeName(char *name, int *curr_number, int number, char *str_kind)
{
	*curr_number = number;
	if (str_kind == NULL)
		sprintf(name,"screens/screen.%d",number);
	else
		sprintf(name,"screens/%s.%d",str_kind,number);
}

void MakeName(char *name, int *curr_number, char *str_number, char *str_kind)
{
	int no;

	no = atoi(str_number);
	if (no == 0) {
		*curr_number = -1;
		strcpy(name,str_number);
	} else {
		*curr_number = no;
		if (str_kind == NULL)
			sprintf(name,"screens/screen.%d",no);
		else
			sprintf(name,"screens/%s.%d",str_kind,no);
	}
}

void
/*=============================================================================
|
| Descr.:    Character based interface for the main menu.
|
| Return:    None.
|
| Side eff.: SearchDepth can be updated. Also when playing a game the
|            chess related global structures are modified.
|
=============================================================================*/
MainMenu()
{
   COMMAND cmd;
   char    cmdstr[SZ_CMDSTR+1];
   char    name[100];
   char    *param;
   FILE    *fp;
   int     no,i;
   MOVE    move;
   UNMOVE  unmove;

   InitMaze(&Maze);
   param = NULL;
   for ( ;; ) {

      cmd=GetCommand("Command",cmdMainMenu,cmdstr);

      switch ( cmd ) {

	case CmdSolve:
			MakeName(name,&Cur_Maze_Number, CmdParam(cmdstr,1),
							CmdParam(cmdstr,2));
			if ((fp = fopen(name,"r")) != NULL) {
   				Mprintf( 0, "Maze %s:\n", name );
				MainIdaInfo.IdaMaze = &Maze;
				ReadMaze(fp,&Maze);
				fclose(fp);
				IdaInfo = &MainIdaInfo;
				StartIda(YES);
			} else {
                                My_exit(1,"Menu: %s %s\n",name,strerror(errno));
			}
			break;
	case CmdAbort:
			param = CmdParam(cmdstr,1);
			if (param != NULL)
				MainIdaInfo.AbortNodeCount = atol(param);
			else MainIdaInfo.AbortNodeCount = -1;
			break;
	case CmdTimer:
			param = CmdParam(cmdstr,1);
			if (param != NULL)
				MainIdaInfo.TimeOut = atoi(param);
			param = CmdParam(cmdstr,2);
			if (param != NULL && param[0]=='R')
				MainIdaInfo.TimeOutType = REAL;
			if (param != NULL && param[0]=='V')
				MainIdaInfo.TimeOutType = VIRTUAL;
			break;
	case CmdTestAll:
			break;
	case CmdPrint:
			MakeName(name,&Cur_Maze_Number, CmdParam(cmdstr,1),
							CmdParam(cmdstr,2));
			if ((fp = fopen(name,"r")) != NULL) {
				int   plainlb;

				MainIdaInfo.IdaMaze = &Maze;
				ReadMaze(fp,&Maze);
				fclose(fp);
				plainlb = PlainLowerBound(&Maze);
				BetterLowerBound(&Maze);
				Mprintf(0,"%15s: ",name);
				Mprintf(0,"lb: %d (pen: %d), plain lb: %d\n",
					Maze.h,Maze.pen,plainlb);
				BetterLowerBound(&Maze);
			} else {
                        	My_exit(1,"Menu: %s %s\n",name,strerror(errno));
			}
			break;
	case CmdBounds:
			if ((param = CmdParam(cmdstr,1)) != NULL) {
				no = atoi(param);
				if (no<1) no = 1;
			} else no = 1;
		        for (i=90; i>0; i--) {
				SimpleMakeName(name,&Cur_Maze_Number, i,
						CmdParam(cmdstr,3));
                		if ( (fp = fopen(name,"r")) == NULL) {
					MainIdaInfo.IdaMaze = &Maze;
                			ReadMaze(fp,&Maze);
                			Mprintf(0,"%i: %i\n",i,Maze.h);
                			fflush(stdout);
                			fclose(fp);
				} else {
                        		My_exit(1,"Menu: %s %s\n",
						name,strerror(errno));
                		}
        		}
			break;
	case CmdOptions: OptionsMenu(); break;
	case CmdShow:    ShowMenu(); break;
	case CmdPosNr:
			param = CmdParam(cmdstr,1);
			if (param != NULL) PosNr = atoi(param);
			else PosNr = 0;
			break;
	case CmdMove:
			move.move_dist = 1;
			move.value = 0;
			move.from = atoi(CmdParam(cmdstr,1));
			if (move.from != 0) {
				move.to   = atoi(CmdParam(cmdstr,2));
				move.last_over = move.from;
				PrintMaze(&Maze);
				/*PrintMatches(&Maze);*/
				MakeMove(&Maze,&move,&unmove,ENDPATH);
			} else {
			   ParseMakeMoves(cmdstr);
			}
			break;
	case CmdTestX:
			break;
	case CmdQuit:
   			DelMaze(&Maze);
			return;
	default: break;
      }
   }
}

void ShowMenu()
{
   COMMAND cmd;
   char    cmdstr[SZ_CMDSTR+1];

   while ( (cmd=GetCommand("Show",cmdShowMenu,cmdstr)) != CmdQuit ) {

      	switch ( cmd ) {

   	case CmdShowConfl:
			PrintConflicts(&Maze,Maze.conflicts);
			break;
   	case CmdShowXDist:
			XDistHist(&Maze,NULL,NULL);
			break;
   	case CmdShowSDist:
			SDistHist(&Maze);
			break;
   	case CmdShowMDist:
			DistHist(&Maze);
			break;
	default: break;
	}
    }
}

void OptionsMenu()
{
   COMMAND cmd;
   char    cmdstr[SZ_CMDSTR+1];
   char   *param,*param2,*param3;
   int     i;

   while ( (cmd=GetCommand("Option",cmdOptionsMenu,cmdstr)) != CmdQuit ) {

      	switch ( cmd ) {

        case CmdOptionEX:
		print_stats(2);
		break;
        case CmdOptionTT:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.tt = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.tt = NO;
		}
		else Options.tt = !Options.tt;
		break;
        case CmdOptionDL:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.dl_mg = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.dl_mg = NO;
		}
		else Options.dl_mg = !Options.dl_mg;
		break;
	case CmdOptionDZ:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.dl2_mg = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.dl2_mg = NO;
		}
		else Options.dl2_mg = !Options.dl2_mg;
		break;
	case CmdOptionPS:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.pen_srch = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.pen_srch = NO;
		}
		else Options.pen_srch = !Options.pen_srch;
		break;
	case CmdOptionCS:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.scan_srch = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.scan_srch = NO;
		}
		else Options.scan_srch = !Options.scan_srch;
		break;
	case CmdOptionPL:
		param = CmdParam(cmdstr,1);
		if (param != NULL) MainIdaInfo.pattern_node_limit = atoi(param);
		break;
	case CmdOptionAS:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.area_srch = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.area_srch = NO;
		}
		else Options.area_srch = !Options.area_srch;
		break;

	case CmdOptionMM:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.minimize = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.minimize = NO;
		}
		else Options.minimize = !Options.minimize;
		break;

	case CmdOptionLP:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.limit_pat = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.limit_pat = NO;
		}
		else Options.limit_pat = !Options.limit_pat;
		break;

	case CmdOptionLM:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.lazy_max = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.lazy_max = NO;
		}
		else Options.lazy_max = !Options.lazy_max;
		break;

	case CmdOptionST:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.st_testd = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.st_testd = NO;
		}
		else Options.st_testd = !Options.st_testd;
		break;
	case CmdOptionDS:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.dl_srch = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.dl_srch = NO;
		}
		else Options.dl_srch = !Options.dl_srch;
		break;
        case CmdOptionPT:
		param = CmdParam(cmdstr,1);
		if ( param != NULL ) {
			i = atoi(param);
			Options.dl_db = min(7,max(0,i));
		}
		break;
        case CmdOptionMP:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.lb_mp = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.lb_mp = NO;
		}
		else Options.lb_mp = !Options.lb_mp;
		break;
        case CmdOptionCF:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.lb_cf = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.lb_cf = NO;
		}
		else Options.lb_cf = !Options.lb_cf;
		break;
         case CmdOptionDD:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.lb_dd = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.lb_dd = NO;
		}
		else Options.lb_dd = !Options.lb_dd;
		break;
        case CmdOptionTM:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.mc_tu = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.mc_tu = NO;
		}
		else Options.mc_tu = !Options.mc_tu;
		break;
        case CmdOptionGM:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.mc_gm = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.mc_gm = NO;
		}
		else Options.mc_gm = !Options.mc_gm;
		break;
        case CmdOptionCG:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.cut_goal = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.cut_goal = NO;
		}
		else Options.cut_goal = !Options.cut_goal;
		break;
       case CmdOptionXD:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.xdist = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.xdist = NO;
		}
		else Options.xdist = !Options.xdist;
		break;
	case CmdOptionLC:
		param  = CmdParam(cmdstr,1);
		param2 = CmdParam(cmdstr,2);
		param3 = CmdParam(cmdstr,3);
		SetLocalCut(atoi(param),atoi(param2),atoi(param3));
		break;
       case CmdOptionLA:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.autolocal = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.autolocal = NO;
		}
		else Options.autolocal = !Options.autolocal;
		break;
       case CmdOptionOE:
		param = CmdParam(cmdstr,1);
		Options.overestim = atof(param);
		param = CmdParam(cmdstr,2);
		Options.hoverestim = atof(param);
		if (Options.hoverestim == 0.0) Options.hoverestim = 1.0;
		break;
       case CmdOptionAD:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.assumedead = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.assumedead = NO;
		}
		else Options.assumedead = !Options.assumedead;
		break;
	case CmdOptionMO:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			i = atoi(param);
			switch (i) {
			case 0: IdaInfo->MoveOrdering = NoMoveOrdering;
				break;
			case 1:
			case 2:
			case 3: IdaInfo->MoveOrdering = ManDistMoveOrdering;
				break;
			case 4: IdaInfo->MoveOrdering = NewMoveOrdering;
				break;
			default: Mprintf(0,
					"WARNING: NoMoveOrdering set!\n");
				IdaInfo->MoveOrdering = NoMoveOrdering;
				break;
			}
		}
		break;
	default: break;
	}
   }
}

COMMAND
/*=============================================================================
|
| Descr.:    Get a command from the user.  By default the input is standard
|            I/O.  Every command entered is logged in a file so they can
|            be replayed.
|
| In:   prompt   The command prompt string.
|       cmdMenu  The menu structure for the menu to display.
|
| Out:  cmdstr   The exact command string as entered by the user.
|                The command CmdParam(cmdstr,nr) can be used to determine
|                the parameters of the cmdstr.
|
| Return:    The command type entered.  The command type is determined by
|            the first word in the command string.
|
| Side eff.: None.
|
=============================================================================*/
GetCommand(char prompt[], CMDMENU cmdMenu[], char cmdstr[] )
{
   int  i;

   /*
      Read input string and log to file.
   */
   Mprintf( 0, "%s:", prompt);
   if (fgets( cmdstr, SZ_CMDSTR, stdin ) == NULL) {
	return(CmdQuit);
   }

   /*
      Make sure string is zero terminated and get rid of newline at end.
   */
   i = 0;
   cmdstr[SZ_CMDSTR] = '\0';
   if ( strlen(cmdstr) )
      cmdstr[strlen(cmdstr)-1] ='\0';

   if ( cmdstr[0] == '\n' ) return ( CmdUnknown );
   if ( cmdstr[0] == '\0' ) return ( CmdUnknown );
   if ( cmdstr[0] ==  '%' ) return ( CmdUnknown );
   if ( cmdstr[0] ==  '#' ) return ( CmdUnknown );
   if ( cmdstr[0] == '?' ) {
      /*
         Help command.  Displays availible menu options.
      */
      Mprintf( 0, "MENU OPTIONS:" );
      while ( cmdMenu[i].keys != NULL ) {
         Mprintf(0, "\n  %-10s - %s", cmdMenu[i].keys, cmdMenu[i].text );
         i++;
      }
      Mprintf( 0, "\n  %-10s - %s", "<", "Back (quit)" );
      Mprintf( 0, "\n  %-10s - %s", "?", "Help" );
      Mprintf( 0, "\n");
      return( CmdHelp );
   }
   else if ( cmdstr[0] == '<' ) {
      /*
         Quit commmand.  Return command CmdQuit.
      */
      return( CmdQuit );
   }
   else {
      /*
         Check if entered command is legal.  If so return the
         command type.
      */
      while ( cmdMenu[i].keys != NULL ) {
         if ( strchr(cmdMenu[i].keys,cmdstr[0]) != NULL )
            return ( cmdMenu[i].cmd );
         i++;
      }
   }

   /*
     Command entered is unknown, so display an error message.
   */
   Mprintf( 0, "Unknown command '%s'\n", cmdstr );

   return ( CmdUnknown );
}



char
/*=============================================================================
|
| Descr.:    Returns a specified paramenter of a given command string.
|
| In:   cmdstr   The command string
|       no       The number of the parameter we want to get.
|
| Return:    Pointer to paramenter number no in the command string
|
| Side eff.: None.
|
=============================================================================*/
*CmdParam( char cmdstr[], int no )
{
   static char tmpcmdstr[SZ_CMDSTR+1];
   char *p, *found;

   strncpy( tmpcmdstr, cmdstr, SZ_CMDSTR );
   found = NULL;
   p = strtok( tmpcmdstr, " " );
   while ( p != NULL ) {
      if ( no == 0 )
         found = p;
      no--;
      p = strtok( NULL, " " );
   }

   return ( found );
}

void ParseMakeMoves(char *param)
/* Makes moves from the string param, enters into the IDA history */
/* Uses IdaInfo and IdaMaze */
{
	int no,cut,i,n_moves,depth;
	IDAARRAY *S;

	no=2;
	while (param[no] != '\0' && param[no] != '\n') {
		S = &IdaInfo->IdaArray[IdaInfo->IdaMaze->currentmovenumber];
		while (param[no]==' ' || param[no]=='*') no++;
		if (param[no]<'A'||param[no]>('A'+XSIZE)) {
			Mprintf( 0, "Error in move string\n");
			goto READ_END;
		}
		S->currentmove.from = (param[no++]-'A')*YSIZE;
		S->currentmove.from +=(YSIZE - (param[no++]-'a')) -1;
READ_TO:
		S = &IdaInfo->IdaArray[IdaInfo->IdaMaze->currentmovenumber];
		while (param[no]=='-' || param[no]=='*') no++;
		if (param[no]<'A'||param[no]>('A'+XSIZE)) {
			Mprintf( 0, "Error in move string\n");
			goto READ_END;
		}
		S->currentmove.to  = (param[no++]-'A')*YSIZE;
		S->currentmove.to += (YSIZE - (param[no++]-'a')) -1;
		S->currentmove.last_over = S->currentmove.from;
		S->currentmove.move_dist =
			DistToGoal(IdaInfo->IdaMaze,S->currentmove.from,
			S->currentmove.to,&S->currentmove.last_over);
		if (  (IdaInfo->IdaMaze->groom_index[S->currentmove.to]>=0)
		    &&(IdaInfo->IdaMaze->gmtrees[IdaInfo->IdaMaze->groom_index[S->currentmove.to]]
		       != NULL)) {
			S->currentmove.macro_id = 4;
		} else S->currentmove.macro_id = 0;
		Mprintf( 0, "\nMoving: %s\n",HumanMove(S->currentmove));
		depth = IdaInfo->IdaMaze->currentmovenumber;
		n_moves = GenerateMoves(IdaInfo->IdaMaze,
			IdaInfo->IdaArray[depth].moves);
		for (i=0; i<n_moves; i++) {
		   if (  IdaInfo->IdaArray[depth].moves[i].from
			  ==S->currentmove.from
		       &&IdaInfo->IdaArray[depth].moves[i].to
			  ==S->currentmove.to  ) {
			S->currentmove.man
				= IdaInfo->IdaArray[depth].moves[i].man;
			S->currentmove.last_over
				= IdaInfo->IdaArray[depth].moves[i].last_over;
			S->currentmove.macro_id
				= IdaInfo->IdaArray[depth].moves[i].macro_id;
			S->currentmove.move_dist
				= IdaInfo->IdaArray[depth].moves[i].move_dist;
		   }
		}
		if (i>n_moves) {
			Debug(0,depth,"no such move found: %s!\n",
				PrintMove(S->currentmove));
		}
		cut = RegisterMove(&S->currentmove,
			IdaInfo->IdaMaze->currentmovenumber);
		/* PrintMatches(IdaInfo->IdaMaze);*/
		PrintMaze(IdaInfo->IdaMaze);
		MakeMove(IdaInfo->IdaMaze,&S->currentmove,&S->unmove,ENDPATH);
		Mprintf(0,"XX g: %3d, h: %3d, g+h: %3d, distant: %c, cut: %c\n",
			IdaInfo->IdaMaze->g, IdaInfo->IdaMaze->h,
			IdaInfo->IdaMaze->g+IdaInfo->IdaMaze->h,
			S->distant?'Y':'N',cut?'Y':'N'
			 );
		while (param[no]==' ' || param[no]=='*') no++;
		if (param[no]=='-') {
			IdaInfo->IdaArray[IdaInfo->IdaMaze->currentmovenumber].
				currentmove.from = S->currentmove.to;
			goto READ_TO;
		}
	}
READ_END:
	;
}
