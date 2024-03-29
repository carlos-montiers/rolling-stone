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

typedef struct {
	unsigned short tt:1;		/* HashTable */
	unsigned short dl_mg:1;		/* use DeadLock det. in moveGen */
	unsigned short dl2_mg:1;	/* use DeadLock2 in moveGen */
	unsigned short dl_srch:1;	/* use DeadMove search */
	unsigned short pen_srch:1;	/* use PenMove search */
	unsigned short area_srch:1;	/* use AreaMove search */
	unsigned short scan_srch:1;	/* use Scan search */
	unsigned short minimize:1;	/* minimize patterns */
	unsigned short limit_pat:1;	/* limit patterns */
	unsigned short lazy_max:1;	/* lazy cuts in maximize penalties */
	unsigned short st_testd:1;	/* store tested patterns */
	unsigned short lb_mp:1;		/* LB using manpos (backward-forward) */
	unsigned short lb_cf:1;		/* LB using conflicts */
	unsigned short lb_dd:1;		/* LB using dynamic distances */
	unsigned short mc_tu:1;		/* Macro using tunnels */
	unsigned short mc_gm:1;		/* General Goal Macros */
	unsigned short cut_goal:1;	/* goal_push cut */
	unsigned short xdist:1;		/* use the extended distance measure */
	unsigned short local:1;  	/* local cut */
	unsigned short autolocal:1;  	/* auto set local cut */
	unsigned short assumedead:1;	/* if run out of search effort, assume
					   dead pattern? */
	float	       overestim;	/* overestimation allowed ? */
	float	       hoverestim;	/* h overestimation allowed ? */
	short          local_k;		/* k parameter of local cut */
	short          local_m;		/* m parameter of local cut */
	short          local_d;		/* m parameter of local cut */
	int            dl_db;		/* Deadlock using Pat. DBs */
} OPTIONS;

extern OPTIONS Options;
extern int32_t total_node_count, mini_node_count;
extern int32_t  scan_node_count, super_node_count;
extern int32_t penscount, penmcount, deadscount, deadmcount;

void InitNodeCount();
void IncNodeCount(int depth);

void init_stats();
void init_opts();
void print_stats(int pri);

