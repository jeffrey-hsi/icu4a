/*
*******************************************************************************
* Copyright (C) 1997-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File SMPDTFMT.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/31/97    aliu        Modified extensively to work with 50 locales.
*   04/01/97    aliu        Added support for centuries.
*   07/09/97    helena      Made ParsePosition into a class.
*   07/21/98    stephen     Added initializeDefaultCentury.
*                             Removed getZoneIndex (added in DateFormatSymbols)
*                             Removed subParseLong
*                             Removed chk
*  02/22/99     stephen     Removed character literals for EBCDIC safety
*   10/14/99    aliu        Updated 2-digit year parsing so that only "00" thru
*                           "99" are recognized. {j28 4182066}
*   11/15/99    weiv        Added support for week of year/day of week format
********************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/resbund.h"
#include "unicode/msgfmt.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/timezone.h"
#include "unicode/decimfmt.h"
#include "unicode/dcfmtsym.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include <float.h>

// *****************************************************************************
// class SimpleDateFormat
// *****************************************************************************

U_NAMESPACE_BEGIN

// For time zones that have no names, use strings GMT+minutes and
// GMT-minutes. For instance, in France the time zone is GMT+60.
// Also accepted are GMT+H:MM or GMT-H:MM.
const UChar SimpleDateFormat::fgGmt[]      = {0x0047, 0x004D, 0x0054, 0x0000};         // "GMT"
const UChar SimpleDateFormat::fgGmtPlus[]  = {0x0047, 0x004D, 0x0054, 0x002B, 0x0000}; // "GMT+"
const UChar SimpleDateFormat::fgGmtMinus[] = {0x0047, 0x004D, 0x0054, 0x002D, 0x0000}; // "GMT-"

// This is a pattern-of-last-resort used when we can't load a usable pattern out
// of a resource.
const UChar SimpleDateFormat::fgDefaultPattern[] =
{
    0x79, 0x79, 0x79, 0x79, 0x4D, 0x4D, 0x64, 0x64, 0x20, 0x68, 0x68, 0x3A, 0x6D, 0x6D, 0x20, 0x61, 0
};  /* "yyyyMMdd hh:mm a" */

// This prefix is designed to NEVER MATCH real text, in order to
// suppress the parsing of negative numbers.  Adjust as needed (if
// this becomes valid Unicode).
static const UChar SUPPRESS_NEGATIVE_PREFIX[] = {0xAB00, 0};

/**
 * These are the tags we expect to see in normal resource bundle files associated
 * with a locale.
 */
const char SimpleDateFormat::fgDateTimePatternsTag[]="DateTimePatterns";

const char      SimpleDateFormat::fgClassID = 0; // Value is irrelevant

/**
 * This value of defaultCenturyStart indicates that the system default is to be
 * used.
 */
const UDate     SimpleDateFormat::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   SimpleDateFormat::fgSystemDefaultCenturyYear    = -1;

UDate           SimpleDateFormat::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         SimpleDateFormat::fgSystemDefaultCenturyStartYear   = -1;

//----------------------------------------------------------------------

SimpleDateFormat::~SimpleDateFormat()
{
    delete fSymbols;
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(UErrorCode& status)
:   fSymbols(NULL),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    construct(kShort, (EStyle) (kShort + kDateOffset), Locale::getDefault(), status);
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   UErrorCode &status)
:   fPattern(pattern),
    fSymbols(new DateFormatSymbols(status)),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    initialize(Locale::getDefault(), status);
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fPattern(pattern),
    fSymbols(new DateFormatSymbols(locale, status)),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    initialize(locale, status);
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   DateFormatSymbols* symbolsToAdopt,
                                   UErrorCode& status)
:   fPattern(pattern),
    fSymbols(symbolsToAdopt),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    initialize(Locale::getDefault(), status);
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const DateFormatSymbols& symbols,
                                   UErrorCode& status)
:   fPattern(pattern),
    fSymbols(new DateFormatSymbols(symbols)),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    initialize(Locale::getDefault(), status);
}

//----------------------------------------------------------------------

// Not for public consumption; used by DateFormat
SimpleDateFormat::SimpleDateFormat(EStyle timeStyle,
                                   EStyle dateStyle,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fSymbols(NULL),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    construct(timeStyle, dateStyle, locale, status);
}

//----------------------------------------------------------------------

/**
 * Not for public consumption; used by DateFormat.  This constructor
 * never fails.  If the resource data is not available, it uses the
 * the last resort symbols.
 */
SimpleDateFormat::SimpleDateFormat(const Locale& locale,
                                   UErrorCode& status)
:   fPattern(fgDefaultPattern),
    fSymbols(NULL),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    if (U_FAILURE(status)) return;
    fSymbols = new DateFormatSymbols(locale, status);
    if (U_FAILURE(status))
    {
        status = U_ZERO_ERROR;
        delete fSymbols;
        // This constructor doesn't fail; it uses last resort data
        fSymbols = new DateFormatSymbols(status);
        /* test for NULL */
        if (fSymbols == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }
    initialize(locale, status);
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const SimpleDateFormat& other)
:   DateFormat(other),
    fSymbols(NULL),
    fDefaultCenturyStart(fgSystemDefaultCentury),
    fDefaultCenturyStartYear(fgSystemDefaultCenturyYear)
{
    *this = other;
}

//----------------------------------------------------------------------

SimpleDateFormat& SimpleDateFormat::operator=(const SimpleDateFormat& other)
{
    DateFormat::operator=(other);

    delete fSymbols;
    fSymbols = NULL;

    if (other.fSymbols)
        fSymbols = new DateFormatSymbols(*other.fSymbols);

    fDefaultCenturyStart         = other.fDefaultCenturyStart;
    fDefaultCenturyStartYear     = other.fDefaultCenturyStartYear;

    fPattern = other.fPattern;

    return *this;
}

//----------------------------------------------------------------------

Format*
SimpleDateFormat::clone() const
{
    return new SimpleDateFormat(*this);
}

//----------------------------------------------------------------------

UBool
SimpleDateFormat::operator==(const Format& other) const
{
    if (DateFormat::operator==(other) &&
        other.getDynamicClassID() == getStaticClassID())
    {
        SimpleDateFormat* that = (SimpleDateFormat*)&other;
        return     (fPattern             == that->fPattern &&
                fSymbols             != NULL && // Check for pathological object
                that->fSymbols         != NULL && // Check for pathological object
                *fSymbols             == *that->fSymbols &&
                fDefaultCenturyStart == that->fDefaultCenturyStart);
    }
    return FALSE;
}

//----------------------------------------------------------------------

void SimpleDateFormat::construct(EStyle timeStyle,
                                 EStyle dateStyle,
                                 const Locale& locale,
                                 UErrorCode& status)
{
    // called by several constructors to load pattern data from the resources

    if (U_FAILURE(status)) return;

    // load up the DateTimePatters resource from the appropriate locale (throw
    // an error if for some weird reason the resource is malformed)

    ResourceBundle resources((char *)0, locale, status);

    ResourceBundle dateTimePatterns = resources.get(fgDateTimePatternsTag, status);
    if (U_FAILURE(status)) return;

    if (dateTimePatterns.getSize() <= kDateTime)
    {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }

    // create a symbols object from the locale
    fSymbols = new DateFormatSymbols(locale, status);
    /* test for NULL */
    if (fSymbols == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    UnicodeString str;

    // Move dateStyle from the range [0, 3] to [4, 7] if necessary
    //if (dateStyle >= 0 && dateStyle < DATE_OFFSET) dateStyle = (EStyle)(dateStyle + DATE_OFFSET);

    // if the pattern should include both date and time information, use the date/time
    // pattern string as a guide to tell use how to glue together the appropriate date
    // and time pattern strings.  The actual gluing-together is handled by a convenience
    // method on MessageFormat.
    if ((timeStyle != kNone) &&
        (dateStyle != kNone))
    {
        //  Object[] dateTimeArgs = {
        //     dateTimePatterns[timeStyle], dateTimePatterns[dateStyle]
        //  };
        //  pattern = MessageFormat.format(dateTimePatterns[8], dateTimeArgs);

        Formattable *timeDateArray = new Formattable[2];
        /* test for NULL */
        if (timeDateArray == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        //timeDateArray[0].setString(UnicodeString(dateTimePatterns[timeStyle]));
        //timeDateArray[1].setString(UnicodeString(dateTimePatterns[dateStyle]));
        // use Formattable::adoptString() so that we can use fastCopyFrom()
        // instead of Formattable::setString()'s unaware, safe, deep string clone
        // see Jitterbug 2296
        timeDateArray[0].adoptString(&(new UnicodeString)->fastCopyFrom(dateTimePatterns.getStringEx(timeStyle, status)));
        timeDateArray[1].adoptString(&(new UnicodeString)->fastCopyFrom(dateTimePatterns.getStringEx(dateStyle, status)));

        //MessageFormat::format(UnicodeString(dateTimePatterns[kDateTime]), timeDateArray, 2, fPattern, status);
        MessageFormat::format(dateTimePatterns.getStringEx(kDateTime, status), timeDateArray, 2, fPattern, status);
        delete [] timeDateArray;
    }
    
    // if the pattern includes just time data or just date date, load the appropriate
    // pattern string from the resources
    //else if (timeStyle != kNone) fPattern = UnicodeString(dateTimePatterns[timeStyle]);
    //else if (dateStyle != kNone) fPattern = UnicodeString(dateTimePatterns[dateStyle]);
    // fastCopyFrom() - see DateFormatSymbols::assignArray comments
    else if (timeStyle != kNone) fPattern.fastCopyFrom(dateTimePatterns.getStringEx(timeStyle, status));
    else if (dateStyle != kNone) fPattern.fastCopyFrom(dateTimePatterns.getStringEx(dateStyle, status));
    
    // and if it includes _neither_, that's an error
    else status = U_INVALID_FORMAT_ERROR;

    // finally, finish initializing by creating a Calendar and a NumberFormat
    initialize(locale, status);
}

//----------------------------------------------------------------------

void
SimpleDateFormat::initialize(const Locale& locale,
                             UErrorCode& status)
{
    if (U_FAILURE(status)) return;

    // {sfb} should this be here?
    if (fSymbols->fZoneStringsColCount < 1)
    {
        status = U_INVALID_FORMAT_ERROR; // Check for bogus locale data
        return;
    }

    // We don't need to check that the row count is >= 1, since all 2d arrays have at
    // least one row
    fCalendar = Calendar::createInstance(TimeZone::createDefault(), locale, status);
    fNumberFormat = NumberFormat::createInstance(locale, status);
    if (fNumberFormat != NULL && U_SUCCESS(status))
    {
        // no matter what the locale's default number format looked like, we want
        // to modify it so that it doesn't use thousands separators, doesn't always
        // show the decimal point, and recognizes integers only when parsing

        fNumberFormat->setGroupingUsed(FALSE);
        if (fNumberFormat->getDynamicClassID() == DecimalFormat::getStaticClassID())
            ((DecimalFormat*)fNumberFormat)->setDecimalSeparatorAlwaysShown(FALSE);
        fNumberFormat->setParseIntegerOnly(TRUE);
        fNumberFormat->setMinimumFractionDigits(0); // To prevent "Jan 1.00, 1997.00"

        initializeDefaultCentury();
    }
    else if (U_SUCCESS(status))
    {
        status = U_MISSING_RESOURCE_ERROR;
    }
}

/* Initialize the fields we use to disambiguate ambiguous years. Separate
 * so we can call it from readObject().
 */
void SimpleDateFormat::initializeDefaultCentury() 
{
    fDefaultCenturyStart        = internalGetDefaultCenturyStart();
    fDefaultCenturyStartYear    = internalGetDefaultCenturyStartYear();

    UErrorCode status = U_ZERO_ERROR;
    fCalendar->setTime(fDefaultCenturyStart, status);
    // {sfb} throw away error
}

/* Define one-century window into which to disambiguate dates using
 * two-digit years. Make public in JDK 1.2.
 */
void SimpleDateFormat::parseAmbiguousDatesAsAfter(UDate startDate, UErrorCode& status) 
{
    if(U_FAILURE(status))
        return;
        
    fCalendar->setTime(startDate, status);
    if(U_SUCCESS(status)) {
        fDefaultCenturyStart = startDate;
        fDefaultCenturyStartYear = fCalendar->get(UCAL_YEAR, status);
    }
}
    
//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::format(Calendar& cal, UnicodeString& appendTo, FieldPosition& pos) const
{
    UErrorCode status = U_ZERO_ERROR;
    pos.setBeginIndex(0);
    pos.setEndIndex(0);

    UBool inQuote = FALSE;
    UChar prevCh = 0;
    int32_t count = 0;
    
    // loop through the pattern string character by character
    for (int32_t i = 0; i < fPattern.length() && U_SUCCESS(status); ++i) {
        UChar ch = fPattern[i];
        
        // Use subFormat() to format a repeated pattern character
        // when a different pattern or non-pattern character is seen
        if (ch != prevCh && count > 0) {
            subFormat(appendTo, prevCh, count, pos, cal, status);
            count = 0;
        }
        if (ch == 0x0027 /*'\''*/) {
            // Consecutive single quotes are a single quote literal,
            // either outside of quotes or between quotes
            if ((i+1) < fPattern.length() && fPattern[i+1] == 0x0027 /*'\''*/) {
                appendTo += (UChar)0x0027 /*'\''*/;
                ++i;
            } else {
                inQuote = ! inQuote;
            }
        } 
        else if ( ! inQuote && ((ch >= 0x0061 /*'a'*/ && ch <= 0x007A /*'z'*/) 
                    || (ch >= 0x0041 /*'A'*/ && ch <= 0x005A /*'Z'*/))) {
            // ch is a date-time pattern character to be interpreted
            // by subFormat(); count the number of times it is repeated
            prevCh = ch;
            ++count;
        }
        else {
            // Append quoted characters and unquoted non-pattern characters
            appendTo += ch;
        }
    }

    // Format the last item in the pattern, if any
    if (count > 0) {
        subFormat(appendTo, prevCh, count, pos, cal, status);
    }

    // and if something failed (e.g., an invalid format character), reset our FieldPosition
    // to (0, 0) to show that
    // {sfb} look at this later- are these being set correctly?
    if (U_FAILURE(status)) {
        pos.setBeginIndex(0);
        pos.setEndIndex(0);
    }
    
    return appendTo;
}

UnicodeString&
SimpleDateFormat::format(const Formattable& obj, 
                         UnicodeString& appendTo, 
                         FieldPosition& pos,
                         UErrorCode& status) const
{
    // this is just here to get around the hiding problem
    // (the previous format() override would hide the version of
    // format() on DateFormat that this function correspond to, so we
    // have to redefine it here)
    return DateFormat::format(obj, appendTo, pos, status);
}

//----------------------------------------------------------------------

// Map index into pattern character string to Calendar field number.
const UCalendarDateFields
SimpleDateFormat::fgPatternIndexToCalendarField[] =
{
    UCAL_ERA, UCAL_YEAR, UCAL_MONTH, UCAL_DATE, 
    UCAL_HOUR_OF_DAY, UCAL_HOUR_OF_DAY, UCAL_MINUTE, 
    UCAL_SECOND, UCAL_MILLISECOND, UCAL_DAY_OF_WEEK,
    UCAL_DAY_OF_YEAR, UCAL_DAY_OF_WEEK_IN_MONTH, 
    UCAL_WEEK_OF_YEAR, UCAL_WEEK_OF_MONTH, 
    UCAL_AM_PM, UCAL_HOUR, UCAL_HOUR, UCAL_ZONE_OFFSET,
    UCAL_YEAR_WOY, UCAL_DOW_LOCAL
};

// Map index into pattern character string to DateFormat field number
const DateFormat::EField
SimpleDateFormat::fgPatternIndexToDateFormatField[] = {
    DateFormat::kEraField, DateFormat::kYearField, DateFormat::kMonthField,
    DateFormat::kDateField, DateFormat::kHourOfDay1Field,
    DateFormat::kHourOfDay0Field, DateFormat::kMinuteField,
    DateFormat::kSecondField, DateFormat::kMillisecondField,
    DateFormat::kDayOfWeekField, DateFormat::kDayOfYearField,
    DateFormat::kDayOfWeekInMonthField, DateFormat::kWeekOfYearField,
    DateFormat::kWeekOfMonthField, DateFormat::kAmPmField,
    DateFormat::kHour1Field, DateFormat::kHour0Field,
    DateFormat::kTimezoneField, DateFormat::kYearWOYField, 
    DateFormat::kDOWLocalField
};


//----------------------------------------------------------------------

void
SimpleDateFormat::subFormat(UnicodeString &appendTo,
                            UChar ch,
                            int32_t count,
                            FieldPosition& pos,
                            Calendar& cal,
                            UErrorCode& status) const
{
    // this function gets called by format() to produce the appropriate substitution
    // text for an individual pattern symbol (e.g., "HH" or "yyyy")

    UChar *patternCharPtr = u_strchr(DateFormatSymbols::getPatternUChars(), ch);
    EField patternCharIndex;
    const int32_t maxIntCount = 10;
    int32_t beginOffset = appendTo.length();

    // if the pattern character is unrecognized, signal an error and dump out
    if (patternCharPtr == NULL)
    {
        status = U_INVALID_FORMAT_ERROR;
    }

    patternCharIndex = (EField)(patternCharPtr - DateFormatSymbols::getPatternUChars());
    UCalendarDateFields field = fgPatternIndexToCalendarField[patternCharIndex];
    int32_t value = cal.get(field, status);
    if (U_FAILURE(status)) {
        return;
    }

    switch (patternCharIndex) {
    
    // for any "G" symbol, write out the appropriate era string
    case kEraField:
        appendTo += fSymbols->fEras[value];
        break;

    // for "yyyy", write out the whole year; for "yy", write out the last 2 digits
    case kYearField:
    case kYearWOYField:
        if (count >= 4) 
            zeroPaddingNumber(appendTo, value, 4, maxIntCount);
        else 
            zeroPaddingNumber(appendTo, value, 2, 2);
        break;

    // for "MMMM", write out the whole month name, for "MMM", write out the month
    // abbreviation, for "M" or "MM", write out the month as a number with the
    // appropriate number of digits
    case kMonthField:
        if (count >= 4) 
            appendTo += fSymbols->fMonths[value];
        else if (count == 3) 
            appendTo += fSymbols->fShortMonths[value];
        else 
            zeroPaddingNumber(appendTo, value + 1, count, maxIntCount);
        break;

    // for "k" and "kk", write out the hour, adjusting midnight to appear as "24"
    case kHourOfDay1Field:
        if (value == 0) 
            zeroPaddingNumber(appendTo, cal.getMaximum(UCAL_HOUR_OF_DAY) + 1, count, maxIntCount);
        else 
            zeroPaddingNumber(appendTo, value, count, maxIntCount);
        break;

    // for "SS" and "S", we want to truncate digits so that you still see the MOST
    // significant digits rather than the LEAST (as is the case with the year)
    case kMillisecondField:
        if (count > 3) 
            count = 3;
        else if (count == 2) 
            value = value / 10;
        else if (count == 1) 
            value = value / 100;
        zeroPaddingNumber(appendTo, value, count, maxIntCount);
        break;

    // for "EEEE", write out the day-of-the-week name; otherwise, use the abbreviation
    case kDayOfWeekField:
        if (count >= 4) 
            appendTo += fSymbols->fWeekdays[value];
        else 
            appendTo += fSymbols->fShortWeekdays[value];
        break;

    // for and "a" symbol, write out the whole AM/PM string
    case kAmPmField:
        appendTo += fSymbols->fAmPms[value];
        break;

    // for "h" and "hh", write out the hour, adjusting noon and midnight to show up
    // as "12"
    case kHour1Field:
        if (value == 0) 
            zeroPaddingNumber(appendTo, cal.getLeastMaximum(UCAL_HOUR) + 1, count, maxIntCount);
        else 
            zeroPaddingNumber(appendTo, value, count, maxIntCount);
        break;

    // for the "z" symbols, we have to check our time zone data first.  If we have a
    // localized name for the time zone, then "zzzz" is the whole name and anything
    // shorter is the abbreviation (we also have to check for daylight savings time
    // since the name will be different).  If we don't have a localized time zone name,
    // then the time zone shows up as "GMT+hh:mm" or "GMT-hh:mm" (where "hh:mm" is the
    // offset from GMT) regardless of how many z's were in the pattern symbol
    case kTimezoneField: {
        UnicodeString str;
        int32_t zoneIndex = fSymbols->getZoneIndex(cal.getTimeZone().getID(str));
        if (zoneIndex == -1) {
            value = cal.get(UCAL_ZONE_OFFSET, status) +
                    cal.get(UCAL_DST_OFFSET, status);

            if (value < 0) {
                appendTo += fgGmtMinus;
                value = -value; // suppress the '-' sign for text display.
            }
            else
                appendTo += fgGmtPlus;
            
            zeroPaddingNumber(appendTo, (int32_t)(value/U_MILLIS_PER_HOUR), 2, 2);
            appendTo += (UChar)0x003A /*':'*/;
            zeroPaddingNumber(appendTo, (int32_t)((value%U_MILLIS_PER_HOUR)/U_MILLIS_PER_MINUTE), 2, 2);
        }
        else if (cal.get(UCAL_DST_OFFSET, status) != 0) {
            if (count >= 4) 
                appendTo += fSymbols->fZoneStrings[zoneIndex][3];
            else 
                appendTo += fSymbols->fZoneStrings[zoneIndex][4];
        }
        else {
            if (count >= 4) 
                appendTo += fSymbols->fZoneStrings[zoneIndex][1];
            else 
                appendTo += fSymbols->fZoneStrings[zoneIndex][2];
        }
        }
        break;
    
    // all of the other pattern symbols can be formatted as simple numbers with
    // appropriate zero padding
    default:
    // case kDateField:
    // case kHourOfDay0Field:
    // case kMinuteField:
    // case kSecondField:
    // case kDayOfYearField:
    // case kDayOfWeekInMonthField:
    // case kWeekOfYearField:
    // case kWeekOfMonthField:
    // case kHour0Field:
    // case kDOWLocalField:
        zeroPaddingNumber(appendTo, value, count, maxIntCount);
        break;
    }

    // if the field we're formatting is the one the FieldPosition says it's interested
    // in, fill in the FieldPosition with this field's positions
    if (pos.getField() == fgPatternIndexToDateFormatField[patternCharIndex]) {
        if (pos.getBeginIndex() == 0 && pos.getEndIndex() == 0) {
            pos.setBeginIndex(beginOffset);
            pos.setEndIndex(appendTo.length());
        }
    }
}

//----------------------------------------------------------------------

void
SimpleDateFormat::zeroPaddingNumber(UnicodeString &appendTo, int32_t value, int32_t minDigits, int32_t maxDigits) const
{
    FieldPosition pos(0);

    fNumberFormat->setMinimumIntegerDigits(minDigits);
    fNumberFormat->setMaximumIntegerDigits(maxDigits);
    fNumberFormat->format(value, appendTo, pos);  // 3rd arg is there to speed up processing
}

//----------------------------------------------------------------------

void
SimpleDateFormat::parse(const UnicodeString& text, Calendar& cal, ParsePosition& pos) const
{
    int32_t start = pos.getIndex();
    int32_t oldStart = start;
    UBool ambiguousYear[] = { FALSE };

    UBool inQuote = FALSE;
    UChar prevCh = 0;
    int32_t count = 0;
    int32_t interQuoteCount = 1; // Number of chars between quotes
    UBool allowNegative = TRUE;

    // loop through the pattern string character by character, using it to control how
    // we match characters in the input
    for (int32_t i = 0; i < fPattern.length();++i) {
        UChar ch = fPattern[i];
        
        // if we're inside a quoted string, match characters exactly until we hit
        // another single quote (two single quotes in a row match one single quote
        // in the input)
        if (inQuote)
        {
            if (ch == 0x0027 /*'\''*/)
            {
                // ends with 2nd single quote
                inQuote = FALSE;
                // two consecutive quotes outside a quote means we have
                // a quote literal we need to match.
                if (count == 0)
                {
                    if(start > text.length() || ch != text[start])
                        {
                            pos.setIndex(oldStart);
                            pos.setErrorIndex(start);
                            return;
                        }
                        ++start;
                }
                count = 0;
                interQuoteCount = 0;
            }
            else
                {
                    // pattern uses text following from 1st single quote.
                    if (start >= text.length() || ch != text[start]) {
                        // Check for cases like: 'at' in pattern vs "xt"
                        // in time text, where 'a' doesn't match with 'x'.
                        // If fail to match, return null.
                        pos.setIndex(oldStart); // left unchanged
                        pos.setErrorIndex(start);
                        return;
                    }
                    ++count;
                    ++start;
                }
        }

        // if we're not inside a quoted string...
        else {
            
            // ...a quote mark puts us into a quoted string (and we parse any pending
            // pattern symbols)
            if (ch == 0x0027 /*'\''*/) {
                inQuote = TRUE;
                if (count > 0) 
                {
                    int32_t startOffset = start;
                    start = subParse(text, start, prevCh, count, FALSE, allowNegative, ambiguousYear, cal);
                    allowNegative = TRUE;
                    if ( start < 0 ) {
                        pos.setErrorIndex(startOffset);
                        pos.setIndex(oldStart);
                        // {sfb} correct Date
                        return;
                    }
                    count = 0;
                }

                    if (interQuoteCount == 0)
                    {
                        // This indicates two consecutive quotes inside a quote,
                        // for example, 'o''clock'.  We need to parse this as
                        // representing a single quote within the quote.
                        int32_t startOffset = start;
                        if (start >= text.length() ||  ch != text[start])
                        {
                            pos.setErrorIndex(startOffset);
                            pos.setIndex(oldStart);
                            // {sfb} correct Date
                            return;
                        }
                        ++start;
                        count = 1; // Make it look like we never left
                    }
            }
            
            // if we're on a letter, collect copies of the same letter to determine
            // the whole parse symbol.  when we hit a different character, parse the
            // input based on the resulting symbol
        else if ((ch >= 0x0061 /*'a'*/ && ch <= 0x007A /*'z'*/) 
             || (ch >= 0x0041 /*'A'*/ && ch <= 0x005A /*'Z'*/))
          {
                // ch is a date-time pattern
                if (ch != prevCh && count > 0) // e.g., yyyyMMdd
                {
                    int32_t startOffset = start;
                    // This is the only case where we pass in 'true' for
                    // obeyCount.  That's because the next field directly
                    // abuts this one, so we have to use the count to know when
                    // to stop parsing. [LIU]
                    // Don't allow negatives in this field or in the next.
                    // This prevents anomalies like HHmmss matching 12-34
                    // as 12:-3:4, or 11:57:04.
                    start = subParse(text, start, prevCh, count, TRUE, FALSE, ambiguousYear, cal);
                    allowNegative = FALSE;
                    if (start < 0) {
                        pos.setErrorIndex(startOffset);
                        pos.setIndex(oldStart);
                        // {sfb} correct Date
                        return;
                    }
                    prevCh = ch;
                    count = 1;
                }
                else {
                    if (ch != prevCh) 
                        prevCh = ch;
                    count++;
                }
            }

            // if we're on a non-letter, parse based on any pending pattern symbols
            else if (count > 0) 
            {
                // handle cases like: MM-dd-yy, HH:mm:ss, or yyyy MM dd,
                // where ch = '-', ':', or ' ', repectively.
                int32_t startOffset = start;
                start = subParse( text, start, prevCh, count, FALSE, allowNegative, ambiguousYear, cal);
                allowNegative = TRUE;
                if ( start < 0 ) {
                    pos.setErrorIndex(startOffset);
                    pos.setIndex(oldStart);
                    return;
                }
                if (start >= text.length() || ch != text[start]) {
                    // handle cases like: 'MMMM dd' in pattern vs. "janx20"
                    // in time text, where ' ' doesn't match with 'x'.
                    pos.setErrorIndex(start);
                    pos.setIndex(oldStart);
                    return;
                }
                start++;
                count = 0;
                prevCh = 0;
            }

            // otherwise, match characters exactly
            else 
            {
                if (start >= text.length() || ch != text[start]) {
                    // handle cases like: 'MMMM   dd' in pattern vs.
                    // "jan,,,20" in time text, where "   " doesn't
                    // match with ",,,".

                    pos.setErrorIndex(start);
                    pos.setIndex(oldStart);
                    return;
                }
                start++;
            }

            ++interQuoteCount;
        }
    }

    // if we still have a pending pattern symbol after we're done looping through
    // characters in the pattern string, parse the input based on the final pending
    // pattern symbol
    if (count > 0) 
    {
        int32_t startOffset = start;
        start = subParse(text, start, prevCh, count, FALSE, allowNegative, ambiguousYear, cal);
        if ( start < 0 ) {
            pos.setIndex(oldStart);
            pos.setErrorIndex(startOffset);
            return;
        }
    }

    // At this point the fields of Calendar have been set.  Calendar
    // will fill in default values for missing fields when the time
    // is computed.

    pos.setIndex(start);

    // This part is a problem:  When we call parsedDate.after, we compute the time.
    // Take the date April 3 2004 at 2:30 am.  When this is first set up, the year
    // will be wrong if we're parsing a 2-digit year pattern.  It will be 1904.
    // April 3 1904 is a Sunday (unlike 2004) so it is the DST onset day.  2:30 am
    // is therefore an "impossible" time, since the time goes from 1:59 to 3:00 am
    // on that day.  It is therefore parsed out to fields as 3:30 am.  Then we
    // add 100 years, and get April 3 2004 at 3:30 am.  Note that April 3 2004 is
    // a Saturday, so it can have a 2:30 am -- and it should. [LIU]
    /*
        UDate parsedDate = calendar.getTime();
        if( ambiguousYear[0] && !parsedDate.after(fDefaultCenturyStart) ) {
            calendar.add(Calendar.YEAR, 100);
            parsedDate = calendar.getTime();
        }
    */
    // Because of the above condition, save off the fields in case we need to readjust.
    // The procedure we use here is not particularly efficient, but there is no other
    // way to do this given the API restrictions present in Calendar.  We minimize
    // inefficiency by only performing this computation when it might apply, that is,
    // when the two-digit year is equal to the start year, and thus might fall at the
    // front or the back of the default century.  This only works because we adjust
    // the year correctly to start with in other cases -- see subParse().
    UErrorCode status = U_ZERO_ERROR;
    if (ambiguousYear[0]) // If this is true then the two-digit year == the default start year
    {
        // We need a copy of the fields, and we need to avoid triggering a call to
        // complete(), which will recalculate the fields.  Since we can't access
        // the fields[] array in Calendar, we clone the entire object.  This will
        // stop working if Calendar.clone() is ever rewritten to call complete().
        Calendar *copy = cal.clone();
        UDate parsedDate = copy->getTime(status);
        // {sfb} check internalGetDefaultCenturyStart
        if (parsedDate < internalGetDefaultCenturyStart())
        {
            // We can't use add here because that does a complete() first.
            cal.set(UCAL_YEAR, internalGetDefaultCenturyStartYear() + 100);
        }
        delete copy;
    }

    // If any Calendar calls failed, we pretend that we
    // couldn't parse the string, when in reality this isn't quite accurate--
    // we did parse it; the Calendar calls just failed.
    if (U_FAILURE(status)) { 
        pos.setErrorIndex(start);
        pos.setIndex(oldStart); 
    }
}

UDate
SimpleDateFormat::parse( const UnicodeString& text,
                         ParsePosition& pos) const {
    // redefined here because the other parse() function hides this function's
    // cunterpart on DateFormat
    return DateFormat::parse(text, pos);
}

UDate
SimpleDateFormat::parse(const UnicodeString& text, UErrorCode& status) const
{
    // redefined here because the other parse() function hides this function's
    // counterpart on DateFormat
    return DateFormat::parse(text, status);
}
//----------------------------------------------------------------------

int32_t SimpleDateFormat::matchString(const UnicodeString& text,
                              int32_t start,
                              UCalendarDateFields field,
                              const UnicodeString* data,
                              int32_t dataCount,
                              Calendar& cal) const
{
    int32_t i = 0;
    int32_t count = dataCount;

    if (field == UCAL_DAY_OF_WEEK) i = 1;

    // There may be multiple strings in the data[] array which begin with
    // the same prefix (e.g., Cerven and Cervenec (June and July) in Czech).
    // We keep track of the longest match, and return that.  Note that this
    // unfortunately requires us to test all array elements.
    int32_t bestMatchLength = 0, bestMatch = -1;

    // {sfb} kludge to support case-insensitive comparison
    // {markus 2002oct11} do not just use caseCompareBetween because we do not know
    // the length of the match after case folding
    UnicodeString lcaseText;
    lcaseText.fastCopyFrom(text).foldCase();

    for (; i < count; ++i)
    {
        int32_t length = data[i].length();
        // Always compare if we have no match yet; otherwise only compare
        // against potentially better matches (longer strings).

        UnicodeString lcase;
        lcase.fastCopyFrom(data[i]).foldCase();
                    
        if (length > bestMatchLength && (lcaseText.compareBetween(start, start + length, lcase, 0, length)) == 0)
        {
            bestMatch = i;
            bestMatchLength = length;
        }
    }
    if (bestMatch >= 0)
    {
        cal.set(field, bestMatch);
        return start + bestMatchLength;
    }
    
    return -start;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::set2DigitYearStart(UDate d, UErrorCode& status)
{
    parseAmbiguousDatesAsAfter(d, status);
}

/**
 * Private member function that converts the parsed date strings into
 * timeFields. Returns -start (for ParsePosition) if failed.
 * @param text the time text to be parsed.
 * @param start where to start parsing.
 * @param ch the pattern character for the date field text to be parsed.
 * @param count the count of a pattern character.
 * @return the new start position if matching succeeded; a negative number
 * indicating matching failure, otherwise.
 */
int32_t SimpleDateFormat::subParse(const UnicodeString& text, int32_t& start, UChar ch, int32_t count,
                           UBool obeyCount, UBool allowNegative, UBool ambiguousYear[], Calendar& cal) const
{
    Formattable number;
    int32_t value = 0;
    int32_t i;
    ParsePosition pos(0);
    int32_t patternCharIndex;
    UnicodeString temp;
    UChar *patternCharPtr = u_strchr(DateFormatSymbols::getPatternUChars(), ch);

    if (patternCharPtr == NULL) {
        return -start;
    }

    patternCharIndex = (EField)(patternCharPtr - DateFormatSymbols::getPatternUChars());

    UCalendarDateFields field = fgPatternIndexToCalendarField[patternCharIndex];

    // If there are any spaces here, skip over them.  If we hit the end
    // of the string, then fail.
    for (;;) {
        if (start >= text.length()) {
            return -start;
        }
        UChar32 c = text.char32At(start);
        if (!u_isUWhiteSpace(c)) {
            break;
        }
        start += UTF_CHAR_LENGTH(c);
    }
    pos.setIndex(start);

    // We handle a few special cases here where we need to parse
    // a number value.  We handle further, more generic cases below.  We need
    // to handle some of them here because some fields require extra processing on
    // the parsed value.
    if (patternCharIndex == kHourOfDay1Field /*HOUR_OF_DAY1_FIELD*/ ||
        patternCharIndex == kHour1Field /*HOUR1_FIELD*/ ||
        (patternCharIndex == kMonthField /*MONTH_FIELD*/ && count <= 2) ||
        patternCharIndex == kYearField /*YEAR*/ ||
        patternCharIndex == kYearWOYField)
    {
        int32_t parseStart = pos.getIndex(); // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
        // It would be good to unify this with the obeyCount logic below,
        // but that's going to be difficult.
        const UnicodeString* src;
        if (obeyCount) {
            if ((start+count) > text.length()) {
                return -start;
            }
            text.extractBetween(0, start + count, temp);
            src = &temp;
        } else {
            src = &text;
        }
        parseInt(*src, number, pos, allowNegative);
        if (pos.getIndex() == parseStart)
            // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
            return -start;
        value = number.getLong();
    }

    switch (patternCharIndex) {
    case kEraField:
        return matchString(text, start, UCAL_ERA, fSymbols->fEras, fSymbols->fErasCount, cal);
    case kYearField:
        // If there are 3 or more YEAR pattern characters, this indicates
        // that the year value is to be treated literally, without any
        // two-digit year adjustments (e.g., from "01" to 2001).  Otherwise
        // we made adjustments to place the 2-digit year in the proper
        // century, for parsed strings from "00" to "99".  Any other string
        // is treated literally:  "2250", "-1", "1", "002".
        if (count <= 2 && (pos.getIndex() - start) == 2
            && u_isdigit(text.charAt(start))
            && u_isdigit(text.charAt(start+1)))
        {
            // Assume for example that the defaultCenturyStart is 6/18/1903.
            // This means that two-digit years will be forced into the range
            // 6/18/1903 to 6/17/2003.  As a result, years 00, 01, and 02
            // correspond to 2000, 2001, and 2002.  Years 04, 05, etc. correspond
            // to 1904, 1905, etc.  If the year is 03, then it is 2003 if the
            // other fields specify a date before 6/18, or 1903 if they specify a
            // date afterwards.  As a result, 03 is an ambiguous year.  All other
            // two-digit years are unambiguous.
            int32_t ambiguousTwoDigitYear = fDefaultCenturyStartYear % 100;
            ambiguousYear[0] = (value == ambiguousTwoDigitYear);
            value += (fDefaultCenturyStartYear/100)*100 +
                (value < ambiguousTwoDigitYear ? 100 : 0);
        }
        cal.set(UCAL_YEAR, value);
        return pos.getIndex();
    case kYearWOYField:
        // Comment is the same as for kYearFiels - look above
        if (count <= 2 && (pos.getIndex() - start) == 2
            && u_isdigit(text.charAt(start))
            && u_isdigit(text.charAt(start+1)))
        {
            int32_t ambiguousTwoDigitYear = fDefaultCenturyStartYear % 100;
            ambiguousYear[0] = (value == ambiguousTwoDigitYear);
            value += (fDefaultCenturyStartYear/100)*100 +
                (value < ambiguousTwoDigitYear ? 100 : 0);
        }
        cal.set(UCAL_YEAR_WOY, value);
        return pos.getIndex();
    case kMonthField:
        if (count <= 2) // i.e., M or MM.
        {
            // Don't want to parse the month if it is a string
            // while pattern uses numeric style: M or MM.
            // [We computed 'value' above.]
            cal.set(UCAL_MONTH, value - 1);
            return pos.getIndex();
        }
        else
        {
            // count >= 3 // i.e., MMM or MMMM
            // Want to be able to parse both short and long forms.
            // Try count == 4 first:
            int32_t newStart = 0;
            if ((newStart = matchString(text, start, UCAL_MONTH,
                                      fSymbols->fMonths, fSymbols->fMonthsCount, cal)) > 0)
                return newStart;
            else // count == 4 failed, now try count == 3
                return matchString(text, start, UCAL_MONTH,
                                   fSymbols->fShortMonths, fSymbols->fShortMonthsCount, cal);
        }
    case kHourOfDay1Field:
        // [We computed 'value' above.]
        if (value == cal.getMaximum(UCAL_HOUR_OF_DAY) + 1) 
            value = 0;
        cal.set(UCAL_HOUR_OF_DAY, value);
        return pos.getIndex();
    case kDayOfWeekField:
        {
            // Want to be able to parse both short and long forms.
            // Try count == 4 (DDDD) first:
            int32_t newStart = 0;
            if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                      fSymbols->fWeekdays, fSymbols->fWeekdaysCount, cal)) > 0)
                return newStart;
            else // DDDD failed, now try DDD
                return matchString(text, start, UCAL_DAY_OF_WEEK,
                                   fSymbols->fShortWeekdays, fSymbols->fShortWeekdaysCount, cal);
        }
    case kAmPmField:
        return matchString(text, start, UCAL_AM_PM, fSymbols->fAmPms, fSymbols->fAmPmsCount, cal);
    case kHour1Field:
        // [We computed 'value' above.]
        if (value == cal.getLeastMaximum(UCAL_HOUR)+1) 
            value = 0;
        cal.set(UCAL_HOUR, value);
        return pos.getIndex();
    case kTimezoneField:
        {
        // First try to parse generic forms such as GMT-07:00. Do this first
        // in case localized DateFormatZoneData contains the string "GMT"
        // for a zone; in that case, we don't want to match the first three
        // characters of GMT+/-HH:MM etc.

        UnicodeString lcaseText(text);
        UnicodeString lcaseGMT(fgGmt);
        int32_t sign = 0;
        int32_t offset;
        int32_t gmtLen = lcaseGMT.length();

        // For time zones that have no known names, look for strings
        // of the form:
        //    GMT[+-]hours:minutes or
        //    GMT[+-]hhmm or
        //    GMT.
        
        // {sfb} kludge for case-insensitive compare
        lcaseText.toLower();
        lcaseGMT.toLower();
        
        if ((text.length() - start) > gmtLen &&
            (lcaseText.compare(start, gmtLen, lcaseGMT, 0, gmtLen)) == 0)
        {
            cal.set(UCAL_DST_OFFSET, 0);

            pos.setIndex(start + gmtLen);

            if( text[pos.getIndex()] == 0x002B /*'+'*/ )
                sign = 1;
            else if( text[pos.getIndex()] == 0x002D /*'-'*/ )
                sign = -1;
            else {
                cal.set(UCAL_ZONE_OFFSET, 0 );
                return pos.getIndex();
            }

            // Look for hours:minutes or hhmm.
            pos.setIndex(pos.getIndex() + 1);
            // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
            int32_t parseStart = pos.getIndex();
            Formattable tzNumber;
            fNumberFormat->parse(text, tzNumber, pos);
            if( pos.getIndex() == parseStart) {
                // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
                return -start;
            }
            if( text[pos.getIndex()] == 0x003A /*':'*/ ) {
                // This is the hours:minutes case
                offset = tzNumber.getLong() * 60;
                pos.setIndex(pos.getIndex() + 1);
                // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
                parseStart = pos.getIndex();
                fNumberFormat->parse(text, tzNumber, pos);
                if( pos.getIndex() == parseStart) {
                    // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
                    return -start;
                }
                offset += tzNumber.getLong();
            }
            else {
                // This is the hhmm case.
                offset = tzNumber.getLong();
                if( offset < 24 )
                    offset *= 60;
                else
                    offset = offset % 100 + offset / 100 * 60;
            }

            // Fall through for final processing below of 'offset' and 'sign'.
        }
        else {
            // At this point, check for named time zones by looking through
            // the locale data from the DateFormatZoneData strings.
            // Want to be able to parse both short and long forms.
            const UnicodeString *zs;
            int32_t j;

            for (i = 0; i < fSymbols->fZoneStringsRowCount; i++)
            {
                // Checking long and short zones [1 & 2],
                // and long and short daylight [3 & 4].
                for (j = 1; j <= 4; ++j)
                {
                    zs = &fSymbols->fZoneStrings[i][j];
                    // ### TODO markus 20021014: This use of caseCompare() will fail
                    // if the text contains a character that case-folds into multiple
                    // characters. In that case, zs->length() may be too long, and it does not match.
                    // We need a case-insensitive version of startsWith().
                    // There are similar cases of such caseCompare() uses elsewhere in ICU.
                    if (0 == (text.caseCompare(start, zs->length(), *zs, 0))) {
                        TimeZone *tz = TimeZone::createTimeZone(fSymbols->fZoneStrings[i][0]);
                        cal.set(UCAL_ZONE_OFFSET, tz->getRawOffset());
                        // Must call set() with something -- TODO -- Fix this to
                        // use the correct DST SAVINGS for the zone.
                        delete tz;
                        cal.set(UCAL_DST_OFFSET, j >= 3 ? U_MILLIS_PER_HOUR : 0);
                        return (start + fSymbols->fZoneStrings[i][j].length());
                    }
                }
            }

            // As a last resort, look for numeric timezones of the form
            // [+-]hhmm as specified by RFC 822.  This code is actually
            // a little more permissive than RFC 822.  It will try to do
            // its best with numbers that aren't strictly 4 digits long.
            UErrorCode status = U_ZERO_ERROR;
            DecimalFormat *fmt = new DecimalFormat("+####;-####", status);
            if(U_FAILURE(status))
                return -start;
            fmt->setParseIntegerOnly(TRUE);
            // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
            int32_t parseStart = pos.getIndex();
            Formattable tzNumber;
            fmt->parse( text, tzNumber, pos );
            if( pos.getIndex() == parseStart) {
                // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
                return -start;   // Wasn't actually a number.
            }
            offset = tzNumber.getLong();
            sign = 1;
            if( offset < 0 ) {
                sign = -1;
                offset = -offset;
            }
            if( offset < 24 )
                offset = offset * 60;
            else
                offset = offset % 100 + offset / 100 * 60;

            // Fall through for final processing below of 'offset' and 'sign'.
        }

        // Do the final processing for both of the above cases.  We only
        // arrive here if the form GMT+/-... or an RFC 822 form was seen.
        if (sign != 0)
        {
            offset *= U_MILLIS_PER_MINUTE * sign;

            if (cal.getTimeZone().useDaylightTime())
            {
                cal.set(UCAL_DST_OFFSET, U_MILLIS_PER_HOUR);
                offset -= U_MILLIS_PER_HOUR;
            }
            cal.set(UCAL_ZONE_OFFSET, offset);

            return pos.getIndex();
        }

        // All efforts to parse a zone failed.
        return -start;
        }
    default:
    // case 3: // 'd' - DATE
    // case 5: // 'H' - HOUR_OF_DAY:0-based.  eg, 23:59 + 1 hour =>> 00:59
    // case 6: // 'm' - MINUTE
    // case 7: // 's' - SECOND
    // case 8: // 'S' - MILLISECOND
    // case 10: // 'D' - DAY_OF_YEAR
    // case 11: // 'F' - DAY_OF_WEEK_IN_MONTH
    // case 12: // 'w' - WEEK_OF_YEAR
    // case 13: // 'W' - WEEK_OF_MONTH
    // case 16: // 'K' - HOUR: 0-based.  eg, 11PM + 1 hour =>> 0 AM
    // 'e' - DOW_LOCAL

        // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
        int32_t parseStart = pos.getIndex();
        // Handle "generic" fields
        const UnicodeString* src;
        if (obeyCount) {
            if ((start+count) > text.length()) {
                return -start;
            }
            text.extractBetween(0, start + count, temp);
            src = &temp;
        } else {
            src = &text;
        }
        parseInt(*src, number, pos, allowNegative);
        if (pos.getIndex() != parseStart) {
            // WORK AROUND BUG IN NUMBER FORMAT IN 1.2B3
            cal.set(field, number.getLong());
            return pos.getIndex();
        }
        return -start;
    }
}

/**
 * Parse an integer using fNumberFormat.  This method is semantically
 * const, but actually may modify fNumberFormat.
 */
void SimpleDateFormat::parseInt(const UnicodeString& text,
                                Formattable& number,
                                ParsePosition& pos,
                                UBool allowNegative) const {
    UnicodeString oldPrefix;
    DecimalFormat* df = NULL;
    if (!allowNegative &&
        fNumberFormat->getDynamicClassID() == DecimalFormat::getStaticClassID()) {
        df = (DecimalFormat*)fNumberFormat;
        df->getNegativePrefix(oldPrefix);
        df->setNegativePrefix(SUPPRESS_NEGATIVE_PREFIX);
    }
    fNumberFormat->parse(text, number, pos);
    if (df != NULL) {
        df->setNegativePrefix(oldPrefix);
    }
}

//----------------------------------------------------------------------

void SimpleDateFormat::translatePattern(const UnicodeString& originalPattern,
                                        UnicodeString& translatedPattern,
                                        const UnicodeString& from,
                                        const UnicodeString& to,
                                        UErrorCode& status)
{
  // run through the pattern and convert any pattern symbols from the version
  // in "from" to the corresponding character ion "to".  This code takes
  // quoted strings into account (it doesn't try to translate them), and it signals
  // an error if a particular "pattern character" doesn't appear in "from".
  // Depending on the values of "from" and "to" this can convert from generic
  // to localized patterns or localized to generic.
  if (U_FAILURE(status)) 
    return;
  
  translatedPattern.remove();
  UBool inQuote = FALSE;
  for (int32_t i = 0; i < originalPattern.length(); ++i) {
    UChar c = originalPattern[i];
    if (inQuote) {
      if (c == 0x0027 /*'\''*/) 
    inQuote = FALSE;
    }
    else {
      if (c == 0x0027 /*'\''*/) 
    inQuote = TRUE;
      else if ((c >= 0x0061 /*'a'*/ && c <= 0x007A) /*'z'*/ 
           || (c >= 0x0041 /*'A'*/ && c <= 0x005A /*'Z'*/)) {
    int32_t ci = from.indexOf(c);
    if (ci == -1) {
      status = U_INVALID_FORMAT_ERROR;
      return;
    }
    c = to[ci];
      }
    }
    translatedPattern += c;
  }
  if (inQuote) {
    status = U_INVALID_FORMAT_ERROR;
    return;
  }
}

//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::toPattern(UnicodeString& result) const
{
    result = fPattern;
    return result;
}

//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::toLocalizedPattern(UnicodeString& result,
                                     UErrorCode& status) const
{
    translatePattern(fPattern, result, DateFormatSymbols::getPatternUChars(), fSymbols->fLocalPatternChars, status);
    return result;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::applyPattern(const UnicodeString& pattern)
{
    fPattern = pattern;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::applyLocalizedPattern(const UnicodeString& pattern,
                                        UErrorCode &status)
{
    translatePattern(pattern, fPattern, fSymbols->fLocalPatternChars, DateFormatSymbols::getPatternUChars(), status);
}

//----------------------------------------------------------------------

const DateFormatSymbols*
SimpleDateFormat::getDateFormatSymbols() const
{
    return fSymbols;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::adoptDateFormatSymbols(DateFormatSymbols* newFormatSymbols)
{
    delete fSymbols;
    fSymbols = newFormatSymbols;
}

//----------------------------------------------------------------------
void
SimpleDateFormat::setDateFormatSymbols(const DateFormatSymbols& newFormatSymbols)
{
    delete fSymbols;
    fSymbols = new DateFormatSymbols(newFormatSymbols);
}


//----------------------------------------------------------------------

UDate
SimpleDateFormat::internalGetDefaultCenturyStart() const
{
    // lazy-evaluate systemDefaultCenturyStart
    if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury)
        initializeSystemDefaultCentury();

    // use defaultCenturyStart unless it's the flag value;
    // then use systemDefaultCenturyStart
    return (fDefaultCenturyStart == fgSystemDefaultCentury) ?
        fgSystemDefaultCenturyStart : fDefaultCenturyStart;
}

int32_t
SimpleDateFormat::internalGetDefaultCenturyStartYear() const
{
    // lazy-evaluate systemDefaultCenturyStartYear
    if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury)
        initializeSystemDefaultCentury();

    // use defaultCenturyStart unless it's the flag value;
    // then use systemDefaultCenturyStartYear
    //return (fDefaultCenturyStart == fgSystemDefaultCentury) ?
    return (fDefaultCenturyStartYear == fgSystemDefaultCenturyYear) ?
        fgSystemDefaultCenturyStartYear : fDefaultCenturyStartYear;
}

void
SimpleDateFormat::initializeSystemDefaultCentury()
{
    // initialize systemDefaultCentury and systemDefaultCenturyYear based
    // on the current time.  They'll be set to 80 years before
    // the current time.
    // No point in locking as it should be idempotent.
    if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury)
    {
        UErrorCode status = U_ZERO_ERROR;
        Calendar *calendar = Calendar::createInstance(status);
        if (calendar != NULL && U_SUCCESS(status))
        {
            calendar->setTime(Calendar::getNow(), status);
            calendar->add(UCAL_YEAR, -80, status);
            fgSystemDefaultCenturyStart = calendar->getTime(status);
            fgSystemDefaultCenturyStartYear = calendar->get(UCAL_YEAR, status);
            delete calendar;
        }
        // We have no recourse upon failure unless we want to propagate the failure
        // out.
    }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
