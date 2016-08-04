#ifndef _LINUX
#include <windows.h>
#include <conio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <./hiredis/hiredis.h>

#ifndef _LINUX
#include "..\826api.h"
#else
#include "826api.h"
#endif

#define ASCII_0_VALU 48
#define ASCII_9_VALU 57
#define ASCII_A_VALU 65
#define ASCII_F_VALU 70


// Helpful macros for DIOs
#define DIO(C)                  ((uint64)1 << (C))                          // convert dio channel number to uint64 bit mask
#define DIOMASK(N)              {(uint)(N) & 0xFFFFFF, (uint)((N) >> 24)}   // convert uint64 bit mask to uint[2] array
#define DIOSTATE(STATES,CHAN)   ((STATES[CHAN / 24] >> (CHAN % 24)) & 1)    // extract dio channel's boolean state from uint[2] array
#define X826(FUNC)   if ((errcode = FUNC) != S826_ERR_OK) { printf("\nERROR: %d\n", errcode); return errcode;}
#define TMR_MODE  (S826_CM_K_1MHZ | S826_CM_UD_REVERSE | S826_CM_PX_ZERO | S826_CM_PX_START | S826_CM_OM_NOTZERO)

static int NRCstart(uint board)
{
    uint safemode = 0;
    int errcode;

    printf("Neuromorphic Robot Control\n");

    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }

    X826( S826_SafeWrenWrite(board, 0) );
    X826( S826_SafeWrenWrite(board, 1) );
    X826( S826_SafeWrenWrite(board, 2) );

    X826( S826_DacRangeWrite(board, 0, S826_DAC_SPAN_5_5, safemode) );  // program dac output range: -10V to +10V
    X826( S826_DacRangeWrite(board, 1, S826_DAC_SPAN_5_5, safemode) );
    X826( S826_DacRangeWrite(board, 2, S826_DAC_SPAN_5_5, safemode) );


    //X826( S826_DacDataWrite(board, 0, 0x8000, safemode) );

    while(1) {
      redisReply *y = redisCommand(c, "GET yaw");
      X826( S826_DacDataWrite(board, 2, strtoul(y->str, 0, 0), safemode) );

      redisReply *p = redisCommand(c, "GET pitch");
      X826( S826_DacDataWrite(board, 0, strtoul(p->str, 0, 0), safemode) );
      printf("%s\n", p->str);
    }

    return errcode;
}

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
        X826( NRCstart(board) );
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
