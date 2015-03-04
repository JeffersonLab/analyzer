/*-----------------------------------------------------------------------------
 * Copyright (c) 2012  Jefferson Science Associates
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: timmer@jlab.org  Tel: (757) 249-7100  Fax: (757) 269-6248
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *  Code to translate a string containing a data format
 *  into an array of integer codes. To be used with the
 *  function eviofmtswap for swapping data held in this
 *  format.
 *  
 * Author:  Sergey Boiarinov, Hall B
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "evio.h"

#define MAX(a,b)  ( (a) > (b) ? (a) : (b) )

#define debugprint(ii) \
  printf("  [%3d] %3d (16 * %2d + %2d), nr=%d\n",ii,ifmt[ii],ifmt[ii]/16,ifmt[ii]-(ifmt[ii]/16)*16,nr)

#undef DEBUG

/**
 * <pre>
 *  This routine transforms a composite, format-containing
 *  ASCII string to an unsigned char array. It is to be used
 *  in conjunction with {@link #eviofmtswap} to swap the endianness of
 *  composite data.
 * 
 *   format code bits <- format in ascii form
 *     [7:4] [3:0]
 *       #     0           #'('
 *       0    15           #'(' same as above, but have to take # from the data (32-bit)
 *       0     0            ')'
 *       #     1           #'i'   unsigned int
 *       #     2           #'F'   floating point
 *       #     3           #'a'   8-bit char (C++)
 *       #     4           #'S'   short
 *       #     5           #'s'   unsigned short
 *       #     6           #'C'   char
 *       #     7           #'c'   unsigned char
 *       #     8           #'D'   double (64-bit float)
 *       #     9           #'L'   long long (64-bit int)
 *       #    10           #'l'   unsigned long long (64-bit int)
 *       #    11           #'I'   int
 *       #    12           #'A'   hollerit (4-byte char with int endining)
 *
 *   NOTES:
 *    1. If format ends but end of data did not reach, format in last parenthesis
 *       will be repeated until all data processed; if there are no parenthesis
 *       in format, data processing will be started from the beginnig of the format
 *       (FORTRAN agreement)
 *    2. The number of repeats '#' must be the number between 2 and 15; if the number
 *       of repeats is symbol 'N' instead of the number, it will be taken from data
 *       assuming 'int' format
 * </pre>
 * 
 *  @param fmt     null-terminated composite data format string
 *  @param ifmt    unsigned char array to hold transformed format
 *  @param ifmtLen length of unsigned char array, ifmt, in # of chars
 * 
 *  @return the number of bytes in ifmt[] (positive)
 *  @return -1 to -8 for improper format string
 *  @return -9 if unsigned char array is too small
 */
int eviofmt(char *fmt, unsigned char *ifmt, int ifmtLen) {

    char ch;
    int  l, n, kf, lev, nr, nn;

    n   = 0; /* ifmt[] index */
    nr  = 0;
    nn  = 1;
    lev = 0;

#ifdef DEBUG
    printf("\nfmt >%s<\n",fmt);
#endif

    /* loop over format string */
    for (l=0; l < strlen(fmt); l++) {
        ch = fmt[l];
        if (ch == ' ') continue;
#ifdef DEBUG
        printf("%c\n",ch);
#endif
        /* if digit, following before komma will be repeated 'number' times */
        if (isdigit(ch)) {
            if (nr < 0) return(-1);
            nr = 10*MAX(0,nr) + atoi((char *)&ch);
            if (nr > 15) return(-2);
#ifdef DEBUG
            printf("the number of repeats nr=%d\n",nr);
#endif
        }
        /* a left parenthesis -> 16*nr + 0 */
        else if (ch == '(') {
            if (nr < 0) return(-3);
            if (--ifmtLen < 0) return(-9);
            lev++;
#ifdef DEBUG
            printf("111: nn=%d nr=%d\n",nn,nr);
#endif
            if (nn == 0) ifmt[n++] = 15; /*special case: if #repeats is in data, use code '15'*/
            else         ifmt[n++] = 16*MAX(nn,nr);

            nn = 1;
            nr = 0;
#ifdef DEBUG
            debugprint(n-1);
#endif
        }
        /* a right parenthesis -> 16*0 + 0 */
        else if (ch == ')') {
            if (nr >= 0) return(-4);
            if (--ifmtLen < 0) return(-9);
            lev--;
            ifmt[n++] = 0;
            nr = -1;
#ifdef DEBUG
            debugprint(n-1);
#endif
        }
        /* a komma, reset nr */
        else if (ch == ',') {
            if (nr >= 0) return(-5);
            nr = 0;
#ifdef DEBUG
            printf("komma, nr=%d\n",nr);
#endif
        }
        /* variable length format */
        else if (ch == 'N') {
            nn = 0;
#ifdef DEBUG
            printf("nn\n");
#endif
        }
        /* actual format */
        else {
            if(     ch == 'i') kf = 1;  /* 32 */
            else if(ch == 'F') kf = 2;  /* 32 */
            else if(ch == 'a') kf = 3;  /*  8 */
            else if(ch == 'S') kf = 4;  /* 16 */
            else if(ch == 's') kf = 5;  /* 16 */
            else if(ch == 'C') kf = 6;  /*  8 */
            else if(ch == 'c') kf = 7;  /*  8 */
            else if(ch == 'D') kf = 8;  /* 64 */
            else if(ch == 'L') kf = 9;  /* 64 */
            else if(ch == 'l') kf = 10; /* 64 */
            else if(ch == 'I') kf = 11; /* 32 */
            else if(ch == 'A') kf = 12; /* 32 */
            else               kf = 0;

            if (kf != 0) {
                if (nr < 0) return(-6);
                if (--ifmtLen < 0) return(-9);
#ifdef DEBUG
                printf("222: nn=%d nr=%d\n",nn,nr);
#endif
                ifmt[n++] = 16*MAX(nn,nr) + kf;
                nn=1;
#ifdef DEBUG
                debugprint(n-1);
#endif
            }
            else {
                /* illegal character */
                return(-7);
            }
            nr = -1;
        }
    }

    if (lev != 0) return(-8);

#ifdef DEBUG
    printf("\n");
#endif

    return(n);
}
