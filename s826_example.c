///////////////////////////////////////////////////////////////////////////////////////////////
// SENSORAY MODEL 826 PROGRAMMING EXAMPLES
// This file contains simple functions that show how to program the 826.
// Copyright (C) 2012 Sensoray
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _LINUX
#include <windows.h>
#include <conio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef _LINUX
#include "..\826api.h"
#else
#include "826api.h"
#endif



// Helpful macros for DIOs
#define DIO(C)                  ((uint64)1 << (C))                          // convert dio channel number to uint64 bit mask
#define DIOMASK(N)              {(uint)(N) & 0xFFFFFF, (uint)((N) >> 24)}   // convert uint64 bit mask to uint[2] array
#define DIOSTATE(STATES,CHAN)   ((STATES[CHAN / 24] >> (CHAN % 24)) & 1)    // extract dio channel's boolean state from uint[2] array


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ERROR HANDLING
// These examples employ very simple error handling: if an error is detected, the example functions will immediately return an error code.
// This behavior may not be suitable for some real-world applications but it makes the code easier to read and understand. In a real
// application, it's likely that additional actions would need to be performed. The examples use the following X826 macro to handle API
// function errors; it calls an API function and stores the returned value in errcode, then returns immediately if an error was detected.

#define X826(FUNC)   if ((errcode = FUNC) != S826_ERR_OK) { printf("\nERROR: %d\n", errcode); return errcode;}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Counter utility functions used by the demos.

// PERIODIC TIMER --------------------

// Counter mode: count down at 1MHz, preload upon start or when 0 reached, assert ExtOut when not 0
#define TMR_MODE  (S826_CM_K_1MHZ | S826_CM_UD_REVERSE | S826_CM_PX_ZERO | S826_CM_PX_START | S826_CM_OM_NOTZERO)

// Configure a counter channel to operate as a periodic timer and start it running.
static int PeriodicTimerStart(uint board, uint counter, uint period)
{
    int errcode;
    X826( S826_CounterStateWrite(board, counter, 0)                         );     // halt channel if it's running
    X826( S826_CounterModeWrite(board, counter, TMR_MODE)                   );     // configure counter as periodic timer
    X826( S826_CounterSnapshotConfigWrite(board, counter, 4, S826_BITWRITE) );     // capture snapshots at zero counts
    X826( S826_CounterPreloadWrite(board, counter, 0, period)               );     // timer period in microseconds
    X826( S826_CounterStateWrite(board, counter, 1)                         );     // start timer
    return errcode;
}

// Halt channel operating as periodic timer.
static int PeriodicTimerStop(uint board, uint counter)
{
    return S826_CounterStateWrite(board, counter, 0);
}

// Wait for periodic timer event.
static int PeriodicTimerWait(uint board, uint counter, uint *timestamp)
{
    uint counts, tstamp, reason;    // counter snapshot
    int errcode = S826_CounterSnapshotRead(board, counter, &counts, &tstamp, &reason, S826_WAIT_INFINITE);    // wait for timer snapshot
    if (timestamp != NULL)
        *timestamp = tstamp;
    return errcode;
}

// PWM GENERATOR -----------------------

// Counter mode: count down at 1MHz, preload when 0 reached, use both preload registers, assert ExtOut when Preload1 is active
#define PWM_MODE  (S826_CM_K_1MHZ | S826_CM_UD_REVERSE | S826_CM_PX_ZERO | S826_CM_PX_START | S826_CM_BP_BOTH | S826_CM_OM_PRELOAD)

//#define PWM_MODE 0x01682020

// Configure a counter channel to operate as a pwm generator and start it running.
static int PwmGeneratorStart(uint board, uint counter, uint ontime, uint offtime)
{
    int errcode;
    X826( S826_CounterStateWrite(board, counter, 0)                         );  // halt counter channel if it's running
    X826( S826_CounterModeWrite(board, counter, PWM_MODE)                   );  // configure counter as pwm generator
    X826( S826_CounterSnapshotConfigWrite(board, counter, 0, S826_BITWRITE) );  // don't need counter snapshots -- we're just outputting pwm signal
    X826( S826_CounterPreloadWrite(board, counter, 0, offtime)              );  // program pwm on-time in microseconds
    X826( S826_CounterPreloadWrite(board, counter, 1, ontime)               );  // program pwm off-time in microseconds
    X826( S826_CounterStateWrite(board, counter, 1)                         );  // start pwm generator
    return errcode;
}

// The demo
static int TestDacRW(uint board)
{
    uint safemode = 0;
    uint setpoint = 0;
    uint range;
    uint chan = 0;
    int errcode;
    uint i;

    printf("\nTestDacRW\n");



    X826( S826_SafeWrenWrite(board, 2) );
    X826( S826_DacRangeWrite(board, 0, S826_DAC_SPAN_5_5, safemode)       );  // program dac output range: -10V to +10V
    X826( S826_DacRangeWrite(board, 1, S826_DAC_SPAN_5_5, safemode) );
    X826( S826_DacRangeWrite(board, 2, S826_DAC_SPAN_5_5, safemode) );

    for (i = 0; i < 0xFFFF; i++)
    {
        printf("%i\n", i);
        X826( S826_DacDataWrite(board, chan, i, safemode) );
        if (i != setpoint)
            printf("%d != %d\n", i, setpoint);

    }

    X826( S826_DacDataWrite(board, 0, 0x8000, safemode) );
    X826( S826_DacDataWrite(board, 1, 0x8000, safemode) );
    X826( S826_DacDataWrite(board, 2, 0x8000, safemode) );

    return errcode;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main function.

int main(int argc, char **argv)
{
    uint board      = 0;                        // change this if you want to use other than board number 0
    int errcode     = S826_ERR_OK;
    int boardflags  = S826_SystemOpen();        // open 826 driver and find all 826 boards
    if (argc > 1)
        board = atoi(argv[1]);

    if (boardflags < 0)
        errcode = boardflags;                       // problem during open
    else if ((boardflags & (1 << board)) == 0) {
        int i;
        printf("TARGET BOARD of index %d NOT FOUND\n",board);         // driver didn't find board you want to use
        for (i = 0; i < 8; i++) {
            if (boardflags & (1 << i)) {
                printf("board %d detected. try \"./s826demo %d\"\n", i, i);
            }
        }
    } else  {
        X826( TestDacRW(board) );
    }

    switch (errcode)
    {
    case S826_ERR_OK:           break;
    case S826_ERR_BOARD:        printf("Illegal board number"); break;
    case S826_ERR_VALUE:        printf("Illegal argument"); break;
    case S826_ERR_NOTREADY:     printf("Device not ready or timeout"); break;
    case S826_ERR_CANCELLED:    printf("Wait cancelled"); break;
    case S826_ERR_DRIVER:       printf("Driver call failed"); break;
    case S826_ERR_MISSEDTRIG:   printf("Missed adc trigger"); break;
    case S826_ERR_DUPADDR:      printf("Two boards have same number"); break;S826_SafeWrenWrite(board, 0x02);
    case S826_ERR_BOARDCLOSED:  printf("Board not open"); break;
    case S826_ERR_CREATEMUTEX:  printf("Can't create mutex"); break;
    case S826_ERR_MEMORYMAP:    printf("Can't map board"); break;
    default:                    printf("Unknown error"); break;
    }


#ifndef _LINUX
    printf("\nKeypress to exit ...\n\n");
    while (!_kbhit());
    _getch();
#endif

    S826_SystemClose();
    return 0;
}
