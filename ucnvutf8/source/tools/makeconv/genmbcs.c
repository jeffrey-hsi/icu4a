/*
*******************************************************************************
*
*   Copyright (C) 2000-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  genmbcs.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000jul06
*   created by: Markus W. Scherer
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "cstring.h"
#include "cmemory.h"
#include "unewdata.h"
#include "ucnv_cnv.h"
#include "ucnvmbcs.h"
#include "ucm.h"
#include "makeconv.h"
#include "genmbcs.h"

/*
 * TODO: Split this file into toUnicode, SBCSFromUnicode and MBCSFromUnicode files.
 * Reduce tests for maxCharLength.
 */

#define UTF8_FRIENDLY 1

/* TODO: remove */
static int countFallbacks=0;

typedef struct MBCSData {
    NewConverter newConverter;

    UCMFile *ucm;

    /* toUnicode (state table in ucm->states) */
    _MBCSToUFallback toUFallbacks[MBCS_MAX_FALLBACK_COUNT];
    int32_t countToUFallbacks;
    uint16_t *unicodeCodeUnits;

    /* fromUnicode */
    uint16_t stage1[MBCS_STAGE_1_SIZE];
    uint16_t stage2Single[MBCS_STAGE_2_SIZE]; /* stage 2 for single-byte codepages */
    uint32_t stage2[MBCS_STAGE_2_SIZE]; /* stage 2 for MBCS */
    uint8_t *fromUBytes;
    uint32_t stage2Top, stage3Top;

    /* fromUTF8 */
    uint16_t stageUTF8[MBCS_UTF8_STAGE_SIZE];
#if 0
    uint16_t stage1UTF8[MBCS_UTF8_STAGE_1_SIZE];
    uint16_t stage2UTF8[MBCS_UTF8_STAGE_2_SIZE];
    uint32_t stage2UTF8Top;
#endif

    UBool utf8Friendly;
} MBCSData;

/* prototypes */
static void
MBCSClose(NewConverter *cnvData);

static UBool
MBCSStartMappings(MBCSData *mbcsData);

static UBool
MBCSAddToUnicode(MBCSData *mbcsData,
                 const uint8_t *bytes, int32_t length,
                 UChar32 c,
                 int8_t flag);

static UBool
MBCSIsValid(NewConverter *cnvData,
            const uint8_t *bytes, int32_t length);

static UBool
MBCSSingleAddFromUnicode(MBCSData *mbcsData,
                         const uint8_t *bytes, int32_t length,
                         UChar32 c,
                         int8_t flag);

static UBool
MBCSAddFromUnicode(MBCSData *mbcsData,
                   const uint8_t *bytes, int32_t length,
                   UChar32 c,
                   int8_t flag);

static void
MBCSPostprocess(MBCSData *mbcsData, const UConverterStaticData *staticData);

static UBool
MBCSAddTable(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData);

static uint32_t
MBCSWrite(NewConverter *cnvData, const UConverterStaticData *staticData,
          UNewDataMemory *pData, int32_t tableType);

/* helper ------------------------------------------------------------------- */

static U_INLINE char
hexDigit(uint8_t digit) {
    return digit<=9 ? (char)('0'+digit) : (char)('a'-10+digit);
}

static U_INLINE char *
printBytes(char *buffer, const uint8_t *bytes, int32_t length) {
    char *s=buffer;
    while(length>0) {
        *s++=hexDigit((uint8_t)(*bytes>>4));
        *s++=hexDigit((uint8_t)(*bytes&0xf));
        ++bytes;
        --length;
    }

    *s=0;
    return buffer;
}

/* implementation ----------------------------------------------------------- */

static void
MBCSInit(MBCSData *mbcsData, UCMFile *ucm) {
    uprv_memset(mbcsData, 0, sizeof(MBCSData));

    mbcsData->ucm=ucm; /* aliased, not owned */

    mbcsData->newConverter.close=MBCSClose;
    mbcsData->newConverter.isValid=MBCSIsValid;
    mbcsData->newConverter.addTable=MBCSAddTable;
    mbcsData->newConverter.write=MBCSWrite;
}

NewConverter *
MBCSOpen(UCMFile *ucm) {
    MBCSData *mbcsData=(MBCSData *)uprv_malloc(sizeof(MBCSData));
    if(mbcsData!=NULL) {
        MBCSInit(mbcsData, ucm);
    }
    return &mbcsData->newConverter;
}

static void
MBCSDestruct(MBCSData *mbcsData) {
    uprv_free(mbcsData->unicodeCodeUnits);
    uprv_free(mbcsData->fromUBytes);
}

static void
MBCSClose(NewConverter *cnvData) {
    MBCSData *mbcsData=(MBCSData *)cnvData;
    if(mbcsData!=NULL) {
        MBCSDestruct(mbcsData);
        uprv_free(mbcsData);
    }
}

static UBool
MBCSStartMappings(MBCSData *mbcsData) {
    int32_t i, sum, maxCharLength,
            stage2NullLength, stage2AllocLength,
            stage3NullLength, stage3AllocLength;

    /* toUnicode */

    /* allocate the code unit array and prefill it with "unassigned" values */
    sum=mbcsData->ucm->states.countToUCodeUnits;
    if(VERBOSE) {
        printf("the total number of offsets is 0x%lx=%ld\n", (long)sum, (long)sum);
    }

    if(sum>0) {
        mbcsData->unicodeCodeUnits=(uint16_t *)uprv_malloc(sum*sizeof(uint16_t));
        if(mbcsData->unicodeCodeUnits==NULL) {
            fprintf(stderr, "error: out of memory allocating %ld 16-bit code units\n",
                (long)sum);
            return FALSE;
        }
        for(i=0; i<sum; ++i) {
            mbcsData->unicodeCodeUnits[i]=0xfffe;
        }
    }

    /* fromUnicode */
    maxCharLength=mbcsData->ucm->states.maxCharLength;

    /* allocate the codepage mappings and preset the first 16 characters to 0 */
    if(maxCharLength==1) {
        /* allocate 64k 16-bit results for single-byte codepages */
        sum=0x20000;
    } else {
        /* allocate 1M * maxCharLength bytes for at most 1M mappings */
        sum=0x100000*maxCharLength;
    }
    mbcsData->fromUBytes=(uint8_t *)uprv_malloc(sum);
    if(mbcsData->fromUBytes==NULL) {
        fprintf(stderr, "error: out of memory allocating %ld B for target mappings\n", (long)sum);
        return FALSE;
    }
    uprv_memset(mbcsData->fromUBytes, 0, sum);

    /*
     * UTF-8-friendly fromUnicode tries: allocate multiple blocks at a time.
     * See ucnvmbcs.h for details.
     * TODO: Document that there is code, for example in
     * ucnv_MBCSGetUnicodeSetForUnicode(), which assumes that the initial
     * stage 2/3 blocks are the all-unassigned ones.
     *
     * UTF-8-friendly fromUnicode tries work from sorted tables and are built
     * pre-compacted, overlapping adjacent stage 2/3 blocks.
     * Non-UTF-8-friendly fromUnicode tries work from unsorted tables
     * (because implicit precision is used), and are compacted
     * in post-processing.
     *
     * Preallocation for UTF-8-friendly fromUnicode tries:
     *
     * Stage 3:
     * 64-entry all-unassigned first block followed by ASCII (128 entries).
     *
     * Stage 2:
     * 64-entry all-unassigned first block followed by preallocated
     * 64-block for ASCII.
     *
     * TODO: remove comment from 2006-jul
     * Stage 2:
     * SBCS: Regular 64-entry all-unassigned first block followed by preallocated
     *   128-block for fast two-byte UTF-8 lookup of U+0000..U+07ff.
     * MBCS: 256-entry all-unassigned first block followed by preallocated
     *   256-block for fast UTF-8 lookup of U+0000..U+0fff.
     */
    /*
     * New 20060712:
     * Preallocate ASCII as a linear 128-entry stage 3 block.
     */
    stage2NullLength=MBCS_STAGE_2_BLOCK_SIZE;
    stage2AllocLength=MBCS_STAGE_2_BLOCK_SIZE;

    stage3NullLength=MBCS_UTF8_STAGE_3_BLOCK_SIZE;
    stage3AllocLength=128; /* ASCII U+0000..U+007f */

    /* Initialize stage 1 for the preallocated blocks. */
    sum=stage2NullLength;
    for(i=0; i<(stage2AllocLength>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT); ++i) {
        mbcsData->stage1[i]=sum;
#if 0
        mbcsData->stage1UTF8[i]=sum;
#endif
        sum+=MBCS_STAGE_2_BLOCK_SIZE;
    }
    mbcsData->stage2Top=/* TODO: remove mbcsData->stage2UTF8Top=*/stage2NullLength+stage2AllocLength; /* ==sum */

    /*
     * Stage 2 indexes count 16-blocks in stage 3 as follows:
     * SBCS: directly, indexes increment by 16
     * MBCS: indexes need to be multiplied by 16*maxCharLength, indexes increment by 1
     * MBCS UTF-8: directly, indexes increment by 16
     */
    if(maxCharLength==1) {
        sum=stage3NullLength;
        for(i=0; i<(stage3AllocLength/MBCS_STAGE_3_BLOCK_SIZE); ++i) {
            mbcsData->stage2Single[mbcsData->stage1[0]+i]=sum;
            sum+=MBCS_STAGE_3_BLOCK_SIZE;
        }
    } else {
        sum=stage3NullLength/MBCS_STAGE_3_GRANULARITY;
        for(i=0; i<(stage3AllocLength/MBCS_STAGE_3_BLOCK_SIZE); ++i) {
            mbcsData->stage2[mbcsData->stage1[0]+i]=sum;
            sum+=MBCS_STAGE_3_BLOCK_SIZE/MBCS_STAGE_3_GRANULARITY;
        }
    }

    sum=stage3NullLength;
    for(i=0; i<(stage3AllocLength/MBCS_UTF8_STAGE_3_BLOCK_SIZE); ++i) {
        mbcsData->stageUTF8[i]=sum;
#if 0
        mbcsData->stage2UTF8[mbcsData->stage1UTF8[0]+i]=sum;
#endif
        sum+=MBCS_UTF8_STAGE_3_BLOCK_SIZE;
    }

    /*
     * Allocate a 64-entry all-unassigned first stage 3 block,
     * for UTF-8-friendly lookup with a trail byte,
     * plus 128 entries for ASCII.
     */
    mbcsData->stage3Top=(stage3NullLength+stage3AllocLength)*maxCharLength; /* ==sum*maxCharLength */

    return TRUE;
}

/* return TRUE for success */
static UBool
setFallback(MBCSData *mbcsData, uint32_t offset, UChar32 c) {
    int32_t i=ucm_findFallback(mbcsData->toUFallbacks, mbcsData->countToUFallbacks, offset);
    if(i>=0) {
        /* if there is already a fallback for this offset, then overwrite it */
        mbcsData->toUFallbacks[i].codePoint=c;
        return TRUE;
    } else {
        /* if there is no fallback for this offset, then add one */
        i=mbcsData->countToUFallbacks;
        if(i>=MBCS_MAX_FALLBACK_COUNT) {
            fprintf(stderr, "error: too many toUnicode fallbacks, currently at: U+%x\n", (int)c);
            return FALSE;
        } else {
            mbcsData->toUFallbacks[i].offset=offset;
            mbcsData->toUFallbacks[i].codePoint=c;
            mbcsData->countToUFallbacks=i+1;
            return TRUE;
        }
    }
}

/* remove fallback if there is one with this offset; return the code point if there was such a fallback, otherwise -1 */
static int32_t
removeFallback(MBCSData *mbcsData, uint32_t offset) {
    int32_t i=ucm_findFallback(mbcsData->toUFallbacks, mbcsData->countToUFallbacks, offset);
    if(i>=0) {
        _MBCSToUFallback *toUFallbacks;
        int32_t limit, old;

        toUFallbacks=mbcsData->toUFallbacks;
        limit=mbcsData->countToUFallbacks;
        old=(int32_t)toUFallbacks[i].codePoint;

        /* copy the last fallback entry here to keep the list contiguous */
        toUFallbacks[i].offset=toUFallbacks[limit-1].offset;
        toUFallbacks[i].codePoint=toUFallbacks[limit-1].codePoint;
        mbcsData->countToUFallbacks=limit-1;
        return old;
    } else {
        return -1;
    }
}

/*
 * isFallback is almost a boolean:
 * 1 (TRUE)  this is a fallback mapping
 * 0 (FALSE) this is a precise mapping
 * -1        the precision of this mapping is not specified
 */
static UBool
MBCSAddToUnicode(MBCSData *mbcsData,
                 const uint8_t *bytes, int32_t length,
                 UChar32 c,
                 int8_t flag) {
    char buffer[10];
    uint32_t offset=0;
    int32_t i=0, entry, old;
    uint8_t state=0;

    if(mbcsData->ucm->states.countStates==0) {
        fprintf(stderr, "error: there is no state information!\n");
        return FALSE;
    }

    /* for SI/SO (like EBCDIC-stateful), double-byte sequences start in state 1 */
    if(length==2 && mbcsData->ucm->states.outputType==MBCS_OUTPUT_2_SISO) {
        state=1;
    }

    /*
     * Walk down the state table like in conversion,
     * much like getNextUChar().
     * We assume that c<=0x10ffff.
     */
    for(i=0;;) {
        entry=mbcsData->ucm->states.stateTable[state][bytes[i++]];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            if(i==length) {
                fprintf(stderr, "error: byte sequence too short, ends in non-final state %hu: 0x%s (U+%x)\n",
                    (short)state, printBytes(buffer, bytes, length), (int)c);
                return FALSE;
            }
            state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
            offset+=MBCS_ENTRY_TRANSITION_OFFSET(entry);
        } else {
            if(i<length) {
                fprintf(stderr, "error: byte sequence too long by %d bytes, final state %hu: 0x%s (U+%x)\n",
                    (int)(length-i), state, printBytes(buffer, bytes, length), (int)c);
                return FALSE;
            }
            switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
            case MBCS_STATE_ILLEGAL:
                fprintf(stderr, "error: byte sequence ends in illegal state at U+%04x<->0x%s\n",
                    (int)c, printBytes(buffer, bytes, length));
                return FALSE;
            case MBCS_STATE_CHANGE_ONLY:
                fprintf(stderr, "error: byte sequence ends in state-change-only at U+%04x<->0x%s\n",
                    (int)c, printBytes(buffer, bytes, length));
                return FALSE;
            case MBCS_STATE_UNASSIGNED:
                fprintf(stderr, "error: byte sequence ends in unassigned state at U+%04x<->0x%s\n",
                    (int)c, printBytes(buffer, bytes, length));
                return FALSE;
            case MBCS_STATE_FALLBACK_DIRECT_16:
            case MBCS_STATE_VALID_DIRECT_16:
            case MBCS_STATE_FALLBACK_DIRECT_20:
            case MBCS_STATE_VALID_DIRECT_20:
                if(MBCS_ENTRY_SET_STATE(entry, 0)!=MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, 0xfffe)) {
                    /* the "direct" action's value is not "valid-direct-16-unassigned" any more */
                    if(MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_VALID_DIRECT_16 || MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_FALLBACK_DIRECT_16) {
                        old=MBCS_ENTRY_FINAL_VALUE(entry);
                    } else {
                        old=0x10000+MBCS_ENTRY_FINAL_VALUE(entry);
                    }
                    if(flag>=0) {
                        fprintf(stderr, "error: duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                        return FALSE;
                    } else if(VERBOSE) {
                        fprintf(stderr, "duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                    }
                    /*
                     * Continue after the above warning
                     * if the precision of the mapping is unspecified.
                     */
                }
                /* reassign the correct action code */
                entry=MBCS_ENTRY_FINAL_SET_ACTION(entry, (MBCS_STATE_VALID_DIRECT_16+(flag==3 ? 2 : 0)+(c>=0x10000 ? 1 : 0)));

                /* put the code point into bits 22..7 for BMP, c-0x10000 into 26..7 for others */
                if(c<=0xffff) {
                    entry=MBCS_ENTRY_FINAL_SET_VALUE(entry, c);
                } else {
                    entry=MBCS_ENTRY_FINAL_SET_VALUE(entry, c-0x10000);
                }
                mbcsData->ucm->states.stateTable[state][bytes[i-1]]=entry;
                break;
            case MBCS_STATE_VALID_16:
                /* bits 26..16 are not used, 0 */
                /* bits 15..7 contain the final offset delta to one 16-bit code unit */
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                /* check that this byte sequence is still unassigned */
                if((old=mbcsData->unicodeCodeUnits[offset])!=0xfffe || (old=removeFallback(mbcsData, offset))!=-1) {
                    if(flag>=0) {
                        fprintf(stderr, "error: duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                        return FALSE;
                    } else if(VERBOSE) {
                        fprintf(stderr, "duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                    }
                }
                if(c>=0x10000) {
                    fprintf(stderr, "error: code point does not fit into valid-16-bit state at U+%04x<->0x%s\n",
                        (int)c, printBytes(buffer, bytes, length));
                    return FALSE;
                }
                if(flag>0) {
                    /* assign only if there is no precise mapping */
                    if(mbcsData->unicodeCodeUnits[offset]==0xfffe) {
                        return setFallback(mbcsData, offset, c);
                    }
                } else {
                    mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                }
                break;
            case MBCS_STATE_VALID_16_PAIR:
                /* bits 26..16 are not used, 0 */
                /* bits 15..7 contain the final offset delta to two 16-bit code units */
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                /* check that this byte sequence is still unassigned */
                old=mbcsData->unicodeCodeUnits[offset];
                if(old<0xfffe) {
                    int32_t real;
                    if(old<0xd800) {
                        real=old;
                    } else if(old<=0xdfff) {
                        real=0x10000+((old&0x3ff)<<10)+((mbcsData->unicodeCodeUnits[offset+1])&0x3ff);
                    } else /* old<=0xe001 */ {
                        real=mbcsData->unicodeCodeUnits[offset+1];
                    }
                    if(flag>=0) {
                        fprintf(stderr, "error: duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)real);
                        return FALSE;
                    } else if(VERBOSE) {
                        fprintf(stderr, "duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)real);
                    }
                }
                if(flag>0) {
                    /* assign only if there is no precise mapping */
                    if(old<=0xdbff || old==0xe000) {
                        /* do nothing */
                    } else if(c<=0xffff) {
                        /* set a BMP fallback code point as a pair with 0xe001 */
                        mbcsData->unicodeCodeUnits[offset++]=0xe001;
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                    } else {
                        /* set a fallback surrogate pair with two second surrogates */
                        mbcsData->unicodeCodeUnits[offset++]=(uint16_t)(0xdbc0+(c>>10));
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)(0xdc00+(c&0x3ff));
                    }
                } else {
                    if(c<0xd800) {
                        /* set a BMP code point */
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                    } else if(c<=0xffff) {
                        /* set a BMP code point above 0xd800 as a pair with 0xe000 */
                        mbcsData->unicodeCodeUnits[offset++]=0xe000;
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                    } else {
                        /* set a surrogate pair */
                        mbcsData->unicodeCodeUnits[offset++]=(uint16_t)(0xd7c0+(c>>10));
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)(0xdc00+(c&0x3ff));
                    }
                }
                break;
            default:
                /* reserved, must never occur */
                fprintf(stderr, "internal error: byte sequence reached reserved action code, entry 0x%02x: 0x%s (U+%x)\n",
                    (int)entry, printBytes(buffer, bytes, length), (int)c);
                return FALSE;
            }

            return TRUE;
        }
    }
}

/* is this byte sequence valid? (this is almost the same as MBCSAddToUnicode()) */
static UBool
MBCSIsValid(NewConverter *cnvData,
            const uint8_t *bytes, int32_t length) {
    MBCSData *mbcsData=(MBCSData *)cnvData;

    return (UBool)(1==ucm_countChars(&mbcsData->ucm->states, bytes, length));
}

static UBool
MBCSSingleAddFromUnicode(MBCSData *mbcsData,
                         const uint8_t *bytes, int32_t length,
                         UChar32 c,
                         int8_t flag) {
    uint16_t *stage3, *p;
    uint32_t index;
    uint16_t old;
    uint8_t b;

    uint32_t blockSize, newTop, i, nextOffset, newBlock, min;

    /* ignore |2 SUB mappings */
    if(flag==2) {
        return TRUE;
    }

    /*
     * Walk down the triple-stage compact array ("trie") and
     * allocate parts as necessary.
     * Note that the first stage 2 and 3 blocks are reserved for all-unassigned mappings.
     * We assume that length<=maxCharLength and that c<=0x10ffff.
     */
    stage3=(uint16_t *)mbcsData->fromUBytes;
    b=*bytes;

    /* inspect stage 1 */
    index=c>>MBCS_STAGE_1_SHIFT;
    if(mbcsData->utf8Friendly && c<=SBCS_UTF8_MAX) {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK&~(MBCS_UTF8_STAGE_3_BLOCKS-1);
    } else {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK;
    }
    if(mbcsData->stage1[index]==MBCS_STAGE_2_ALL_UNASSIGNED_INDEX) {
        /* allocate another block in stage 2 */
        newBlock=mbcsData->stage2Top;
        if(mbcsData->utf8Friendly) {
            min=newBlock-nextOffset; /* minimum block start with overlap */
            while(min<newBlock && mbcsData->stage2Single[newBlock-1]==0) {
                --newBlock;
            }
        }
        newTop=newBlock+MBCS_STAGE_2_BLOCK_SIZE;

        if(newTop>MBCS_MAX_STAGE_2_TOP) {
            fprintf(stderr, "error: too many stage 2 entries at U+%04x<->0x%02x\n", (int)c, b);
            return FALSE;
        }

        /*
         * each stage 2 block contains 64 16-bit words:
         * 6 code point bits 9..4 with 1 stage 3 index
         */
        mbcsData->stage1[index]=(uint16_t)newBlock;
        mbcsData->stage2Top=newTop;
    }

    /* inspect stage 2 */
    index=mbcsData->stage1[index]+nextOffset;
    if(mbcsData->utf8Friendly && c<=SBCS_UTF8_MAX) {
        /* allocate 64-entry blocks for UTF-8-friendly lookup */
        blockSize=MBCS_UTF8_STAGE_3_BLOCK_SIZE;
        nextOffset=c&MBCS_UTF8_STAGE_3_BLOCK_MASK;
    } else {
        blockSize=MBCS_STAGE_3_BLOCK_SIZE;
        nextOffset=c&MBCS_STAGE_3_BLOCK_MASK;
    }
    if(mbcsData->stage2Single[index]==0) {
        /* allocate another block in stage 3 */
        newBlock=mbcsData->stage3Top;
        if(mbcsData->utf8Friendly) {
            min=newBlock-nextOffset; /* minimum block start with overlap */
            while(min<newBlock && stage3[newBlock-1]==0) {
                --newBlock;
            }
        }
        newTop=newBlock+blockSize;

        if(newTop>MBCS_STAGE_3_SBCS_SIZE) {
            fprintf(stderr, "error: too many code points at U+%04x<->0x%02x\n", (int)c, b);
            return FALSE;
        }
        /* each block has 16 uint16_t entries */
        i=index;
        while(newBlock<newTop) {
            mbcsData->stage2Single[i++]=(uint16_t)newBlock;
            newBlock+=MBCS_STAGE_3_BLOCK_SIZE;
        }
        mbcsData->stage3Top=newTop; /* ==newBlock */
    }

    /* write the codepage entry into stage 3 and get the previous entry */
    p=stage3+mbcsData->stage2Single[index]+nextOffset;
    old=*p;
    if(flag<=0) {
        *p=(uint16_t)(0xf00|b);
    } else if(IS_PRIVATE_USE(c)) {
        *p=(uint16_t)(0xc00|b);
    } else {
        *p=(uint16_t)(0x800|b);
    }

    /* check that this Unicode code point was still unassigned */
    if(old>=0x100) {
        if(flag>=0) {
            fprintf(stderr, "error: duplicate Unicode code point at U+%04x<->0x%02x see 0x%02x\n",
                (int)c, b, old&0xff);
            return FALSE;
        } else if(VERBOSE) {
            fprintf(stderr, "duplicate Unicode code point at U+%04x<->0x%02x see 0x%02x\n",
                (int)c, b, old&0xff);
        }
        /* continue after the above warning if the precision of the mapping is unspecified */
    }

    return TRUE;
}

static UBool
MBCSAddFromUnicode(MBCSData *mbcsData,
                   const uint8_t *bytes, int32_t length,
                   UChar32 c,
                   int8_t flag) {
    char buffer[10];
    const uint8_t *pb;
    uint8_t *stage3, *p;
    uint32_t index, b, old, stage3Index;
    int32_t maxCharLength;

    uint32_t blockSize, newTop, i, nextOffset, newBlock, min, overlap, maxOverlap;

    maxCharLength=mbcsData->ucm->states.maxCharLength;

    if( mbcsData->ucm->states.outputType==MBCS_OUTPUT_2_SISO &&
        (*bytes==0xe || *bytes==0xf)
    ) {
        fprintf(stderr, "error: illegal mapping to SI or SO for SI/SO codepage: U+%04x<->0x%s\n",
            (int)c, printBytes(buffer, bytes, length));
        return FALSE;
    }

    if(flag==1 && length==1 && *bytes==0) {
        fprintf(stderr, "error: unable to encode a |1 fallback from U+%04x to 0x%02x\n",
            (int)c, *bytes);
        return FALSE;
    }

    /*
     * Walk down the triple-stage compact array ("trie") and
     * allocate parts as necessary.
     * Note that the first stage 2 and 3 blocks are reserved for
     * all-unassigned mappings.
     * We assume that length<=maxCharLength and that c<=0x10ffff.
     */
    stage3=mbcsData->fromUBytes;

    /* inspect stage 1 */
    index=c>>MBCS_STAGE_1_SHIFT;
    if(mbcsData->utf8Friendly && c<=MBCS_UTF8_MAX) {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK&~(MBCS_UTF8_STAGE_3_BLOCKS-1);
    } else {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK;
    }
    if(mbcsData->stage1[index]==MBCS_STAGE_2_ALL_UNASSIGNED_INDEX) {
        /* allocate another block in stage 2 */
        newBlock=mbcsData->stage2Top;
        if(mbcsData->utf8Friendly) {
            min=newBlock-nextOffset; /* minimum block start with overlap */
            while(min<newBlock && mbcsData->stage2[newBlock-1]==0) {
                --newBlock;
            }
        }
        newTop=newBlock+MBCS_STAGE_2_BLOCK_SIZE;

        if(newTop>MBCS_MAX_STAGE_2_TOP) {
            fprintf(stderr, "error: too many stage 2 entries at U+%04x<->0x%s\n",
                (int)c, printBytes(buffer, bytes, length));
            return FALSE;
        }

        /*
         * each stage 2 block contains 64 32-bit words:
         * 6 code point bits 9..4 with value with bits 31..16 "assigned" flags and bits 15..0 stage 3 index
         */
        i=index;
        while(newBlock<newTop) {
            mbcsData->stage1[i++]=(uint16_t)newBlock;
            newBlock+=MBCS_STAGE_2_BLOCK_SIZE;
        }
        mbcsData->stage2Top=newTop; /* ==newBlock */
    }

    /* inspect stage 2 */
    index=mbcsData->stage1[index]+nextOffset;
    if(mbcsData->utf8Friendly && c<=MBCS_UTF8_MAX) {
        /* allocate 64-entry blocks for UTF-8-friendly lookup */
        blockSize=MBCS_UTF8_STAGE_3_BLOCK_SIZE*maxCharLength;
        nextOffset=c&MBCS_UTF8_STAGE_3_BLOCK_MASK;
    } else {
        blockSize=MBCS_STAGE_3_BLOCK_SIZE*maxCharLength;
        nextOffset=c&MBCS_STAGE_3_BLOCK_MASK;
    }
    if(mbcsData->stage2[index]==0) {
        /* allocate another block in stage 3 */
        newBlock=mbcsData->stage3Top;
        if(mbcsData->utf8Friendly && nextOffset>=MBCS_STAGE_3_GRANULARITY) {
            /*
             * Overlap stage 3 blocks only in multiples of 16-entry blocks
             * because of the indexing granularity in stage 2.
             */
            maxOverlap=(nextOffset&~(MBCS_STAGE_3_GRANULARITY-1))*maxCharLength;
            for(overlap=0;
                overlap<maxOverlap && stage3[newBlock-overlap-1]==0;
                ++overlap) {}

            overlap=(overlap/MBCS_STAGE_3_GRANULARITY)/maxCharLength;
            overlap=(overlap*MBCS_STAGE_3_GRANULARITY)*maxCharLength;

            newBlock-=overlap;
        }
        newTop=newBlock+blockSize;

        if(newTop>MBCS_STAGE_3_MBCS_SIZE*(uint32_t)maxCharLength) {
            fprintf(stderr, "error: too many code points at U+%04x<->0x%s\n",
                (int)c, printBytes(buffer, bytes, length));
            return FALSE;
        }
        /* each block has 16*maxCharLength bytes */
        i=index;
        while(newBlock<newTop) {
            mbcsData->stage2[i++]=(newBlock/MBCS_STAGE_3_GRANULARITY)/maxCharLength;
            newBlock+=MBCS_STAGE_3_BLOCK_SIZE*maxCharLength;
        }
        mbcsData->stage3Top=newTop; /* ==newBlock */
    }

    stage3Index=MBCS_STAGE_3_GRANULARITY*(uint32_t)(uint16_t)mbcsData->stage2[index];

    /* Build an alternate, UTF-8-friendly stage table as well. */
    if(mbcsData->utf8Friendly && c<=MBCS_UTF8_MAX) {
        /* Overflow for uint16_t entries in stageUTF8? */
        if(stage3Index>0xffff) {
            /*
             * TODO: The following comments are all wrong. Remove them and restructure the code!
             * See comment after this if().
             *
             * This can occur only if the mapping table is nearly perfectly filled.
             * stage3Index can reach exactly MBCS_UTF8_LIMIT because of the pre-allocated
             * initial all-unassigned block.
             *
             * TODO: Think about whether to just return FALSE in this case (with stderr output)
             * and break MBCSAddTable() rather than looping for another round without
             * utf8Friendly.
             * If so, remove MBCSAddTable()'s loop and just hardcode UTF-8-friendliness.
             */
            printf("too many stage 3 entries for UTF-8-friendly format, processing U+%04x<->0x%s\n",
                   (int)c, printBytes(buffer, bytes, length));
            mbcsData->utf8Friendly=FALSE;
            return FALSE;
        }

        /*
         * The stage 3 block has been assigned for the regular trie.
         * Just copy its index into stageUTF8[], without the granularity.
         *
         * It is safe to cast stage3Index to uint16_t because
         * stage3Index<=MBCS_UTF8_LIMIT<0xffff
         * because we have a sorted table and there are at most MBCS_UTF8_LIMIT
         * mappings with 0<=c<MBCS_UTF8_LIMIT, and there is only also
         * the initial all-unassigned block in stage3.
         */
        if(mbcsData->stageUTF8[c>>MBCS_UTF8_STAGE_SHIFT]==0) { /* TODO: remove if() */
        mbcsData->stageUTF8[c>>MBCS_UTF8_STAGE_SHIFT]=(uint16_t)stage3Index;
        }
    }
#if 0
    /* Build alternate, UTF-8-friendly stages 1 and 2 as well. */
    if(mbcsData->utf8Friendly && c<=0xffff) {
        uint32_t index8, nextOffset8;

        if(stage3Index>0xffff) {
            printf("too many stage 3 entries for UTF-8-friendly format, processing U+%04x<->0x%s\n",
                   (int)c, printBytes(buffer, bytes, length));
            mbcsData->utf8Friendly=FALSE;
        }

        /* inspect stage 1 */
        index8=c>>MBCS_UTF8_STAGE_1_SHIFT;
        nextOffset8=(c>>MBCS_UTF8_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK;
        if(mbcsData->stage1UTF8[index8]==0) {
            /* allocate another block in stage 2 */
            newBlock=mbcsData->stage2UTF8Top;
            min=newBlock-nextOffset8; /* minimum block start with overlap */
            while(min<newBlock && mbcsData->stage2UTF8[newBlock-1]==0) {
                --newBlock;
            }
            newTop=newBlock+MBCS_STAGE_2_BLOCK_SIZE;

            if(newTop>MBCS_UTF8_STAGE_2_SIZE) {
                printf("too many stage 2 entries for UTF-8-friendly format, processing U+%04x<->0x%s\n",
                       (int)c, printBytes(buffer, bytes, length));
                mbcsData->utf8Friendly=FALSE;
                index8=0; /* avoid array overflows */
            }

            mbcsData->stage1UTF8[index8]=(uint16_t)newBlock;
            mbcsData->stage2UTF8Top=newTop;
        }

        /*
         * The stage 3 block has been assigned for the regular trie.
         * Just copy its index into stage2UTF8[], without the granularity.
         */
        mbcsData->stage2UTF8[mbcsData->stage1UTF8[index8]+nextOffset8]=(uint16_t)stage3Index;
    }
#endif

    /* write the codepage bytes into stage 3 and get the previous bytes */

    /* assemble the bytes into a single integer */
    pb=bytes;
    b=0;
    switch(length) {
    case 4:
        b=*pb++;
    case 3:
        b=(b<<8)|*pb++;
    case 2:
        b=(b<<8)|*pb++;
    case 1:
    default:
        b=(b<<8)|*pb++;
        break;
    }

    old=0;
    p=stage3+(stage3Index+nextOffset)*maxCharLength;
    switch(maxCharLength) {
    case 2:
        old=*(uint16_t *)p;
        *(uint16_t *)p=(uint16_t)b;
        break;
    case 3:
        old=(uint32_t)*p<<16;
        *p++=(uint8_t)(b>>16);
        old|=(uint32_t)*p<<8;
        *p++=(uint8_t)(b>>8);
        old|=*p;
        *p=(uint8_t)b;
        break;
    case 4:
        old=*(uint32_t *)p;
        *(uint32_t *)p=b;
        break;
    default:
        /* will never occur */
        break;
    }

    /* check that this Unicode code point was still unassigned */
    if((mbcsData->stage2[index+(nextOffset>>MBCS_STAGE_2_SHIFT)]&(1UL<<(16+(c&0xf))))!=0 || old!=0) {
        if(flag>=0) {
            fprintf(stderr, "error: duplicate Unicode code point at U+%04x<->0x%s see 0x%02x\n",
                (int)c, printBytes(buffer, bytes, length), (int)old);
            return FALSE;
        } else if(VERBOSE) {
            fprintf(stderr, "duplicate Unicode code point at U+%04x<->0x%s see 0x%02x\n",
                (int)c, printBytes(buffer, bytes, length), (int)old);
        }
        /* continue after the above warning if the precision of the mapping is
           unspecified */
    }
    if(flag<=0) {
        /* set the roundtrip flag */
        mbcsData->stage2[index+(nextOffset>>4)]|=(1UL<<(16+(c&0xf)));
    }

    return TRUE;
}

U_CFUNC UBool
MBCSOkForBaseFromUnicode(UBool utf8Friendly,
                         const uint8_t *bytes, int32_t length,
                         UChar32 c, int8_t flag) {
    /*
     * A 1:1 mapping does not fit into the MBCS base table's fromUnicode table under
     * the following conditions:
     *
     * - a |2 SUB mapping for <subchar1> (no base table data structure for them)
     * - a |1 fallback to 0x00 (result value 0, indistinguishable from unmappable entry)
     * - a multi-byte mapping with leading 0x00 bytes (no explicit length field)
     *
     * Some of these tests are redundant with ucm_mappingType().
     */
    if( (flag==2 && length==1) ||
        (flag==1 && bytes[0]==0) || /* testing length==1 would be redundant with the next test */
        (flag<=1 && length>1 && bytes[0]==0)
    ) {
        return FALSE;
    }

    /*
     * Additional restrictions for UTF-8-friendly fromUnicode tables,
     * for code points up to the maximum optimized one:
     *
     * - any mapping to 0x00 (result value 0, indistinguishable from unmappable entry)
     * - any |1 fallback (no roundtrip flags in the optimized table)
     */
    if(utf8Friendly && flag<=1 && c<=MBCS_UTF8_MAX && (bytes[0]==0 || flag==1)) {
        return FALSE;
    }

    /* All other mappings do fit into the base table. */
    return TRUE;
}

/* we can assume that the table only contains 1:1 mappings with <=4 bytes each */
static UBool
MBCSAddTable(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData) {
    MBCSData *mbcsData;
    UCMapping *m;
    UChar32 c;
    int32_t i, maxCharLength;
    int8_t f;
    UBool isOK, utf8Friendly;

    staticData->unicodeMask=table->unicodeMask;
    if(staticData->unicodeMask==3) {
        fprintf(stderr, "error: contains mappings for both supplementary and surrogate code points\n");
        return FALSE;
    }

    staticData->conversionType=UCNV_MBCS;

    mbcsData=(MBCSData *)cnvData;
    maxCharLength=mbcsData->ucm->states.maxCharLength;

    /*
     * Generation of UTF-8-friendly data requires
     * a sorted table, which makeconv generates when explicit precision
     * indicators are used.
     */
    mbcsData->utf8Friendly=utf8Friendly=(UBool)(
        UTF8_FRIENDLY &&
        (table->flagsType&UCM_FLAGS_EXPLICIT)!=0);

    do {
        if(!MBCSStartMappings(mbcsData)) {
            return FALSE;
        }

countFallbacks=0;
        staticData->hasFromUnicodeFallback=FALSE;
        staticData->hasToUnicodeFallback=FALSE;

        isOK=TRUE;

        m=table->mappings;
        for(i=0; i<table->mappingsLength; ++m, ++i) {
            c=m->u;
            f=m->f;
            if(f>=0) {
                f&=MBCS_FROM_U_EXT_MASK;
            }

            switch(f) {
            case -1:
                /* there was no precision/fallback indicator */
                /* fall through to set the mappings */
            case 0:
                /* set roundtrip mappings */
                isOK&=MBCSAddToUnicode(mbcsData, m->b.bytes, m->bLen, c, f);

                if(maxCharLength==1) {
                    isOK&=MBCSSingleAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
                    m->f=f;
                    m->moveFlag=0;
                } else if(MBCSOkForBaseFromUnicode(utf8Friendly, m->b.bytes, m->bLen, c, f)) {
                    isOK&=MBCSAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
                    m->f=f;
                    m->moveFlag=0;
                } else {
                    m->f|=MBCS_FROM_U_EXT_FLAG;
                    m->moveFlag=UCM_MOVE_TO_EXT;
                }
                break;
            case 1:
                /* set only a fallback mapping from Unicode to codepage */
                if(maxCharLength==1) {
++countFallbacks;
                    staticData->hasFromUnicodeFallback=TRUE;
                    isOK&=MBCSSingleAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
                    m->f=f;
                    m->moveFlag=0;
                } else if(MBCSOkForBaseFromUnicode(utf8Friendly, m->b.bytes, m->bLen, c, f)) {
++countFallbacks;
                    staticData->hasFromUnicodeFallback=TRUE;
                    isOK&=MBCSAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
                    m->f=f;
                    m->moveFlag=0;
                } else {
                    m->f|=MBCS_FROM_U_EXT_FLAG;
                    m->moveFlag=UCM_MOVE_TO_EXT;
                }
                break;
            case 2:
                /* ignore |2 SUB mappings, except to move <subchar1> mappings to the extension table */
                if(maxCharLength==1 || MBCSOkForBaseFromUnicode(utf8Friendly, m->b.bytes, m->bLen, c, f)) {
                    m->f=f;
                    m->moveFlag=0;
                } else {
                    m->f|=MBCS_FROM_U_EXT_FLAG;
                    m->moveFlag=UCM_MOVE_TO_EXT;
                }
                break;
            case 3:
                /* set only a fallback mapping from codepage to Unicode */
                staticData->hasToUnicodeFallback=TRUE;
                isOK&=MBCSAddToUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
                break;
            default:
                /* will not occur because the parser checked it already */
                fprintf(stderr, "error: illegal fallback indicator %d\n", f);
                return FALSE;
            }
        }

        if(isOK && utf8Friendly && !mbcsData->utf8Friendly) {
            /* something overflowed; build the data not UTF-8-friendly after all */
            UCMFile *ucm;

            printf("second pass generating conversion data, not UTF-8-friendly due to a data structure limitation\n");
            utf8Friendly=FALSE;
            ucm=mbcsData->ucm;
            MBCSDestruct(mbcsData);
            MBCSInit(mbcsData, ucm);
            continue;
        }
    } while(FALSE);

    MBCSPostprocess(mbcsData, staticData);

    return isOK;
}

static UBool
transformEUC(MBCSData *mbcsData) {
    uint8_t *p8;
    uint32_t i, value, oldLength, old3Top, new3Top;
    uint8_t b;

    oldLength=mbcsData->ucm->states.maxCharLength;
    if(oldLength<3) {
        return FALSE;
    }

    old3Top=mbcsData->stage3Top;

    /* careful: 2-byte and 4-byte codes are stored in platform endianness! */

    /* test if all first bytes are in {0, 0x8e, 0x8f} */
    p8=mbcsData->fromUBytes;

#if !U_IS_BIG_ENDIAN
    if(oldLength==4) {
        p8+=3;
    }
#endif

    for(i=0; i<old3Top; i+=oldLength) {
        b=p8[i];
        if(b!=0 && b!=0x8e && b!=0x8f) {
            /* some first byte does not fit the EUC pattern, nothing to be done */
            return FALSE;
        }
    }
    /* restore p if it was modified above */
    p8=mbcsData->fromUBytes;

    /* modify outputType and adjust stage3Top */
    mbcsData->ucm->states.outputType=(int8_t)(MBCS_OUTPUT_3_EUC+oldLength-3);
    mbcsData->stage3Top=new3Top=(old3Top*(oldLength-1))/oldLength;

    /*
     * EUC-encode all byte sequences;
     * see "CJKV Information Processing" (1st ed. 1999) from Ken Lunde, O'Reilly,
     * p. 161 in chapter 4 "Encoding Methods"
     *
     * This also must reverse the byte order if the platform is little-endian!
     */
    if(oldLength==3) {
        uint16_t *q=(uint16_t *)p8;
        for(i=0; i<old3Top; i+=oldLength) {
            b=*p8;
            if(b==0) {
                /* short sequences are stored directly */
                /* code set 0 or 1 */
                (*q++)=(uint16_t)((p8[1]<<8)|p8[2]);
            } else if(b==0x8e) {
                /* code set 2 */
                (*q++)=(uint16_t)(((p8[1]&0x7f)<<8)|p8[2]);
            } else /* b==0x8f */ {
                /* code set 3 */
                (*q++)=(uint16_t)((p8[1]<<8)|(p8[2]&0x7f));
            }
            p8+=3;
        }
    } else /* oldLength==4 */ {
        uint8_t *q=p8;
        uint32_t *p32=(uint32_t *)p8;
        for(i=0; i<old3Top; i+=4) {
            value=(*p32++);
            if(value<=0xffffff) {
                /* short sequences are stored directly */
                /* code set 0 or 1 */
                (*q++)=(uint8_t)(value>>16);
                (*q++)=(uint8_t)(value>>8);
                (*q++)=(uint8_t)value;
            } else if(value<=0x8effffff) {
                /* code set 2 */
                (*q++)=(uint8_t)((value>>16)&0x7f);
                (*q++)=(uint8_t)(value>>8);
                (*q++)=(uint8_t)value;
            } else /* first byte is 0x8f */ {
                /* code set 3 */
                (*q++)=(uint8_t)(value>>16);
                (*q++)=(uint8_t)((value>>8)&0x7f);
                (*q++)=(uint8_t)value;
            }
        }
    }

    return TRUE;
}

/*
 * Compact stage 2 for SBCS by overlapping adjacent stage 2 blocks as far
 * as possible. Overlapping is done on unassigned head and tail
 * parts of blocks in steps of MBCS_STAGE_2_MULTIPLIER.
 * Stage 1 indexes need to be adjusted accordingly.
 * This function is very similar to genprops/store.c/compactStage().
 */
static void
singleCompactStage2(MBCSData *mbcsData) {
    /* this array maps the ordinal number of a stage 2 block to its new stage 1 index */
    uint16_t map[MBCS_STAGE_2_MAX_BLOCKS];
    uint16_t i, start, prevEnd, newStart;

    /* enter the all-unassigned first stage 2 block into the map */
    map[0]=MBCS_STAGE_2_ALL_UNASSIGNED_INDEX;

    /* begin with the first block after the all-unassigned one */
    start=newStart=MBCS_STAGE_2_FIRST_ASSIGNED;
    while(start<mbcsData->stage2Top) {
        prevEnd=(uint16_t)(newStart-1);

        /* find the size of the overlap */
        for(i=0; i<MBCS_STAGE_2_BLOCK_SIZE && mbcsData->stage2Single[start+i]==0 && mbcsData->stage2Single[prevEnd-i]==0; ++i) {}

        if(i>0) {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=(uint16_t)(newStart-i);

            /* move the non-overlapping indexes to their new positions */
            start+=i;
            for(i=(uint16_t)(MBCS_STAGE_2_BLOCK_SIZE-i); i>0; --i) {
                mbcsData->stage2Single[newStart++]=mbcsData->stage2Single[start++];
            }
        } else if(newStart<start) {
            /* move the indexes to their new positions */
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=newStart;
            for(i=MBCS_STAGE_2_BLOCK_SIZE; i>0; --i) {
                mbcsData->stage2Single[newStart++]=mbcsData->stage2Single[start++];
            }
        } else /* no overlap && newStart==start */ {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=start;
            start=newStart+=MBCS_STAGE_2_BLOCK_SIZE;
        }
    }

    /* adjust stage2Top */
    if(VERBOSE && newStart<mbcsData->stage2Top) {
        printf("compacting stage 2 from stage2Top=0x%lx to 0x%lx, saving %ld bytes\n",
                (unsigned long)mbcsData->stage2Top, (unsigned long)newStart,
                (long)(mbcsData->stage2Top-newStart)*2);
    }
    mbcsData->stage2Top=newStart;

    /* now adjust stage 1 */
    for(i=0; i<MBCS_STAGE_1_SIZE; ++i) {
        mbcsData->stage1[i]=map[mbcsData->stage1[i]>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT];
    }
}

/* Compact stage 3 for SBCS - same algorithm as above. */
static void
singleCompactStage3(MBCSData *mbcsData) {
    uint16_t *stage3=(uint16_t *)mbcsData->fromUBytes;

    /* this array maps the ordinal number of a stage 3 block to its new stage 2 index */
    uint16_t map[0x1000];
    uint16_t i, start, prevEnd, newStart;

    /* enter the all-unassigned first stage 3 block into the map */
    map[0]=0;

    /* begin with the first block after the all-unassigned one */
    start=newStart=16;
    while(start<mbcsData->stage3Top) {
        prevEnd=(uint16_t)(newStart-1);

        /* find the size of the overlap */
        for(i=0; i<16 && stage3[start+i]==0 && stage3[prevEnd-i]==0; ++i) {}

        if(i>0) {
            map[start>>4]=(uint16_t)(newStart-i);

            /* move the non-overlapping indexes to their new positions */
            start+=i;
            for(i=(uint16_t)(16-i); i>0; --i) {
                stage3[newStart++]=stage3[start++];
            }
        } else if(newStart<start) {
            /* move the indexes to their new positions */
            map[start>>4]=newStart;
            for(i=16; i>0; --i) {
                stage3[newStart++]=stage3[start++];
            }
        } else /* no overlap && newStart==start */ {
            map[start>>4]=start;
            start=newStart+=16;
        }
    }

    /* adjust stage3Top */
    if(VERBOSE && newStart<mbcsData->stage3Top) {
        printf("compacting stage 3 from stage3Top=0x%lx to 0x%lx, saving %ld bytes\n",
                (unsigned long)mbcsData->stage3Top, (unsigned long)newStart,
                (long)(mbcsData->stage3Top-newStart)*2);
    }
    mbcsData->stage3Top=newStart;

    /* now adjust stage 2 */
    for(i=0; i<mbcsData->stage2Top; ++i) {
        mbcsData->stage2Single[i]=map[mbcsData->stage2Single[i]>>4];
    }
}

/*
 * Compact stage 2 by overlapping adjacent stage 2 blocks as far
 * as possible. Overlapping is done on unassigned head and tail
 * parts of blocks in steps of MBCS_STAGE_2_MULTIPLIER.
 * Stage 1 indexes need to be adjusted accordingly.
 * This function is very similar to genprops/store.c/compactStage().
 */
static void
compactStage2(MBCSData *mbcsData) {
    /* this array maps the ordinal number of a stage 2 block to its new stage 1 index */
    uint16_t map[MBCS_STAGE_2_MAX_BLOCKS];
    uint16_t i, start, prevEnd, newStart;

    /* enter the all-unassigned first stage 2 block into the map */
    map[0]=MBCS_STAGE_2_ALL_UNASSIGNED_INDEX;

    /* begin with the first block after the all-unassigned one */
    start=newStart=MBCS_STAGE_2_FIRST_ASSIGNED;
    while(start<mbcsData->stage2Top) {
        prevEnd=(uint16_t)(newStart-1);

        /* find the size of the overlap */
        for(i=0; i<MBCS_STAGE_2_BLOCK_SIZE && mbcsData->stage2[start+i]==0 && mbcsData->stage2[prevEnd-i]==0; ++i) {}

        if(i>0) {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=(uint16_t)(newStart-i);

            /* move the non-overlapping indexes to their new positions */
            start+=i;
            for(i=(uint16_t)(MBCS_STAGE_2_BLOCK_SIZE-i); i>0; --i) {
                mbcsData->stage2[newStart++]=mbcsData->stage2[start++];
            }
        } else if(newStart<start) {
            /* move the indexes to their new positions */
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=newStart;
            for(i=MBCS_STAGE_2_BLOCK_SIZE; i>0; --i) {
                mbcsData->stage2[newStart++]=mbcsData->stage2[start++];
            }
        } else /* no overlap && newStart==start */ {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=start;
            start=newStart+=MBCS_STAGE_2_BLOCK_SIZE;
        }
    }

    /* adjust stage2Top */
    if(VERBOSE && newStart<mbcsData->stage2Top) {
        printf("compacting stage 2 from stage2Top=0x%lx to 0x%lx, saving %ld bytes\n",
                (unsigned long)mbcsData->stage2Top, (unsigned long)newStart,
                (long)(mbcsData->stage2Top-newStart)*4);
    }
    mbcsData->stage2Top=newStart;

    /* now adjust stage 1 */
    for(i=0; i<MBCS_STAGE_1_SIZE; ++i) {
        mbcsData->stage1[i]=map[mbcsData->stage1[i]>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT];
    }
}

static void
MBCSPostprocess(MBCSData *mbcsData, const UConverterStaticData *staticData) {
    UCMStates *states;
    int32_t maxCharLength, stage3Width;

    states=&mbcsData->ucm->states;
    stage3Width=maxCharLength=states->maxCharLength;
if(maxCharLength>1) { printf("maxCharLength=%d number of fallbacks: %d\n", maxCharLength, countFallbacks); }

    ucm_optimizeStates(states,
                       &mbcsData->unicodeCodeUnits,
                       mbcsData->toUFallbacks, mbcsData->countToUFallbacks,
                       VERBOSE);

    /* try to compact the fromUnicode tables */
    if(transformEUC(mbcsData)) {
        --stage3Width;
    }

    /*
     * UTF-8-friendly tries are built precompacted, to cope with variable
     * stage 3 allocation block sizes.
     *
     * Tables without precision indicators cannot be built that way,
     * because if a block was overlapped with a previous one, then a smaller
     * code point for the same block would not fit.
     * Therefore, such tables are not marked UTF-8-friendly and must be
     * compacted after all mappings are entered.
     */
    if(!mbcsData->utf8Friendly) {
        if(maxCharLength==1) {
            singleCompactStage3(mbcsData);
            singleCompactStage2(mbcsData);
        } else {
            compactStage2(mbcsData);
        }
    }

    /* TODO: remove printf */
#if 0
    if(maxCharLength>1 && mbcsData->utf8Friendly) {
        unsigned long stageUTF8Length=MBCS_UTF8_STAGE_1_SIZE+mbcsData->stage2UTF8Top;
        printf("fromUnicode number of uint16_t in the UTF-8 stage 1/2 array: 0x%lx=%lu\n", stageUTF8Length, stageUTF8Length);
    }
#endif

    if(VERBOSE) {
        /*uint32_t c, i1, i2, i2Limit, i3;*/

        printf("fromUnicode number of uint%s_t in stage 2: 0x%lx=%lu\n",
               maxCharLength==1 ? "16" : "32",
               (unsigned long)mbcsData->stage2Top,
               (unsigned long)mbcsData->stage2Top);
#if 0
        if(maxCharLength>1 && mbcsData->utf8Friendly) {
            printf("fromUnicode number of uint16_t in the UTF-8 stage 1/2 array: 0x%lx=%lu\n",
                   (unsigned long)(MBCS_UTF8_STAGE_1_SIZE+mbcsData->stage2UTF8Top),
                   (unsigned long)(MBCS_UTF8_STAGE_1_SIZE+mbcsData->stage2UTF8Top));
        }
#endif
        printf("fromUnicode number of %d-byte stage 3 mapping entries: 0x%lx=%lu\n",
               (int)stage3Width,
               (unsigned long)mbcsData->stage3Top/stage3Width,
               (unsigned long)mbcsData->stage3Top/stage3Width);
#if 0
        c=0;
        for(i1=0; i1<MBCS_STAGE_1_SIZE; ++i1) {
            i2=mbcsData->stage1[i1];
            if(i2==0) {
                c+=MBCS_STAGE_2_BLOCK_SIZE*MBCS_STAGE_3_BLOCK_SIZE;
                continue;
            }
            for(i2Limit=i2+MBCS_STAGE_2_BLOCK_SIZE; i2<i2Limit; ++i2) {
                if(maxCharLength==1) {
                    i3=mbcsData->stage2Single[i2];
                } else {
                    i3=(uint16_t)mbcsData->stage2[i2];
                }
                if(i3==0) {
                    c+=MBCS_STAGE_3_BLOCK_SIZE;
                    continue;
                }
                printf("U+%04lx i1=0x%02lx i2=0x%04lx i3=0x%04lx\n",
                       (unsigned long)c,
                       (unsigned long)i1,
                       (unsigned long)i2,
                       (unsigned long)i3);
                c+=MBCS_STAGE_3_BLOCK_SIZE;
            }
        }
#endif
    }

    /* TODO: print info if not utf8Friendly */
}

static uint32_t
MBCSWrite(NewConverter *cnvData, const UConverterStaticData *staticData,
          UNewDataMemory *pData, int32_t tableType) {
    MBCSData *mbcsData=(MBCSData *)cnvData;
    uint32_t top, stageUTF8Length=0;
    int32_t i, stage1Top;

    _MBCSHeader header={ { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 };

    /* adjust stage 1 entries to include the size of stage 1 in the offsets to stage 2 */
    if(mbcsData->ucm->states.maxCharLength==1) {
        if(staticData->unicodeMask&UCNV_HAS_SUPPLEMENTARY) {
            stage1Top=MBCS_STAGE_1_SIZE; /* 0x440==1088 */
        } else {
            stage1Top=0x40; /* 0x40==64 */
        }
        for(i=0; i<stage1Top; ++i) {
            mbcsData->stage1[i]+=(uint16_t)stage1Top;
        }

        /* stage2Top has counted 16-bit results, now we need to count bytes */
        mbcsData->stage2Top*=2;

        /* stage3Top has counted 16-bit results, now we need to count bytes */
        mbcsData->stage3Top*=2;

        if(mbcsData->utf8Friendly) {
            header.version[2]=(uint8_t)(SBCS_UTF8_MAX>>8); /* store 0x1f for max==0x1fff */
        }
    } else {
        if(staticData->unicodeMask&UCNV_HAS_SUPPLEMENTARY) {
            stage1Top=MBCS_STAGE_1_SIZE; /* 0x440==1088 */
        } else {
            stage1Top=0x40; /* 0x40==64 */
        }
        for(i=0; i<stage1Top; ++i) {
            mbcsData->stage1[i]+=(uint16_t)stage1Top/2; /* stage 2 contains 32-bit entries, stage 1 16-bit entries */
        }
#if 0
        for(i=0; i<MBCS_UTF8_STAGE_1_SIZE; ++i) {
            mbcsData->stage1UTF8[i]+=(uint16_t)MBCS_UTF8_STAGE_1_SIZE;
        }
#endif

/* TODO: remove printf */
printf("bytes in MBCS roundtrip flags: %d\n", mbcsData->stage2Top*2);
        /* stage2Top has counted 32-bit results, now we need to count bytes */
        mbcsData->stage2Top*=4;

        if(mbcsData->utf8Friendly) {
            stageUTF8Length=MBCS_UTF8_STAGE_SIZE;
            header.version[2]=(uint8_t)(MBCS_UTF8_MAX>>8); /* store 0xd7 for max==0xd7ff */
        }
#if 0
        if(mbcsData->utf8Friendly) {
            /* round up stage2UTF8Top to a multiple of 16 because stageUTF8Length>>4 will be stored */
            mbcsData->stage2UTF8Top=(mbcsData->stage2UTF8Top+15)&~15;
            stageUTF8Length=MBCS_UTF8_STAGE_1_SIZE+mbcsData->stage2UTF8Top;
            header.version[2]=(uint8_t)(stageUTF8Length>>4);

            /* stage2UTF8Top has counted 16-bit results, now we need to count bytes */
            mbcsData->stage2UTF8Top*=2;
        }
#endif

        /* stage3Top has already counted bytes */
    }

    /* round up stage2Top and stage3Top so that the sizes of all data blocks are multiples of 4 */
    mbcsData->stage2Top=(mbcsData->stage2Top+3)&~3;
    mbcsData->stage3Top=(mbcsData->stage3Top+3)&~3;

    /* fill the header */
    header.version[0]=4;
    header.version[1]=3;
    /* header.version[2] set above for utf8Friendly data */

    /*
     *        TODO: further documentation
     *        TODO: doc in ucnvmbcs.h
     */

    header.countStates=mbcsData->ucm->states.countStates;
    header.countToUFallbacks=mbcsData->countToUFallbacks;

    header.offsetToUCodeUnits=
        sizeof(_MBCSHeader)+
        mbcsData->ucm->states.countStates*1024+
        mbcsData->countToUFallbacks*sizeof(_MBCSToUFallback);
    header.offsetFromUTable=
        header.offsetToUCodeUnits+
        mbcsData->ucm->states.countToUCodeUnits*2;
    header.offsetFromUBytes=
        header.offsetFromUTable+
        stage1Top*2+
        mbcsData->stage2Top;
    header.fromUBytesLength=mbcsData->stage3Top;

    top=header.offsetFromUBytes+header.fromUBytesLength+stageUTF8Length*2;

    header.flags=(uint8_t)(mbcsData->ucm->states.outputType);

    if(tableType&TABLE_EXT) {
        if(top>0xffffff) {
            fprintf(stderr, "error: offset 0x%lx to extension table exceeds 0xffffff\n", (long)top);
            return 0;
        }

        header.flags|=top<<8;
    }

    /* write the MBCS data */
    udata_writeBlock(pData, &header, sizeof(_MBCSHeader));
    udata_writeBlock(pData, mbcsData->ucm->states.stateTable, header.countStates*1024);
    udata_writeBlock(pData, mbcsData->toUFallbacks, mbcsData->countToUFallbacks*sizeof(_MBCSToUFallback));
    udata_writeBlock(pData, mbcsData->unicodeCodeUnits, mbcsData->ucm->states.countToUCodeUnits*2);
    udata_writeBlock(pData, mbcsData->stage1, stage1Top*2);
    if(mbcsData->ucm->states.maxCharLength==1) {
        udata_writeBlock(pData, mbcsData->stage2Single, mbcsData->stage2Top);
    } else {
        udata_writeBlock(pData, mbcsData->stage2, mbcsData->stage2Top);
    }
    udata_writeBlock(pData, mbcsData->fromUBytes, mbcsData->stage3Top);

    if(stageUTF8Length>0) {
        udata_writeBlock(pData, mbcsData->stageUTF8, stageUTF8Length*2);
#if 0
        udata_writeBlock(pData, mbcsData->stage1UTF8, MBCS_UTF8_STAGE_1_SIZE*2);
        udata_writeBlock(pData, mbcsData->stage2UTF8, mbcsData->stage2UTF8Top);
#endif
    }

    /* return the number of bytes that should have been written */
    return top;
}
