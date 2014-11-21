/*
******************************************************************************
* Copyright (C) 2014, International Business Machines
* Corporation and others.  All Rights Reserved.
******************************************************************************
* simplepatternformatter.cpp
*/
#include "simplepatternformatter.h"
#include "cstring.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

static UBool isInvalidArray(const void *array, int32_t size) {
   return (size < 0 || (size > 0 && array == NULL));
}

typedef enum SimplePatternFormatterCompileState {
    INIT,
    APOSTROPHE,
    PLACEHOLDER
} SimplePatternFormatterCompileState;

// Handles parsing placeholders in the pattern string, e.g {4} or {35}
class SimplePatternFormatterIdBuilder {
public:
    SimplePatternFormatterIdBuilder() : id(0), idLen(0) { }
    ~SimplePatternFormatterIdBuilder() { }

    // Resets so that this object has seen no placeholder ID.
    void reset() { id = 0; idLen = 0; }

    // Returns the numeric placeholder ID parsed so far
    int32_t getId() const { return id; }

    // Appends the numeric placeholder ID parsed so far back to a
    // UChar buffer. Used to recover if parser using this object finds
    // no closing curly brace.
    void appendTo(UChar *buffer, int32_t *len) const;

    // Returns true if this object has seen a placeholder ID.
    UBool isValid() const { return (idLen > 0); }

    // Processes a single digit character. Pattern string parser calls this
    // as it processes digits after an opening curly brace.
    void add(UChar ch);
private:
    int32_t id;
    int32_t idLen;
    SimplePatternFormatterIdBuilder(
            const SimplePatternFormatterIdBuilder &other);
    SimplePatternFormatterIdBuilder &operator=(
            const SimplePatternFormatterIdBuilder &other);
};

void SimplePatternFormatterIdBuilder::appendTo(
        UChar *buffer, int32_t *len) const {
    int32_t origLen = *len;
    int32_t kId = id;
    for (int32_t i = origLen + idLen - 1; i >= origLen; i--) {
        int32_t digit = kId % 10;
        buffer[i] = digit + 0x30;
        kId /= 10;
    }
    *len = origLen + idLen;
}

void SimplePatternFormatterIdBuilder::add(UChar ch) {
    id = id * 10 + (ch - 0x30);
    idLen++;
}

// Creates fixed placeholder values given a set of original placeholder
// values. If a placeholder value is the same UnicodeString as builder,
// records a copy of builder for the corresponding fixed placeholder
// value. Otherwise, each fixed placeholder value is the same UnicodeString
// as the original placeholder value. If skipIndex is non negative, then
// the skipIndexth fixed placeholder value is always the same UnicodeString
// as the original even if it is the same as builder.
class SimplePatternFormatterFixedValues {
public:
    SimplePatternFormatterFixedValues(
        const UnicodeString &builder,
        const UnicodeString * const *values,
        int32_t valuesCount,
        int32_t skipIndex,
        UErrorCode &status);

    // Returns the fixed placeholder values. This object owns any
    // additional memory allocations needed to produce the fixed
    // placeholder values from the original ones. When this object goes
    // out of scope, the returned values are no longer valid.
    const UnicodeString * const *get() const { return fixedValues; }
    ~SimplePatternFormatterFixedValues();
private:
    const UnicodeString *fewNewValues[3];
    const UnicodeString **manyNewValues;
    const UnicodeString * const *fixedValues;
    UnicodeString builderCopy;
};

SimplePatternFormatterFixedValues::SimplePatternFormatterFixedValues(
        const UnicodeString &builder,
        const UnicodeString * const *values,
        int32_t valuesCount,
        int32_t skipIndex,
        UErrorCode &status) 
        : manyNewValues(NULL),
          fixedValues(values) {
    if (U_FAILURE(status)) {
        return;
    }
    for (int32_t i = 0; TRUE; ++i) {
        if (i == valuesCount) {
          return;
        }
        if (i != skipIndex && values[i] == &builder) {
            break;
        }
    }
    const UnicodeString **newValues;
    if (valuesCount <= UPRV_LENGTHOF(fewNewValues)) {
        newValues = fewNewValues;
    } else {
        manyNewValues = (const UnicodeString **) uprv_malloc(
            valuesCount * sizeof(const UnicodeString *));
        if (manyNewValues == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        newValues = manyNewValues;
    }
    builderCopy.append(builder);
    for (int32_t i = 0; i < valuesCount; ++i) {
        if (i != skipIndex && values[i] == &builder) {
            newValues[i] = &builderCopy;
        } else {
            newValues[i] = values[i];
        }
    }
    fixedValues = newValues;
}

SimplePatternFormatterFixedValues::~SimplePatternFormatterFixedValues() {
    uprv_free(manyNewValues);
}

SimplePatternFormatter::SimplePatternFormatter() :
        noPlaceholders(),
        placeholders(),
        placeholderSize(0),
        placeholderCount(0),
        firstPlaceholderReused(FALSE) {
}

SimplePatternFormatter::SimplePatternFormatter(const UnicodeString &pattern) :
        noPlaceholders(),
        placeholders(),
        placeholderSize(0),
        placeholderCount(0),
        firstPlaceholderReused(FALSE) {
    UErrorCode status = U_ZERO_ERROR;
    compile(pattern, status);
}

SimplePatternFormatter::SimplePatternFormatter(
        const SimplePatternFormatter &other) :
        noPlaceholders(other.noPlaceholders),
        placeholders(),
        placeholderSize(0),
        placeholderCount(other.placeholderCount),
        firstPlaceholderReused(other.firstPlaceholderReused) {
    placeholderSize = ensureCapacity(other.placeholderSize);
    uprv_memcpy(
            placeholders.getAlias(),
            other.placeholders.getAlias(),
            placeholderSize * sizeof(PlaceholderInfo));
}

SimplePatternFormatter &SimplePatternFormatter::operator=(
        const SimplePatternFormatter& other) {
    if (this == &other) {
        return *this;
    }
    noPlaceholders = other.noPlaceholders;
    placeholderSize = ensureCapacity(other.placeholderSize);
    placeholderCount = other.placeholderCount;
    firstPlaceholderReused = other.firstPlaceholderReused;
    uprv_memcpy(
            placeholders.getAlias(),
            other.placeholders.getAlias(),
            placeholderSize * sizeof(PlaceholderInfo));
    return *this;
}

SimplePatternFormatter::~SimplePatternFormatter() {
}

UBool SimplePatternFormatter::compile(
        const UnicodeString &pattern, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    const UChar *patternBuffer = pattern.getBuffer();
    int32_t patternLength = pattern.length();
    UChar *buffer = noPlaceholders.getBuffer(patternLength);
    int32_t len = 0;
    placeholderSize = 0;
    placeholderCount = 0;
    SimplePatternFormatterCompileState state = INIT;
    SimplePatternFormatterIdBuilder idBuilder;
    for (int32_t i = 0; i < patternLength; ++i) {
        UChar ch = patternBuffer[i];
        switch (state) {
        case INIT:
            if (ch == 0x27) {
                state = APOSTROPHE;
            } else if (ch == 0x7B) {
                state = PLACEHOLDER;
                idBuilder.reset();
            } else {
               buffer[len++] = ch;
            }
            break;
        case APOSTROPHE:
            if (ch == 0x27) {
                buffer[len++] = 0x27;
            } else if (ch == 0x7B) {
                buffer[len++] = 0x7B;
            } else {
                buffer[len++] = 0x27;
                buffer[len++] = ch;
            }
            state = INIT;
            break;
        case PLACEHOLDER:
            if (ch >= 0x30 && ch <= 0x39) {
                idBuilder.add(ch);
            } else if (ch == 0x7D && idBuilder.isValid()) {
                if (!addPlaceholder(idBuilder.getId(), len)) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return FALSE;
                }
                state = INIT;
            } else {
                buffer[len++] = 0x7B;
                idBuilder.appendTo(buffer, &len);
                buffer[len++] = ch;
                state = INIT;
            }
            break;
        default:
            U_ASSERT(FALSE);
            break;
        }
    }
    switch (state) {
    case INIT:
        break;
    case APOSTROPHE:
        buffer[len++] = 0x27;
        break;
    case PLACEHOLDER:
        buffer[len++] = 0X7B;
        idBuilder.appendTo(buffer, &len);
        break;
    default:
        U_ASSERT(false);
        break;
    }
    noPlaceholders.releaseBuffer(len);
    return TRUE;
}

UnicodeString& SimplePatternFormatter::format(
        const UnicodeString &arg0,
        UnicodeString &appendTo,
        UErrorCode &status) const {
    const UnicodeString *params[] = {&arg0};
    return formatAndAppend(
            params,
            UPRV_LENGTHOF(params),
            appendTo,
            NULL,
            0,
            status);
}

UnicodeString& SimplePatternFormatter::format(
        const UnicodeString &arg0,
        const UnicodeString &arg1,
        UnicodeString &appendTo,
        UErrorCode &status) const {
    const UnicodeString *params[] = {&arg0, &arg1};
    return formatAndAppend(
            params,
            UPRV_LENGTHOF(params),
            appendTo,
            NULL,
            0,
            status);
}

UnicodeString& SimplePatternFormatter::format(
        const UnicodeString &arg0,
        const UnicodeString &arg1,
        const UnicodeString &arg2,
        UnicodeString &appendTo,
        UErrorCode &status) const {
    const UnicodeString *params[] = {&arg0, &arg1, &arg2};
    return formatAndAppend(
            params,
            UPRV_LENGTHOF(params),
            appendTo,
            NULL,
            0,
            status);
}

static void updatePlaceholderOffset(
        int32_t placeholderId,
        int32_t placeholderOffset,
        int32_t *offsetArray,
        int32_t offsetArrayLength) {
    if (placeholderId < offsetArrayLength) {
        offsetArray[placeholderId] = placeholderOffset;
    }
}

static void appendRange(
        const UnicodeString &src,
        int32_t start,
        int32_t end,
        UnicodeString &dest) {
    // This check improves performance significantly.
    if (start == end) {
        return;
    }
    dest.append(src, start, end - start);
}

UnicodeString& SimplePatternFormatter::formatAndAppend(
        const UnicodeString * const *placeholderValues,
        int32_t placeholderValueCount,
        UnicodeString &appendTo,
        int32_t *offsetArray,
        int32_t offsetArrayLength,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    if (isInvalidArray(placeholderValues, placeholderValueCount)
            || isInvalidArray(offsetArray, offsetArrayLength)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    if (placeholderValueCount < placeholderCount) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    SimplePatternFormatterFixedValues fixedValues(
            appendTo, placeholderValues, placeholderCount, -1, status);
    if (U_FAILURE(status)) {
        return appendTo;
    }
    return formatAndAppendNoFixValues(
            fixedValues.get(),
            appendTo,
            offsetArray,
            offsetArrayLength);
}

UnicodeString& SimplePatternFormatter::formatAndReplace(
        const UnicodeString * const *placeholderValues,
        int32_t placeholderValueCount,
        UnicodeString &result,
        int32_t *offsetArray,
        int32_t offsetArrayLength,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return result;
    }
    if (isInvalidArray(placeholderValues, placeholderValueCount)
            || isInvalidArray(offsetArray, offsetArrayLength)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return result;
    }
    if (placeholderValueCount < placeholderCount) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return result;
    }
    int32_t placeholderAtStart = getUniquePlaceholderAtStart();

    // If pattern starts with a placeholder and the value for that
    // placeholder is result, then we can optimize by just appending to
    // result.
    if (placeholderAtStart >= 0
            && placeholderValues[placeholderAtStart] == &result) {

        // Append to result, but let the value of the placeholderAtStart
        // placeholder remain the same as result so that it is treated
        // as the empty string.
        SimplePatternFormatterFixedValues fixedValues(
                result,
                placeholderValues,
                placeholderCount,
                placeholderAtStart,
                status);
        if (U_FAILURE(status)) {
            return result;
        }
        formatAndAppendNoFixValues(
                fixedValues.get(),
                result,
                offsetArray,
                offsetArrayLength);
        
        // We have to make the offset for the placeholderAtStart
        // placeholder be 0. Otherwise it would be the length of the
        // previous value of result.
        if (offsetArrayLength > placeholderAtStart) {
            offsetArray[placeholderAtStart] = 0;
        }
        return result;
    }
    SimplePatternFormatterFixedValues fixedValues(
            result, placeholderValues, placeholderCount, -1, status);
    if (U_FAILURE(status)) {
        return result;
    }
    result.remove();
    return formatAndAppendNoFixValues(
            fixedValues.get(),
            result,
            offsetArray,
            offsetArrayLength);
}

UnicodeString& SimplePatternFormatter::formatAndAppendNoFixValues(
        const UnicodeString * const *placeholderValues,
        UnicodeString &appendTo,
        int32_t *offsetArray,
        int32_t offsetArrayLength) const {
    for (int32_t i = 0; i < offsetArrayLength; ++i) {
        offsetArray[i] = -1;
    }
    if (placeholderSize == 0) {
        appendTo.append(noPlaceholders);
        return appendTo;
    }
    appendRange(
            noPlaceholders,
            0,
            placeholders[0].offset,
            appendTo);
    updatePlaceholderOffset(
            placeholders[0].id,
            appendTo.length(),
            offsetArray,
            offsetArrayLength);
    const UnicodeString *placeholderValue =
            placeholderValues[placeholders[0].id];
    if (placeholderValue != &appendTo) {
        appendTo.append(*placeholderValue);
    }
    for (int32_t i = 1; i < placeholderSize; ++i) {
        appendRange(
                noPlaceholders,
                placeholders[i - 1].offset,
                placeholders[i].offset,
                appendTo);
        updatePlaceholderOffset(
                placeholders[i].id,
                appendTo.length(),
                offsetArray,
                offsetArrayLength);
        placeholderValue =
                placeholderValues[placeholders[i].id];
        if (placeholderValue != &appendTo) {
            appendTo.append(*placeholderValue);
        }
    }
    appendRange(
            noPlaceholders,
            placeholders[placeholderSize - 1].offset,
            noPlaceholders.length(),
            appendTo);
    return appendTo;
}

int32_t SimplePatternFormatter::getUniquePlaceholderAtStart() const {
    if (placeholderSize == 0
            || firstPlaceholderReused || placeholders[0].offset != 0) {
        return -1;
    }
    return placeholders[0].id;
}

int32_t SimplePatternFormatter::ensureCapacity(
        int32_t desiredCapacity, int32_t allocationSize) {
    if (allocationSize < desiredCapacity) {
        allocationSize = desiredCapacity;
    }
    if (desiredCapacity <= placeholders.getCapacity()) {
        return desiredCapacity;
    }
    // allocate new buffer
    if (placeholders.resize(allocationSize, placeholderSize) == NULL) {
        return placeholders.getCapacity();
    }
    return desiredCapacity;
}

UBool SimplePatternFormatter::addPlaceholder(int32_t id, int32_t offset) {
    if (ensureCapacity(placeholderSize + 1, 2 * placeholderSize) < placeholderSize + 1) {
        return FALSE;
    }
    ++placeholderSize;
    PlaceholderInfo *placeholderEnd = &placeholders[placeholderSize - 1];
    placeholderEnd->offset = offset;
    placeholderEnd->id = id;
    if (id >= placeholderCount) {
        placeholderCount = id + 1;
    }
    if (placeholderSize > 1
            && placeholders[placeholderSize - 1].id == placeholders[0].id) {
        firstPlaceholderReused = TRUE;
    }
    return TRUE;
}
    
U_NAMESPACE_END
