/*******************************************************************************
* Copyright (C) 2008-2015, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File DTITVINF.CPP 
*
*******************************************************************************
*/

#include "unicode/dtitvinf.h"


#if !UCONFIG_NO_FORMATTING

//TODO: define it in compiler time
//#define DTITVINF_DEBUG 1


#ifdef DTITVINF_DEBUG 
#include <iostream>
#endif

#include "cstring.h"
#include "unicode/msgfmt.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "dtitv_impl.h"
#include "hash.h"
#include "gregoimp.h"
#include "uresimp.h"
#include "hash.h"
#include "gregoimp.h"
#include "uresimp.h"


U_NAMESPACE_BEGIN


#ifdef DTITVINF_DEBUG 
#define PRINTMESG(msg) { std::cout << "(" << __FILE__ << ":" << __LINE__ << ") " << msg << "\n"; }
#endif

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DateIntervalInfo)

static const char gCalendarTag[]="calendar";
static const char gGregorianTag[]="gregorian";
static const char gIntervalDateTimePatternTag[]="intervalFormats";
static const char gFallbackPatternTag[]="fallback";

// {0}
static const UChar gFirstPattern[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET};
// {1}
static const UChar gSecondPattern[] = {LEFT_CURLY_BRACKET, DIGIT_ONE, RIGHT_CURLY_BRACKET};

// default fall-back
static const UChar gDefaultFallbackPattern[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, EN_DASH, SPACE, LEFT_CURLY_BRACKET, DIGIT_ONE, RIGHT_CURLY_BRACKET, 0};



DateIntervalInfo::DateIntervalInfo(UErrorCode& status) 
:   fFallbackIntervalPattern(gDefaultFallbackPattern),
    fFirstDateInPtnIsLaterDate(false),
    fIntervalPatterns(NULL)
{
    fIntervalPatterns = initHash(status);
}



DateIntervalInfo::DateIntervalInfo(const Locale& locale, UErrorCode& status)
:   fFallbackIntervalPattern(gDefaultFallbackPattern),
    fFirstDateInPtnIsLaterDate(false),
    fIntervalPatterns(NULL)
{
    initializeData(locale, status);
}



void
DateIntervalInfo::setIntervalPattern(const UnicodeString& skeleton,
                                     UCalendarDateFields lrgDiffCalUnit,
                                     const UnicodeString& intervalPattern,
                                     UErrorCode& status) {
    
    if ( lrgDiffCalUnit == UCAL_HOUR_OF_DAY ) {
        setIntervalPatternInternally(skeleton, UCAL_AM_PM, intervalPattern, status);
        setIntervalPatternInternally(skeleton, UCAL_HOUR, intervalPattern, status);
    } else if ( lrgDiffCalUnit == UCAL_DAY_OF_MONTH ||
                lrgDiffCalUnit == UCAL_DAY_OF_WEEK ) {
        setIntervalPatternInternally(skeleton, UCAL_DATE, intervalPattern, status);
    } else {
        setIntervalPatternInternally(skeleton, lrgDiffCalUnit, intervalPattern, status);
    }
}


void
DateIntervalInfo::setFallbackIntervalPattern(
                                    const UnicodeString& fallbackPattern,
                                    UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t firstPatternIndex = fallbackPattern.indexOf(gFirstPattern, 
                        sizeof(gFirstPattern)/sizeof(gFirstPattern[0]), 0);
    int32_t secondPatternIndex = fallbackPattern.indexOf(gSecondPattern, 
                        sizeof(gSecondPattern)/sizeof(gSecondPattern[0]), 0);
    if ( firstPatternIndex == -1 || secondPatternIndex == -1 ) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if ( firstPatternIndex > secondPatternIndex ) { 
        fFirstDateInPtnIsLaterDate = true;
    }
    fFallbackIntervalPattern = fallbackPattern;
}



DateIntervalInfo::DateIntervalInfo(const DateIntervalInfo& dtitvinf)
:   UObject(dtitvinf),
    fIntervalPatterns(NULL)
{
    *this = dtitvinf;
}
    


DateIntervalInfo&
DateIntervalInfo::operator=(const DateIntervalInfo& dtitvinf) {
    if ( this == &dtitvinf ) {
        return *this;
    }
    
    UErrorCode status = U_ZERO_ERROR;
    deleteHash(fIntervalPatterns);
    fIntervalPatterns = initHash(status);
    copyHash(dtitvinf.fIntervalPatterns, fIntervalPatterns, status);
    if ( U_FAILURE(status) ) {
        return *this;
    } 

    fFallbackIntervalPattern = dtitvinf.fFallbackIntervalPattern;
    fFirstDateInPtnIsLaterDate = dtitvinf.fFirstDateInPtnIsLaterDate;
    return *this;
}


DateIntervalInfo*
DateIntervalInfo::clone() const {
    return new DateIntervalInfo(*this);
}


DateIntervalInfo::~DateIntervalInfo() {
    deleteHash(fIntervalPatterns);
    fIntervalPatterns = NULL;
}


UBool
DateIntervalInfo::operator==(const DateIntervalInfo& other) const {
    UBool equal = ( 
      fFallbackIntervalPattern == other.fFallbackIntervalPattern &&
      fFirstDateInPtnIsLaterDate == other.fFirstDateInPtnIsLaterDate );

    if ( equal == TRUE ) {
        equal = fIntervalPatterns->equals(*(other.fIntervalPatterns));
    }

    return equal;
}


UnicodeString&
DateIntervalInfo::getIntervalPattern(const UnicodeString& skeleton,
                                     UCalendarDateFields field,
                                     UnicodeString& result,
                                     UErrorCode& status) const {
    if ( U_FAILURE(status) ) {
        return result;
    }
   
    const UnicodeString* patternsOfOneSkeleton = (UnicodeString*) fIntervalPatterns->get(skeleton);
    if ( patternsOfOneSkeleton != NULL ) {
        IntervalPatternIndex index = calendarFieldToIntervalIndex(field, status);
        if ( U_FAILURE(status) ) {
            return result;
        }
        const UnicodeString& intervalPattern =  patternsOfOneSkeleton[index];
        if ( !intervalPattern.isEmpty() ) {
            result = intervalPattern;
        }
    }
    return result;
}


UBool
DateIntervalInfo::getDefaultOrder() const {
    return fFirstDateInPtnIsLaterDate;
}


UnicodeString&
DateIntervalInfo::getFallbackIntervalPattern(UnicodeString& result) const {
    result = fFallbackIntervalPattern;
    return result;
}

#define ULOC_LOCALE_IDENTIFIER_CAPACITY (ULOC_FULLNAME_CAPACITY + 1 + ULOC_KEYWORD_AND_VALUES_CAPACITY)


static const int pathPrefixLength = 17;
static const UChar pathPrefix[] = {SOLIDUS, CAP_L, CAP_O, CAP_C, CAP_A, CAP_L, CAP_E, SOLIDUS,
                                    LOW_C, LOW_A, LOW_L, LOW_E, LOW_N, LOW_D, LOW_A, LOW_R, SOLIDUS};
static const int pathSufixLength = 16;
static const UChar pathSufix[] = {SOLIDUS, LOW_I, LOW_N, LOW_T, LOW_E, LOW_R, LOW_V, LOW_A,
                                    LOW_L, CAP_F, LOW_O, LOW_R, LOW_M, LOW_A, LOW_T, LOW_S};

/**
 * Sink for enumerating all of the date interval skeletons.
 * Contains inner sink classes, each one corresponding to a type of resource table.
 * The outer class finds the dateInterval table or an alias.
 */
struct DateIntervalSink : public ResourceTableSink {

    /**
     * Sink to handle each skeleton table.
     */
    struct SkeletonSink : public ResourceTableSink {
        SkeletonSink(DateIntervalSink &sink) : outer(sink) {}
        ~SkeletonSink();

        virtual ResourceTableSink *getOrCreateTableSink(
                const char *key, int32_t, UErrorCode &errorCode) {
            if (U_SUCCESS(errorCode)) {
                outer.currentSkeleton = key;
                return &outer.patternSink;
            }
            return NULL;
        }

        DateIntervalSink &outer;
    } skeletonSink;

    /**
     * Sink to store the date interval pattern for each skeleton pattern character.
     */
    struct PatternSink : public ResourceTableSink {
        PatternSink(DateIntervalSink &sink) : outer(sink) {}
        ~PatternSink();

        virtual void put(const char *key, const ResourceValue &value, UErrorCode &errorCode) {
            // Process the key
            UCalendarDateFields calendarField = validateAndProcessPatternLetter(key);

            // If the calendar field has a valid value
            if (calendarField < UCAL_FIELD_COUNT) {
                // Set the interval pattern
                setIntervalPatternIfAbsent(calendarField, value, errorCode);
            } else {
                errorCode = U_INVALID_FORMAT_ERROR;
            }
        }

        UCalendarDateFields validateAndProcessPatternLetter(const char *patternLetter) {
            // TODO(fabalbon): Check that patternLetter is just one letter?

            // Check that the pattern letter is accepted
            UCalendarDateFields calendarField = UCAL_FIELD_COUNT;
            if ( !uprv_strcmp(patternLetter, "y") ) {
                calendarField = UCAL_YEAR;
            } else if ( !uprv_strcmp(patternLetter, "M") ) {
                calendarField = UCAL_MONTH;
            } else if ( !uprv_strcmp(patternLetter, "d") ) {
                calendarField = UCAL_DATE;
            } else if ( !uprv_strcmp(patternLetter, "a") ) {
                calendarField = UCAL_AM_PM;
            } else if ( !uprv_strcmp(patternLetter, "h") || !uprv_strcmp(patternLetter, "H") ) {
                calendarField = UCAL_HOUR;
            } else if ( !uprv_strcmp(patternLetter, "m") ) {
                calendarField = UCAL_MINUTE; // TODO(fabalbon): Why icu4c doesn't accept the calendar field "s" but icu4j does?
            }
            return calendarField;
        }

        /**
         * Stores the interval pattern for the current skeleton in the internal data structure
         * if it's not present.
         */
        void setIntervalPatternIfAbsent(UCalendarDateFields lrgDiffCalUnit,
                                        const ResourceValue &value, UErrorCode &errorCode) {
            // Check if the pattern has already been stored on the data structure
            DateIntervalInfo::IntervalPatternIndex index =
                    outer.dateIntervalInfo.calendarFieldToIntervalIndex(lrgDiffCalUnit, errorCode);
            if (U_FAILURE(errorCode)) { return; }

            UnicodeString skeleton(outer.currentSkeleton);
            UnicodeString* patternsOfOneSkeleton =
                    (UnicodeString*)(outer.dateIntervalInfo.fIntervalPatterns->get(skeleton));

            if (patternsOfOneSkeleton == NULL || patternsOfOneSkeleton[(int)index].isEmpty()) {
                UnicodeString pattern = value.getUnicodeString(errorCode);
                if (U_FAILURE(errorCode)) { return; }
                outer.dateIntervalInfo.setIntervalPatternInternally(skeleton, lrgDiffCalUnit,
                                                                    pattern, errorCode);
            }
        }

        DateIntervalSink &outer;
    } patternSink;


    DateIntervalSink(DateIntervalInfo &diInfo)
            : skeletonSink(*this), patternSink(*this), dateIntervalInfo(diInfo),
              currentSkeleton(NULL)
    {
        nextCalendarType.setToBogus();
    }
    ~DateIntervalSink();

    virtual void put(const char *key, const ResourceValue &value, UErrorCode &errorCode) {
        // Check if it's an alias of intervalFormats
        if (U_FAILURE(errorCode) || value.getType() != URES_ALIAS
            || uprv_strcmp(key, gIntervalDateTimePatternTag) != 0) {
            return;
        }

        // Get the calendar type for the alias path.
        const UnicodeString aliasPath = value.getAliasUnicodeString(errorCode);
        if (U_FAILURE(errorCode)) { return; }

        nextCalendarType.remove();
        getCalendarTypeFromPath(aliasPath, nextCalendarType, errorCode);

        if (U_FAILURE(errorCode)) {
            resetNextCalendarType();
        }
    }

    virtual ResourceTableSink *getOrCreateTableSink(
            const char *key, int32_t, UErrorCode &errorCode) {
        // Check if it's the intervalFormat table
        if (U_SUCCESS(errorCode) && uprv_strcmp(key, gIntervalDateTimePatternTag) == 0) {
            return &skeletonSink;
        }
        return NULL;
    }

    /**
     * Extracts the calendar type from the path.
     */
    static void getCalendarTypeFromPath(const UnicodeString &path, UnicodeString &calendarType,
                                        UErrorCode &errorCode) {
        if (U_FAILURE(errorCode)) { return; }

        if (!path.startsWith(pathPrefix, pathPrefixLength) || !path.endsWith(pathSufix, pathSufixLength)) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }

        path.extractBetween(pathPrefixLength, path.length() - pathSufixLength, calendarType);
    }

    const UnicodeString &getNextCalendarType() {
        return nextCalendarType;
    }

    void resetNextCalendarType() {
        nextCalendarType.remove();
        nextCalendarType.setToBogus();
    }


    // Output data
    DateIntervalInfo &dateIntervalInfo;

    // Next calendar type
    UnicodeString nextCalendarType;

    // Current skeleton table being enumerated
    const char *currentSkeleton;
};

// Virtual destructors must be defined out of line.
DateIntervalSink::SkeletonSink::~SkeletonSink() {}
DateIntervalSink::PatternSink::~PatternSink() {}
DateIntervalSink::~DateIntervalSink() {}



void 
DateIntervalInfo::initializeData(const Locale& locale, UErrorCode& status)
{
    fIntervalPatterns = initHash(status);
    if ( U_FAILURE(status) ) {
      return;
    }
    const char *locName = locale.getName();

    // Get the correct calendar type
    const char * calendarTypeToUse = gGregorianTag; // initial default
    char         calendarType[ULOC_KEYWORDS_CAPACITY]; // to be filled in with the type to use, if all goes well
    char         localeWithCalendarKey[ULOC_LOCALE_IDENTIFIER_CAPACITY];
    // obtain a locale that always has the calendar key value that should be used
    (void)ures_getFunctionalEquivalent(localeWithCalendarKey, ULOC_LOCALE_IDENTIFIER_CAPACITY, NULL,
                                     "calendar", "calendar", locName, NULL, FALSE, &status);
    localeWithCalendarKey[ULOC_LOCALE_IDENTIFIER_CAPACITY-1] = 0; // ensure null termination
    // now get the calendar key value from that locale
    int32_t calendarTypeLen = uloc_getKeywordValue(localeWithCalendarKey, "calendar", calendarType,
                                                   ULOC_KEYWORDS_CAPACITY, &status);
    if (U_SUCCESS(status) && calendarTypeLen < ULOC_KEYWORDS_CAPACITY) {
        calendarTypeToUse = calendarType;
    }
    status = U_ZERO_ERROR;

    UnicodeString calendarTypeToUseUString(calendarTypeToUse);

    // Instantiate the resource bundles
    UResourceBundle *rb, *calBundle;
    rb = ures_open(NULL, locName, &status);
    if ( U_FAILURE(status) ) {
        return;
    }
    calBundle = ures_getByKey(rb, gCalendarTag, NULL, &status);


    if (U_SUCCESS(status)) {
        UResourceBundle *calTypeBundle, *itvDtPtnResource;

        // Get the fallback pattern
        const UChar* resStr;
        int32_t resStrLen = 0;
        calTypeBundle = ures_getByKey(calBundle, calendarTypeToUse, NULL, &status);
        itvDtPtnResource = ures_getByKeyWithFallback(calTypeBundle,
                                                     gIntervalDateTimePatternTag, NULL, &status);
        resStr = ures_getStringByKeyWithFallback(itvDtPtnResource, gFallbackPatternTag,
                                                 &resStrLen, &status);
        if ( U_SUCCESS(status) ) {
            UnicodeString pattern = UnicodeString(TRUE, resStr, resStrLen);
            setFallbackIntervalPattern(pattern, status);
        }
        ures_close(itvDtPtnResource);
        ures_close(calTypeBundle);


        // Instantiate the sink
        DateIntervalSink sink(*this);

        // Already loaded calendar types
        Hashtable loadedCalendarTypes(FALSE, status);

        if (U_SUCCESS(status)) {
            while (!calendarTypeToUseUString.isBogus()) {
                // Set an error when a loop is detected
                if (loadedCalendarTypes.geti(calendarTypeToUseUString) == 1) {
                    status = U_ILLEGAL_ARGUMENT_ERROR;//TODO(fabalbon): Which error should I use here?
                    break;
                }

                // Register the calendar type to avoid loops
                loadedCalendarTypes.puti(calendarTypeToUseUString, 1, status);
                if (U_FAILURE(status)) {
                    break;
                }

                // Get all resources for this calendar type
                char calType[ULOC_KEYWORDS_CAPACITY];
                calendarTypeToUseUString.extract(0, calendarTypeToUseUString.length(), calType, ULOC_KEYWORDS_CAPACITY);
                calTypeBundle = ures_getByKey(calBundle, calType, NULL, &status);
                ures_getAllTableItemsWithFallback(calTypeBundle, "", sink, status);

                // Get next calendar type to load if there was an alias pointing at it
                const UnicodeString &nextCalendarTypeToUseUString = sink.getNextCalendarType();
                if (!nextCalendarTypeToUseUString.isBogus()) {
                    calendarTypeToUseUString = nextCalendarTypeToUseUString;
                } else {
                    calendarTypeToUseUString.setToBogus();
                }
                sink.resetNextCalendarType();

                // Close the calendar type bundle
                ures_close(calTypeBundle);
            }
        }
    }

    // Close the opened resource bundles
    ures_close(calBundle);
    ures_close(rb);
}

void
DateIntervalInfo::setIntervalPatternInternally(const UnicodeString& skeleton,
                                      UCalendarDateFields lrgDiffCalUnit,
                                      const UnicodeString& intervalPattern,
                                      UErrorCode& status) {

    IntervalPatternIndex index = calendarFieldToIntervalIndex(lrgDiffCalUnit,status);
    if ( U_FAILURE(status) ) {
        return;
    }
    UnicodeString* patternsOfOneSkeleton = (UnicodeString*)(fIntervalPatterns->get(skeleton));
    UBool emptyHash = false;
    if ( patternsOfOneSkeleton == NULL ) {
        patternsOfOneSkeleton = new UnicodeString[kIPI_MAX_INDEX];
        emptyHash = true;
    }
    
    patternsOfOneSkeleton[index] = intervalPattern;
    if ( emptyHash == TRUE ) {
        fIntervalPatterns->put(skeleton, patternsOfOneSkeleton, status);
    }
}



void 
DateIntervalInfo::parseSkeleton(const UnicodeString& skeleton, 
                                int32_t* skeletonFieldWidth) {
    const int8_t PATTERN_CHAR_BASE = 0x41;
    int32_t i;
    for ( i = 0; i < skeleton.length(); ++i ) {
        // it is an ASCII char in skeleton
        int8_t ch = (int8_t)skeleton.charAt(i);  
        ++skeletonFieldWidth[ch - PATTERN_CHAR_BASE];
    }
}



UBool 
DateIntervalInfo::stringNumeric(int32_t fieldWidth, int32_t anotherFieldWidth,
                                char patternLetter) {
    if ( patternLetter == 'M' ) {
        if ( (fieldWidth <= 2 && anotherFieldWidth > 2) ||
             (fieldWidth > 2 && anotherFieldWidth <= 2 )) {
            return true;
        }
    }
    return false;
}



const UnicodeString* 
DateIntervalInfo::getBestSkeleton(const UnicodeString& skeleton,
                                  int8_t& bestMatchDistanceInfo) const {
#ifdef DTITVINF_DEBUG
    char result[1000];
    char result_1[1000];
    char mesg[2000];
    skeleton.extract(0,  skeleton.length(), result, "UTF-8");
    sprintf(mesg, "in getBestSkeleton: skeleton: %s; \n", result);
    PRINTMESG(mesg)
#endif


    int32_t inputSkeletonFieldWidth[] =
    {
    //       A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //   P   Q   R   S   T   U   V   W   X   Y   Z
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0, 0, 0,
    //       a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //   p   q   r   s   t   u   v   w   x   y   z
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };

    int32_t skeletonFieldWidth[] =
    {
    //       A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //   P   Q   R   S   T   U   V   W   X   Y   Z
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0, 0, 0,
    //       a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //   p   q   r   s   t   u   v   w   x   y   z
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };

    const int32_t DIFFERENT_FIELD = 0x1000;
    const int32_t STRING_NUMERIC_DIFFERENCE = 0x100;
    const int32_t BASE = 0x41;
    const UChar CHAR_V = 0x0076;
    const UChar CHAR_Z = 0x007A;

    // hack for 'v' and 'z'.
    // resource bundle only have time skeletons ending with 'v',
    // but not for time skeletons ending with 'z'.
    UBool replaceZWithV = false;
    const UnicodeString* inputSkeleton = &skeleton; 
    UnicodeString copySkeleton;
    if ( skeleton.indexOf(CHAR_Z) != -1 ) {
        copySkeleton = skeleton;
        copySkeleton.findAndReplace(UnicodeString(CHAR_Z), UnicodeString(CHAR_V));
        inputSkeleton = &copySkeleton;
        replaceZWithV = true;
    }

    parseSkeleton(*inputSkeleton, inputSkeletonFieldWidth);
    int32_t bestDistance = MAX_POSITIVE_INT;
    const UnicodeString* bestSkeleton = NULL;

    // 0 means exact the same skeletons;
    // 1 means having the same field, but with different length,
    // 2 means only z/v differs
    // -1 means having different field.
    bestMatchDistanceInfo = 0;
    int8_t fieldLength = sizeof(skeletonFieldWidth)/sizeof(skeletonFieldWidth[0]);

    int32_t pos = UHASH_FIRST;
    const UHashElement* elem = NULL;
    while ( (elem = fIntervalPatterns->nextElement(pos)) != NULL ) {
        const UHashTok keyTok = elem->key;
        UnicodeString* skeleton = (UnicodeString*)keyTok.pointer;
#ifdef DTITVINF_DEBUG
    skeleton->extract(0,  skeleton->length(), result, "UTF-8");
    sprintf(mesg, "available skeletons: skeleton: %s; \n", result);
    PRINTMESG(mesg)
#endif

        // clear skeleton field width
        int8_t i;
        for ( i = 0; i < fieldLength; ++i ) {
            skeletonFieldWidth[i] = 0;    
        }
        parseSkeleton(*skeleton, skeletonFieldWidth);
        // calculate distance
        int32_t distance = 0;
        int8_t fieldDifference = 1;
        for ( i = 0; i < fieldLength; ++i ) {
            int32_t inputFieldWidth = inputSkeletonFieldWidth[i];
            int32_t fieldWidth = skeletonFieldWidth[i];
            if ( inputFieldWidth == fieldWidth ) {
                continue;
            }
            if ( inputFieldWidth == 0 ) {
                fieldDifference = -1;
                distance += DIFFERENT_FIELD;
            } else if ( fieldWidth == 0 ) {
                fieldDifference = -1;
                distance += DIFFERENT_FIELD;
            } else if (stringNumeric(inputFieldWidth, fieldWidth, 
                                     (char)(i+BASE) ) ) {
                distance += STRING_NUMERIC_DIFFERENCE;
            } else {
                distance += (inputFieldWidth > fieldWidth) ? 
                            (inputFieldWidth - fieldWidth) : 
                            (fieldWidth - inputFieldWidth);
            }
        }
        if ( distance < bestDistance ) {
            bestSkeleton = skeleton;
            bestDistance = distance;
            bestMatchDistanceInfo = fieldDifference;
        }
        if ( distance == 0 ) {
            bestMatchDistanceInfo = 0;
            break;
        }
    }
    if ( replaceZWithV && bestMatchDistanceInfo != -1 ) {
        bestMatchDistanceInfo = 2;
    }
    return bestSkeleton;
}



DateIntervalInfo::IntervalPatternIndex
DateIntervalInfo::calendarFieldToIntervalIndex(UCalendarDateFields field, 
                                               UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return kIPI_MAX_INDEX;
    }
    IntervalPatternIndex index = kIPI_MAX_INDEX;
    switch ( field ) {
      case UCAL_ERA:
        index = kIPI_ERA;
        break;
      case UCAL_YEAR:
        index = kIPI_YEAR;
        break;
      case UCAL_MONTH:
        index = kIPI_MONTH;
        break;
      case UCAL_DATE:
      case UCAL_DAY_OF_WEEK:
      //case UCAL_DAY_OF_MONTH:
        index = kIPI_DATE;
        break;
      case UCAL_AM_PM:
        index = kIPI_AM_PM;
        break;
      case UCAL_HOUR:
      case UCAL_HOUR_OF_DAY:
        index = kIPI_HOUR;
        break;
      case UCAL_MINUTE:
        index = kIPI_MINUTE;
        break;
      case UCAL_SECOND:
        index = kIPI_SECOND;
        break;
      default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return index;
}



void
DateIntervalInfo::deleteHash(Hashtable* hTable) 
{
    if ( hTable == NULL ) {
        return;
    }
    int32_t pos = UHASH_FIRST;
    const UHashElement* element = NULL;
    while ( (element = hTable->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const UnicodeString* value = (UnicodeString*)valueTok.pointer;
        delete[] value;
    }
    delete fIntervalPatterns;
}


U_CDECL_BEGIN 

/**
 * set hash table value comparator
 *
 * @param val1  one value in comparison
 * @param val2  the other value in comparison
 * @return      TRUE if 2 values are the same, FALSE otherwise
 */
static UBool U_CALLCONV dtitvinfHashTableValueComparator(UHashTok val1, UHashTok val2);

static UBool 
U_CALLCONV dtitvinfHashTableValueComparator(UHashTok val1, UHashTok val2) {
    const UnicodeString* pattern1 = (UnicodeString*)val1.pointer;
    const UnicodeString* pattern2 = (UnicodeString*)val2.pointer;
    UBool ret = TRUE;
    int8_t i;
    for ( i = 0; i < DateIntervalInfo::kMaxIntervalPatternIndex && ret == TRUE; ++i ) {
        ret = (pattern1[i] == pattern2[i]);
    }
    return ret;
}

U_CDECL_END


Hashtable*
DateIntervalInfo::initHash(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(FALSE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if ( U_FAILURE(status) ) {
        delete hTable; 
        return NULL;
    }
    hTable->setValueComparator(dtitvinfHashTableValueComparator);
    return hTable;
}


void
DateIntervalInfo::copyHash(const Hashtable* source,
                           Hashtable* target,
                           UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = UHASH_FIRST;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;
            const UHashTok valueTok = element->value;
            const UnicodeString* value = (UnicodeString*)valueTok.pointer;
            UnicodeString* copy = new UnicodeString[kIPI_MAX_INDEX];
            int8_t i;
            for ( i = 0; i < kIPI_MAX_INDEX; ++i ) {
                copy[i] = value[i];
            }
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}


U_NAMESPACE_END

#endif
