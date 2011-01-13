/*
******************************************************************************
* Copyright (c) 1996-2010, International Business Machines
* Corporation and others. All Rights Reserved.
******************************************************************************
* File unorm.cpp
*
* Created by: Vladimir Weinstein 12052000
*
* Modification history :
*
* Date        Name        Description
* 02/01/01    synwee      Added normalization quickcheck enum and method.
* 02/12/01    synwee      Commented out quickcheck util api has been approved
*                         Added private method for doing FCD checks
* 02/23/01    synwee      Modified quickcheck and checkFCE to run through 
*                         string for codepoints < 0x300 for the normalization 
*                         mode NFC.
* 05/25/01+   Markus Scherer total rewrite, implement all normalization here
*                         instead of just wrappers around normlzr.cpp,
*                         load unorm.dat, support Unicode 3.1 with
*                         supplementary code points, etc.
* 2009-nov..2010-jan  Markus Scherer  total rewrite, new Normalizer2 API & code
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/udata.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/uiter.h"
#include "unicode/unorm.h"
#include "normalizer2impl.h"
#include "ucln_cmn.h"
#include "unormimp.h"
#include "uprops.h"
#include "cmemory.h"
#include "umutex.h"
#include "utrie2.h"
#include "unicode/uset.h"
#include "putilimp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

U_NAMESPACE_USE

/*
 * This new implementation of the normalization code loads its data from
 * unorm.dat, which is generated with the gennorm tool.
 * The format of that file is described in unormimp.h .
 */

/* load unorm.dat ----------------------------------------------------------- */

#define UNORM_HARDCODE_DATA 1

#if UNORM_HARDCODE_DATA

/* unorm_props_data.c is machine-generated by gennorm --csource */
#include "unorm_props_data.c"

static const UBool formatVersion_2_2=TRUE;

#else

#define DATA_NAME "unorm"
#define DATA_TYPE "icu"

static UDataMemory *normData=NULL;
static UErrorCode dataErrorCode=U_ZERO_ERROR;
static int8_t haveNormData=0;

static int32_t indexes[_NORM_INDEX_TOP]={ 0 };
static UTrie normTrie={ 0,0,0,0,0,0,0 }, fcdTrie={ 0,0,0,0,0,0,0 }, auxTrie={ 0,0,0,0,0,0,0 };

/*
 * pointers into the memory-mapped unorm.icu
 */
static const uint16_t *extraData=NULL,
                      *combiningTable=NULL,
                      *canonStartSets=NULL;

static uint8_t formatVersion[4]={ 0, 0, 0, 0 };
static UBool formatVersion_2_1=FALSE, formatVersion_2_2=FALSE;

/* the Unicode version of the normalization data */
static UVersionInfo dataVersion={ 0, 0, 0, 0 };

#endif

U_CDECL_BEGIN

static UBool U_CALLCONV
unorm_cleanup(void) {
#if !UNORM_HARDCODE_DATA
    if(normData!=NULL) {
        udata_close(normData);
        normData=NULL;
    }
    dataErrorCode=U_ZERO_ERROR;
    haveNormData=0;
#endif

    return TRUE;
}

#if !UNORM_HARDCODE_DATA

static UBool U_CALLCONV
isAcceptable(void * /* context */,
             const char * /* type */, const char * /* name */,
             const UDataInfo *pInfo) {
    if(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x4e &&   /* dataFormat="Norm" */
        pInfo->dataFormat[1]==0x6f &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6d &&
        pInfo->formatVersion[0]==2 &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    ) {
        uprv_memcpy(formatVersion, pInfo->formatVersion, 4);
        uprv_memcpy(dataVersion, pInfo->dataVersion, 4);
        return TRUE;
    } else {
        return FALSE;
    }
}

#endif

static UBool U_CALLCONV
_enumPropertyStartsRange(const void *context, UChar32 start, UChar32 /*end*/, uint32_t /*value*/) {
    /* add the start code point to the USet */
    const USetAdder *sa=(const USetAdder *)context;
    sa->add(sa->set, start);
    return TRUE;
}

U_CDECL_END

#if !UNORM_HARDCODE_DATA

static int8_t
loadNormData(UErrorCode &errorCode) {
    /* load Unicode normalization data from file */

    /*
     * This lazy intialization with double-checked locking (without mutex protection for
     * haveNormData==0) is transiently unsafe under certain circumstances.
     * Check the readme and use u_init() if necessary.
     *
     * While u_init() initializes the main normalization data via this functions,
     * it does not do so for exclusion sets (which are fully mutexed).
     * This is because
     * - there can be many exclusion sets
     * - they are rarely used
     * - they are not usually used in execution paths that are
     *   as performance-sensitive as others
     *   (e.g., IDNA takes more time than unorm_quickCheck() anyway)
     *
     *  TODO:  Remove code in support for non-hardcoded data.  u_init() is now advertised
     *         as not being required for thread safety, and we can't reasonably
     *         revert to requiring it.
     */
    if(haveNormData==0) {
        UTrie _normTrie={ 0,0,0,0,0,0,0 }, _fcdTrie={ 0,0,0,0,0,0,0 }, _auxTrie={ 0,0,0,0,0,0,0 };
        UDataMemory *data;

        const int32_t *p=NULL;
        const uint8_t *pb;

        if(&errorCode==NULL || U_FAILURE(errorCode)) {
            return 0;
        }

        /* open the data outside the mutex block */
        data=udata_openChoice(NULL, DATA_TYPE, DATA_NAME, isAcceptable, NULL, &errorCode);
        dataErrorCode=errorCode;
        if(U_FAILURE(errorCode)) {
            return haveNormData=-1;
        }

        p=(const int32_t *)udata_getMemory(data);
        pb=(const uint8_t *)(p+_NORM_INDEX_TOP);
        utrie_unserialize(&_normTrie, pb, p[_NORM_INDEX_TRIE_SIZE], &errorCode);
        _normTrie.getFoldingOffset=getFoldingNormOffset;

        pb+=p[_NORM_INDEX_TRIE_SIZE]+p[_NORM_INDEX_UCHAR_COUNT]*2+p[_NORM_INDEX_COMBINE_DATA_COUNT]*2;
        if(p[_NORM_INDEX_FCD_TRIE_SIZE]!=0) {
            utrie_unserialize(&_fcdTrie, pb, p[_NORM_INDEX_FCD_TRIE_SIZE], &errorCode);
        }
        pb+=p[_NORM_INDEX_FCD_TRIE_SIZE];

        if(p[_NORM_INDEX_AUX_TRIE_SIZE]!=0) {
            utrie_unserialize(&_auxTrie, pb, p[_NORM_INDEX_AUX_TRIE_SIZE], &errorCode);
            _auxTrie.getFoldingOffset=getFoldingAuxOffset;
        }

        if(U_FAILURE(errorCode)) {
            dataErrorCode=errorCode;
            udata_close(data);
            return haveNormData=-1;
        }

        /* in the mutex block, set the data for this process */
        umtx_lock(NULL);
        if(normData==NULL) {
            normData=data;
            data=NULL;

            uprv_memcpy(&indexes, p, sizeof(indexes));
            uprv_memcpy(&normTrie, &_normTrie, sizeof(UTrie));
            uprv_memcpy(&fcdTrie, &_fcdTrie, sizeof(UTrie));
            uprv_memcpy(&auxTrie, &_auxTrie, sizeof(UTrie));
        } else {
            p=(const int32_t *)udata_getMemory(normData);
        }

        /* initialize some variables */
        extraData=(uint16_t *)((uint8_t *)(p+_NORM_INDEX_TOP)+indexes[_NORM_INDEX_TRIE_SIZE]);
        combiningTable=extraData+indexes[_NORM_INDEX_UCHAR_COUNT];
        formatVersion_2_1=formatVersion[0]>2 || (formatVersion[0]==2 && formatVersion[1]>=1);
        formatVersion_2_2=formatVersion[0]>2 || (formatVersion[0]==2 && formatVersion[1]>=2);
        if(formatVersion_2_1) {
            canonStartSets=combiningTable+
                indexes[_NORM_INDEX_COMBINE_DATA_COUNT]+
                (indexes[_NORM_INDEX_FCD_TRIE_SIZE]+indexes[_NORM_INDEX_AUX_TRIE_SIZE])/2;
        }
        haveNormData=1;
        ucln_common_registerCleanup(UCLN_COMMON_UNORM, unorm_cleanup);
        umtx_unlock(NULL);

        /* if a different thread set it first, then close the extra data */
        if(data!=NULL) {
            udata_close(data); /* NULL if it was set correctly */
        }
    }

    return haveNormData;
}

#endif

static inline UBool
_haveData(UErrorCode &errorCode) {
#if UNORM_HARDCODE_DATA
    return U_SUCCESS(errorCode);
#else
    if(U_FAILURE(errorCode)) {
        return FALSE;
    } else if(haveNormData>0) {
        return TRUE;
    } else if(haveNormData<0) {
        errorCode=dataErrorCode;
        return FALSE;
    } else /* haveNormData==0 */ {
        return (UBool)(loadNormData(errorCode)>0);
    }
#endif
}

U_CAPI UBool U_EXPORT2
unorm_haveData(UErrorCode *pErrorCode) {
    return _haveData(*pErrorCode);
}

/* normalization properties ------------------------------------------------- */

U_CFUNC UBool U_EXPORT2
unorm_isCanonSafeStart(UChar32 c) {
#if UNORM_HARDCODE_DATA
    if(auxTrie.index!=NULL) {
#else
    UErrorCode errorCode=U_ZERO_ERROR;
    if(_haveData(errorCode) && auxTrie.index!=NULL) {
#endif
        uint16_t aux=UTRIE2_GET16(&auxTrie, c);
        return (UBool)((aux&_NORM_AUX_UNSAFE_MASK)==0);
    } else {
        return FALSE;
    }
}

U_CAPI UBool U_EXPORT2
unorm_getCanonStartSet(UChar32 c, USerializedSet *fillSet) {
#if !UNORM_HARDCODE_DATA
    UErrorCode errorCode=U_ZERO_ERROR;
#endif
    if( fillSet!=NULL && (uint32_t)c<=0x10ffff &&
#if !UNORM_HARDCODE_DATA
        _haveData(errorCode) &&
#endif
        canonStartSets!=NULL
    ) {
        const uint16_t *table;
        int32_t i, start, limit;

        /*
         * binary search for c
         *
         * There are two search tables,
         * one for BMP code points and one for supplementary ones.
         * See unormimp.h for details.
         */
        if(c<=0xffff) {
            table=canonStartSets+canonStartSets[_NORM_SET_INDEX_CANON_SETS_LENGTH];
            start=0;
            limit=canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH];

            /* each entry is a pair { c, result } */
            while(start<limit-2) {
                i=(uint16_t)(((start+limit)/4)*2); /* (start+limit)/2 and address pairs */
                if(c<table[i]) {
                    limit=i;
                } else {
                    start=i;
                }
            }

            /* found? */
            if(c==table[start]) {
                i=table[start+1];
                if((i&_NORM_CANON_SET_BMP_MASK)==_NORM_CANON_SET_BMP_IS_INDEX) {
                    /* result 01xxxxxx xxxxxx contains index x to a USerializedSet */
                    i&=(_NORM_MAX_CANON_SETS-1);
                    return uset_getSerializedSet(fillSet,
                                            canonStartSets+i,
                                            canonStartSets[_NORM_SET_INDEX_CANON_SETS_LENGTH]-i);
                } else {
                    /* other result values are BMP code points for single-code point sets */
                    uset_setSerializedToOne(fillSet, (UChar32)i);
                    return TRUE;
                }
            }
        } else {
            uint16_t high, low, h;

            table=canonStartSets+canonStartSets[_NORM_SET_INDEX_CANON_SETS_LENGTH]+
                                 canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH];
            start=0;
            limit=canonStartSets[_NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH];

            high=(uint16_t)(c>>16);
            low=(uint16_t)c;

            /* each entry is a triplet { high(c), low(c), result } */
            while(start<limit-3) {
                i=(uint16_t)(((start+limit)/6)*3); /* (start+limit)/2 and address triplets */
                h=table[i]&0x1f; /* high word */
                if(high<h || (high==h && low<table[i+1])) {
                    limit=i;
                } else {
                    start=i;
                }
            }

            /* found? */
            h=table[start];
            if(high==(h&0x1f) && low==table[start+1]) {
                i=table[start+2];
                if((h&0x8000)==0) {
                    /* the result is an index to a USerializedSet */
                    return uset_getSerializedSet(fillSet,
                                            canonStartSets+i,
                                            canonStartSets[_NORM_SET_INDEX_CANON_SETS_LENGTH]-i);
                } else {
                    /*
                     * single-code point set {x} in
                     * triplet { 100xxxxx 000hhhhh  llllllll llllllll  xxxxxxxx xxxxxxxx }
                     */
                    i|=((int32_t)h&0x1f00)<<8; /* add high bits from high(c) */
                    uset_setSerializedToOne(fillSet, (UChar32)i);
                    return TRUE;
                }
            }
        }
    }

    return FALSE; /* not found */
}

U_CAPI void U_EXPORT2
unorm_addPropertyStarts(const USetAdder *sa, UErrorCode *pErrorCode) {
    UChar c;

    if(!_haveData(*pErrorCode)) {
        return;
    }

    /* add the start code point of each same-value range of each trie */
    if(auxTrie.index!=NULL) {
        utrie2_enum(&auxTrie, NULL, _enumPropertyStartsRange, sa);
    }

    /* add Hangul LV syllables and LV+1 because of skippables */
    for(c=Hangul::HANGUL_BASE; c<Hangul::HANGUL_LIMIT; c+=Hangul::JAMO_T_COUNT) {
        sa->add(sa->set, c);
        sa->add(sa->set, c+1);
    }
    sa->add(sa->set, Hangul::HANGUL_LIMIT); /* add Hangul+1 to continue with other properties */
}

/* quick check functions ---------------------------------------------------- */

U_CAPI UNormalizationCheckResult U_EXPORT2
unorm_quickCheck(const UChar *src,
                 int32_t srcLength, 
                 UNormalizationMode mode,
                 UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    return unorm2_quickCheck((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
}

U_CAPI UNormalizationCheckResult U_EXPORT2
unorm_quickCheckWithOptions(const UChar *src, int32_t srcLength, 
                            UNormalizationMode mode, int32_t options,
                            UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        FilteredNormalizer2 fn2(*n2, *uniset_getUnicode32Instance(*pErrorCode));
        return unorm2_quickCheck((const UNormalizer2 *)&fn2, src, srcLength, pErrorCode);
    } else {
        return unorm2_quickCheck((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
    }
}

U_CAPI UBool U_EXPORT2
unorm_isNormalized(const UChar *src, int32_t srcLength,
                   UNormalizationMode mode,
                   UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    return unorm2_isNormalized((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
}

U_CAPI UBool U_EXPORT2
unorm_isNormalizedWithOptions(const UChar *src, int32_t srcLength,
                              UNormalizationMode mode, int32_t options,
                              UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        FilteredNormalizer2 fn2(*n2, *uniset_getUnicode32Instance(*pErrorCode));
        return unorm2_isNormalized((const UNormalizer2 *)&fn2, src, srcLength, pErrorCode);
    } else {
        return unorm2_isNormalized((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
    }
}

/* normalize() API ---------------------------------------------------------- */

/** Public API for normalizing. */
U_CAPI int32_t U_EXPORT2
unorm_normalize(const UChar *src, int32_t srcLength,
                UNormalizationMode mode, int32_t options,
                UChar *dest, int32_t destCapacity,
                UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        FilteredNormalizer2 fn2(*n2, *uniset_getUnicode32Instance(*pErrorCode));
        return unorm2_normalize((const UNormalizer2 *)&fn2,
            src, srcLength, dest, destCapacity, pErrorCode);
    } else {
        return unorm2_normalize((const UNormalizer2 *)n2,
            src, srcLength, dest, destCapacity, pErrorCode);
    }
}


/* iteration functions ------------------------------------------------------ */

static int32_t
unorm_iterate(UCharIterator *src, UBool forward,
              UChar *dest, int32_t destCapacity,
              UNormalizationMode mode, int32_t options,
              UBool doNormalize, UBool *pNeededToNormalize,
              UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    const UnicodeSet *uni32;
    if(options&UNORM_UNICODE_3_2) {
        uni32=uniset_getUnicode32Instance(*pErrorCode);
    } else {
        uni32=NULL;  // unused
    }
    FilteredNormalizer2 fn2(*n2, *uni32);
    if(options&UNORM_UNICODE_3_2) {
        n2=&fn2;
    }
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if( destCapacity<0 || (dest==NULL && destCapacity>0) ||
        src==NULL
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(pNeededToNormalize!=NULL) {
        *pNeededToNormalize=FALSE;
    }
    if(!(forward ? src->hasNext(src) : src->hasPrevious(src))) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    }

    UnicodeString buffer;
    UChar32 c;
    if(forward) {
        /* get one character and ignore its properties */
        buffer.append(uiter_next32(src));
        /* get all following characters until we see a boundary */
        while((c=uiter_next32(src))>=0) {
            if(n2->hasBoundaryBefore(c)) {
                /* back out the latest movement to stop at the boundary */
                src->move(src, -U16_LENGTH(c), UITER_CURRENT);
                break;
            } else {
                buffer.append(c);
            }
        }
    } else {
        while((c=uiter_previous32(src))>=0) {
            /* always write this character to the front of the buffer */
            buffer.insert(0, c);
            /* stop if this just-copied character is a boundary */
            if(n2->hasBoundaryBefore(c)) {
                break;
            }
        }
    }

    UnicodeString destString(dest, 0, destCapacity);
    if(buffer.length()>0 && doNormalize) {
        n2->normalize(buffer, destString, *pErrorCode).extract(dest, destCapacity, *pErrorCode);
        if(pNeededToNormalize!=NULL && U_SUCCESS(*pErrorCode)) {
            *pNeededToNormalize= destString!=buffer;
        }
        return destString.length();
    } else {
        /* just copy the source characters */
        return buffer.extract(dest, destCapacity, *pErrorCode);
    }
}

U_CAPI int32_t U_EXPORT2
unorm_previous(UCharIterator *src,
               UChar *dest, int32_t destCapacity,
               UNormalizationMode mode, int32_t options,
               UBool doNormalize, UBool *pNeededToNormalize,
               UErrorCode *pErrorCode) {
    return unorm_iterate(src, FALSE,
                         dest, destCapacity,
                         mode, options,
                         doNormalize, pNeededToNormalize,
                         pErrorCode);
}

U_CAPI int32_t U_EXPORT2
unorm_next(UCharIterator *src,
           UChar *dest, int32_t destCapacity,
           UNormalizationMode mode, int32_t options,
           UBool doNormalize, UBool *pNeededToNormalize,
           UErrorCode *pErrorCode) {
    return unorm_iterate(src, TRUE,
                         dest, destCapacity,
                         mode, options,
                         doNormalize, pNeededToNormalize,
                         pErrorCode);
}

/* Concatenation of normalized strings -------------------------------------- */

U_CAPI int32_t U_EXPORT2
unorm_concatenate(const UChar *left, int32_t leftLength,
                  const UChar *right, int32_t rightLength,
                  UChar *dest, int32_t destCapacity,
                  UNormalizationMode mode, int32_t options,
                  UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    const UnicodeSet *uni32;
    if(options&UNORM_UNICODE_3_2) {
        uni32=uniset_getUnicode32Instance(*pErrorCode);
    } else {
        uni32=NULL;  // unused
    }
    FilteredNormalizer2 fn2(*n2, *uni32);
    if(options&UNORM_UNICODE_3_2) {
        n2=&fn2;
    }
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if( destCapacity<0 || (dest==NULL && destCapacity>0) ||
        left==NULL || leftLength<-1 ||
        right==NULL || rightLength<-1
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* check for overlapping right and destination */
    if( dest!=NULL &&
        ((right>=dest && right<(dest+destCapacity)) ||
         (rightLength>0 && dest>=right && dest<(right+rightLength)))
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* allow left==dest */
    UnicodeString destString;
    if(left==dest) {
        destString.setTo(dest, leftLength, destCapacity);
    } else {
        destString.setTo(dest, 0, destCapacity);
        destString.append(left, leftLength);
    }
    return n2->append(destString, UnicodeString(rightLength<0, right, rightLength), *pErrorCode).
           extract(dest, destCapacity, *pErrorCode);
}

#endif /* #if !UCONFIG_NO_NORMALIZATION */
