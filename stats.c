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
#include <stdint.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/resource.h>
#endif

MOVE DummyMove = {ENDPATH,ENDPATH,ENDPATH,ENDPATH};
int32_t area_pos_nc=0, area_neg_nc=0;	/* node counts for pos/neg searches */
int  area_pos_sc=0, area_neg_sc=0;	/* search count for pos/neg */
int32_t dl_pos_nc=0, dl_neg_nc=0;	/* node counts for pos/neg searches */
int  dl_pos_sc=0, dl_neg_sc=0;		/* search count for pos/neg */
int32_t pen_pos_nc=0, pen_neg_nc=0;	/* node counts for pos/neg searches */
int  pen_pos_sc=0, pen_neg_sc=0;	/* search count for pos/neg */

OPTIONS Options;

int32_t total_node_count, mini_node_count, scan_node_count;
int32_t pattern_counter[256];

int64_t start_time;			/* in microseconds */

int64_t GetUSec()
{
#ifdef _WIN32
	FILETIME creationtime, exittime, kerneltime, usertime;
	GetProcessTimes(GetCurrentProcess(), &creationtime, &exittime, &kerneltime, &usertime);
	ULARGE_INTEGER uli;
	uli.LowPart = usertime.dwLowDateTime;
	uli.HighPart = usertime.dwHighDateTime;
	return uli.QuadPart / 10;
#else
	struct rusage r_usage;
	getrusage(RUSAGE_SELF,&r_usage);
	return (int64_t)r_usage.ru_utime.tv_sec * 1000000 + r_usage.ru_utime.tv_usec;
#endif
}

void InitNodeCount()
{
	total_node_count = mini_node_count = scan_node_count = 0;
	MainIdaInfo.node_count = 0;
	IdaInfo->node_count = 0;

	start_time = GetUSec();
}

void IncNodeCount(int dth) {
	total_node_count++;
	IdaInfo->node_count++;
	IdaInfo->nodes_depth[dth]++;
}

void init_opts() {
	Options.tt=1;
	Options.dl_mg=1;
	Options.dl2_mg=1;
	Options.dl_srch=1;
	Options.pen_srch=1;
	Options.area_srch=1;
	Options.scan_srch=1;
	Options.minimize=1;
	Options.limit_pat=1;
	Options.lazy_max=1;
	Options.st_testd=1;
	Options.dl_db=7;
	Options.lb_mp=1;
	Options.lb_cf=1;
	Options.lb_dd=1;
	Options.mc_tu=1;
	Options.mc_gm=1;
	Options.cut_goal=1;
	Options.xdist=1;
	Options.local=1;
	Options.autolocal=1;
	Options.overestim=0.0;
	Options.hoverestim=1.0;
	Options.assumedead=0;
	Options.local_k=-1;
	Options.local_m=-1;
	Options.local_d=max(YSIZE,XSIZE);
}

void init_stats() {

	int i;

	for (i=0; i<MAX_DEPTH; i++) {
		IdaInfo->nodes_depth[i]=0;
	}

	IdaInfo->tt_hits    = 0;
	IdaInfo->tt_cols    = 0;
	IdaInfo->tt_reqs    = 0;
}

void print_stats(int pri) {
	int i,ttl;
	time_t t;
	int64_t usec;
	int seconds, useconds;

	Debug(pri,0, "tt: %c, dl_mg: %c, dl2_mg: %c\n",
		Options.tt==1?'Y':'N', Options.dl_mg==1?'Y':'N',
		Options.dl2_mg==1?'Y':'N');
	Debug(pri,0,"area_srch: %c dl_srch: %c pen_srch: %c\n",
		Options.area_srch==1?'Y':'N', Options.dl_srch==1?'Y':'N',
		Options.pen_srch==1?'Y':'N');
	Debug(pri,0,"scan_srch: %c\n", Options.scan_srch==1?'Y':'N');
	Debug(pri,0, "node limit: %i, minimize: %c\n",
		MainIdaInfo.pattern_node_limit, Options.minimize==1?'Y':'N');
	Debug(pri,0,"st_testd: %c, dl_db: %i, cut_goal: %c\n",
		Options.st_testd==1?'Y':'N', Options.dl_db,
		Options.cut_goal==1?'Y':'N');
	Debug(pri,0,"xdist: %c, auto: %c, local: %c(%i,%i,%i)\n",
		Options.xdist==1?'Y':'N',
		Options.autolocal==1?'Y':'N', Options.local==1?'Y':'N',
		Options.local_k,Options.local_m,Options.local_d);
	Debug(pri,0, "lb_mp: %c, lb_cf: %c, lb_dd: %c(%d/%d), mc_tu: %c, mc_gm: %c\n",
		Options.lb_mp==1?'Y':'N', Options.lb_cf==1?'Y':'N',
		Options.lb_dd==1?'Y':'N',
		MainIdaInfo.dcache_hits,
		MainIdaInfo.IdaMaze==0?0:MainIdaInfo.IdaMaze->number_d_cache,
		Options.mc_tu==1?'Y':'N', Options.mc_gm==1?'Y':'N');
	Debug(pri,0, "overestim: %4.2f, hoverestim: %4.2f, assume dead: %c\n",
		Options.overestim, Options.hoverestim, Options.assumedead==1?'Y':'N');
	Debug(pri,0, "limit patterns: %c, lazy maximize: %c\n",
		Options.limit_pat==1?'Y':'N', Options.lazy_max==1?'Y':'N');
	Debug(pri,0,"nodes searched: total: %" PRIi32 ", top level: %" PRIi32 "\n",
		total_node_count, IdaInfo->node_count);
	Debug(pri,0,"mini: %" PRIi32 ", scan: %" PRIi32 "\n",
		mini_node_count, scan_node_count);
	Debug(pri,0,"AbortNodeCount: %" PRIi32 ", TimeOut: %i TimeOutType: %s\n",
		MainIdaInfo.AbortNodeCount, MainIdaInfo.TimeOut,
		MainIdaInfo.TimeOutType==REAL?"REAL":"VIRTUAL");
	Debug(pri,0,"TT hits: %" PRIi32 ", TT collisions: %" PRIi32 " Req: %" PRIi32 ", (%f)\n",
		IdaInfo->tt_hits,
		IdaInfo->tt_cols,
		IdaInfo->tt_reqs,
		((float)100*IdaInfo->tt_hits)/IdaInfo->tt_reqs);
        i = ttl = 0;
        while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
                Mprintf( 0, " %" PRIi32, IdaInfo->nodes_depth[i]);
                ttl += IdaInfo->nodes_depth[i];
                i++;
        }
	if (IdaInfo->PrintPriority >= pri) {
		Mprintf( 0, " \n");

		usec = GetUSec() - start_time;
		seconds = usec / 1000000;
		useconds = usec % 1000000;
		t = seconds;
		if (useconds>500000) t++;
		if (t==0) t=1;
	} else {
		seconds = 1;
		useconds = 0;
		t=1;
	}

	Debug(pri,0,"run: sec: %li, usec: %li, t: %li\n",
			seconds,useconds,t);
	Debug(pri,0,"Nodes per Second: %8.0f (%" PRId32 " sec.)\n",
			(float)total_node_count/t,t);
	Debug(pri,0,"AREA POS: #: %5i (%3i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  area_pos_sc,
		  (int)(100*area_pos_sc)/
		    (area_pos_sc+area_neg_sc+(area_pos_sc+area_neg_sc==0?1:0)),
		  area_pos_nc,(area_pos_sc==0)?0:(int)area_pos_nc/area_pos_sc);
	Debug(pri,0,"AREA NEG: #: %5i (%3i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  area_neg_sc,
		  (int)(100*area_neg_sc)/
		    (area_pos_sc+area_neg_sc+(area_pos_sc+area_neg_sc==0?1:0)),
		  area_neg_nc,(area_neg_sc==0)?0:(int)area_neg_nc/area_neg_sc);
	Debug(pri,0,"DL   POS: #: %5i (%3i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  dl_pos_sc,
		  (int)(100*dl_pos_sc)/
		       (dl_pos_sc+dl_neg_sc+(dl_pos_sc+dl_neg_sc==0?1:0)),
		  dl_pos_nc,(dl_pos_sc==0)?0:(int)dl_pos_nc/dl_pos_sc);
	Debug(pri,0,"DL   NEG: #: %5i (%3i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  dl_neg_sc,
		  (int)(100*dl_neg_sc)/
		       (dl_pos_sc+dl_neg_sc+(dl_pos_sc+dl_neg_sc==0?1:0)),
		  dl_neg_nc,(dl_neg_sc==0)?0:(int)dl_neg_nc/dl_neg_sc);
	Debug(pri,0,"PEN  POS: #: %5i (%3i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  pen_pos_sc,
		  (int)(100*pen_pos_sc)/
		       (pen_pos_sc+pen_neg_sc+(pen_pos_sc+pen_neg_sc==0?1:0)),
		  pen_pos_nc,(pen_pos_sc==0)?0:(int)pen_pos_nc/pen_pos_sc);
	Debug(pri,0,"PEN  NEG: #: %5i (%3i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  pen_neg_sc,
		  (int)(100*pen_neg_sc)/
		       (pen_pos_sc+pen_neg_sc+(pen_pos_sc+pen_neg_sc==0?1:0)),
		  pen_neg_nc,(pen_neg_sc==0)?0:(int)pen_neg_nc/pen_neg_sc);
	if (MainIdaInfo.IdaMaze!=NULL)
	Debug(pri,0,"DeadTested: %6i, PenTested : %6i, patterns: %i, removed: %i, killed: %i\n",
		MainIdaInfo.IdaMaze->conflicts->number_deadtested,
		MainIdaInfo.IdaMaze->conflicts->number_pentested,
		MainIdaInfo.IdaMaze->conflicts->number_patterns,
		MainIdaInfo.IdaMaze->conflicts->number_removed,
		MainIdaInfo.IdaMaze->conflicts->number_killed);
	Debug(pri,0,"\n");
}

