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
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
__declspec(dllimport) BOOL WINAPI QueryUnbiasedInterruptTime(PULONGLONG UnbiasedTime);
#else
#include <sys/time.h>
#endif

#ifdef _WIN32
HANDLE hTimer = NULL;
#endif

#define MILLISECONDS_PER_SECOND 1000
#define NANOSECONDS_PER_MILLISECOND 1000000

/* this code was generously made available by Diego Novillo
 * and Peter van Beek */

/*
 *  Signal handler for when timer expires.
 */
static void
#ifdef _WIN32
CALLBACK expire(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
#else
expire()
#endif
{
        MainIdaInfo.TimedOut = YES;
}

/*
 *  Set a timer (an alarm) that will expire after ``time_limit'' seconds.
 *  The timer counts in virtual time (CPU time) or real time, depending
 *  on ``type''.
 */
void Set_Timer()
{
#ifdef _WIN32
	BOOL   timer_created;
	DWORD  due_time_ms;
#else
        struct itimerval value;
	int    time_limit;
	int    type;
#endif

	MainIdaInfo.TimedOut = NO;

	/* set alarm only, if time limit was set */
	if (MainIdaInfo.TimeOut<=0) return;

#ifdef _WIN32
	due_time_ms = MainIdaInfo.TimeOut * 1000;
	timer_created = CreateTimerQueueTimer(&hTimer, NULL, expire, NULL, due_time_ms, 0, 0);
	if (!timer_created) { /* BD */
		hTimer = NULL;
		My_exit(1, "Timer allocation failed. Error code: %lu\n", GetLastError());
	};
#else
	time_limit = MainIdaInfo.TimeOut;
	type = MainIdaInfo.TimeOutType;

        /*
         *  Setting it_interval to zero causes timer to be disabled
         *  after its next expiration (assuming it_value is non-zero).
         */
        value.it_interval.tv_sec = 0;
        value.it_interval.tv_usec = 0;

        /*
         *  If it_value is non-zero, it indicates the time to the next
         *  timer expiration.
         */
        value.it_value.tv_sec = time_limit;
        value.it_value.tv_usec = 0;

        /*
         *  Set the signal handler and start the timer.
         */
        if( type == REAL ) {
                /*
                 *  ITIMER_REAL is a timer that decrements in real time.
                 *  A SIGALRM signal is delivered when it expires.
                 */
                signal( SIGALRM, expire );
                setitimer( ITIMER_REAL, &value, (struct itimerval *) NULL );
        }
        else
        if( type == VIRTUAL ) {
                /*
                 *  ITIMER_VIRTUAL is a timer that decrements in process
                 *  virtual time.  It runs only when the process is executing.
                 *  A SIGVTALRM signal is delivered when it expires.
                 */
                signal( SIGVTALRM, expire );
                setitimer( ITIMER_VIRTUAL, &value, (struct itimerval *) NULL );
        }
#endif
}

void Remove_Timer()
{
#ifdef _WIN32
        if (hTimer != NULL) {
            /* Wait for any running callback to complete before removing the timer - BD */
            DeleteTimerQueueTimer(NULL, hTimer, INVALID_HANDLE_VALUE);
            hTimer = NULL;
        }
#else
        signal( MainIdaInfo.TimeOutType == REAL ? SIGALRM : SIGVTALRM, SIG_IGN );
#endif
}

/* Common function to get the start and stop times depending on the platform */
/* This function measures time that is not affected by hibernation or sleep */
uint64_t GetTimeInMilliseconds() { /* CM */
#ifdef _WIN32
	ULONGLONG unbiased_time_100ns;
	QueryUnbiasedInterruptTime(&unbiased_time_100ns);
	return (unbiased_time_100ns / (NANOSECONDS_PER_MILLISECOND / 100));
#else
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return (time.tv_sec * MILLISECONDS_PER_SECOND) + (time.tv_nsec / NANOSECONDS_PER_MILLISECOND);
#endif
}

// Function to calculate the elapsed time in seconds, rounded to the nearest second
uint64_t CalculateElapsedTimeSeconds(uint64_t startTimeMS, uint64_t stopTimeMS) { /* BD */
	uint64_t elapsedMilliseconds = stopTimeMS - startTimeMS;
	return (elapsedMilliseconds + MILLISECONDS_PER_SECOND / 2) / MILLISECONDS_PER_SECOND;
}
