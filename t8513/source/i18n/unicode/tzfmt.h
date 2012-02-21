/*
*******************************************************************************
* Copyright (C) 2011-2012, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#ifndef __TZFMT_H
#define __TZFMT_H

/**
 * \file 
 * \brief C++ API: TimeZoneFormat
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#ifndef U_HIDE_DRAFT_API

#include "unicode/format.h"
#include "unicode/timezone.h"
#include "unicode/tznames.h"

U_CDECL_BEGIN
/**
 * Constants for time zone display format style used by format/parse APIs
 * in TimeZoneFormat.
 * @draft ICU 49
 */
typedef enum UTimeZoneFormatStyle {
    /**
     * Generic location format, such as "United States Time (New York)", "Italy Time"
     * @draft ICU 49
     */
    UTZFMT_STYLE_GENERIC_LOCATION,
    /**
     * Generic long non-location format, such as "Eastern Time".
     * @draft ICU 49
     */
    UTZFMT_STYLE_GENERIC_LONG,
    /**
     * Generic short non-location format, such as "ET".
     * @draft ICU 49
     */
    UTZFMT_STYLE_GENERIC_SHORT,
    /**
     * Specific long format, such as "Eastern Standard Time".
     * @draft ICU 49
     */
    UTZFMT_STYLE_SPECIFIC_LONG,
    /**
     * Specific short format, such as "EST", "PDT".
     * @draft ICU 49
     */
    UTZFMT_STYLE_SPECIFIC_SHORT,
    /**
     * RFC822 format, such as "-0500"
     * @draft ICU 49
     */
    UTZFMT_STYLE_RFC822,
    /**
     * Localized GMT offset format, such as "GMT-05:00", "UTC+0100"
     * @draft ICU 49
     */
    UTZFMT_STYLE_LOCALIZED_GMT,
    /**
     * ISO 8601 format (extended), such as "-05:00", "Z"(UTC)
     * @draft ICU 49
     */
    UTZFMT_STYLE_ISO8601
} UTimeZoneFormatStyle;

/**
 * Constants for GMT offset pattern types.
 * @draft ICU 49
 */
typedef enum UTimeZoneFormatGMTOffsetPatternType {
    /**
     * Positive offset with hour and minute fields
     * @draft ICU 49
     */
    UTZFMT_PAT_POSITIVE_HM,
    /**
     * Positive offset with hour, minute and second fields
     * @draft ICU 49
     */
    UTZFMT_PAT_POSITIVE_HMS,
    /**
     * Negative offset with hour and minute fields
     * @draft ICU 49
     */
    UTZFMT_PAT_NEGATIVE_HM,
    /**
     * Negative offset with hour, minute and second fields
     * @draft ICU 49
     */
    UTZFMT_PAT_NEGATIVE_HMS
} UTimeZoneFormatGMTOffsetPatternType;

/**
 * Constants for time types used by TimeZoneFormat APIs for
 * receiving time type (standard time, daylight time or unknown).
 * @draft ICU 49
 */
typedef enum UTimeZoneFormatTimeType {
    /**
     * Unknown
     * @draft ICU 49
     */
    UTZFMT_TIME_TYPE_UNKNOWN,
    /**
     * Standard time
     * @draft ICU 49
     */
    UTZFMT_TIME_TYPE_STANDARD,
    /**
     * Daylight saving time
     * @draft ICU 49
     */
    UTZFMT_TIME_TYPE_DAYLIGHT
} UTimeZoneFormatTimeType;

/**
 * Constants for parse option flags, used for specifying optional parse behavior.
 * @draft ICU 49
 */
typedef enum UTimeZoneFormatParseOption {
    /**
     * No option.
     * @draft ICU 49
     */
    UTZFMT_PARSE_OPTION_NONE        = 0x00,
    /**
     * When a time zone display name is not found within a set of display names
     * used for the specified style, look for the name from display names used
     * by other styles.
     * @draft ICU 49
     */
    UTZFMT_PARSE_OPTION_ALL_STYLES  = 0x01
} UTimeZoneFormatParseOption;

U_CDECL_END

typedef void *UMTX;

U_NAMESPACE_BEGIN

class TimeZoneGenericNames;
class UVector;

/**
 * <code>TimeZoneFormat</code> supports time zone display name formatting and parsing.
 * An instance of TimeZoneFormat works as a subformatter of {@link SimpleDateFormat},
 * but you can also directly get a new instance of <code>TimeZoneFormat</code> and
 * formatting/parsing time zone display names.
 * <p>
 * ICU implements the time zone display names defined by <a href="http://www.unicode.org/reports/tr35/">UTS#35
 * Unicode Locale Data Markup Language (LDML)</a>. {@link TimeZoneNames} represents the
 * time zone display name data model and this class implements the algorithm for actual
 * formatting and parsing.
 * 
 * @see SimpleDateFormat
 * @see TimeZoneNames
 * @draft ICU 49
 */
class U_I18N_API TimeZoneFormat : public Format {
public:
    /**
     * Copy constructor.
     * @draft ICU 49
     */
    TimeZoneFormat(const TimeZoneFormat& other);

    /**
     * Destructor.
     * @draft ICU 49
     */
    virtual ~TimeZoneFormat();

    /**
     * Assignment operator.
     * @draft ICU 49
     */
    TimeZoneFormat& operator=(const TimeZoneFormat& other);

    /**
     * Return true if the given Format objects are semantically equal.
     * Objects of different subclasses are considered unequal.
     * @param other The object to be compared with.
     * @return Return TRUE if the given Format objects are semantically equal.
     *                Objects of different subclasses are considered unequal.
     * @draft ICU 49
     */
    virtual UBool operator==(const Format& other) const;

    /**
     * Clone this object polymorphically. The caller is responsible
     * for deleting the result when done.
     * @return A copy of the object
     * @draft ICU 49
     */
    virtual Format* clone() const;

    /**
     * Creates an instance of <code>TimeZoneFormat</code> for the given locale.
     * @param locale The locale.
     * @param status Recevies the status.
     * @return An instance of <code>TimeZoneFormat</code> for the given locale,
     *          owned by the caller.
     * @draft ICU 49
     */
    static TimeZoneFormat* U_EXPORT2 createInstance(const Locale& locale, UErrorCode& status);

    /**
     * Returns the time zone display name data used by this instance.
     * @return The time zone display name data.
     * @draft ICU 49
     */
    const TimeZoneNames* getTimeZoneNames() const;

    /**
     * Sets the time zone display name data to this format instnace.
     * The caller should not delete the TimeZoenNames object after it is adopted
     * by this call.
     * @param tznames TimeZoneNames object to be adopted.
     * @draft ICU 49
     */
    void adoptTimeZoneNames(TimeZoneNames *tznames);

    /**
     * Sets the time zone display name data to this format instnace.
     * @param tznames TimeZoneNames object to be set.
     * @draft ICU 49
     */
    void setTimeZoneNames(const TimeZoneNames &tznames);

    /**
     * Returns the localized GMT format pattern.
     * @param pattern Receives the localized GMT format pattern.
     * @return A reference to the result pattern.
     * @see #setGMTPattern
     * @draft ICU 49
     */
    UnicodeString& getGMTPattern(UnicodeString& pattern) const;

    /**
     * Sets the localized GMT format pattern. The pattern must contain
     * a single argument {0}, for example "GMT {0}".
     * @param pattern The localized GMT format pattern to be used by this object.
     * @param status Recieves the status.
     * @see #getGMTPattern
     * @draft ICU 49
     */
    void setGMTPattern(const UnicodeString& pattern, UErrorCode& status);

    /**
     * Returns the offset pattern used for localized GMT format.
     * @param type The offset pattern type enum.
     * @param pattern Receives the offset pattern.
     * @return A reference to the result pattern.
     * @see #setGMTOffsetPattern
     * @draft ICU 49
     */
    UnicodeString& getGMTOffsetPattern(UTimeZoneFormatGMTOffsetPatternType type, UnicodeString& pattern) const;

    /**
     * Sets the offset pattern for the given offset type.
     * @param type The offset pattern type enum.
     * @param pattern The offset pattern used for localized GMT format for the type.
     * @param status Receives the status.
     * @see #getGMTOffsetPattern
     * @draft ICU 49
     */
    void setGMTOffsetPattern(UTimeZoneFormatGMTOffsetPatternType type, const UnicodeString& pattern, UErrorCode& status);

    /**
     * Returns the decimal digit characters used for localized GMT format in a single string
     * containing from 0 to 9 in the ascending order.
     * @param digits Receives the decimal digits used for localized GMT format.
     * @see #setGMTOffsetDigits
     */
    UnicodeString& getGMTOffsetDigits(UnicodeString& digits) const;

    /**
     * Sets the decimal digit characters used for localized GMT format.
     * @param digits The decimal digits used for localized GMT format.
     * @param status Receives the status.
     * @see #getGMTOffsetDigits
     */
    void setGMTOffsetDigits(const UnicodeString& digits, UErrorCode& status);

    /**
     * Returns the localized GMT format string for GMT(UTC) itself (GMT offset is 0).
     * @param gmtZeroFormat Receives the localized GMT string string for GMT(UTC) itself.
     * @return A reference to the result GMT string.
     * @see #setGMTZeroFormat
     */
    UnicodeString& getGMTZeroFormat(UnicodeString& gmtZeroFormat) const;

    /**
     * Sets the localized GMT format string for GMT(UTC) itself (GMT offset is 0).
     * @param gmtZeroFormat The localized GMT format string for GMT(UTC).
     * @param status Receives the status.
     * @see #getGMTZeroFormat
     */
    void setGMTZeroFormat(const UnicodeString& gmtZeroFormat, UErrorCode& status);

    /**
     * Returns the bitwise flags of UTimeZoneFormatParseOption representing the default parse
     * options used by this object.
     * @return the default parse options.
     * @see ParseOption
     * @draft ICU 49
     */
    int32_t getDefaultParseOptions(void) const;

    /**
     * Sets the default parse options.
     * <p><b>Note</b>: By default, an instance of <code>TimeZoneFormat</code>
     * created by {@link #createInstance} has no parse options set (UTZFMT_PARSE_OPTION_NONE).
     * To specify multipe options, use bitwise flags of UTimeZoneFormatParseOption.
     * @see #UTimeZoneFormatParseOption
     * @draft ICU 49
     */
    void setDefaultParseOptions(int32_t flags);

    /**
     * Returns the RFC822 style time zone string for the given offset.
     * For example, "-0800".
     * @param offset The offset from GMT(UTC) in milliseconds.
     * @param result Recevies the RFC822 style GMT(UTC) offset format.
     * @return A reference to the result.
     * @see #parseOffsetRFC822
     * @draft ICU 49
     */
    UnicodeString& formatOffsetRFC822(int32_t offset, UnicodeString& result, UErrorCode& status) const;

    /**
     * Returns the ISO 8601 style time zone string for the given offset.
     * For example, "-08:00" and "Z".
     * @param offset The offset from GMT(UTC) in milliseconds.
     * @param result Recevies the ISO 8601 style GMT(UTC) offset format.
     * @return A reference to the result.
     * @see #parseOffsetISO8601
     * @draft ICU 49
     */
    UnicodeString& formatOffsetISO8601(int32_t offset, UnicodeString& result, UErrorCode& status) const;

    /**
     * Returns the localized GMT(UTC) offset format for the given offset.
     * The localized GMT offset is defined by;
     * <ul>
     * <li>GMT format pattern (e.g. "GMT {0}" - see {@link #getGMTPattern})
     * <li>Offset time pattern (e.g. "+HH:mm" - see {@link #getGMTOffsetPattern})
     * <li>Offset digits (e.g. "0123456789" - see {@link #getGMTOffsetDigits})
     * <li>GMT zero format (e.g. "GMT" - see {@link #getGMTZeroFormat})
     * </ul>
     * @param offset the offset from GMT(UTC) in milliseconds.
     * @param result Receives the localized GMT format string.
     * @return A reference to the result.
     * @see #parseOffsetLocalizedGMT
     * @draft ICU 49
     */
    UnicodeString& formatOffsetLocalizedGMT(int32_t offset, UnicodeString& result, UErrorCode& status) const;

    /**
     * Returns the display name of the time zone at the given date for the style.
     * @param style The style (e.g. <code>UTZFMT_STYLE_GENERIC_LONG</code>, <code>UTZFMT_STYLE_LOCALIZED_GMT</code>...)
     * @param tz The time zone.
     * @param date The date.
     * @param name Receives the display name.
     * @param timeType the output argument for receiving the time type (standard/daylight/unknown)
     * used for the display name, or NULL if the information is not necessary.
     * @return A reference to the result
     * @see #UTimeZoneFormatStyle
     * @see #UTimeZoneFormatTimeType
     * @draft ICU 49
     */
    virtual UnicodeString& format(UTimeZoneFormatStyle style, const TimeZone& tz, UDate date,
        UnicodeString& name, UTimeZoneFormatTimeType* timeType = NULL) const;

    /**
     * Returns offset from GMT(UTC) in milliseconds for the given RFC822
     * style time zone string. When the given string is not an RFC822 time zone
     * string, this method sets the current position as the error index
     * to <code>ParsePosition pos</code> and returns 0.
     * @param text The text contains RFC822 style time zone string (e.g. "-0800")
     *              at the position.
     * @param pos The ParsePosition object.
     * @return The offset from GMT(UTC) in milliseconds for the given RFC822 style
     *              time zone string.
     * @see #formatOffsetRFC822
     * @draft ICU 49
     */
    int32_t parseOffsetRFC822(const UnicodeString& text, ParsePosition& pos) const;

    /**
     * Returns offset from GMT(UTC) in milliseconds for the given ISO 8601
     * style time zone string. When the given string is not an ISO 8601 time zone
     * string, this method sets the current position as the error index
     * to <code>ParsePosition pos</code> and returns 0.
     * @param text The text contains RFC822 style time zone string (e.g. "-08:00", "Z")
     *              at the position.
     * @param pos The ParsePosition object.
     * @return The offset from GMT(UTC) in milliseconds for the given ISO 8601 style
     *              time zone string.
     * @see #formatOffsetISO8601
     * @draft ICU 49
     */
    int32_t parseOffsetISO8601(const UnicodeString& text, ParsePosition& pos) const;

    /**
     * Returns offset from GMT(UTC) in milliseconds for the given localized GMT
     * offset format string. When the given string cannot be parsed, this method
     * sets the current position as the error index to <code>ParsePosition pos</code>
     * and returns 0.
     * @param text The text contains a localized GMT offset string at the position.
     * @param pos The ParsePosition object.
     * @return The offset from GMT(UTC) in milliseconds for the given localized GMT
     *          offset format string.
     * @see #formatOffsetLocalizedGMT
     * @draft ICU 49
     */
    int32_t parseOffsetLocalizedGMT(const UnicodeString& text, ParsePosition& pos) const;

    /**
     * Returns a <code>TimeZone</code> by parsing the time zone string according to
     * the given parse position, the specified format style and parse options.
     * 
     * @param text The text contains a time zone string at the position.
     * @param style The format style
     * @param pos The position.
     * @param parseOptions The parse options repesented by bitwise flags of UTimeZoneFormatParseOption.
     * @param timeType The output argument for receiving the time type (standard/daylight/unknown),
     * or NULL if the information is not necessary.
     * @return A <code>TimeZone</code>, or null if the input could not be parsed.
     * @see UTimeZoneFormatStyle
     * @see UTimeZoneFormatParseOption
     * @see UTimeZoneFormatTimeType
     * @draft ICU 49
     */
    virtual TimeZone* parse(UTimeZoneFormatStyle style, const UnicodeString& text, ParsePosition& pos,
        int32_t parseOptions, UTimeZoneFormatTimeType* timeType = NULL) const;

    /**
     * Returns a <code>TimeZone</code> by parsing the time zone string according to
     * the given parse position, the specified format style and the default parse options.
     * 
     * @param text The text contains a time zone string at the position.
     * @param style The format style
     * @param pos The position.
     * @param timeType The output argument for receiving the time type (standard/daylight/unknown),
     * or NULL if the information is not necessary.
     * @return A <code>TimeZone</code>, or null if the input could not be parsed.
     * @see UTimeZoneFormatStyle
     * @see UTimeZoneFormatParseOption
     * @see UTimeZoneFormatTimeType
     * @draft ICU 49
     */
    TimeZone* parse(UTimeZoneFormatStyle style, const UnicodeString& text, ParsePosition& pos,
        UTimeZoneFormatTimeType* timeType = NULL) const;

    /* ----------------------------------------------
     * Format APIs
     * ---------------------------------------------- */

    /**
     * Format an object to produce a time zone display string using localized GMT offset format.
     * This method handles Formattable objects with a <code>TimeZone</code>. If a the Formattable
     * object type is not a <code>TimeZone</code>, then it returns a failing UErrorCode.
     * @param obj The object to format. Must be a <code>TimeZone</code>.
     * @param appendTo Output parameter to receive result. Result is appended to existing contents.
     * @param pos On input: an alignment field, if desired. On output: the offsets of the alignment field.
     * @param status Output param filled with success/failure status.
     * @return Reference to 'appendTo' parameter.
     * @draft ICU 49
     */
    virtual UnicodeString& format(const Formattable& obj, UnicodeString& appendTo,
        FieldPosition& pos, UErrorCode& status) const;

    /**
     * Parse a string to produce an object. This methods handles parsing of
     * time zone display strings into Formattable objects with <code>TimeZone</code>.
     * @param source The string to be parsed into an object.
     * @param result Formattable to be set to the parse result. If parse fails, return contents are undefined.
     * @param parse_pos The position to start parsing at. Upon return this param is set to the position after the
     *                  last character successfully parsed. If the source is not parsed successfully, this param
     *                  will remain unchanged.
     * @return A newly created Formattable* object, or NULL on failure.  The caller owns this and should
     *                 delete it when done.
     * @draft ICU 49
     */
    virtual void parseObject(const UnicodeString& source, Formattable& result, ParsePosition& parse_pos) const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     * @draft ICU 49
     */
    static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     * @draft ICU 49
     */
    virtual UClassID getDynamicClassID() const;

protected:
    /**
     * Constructs a TimeZoneFormat object for the specified locale.
     * @param locale the locale
     * @param status receives the status.
     * @draft ICU 49
     */
    TimeZoneFormat(const Locale& locale, UErrorCode& status);

private:
    /* mutex */
    UMTX fLock;

    /* Locale of this object */
    Locale fLocale;

    /* Stores the region (could be implicit default) */ 
    char fTargetRegion[ULOC_COUNTRY_CAPACITY];

    /* TimeZoneNames object used by this formatter */
    TimeZoneNames* fTimeZoneNames;

    /* TimeZoneGenericNames object used by this formatter - lazily instantiated */
    TimeZoneGenericNames* fTimeZoneGenericNames;

    /* Localized GMT format pattern - e.g. "GMT{0}" */
    UnicodeString fGMTPattern;

    /* Array of offset patterns used by Localized GMT format - e.g. "+HH:mm" */
    UnicodeString fGMTOffsetPatterns[UTZFMT_PAT_NEGATIVE_HMS + 1];

    /* Localized decimal digits used by Localized GMT format */
    UChar32 fGMTOffsetDigits[10];

    /* Localized GMT zero format - e.g. "GMT" */
    UnicodeString fGMTZeroFormat;

    /* Bit flags representing parse options */
    int32_t fDefParseOptionFlags;

    /* Constant parts of GMT format pattern, populated from localized GMT format pattern*/
    UnicodeString fGMTPatternPrefix;    /* Substring before {0} */
    UnicodeString fGMTPatternSuffix;    /* Substring after {0} */

    /* Compiled offset patterns generated from fGMTOffsetPatterns[] */
    UVector* fGMTOffsetPatternItems[UTZFMT_PAT_NEGATIVE_HMS + 1];

    /**
     * Returns the time zone's specific format string.
     * @param tz the time zone
     * @param stdType the name type used for standard time
     * @param dstType the name type used for daylight time
     * @param date the date
     * @param name receives the time zone's specific format name string
     * @param timeType when null, actual time type is set
     * @return a reference to name.
     */
    UnicodeString& formatSpecific(const TimeZone& tz, UTimeZoneNameType stdType, UTimeZoneNameType dstType,
        UDate date, UnicodeString& name, UTimeZoneFormatTimeType *timeType) const;

    /**
     * Returns the time zone's generic format string.
     * @param tz the time zone
     * @param genType the generic name type
     * @param date the date
     * @param name receives the time zone's generic format name string
     * @return a reference to name.
     */
    UnicodeString& formatGeneric(const TimeZone& tz, int32_t genType, UDate date, UnicodeString& name) const;

    /**
     * Lazily create a TimeZoneGenericNames instance
     * @param status receives the status
     * @return the cached TimeZoneGenericNames.
     */
    const TimeZoneGenericNames* getTimeZoneGenericNames(UErrorCode& status) const;

    /**
     * Private enum specifying a combination of offset fields
     */
    enum OffsetFields {
        FIELDS_H,
        FIELDS_HM,
        FIELDS_HMS
    };

    /**
     * Parses the localized GMT pattern string and initialize
     * localized gmt pattern fields.
     * @param gmtPattern the localized GMT pattern string such as "GMT {0}"
     * @param status U_ILLEGAL_ARGUMENT_ERROR is set when the specified pattern does not
     *               contain an argument "{0}".
     */
    void initGMTPattern(const UnicodeString& gmtPattern, UErrorCode& status);

    /**
     * Parse the GMT offset pattern into runtime optimized format.
     * @param pattern the offset pattern string
     * @param required the required set of fields, such as FIELDS_HM
     * @param status U_ILLEGAL_ARGUMENT is set when the specified pattern does not contain
     *               pattern letters for the required fields.
     * @return A list of GMTOffsetField objects, or NULL on error.
     */
    static UVector* parseOffsetPattern(const UnicodeString& pattern, OffsetFields required, UErrorCode& status);

    /**
     * Appends second field to the offset pattern with hour/minute
     * Note: This code will be obsoleted once we add hour-minute-second pattern data in CLDR.
     * @param offsetHM the offset pattern including hour and minute fields
     * @param result the output offset pattern including hour, minute and second fields
     * @return a reference to result
     */
    static UnicodeString& expandOffsetPattern(const UnicodeString& offsetHM, UnicodeString& result);

    /**
     * Break input string into UChar32[]. Each array element represents
     * a code point. This method is used for parsing localized digit
     * characters and support characters in Unicode supplemental planes.
     * @param str the string
     * @param codeArray receives the result
     * @param capacity the capacity of codeArray
     * @return TRUE when the specified code array is fully filled with code points
     *         (no under/overflow).
     */
    static UBool toCodePoints(const UnicodeString& str, UChar32* codeArray, int32_t capacity);

    /**
     * Returns offset from GMT(UTC) in milliseconds for the given ISO 8601 style
     * (extended format) time zone string. When the given string is not an ISO 8601 time
     * zone string, this method sets the current position as the error index
     * to <code>ParsePosition pos</code> and returns 0.
     * @param text the text contains ISO 8601 style time zone string (e.g. "-08:00", "Z")
     *      at the position.
     * @param pos the position, non-negative error index will be set on failure.
     * @param extendedOnly TRUE if parsing the text as ISO 8601 extended offset format (e.g. "-08:00"),
     *      or FALSE to evaluate the text as basic format.
     * @param hasDigitOffset receiving if the parsed zone string contains offset digits.
     * @return the offset from GMT(UTC) in milliseconds for the given ISO 8601 style
     *      time zone string.
     */
    int32_t parseOffsetISO8601(const UnicodeString& text, ParsePosition& pos, UBool extendedOnly,
        UBool* hasDigitOffset = NULL) const;

    /**
     * Appends localized digits to the buffer.
     * This code assumes that the input number is 0 - 59
     * @param buf the target buffer
     * @param n the integer number
     * @param minDigits the minimum digits width
     */
    void appendOffsetDigits(UnicodeString& buf, int32_t n, uint8_t minDigits) const;

    /**
     * Returns offset from GMT(UTC) in milliseconds for the given localized GMT
     * offset format string. When the given string cannot be parsed, this method
     * sets the current position as the error index to <code>ParsePosition pos</code>
     * and returns 0.
     * @param text the text contains a localized GMT offset string at the position.
     * @param pos the position, non-negative error index will be set on failure.
     * @param hasDigitOffset receiving if the parsed zone string contains offset digits.
     * @return the offset from GMT(UTC) in milliseconds for the given localized GMT
     *      offset format string.
     */
    int32_t parseOffsetLocalizedGMT(const UnicodeString& text, ParsePosition& pos,
        UBool* hasDigitOffset) const;

    /**
     * Parses localized GMT offset fields into offset.
     * @param text the input text
     * @param start the start index
     * @param minimumHourWidth true if the parser allows hour field width to be 1
     * @param parsedLen the parsed length, or 0 on failure.
     * @return the parsed offset in milliseconds.
     */
    int32_t parseOffsetFields(const UnicodeString& text, int32_t start, UBool minimumHourWidth,
        int32_t& parsedLen) const;

    /**
     * Parses abutting localized GMT offset fields (such as 0800) into offset.
     * @param text the input text
     * @param start the start index
     * @param parsedLen the parsed length, or 0 on failure
     * @return the parsed offset in milliseconds.
     */
    int32_t parseAbuttingOffsetFields(const UnicodeString& text, int32_t start, int32_t& parsedLen) const;

    /**
     * Parses the input text using the default format patterns (e.g. "UTC{0}").
     * @param text the input text
     * @param start the start index
     * @param parsedLen the parsed length, or 0 on failure
     * @return the parsed offset in milliseconds.
     */
    int32_t parseOffsetDefaultLocalizedGMT(const UnicodeString& text, int start, int32_t& parsedLen) const;

    /**
     * Parses the input GMT offset fields with the default offset pattern.
     * @param text the input text
     * @param start the start index
     * @param separator the separator character, e.g. ':'
     * @param parsedLen the parsed length, or 0 on failure.
     * @return the parsed offset in milliseconds.
     */
    int32_t parseDefaultOffsetFields(const UnicodeString& text, int32_t start, UChar separator,
        int32_t& parsedLen) const;

    /**
     * Reads an offset field value. This method will stop parsing when
     * 1) number of digits reaches <code>maxDigits</code>
     * 2) just before already parsed number exceeds <code>maxVal</code>
     * 
     * @param text the text
     * @param start the start offset
     * @param minDigits the minimum number of required digits
     * @param maxDigits the maximum number of digits
     * @param minVal the minimum value
     * @param maxVal the maximum value
     * @param parsedLen the actual parsed length.
     * @return the integer value parsed
     */
    int32_t parseOffsetFieldWithLocalizedDigits(const UnicodeString& text, int32_t start,
        uint8_t minDigits, uint8_t maxDigits, uint16_t minVal, uint16_t maxVal, int32_t& parsedLen) const;

    /**
     * Reads a single decimal digit, either localized digits used by this object
     * or any Unicode numeric character.
     * @param text the text
     * @param start the start index
     * @param len the actual length read from the text
     * the start index is not a decimal number.
     * @return the integer value of the parsed digit, or -1 on failure.
     */
    int32_t parseSingleLocalizedDigit(const UnicodeString& text, int32_t start, int32_t& len) const;

    /**
     * Formats offset using ASCII digits. The input offset range must be
     * within +/-24 hours (exclusive).
     * @param offset The offset
     * @param sep The field separator character or 0 if not required
     * @param minFields The minimum fields
     * @param maxFields The maximum fields
     * @return The offset string
     */
    static UnicodeString& formatOffsetWithAsciiDigits(int32_t offset, UChar sep,
        OffsetFields minFields, OffsetFields maxFields, UnicodeString& result);

    /**
     * Parses offset represented by contiguous ASCII digits.
     * <p>
     * Note: This method expects the input position is already at the start of
     * ASCII digits and does not parse sign (+/-).
     * @param text The text contains a sequence of ASCII digits
     * @param pos The parse position
     * @param minFields The minimum Fields to be parsed
     * @param maxFields The maximum Fields to be parsed
     * @param fixedHourWidth true if hour field must be width of 2
     * @return Parsed offset, 0 or positive number.
     */
    static int32_t parseAbuttingAsciiOffsetFields(const UnicodeString& text, ParsePosition& pos,
        OffsetFields minFields, OffsetFields maxFields, UBool fixedHourWidth);

    /**
     * Parses offset represented by ASCII digits and separators.
     * <p>
     * Note: This method expects the input position is already at the start of
     * ASCII digits and does not parse sign (+/-).
     * @param text The text
     * @param pos The parse position
     * @param sep The separator character
     * @param minFields The minimum Fields to be parsed
     * @param maxFields The maximum Fields to be parsed
     * @param fixedHourWidth true if hour field must be width of 2
     * @return Parsed offset, 0 or positive number.
     */
    static int32_t parseAsciiOffsetFields(const UnicodeString& text, ParsePosition& pos, UChar sep,
        OffsetFields minFields, OffsetFields maxFields, UBool fixedHourWidth);

    /**
     * Unquotes the message format style pattern.
     * @param pattern the pattern
     * @param result receive the unquoted pattern.
     * @return A reference to result.
     */
    static UnicodeString& unquote(const UnicodeString& pattern, UnicodeString& result);

    /**
     * Initialize localized GMT format offset hour/min/sec patterns.
     * This method parses patterns into optimized run-time format.
     * @param status receives the status.
     */
    void initGMTOffsetPatterns(UErrorCode& status);

    /**
     */
    //TODO
    TimeZone* createTimeZoneForOffset(int32_t offset) const;
    static UTimeZoneFormatTimeType getTimeType(UTimeZoneNameType nameType);
    UnicodeString& getTimeZoneID(const TimeZoneNames::MatchInfoCollection* matches, int32_t idx, UnicodeString& tzID) const;
};

U_NAMESPACE_END

#endif  /* U_HIDE_DRAFT_API */
#endif
#endif

