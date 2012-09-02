/*
*******************************************************************************
* Copyright (C) 1996-2012, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* rulebasedcollator.cpp
*
* created on: 2012feb14 with new and old collation code
* created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/locid.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/ucol.h"
#include "cmemory.h"
#include "collation.h"
#include "collationcompare.h"
#include "collationdata.h"
#include "collationiterator.h"
#include "rulebasedcollator.h"
#include "uassert.h"
#include "utf16collationiterator.h"

U_NAMESPACE_BEGIN

// TODO: Add UTRACE_... calls back in.

RuleBasedCollator2::~RuleBasedCollator2() {
    delete ownedData;
    uprv_free(ownedReorderTable);
    uprv_free(ownedReorderCodes);
}

Collator *
RuleBasedCollator2::clone() const {
    // TODO: new RuleBasedCollator2(), copyFrom(*this, UErrorCode), delete & return NULL if U_FAILURE
    return NULL;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedCollator2)

UBool
RuleBasedCollator2::operator==(const Collator& other) const {
    return FALSE;  // TODO
}

int32_t
RuleBasedCollator2::hashCode() const {
    return 0;  // TODO
}

Locale
RuleBasedCollator2::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    return Locale::getDefault();  // TODO
}

// TODO: const UnicodeString& getRules(void) const;
// TODO: void getRules(UColRuleOption delta, UnicodeString &buffer);

// TODO: uint8_t *cloneRuleData(int32_t &length, UErrorCode &status);

// TODO: int32_t cloneBinary(uint8_t *buffer, int32_t capacity, UErrorCode &status);

void
RuleBasedCollator2::getVersion(UVersionInfo info) const {
    // TODO
}

// TODO: int32_t getMaxExpansion(int32_t order) const;

UnicodeSet *
RuleBasedCollator2::getTailoredSet(UErrorCode &status) const {
    return NULL;  // TODO
}

UBool
RuleBasedCollator2::ensureOwnedData(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return FALSE; }
    if(ownedData != NULL) { return TRUE; }
    ownedData = new CollationData(*defaultData);
    if(ownedData == NULL) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    data = ownedData;
    return TRUE;
}

UColAttributeValue
RuleBasedCollator2::getAttribute(UColAttribute attr, UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode)) { return UCOL_DEFAULT; }
    int32_t option;
    switch(attr) {
    case UCOL_FRENCH_COLLATION:
        option = CollationData::BACKWARD_SECONDARY;
        break;
    case UCOL_ALTERNATE_HANDLING:
        return data->getAlternateHandling();
    case UCOL_CASE_FIRST:
        return data->getCaseFirst();
    case UCOL_CASE_LEVEL:
        option = CollationData::CASE_LEVEL;
        break;
    case UCOL_NORMALIZATION_MODE:
        option = CollationData::CHECK_FCD;
        break;
    case UCOL_STRENGTH:
        return (UColAttributeValue)data->getStrength();
    case UCOL_HIRAGANA_QUATERNARY_MODE:
        // Deprecated attribute, unsettable.
        return UCOL_OFF;
    case UCOL_NUMERIC_COLLATION:
        option = CollationData::CODAN;
        break;
    default:
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return UCOL_DEFAULT;
    }
    return ((data->options & option) == 0) ? UCOL_OFF : UCOL_ON;
}

void
RuleBasedCollator2::setAttribute(UColAttribute attr, UColAttributeValue value,
                                 UErrorCode &errorCode) {
    UColAttributeValue oldValue = getAttribute(attr, errorCode);
    if(U_FAILURE(errorCode)) { return; }
    if(value == oldValue) {
        setAttributeExplicitly(attr);
        return;
    }
    if(ownedData == NULL) {
        if(value == UCOL_DEFAULT) {
            setAttributeDefault(attr);
            return;
        }
        if(!ensureOwnedData(errorCode)) { return; }
    }

    switch(attr) {
    case UCOL_FRENCH_COLLATION:
        ownedData->setFlag(CollationData::BACKWARD_SECONDARY, value, defaultData->options, errorCode);
        break;
    case UCOL_ALTERNATE_HANDLING:
        ownedData->setAlternateHandling(value, defaultData->options, errorCode);
        break;
    case UCOL_CASE_FIRST:
        ownedData->setCaseFirst(value, defaultData->options, errorCode);
        break;
    case UCOL_CASE_LEVEL:
        ownedData->setFlag(CollationData::CASE_LEVEL, value, defaultData->options, errorCode);
        break;
    case UCOL_NORMALIZATION_MODE:
        ownedData->setFlag(CollationData::CHECK_FCD, value, defaultData->options, errorCode);
        break;
    case UCOL_STRENGTH:
        ownedData->setStrength(value, defaultData->options, errorCode);
        break;
    case UCOL_HIRAGANA_QUATERNARY_MODE:
        // Deprecated attribute. Check for valid values but do not change anything.
        if(value != UCOL_OFF && value != UCOL_ON && value != UCOL_DEFAULT) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case UCOL_NUMERIC_COLLATION:
        ownedData->setFlag(CollationData::CODAN, value, defaultData->options, errorCode);
        break;
    default:
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }
    if(U_FAILURE(errorCode)) { return; }
    if(value == UCOL_DEFAULT) {
        setAttributeDefault(attr);
    } else {
        setAttributeExplicitly(attr);
    }
}

uint32_t
RuleBasedCollator2::getVariableTop(UErrorCode &errorCode) const {
    return data->variableTop;
}

uint32_t
RuleBasedCollator2::setVariableTop(const UChar *varTop, int32_t len, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    if(varTop == NULL && len !=0) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(len < 0) { len = u_strlen(varTop); }
    if(len == 0) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    int64_t ce1, ce2;
    if((data->options & CollationData::CHECK_FCD) == 0) {
        UTF16CollationIterator ci(data, varTop, varTop + len);
        ce1 = ci.nextCE(errorCode);
        ce2 = ci.nextCE(errorCode);
    } else {
        FCDUTF16CollationIterator ci(data, varTop, varTop + len, errorCode);
        ce1 = ci.nextCE(errorCode);
        ce2 = ci.nextCE(errorCode);
    }
    if(ce1 == Collation::NO_CE || ce2 != Collation::NO_CE) {
        errorCode = U_CE_NOT_FOUND_ERROR;
        return 0;
    }
    setVariableTop((uint32_t)(ce1 >> 32), errorCode);
    return data->variableTop;
}

uint32_t
RuleBasedCollator2::setVariableTop(const UnicodeString &varTop, UErrorCode &errorCode) {
    return setVariableTop(varTop.getBuffer(), varTop.length(), errorCode);
}

void
RuleBasedCollator2::setVariableTop(uint32_t varTop, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode) || varTop == data->variableTop) { return; }
    if(!ensureOwnedData(errorCode)) { return; }
    ownedData->variableTop = varTop;
    if(varTop == defaultData->variableTop) {
        setAttributeDefault(ATTR_VARIABLE_TOP);
    } else {
        setAttributeExplicitly(ATTR_VARIABLE_TOP);
    }
}

// TODO: hack for prototyping, remove
static const CollationBaseData *gBaseData = NULL;

U_CAPI void U_EXPORT2 ucol_setCollationBaseData(const CollationBaseData *base) {
    gBaseData = base;
}

static const CollationBaseData *ucol_getCollationBaseData(UErrorCode & /*errorCode*/) { return gBaseData; }  // TODO

int32_t
RuleBasedCollator2::getReorderCodes(int32_t *dest, int32_t capacity,
                                    UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode)) { return 0; }
    if(capacity < 0 || (dest == NULL && capacity > 0)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    int32_t length = data->reorderCodesLength;
    if(length == 0) { return 0; }
    if(length > capacity) {
        errorCode = U_BUFFER_OVERFLOW_ERROR;
        return length;
    }
    uprv_memcpy(dest, data->reorderCodes, length * 4);
    return length;
}

void
RuleBasedCollator2::setReorderCodes(const int32_t *reorderCodes, int32_t length,
                                    UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    if(length < 0 || (reorderCodes == NULL && length > 0)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(length == 1 && reorderCodes[0] == UCOL_REORDER_CODE_DEFAULT) {
        if(ownedData != NULL) {
            ownedData->reorderTable = defaultData->reorderTable;
            ownedData->reorderCodes = defaultData->reorderCodes;
            ownedData->reorderCodesLength = defaultData->reorderCodesLength;
        }
        return;
    }
    if(!ensureOwnedData(errorCode)) { return; }
    if(ownedReorderTable == NULL) {
        ownedReorderTable = (uint8_t *)uprv_malloc(256);
        if(ownedReorderTable == NULL) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }
    const CollationBaseData *baseData = ucol_getCollationBaseData(errorCode);
    baseData->makeReorderTable(reorderCodes, length, ownedReorderTable, errorCode);
    if(U_FAILURE(errorCode)) { return; }
    if(length > ownedReorderCodesCapacity) {
        int32_t newCapacity = length + 20;
        int32_t *newReorderCodes = (int32_t *)uprv_malloc(newCapacity * 4);
        if(newReorderCodes == NULL) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        uprv_free(ownedReorderCodes);
        ownedReorderCodes = newReorderCodes;
        ownedReorderCodesCapacity = newCapacity;
    }
    uprv_memcpy(ownedReorderCodes, reorderCodes, length * 4);
    ownedData->reorderTable = ownedReorderTable;
    ownedData->reorderCodes = ownedReorderCodes;
    ownedData->reorderCodesLength = length;
}

int32_t
RuleBasedCollator2::getEquivalentReorderCodes(int32_t reorderCode,
                                              int32_t *dest, int32_t capacity,
                                              UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    if(capacity < 0 || (dest == NULL && capacity > 0)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    const CollationBaseData *baseData = ucol_getCollationBaseData(errorCode);
    if(U_FAILURE(errorCode)) { return 0; }
    return baseData->getEquivalentScripts(reorderCode, dest, capacity, errorCode);
}

UCollationResult
RuleBasedCollator2::compare(const UnicodeString &left, const UnicodeString &right,
                            UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode)) { return UCOL_EQUAL; }
    return doCompare(left.getBuffer(), left.length(),
                     right.getBuffer(), right.length(), errorCode);
}

UCollationResult
RuleBasedCollator2::compare(const UnicodeString &left, const UnicodeString &right,
                            int32_t length, UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode) || length == 0) { return UCOL_EQUAL; }
    if(length < 0) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return UCOL_EQUAL;
    }
    int32_t leftLength = left.length();
    int32_t rightLength = right.length();
    if(leftLength > length) { leftLength = length; }
    if(rightLength > length) { rightLength = length; }
    return doCompare(left.getBuffer(), leftLength,
                     right.getBuffer(), rightLength, errorCode);
}

UCollationResult
RuleBasedCollator2::compare(const UChar *left, int32_t leftLength,
                            const UChar *right, int32_t rightLength,
                            UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode)) { return UCOL_EQUAL; }
    if((left == NULL && leftLength != 0) || (right == NULL && rightLength != 0)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return UCOL_EQUAL;
    }
    // Make sure both or neither strings have a known length.
    // We do not optimize for mixed length/termination.
    if(leftLength >= 0) {
        if(rightLength < 0) { rightLength = u_strlen(right); }
    } else {
        if(rightLength >= 0) { leftLength = u_strlen(left); }
    }
    return doCompare(left, leftLength, right, rightLength, errorCode);
}

static int32_t
findEndOrFFFE(const UChar *s, int32_t i, int32_t &length) {
    if(length >= 0) {
        while(i < length && s[i] != 0xfffe) { ++i; }
    } else {
        // s is NUL-terminated.
        UChar c;
        while((c = s[i]) != 0xfffe) {
            if(c == 0) {
                length = i;
                break;
            }
            ++i;
        }
    }
    return i;
}

UCollationResult
RuleBasedCollator2::doCompare(const UChar *left, int32_t leftLength,
                              const UChar *right, int32_t rightLength,
                              UErrorCode &errorCode) const {
    // U_FAILURE(errorCode) checked by caller.
    if(left == right && leftLength == rightLength) {
        return UCOL_EQUAL;
    }

    const UChar *leftLimit;
    const UChar *rightLimit;
    int32_t equalPrefixLength = 0;  // Identical-prefix test.
    if(leftLength < 0) {
        leftLimit = NULL;
        rightLimit = NULL;
        UChar c;
        while((c = left[equalPrefixLength]) == right[equalPrefixLength]) {
            if(c == 0) { return UCOL_EQUAL; }
            ++equalPrefixLength;
        }
    } else {
        leftLimit = left + leftLength;
        rightLimit = right + rightLength;
        for(;;) {
            if(equalPrefixLength == leftLength) {
                if(equalPrefixLength == rightLength) { return UCOL_EQUAL; }
                break;
            } else if(equalPrefixLength == rightLength ||
                      left[equalPrefixLength] != right[equalPrefixLength]) {
                break;
            }
            ++equalPrefixLength;
        }
    }

    // Identical prefix: Back up to the start of a contraction or reordering sequence.
    if(equalPrefixLength > 0) {
        if((equalPrefixLength != leftLength && data->isUnsafeBackward(left[equalPrefixLength])) ||
                (equalPrefixLength != rightLength && data->isUnsafeBackward(right[equalPrefixLength]))) {
            while(--equalPrefixLength > 0 && data->isUnsafeBackward(left[equalPrefixLength])) {}
        }
        left += equalPrefixLength;
        right += equalPrefixLength;
    }

    UCollationResult result;
    if((data->options & CollationData::CHECK_FCD) == 0) {
        UTF16CollationIterator leftIter(data, left, leftLimit);
        UTF16CollationIterator rightIter(data, right, rightLimit);
        result = CollationCompare::compareUpToQuaternary(leftIter, rightIter, errorCode);
    } else {
        FCDUTF16CollationIterator leftIter(data, left, leftLimit, errorCode);
        FCDUTF16CollationIterator rightIter(data, right, rightLimit, errorCode);
        result = CollationCompare::compareUpToQuaternary(leftIter, rightIter, errorCode);
    }
    if(result != UCOL_EQUAL || data->getStrength() < UCOL_IDENTICAL || U_FAILURE(errorCode)) {
        return result;
    }

    // TODO: If NUL-terminated, get actual limits from iterators?

    if(leftLength > 0) {
        leftLength -= equalPrefixLength;
        rightLength -= equalPrefixLength;
    }

    uint32_t options = U_COMPARE_CODE_POINT_ORDER;
    if((data->options & CollationData::CHECK_FCD) == 0) {
        options |= UNORM_INPUT_IS_FCD;
    }
    // Treat U+FFFE as a merge separator.
    // TODO: Test this comparing ab*cde vs. abc*d where *=U+FFFE, all others are ignorable, and c<U+FFFE.
    // TODO: When generating sort keys, write MERGE_SEPARATOR_BYTE for U+FFFE.
    int32_t leftStart = 0;
    int32_t rightStart = 0;
    for(;;) {
        int32_t leftEnd = findEndOrFFFE(left, leftStart, leftLength);
        int32_t rightEnd = findEndOrFFFE(right, rightStart, rightLength);
        // Compare substrings between U+FFFEs.
        int32_t intResult = unorm_compare(left + leftStart, leftEnd - leftStart,
                                          right + rightStart, rightEnd - rightStart,
                                          options, &errorCode);
        if(intResult != 0) {
            return (intResult < 0) ? UCOL_LESS : UCOL_GREATER;
        }
        // Did we reach the end of either string?
        // Both strings have the same number of merge separators,
        // or else there would have been a primary-level difference.
        U_ASSERT((leftEnd == leftLength) == (rightEnd == rightLength));
        if(leftEnd == leftLength) {
            return UCOL_EQUAL;
        }
        // Skip both U+FFFEs and continue.
        leftStart = leftEnd + 1;
        rightStart = rightEnd + 1;
    }
}

CollationKey &
RuleBasedCollator2::getCollationKey(const UnicodeString &source,
                                    CollationKey &key,
                                    UErrorCode &errorCode) const {
    return key;  // TODO
}

CollationKey &
RuleBasedCollator2::getCollationKey(const UChar *source, int32_t sourceLength,
                                    CollationKey& key,
                                    UErrorCode &errorCode) const {
    return key;  // TODO
}

int32_t
RuleBasedCollator2::getSortKey(const UnicodeString &source, uint8_t *result,
                               int32_t resultLength) const {
    if(resultLength > 0) { *result = 0; }
    return 0;  // TODO
}

int32_t
RuleBasedCollator2::getSortKey(const UChar *source, int32_t sourceLength,
                               uint8_t *result, int32_t resultLength) const {
    if(resultLength > 0) { *result = 0; }
    return 0;  // TODO
}

CollationElementIterator *
RuleBasedCollator2::createCollationElementIterator(const UnicodeString& source) const {
    return NULL;  // TODO
}

CollationElementIterator *
RuleBasedCollator2::createCollationElementIterator(const CharacterIterator& source) const {
    return NULL;  // TODO
}

// TODO: For sort key generation, an iterator like QuaternaryIterator might be useful.
// Public fields p, s, t, q for the result weights. UBool next().
// Should help share code between normal & partial sortkey generation.

// TODO: If we generate a Latin-1 sort key table like in v1,
// then make sure to exclude contractions that contain non-Latin-1 characters.
// (For example, decomposed versions of Latin-1 composites.)
// (Unless there is special, clever contraction-matching code that can consume them.)

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
