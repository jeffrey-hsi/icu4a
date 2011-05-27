/*
*******************************************************************************
* Copyright (C) 1997-2011, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File CHOICFMT.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/20/97    helena      Finished first cut of implementation and got rid 
*                           of nextDouble/previousDouble and replaced with
*                           boolean array.
*   4/10/97     aliu        Clean up.  Modified to work on AIX.
*   06/04/97    helena      Fixed applyPattern(), toPattern() and not to include 
*                           wchar.h.
*   07/09/97    helena      Made ParsePosition into a class.
*   08/06/97    nos         removed overloaded constructor, fixed 'format(array)'
*   07/22/98    stephen     JDK 1.2 Sync - removed UBool array (doubleFlags)
*   02/22/99    stephen     Removed character literals for EBCDIC safety
********************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/choicfmt.h"
#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "cpputils.h"
#include "cstring.h"
#include "messageimpl.h"
#include "putilimp.h"
#include "uassert.h"
#include <stdio.h>
#include <float.h>

// *****************************************************************************
// class ChoiceFormat
// *****************************************************************************

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ChoiceFormat)

// Special characters used by ChoiceFormat.  There are two characters
// used interchangeably to indicate <=.  Either is parsed, but only
// LESS_EQUAL is generated by toPattern().
#define SINGLE_QUOTE ((UChar)0x0027)   /*'*/
#define LESS_THAN    ((UChar)0x003C)   /*<*/
#define LESS_EQUAL   ((UChar)0x0023)   /*#*/
#define LESS_EQUAL2  ((UChar)0x2264)
#define VERTICAL_BAR ((UChar)0x007C)   /*|*/
#define MINUS        ((UChar)0x002D)   /*-*/

static const UChar LEFT_CURLY_BRACE = 0x7B;     /*{*/
static const UChar RIGHT_CURLY_BRACE = 0x7D;    /*}*/

#ifdef INFINITY
#undef INFINITY
#endif
#define INFINITY     ((UChar)0x221E)

static const UChar gPositiveInfinity[] = {INFINITY, 0};
static const UChar gNegativeInfinity[] = {MINUS, INFINITY, 0};
#define POSITIVE_INF_STRLEN 1
#define NEGATIVE_INF_STRLEN 2

// -------------------------------------
// Creates a ChoiceFormat instance based on the pattern.

ChoiceFormat::ChoiceFormat(const UnicodeString& newPattern,
                           UErrorCode& status)
: constructorErrorCode(status),
  msgPattern(status)
{
    applyPattern(newPattern, status);
}

// -------------------------------------
// Creates a ChoiceFormat instance with the limit array and 
// format strings for each limit.

ChoiceFormat::ChoiceFormat(const double* limits, 
                           const UnicodeString* formats, 
                           int32_t cnt )
: constructorErrorCode(U_ZERO_ERROR),
  msgPattern(constructorErrorCode)
{
    setChoices(limits, NULL, formats, cnt, constructorErrorCode);
}

// -------------------------------------

ChoiceFormat::ChoiceFormat(const double* limits, 
                           const UBool* closures,
                           const UnicodeString* formats, 
                           int32_t cnt )
: constructorErrorCode(U_ZERO_ERROR),
  msgPattern(constructorErrorCode)
{
    setChoices(limits, closures, formats, cnt, constructorErrorCode);
}

// -------------------------------------
// copy constructor

ChoiceFormat::ChoiceFormat(const    ChoiceFormat&   that) 
: NumberFormat(that),
  constructorErrorCode(that.constructorErrorCode),
  msgPattern(that.msgPattern)
{
}

// -------------------------------------
// Private constructor that creates a 
// ChoiceFormat instance based on the 
// pattern and populates UParseError

ChoiceFormat::ChoiceFormat(const UnicodeString& newPattern,
                           UParseError& parseError,
                           UErrorCode& status)
: constructorErrorCode(status),
  msgPattern(status)
{
    applyPattern(newPattern,parseError, status);
}
// -------------------------------------

UBool
ChoiceFormat::operator==(const Format& that) const
{
    if (this == &that) return TRUE;
    if (!NumberFormat::operator==(that)) return FALSE;
    ChoiceFormat& thatAlias = (ChoiceFormat&)that;
    return msgPattern == thatAlias.msgPattern;
}

// -------------------------------------
// copy constructor

const ChoiceFormat&
ChoiceFormat::operator=(const   ChoiceFormat& that)
{
    if (this != &that) {
        NumberFormat::operator=(that);
        constructorErrorCode = that.constructorErrorCode;
        msgPattern = that.msgPattern;
    }
    return *this;
}

// -------------------------------------

ChoiceFormat::~ChoiceFormat()
{
}

// -------------------------------------

/**
 * Convert a double value to a string without the overhead of NumberFormat.
 */
UnicodeString&
ChoiceFormat::dtos(double value,
                   UnicodeString& string)
{
    /* Buffer to contain the digits and any extra formatting stuff. */
    char temp[DBL_DIG + 16];
    char *itrPtr = temp;
    char *expPtr;

    sprintf(temp, "%.*g", DBL_DIG, value);

    /* Find and convert the decimal point.
       Using setlocale on some machines will cause sprintf to use a comma for certain locales.
    */
    while (*itrPtr && (*itrPtr == '-' || isdigit(*itrPtr))) {
        itrPtr++;
    }
    if (*itrPtr != 0 && *itrPtr != 'e') {
        /* We reached something that looks like a decimal point.
        In case someone used setlocale(), which changes the decimal point. */
        *itrPtr = '.';
        itrPtr++;
    }
    /* Search for the exponent */
    while (*itrPtr && *itrPtr != 'e') {
        itrPtr++;
    }
    if (*itrPtr == 'e') {
        itrPtr++;
        /* Verify the exponent sign */
        if (*itrPtr == '+' || *itrPtr == '-') {
            itrPtr++;
        }
        /* Remove leading zeros. You will see this on Windows machines. */
        expPtr = itrPtr;
        while (*itrPtr == '0') {
            itrPtr++;
        }
        if (*itrPtr && expPtr != itrPtr) {
            /* Shift the exponent without zeros. */
            while (*itrPtr) {
                *(expPtr++)  = *(itrPtr++);
            }
            // NULL terminate
            *expPtr = 0;
        }
    }

    string = UnicodeString(temp, -1, US_INV);    /* invariant codepage */
    return string;
}

// -------------------------------------
// calls the overloaded applyPattern method.

void
ChoiceFormat::applyPattern(const UnicodeString& pattern,
                           UErrorCode& status)
{
    msgPattern.parseChoiceStyle(pattern, NULL, status);
    constructorErrorCode = status;
}

// -------------------------------------
// Applies the pattern to this ChoiceFormat instance.

void
ChoiceFormat::applyPattern(const UnicodeString& pattern,
                           UParseError& parseError,
                           UErrorCode& status)
{
    msgPattern.parseChoiceStyle(pattern, &parseError, status);
    constructorErrorCode = status;
}
// -------------------------------------
// Returns the input pattern string.

UnicodeString&
ChoiceFormat::toPattern(UnicodeString& result) const
{
    return result = msgPattern.getPatternString();
}

// -------------------------------------
// Sets the limit and format arrays. 
void
ChoiceFormat::setChoices(  const double* limits, 
                           const UnicodeString* formats, 
                           int32_t cnt )
{
    UErrorCode errorCode = U_ZERO_ERROR;
    setChoices(limits, NULL, formats, cnt, errorCode);
}

// -------------------------------------
// Sets the limit and format arrays. 
void
ChoiceFormat::setChoices(  const double* limits, 
                           const UBool* closures,
                           const UnicodeString* formats, 
                           int32_t cnt )
{
    UErrorCode errorCode = U_ZERO_ERROR;
    setChoices(limits, closures, formats, cnt, errorCode);
}

void
ChoiceFormat::setChoices(const double* limits,
                         const UBool* closures,
                         const UnicodeString* formats,
                         int32_t count,
                         UErrorCode &errorCode) {
    if (U_FAILURE(errorCode)) {
        return;
    }
    if (limits == NULL || formats == NULL) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    // Reconstruct the original input pattern.
    // Modified version of the pre-ICU 4.8 toPattern() implementation.
    UnicodeString result;
    for (int32_t i = 0; i < count; ++i) {
        if (i != 0) {
            result += VERTICAL_BAR;
        }
        UnicodeString buf;
        if (uprv_isPositiveInfinity(limits[i])) {
            result += INFINITY;
        } else if (uprv_isNegativeInfinity(limits[i])) {
            result += MINUS;
            result += INFINITY;
        } else {
            result += dtos(limits[i], buf);
        }
        if (closures != NULL && closures[i]) {
            result += LESS_THAN;
        } else {
            result += LESS_EQUAL;
        }
        // Append formats[i], using quotes if there are special
        // characters.  Single quotes themselves must be escaped in
        // either case.
        const UnicodeString& text = formats[i];
        int32_t textLength = text.length();
        int32_t nestingLevel = 0;
        for (int32_t j = 0; j < textLength; ++j) {
            UChar c = text[j];
            if (c == SINGLE_QUOTE && nestingLevel == 0) {
                // Double each top-level apostrophe.
                result.append(c);
            } else if (c == VERTICAL_BAR && nestingLevel == 0) {
                // Surround each pipe symbol with apostrophes for quoting.
                // If the next character is an apostrophe, then that will be doubled,
                // and although the parser will see the apostrophe pairs beginning
                // and ending one character earlier than our doubling, the result
                // is as desired.
                //   | -> '|'
                //   |' -> '|'''
                //   |'' -> '|''''' etc.
                result.append(SINGLE_QUOTE).append(c).append(SINGLE_QUOTE);
                continue;  // Skip the append(c) at the end of the loop body.
            } else if (c == LEFT_CURLY_BRACE) {
                ++nestingLevel;
            } else if (c == RIGHT_CURLY_BRACE && nestingLevel > 0) {
                --nestingLevel;
            }
            result.append(c);
        }
    }
    // Apply the reconstructed pattern.
    applyPattern(result, errorCode);
}

// -------------------------------------
// Gets the limit array.

const double*
ChoiceFormat::getLimits(int32_t& cnt) const 
{
    cnt = 0;
    return NULL;
}

// -------------------------------------
// Gets the closures array.

const UBool*
ChoiceFormat::getClosures(int32_t& cnt) const 
{
    cnt = 0;
    return NULL;
}

// -------------------------------------
// Gets the format array.

const UnicodeString*
ChoiceFormat::getFormats(int32_t& cnt) const
{
    cnt = 0;
    return NULL;
}

// -------------------------------------
// Formats an int64 number, it's actually formatted as
// a double.  The returned format string may differ
// from the input number because of this.

UnicodeString&
ChoiceFormat::format(int64_t number, 
                     UnicodeString& appendTo, 
                     FieldPosition& status) const
{
    return format((double) number, appendTo, status);
}

// -------------------------------------
// Formats an int32_t number, it's actually formatted as
// a double.

UnicodeString&
ChoiceFormat::format(int32_t number, 
                     UnicodeString& appendTo, 
                     FieldPosition& status) const
{
    return format((double) number, appendTo, status);
}

// -------------------------------------
// Formats a double number.

UnicodeString&
ChoiceFormat::format(double number, 
                     UnicodeString& appendTo, 
                     FieldPosition& /*pos*/) const
{
    if (msgPattern.countParts() == 0) {
        // No pattern was applied, or it failed.
        return appendTo;
    }
    // Get the appropriate sub-message.
    int32_t msgStart = findSubMessage(msgPattern, 0, number);
    if (!MessageImpl::jdkAposMode(msgPattern)) {
        int32_t patternStart = msgPattern.getPart(msgStart).getLimit();
        int32_t msgLimit = msgPattern.getLimitPartIndex(msgStart);
        appendTo.append(msgPattern.getPatternString(),
                        patternStart,
                        msgPattern.getPatternIndex(msgLimit) - patternStart);
        return appendTo;
    }
    // JDK compatibility mode: Remove SKIP_SYNTAX.
    return MessageImpl::appendSubMessageWithoutSkipSyntax(msgPattern, msgStart, appendTo);
}

int32_t
ChoiceFormat::findSubMessage(const MessagePattern &pattern, int32_t partIndex, double number) {
    int32_t count = pattern.countParts();
    int32_t msgStart;
    // Iterate over (ARG_INT|DOUBLE, ARG_SELECTOR, message) tuples
    // until ARG_LIMIT or end of choice-only pattern.
    // Ignore the first number and selector and start the loop on the first message.
    partIndex += 2;
    for (;;) {
        // Skip but remember the current sub-message.
        msgStart = partIndex;
        partIndex = pattern.getLimitPartIndex(partIndex);
        if (++partIndex >= count) {
            // Reached the end of the choice-only pattern.
            // Return with the last sub-message.
            break;
        }
        const MessagePattern::Part &part = pattern.getPart(partIndex++);
        UMessagePatternPartType type = part.getType();
        if (type == UMSGPAT_PART_TYPE_ARG_LIMIT) {
            // Reached the end of the ChoiceFormat style.
            // Return with the last sub-message.
            break;
        }
        // part is an ARG_INT or ARG_DOUBLE
        U_ASSERT(MessagePattern::Part::hasNumericValue(type));
        double boundary = pattern.getNumericValue(part);
        // Fetch the ARG_SELECTOR character.
        int32_t selectorIndex = pattern.getPatternIndex(partIndex++);
        UChar boundaryChar = pattern.getPatternString().charAt(selectorIndex);
        if (boundaryChar == LESS_THAN ? !(number > boundary) : !(number >= boundary)) {
            // The number is in the interval between the previous boundary and the current one.
            // Return with the sub-message between them.
            // The !(a>b) and !(a>=b) comparisons are equivalent to
            // (a<=b) and (a<b) except they "catch" NaN.
            break;
        }
    }
    return msgStart;
}

// -------------------------------------
// Formats an array of objects. Checks if the data type of the objects
// to get the right value for formatting.  

UnicodeString&
ChoiceFormat::format(const Formattable* objs,
                     int32_t cnt,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode& status) const
{
    if(cnt < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    if (msgPattern.countParts() == 0) {
        status = U_INVALID_STATE_ERROR;
        return appendTo;
    }

    for (int32_t i = 0; i < cnt; i++) {
        double objDouble = objs[i].getDouble(status);
        if (U_SUCCESS(status)) {
            format(objDouble, appendTo, pos);
        }
    }

    return appendTo;
}

// -------------------------------------
// Formats an array of objects. Checks if the data type of the objects
// to get the right value for formatting.  

UnicodeString&
ChoiceFormat::format(const Formattable& obj, 
                     UnicodeString& appendTo, 
                     FieldPosition& pos,
                     UErrorCode& status) const
{
    return NumberFormat::format(obj, appendTo, pos, status);
}
// -------------------------------------

void
ChoiceFormat::parse(const UnicodeString& text, 
                    Formattable& result,
                    ParsePosition& pos) const
{
    result.setDouble(parseArgument(msgPattern, 0, text, pos));
}

double
ChoiceFormat::parseArgument(
        const MessagePattern &pattern, int32_t partIndex,
        const UnicodeString &source, ParsePosition &pos) {
    // find the best number (defined as the one with the longest parse)
    int32_t start = pos.getIndex();
    int32_t furthest = start;
    double bestNumber = uprv_getNaN();
    double tempNumber = 0.0;
    int32_t count = pattern.countParts();
    while (partIndex < count && pattern.getPartType(partIndex) != UMSGPAT_PART_TYPE_ARG_LIMIT) {
        tempNumber = pattern.getNumericValue(pattern.getPart(partIndex));
        partIndex += 2;  // skip the numeric part and ignore the ARG_SELECTOR
        int32_t msgLimit = pattern.getLimitPartIndex(partIndex);
        int32_t len = matchStringUntilLimitPart(pattern, partIndex, msgLimit, source, start);
        if (len >= 0) {
            int32_t newIndex = start + len;
            if (newIndex > furthest) {
                furthest = newIndex;
                bestNumber = tempNumber;
                if (furthest == source.length()) {
                    break;
                }
            }
        }
        partIndex = msgLimit + 1;
    }
    if (furthest == start) {
        pos.setErrorIndex(start);
    } else {
        pos.setIndex(furthest);
    }
    return bestNumber;
}

int32_t
ChoiceFormat::matchStringUntilLimitPart(
        const MessagePattern &pattern, int32_t partIndex, int32_t limitPartIndex,
        const UnicodeString &source, int32_t sourceOffset) {
    int32_t matchingSourceLength = 0;
    const UnicodeString &msgString = pattern.getPatternString();
    int32_t prevIndex = pattern.getPart(partIndex).getLimit();
    for (;;) {
        const MessagePattern::Part &part = pattern.getPart(++partIndex);
        if (partIndex == limitPartIndex || part.getType() == UMSGPAT_PART_TYPE_SKIP_SYNTAX) {
            int32_t index = part.getIndex();
            int32_t length = index - prevIndex;
            if (length != 0 && 0 != source.compare(sourceOffset, length, msgString, prevIndex, length)) {
                return -1;  // mismatch
            }
            matchingSourceLength += length;
            if (partIndex == limitPartIndex) {
                return matchingSourceLength;
            }
            prevIndex = part.getLimit();  // SKIP_SYNTAX
        }
    }
}

// -------------------------------------
// Parses the text and return the Formattable object.  

void
ChoiceFormat::parse(const UnicodeString& text, 
                    Formattable& result,
                    UErrorCode& status) const
{
    NumberFormat::parse(text, result, status);
}

// -------------------------------------

Format*
ChoiceFormat::clone() const
{
    ChoiceFormat *aCopy = new ChoiceFormat(*this);
    return aCopy;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
