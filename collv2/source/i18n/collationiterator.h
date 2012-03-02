/*
*******************************************************************************
* Copyright (C) 2010-2012, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* collationiterator.h
*
* created on: 2010oct27
* created by: Markus W. Scherer
*/

#ifndef __COLLATIONITERATOR_H__
#define __COLLATIONITERATOR_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "collationiterator.h"
#include "normalizer2impl.h"

U_NAMESPACE_BEGIN

/**
 * Buffer for CEs.
 * Rather than its own position and length fields,
 * we share the cesIndex and cesMaxIndex of the CollationIterator.
 */
class CEBuffer {
public:
    CEBuffer() {}

    inline int32_t append(int32_t length, int64_t ce, UErrorCode &errorCode) {
        if(length < buffer.getCapacity()) {
            buffer[length++] = ce;
            return length;
        } else {
            return doAppend(length, ce, errorCode);
        }
    }

    inline int64_t &operator[](ptrdiff_t i) { return buffer[i]; }

private:
    CEBuffer(const CEBuffer &);
    void operator=(const CEBuffer &);

    void doAppend(int32_t length, int64_t ce, UErrorCode &errorCode);

    MaybeStackArray<int64_t, 40> buffer;
};

int32_t
CEBuffer::doAppend(int32_t length, int64_t ce, UErrorCode &errorCode) {
    // length == buffer.getCapacity()
    if(U_FAILURE(errorCode)) { return; }
    int32_t capacity = buffer.getCapacity();
    int32_t newCapacity;
    if(capacity < 1000) {
        newCapacity = 4 * capacity;
    } else {
        newCapacity = 2 * capacity;
    }
    int64_t *p = buffer.resize(newCapacity, length);
    if(p == NULL) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    } else {
        p[length++] = ce;
    }
    return length;
}

/**
 * Collation element and character iterator.
 * Handles normalized UTF-16 text inline, with length or NUL-terminated.
 * Other text is handled by subclasses.
 *
 * This iterator only moves forward through CE space.
 * For random access, use the TwoWayCollationIterator.
 *
 * Forward iteration and random access are separate for
 * - minimal state & setup for the forward-only iterator
 * - modularization
 * - smaller units of code, easier to understand
 * - easier porting of partial functionality to other languages
 */
class U_I18N_API CollationIterator : public UObject {
public:
    CollationIterator(const Normalizer2Impl &nfc,
                      const CollationData *d, int8_t iterFlags,
                      const UChar *s, const UChar *lim)
            // Optimization: Skip initialization of fields that are not used
            // until they are set together with other state changes.
            : start(s), pos(s), limit(lim),
              nfcImpl(nfc),
              flags(iterFlags),
              trie(d->getTrie()),
              cesIndex(-1),  // cesMaxIndex(0), ces(NULL), -- unused while cesIndex<0
              hiragana(0),
              data(d) {}

    void setFlags(int8_t f) { flags = f; }

    /**
     * Returns the next collation element.
     * Optimized inline fastpath for the most common types of text and data.
     */
    inline int64_t nextCE(UErrorCode &errorCode) {
        if(cesIndex >= 0) {
            // Return the next buffered CE.
            int64_t ce = ces[cesIndex];
            if(cesIndex < cesMaxIndex) {
                ++cesIndex;
            } else {
                cesIndex = -1;
            }
            return ce;
        }
        hiragana = 0;
        UChar32 c;
        uint32_t ce32;
        if(pos != limit) {
            UTRIE2_U16_NEXT32(trie, pos, limit, c, ce32);
        } else {
            c = handleNextCodePoint(errorCode);
            if(c < 0) {
                return Collation::NO_CE;
            }
            ce32 = data->getCE32(c);
        }
        // Java: Emulate unsigned-int less-than comparison.
        // int xce32 = ce32 ^ 0x80000000;
        // if(xce32 < 0x7f000000) { special }
        // if(xce32 == 0x7f000000) { fallback }
        if(ce32 < Collation::MIN_SPECIAL_CE32) {  // Forced-inline of isSpecialCE32(ce32).
            // Normal CE from the main data.
            return Collation::ceFromCE32(ce32);
        }
        const CollationData *d;
        // The compiler should be able to optimize the previous and the following
        // comparisons of ce32 with the same constant.
        if(ce32 == Collation::MIN_SPECIAL_CE32) {
            d = data->getBase();
            ce32 = d->getCE32(c);
            if(!Collation::isSpecialCE32(ce32)) {
                // Normal CE from the base data.
                return Collation::ceFromCE32(ce32);
            }
        } else {
            d = data;
        }
        return nextCEFromSpecialCE32(d, c, ce32, errorCode);
    }

    /**
     * Returns the Hiragana flag.
     * The caller must remember the previous flag value.
     * @return -1 inherit Hiragana-ness from previous character;
     *         0 not Hiragana; 1 Hiragana
     */
    inline int8_t getHiragana() const { return hiragana; }

    inline UChar32 nextCodePoint(UErrorCode &errorCode) {
        if(pos != limit) {
            UChar32 c = *pos;
            if(c == 0 && limit == NULL) {
                limit = pos;
                return U_SENTINEL;
            }
            ++pos;
            UChar trail;
            if(U16_IS_LEAD(c) && pos != limit && U16_IS_TRAIL(trail = *pos)) {
                ++pos;
                return U16_GET_SUPPLEMENTARY(c, trail);
            } else {
                return c;
            }
        }
        return handleNextCodePoint(errorCode);
    }

    inline UChar32 previousCodePoint(UErrorCode &errorCode) {
        if(pos != start) {
            UChar32 c = *--pos;
            UChar lead;
            if(U16_IS_TRAIL(c) && pos != start && U16_IS_LEAD(lead = *(pos - 1))) {
                --pos;
                return U16_GET_SUPPLEMENTARY(lead, c);
            } else {
                return c;
            }
        }
        return handlePreviousCodePoint(errorCode);
    }

    void forwardNumCodePoints(int32_t num, UErrorCode &errorCode) {
        while(num > 0) {
            // Go forward in the inner buffer as far as possible.
            while(pos != limit) {
                UChar32 c = *pos;
                if(c == 0 && limit == NULL) {
                    limit = pos;
                    break;
                }
                ++pos;
                --num;
                if(U16_IS_LEAD(c) && pos != limit && U16_IS_TRAIL(*pos)) {
                    ++pos;
                }
                if(num == 0) {
                    return;
                }
            }
            // Try to go forward by one code point via the iterator,
            // then return to the inner buffer in case the subclass moved that forward.
            if(handleNextCodePoint(errorCode) < 0) {
                return;
            }
            --num;
        }
    }

    void backwardNumCodePoints(int32_t num, UErrorCode &errorCode) {
        while(num > 0) {
            // Go backward in the inner buffer as far as possible.
            while(pos != start) {
                UChar32 c = *--pos;
                --num;
                if(U16_IS_TRAIL(c) && pos != start && U16_IS_LEAD(*(pos-1))) {
                    --pos;
                }
                if(num == 0) {
                    return;
                }
            }
            // Try to go backward by one code point via the iterator,
            // then return to the inner buffer in case the subclass moved that backward.
            if(handlePreviousCodePoint(errorCode) < 0) {
                return;
            }
            --num;
        }
    }

    // TODO: setText(start, pos, limit)  ?

protected:
    /**
     * Returns the next code point, or <0 if none, assuming pos==limit.
     * Post-increment semantics.
     */
    virtual UChar32 handleNextCodePoint(UErrorCode & /* errorCode */) {
        U_ASSERT(pos == limit);
        return U_SENTINEL;
    }

    /**
     * Returns the previous code point, or <0 if none, assuming pos==start.
     * Pre-decrement semantics.
     */
    virtual UChar32 handlePreviousCodePoint(UErrorCode & /* errorCode */) {
        U_ASSERT(pos == start);
        return U_SENTINEL;
    }

    /**
     * Saves the current iteration limit for later,
     * and sets it to after c which was read by previousCodePoint() or equivalent.
     */
    virtual const UChar *saveLimitAndSetAfter(UChar32 c) {
        const UChar *savedLimit = limit;
        limit = pos + U16_LENGTH(c);
        return savedLimit;
    }

    /** Restores the iteration limit from before saveLimitAndSetAfter(). */
    virtual void restoreLimit(const UChar *savedLimit) {
        limit = savedLimit;
    }

    // UTF-16 string pointers.
    // limit can be NULL for NUL-terminated strings.
    // This class assumes that whole code points are stored within [start..limit[.
    // That is, a trail surrogate at start or a lead surrogate at limit-1
    // will be assumed to be surrogate code points rather than attempting to pair it
    // with a surrogate retrieved from the subclass.
    const UChar *start, *pos, *limit;
    // TODO: getter for limit, so that caller can find out length of NUL-terminated text?

    const Normalizer2Impl &nfcImpl;

    int8_t flags;

    // TODO: Do we need to support changing iteration direction? (ICU ticket #9104.)
    // If so, then nextCE() (rather, a "slow" version of it)
    // and previousCE() must count how many code points
    // resulted in their CE or ces[], and when they exhaust ces[] they need to
    // check if the signed code point count is in the right direction;
    // if not, move by that much in the opposite direction.
    // For example, if previousCE() read 3 code points, set ces[],
    // and then we change to nextCE() and it exhausts those ces[],
    // then we need to skip forward over those 3 code points before reading
    // more text.

private:
    friend class TwoWayCollationIterator;

    int64_t nextCEFromSpecialCE32(const CollationData *d, UChar32 c, uint32_t ce32,
                                  UErrorCode &errorCode) const {
        for(;;) {  // Loop while ce32 is special.
            // TODO: Share code with previousCEFromSpecialCE32().
            int32_t tag = Collation::getSpecialCE32Tag(ce32);
            if(tag <= Collation::MAX_LATIN_EXPANSION_TAG) {
                U_ASSERT(ce32 != MIN_SPECIAL_CE32);
                setLatinExpansion(ce32);
                return ces[0];
            }
            switch(tag) {
            case Collation::EXPANSION32_TAG:
                setCE32s(d, (ce32 >> 4) & 0xffff, (int32_t)ce32 & 0xf);
                cesIndex = (cesMaxIndex > 0) ? 1 : -1;
                return ces[0];
            case Collation::EXPANSION_TAG:
                ces = d->getCEs((ce32 >> 4) & 0xffff);
                cesMaxIndex = (int32_t)ce32 & 0xf;
                cesIndex = (cesMaxIndex > 0) ? 1 : -1;
                return ces[0];
            case Collation::PREFIX_TAG:
                backwardNumCodePoints(1, errorCode);
                ce32 = getCE32FromPrefix(d, ce32, errorCode);
                forwardNumCodePoints(1, errorCode);
                break;
            case Collation::CONTRACTION_TAG:
                ce32 = nextCE32FromContraction(d, c, ce32, errorCode);
                if(ce32 != 0x100) {
                    // Normal result from contiguous contraction.
                    break;
                } else {
                    // CEs from a discontiguous contraction plus the skipped combining marks.
                    return ces[0];
                }
            /**
            * Used only in the collation data builder.
            * Data bits point into a builder-specific data structure with non-final data.
            */
            case Collation::BUILDER_CONTEXT_TAG:
                ce32 = getCE32FromBuilderContext(ce32, errorCode);
                break;
            case Collation::DIGIT_TAG:
                if(flags & Collation::CODAN) {
                    // Collect digits, omit leading zeros.
                    CharString digits;
                    for(;;) {
                        char digit = (char)(ce32 & 0xf);
                        if(digit != 0 || !digits.isEmpty()) {
                            digits.append(digit, errorCode);
                        }
                        c = nextCodePoint(errorCode);
                        if(c < 0) { break; }
                        ce32 = data->getCE32(c);
                        if(ce32 == Collation::MIN_SPECIAL_CE32) {
                            ce32 = data->getBase()->getCE32(c);
                        }
                        if(!Collation::isSpecialCE32(ce32) ||
                            Collation::DIGIT_TAG != Collation::getSpecialCE32Tag(ce32)
                        ) {
                            backwardNumCodePoints(1, errorCode);
                            break;
                        }
                    }
                    int32_t length = digits.length();
                    if(length == 0) {
                        // A string of only "leading" zeros.
                        // Just use the NUL terminator in the digits buffer.
                        length = 1;
                    }
                    setCodanCEs(digits.data(), length, errorCode);
                    cesIndex = (cesMaxIndex > 0) ? 1 : -1;
                    return ces[0];
                } else {
                    // Fetch the non-CODAN CE32 and continue.
                    ce32 = *d->getCE32s((ce32 >> 4) & 0xffff);
                    break;
                }
            case Collation::HIRAGANA_TAG:
                hiragana = (0x3099 <= c && c <= 0x309c) ? -1 : 1;
                // Fetch the normal CE32 and continue.
                ce32 = *d->getCE32s(ce32 & 0xfffff);
                break;
            case Collation::HANGUL_TAG:
                setHangulExpansion(ce32, errorCode);
                cesIndex = 1;
                return ces[0];
            case Collation::OFFSET_TAG:
                return getCEFromOffsetCE32(d, c, ce32);
            case Collation::IMPLICIT_TAG:
                if((ce32 & 1) == 0) {
                    U_ASSERT(c == 0);
                    if(limit == NULL) {
                        // Handle NUL-termination. (Not needed in Java.)
                        limit = --pos;
                        return Collation::NO_CE;
                    } else {
                        // Fetch the normal ce32 for U+0000 and continue.
                        ce32 = *d->getCE32s(0);
                        break;
                    }
                } else {
                    return Collation::unassignedCEFromCodePoint(c);
                }
            }
            if(!Collation::isSpecialCE32(ce32)) {
                return Collation::ceFromCE32(ce32);
            }
        }
    }

    /**
     * Computes a CE from c's ce32 which has the OFFSET_TAG.
     */
    static int64_t getCEFromOffsetCE32(const CollationData *d, UChar32 c, uint32_t ce32) {
        UChar32 baseCp = (c & 0x1f0000) | (((UChar32)ce32 >> 4) & 0xffff);
        int32_t offset = (c - baseCp) * ((ce32 & 0xf) + 1);  // delta * increment
        ce32 = d->getCE32(trie, baseCp);
        // ce32 must be a long-primary pppppp01.
        U_ASSERT(!Collation::isSpecialCE32(ce32) && (ce32 & 0xff) == 1);
        --ce32;  // Turn the long-primary CE32 into a primary weight pppppp00.
        return Collation::getCEFromThreeByteOffset(ce32, d->isCompressiblePrimary(ce32), offset);
    }

    uint32_t getCE32FromPrefix(const CollationData *d, uint32_t ce32,
                               UErrorCode &errorCode) const {
        const uint16_t *p = d->getContext((int32_t)ce32 & 0xfffff);
        ce32 = ((uint32_t)p[0] << 16) | p[1];  // Default if no prefix match.
        p += 2;
        // Number of code points read before the original code point.
        int32_t lookBehind = 0;
        UCharsTrie prefixes(p);
        for(;;) {
            UChar32 c = previousCodePoint(errorCode);
            if(c < 0) { break; }
            ++lookBehind;
            UStringTrieResult match = prefixes.nextForCodePoint(c);
            if(USTRINGTRIE_HAS_VALUE(match)) {
                ce32 = (uint32_t)prefixes.getValue();
            }
            if(!USTRINGTRIE_HAS_NEXT(match)) { break; }
        }
        forwardNumCodePoints(lookBehind, errorCode);
        return ce32;
    }

    uint32_t nextCE32FromContraction(const CollationData *d, UChar32 originalCp, uint32_t ce32,
                                     UErrorCode &errorCode) const {
        // originalCp: Only needed as input to nextCE32FromDiscontiguousContraction().
        UBool maybeDiscontiguous = (UBool)(ce32 & 1);  // TODO: Builder set this if any suffix ends with cc != 0.
        const uint16_t *p = d->getContext((int32_t)(ce32 >> 1) & 0x7ffff);
        ce32 = ((uint32_t)p[0] << 16) | p[1];  // Default if no suffix match.
        UChar32 lowestChar = p[2];
        p += 3;
        UChar32 c = nextCodePoint(errorCode);
        if(c < 0) {
            // No more text.
            return ce32;
        }
        if(c < lowestChar) {
            // The next code point is too low for either a match
            // or a combining mark that would be skipped in discontiguous contraction.
            // TODO: Builder:
            // lowestChar = lowest code point of the first ones that could start a match.
            // If the character is supplemental, set to U+FFFF.
            // If there are combining marks, then find the lowest combining class c1 among them,
            // then set instead to the lowest character (below what we have so far)
            // that has combining class in the range 1..c1-1.
            // Actually test characters with lccc!=0 and look for their tccc=1..c1-1.
            // TODO: This is simpler at runtime than checking for the combining class then,
            // and might be good enough for Latin performance. Keep or redesign?
            backwardNumCodePoints(1, errorCode);
            return ce32;
        }
        // Number of code points read beyond the original code point.
        // Only needed as input to nextCE32FromDiscontiguousContraction().
        int32_t lookAhead = 1;
        // Number of code points read since the last match value.
        int32_t sinceMatch = 1;
        // Assume that we only need a contiguous match,
        // and therefore need not remember the suffixes state from before a mismatch for retrying.
        UCharsTrie suffixes(p);
        UStringTrieResult match = suffixes.firstForCodePoint(c);
        for(;;) {
            if(match == USTRINGTRIE_NO_MATCH) {
                if(maybeDiscontiguous) {
                    return nextCE32FromDiscontiguousContraction(
                        d, originalCp, p, ce32, lookAhead, sinceMatch, c, errorCode);
                }
                break;
            }
            if(USTRINGTRIE_HAS_VALUE(match)) {
                ce32 = (uint32_t)suffixes.getValue();
                sinceMatch = 0;
            }
            if(!USTRINGTRIE_HAS_NEXT(match)) { break; }
            if((c = nextCodePoint(errorCode)) < 0) { break; }
            ++lookAhead;
            ++sinceMatch;
            match = suffixes.nextForCodePoint(c);
        }
        backwardNumCodePoints(sinceMatch, errorCode);
        return ce32;
    }
    // TODO: How can we match the second code point of a precomposed Tibetan vowel mark??
    //    Add contractions with the skipped mark as an expansion??
    //    Forbid contractions with the problematic characters??

    uint32_t nextCE32FromDiscontiguousContraction(
            const CollationData *d, UChar32 originalCp,
            const uint16_t *p, uint32_t ce32,
            int32_t lookAhead, int32_t sinceMatch, UChar32 c,
            UErrorCode &errorCode) const {
        // UCA 3.3.2 Contractions:
        // Contractions that end with non-starter characters
        // are known as discontiguous contractions.
        // ... discontiguous contractions must be detected in input text
        // whenever the final sequence of non-starter characters could be rearranged
        // so as to make a contiguous matching sequence that is canonically equivalent.

        // UCA: http://www.unicode.org/reports/tr10/#S2.1
        // S2.1 Find the longest initial substring S at each point that has a match in the table.
        // S2.1.1 If there are any non-starters following S, process each non-starter C.
        // S2.1.2 If C is not blocked from S, find if S + C has a match in the table.
        //     Note: A non-starter in a string is called blocked
        //     if there is another non-starter of the same canonical combining class or zero
        //     between it and the last character of canonical combining class 0.
        // S2.1.3 If there is a match, replace S by S + C, and remove C.

        // First: Is a discontiguous contraction even possible?
        uint16_t fcd16 = nfcImpl.getFCD16(c);
        UChar32 nextCp;
        if(fcd16 <= 0xff || (nextCp = nextCodePoint(errorCode)) < 0) {
            // The non-matching c is a starter (which blocks all further non-starters),
            // or there is no further text.
            backwardNumCodePoints(sinceMatch, errorCode);
            return ce32;
        }
        ++lookAhead;
        ++sinceMatch;
        uint8_t prevCC = (uint8_t)fcd16;
        fcd16 = nfcImpl.getFCD16(nextCp);
        if(prevCC >= (fcd16 >> 8)) {
            // The next code point after c is a starter (S2.1.1 "process each non-starter"),
            // or blocked by c (S2.1.2).
            backwardNumCodePoints(sinceMatch, errorCode);
            return ce32;
        }

        // We have read and matched (lookAhead-2) code points,
        // read non-matching c and peeked ahead at nextCp.
        // Replay the partial match so far, return to the state before the mismatch,
        // and continue matching with nextCp.
        UCharsTrie suffixes(p);
        if(lookAhead > 2) {
            backwardNumCodePoints(lookAhead, errorCode);
            suffixes.firstForCodePoint(nextCodePoint(errorCode));
            for(int32_t i = 3; i < lookAhead; ++i) {
                suffixes.nextForCodePoint(nextCodePoint(errorCode));
            }
            // Skip c (which did not match) and nextCp (which we will try now).
            forwardNumCodePoints(2, errorCode);
        }

        // Buffer for skipped combining marks.
        UnicodeString skipBuffer(c);
        int32_t skipLengthAtMatch = 0;
        UCharsTrie.State state;
        suffixes.saveState(state);
        c = nextCp;
        for(;;) {
            prevCC = (uint8_t)fcd16;
            UStringTrieResult match = suffixes.nextForCodePoint(c);
            if(match == USTRINGTRIE_NO_MATCH) {
                skipBuffer.append(c);
                suffixes.resetToState(state);
            } else {
                if(USTRINGTRIE_HAS_VALUE(match)) {
                    ce32 = (uint32_t)suffixes.getValue();
                    sinceMatch = 0;
                    skipLengthAtMatch = skipBuffer.length();
                }
                if(!USTRINGTRIE_HAS_NEXT(match)) { break; }
                suffixes.saveState(state);
            }
            if((c = nextCodePoint(errorCode)) < 0) { break; }
            ++sinceMatch;
            fcd16 = nfcImpl.getFCD16(c);
            if(prevCC >= (fcd16 >> 8)) {
                // The next code point after c is a starter (S2.1.1 "process each non-starter"),
                // or blocked by c (S2.1.2).
                break;
            }
        }
        if(skipLengthAtMatch > 0) {
            // We did get a match after skipping one or more combining marks.
            // Append CEs from the contraction ce32
            // and then from the combining marks that we skipped before the match.
            int32_t cesLength = 0;
            c = originalCp;
            cesLength = appendCEsFromCE32NoContext(cesLength, d, c, ce32, errorCode);
            // Fetch CE32s for skipped combining marks from the normal data, with fallback,
            // rather than from the CollationData where we found the contraction.
            for(int32_t i = 0; i < skipLengthAtMatch; i += U16_LENGTH(c)) {
                c = skipBuffer.char32At(i);
                cesLength = appendCEsFromCpNoContext(cesLength, c, errorCode);
            }
            cesIndex = 1;  // Caller returns ces[0].
            cesMaxIndex = cesLength - 1;
            ce32 = 0x100;  // Signal to nextCEFromSpecialCE32() that the result is in ces[].
        }
        backwardNumCodePoints(sinceMatch, errorCode);
        return ce32;
    }

    int32_t appendCEsFromCpNoContext(int32_t cesLength, UChar32 c, UErrorCode &errorCode) {
        const CollationData *d;
        ce32 = data->getCE32(c);
        if(ce32 == Collation::MIN_SPECIAL_CE32) {
            d = data->getBase();
            ce32 = d->getCE32(c);
        } else {
            d = data;
        }
        return appendCEsFromCE32NoContext(cesLength, d, c, ce32, errorCode);
    }

    int32_t appendCEsFromCE32NoContext(int32_t cesLength,
                                       const CollationData *d, UChar32 c, uint32_t ce32,
                                       UErrorCode &errorCode) {
        // TODO: copy much of nextCEFromSpecialCE32()
        // handle non-special CE32 first
        // contraction/digit/Hiragana just get default CE32
        // U_INTERNAL_PROGRAM_ERROR for prefix
        // U_INTERNAL_PROGRAM_ERROR for contraction? (depends on whether we forbid contractions from lccc!=0)
        // most cases fall through to end which calls  cesLength = forwardCEs.append(cesLength, ce, errorCode);
    }

    /**
     * Turns a string of digits (bytes 0..9)
     * into a sequence of CEs that will sort in numeric order.
     * CODAN = COllate Digits As Numbers.
     *
     * Sets ces and cesMaxIndex.
     *
     * The digits string must not be empty and must not have leading zeros.
     */
    void setCodanCEs(const char *digits, int32_t length, UErrorCode &errorCode) {
        cesMaxIndex = 0;
        if(U_FAILURE(errorCode)) {
            forwardCEs[0] = 0;
            return;
        }
        U_ASSERT(length > 0);
        U_ASSERT(length == 1 || digits[0] != 0);
        uint32_t zeroPrimary = data->getZeroPrimary();
        // Note: We use primary byte values 3..255: digits are not compressible.
        if(length <= 5) {
            // Very dense encoding for small numbers.
            int32_t value = digits[0];
            for(int32_t i = 1; i < length; ++i) {
                value = value * 10 + digits[i];
            }
            if(value <= 31) {
                // Two-byte primary for 0..31, good for days & months.
                uint32_t primary = zeroPrimary | ((3 + value) << 16);
                forwardCEs[0] = ((int64_t)primary << 32) | Collation::COMMON_SEC_AND_TER_CE;
                return;
            }
            value -= 32;
            if(value < 40 * 253) {
                // Three-byte primary for 32..10151, good for years.
                // 10151 = 32+40*253-1
                uint32_t primary = zeroPrimary |
                    ((3 + 32 + value / 253) << 16) | ((3 + value % 253) << 8);
                forwardCEs[0] = ((int64_t)primary << 32) | Collation::COMMON_SEC_AND_TER_CE;
                return;
            }
        }
        // value > 10151, length >= 5

        // The second primary byte 75..255 indicates the number of digit pairs (3..183),
        // then we generate primary bytes with those pairs.
        // Omit trailing 00 pairs.
        // Decrement the value for the last pair.
        if(length > 2 * 183) {
            // Overflow
            uint32_t primary = zeroPrimary | 0xffff00;
            forwardCEs[0] = ((int64_t)primary << 32) | Collation::COMMON_SEC_AND_TER_CE;
            return;
        }
        // Set the exponent. 3 pairs->75, 4 pairs->76, ..., 183 pairs->255.
        int32_t numPairs = (length + 1) / 2;
        uint32_t primary = zeroPrimary | ((75 - 3 + numPairs) << 16);
        // Find the length without trailing 00 pairs.
        while(digits[length - 1] == 0 && digits[length - 2] == 0) {
            length -= 2;
        }
        // Read the first pair.
        uint32_t pair;
        int32_t pos;
        if(length & 1) {
            // Only "half a pair" if we have an odd number of digits.
            pair = digits[0];
            pos = 1;
        } else {
            pair = digits[0] * 10 + digits[1];
            pos = 2;
        }
        pair = 11 + 2 * pair;
        // Add the pairs of digits between pos and length.
        int32_t shift = 8;
        int32_t cesLength = 0;
        while(pos < length) {
            if(shift == 0) {
                // Every three pairs/bytes we need to store a 4-byte-primary CE
                // and start with a new CE with the '0' primary lead byte.
                primary |= pair;
                cesLength = forwardCEs.append(cesLength,
                    ((int64_t)primary << 32) | Collation::COMMON_SEC_AND_TER_CE, errorCode);
                primary = zeroPrimary;
                shift = 16;
            } else {
                primary |= pair << shift;
                shift -= 8;
            }
            pair = 11 + 2 * (digits[pos] * 10 + digits[pos + 1]);
            pos += 2;
        }
        primary |= (pair - 1) << shift;
        cesLength = forwardCEs.append(cesLength,
            ((int64_t)primary << 32) | Collation::COMMON_SEC_AND_TER_CE, errorCode);
        ces = forwardCEs.getBuffer();
        cesMaxIndex = cesLength;
    }

    /**
     * Sets 2 or 3 buffered CEs from a Hangul syllable,
     * assuming that the constituent Jamos all have non-special CE32s.
     * Otherwise DECOMP_HANGUL would have to be set.
     *
     * Sets cesMaxIndex as necessary.
     * Does not set cesIndex;
     * caller needs to set cesIndex=1 for forward iteration,
     * or cesIndex=cesMaxIndex for backward iteration.
     */
    void setHangulExpansion(UChar32 c, UErrorCode &errorCode) {
        const uint32_t *jamoCE32s = data->getJamoCE32s();
        c -= Hangul::HANGUL_BASE;
        UChar32 t = c % Hangul::JAMO_T_COUNT;
        c /= Hangul::JAMO_T_COUNT;
        if(t == 0) {
            cesMaxIndex = 1;
        } else {
            // offset 39 = 19 + 21 - 1:
            // 19 = JAMO_L_COUNT
            // 21 = JAMO_T_COUNT
            // -1 = omit t==0
            forwardCEs[2] = Collation::ceFromCE32(jamoCE32s[39 + t]);
            cesMaxIndex = 2;
        }
        UChar32 v = c % Hangul::JAMO_V_COUNT;
        c /= Hangul::JAMO_V_COUNT;
        forwardCEs[0] = Collation::ceFromCE32(jamoCE32s[c]);
        forwardCEs[1] = Collation::ceFromCE32(jamoCE32s[19 + v]);
        ces = forwardCEs.getBuffer();
    }

    /**
     * Sets 2 buffered CEs from a Latin mini-expansion CE32.
     * Sets cesIndex=cesMaxIndex=1.
     */
    void setLatinExpansion(uint32_t ce32) {
        forwardCEs[0] = ((int64_t)(ce32 & 0xff0000) << 40) | COMMON_SECONDARY_CE | (ce32 & 0xff00);
        forwardCEs[1] = ((ce32 & 0xff) << 24) | COMMON_TERTIARY_CE;
        ces = forwardCEs.getBuffer();
        cesIndex = cesMaxIndex = 1;
    }

    /**
     * Sets buffered CEs from CE32s.
     */
    void setCE32s(const CollationData *d, int32_t expIndex, int32_t max) {
        ces = forwardCEs.getBuffer();
        const uint32_t *ce32s = d->getCE32s(expIndex);
        cesMaxIndex = max;
        for(int32_t i = 0; i <= max; ++i) {
            forwardCEs[i] = ceFromCE32(ce32s[i]);
        }
    }

    // Main lookup trie of the data object.
    const UTrie2 *trie;

    // List of CEs.
    int32_t cesIndex, cesMaxIndex;
    const int64_t *ces;

    int8_t hiragana;

    const CollationData *data;

    // 64-bit-CE buffer for forward and safe-backward iteration
    // (computed expansions and CODAN CEs).
    CEBuffer forwardCEs;
};

/**
 * Checks the input text for FCD, passes already-FCD segments into the base iterator,
 * and normalizes other segments on the fly.
 */
class U_I18N_API FCDCollationIterator : public CollationIterator {
public:
    FCDCollationIterator(const Normalizer2Impl &nfc,
                         const CollationData *data, int8_t iterFlags,
                         const UChar *s, const UChar *lim,
                         UErrorCode &errorCode)
            : CollationIterator(nfc, data, iterFlags, s, s),
              rawStart(s), segmentStart(s), segmentLimit(s), rawLimit(lim),
              lengthBeforeLimit(0),
              smallSteps(TRUE),
              buffer(nfc, normalized) {
        if(U_SUCCESS(errorCode)) {
            buffer.init(2, errorCode);
        }
    }

    void setSmallSteps(UBool small) { smallSteps = small; }

protected:
    virtual UChar32 handleNextCodePoint(UErrorCode &errorCode) {
        if(U_FAILURE(errorCode) || segmentLimit == rawLimit) { return U_SENTINEL; }
        U_ASSERT(pos == limit);
        if(lengthBeforeLimit != 0) {
            int32_t length = (int32_t)(limit - start);
            if(lengthBeforeLimit <= length) {
                // We have reached the end of the saveLimitAndSetAfter() range.
                return U_SENTINEL;
            }
            lengthBeforeLimit -= length;
        }
        if(limit != segmentLimit) {
            // The previous segment had to be normalized
            // and was pointing into the normalized string.
            start = pos = limit = segmentLimit;
        }
        segmentStart = segmentLimit;
        if((flags & Collation::CHECK_FCD) == 0) {
            U_ASSERT((flags & Collation::DECOMP_HANGUL) != 0);
            return nextCodePointDecompHangul(errorCode);
        }
        const UChar *p = segmentLimit;
        uint8_t prevCC = 0;
        for(;;) {
            // So far, we have limit<=segmentLimit<=p,
            // [limit, p[ passes the FCD test,
            // and segmentLimit is at the last FCD boundary before or on p.
            // Advance p by one code point, fetch its fcd16 value,
            // and continue the incremental FCD test.
            const UChar *q = p;
            UChar32 c = *p++;
            if(c < 0x180) {
                if(c == 0) {
                    if(rawLimit == NULL) {
                        // We hit the NUL terminator; remember its pointer.
                        segmentLimit = rawLimit == q;
                        if(limit == rawLimit) { return U_SENTINEL; }
                        limit = rawLimit;
                        break;
                    }
                    segmentLimit = p;
                    prevCC = 0;
                } else {
                    prevCC = (uint8_t)nfcImpl.getFCD16FromBelow180(c);  // leadCC == 0
                    if(prevCC <= 1) {
                        segmentLimit = p;  // FCD boundary after the [q, p[ code point.
                    } else {
                        segmentLimit = q;  // FCD boundary before the [q, p[ code point.
                    }
                }
            } else if(!nfcImpl.singleLeadMightHaveNonZeroFCD16(c)) {
                if(c >= 0xac00) {
                    if((flags & Collation::DECOMP_HANGUL) && c <= 0xd7a3) {
                        if(limit != q) {
                            // Deliver the non-Hangul text segment so far.
                            // We know there is an FCD boundary before the Hangul syllable.
                            limit = segmentLimit = q;
                            break;
                        }
                        segmentLimit = p;
                        // TODO: Create UBool ReorderingBuffer::setToDecomposedHangul(UChar32 c, UErrorCode &errorCode);
                        buffer.remove();
                        UChar jamos[3];
                        int32_t length = Hangul::decompose(c, jamos);
                        if(!buffer.appendZeroCC(jamos, jamos + length, errorCode)) { return U_SENTINEL; }
                        start = buffer.getStart();
                        limit = buffer.getLimit();
                        break;
                    } else if(U16_IS_LEAD(c) && p != rawLimit && U16_IS_TRAIL(*p)) {
                        // c is the lead surrogate of an inert supplementary code point.
                        ++p;
                    }
                }
                segmentLimit = p;
                prevCC = 0;
            } else {
                UChar c2;
                if(U16_IS_LEAD(c) && p != rawLimit && U16_IS_TRAIL(c2 = *p)) {
                    c = U16_GET_SUPPLEMENTARY(c, c2);
                    ++p;
                }
                uint16_t fcd16 = nfcImpl.getFCD16FromNormData(c);
                uint8_t leadCC = (uint8_t)(fcd16 >> 8);
                if(leadCC != 0 && prevCC > leadCC) {
                    // Fails FCD test.
                    if(limit != segmentLimit) {
                        // Deliver the already-FCD text segment so far.
                        limit = segmentLimit;
                        break;
                    }
                    // Find the next FCD boundary and normalize.
                    do {
                        segmentLimit = p;
                    } while(p != rawLimit && (fcd16 = nfcImpl.nextFCD16(p, rawLimit)) > 0xff);
                    buffer.remove();
                    nfcImpl.decompose(limit, segmentLimit, &buffer, errorCode);
                    if(U_FAILURE(errorCode)) { return U_SENTINEL; }
                    // Switch collation processing into the FCD buffer
                    // with the result of normalizing [limit, segmentLimit[.
                    start = buffer.getStart();
                    limit = buffer.getLimit();
                    break;
                }
                prevCC = (uint8_t)(fcd16 & 0xff);
                if(prevCC <= 1) {
                    segmentLimit = p;  // FCD boundary after the [q, p[ code point.
                } else if(leadCC == 0) {
                    segmentLimit = q;  // FCD boundary before the [q, p[ code point.
                }
            }
            if(p == rawLimit) {
                limit = segmentLimit = p;
                break;
            }
            if(smallSteps && (segmentLimit - limit) >= 5) {
                // Compromise: During string comparison, where differences are often
                // found after a few characters, we do not want to read ahead too far.
                // However, we do want to go forward several characters
                // which will then be handled in the base class fastpath.
                limit = segmentLimit;
                break;
            }
        }
        U_ASSERT(start < limit);
        if(lengthBeforeLimit != 0) {
            if(lengthBeforeLimit < (int32_t)(limit - start)) {
                limit = start + lengthBeforeLimit;
            }
        }
        pos = start;
        // Return the next code point at pos != limit; no need to check for NUL-termination.
        return simpleNext();
    }

    inline UChar32 simpleNext() {
        UChar32 c = *pos++;
        UChar trail;
        if(U16_IS_LEAD(c) && pos != limit && U16_IS_TRAIL(trail = *pos)) {
            ++pos;
            return U16_GET_SUPPLEMENTARY(c, trail);
        } else {
            return c;
        }
    }

    UChar32 nextCodePointDecompHangul(UErrorCode &errorCode) {
        // Only called from handleNextCodePoint() after checking for rawLimit etc.
        const UChar *p = segmentLimit;
        for(;;) {
            // So far, we have limit==segmentLimit<=p,
            // and [limit, p[ does not contain Hangul syllables.
            // Advance p by one code point and check for a Hangul syllable.
            UChar32 c = *p++;
            if(c < 0xac00) {
                if(c == 0 && rawLimit == NULL) {
                    // We hit the NUL terminator; remember its pointer.
                    segmentLimit = rawLimit == p - 1;
                    if(limit == rawLimit) { return U_SENTINEL; }
                    limit = rawLimit;
                    break;
                }
            } else if(c <= 0xd7a3) {
                if(limit != (p - 1)) {
                    // Deliver the non-Hangul text segment so far.
                    limit = segmentLimit = p - 1;
                    break;
                }
                segmentLimit = p;
                // TODO: Create UBool ReorderingBuffer::setToDecomposedHangul(UChar32 c, UErrorCode &errorCode);
                buffer.remove();
                UChar jamos[3];
                int32_t length = Hangul::decompose(c, jamos);
                if(!buffer.appendZeroCC(jamos, jamos + length, errorCode)) { return U_SENTINEL; }
                start = buffer.getStart();
                limit = buffer.getLimit();
                break;
            } else if(U16_IS_LEAD(c) && p != rawLimit && U16_IS_TRAIL(*p)) {
                // c is the lead surrogate of a supplementary code point.
                ++p;
            }
            if(p == rawLimit) {
                limit = segmentLimit = p;
                break;
            }
            if(smallSteps && (p - limit) >= 5) {
                // Compromise: During string comparison, where differences are often
                // found after a few characters, we do not want to read ahead too far.
                // However, we do want to go forward several characters
                // which will then be handled in the base class fastpath.
                limit = segmentLimit = p;
                break;
            }
        }
        U_ASSERT(start < limit);
        if(lengthBeforeLimit != 0) {
            if(lengthBeforeLimit < (int32_t)(limit - start)) {
                limit = start + lengthBeforeLimit;
            }
        }
        pos = start;
        // Return the next code point at pos != limit; no need to check for NUL-termination.
    }

    virtual UChar32 handlePreviousCodePoint(UErrorCode &errorCode) {
        if(U_FAILURE(errorCode) || segmentStart == rawStart) { return U_SENTINEL; }
        U_ASSERT(pos == start);
        if(start != segmentStart) {
            // The previous segment had to be normalized
            // and was pointing into the normalized string.
            start = pos = limit = segmentStart;
        }
        segmentLimit = segmentStart;
        if((flags & Collation::CHECK_FCD) == 0) {
            U_ASSERT((flags & Collation::DECOMP_HANGUL) != 0);
            return previousCodePointDecompHangul(errorCode);
        }
        const UChar *p = segmentStart;
        uint8_t nextCC = 0;
        for(;;) {
            // So far, we have p<=segmentStart<=start,
            // [p, start[ passes the FCD test,
            // and segmentStart is at the first FCD boundary on or after p.
            // Go back with p by one code point, fetch its fcd16 value,
            // and continue the incremental FCD test.
            const UChar *q = p;
            UChar32 c = *--p;
            uint16_t fcd16;
            if(c < 0x180) {
                fcd16 = nfcImpl.getFCD16FromBelow180(c);
            } else if(c < 0xac00) {
                if(!nfcImpl.singleLeadMightHaveNonZeroFCD16(c)) {
                    fcd16 = 0;
                } else {
                    fcd16 = nfcImpl.getFCD16FromNormData(c);
                }
            } else if(c <= 0xd7a3) {
                if(flags & Collation::DECOMP_HANGUL) {
                    if(start != q) {
                        // Deliver the non-Hangul text segment so far.
                        // We know there is an FCD boundary after the Hangul syllable.
                        start = segmentStart = q;
                        break;
                    }
                    segmentStart = p;
                    // TODO: Create UBool ReorderingBuffer::setToDecomposedHangul(UChar32 c, UErrorCode &errorCode);
                    buffer.remove();
                    UChar jamos[3];
                    int32_t length = Hangul::decompose(c, jamos);
                    if(!buffer.appendZeroCC(jamos, jamos + length, errorCode)) { return U_SENTINEL; }
                    start = buffer.getStart();
                    limit = buffer.getLimit();
                    break;
                } else {
                    fcd16 = 0;
                }
            } else {
                UChar c2;
                if(U16_IS_TRAIL(c) && p != rawStart && U16_IS_LEAD(c2 = *(p - 1))) {
                    c = U16_GET_SUPPLEMENTARY(c2, c);
                    --p;
                }
                fcd16 = nfcImpl.getFCD16FromNormData(c);
            }
            if(fcd16 == 0) {
                segmentStart = p;
                nextCC = 0;
            } else {
                uint8_t trailCC = (uint8_t)fcd16;
                if(nextCC != 0 && trailCC > nextCC) {
                    // Fails FCD test.
                    if(start != segmentStart) {
                        // Deliver the already-FCD text segment so far.
                        start = segmentStart;
                        break;
                    }
                    // Find the previous FCD boundary and normalize.
                    while(p != rawStart && (fcd16 = nfcImpl.previousFCD16(rawStart, p)) > 0xff) {}
                    segmentStart = p;
                    buffer.remove();
                    nfcImpl.decompose(segmentStart, start, &buffer, errorCode);
                    if(U_FAILURE(errorCode)) { return U_SENTINEL; }
                    // Switch collation processing into the FCD buffer
                    // with the result of normalizing [segmentStart, start[.
                    start = buffer.getStart();
                    limit = buffer.getLimit();
                    break;
                }
                nextCC = (uint8_t)(fcd16 >> 8);
                if(nextCC == 0) {
                    segmentStart = p;  // FCD boundary before the [p, q[ code point.
                }
            }
            if(p == rawStart) {
                start = segmentStart = p;
                break;
            }
            if((start - segmentStart) >= 8) {
                // Go back several characters at a time, for the base class fastpath.
                start = segmentStart;
                break;
            }
        }
        U_ASSERT(start < limit);
        if(lengthBeforeLimit != 0) {
            lengthBeforeLimit += (int32_t)(limit - start);
        }
        pos = limit;
        // Return the previous code point before pos != start.
        return simplePrevious();
    }

    inline UChar32 simplePrevious() {
        UChar32 c = *--pos;
        UChar lead;
        if(U16_IS_TRAIL(c) && pos != start && U16_IS_LEAD(lead = *(pos - 1))) {
            --pos;
            return U16_GET_SUPPLEMENTARY(lead, c);
        } else {
            return c;
        }
    }

    UChar32 previousCodePointDecompHangul(UErrorCode &errorCode) {
        // Only called from handleNextCodePoint() after checking for rawStart etc.
        const UChar *p = segmentStart;
        for(;;) {
            // So far, we have p<=segmentStart==start,
            // and [p, start[ does not contain Hangul syllables.
            // Go back with p by one code point and check for a Hangul syllable.
            UChar32 c = *--p;
            if(c < 0xac00) {
                // Nothing to be done.
            } else if(c <= 0xd7a3) {
                if(start != (p + 1)) {
                    // Deliver the non-Hangul text segment so far.
                    start = segmentStart = p + 1;
                    break;
                }
                segmentStart = p;
                // TODO: Create UBool ReorderingBuffer::setToDecomposedHangul(UChar32 c, UErrorCode &errorCode);
                buffer.remove();
                UChar jamos[3];
                int32_t length = Hangul::decompose(c, jamos);
                if(!buffer.appendZeroCC(jamos, jamos + length, errorCode)) { return U_SENTINEL; }
                start = buffer.getStart();
                limit = buffer.getLimit();
                break;
            } else {
                if(U16_IS_TRAIL(c) && p != rawStart && U16_IS_LEAD(*(p - 1))) {
                    --p;
                }
            }
            if(p == rawStart) {
                start = segmentStart = p;
                break;
            }
            if((start - p) >= 8) {
                // Go back several characters at a time, for the base class fastpath.
                start = segmentStart = p;
                break;
            }
        }
        U_ASSERT(start < limit);
        if(lengthBeforeLimit != 0) {
            lengthBeforeLimit += (int32_t)(limit - start);
        }
        pos = limit;
        // Return the previous code point before pos != start.
        return simplePrevious();
    }

    virtual const UChar *saveLimitAndSetAfter(UChar32 c) {
        limit = pos + U16_LENGTH(c);
        lengthBeforeLimit = (int32_t)(limit - start);
        return NULL;
    }

    virtual void restoreLimit(const UChar * /* savedLimit */) {
        if(start == segmentStart) {
            limit = segmentLimit;
        } else {
            limit = buffer.getLimit();
        }
    }

private:
    // Text pointers: The input text is [rawStart, rawLimit[
    // where rawLimit can be NULL for NUL-terminated text.
    // segmentStart and segmentLimit point into the text and indicate
    // the start and exclusive end of the text segment currently being processed.
    // They are at FCD boundaries.
    // Either the current text segment already passes the FCD test
    // and segmentStart==start<=pos<=limit==segmentLimit,
    // or the current segment had to be normalized so that
    // [segmentStart, segmentLimit[ turned into the normalized string,
    // corresponding to buffer.getStart()==start<=pos<=limit==buffer.getLimit().
    const UChar *rawStart;
    const UChar *segmentStart;
    const UChar *segmentLimit;
    // rawLimit==NULL for a NUL-terminated string.
    const UChar *rawLimit;
    // Normally zero.
    // Between calls to saveLimitAndSetAfter() and restoreLimit(),
    // it tracks the positive number of normalized UChars
    // between the start pointer and the temporary iteration limit.
    int32_t lengthBeforeLimit;
    // We make small steps for string comparisons and larger steps for sort key generation.
    UBool smallSteps;
    UnicodeString normalized;
    ReorderingBuffer buffer;
};

/**
 * Collation element and character iterator.
 * This iterator delegates to another CollationIterator
 * for forward iteration and character iteration,
 * and adds API for backward iteration.
 */
class U_I18N_API TwoWayCollationIterator : public UObject {
public:
    TwoWayCollationIterator(CollationIterator &iter) : fwd(iter) {}

    // TODO: So far, this is just the initial code collection moved out of the
    // base CollationIterator.
    // Add delegation to "fwd." etc.
    // Move this implementation into a separate .cpp file.

    int64_t nextCE(UErrorCode &errorCode) {
        return fwd.nextCE(errorCode);
    }
    // TODO: Jump by delta code points if direction changed? (See ICU ticket #9104.)
    // If so, then copy nextCE() to a not-inline slowNextCE()
    // which keeps track of the text movements together with previousCE()
    // and is used by the CollationElementIterator.
    // Keep the normal, inline nextCE() maximally fast and efficient.

    /**
     * Returns the previous collation element.
     */
    int64_t previousCE(UErrorCode &errorCode) {
        if(cesIndex > 0) {
            // Return the previous buffered CE.
            int64_t ce = ces[--cesIndex];
            if(cesIndex == 0) { cesIndex = -1; }
            // TODO: Jump by delta code points if the direction changed?
            return ce;
        }
        // TODO: Do we need to handle hiragana going backward?
        // Note: v1 ucol_IGetPrevCE() does not handle 3099..309C inheriting the Hiragana-ness from the preceding character.
        UChar32 c;
        uint32_t ce32;
        if(pos != start) {
            UTRIE2_U16_PREV32(trie, start, pos, c, ce32);
        } else {
            c = handlePreviousCodePoint(errorCode);
            if(c < 0) {
                return Collation::NO_CE;
            }
            ce32 = data->getCE32(c);
        }
        if(data->isUnsafeBackward(c)) {
            return previousCEUnsafe(c);
        }
        // Simple, safe-backwards iteration:
        // Get a CE going backwards, handle prefixes but no contractions.
        int64_t ce;
        if(ce32 < Collation::MIN_SPECIAL_CE32) {  // Forced-inline of isSpecialCE32(ce32).
            // Normal CE from the main data.
            ce = Collation::ceFromCE32(ce32);
        }
        const CollationData *d;
        if(ce32 == Collation::MIN_SPECIAL_CE32) {
            d = data->getBase();
            ce32 = d->getCE32(c);
            if(!Collation::isSpecialCE32(ce32)) {
                // Normal CE from the base data.
                return Collation::ceFromCE32(ce32);
            }
        } else {
            d = data;
        }
        return previousCEFromSpecialCE32(d, c, ce32, errorCode);
    }

private:
    int64_t previousCEFromSpecialCE32(const CollationData *d, UChar32 c, uint32_t ce32,
                                      UErrorCode &errorCode) const {
        for(;;) {  // Loop while ce32 is special.
            int32_t tag = Collation::getSpecialCE32Tag(ce32);
            if(tag <= Collation::MAX_LATIN_EXPANSION_TAG) {
                U_ASSERT(ce32 != MIN_SPECIAL_CE32);
                setLatinExpansion(ce32);
                return ces[1];
            }
            switch(tag) {
            case Collation::EXPANSION32_TAG:
                setCE32s(d, (ce32 >> 4) & 0xffff, (int32_t)ce32 & 0xf);
                cesIndex = cesMaxIndex;
                return ces[cesMaxIndex];
            case Collation::EXPANSION_TAG:
                ces = d->getCEs((ce32 >> 4) & 0xffff);
                cesMaxIndex = (int32_t)ce32 & 0xf;
                cesIndex = cesMaxIndex;
                return ces[cesMaxIndex];
            case Collation::PREFIX_TAG:
                ce32 = getCE32FromPrefix(d, ce32, errorCode);
                break;
            case Collation::CONTRACTION_TAG:
                // Must not occur. Backward contractions are handled by previousCEUnsafe().
                if(U_SUCCESS(errorCode)) { errorCode = U_INTERNAL_PROGRAM_ERROR; }
                return 0;
            case Collation::BUILDER_CONTEXT_TAG:
                ce32 = getCE32FromBuilderContext(ce32, errorCode);
                break;
            case Collation::DIGIT_TAG:
                if(flags & Collation::CODAN) {
                    // Collect digits, omit leading zeros.
                    CharString digits;
                    int32_t numLeadingZeros = 0;
                    for(;;) {
                        char digit = (char)(ce32 & 0xf);
                        if(digit == 0) {
                            ++numLeadingZeros;
                        } else {
                            numLeadingZeros = 0;
                        }
                        digits.append(digit, errorCode);
                        c = previousCodePoint(errorCode);
                        if(c < 0) { break; }
                        ce32 = data->getCE32(c);
                        if(ce32 == Collation::MIN_SPECIAL_CE32) {
                            ce32 = data->getBase()->getCE32(c);
                        }
                        if(!Collation::isSpecialCE32(ce32) ||
                            Collation::DIGIT_TAG != Collation::getSpecialCE32Tag(ce32)
                        ) {
                            forwardNumCodePoints(1, errorCode);
                            break;
                        }
                    }
                    int32_t length = digits.length() - numLeadingZeros;
                    if(length == 0) {
                        // A string of only "leading" zeros.
                        length = 1;
                    }
                    // Reverse the digit string.
                    char *p = digits.data();
                    char *q = p + length - 1;
                    while(p < q) {
                        char digit = *p;
                        *p++ = *q;
                        *q-- = digit;
                    }
                    setCodanCEs(digits.data(), length, errorCode);
                    cesIndex = cesMaxIndex;
                    return ces[cesMaxIndex];
                } else {
                    // Fetch the non-CODAN CE32 and continue.
                    ce32 = *d->getCE32s((ce32 >> 4) & 0xffff);
                    break;
                }
            case Collation::HIRAGANA_TAG:
                // TODO: Do we need to handle hiragana going backward? (I.e., do string search or the CollationElementIterator need it?)
                // Fetch the normal CE32 and continue.
                ce32 = *d->getCE32s(ce32 & 0xfffff);
                break;
            case Collation::HANGUL_TAG:
                setHangulExpansion(ce32, errorCode);
                cesIndex = cesMaxIndex;
                return ces[cesMaxIndex];
            case Collation::OFFSET_TAG:
                return getCEFromOffsetCE32(d, c, ce32);
            case Collation::IMPLICIT_TAG:
                if((ce32 & 1) == 0) {
                    U_ASSERT(c == 0);
                    // Fetch the normal ce32 for U+0000 and continue.
                    ce32 = *d->getCE32s(0);
                    break;
                } else {
                    return Collation::unassignedCEFromCodePoint(c);
                }
            }
            if(!Collation::isSpecialCE32(ce32)) {
                return Collation::ceFromCE32(ce32);
            }
        }
    }

    /**
     * Returns the previous CE when data->isUnsafeBackward(c).
     */
    int64_t previousCEUnsafe(UChar32 c, UErrorCode &errorCode) {
        // We just move through the input counting safe and unsafe code points
        // without collecting the unsafe-backward substring into a buffer and
        // switching to it.
        // This is to keep the logic simple. Otherwise we would have to handle
        // prefix matching going before the backward buffer, switching
        // to iteration and back, etc.
        // In the most important case of iterating over a normal string,
        // reading from the string itself is already maximally fast.
        // The only drawback there is that after getting the CEs we always
        // skip backward to the safe character rather than switching out
        // of a backwardBuffer.
        // But this should not be the common case for previousCE(),
        // and correctness and maintainability are more important than
        // complex optimizations.
        // TODO: Verify that the v1 code uses a backward buffer and gets into trouble
        // with a prefix match that would need to move before the backward buffer.
        const UChar *savedLimit = saveLimitAndSetAfter(c);
        // Find the first safe character before c.
        int32_t numBackward = 1;
        while(c = previousCodePoint(errorCode) >= 0) {
            ++numBackward;
            if(!data->isUnsafeBackward(c)) {
                break;
            }
        }
        // Ensure that we don't see CEs from a later-in-text expansion.
        cesIndex = -1;
        // Go forward and collect the CEs.
        int32_t cesLength = 0;
        int64_t ce;
        while((ce = nextCE(errorCode)) != Collation::NO_CE) {
            cesLength = backwardCEs.append(cesLength, ce, errorCode);
        }
        restoreLimit(savedLimit);
        backwardNumCodePoints(numBackward, errorCode);
        // Use the collected CEs and return the last one.
        U_ASSERT(0 != cesLength);
        ces = backwardCEs.getBuffer();
        cesIndex = cesMaxIndex = cesLength - 1;
        return ces[cesIndex];
        // TODO: Does this method deliver backward-iteration offsets tight enough
        // for string search? Is this equivalent to how v1 behaves?
    }

    CollationIterator &fwd;

    // 64-bit-CE buffer for unsafe-backward iteration.
    CEBuffer backwardCEs;
};

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
#endif  // __COLLATIONITERATOR_H__
