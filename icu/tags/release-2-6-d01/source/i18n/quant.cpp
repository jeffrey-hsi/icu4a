/*
* Copyright (C) 2001, International Business Machines Corporation and others. All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   07/26/01    aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "quant.h"
#include "unicode/unistr.h"
#include "util.h"

U_NAMESPACE_BEGIN

const char Quantifier::fgClassID=0;

Quantifier::Quantifier(UnicodeFunctor *adoptedMatcher,
                       uint32_t _minCount, uint32_t _maxCount) {
    // assert(adopted != 0);
    // assert(minCount <= maxCount);
    matcher = adoptedMatcher;
    this->minCount = _minCount;
    this->maxCount = _maxCount;
}

Quantifier::Quantifier(const Quantifier& o) :
    UnicodeFunctor(o),
    matcher(o.matcher->clone()),
    minCount(o.minCount),
    maxCount(o.maxCount)
{
}

Quantifier::~Quantifier() {
    delete matcher;
}

/**
 * Implement UnicodeFunctor
 */
UnicodeFunctor* Quantifier::clone() const {
    return new Quantifier(*this);
}

/**
 * UnicodeFunctor API.  Cast 'this' to a UnicodeMatcher* pointer
 * and return the pointer.
 */
UnicodeMatcher* Quantifier::toMatcher() const {
    return (UnicodeMatcher*) this;
}

UMatchDegree Quantifier::matches(const Replaceable& text,
                                 int32_t& offset,
                                 int32_t limit,
                                 UBool incremental) {
    int32_t start = offset;
    uint32_t count = 0;
    while (count < maxCount) {
        int32_t pos = offset;
        UMatchDegree m = matcher->toMatcher()->matches(text, offset, limit, incremental);
        if (m == U_MATCH) {
            ++count;
            if (pos == offset) {
                // If offset has not moved we have a zero-width match.
                // Don't keep matching it infinitely.
                break;
            }
        } else if (incremental && m == U_PARTIAL_MATCH) {
            return U_PARTIAL_MATCH;
        } else {
            break;
        }
    }
    if (incremental && offset == limit) {
        return U_PARTIAL_MATCH;
    }
    if (count >= minCount) {
        return U_MATCH;
    }
    offset = start;
    return U_MISMATCH;
}

/**
 * Implement UnicodeMatcher
 */
UnicodeString& Quantifier::toPattern(UnicodeString& result,
                                     UBool escapeUnprintable) const {
    result.truncate(0);
    matcher->toMatcher()->toPattern(result, escapeUnprintable);
    if (minCount == 0) {
        if (maxCount == 1) {
            return result.append((UChar)63); /*?*/
        } else if (maxCount == MAX) {
            return result.append((UChar)42); /***/
        }
        // else fall through
    } else if (minCount == 1 && maxCount == MAX) {
        return result.append((UChar)43); /*+*/
    }
    result.append((UChar)123); /*{*/
    ICU_Utility::appendNumber(result, minCount);
    result.append((UChar)44); /*,*/
    if (maxCount != MAX) {
        ICU_Utility::appendNumber(result, maxCount);
    }
    result.append((UChar)125); /*}*/
    return result;
}

/**
 * Implement UnicodeMatcher
 */
UBool Quantifier::matchesIndexValue(uint8_t v) const {
    return (minCount == 0) || matcher->toMatcher()->matchesIndexValue(v);
}

/**
 * Implement UnicodeMatcher
 */
void Quantifier::addMatchSetTo(UnicodeSet& toUnionTo) const {
    if (maxCount > 0) {
	matcher->toMatcher()->addMatchSetTo(toUnionTo);
    }
}

/**
 * Implement UnicodeFunctor
 */
void Quantifier::setData(const TransliterationRuleData* d) {
    matcher->setData(d);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

//eof
