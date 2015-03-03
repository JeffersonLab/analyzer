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
 *  Code to swap an array containing EVIO composite format
 *  data in place. To be used with the function eviofmt for
 *  analyzing the format string of the composite data.
 *  
 * Author:  Sergey Boiarinov, Hall B
 */
 

#include <stdio.h>
#include "evio.h"

#undef DEBUG

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

typedef struct {
    int left;    /* index of ifmt[] element containing left parenthesis */
    int nrepeat; /* how many times format in parenthesis must be repeated */
    int irepeat; /* right parenthesis counter, or how many times format
                    in parenthesis already repeated */
} LV;


/**
 * This function converts (swaps) an array of EVIO composite type data
 * between IEEE (big endian) and DECS (little endian) in place. This
 * data does <b>NOT</b> include the composite type's beginning tagsegment and
 * the format string it contains. It also does <b>NOT</b> include the data's
 * bank header words.
 *
 * Converts the data of array (iarr[i], i=0...nwrd-1)
 * using the format code      (ifmt[j], j=0...nfmt-1) .
 *
 * <p>
 * Algorithm description:<p>
 * Data processed inside while (ib &lt; nwrd)
 * loop, where 'ib' is iarr[] index;
 * loop breaks when 'ib' reaches the number of elements in iarr[]
 *
 *
 * @param iarr    pointer to data to be swapped
 * @param nwrd    number of data words (32-bit ints) to be swapped
 * @param ifmt    unsigned char array holding translated format
 * @param nfmt    length of unsigned char array, ifmt, in # of chars
 * @param tolocal if 0 data is of same endian as local host,
 *                else data is of opposite endian
 *
 * @return  0 if success
 * @return -1 if nwrd or nfmt arg(s) < 0
 */
int eviofmtswap(uint32_t *iarr, int nwrd, unsigned char *ifmt, int nfmt, int tolocal) {

    int      imt, ncnf, kcnf, lev, iterm;
    int64_t *b64, *b64end;
    int32_t *b32, *b32end;
    int16_t *b16, *b16end;
    int8_t  *b8,  *b8end;
    LV lv[10];

    if (nwrd <= 0 || nfmt <= 0) return(-1);

    imt   = 0;  /* ifmt[] index */
    lev   = 0;  /* parenthesis level */
    ncnf  = 0;  /* how many times must repeat a format */
    iterm = 0;

    b8    = (int8_t *)&iarr[0];    /* beginning of data */
    b8end = (int8_t *)&iarr[nwrd]; /* end of data + 1 */

#ifdef DEBUG
    printf("\n======== eviofmtswap ==========\n");
#endif

    while (b8 < b8end) {
#ifdef DEBUG
        printf("+++ begin = 0x%08x, end = 0x%08x\n", b8, b8end);
#endif
        /* get next format code */
        while (1) {
            imt++;
            /* end of format statement reached, back to iterm - last parenthesis or format begining */
            if (imt > nfmt) {
                imt = iterm;
#ifdef DEBUG
                printf("1\n");
#endif
            }
            /* meet right parenthesis, so we're finished processing format(s) in parenthesis */
            else if (ifmt[imt-1] == 0) {
                /* increment counter */
                lv[lev-1].irepeat ++;
                
                /* if format in parenthesis was processed */
                if (lv[lev-1].irepeat >= lv[lev-1].nrepeat) {
                    /* required number of times */
                    
                    /* store left parenthesis index minus 1
                       (if will meet end of format, will start from format index imt=iterm;
                       by default we continue from the beginning of the format (iterm=0)) */
                    iterm = lv[lev-1].left - 1;
                    lev--; /* done with this level - decrease parenthesis level */
#ifdef DEBUG
                    printf("2\n");
#endif
                }
                /* go for another round of processing by setting 'imt' to the left parenthesis */
                else {
                    /* points ifmt[] index to the left parenthesis from the same level 'lev' */
                    imt = lv[lev-1].left;
#ifdef DEBUG
                    printf("3\n");
#endif
                }
            }
            else {
                /* how many times to repeat format code */
                ncnf = ifmt[imt-1]/16;
                /* format code */
                kcnf = ifmt[imt-1] - 16*ncnf;
        
                /* left parenthesis, SPECIAL case: # of repeats must be taken from data */
                if (kcnf == 15) {
                    /* set it to regular left parenthesis code */
                    kcnf = 0;
                    /* get # of repeats from data (watch out for endianness) */
                    b32 = (int32_t *)b8;
                    if (!tolocal) ncnf = *b32;
                    *b32 = EVIO_SWAP32(*b32);
                    if (tolocal) ncnf = *b32;
                    b8 += 4;
#ifdef DEBUG
                    printf("\n*1 ncnf from data = %#10.8x, b8 = 0x%08x\n",ncnf, b8);
#endif
                }
        
                /* left parenthesis - set new lv[] */
                if (kcnf == 0) {
                    /* store ifmt[] index */
                    lv[lev].left = imt;
                    /* how many time will repeat format code inside parenthesis */
                    lv[lev].nrepeat = ncnf;
                    /* cleanup place for the right parenthesis (do not get it yet) */
                    lv[lev].irepeat = 0;
                    /* increase parenthesis level */
                    lev++;
#ifdef DEBUG
                    printf("4\n");
#endif
                }
                /* format F or I or ... */
                else {
                    /* there are no parenthesis, just simple format, go to processing */
                    if (lev == 0) {
#ifdef DEBUG
                        printf("51\n");
#endif
                    }
                    /* have parenthesis, NOT the pre-last format element (last assumed ')' ?) */
                    else if (imt != (nfmt-1)) {
#ifdef DEBUG
                        printf("52: %d %d\n",imt,nfmt-1);
#endif
                    }
                    /* have parenthesis, NOT the first format after the left parenthesis */
                    else if (imt != lv[lev-1].left+1) {
#ifdef DEBUG
                        printf("53: %d %d\n",imt,nfmt-1);
#endif
                    }
                    /* if none of above, we are in the end of format */
                    else {
                        /* set format repeat to the big number */
                        ncnf = 999999999;
#ifdef DEBUG
                        printf("54\n");
#endif
                    }
                    break;
                }
            }
        } /* while(1) */


        if (ncnf == 0) {
            /* get # of repeats from data (watch out for endianness) */
            b32 = (int32_t *)b8;
            if (!tolocal) ncnf = *b32;
            *b32 = EVIO_SWAP32(*b32);
            if (tolocal) ncnf = *b32;
            b8 += 4;
#ifdef DEBUG
            printf("\n*2 ncnf from data = %d\n",ncnf);
#endif
        }


        /* At this point we are ready to process next piece of data.
           We have following entry info:
             kcnf - format type
             ncnf - how many times format repeated
             b8   - starting data pointer (it comes from previously processed piece)
        */

        /* Convert data in according to type kcnf */
    
        /* 64-bits */
        if (kcnf == 8 || kcnf == 9 || kcnf == 10) {
            b64 = (int64_t *)b8;
            b64end = b64 + ncnf;
            if (b64end > (int64_t *)b8end) b64end = (int64_t *)b8end;
            while (b64 < b64end) *b64++ = EVIO_SWAP64(*b64);
            b8 = (int8_t *)b64;
#ifdef DEBUG
            printf("64bit: %d elements\n",ncnf);
#endif
        }
        /* 32-bits */
        else if ( kcnf == 1 || kcnf == 2 || kcnf == 11 || kcnf == 12) {
            b32 = (int32_t *)b8;
            b32end = b32 + ncnf;
            if (b32end > (int32_t *)b8end) b32end = (int32_t *)b8end;
            while (b32 < b32end) *b32++ = EVIO_SWAP32(*b32);
            b8 = (int8_t *)b32;
#ifdef DEBUG
            printf("32bit: %d elements, b8 = 0x%08x\n",ncnf, b8);
#endif
        }
        /* 16 bits */
        else if ( kcnf == 4 || kcnf == 5) {
            b16 = (int16_t *)b8;
            b16end = b16 + ncnf;
            if (b16end > (int16_t *)b8end) b16end = (int16_t *)b8end;
            while (b16 < b16end) *b16++ = EVIO_SWAP16(*b16);
            b8 = (int8_t *)b16;
#ifdef DEBUG
            printf("16bit: %d elements\n",ncnf);
#endif
        }
        /* 8 bits */
        else if ( kcnf == 6 || kcnf == 7 || kcnf == 3) {
            /* do nothing for characters */
            b8 += ncnf;
#ifdef DEBUG
            printf("8bit: %d elements\n",ncnf);
#endif
        }

    } /* while(b8 < b8end) */

    return(0);
}
