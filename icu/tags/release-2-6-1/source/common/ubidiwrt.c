/*  
******************************************************************************
*
*   Copyright (C) 2000-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  ubidiwrt.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999aug06
*   created by: Markus W. Scherer
*
* This file contains implementations for BiDi functions that use
* the core algorithm and core API to write reordered text.
*/

/* set import/export definitions */
#ifndef U_COMMON_IMPLEMENTATION
#   define U_COMMON_IMPLEMENTATION
#endif

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "cmemory.h"
#include "ustr_imp.h"
#include "ubidiimp.h"

/*
 * The function implementations in this file are designed
 * for UTF-16 and UTF-32, not for UTF-8.
 *
 * Assumptions that are not true for UTF-8:
 * - Any code point always needs the same number of code units
 *   ("minimum-length-problem" of UTF-8)
 * - The BiDi control characters need only one code unit each
 *
 * Further assumptions for all UTFs:
 * - u_charMirror(c) needs the same number of code units as c
 */
#if UTF_SIZE==8
# error reimplement ubidi_writeReordered() for UTF-8, see comment above
#endif

/** BiDi control code points */
enum {
    LRM_CHAR=0x200e,
    RLM_CHAR,
    LRE_CHAR=0x202a,
    RLE_CHAR,
    PDF_CHAR,
    LRO_CHAR,
    RLO_CHAR
};

#define IS_BIDI_CONTROL_CHAR(c) (((uint32_t)(c)&0xfffffffe)==LRM_CHAR || (uint32_t)((c)-LRE_CHAR)<5)
#define IS_COMBINING(type) ((1UL<<(type))&(1UL<<U_NON_SPACING_MARK|1UL<<U_COMBINING_SPACING_MARK|1UL<<U_ENCLOSING_MARK))

/*
 * When we have UBIDI_OUTPUT_REVERSE set on ubidi_writeReordered(), then we
 * semantically write RTL runs in reverse and later reverse them again.
 * Instead, we actually write them in forward order to begin with.
 * However, if the RTL run was to be mirrored, we need to mirror here now
 * since the implicit second reversal must not do it.
 * It looks strange to do mirroring in LTR output, but it is only because
 * we are writing RTL output in reverse.
 */
static int32_t
doWriteForward(const UChar *src, int32_t srcLength,
               UChar *dest, int32_t destSize,
               uint16_t options,
               UErrorCode *pErrorCode) {
    /* optimize for several combinations of options */
    switch(options&(UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_DO_MIRRORING)) {
    case 0: {
        /* simply copy the LTR run to the destination */
        int32_t length=srcLength;
        if(destSize<length) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        do {
            *dest++=*src++;
        } while(--length>0);
        return srcLength;
    }
    case UBIDI_DO_MIRRORING: {
        /* do mirroring */
        int32_t i=0, j=0;
        UChar32 c;

        if(destSize<srcLength) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        do {
            UTF_NEXT_CHAR(src, i, srcLength, c);
            c=u_charMirror(c);
            UTF_APPEND_CHAR_UNSAFE(dest, j, c);
        } while(i<srcLength);
        return srcLength;
    }
    case UBIDI_REMOVE_BIDI_CONTROLS: {
        /* copy the LTR run and remove any BiDi control characters */
        int32_t remaining=destSize;
        UChar c;
        do {
            c=*src++;
            if(!IS_BIDI_CONTROL_CHAR(c)) {
                if(--remaining<0) {
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;

                    /* preflight the length */
                    while(--srcLength>0) {
                        c=*src++;
                        if(!IS_BIDI_CONTROL_CHAR(c)) {
                            --remaining;
                        }
                    }
                    return destSize-remaining;
                }
                *dest++=c;
            }
        } while(--srcLength>0);
        return destSize-remaining;
    }
    default: {
        /* remove BiDi control characters and do mirroring */
        int32_t remaining=destSize;
        int32_t i, j=0;
        UChar32 c;
        do {
            i=0;
            UTF_NEXT_CHAR(src, i, srcLength, c);
            src+=i;
            srcLength-=i;
            if(!IS_BIDI_CONTROL_CHAR(c)) {
                remaining-=i;
                if(remaining<0) {
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;

                    /* preflight the length */
                    while(srcLength>0) {
                        c=*src++;
                        if(!IS_BIDI_CONTROL_CHAR(c)) {
                            --remaining;
                        }
                        --srcLength;
                    }
                    return destSize-remaining;
                }
                c=u_charMirror(c);
                UTF_APPEND_CHAR_UNSAFE(dest, j, c);
            }
        } while(srcLength>0);
        return j;
    }
    } /* end of switch */
}

static int32_t
doWriteReverse(const UChar *src, int32_t srcLength,
               UChar *dest, int32_t destSize,
               uint16_t options,
               UErrorCode *pErrorCode) {
    /*
     * RTL run -
     *
     * RTL runs need to be copied to the destination in reverse order
     * of code points, not code units, to keep Unicode characters intact.
     *
     * The general strategy for this is to read the source text
     * in backward order, collect all code units for a code point
     * (and optionally following combining characters, see below),
     * and copy all these code units in ascending order
     * to the destination for this run.
     *
     * Several options request whether combining characters
     * should be kept after their base characters,
     * whether BiDi control characters should be removed, and
     * whether characters should be replaced by their mirror-image
     * equivalent Unicode characters.
     */
    int32_t i, j;
    UChar32 c;

    /* optimize for several combinations of options */
    switch(options&(UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_DO_MIRRORING|UBIDI_KEEP_BASE_COMBINING)) {
    case 0:
        /*
         * With none of the "complicated" options set, the destination
         * run will have the same length as the source run,
         * and there is no mirroring and no keeping combining characters
         * with their base characters.
         */
        if(destSize<srcLength) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        destSize=srcLength;

        /* preserve character integrity */
        do {
            /* i is always after the last code unit known to need to be kept in this segment */
            i=srcLength;

            /* collect code units for one base character */
            UTF_BACK_1(src, 0, srcLength);

            /* copy this base character */
            j=srcLength;
            do {
                *dest++=src[j++];
            } while(j<i);
        } while(srcLength>0);
        break;
    case UBIDI_KEEP_BASE_COMBINING:
        /*
         * Here, too, the destination
         * run will have the same length as the source run,
         * and there is no mirroring.
         * We do need to keep combining characters with their base characters.
         */
        if(destSize<srcLength) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        destSize=srcLength;

        /* preserve character integrity */
        do {
            /* i is always after the last code unit known to need to be kept in this segment */
            i=srcLength;

            /* collect code units and modifier letters for one base character */
            do {
                UTF_PREV_CHAR(src, 0, srcLength, c);
            } while(srcLength>0 && IS_COMBINING(u_charType(c)));

            /* copy this "user character" */
            j=srcLength;
            do {
                *dest++=src[j++];
            } while(j<i);
        } while(srcLength>0);
        break;
    default:
        /*
         * With several "complicated" options set, this is the most
         * general and the slowest copying of an RTL run.
         * We will do mirroring, remove BiDi controls, and
         * keep combining characters with their base characters
         * as requested.
         */
        if(!(options&UBIDI_REMOVE_BIDI_CONTROLS)) {
            i=srcLength;
        } else {
            /* we need to find out the destination length of the run,
               which will not include the BiDi control characters */
            int32_t length=srcLength;
            UChar ch;

            i=0;
            do {
                ch=*src++;
                if(!IS_BIDI_CONTROL_CHAR(ch)) {
                    ++i;
                }
            } while(--length>0);
            src-=srcLength;
        }

        if(destSize<i) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return i;
        }
        destSize=i;

        /* preserve character integrity */
        do {
            /* i is always after the last code unit known to need to be kept in this segment */
            i=srcLength;

            /* collect code units for one base character */
            UTF_PREV_CHAR(src, 0, srcLength, c);
            if(options&UBIDI_KEEP_BASE_COMBINING) {
                /* collect modifier letters for this base character */
                while(srcLength>0 && IS_COMBINING(u_charType(c))) {
                    UTF_PREV_CHAR(src, 0, srcLength, c);
                }
            }

            if(options&UBIDI_REMOVE_BIDI_CONTROLS && IS_BIDI_CONTROL_CHAR(c)) {
                /* do not copy this BiDi control character */
                continue;
            }

            /* copy this "user character" */
            j=srcLength;
            if(options&UBIDI_DO_MIRRORING) {
                /* mirror only the base character */
                int32_t k=0;
                c=u_charMirror(c);
                UTF_APPEND_CHAR_UNSAFE(dest, k, c);
                dest+=k;
                j+=k;
            }
            while(j<i) {
                *dest++=src[j++];
            }
        } while(srcLength>0);
        break;
    } /* end of switch */

    return destSize;
}

U_CAPI int32_t U_EXPORT2
ubidi_writeReverse(const UChar *src, int32_t srcLength,
                   UChar *dest, int32_t destSize,
                   uint16_t options,
                   UErrorCode *pErrorCode) {
    int32_t destLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* more error checking */
    if( src==NULL || srcLength<-1 ||
        destSize<0 || (destSize>0 && dest==NULL))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* do input and output overlap? */
    if( dest!=NULL &&
        ((src>=dest && src<dest+destSize) ||
         (dest>=src && dest<src+srcLength)))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(srcLength==-1) {
        srcLength=u_strlen(src);
    }
    if(srcLength>0) {
        destLength=doWriteReverse(src, srcLength, dest, destSize, options, pErrorCode);
    } else {
        /* nothing to do */
        destLength=0;
    }

    return u_terminateUChars(dest, destSize, destLength, pErrorCode);
}

#define MASK_R_AL (1UL<<U_RIGHT_TO_LEFT|1UL<<U_RIGHT_TO_LEFT_ARABIC)

U_CAPI int32_t U_EXPORT2
ubidi_writeReordered(UBiDi *pBiDi,
                     UChar *dest, int32_t destSize,
                     uint16_t options,
                     UErrorCode *pErrorCode) {
    const UChar *text;
    UChar *saveDest;
    int32_t length, destCapacity;
    int32_t run, runCount, logicalStart, runLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* more error checking */
    if( pBiDi==NULL ||
        (text=ubidi_getText(pBiDi))==NULL || (length=ubidi_getLength(pBiDi))<0 ||
        destSize<0 || (destSize>0 && dest==NULL))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* do input and output overlap? */
    if( dest!=NULL &&
        ((text>=dest && text<dest+destSize) ||
         (dest>=text && dest<text+length)))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(length==0) {
        /* nothing to do */
        return u_terminateUChars(dest, destSize, 0, pErrorCode);
    }

    runCount=ubidi_countRuns(pBiDi, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* destSize shrinks, later destination length=destCapacity-destSize */
    saveDest=dest;
    destCapacity=destSize;

    /*
     * If we do not perform the "inverse BiDi" algorithm, then we
     * don't need to insert any LRMs, and don't need to test for it.
     */
    if(!ubidi_isInverse(pBiDi)) {
        options&=~UBIDI_INSERT_LRM_FOR_NUMERIC;
    }

    /*
     * Iterate through all visual runs and copy the run text segments to
     * the destination, according to the options.
     *
     * The tests for where to insert LRMs ignore the fact that there may be
     * BN codes or non-BMP code points at the beginning and end of a run;
     * they may insert LRMs unnecessarily but the tests are faster this way
     * (this would have to be improved for UTF-8).
     *
     * Note that the only errors that are set by doWriteXY() are buffer overflow
     * errors. Ignore them until the end, and continue for preflighting.
     */
    if(!(options&UBIDI_OUTPUT_REVERSE)) {
        /* forward output */
        if(!(options&UBIDI_INSERT_LRM_FOR_NUMERIC)) {
            /* do not insert BiDi controls */
            for(run=0; run<runCount; ++run) {
                if(UBIDI_LTR==ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength)) {
                    runLength=doWriteForward(text+logicalStart, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                } else {
                    runLength=doWriteReverse(text+logicalStart, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                }
                dest+=runLength;
                destSize-=runLength;
            }
        } else {
            /* insert BiDi controls for "inverse BiDi" */
            const UChar *src;
            UBiDiDirection dir;

            for(run=0; run<runCount; ++run) {
                dir=ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength);
                src=text+logicalStart;

                if(UBIDI_LTR==dir) {
                    if(/*run>0 &&*/ u_charDirection(*src)!=U_LEFT_TO_RIGHT) {
                        if(destSize>0) {
                            *dest++=LRM_CHAR;
                        }
                        --destSize;
                    }

                    runLength=doWriteForward(src, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                    dest+=runLength;
                    destSize-=runLength;

                    if(/*run<runCount-1 &&*/ u_charDirection(src[runLength-1])!=U_LEFT_TO_RIGHT) {
                        if(destSize>0) {
                            *dest++=LRM_CHAR;
                        }
                        --destSize;
                    }
                } else {
                    if(/*run>0 &&*/ !(MASK_R_AL&1UL<<u_charDirection(src[runLength-1]))) {
                        if(destSize>0) {
                            *dest++=RLM_CHAR;
                        }
                        --destSize;
                    }

                    runLength=doWriteReverse(src, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                    dest+=runLength;
                    destSize-=runLength;

                    if(/*run<runCount-1 &&*/ !(MASK_R_AL&1UL<<u_charDirection(*src))) {
                        if(destSize>0) {
                            *dest++=RLM_CHAR;
                        }
                        --destSize;
                    }
                }
            }
        }
    } else {
        /* reverse output */
        if(!(options&UBIDI_INSERT_LRM_FOR_NUMERIC)) {
            /* do not insert BiDi controls */
            for(run=runCount; --run>=0;) {
                if(UBIDI_LTR==ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength)) {
                    runLength=doWriteReverse(text+logicalStart, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                } else {
                    runLength=doWriteForward(text+logicalStart, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                }
                dest+=runLength;
                destSize-=runLength;
            }
        } else {
            /* insert BiDi controls for "inverse BiDi" */
            const UChar *src;
            UBiDiDirection dir;

            for(run=runCount; --run>=0;) {
                /* reverse output */
                dir=ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength);
                src=text+logicalStart;

                if(UBIDI_LTR==dir) {
                    if(/*run<runCount-1 &&*/ u_charDirection(src[runLength-1])!=U_LEFT_TO_RIGHT) {
                        if(destSize>0) {
                            *dest++=LRM_CHAR;
                        }
                        --destSize;
                    }

                    runLength=doWriteReverse(src, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                    dest+=runLength;
                    destSize-=runLength;

                    if(/*run>0 &&*/ u_charDirection(*src)!=U_LEFT_TO_RIGHT) {
                        if(destSize>0) {
                            *dest++=LRM_CHAR;
                        }
                        --destSize;
                    }
                } else {
                    if(/*run<runCount-1 &&*/ !(MASK_R_AL&1UL<<u_charDirection(*src))) {
                        if(destSize>0) {
                            *dest++=RLM_CHAR;
                        }
                        --destSize;
                    }

                    runLength=doWriteForward(src, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                    dest+=runLength;
                    destSize-=runLength;

                    if(/*run>0 &&*/ !(MASK_R_AL&1UL<<u_charDirection(src[runLength-1]))) {
                        if(destSize>0) {
                            *dest++=RLM_CHAR;
                        }
                        --destSize;
                    }
                }
            }
        }
    }

    return u_terminateUChars(saveDest, destCapacity, destCapacity-destSize, pErrorCode);
}
