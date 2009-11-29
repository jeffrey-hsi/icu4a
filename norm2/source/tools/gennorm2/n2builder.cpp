/*
*******************************************************************************
*
*   Copyright (C) 2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  n2builder.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009nov25
*   created by: Markus W. Scherer
*
* Builds Normalizer2Data and writes a binary .nrm file.
* For the file format see source/common/normalizer2impl.h.
*/

#include "n2builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "unicode/utypes.h"
#include "unicode/errorcode.h"
#include "unicode/localpointer.h"
#include "unicode/putil.h"
#include "unicode/udata.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "normalizer2impl.h"
#include "toolutil.h"
#include "unewdata.h"
#include "unormimp.h"
#include "utrie2.h"
#include "writesrc.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/* file data ---------------------------------------------------------------- */

#if UCONFIG_NO_NORMALIZATION

/* dummy UDataInfo cf. udata.h */
static UDataInfo dataInfo = {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0, 0, 0, 0 },                 /* dummy dataFormat */
    { 0, 0, 0, 0 },                 /* dummy formatVersion */
    { 0, 0, 0, 0 }                  /* dummy dataVersion */
};

#else

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0x4e, 0x72, 0x6d, 0x32 }, /* dataFormat="Nrm2" */
    { 1, 0, 0, 0 },             /* formatVersion */
    { 5, 2, 0, 0 }              /* dataVersion (Unicode version) */
};

U_NAMESPACE_BEGIN

class HangulIterator {
public:
    struct Range {
        UChar32 start, limit;
        uint16_t norm16;
    };

    HangulIterator() : rangeIndex(0) {}
    const Range *nextRange() {
        if(rangeIndex<LENGTHOF(ranges)) {
            return ranges+rangeIndex++;
        } else {
            return NULL;
        }
    }
    void reset() { rangeIndex=0; }
private:
    static const Range ranges[4];
    int32_t rangeIndex;
};

const HangulIterator::Range HangulIterator::ranges[4]={
    { JAMO_L_BASE, JAMO_L_BASE+JAMO_L_COUNT, 1 },
    { JAMO_V_BASE, JAMO_V_BASE+JAMO_V_COUNT, Normalizer2Data::JAMO_VT },
    { JAMO_T_BASE+1, JAMO_T_BASE+JAMO_T_COUNT, Normalizer2Data::JAMO_VT },  // +1: not U+11A7
    { HANGUL_BASE, HANGUL_BASE+HANGUL_COUNT, 0 },  // will become minYesNo
};

struct CompositionPair {
    CompositionPair(UChar32 t, UChar32 c) : trail(t), composite(c) {}
    UChar32 trail, composite;
};

struct Norm {
    enum MappingType { NONE, REMOVED, ROUND_TRIP, ONE_WAY };

    // Requires mappingType>REMOVED and well-formed mapping.
    void setMappingCP() {
        UChar32 c;
        if(!mapping->isEmpty() && mapping->length()==U16_LENGTH(c=mapping->char32At(0))) {
            mappingCP=c;
        } else {
            mappingCP=U_SENTINEL;
        }
    }

    UnicodeString *mapping;
    UChar32 mappingCP;  // set if mapping to 1 code point
    int32_t mappingPhase;
    MappingType mappingType;

    std::vector<CompositionPair> *compositions;
    uint8_t cc;
    UBool combinesBack;

    enum OffsetType {
        OFFSET_NONE, OFFSET_MAYBEYES,
        OFFSET_YESYES, OFFSET_YESNO, OFFSET_NONO,
        OFFSET_DELTA
    };
    enum { OFFSET_SHIFT=4 };
    int32_t offset;
};

class Normalizer2DBEnumerator {
public:
    Normalizer2DBEnumerator(Normalizer2DataBuilder &b) : builder(b) {}
    virtual ~Normalizer2DBEnumerator() {}
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) = 0;
    Normalizer2DBEnumerator *ptr() { return this; }
protected:
    Normalizer2DataBuilder &builder;
};

U_CDECL_BEGIN

static UBool U_CALLCONV
enumRangeHandler(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    return ((Normalizer2DBEnumerator *)context)->rangeHandler(start, end, value);
}

U_CDECL_END

Normalizer2DataBuilder::Normalizer2DataBuilder(UErrorCode &errorCode) :
        phase(0), overrideHandling(OVERRIDE_PREVIOUS) {
    memset(unicodeVersion, 0, sizeof(unicodeVersion));
    normTrie=utrie2_open(0, 0, &errorCode);
    normMem=utm_open("gennorm2 normalization structs", 10000, 200000, sizeof(Norm));
    norms=allocNorm();  // unused Norm struct at index 0
}

Normalizer2DataBuilder::~Normalizer2DataBuilder() {
    utrie2_close(normTrie);
    int32_t normsLength=utm_countItems(normMem);
    for(int32_t i=1; i<normsLength; ++i) {
        delete norms[i].mapping;
        delete norms[i].compositions;
    }
    utm_close(normMem);
    utrie2_close(norm16Trie);
}

void
Normalizer2DataBuilder::setUnicodeVersion(const char *v) {
    u_versionFromString(unicodeVersion, v);
    // TODO: do when writing the file: uprv_memcpy(dataInfo.dataVersion, unicodeVersion, 4);
}

Norm *Normalizer2DataBuilder::allocNorm() {
    Norm *p=(Norm *)utm_alloc(normMem);
    norms=(Norm *)utm_getStart(normMem);  // in case it got reallocated
    return p;
}

/* get an existing Norm unit */
Norm *Normalizer2DataBuilder::getNorm(UChar32 c) {
    uint32_t i=utrie2_get32(normTrie, c);
    if(i==0) {
        return NULL;
    }
    return norms+i;
}

/*
 * get or create a Norm unit;
 * get or create the intermediate trie entries for it as well
 */
Norm *Normalizer2DataBuilder::createNorm(UChar32 c) {
    uint32_t i=utrie2_get32(normTrie, c);
    if(i!=0) {
        return norms+i;
    } else {
        /* allocate Norm */
        Norm *p=allocNorm();
        IcuToolErrorCode errorCode("gennorm2/createNorm()");
        utrie2_set32(normTrie, c, (uint32_t)(p-norms), errorCode);
        return p;
    }
}

uint8_t Normalizer2DataBuilder::getCC(UChar32 c) {
    return norms[utrie2_get32(normTrie, c)].cc;
}

Norm *Normalizer2DataBuilder::checkNormForMapping(Norm *p, UChar32 c) {
    if(p!=NULL) {
        if(p->mappingType!=Norm::NONE) {
            if( overrideHandling==OVERRIDE_NONE ||
                (overrideHandling==OVERRIDE_PREVIOUS && p->mappingPhase==phase)
            ) {
                fprintf(stderr,
                        "error in gennorm2 phase %d: "
                        "not permitted to override mapping for U+%04lX from phase %d\n",
                        (int)phase, (long)c, (int)p->mappingPhase);
                exit(U_INVALID_FORMAT_ERROR);
            }
            delete p->mapping;
            p->mapping=NULL;
        }
        p->mappingPhase=phase;
    }
    return p;
}

void Normalizer2DataBuilder::setOverrideHandling(OverrideHandling oh) {
    overrideHandling=oh;
    ++phase;
}

void Normalizer2DataBuilder::setCC(UChar32 c, uint8_t cc) {
    createNorm(c)->cc=cc;
}

static UBool isWellFormed(const UnicodeString &s) {
    UErrorCode errorCode=U_ZERO_ERROR;
    u_strToUTF8(NULL, 0, NULL, s.getBuffer(), s.length(), &errorCode);
    return U_SUCCESS(errorCode) || errorCode==U_BUFFER_OVERFLOW_ERROR;
}

void Normalizer2DataBuilder::setOneWayMapping(UChar32 c, const UnicodeString &m) {
    if(!isWellFormed(m)) {
        fprintf(stderr,
                "error in gennorm2 phase %d: "
                "illegal one-way mapping from U+%04lX to malformed string\n",
                (int)phase, (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    Norm *p=checkNormForMapping(createNorm(c), c);
    p->mapping=new UnicodeString(m);
    p->mappingType=Norm::ONE_WAY;
    p->setMappingCP();
}

void Normalizer2DataBuilder::setRoundTripMapping(UChar32 c, const UnicodeString &m) {
    if(!isWellFormed(m)) {
        fprintf(stderr,
                "error in gennorm2 phase %d: "
                "illegal round-trip mapping from U+%04lX to malformed string\n",
                (int)phase, (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    int32_t numCP=u_countChar32(m.getBuffer(), m.length());
    if(numCP!=2) {
        fprintf(stderr,
                "error in gennorm2 phase %d: "
                "illegal round-trip mapping from U+%04lX to %d!=2 code points\n",
                (int)phase, (long)c, (int)numCP);
        exit(U_INVALID_FORMAT_ERROR);
    }
    Norm *p=checkNormForMapping(createNorm(c), c);
    p->mapping=new UnicodeString(m);
    p->mappingType=Norm::ROUND_TRIP;
    p->mappingCP=U_SENTINEL;
}

void Normalizer2DataBuilder::removeMapping(UChar32 c) {
    Norm *p=checkNormForMapping(getNorm(c), c);
    if(p!=NULL) {
        p->mappingType=Norm::REMOVED;
    }
}

class CompositionBuilder : public Normalizer2DBEnumerator {
public:
    CompositionBuilder(Normalizer2DataBuilder &b) : Normalizer2DBEnumerator(b) {}
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        builder.addComposition(start, end, value);
        return TRUE;
    }
};

void
Normalizer2DataBuilder::addComposition(UChar32 start, UChar32 end, uint32_t value) {
    if(norms[value].mappingType==Norm::ROUND_TRIP) {
        if(start!=end) {
            fprintf(stderr,
                    "gennorm2 error: same round-trip mapping for "
                    "more than 1 code point U+%04lX..U+%04lX\n",
                    (long)start, (long)end);
            exit(U_INVALID_FORMAT_ERROR);
        }
        // setRoundTripMapping() ensured that there are exactly two code points.
        const UnicodeString &m=*norms[value].mapping;
        UChar32 lead=m.char32At(0);
        UChar32 trail=m.char32At(m.length()-1);
        // Flag for trailing character.
        createNorm(trail)->combinesBack=TRUE;
        // Insert (trail, composite) pair into compositions list for the lead character.
        CompositionPair pair(trail, start);
        Norm *leadNorm=createNorm(lead);
        std::vector<CompositionPair> *compositions=leadNorm->compositions;
        if(compositions==NULL) {
            compositions=leadNorm->compositions=new std::vector<CompositionPair>;
            compositions->push_back(pair);
        } else {
            // Insertion sort, and check for duplicate trail characters.
            std::vector<CompositionPair>::iterator it;
            for(it=compositions->begin(); it!=compositions->end(); ++it) {
                if(trail==it->trail) {
                    fprintf(stderr,
                            "gennorm2 error: same round-trip mapping for "
                            "more than 1 code point (e.g., U+%04lX) to U+%04lX + U+%04lX\n",
                            (long)start, (long)lead, (long)trail);
                    exit(U_INVALID_FORMAT_ERROR);
                }
                if(trail<it->trail) {
                    break;
                }
            }
            compositions->insert(it, pair);
        }
    }
}

class Decomposer : public Normalizer2DBEnumerator {
public:
    Decomposer(Normalizer2DataBuilder &b) : Normalizer2DBEnumerator(b), didDecompose(FALSE) {}
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        didDecompose|=builder.decompose(start, end, value);
        return TRUE;
    }
    UBool didDecompose;
};

static inline UBool isHangul(UChar32 c) {
    return HANGUL_BASE<=c && c<(HANGUL_BASE+HANGUL_COUNT);
}

static UBool
getHangulDecomposition(UChar32 c, UChar hangulBuffer[3]) {
    // Hangul syllable: decompose algorithmically
    c-=HANGUL_BASE;
    if((uint32_t)c>=HANGUL_COUNT) {
        return FALSE;  // not a Hangul syllable
    }

    UChar32 c2;
    c2=c%JAMO_T_COUNT;
    c/=JAMO_T_COUNT;
    hangulBuffer[0]=JAMO_L_BASE+c/JAMO_V_COUNT;
    hangulBuffer[1]=JAMO_V_BASE+c%JAMO_V_COUNT;
    if(c2==0) {
        hangulBuffer[2]=0;
    } else {
        hangulBuffer[2]=JAMO_T_BASE+c2;
    }
    return TRUE;
}

UBool
Normalizer2DataBuilder::decompose(UChar32 start, UChar32 end, uint32_t value) {
    if(norms[value].mappingType>Norm::REMOVED) {
        UChar hangulBuffer[3];
        const UnicodeString &m=*norms[value].mapping;
        UnicodeString *decomposed=NULL;
        const UChar *s=m.getBuffer();
        int32_t length=m.length();
        int32_t prev, i=0;
        UChar32 c;
        while(i<length) {
            prev=i;
            U16_NEXT(s, i, length, c);
            if(start<=c && c<=end) {
                fprintf(stderr,
                        "gennorm2 error: U+%04lX maps to itself directly or indirectly\n",
                        (long)c);
                exit(U_INVALID_FORMAT_ERROR);
            }
            Norm *p=getNorm(c);
            if(p!=NULL && p->mappingType>Norm::REMOVED) {
                if(decomposed==NULL) {
                    decomposed=new UnicodeString(m, 0, prev);
                }
                decomposed->append(*p->mapping);
                if(norms[value].mappingType==Norm::ROUND_TRIP && p->mappingType!=Norm::ROUND_TRIP) {
                    norms[value].mappingType=Norm::ONE_WAY;
                }
            } else if(getHangulDecomposition(c, hangulBuffer)) {
                if(decomposed==NULL) {
                    decomposed=new UnicodeString(m, 0, prev);
                }
                decomposed->append(hangulBuffer, hangulBuffer[2]==0 ? 2 : 3);
            } else if(decomposed!=NULL) {
                decomposed->append(m, prev, i-prev);
            }
        }
        if(decomposed!=NULL) {
            delete norms[value].mapping;
            norms[value].mapping=decomposed;
            // Not  norms[value].setMappingCP();  because the original mapping
            // is most likely to be encodable as a delta.
            return TRUE;
        }
    }
    return FALSE;
}

void
Normalizer2DataBuilder::reorder(Norm *p) {
    if(p->mappingType>Norm::REMOVED) {
        UnicodeString &m=*p->mapping;
        int32_t length=m.length();
        UChar *s=m.getBuffer(-1);
        int32_t prev, i=0;
        UChar32 c;
        uint8_t cc, lastCC=0;
        while(i<length) {
            prev=i;
            U16_NEXT(s, i, length, c);
            cc=getCC(c);
            if(lastCC<=cc || cc==0) {
                lastCC=cc;
            } else {
                // Let this code point bubble back to its canonical order.
                int32_t cpStart=prev, cpLimit;
                UChar32 c2;
                uint8_t cc2;
                U16_BACK_1(s, 0, cpStart);  // Skip the previous code point where 0<cc<lastCC.
                for(;;) {
                    cpLimit=cpStart;
                    if(cpStart==0) {
                        break;
                    }
                    U16_PREV(s, 0, cpStart, c2);
                    cc2=getCC(c2);
                    if(cc2<=cc) {
                        break;
                    }
                }
                // Insert c at cpLimit.
                for(int32_t q=prev, r=i; cpLimit!=q;) {
                    s[--r]=s[--q];
                }
                U16_APPEND_UNSAFE(s, cpLimit, c);
            }
        }
        m.releaseBuffer(length);
    }
}

// Requires p->mappingType>Norm::REMOVED.
void Normalizer2DataBuilder::writeMapping(UChar32 c, Norm *p, UnicodeString &dataString) {
    UnicodeString &m=*p->mapping;
    int32_t length=m.length();
    if(length>Normalizer2Data::MAPPING_LENGTH_MASK) {
        fprintf(stderr,
                "gennorm2 error: "
                "mapping for U+%04lX longer than maximum of %d\n",
                (long)c, Normalizer2Data::MAPPING_LENGTH_MASK);
        exit(U_INVALID_FORMAT_ERROR);
    }
    int32_t leadCC, trailCC;
    if(length==0) {
        leadCC=trailCC=0;
    } else {
        leadCC=getCC(m.char32At(0));
        trailCC=getCC(m.char32At(length-1));
    }
    int32_t firstUnit=length|(trailCC<<8);
    int32_t secondUnit=p->cc|(leadCC<<8);
    if(secondUnit!=0) {
        firstUnit|=Normalizer2Data::MAPPING_HAS_CCC_LCCC_WORD;
    }
    if(p->compositions!=NULL) {
        firstUnit|=Normalizer2Data::MAPPING_PLUS_COMPOSITION_LIST;
    }
    dataString.append((UChar)firstUnit);
    if(secondUnit!=0) {
        dataString.append((UChar)secondUnit);
    }
    dataString.append(m);
}

// Requires p->compositions!=NULL.
void Normalizer2DataBuilder::writeCompositions(UChar32 c, Norm *p, UnicodeString &dataString) {
    if(p->cc!=0) {
        fprintf(stderr,
                "gennorm2 error: "
                "U+%04lX combines-forward and has ccc!=0, not possible in Unicode normalization\n",
                (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    int32_t length=p->compositions->size();
    for(int32_t i=0; i<length; ++i) {
        CompositionPair &pair=p->compositions->at(i);
        // 22 bits for the composite character and whether it combines forward.
        UChar32 compositeAndFwd=pair.composite<<1;
        Norm *compositeNorm=getNorm(pair.composite);
        if(compositeNorm!=NULL && compositeNorm->compositions!=NULL) {
            compositeAndFwd|=1;  // The composite character also combines-forward.
        }
        // Encode most pairs in two units and some in three.
        int32_t firstUnit, secondUnit, thirdUnit;
        if(pair.trail<Normalizer2Data::COMP_1_TRAIL_LIMIT) {
            if(compositeAndFwd<=0xffff) {
                firstUnit=pair.trail<<1;
                secondUnit=compositeAndFwd;
                thirdUnit=-1;
            } else {
                firstUnit=(pair.trail<<1)|Normalizer2Data::COMP_1_TRIPLE;
                secondUnit=compositeAndFwd>>16;
                thirdUnit=compositeAndFwd;
            }
        } else {
            firstUnit=(Normalizer2Data::COMP_1_TRAIL_LIMIT+
                       (pair.trail>>Normalizer2Data::COMP_1_TRAIL_SHIFT))|
                      Normalizer2Data::COMP_1_TRIPLE;
            secondUnit=(pair.trail<<Normalizer2Data::COMP_2_TRAIL_SHIFT)|
                       (compositeAndFwd>>16);
            thirdUnit=compositeAndFwd;
        }
        // Set the high bit of the first unit if this is the last composition pair.
        if(i==(length-1)) {
            firstUnit|=Normalizer2Data::COMP_1_LAST_TUPLE;
        }
        dataString.append((UChar)firstUnit).append((UChar)secondUnit);
        if(thirdUnit>=0) {
            dataString.append((UChar)thirdUnit);
        }
    }
}

class ExtraDataWriter : public Normalizer2DBEnumerator {
public:
    ExtraDataWriter(Normalizer2DataBuilder &b) :
        Normalizer2DBEnumerator(b),
        yesYesCompositions(1000, (UChar32)0xffff, 2),  // 0=inert, 1=Jamo L, 2=start of compositions
        yesNoData(1000, (UChar32)0, 1) {}  // 0=Hangul, 1=start of normal data
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        if(value!=0) {
            if(start!=end) {
                fprintf(stderr,
                        "gennorm2 error: unexpected shared data for "
                        "multiple code points U+%04lX..U+%04lX\n",
                        (long)start, (long)end);
                exit(U_INTERNAL_PROGRAM_ERROR);
            }
            builder.writeExtraData(start, value, *this);
        }
        return TRUE;
    }
    UnicodeString maybeYesCompositions;
    UnicodeString yesYesCompositions;
    UnicodeString yesNoData;
    UnicodeString noNoMappings;
};

void Normalizer2DataBuilder::writeExtraData(UChar32 c, uint32_t value, ExtraDataWriter &writer) {
    Norm *p=norms+value;
    if(p->combinesBack) {
        if(p->mappingType>Norm::REMOVED) {
            fprintf(stderr,
                    "gennorm2 error: "
                    "U+%04lX combines-back and decomposes, not possible in Unicode normalization\n",
                    (long)c);
            exit(U_INVALID_FORMAT_ERROR);
        }
        if(p->compositions!=NULL) {
            p->offset=
                (writer.maybeYesCompositions.length()<<Norm::OFFSET_SHIFT)|
                Norm::OFFSET_MAYBEYES;
            writeCompositions(c, p, writer.maybeYesCompositions);
        }
    } else if(p->mappingType<=Norm::REMOVED) {
        if(p->compositions!=NULL) {
            p->offset=
                (writer.yesYesCompositions.length()<<Norm::OFFSET_SHIFT)|
                Norm::OFFSET_YESYES;
            writeCompositions(c, p, writer.yesYesCompositions);
        }
    } else if(p->mappingType==Norm::ROUND_TRIP) {
        if(p->cc!=0) {
            fprintf(stderr,
                    "gennorm2 error: "
                    "U+%04lX has a round-trip mapping and ccc!=0, "
                    "not possible in Unicode normalization\n",
                    (long)c);
            exit(U_INVALID_FORMAT_ERROR);
        }
        p->offset=
            (writer.yesNoData.length()<<Norm::OFFSET_SHIFT)|
            Norm::OFFSET_YESNO;
        writeMapping(c, p, writer.yesNoData);
        if(p->compositions!=NULL) {
            writeCompositions(c, p, writer.yesNoData);
        }
    } else /* one-way */ {
        if(p->compositions!=NULL) {
            fprintf(stderr,
                    "gennorm2 error: "
                    "U+%04lX combines-forward and has a one-way mapping, "
                    "not possible in Unicode normalization\n",
                    (long)c);
            exit(U_INVALID_FORMAT_ERROR);
        }
        if(p->cc==0) {
            // Try a compact, algorithmic encoding.
            // Only for ccc=0.
            // Not when mapping to a Hangul syllable, or else the runtime decomposition and
            // data access functions would have to deal with Hangul-Jamo decomposition.
            if(p->mapping->isEmpty()) {
                p->offset=Norm::OFFSET_DELTA;  // maps to empty string
            } else if(p->mappingCP>=0 && !isHangul(p->mappingCP)) {
                int32_t delta=c-p->mappingCP;
                if(-Normalizer2Data::MAX_DELTA<=delta && delta<=Normalizer2Data::MAX_DELTA) {
                    p->offset=(delta<<Norm::OFFSET_SHIFT)|Norm::OFFSET_DELTA;
                }
            }
        }
        if(p->offset==0) {
            p->offset=
                (writer.noNoMappings.length()<<Norm::OFFSET_SHIFT)|
                Norm::OFFSET_NONO;
            writeMapping(c, p, writer.noNoMappings);
        }
    }
}

void Normalizer2DataBuilder::setHangulData() {
    HangulIterator hi;
    const HangulIterator::Range *range;
    // Check that none of the Hangul/Jamo code points have data.
    while((range=hi.nextRange())!=NULL) {
        for(UChar32 c=range->start; c<range->limit; ++c) {
            if(utrie2_get32(norm16Trie, c)!=0) {
                fprintf(stderr,
                        "gennorm2 error: "
                        "illegal mapping/composition/ccc data for Hangul or Jamo U+%04lX\n",
                        (long)c);
                exit(U_INVALID_FORMAT_ERROR);
            }
        }
    }
    // Set data for algorithmic runtime handling.
    IcuToolErrorCode errorCode("gennorm2/setHangulData()");
    hi.reset();
    while((range=hi.nextRange())!=NULL) {
        uint16_t norm16=range->norm16;
        if(norm16==0) {
            norm16=indexes[Normalizer2Data::IX_MIN_YES_NO];  // Hangul LV/LVT encoded as minYesNo
        }
        utrie2_setRange32(norm16Trie, range->start, range->limit-1, norm16, TRUE, errorCode);
        errorCode.assertSuccess();
    }
}

void Normalizer2DataBuilder::processData() {
    IcuToolErrorCode errorCode("gennorm2/processData()");
    norm16Trie=utrie2_open(0, 0, errorCode);
    errorCode.assertSuccess();

    utrie2_enum(normTrie, NULL, enumRangeHandler, CompositionBuilder(*this).ptr());

    Decomposer decomposer(*this);
    do {
        decomposer.didDecompose=FALSE;
        utrie2_enum(normTrie, NULL, enumRangeHandler, &decomposer);
    } while(decomposer.didDecompose);

    int32_t normsLength=utm_countItems(normMem);
    for(int32_t i=1; i<normsLength; ++i) {
        reorder(norms+i);
    }

    ExtraDataWriter extraDataWriter(*this);
    utrie2_enum(normTrie, NULL, enumRangeHandler, &extraDataWriter);

    extraData=extraDataWriter.maybeYesCompositions;
    extraData.append(extraDataWriter.yesYesCompositions).
              append(extraDataWriter.yesNoData).
              append(extraDataWriter.noNoMappings);

    indexes[Normalizer2Data::IX_MIN_YES_NO]=
        extraDataWriter.yesYesCompositions.length();
    indexes[Normalizer2Data::IX_MIN_NO_NO]=
        indexes[Normalizer2Data::IX_MIN_YES_NO]+
        extraDataWriter.yesNoData.length();
    indexes[Normalizer2Data::IX_LIMIT_NO_NO]=
        indexes[Normalizer2Data::IX_MIN_NO_NO]+
        extraDataWriter.noNoMappings.length();
    indexes[Normalizer2Data::IX_MIN_MAYBE_YES]=
        Normalizer2Data::MIN_NORMAL_MAYBE_YES-
        extraDataWriter.maybeYesCompositions.length();

    int32_t centerNoNoDelta=indexes[Normalizer2Data::IX_MIN_MAYBE_YES]-Normalizer2Data::MAX_DELTA-1;
    int32_t minNoNoDelta=centerNoNoDelta-Normalizer2Data::MAX_DELTA;
    if(indexes[Normalizer2Data::IX_LIMIT_NO_NO]>minNoNoDelta) {
        fprintf(stderr,
                "gennorm2 error: "
                "data structure overflow, too much mapping composition data\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    // TODO: write final norm16

    setHangulData();
}

void Normalizer2DataBuilder::writeBinaryFile(const char *filename) {
    processData();
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_NORMALIZATION */

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 */
