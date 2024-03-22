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

typedef enum {               /* Commands types */

   /* General menu commands. */
   CmdUnknown,               /* Unknown command. */
   CmdQuit,                  /* Quit in menu. */
   CmdHelp,                  /* List menu options. */

   /* Main menu commands. */
   CmdSolve,		     /* Solve Maze */
   CmdAbort,		     /* Abort Node Count set */
   CmdTimer,		     /* Abort Time setting  */
   CmdTestAll,		     /* Test All Mazes in screen */
   CmdPrint,		     /* */
   CmdBounds,		     /* */
   CmdPosNr,		     /* */
   CmdMove,		     /* */
   CmdTestX,		     /* */

   CmdOptions,		     /* */
   CmdOptionEX,  	     /* E Examine all settings */
   CmdOptionTT,   	     /* T [on/off] Toggle TT */
   CmdOptionDL,   	     /* D [on/off] Toggle deadlock det. movegen */
   CmdOptionDZ,   	     /* Z [on/off] Toggle deadlock2 det. movegen */
   CmdOptionDS,   	     /* S [on/off] Toggle DeadMove search */
   CmdOptionPS,   	     /* N [on/off] Toggle PenMove search */
   CmdOptionAS,   	     /* A [on/off] Toggle AreaMove search */
   CmdOptionCS,   	     /* W [on/off] Toggle Scan search */
   CmdOptionPL,   	     /* pattern search node limit */
   CmdOptionMM,   	     /* K [on/off] Toggle minimization */
   CmdOptionLP,   	     /* Y [on/off] Toggle limit patterns */
   CmdOptionLM,   	     /* m [on/off] Toggle lazy maxmization */
   CmdOptionST,   	     /* X [on/off] Toggle store tested */
   CmdOptionPT,   	     /* P [number] Switch Pattern DBs */
   CmdOptionMP,   	     /* M [on/off] Toggle LB manpos */
   CmdOptionCF,   	     /* C [on/off] Toggle LB conflict */
   CmdOptionDD,   	     /* d [on/off] Toggle dynamic distances */
   CmdOptionTM,   	     /* Tunnel Macro switch */
   CmdOptionGM,   	     /* General Goal Macro switch */
   CmdOptionGO,   	     /* Goal Macro switch */
   CmdOptionCG,   	     /* Cut Goal Macro */
   CmdOptionXD,   	     /* Cut Goal Macro */
   CmdOptionLC,   	     /* Local Cut (k,m) */
   CmdOptionLA,   	     /* Auto Set Local Cut Params */
   CmdOptionOE,   	     /* Overestimation */
   CmdOptionAD,   	     /* Assume Dead */
   CmdOptionMO,   	     /* Move Order Index */

   CmdShow,		     /* */
   CmdShowConfl,	     /* */
   CmdShowXDist,	     /* */
   CmdShowSDist,	     /* */
   CmdShowMDist,	     /* */

   /* Navigator Menu Commands */
   CmdNavigator,	     /* Navigator Menu */
   CmdNavEX,		     /* Navigator Menu */
   CmdNavPatt,		     /* Navigate through patterns */
   CmdNavOnDeep,	     /* Set on_deepest */
   CmdNavOnPS,		     /* Set on_ps_start */

   CmdNavPrNM,		     /* print? number_moves */
   CmdNavPrUA,		     /* set update at count */

   CmdLastCommand	     /* Just last command, not used */
} COMMAND;


typedef struct {
  char    *keys;             /* Key code for command. */
  COMMAND  cmd;              /* Command type. */
  char    *text;             /* Information text. */
} CMDMENU;

#define SZ_CMDSTR             8000      /* Size of the input command buffer */
#define COMMAND_IS(buff,cmd) !strncmp(cmd,buff,MIN(strlen(cmd),strlen(buff)) )

extern int Cur_Maze_Number;
extern IDA MainIdaInfo;

void MainMenu();
void OptionsMenu();
void ShowMenu();
COMMAND  GetCommand(char prompt[], CMDMENU *pcmdMenu, char cmdstr[] );
char    *CmdParam( char cmdstr[], int no );
void     ParseMakeMoves(char *param);

