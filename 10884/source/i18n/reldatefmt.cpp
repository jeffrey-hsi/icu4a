/*
******************************************************************************
* Copyright (C) 2014, International Business Machines Corporation and         
* others. All Rights Reserved.                                                
******************************************************************************
*                                                                             
* File RELDATEFMT.CPP                                                             
******************************************************************************
*/

#include "unicode/reldatefmt.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"
#include "quantityformatter.h"
#include "unicode/plurrule.h"
#include "unicode/msgfmt.h"
#include "unicode/decimfmt.h"
#include "unicode/numfmt.h"
#include "lrucache.h"
#include "uresimp.h"
#include "unicode/ures.h"
#include "cstring.h"
#include "ucln_in.h"
#include "mutex.h"
#include "charstr.h"

#include "sharedptr.h"
#include "sharedpluralrules.h"
#include "sharednumberformat.h"

#define RELDATE_STYLE_FULL 0
#define RELDATE_STYLE_SHORT 1
#define RELDATE_STYLE_NARROW 2
#define RELDATE_STYLE_COUNT 3 

// Copied from uscript_props.cpp
#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static icu::LRUCache *gCache = NULL;
static UMutex gCacheMutex = U_MUTEX_INITIALIZER;
static icu::UInitOnce gCacheInitOnce = U_INITONCE_INITIALIZER;

U_CDECL_BEGIN
static UBool U_CALLCONV reldatefmt_cleanup() {
    gCacheInitOnce.reset();
    if (gCache) {
        delete gCache;
        gCache = NULL;
    }
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN

static int32_t getStyleIndex(UDateFormatStyle style) {
    switch (style) {
        case UDAT_FULL:
        case UDAT_LONG:
            return 0;
        case UDAT_MEDIUM:
            return 1;
        case UDAT_SHORT:
            return 2;
        default:
            return 0;
    }
}

// RelativeDateTimeFormatter specific data for a single locale
class RelativeDateTimeCacheData: public SharedObject {
public:
    RelativeDateTimeCacheData() : combinedDateAndTime(NULL) { }
    virtual ~RelativeDateTimeCacheData();

    // no numbers: e.g Next Tuesday; Yesterday; etc.
    UnicodeString absoluteUnits[RELDATE_STYLE_COUNT][UDAT_ABSOLUTE_UNIT_COUNT][UDAT_DIRECTION_COUNT];

    // has numbers: e.g Next Tuesday; Yesterday; etc. For second index, 0
    // means past e.g 5 days ago; 1 means future e.g in 5 days.
    QuantityFormatter relativeUnits[RELDATE_STYLE_COUNT][UDAT_RELATIVE_UNIT_COUNT][2];

    void adoptCombinedDateAndTime(MessageFormat *mfToAdopt) {
        delete combinedDateAndTime;
        combinedDateAndTime = mfToAdopt;
    }
    const MessageFormat *getCombinedDateAndTime() const {
        return combinedDateAndTime;
    }
private:
    MessageFormat *combinedDateAndTime;
    RelativeDateTimeCacheData(const RelativeDateTimeCacheData &other);
    RelativeDateTimeCacheData& operator=(
            const RelativeDateTimeCacheData &other);
};

RelativeDateTimeCacheData::~RelativeDateTimeCacheData() {
    delete combinedDateAndTime;
}

static UBool getStringWithFallback(
        const UResourceBundle *resource, 
        const char *key,
        UnicodeString &result,
        UErrorCode &status) {
    int32_t len = 0;
    const UChar *resStr = ures_getStringByKeyWithFallback(
        resource, key, &len, &status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    result.setTo(TRUE, resStr, len);
    return TRUE;
}

static UBool getOptionalStringWithFallback(
        const UResourceBundle *resource, 
        const char *key,
        UnicodeString &result,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    int32_t len = 0;
    const UChar *resStr = ures_getStringByKey(
        resource, key, &len, &status);
    if (status == U_MISSING_RESOURCE_ERROR) {
        result.remove();
        status = U_ZERO_ERROR;
        return TRUE;
    }
    if (U_FAILURE(status)) {
        return FALSE;
    }
    result.setTo(TRUE, resStr, len);
    return TRUE;
}

static UBool getString(
        const UResourceBundle *resource, 
        UnicodeString &result,
        UErrorCode &status) {
    int32_t len = 0;
    const UChar *resStr = ures_getString(resource, &len, &status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    result.setTo(TRUE, resStr, len);
    return TRUE;
}

static UBool getStringByIndex(
        const UResourceBundle *resource, 
        int32_t idx,
        UnicodeString &result,
        UErrorCode &status) {
    int32_t len = 0;
    const UChar *resStr = ures_getStringByIndex(
            resource, idx, &len, &status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    result.setTo(TRUE, resStr, len);
    return TRUE;
}

static void initAbsoluteUnit(
            const UResourceBundle *resource,
            const UnicodeString &unitName,
            UnicodeString *absoluteUnit,
            UErrorCode &status) {
    getStringWithFallback(
            resource, 
            "-1",
            absoluteUnit[UDAT_DIRECTION_LAST],
            status);
    getStringWithFallback(
            resource, 
            "0",
            absoluteUnit[UDAT_DIRECTION_THIS],
            status);
    getStringWithFallback(
            resource, 
            "1",
            absoluteUnit[UDAT_DIRECTION_NEXT],
            status);
    getOptionalStringWithFallback(
            resource,
            "-2",
            absoluteUnit[UDAT_DIRECTION_LAST_2],
            status);
    getOptionalStringWithFallback(
            resource,
            "2",
            absoluteUnit[UDAT_DIRECTION_NEXT_2],
            status);
    absoluteUnit[UDAT_DIRECTION_PLAIN] = unitName;
}

static void initQuantityFormatter(
        const UResourceBundle *resource,
        QuantityFormatter &formatter,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    int32_t size = ures_getSize(resource);
    for (int32_t i = 0; i < size; ++i) {
        LocalUResourceBundlePointer pluralBundle(
                ures_getByIndex(resource, i, NULL, &status));
        if (U_FAILURE(status)) {
            return;
        }
        UnicodeString rawPattern;
        if (!getString(pluralBundle.getAlias(), rawPattern, status)) {
            return;
        }
        if (!formatter.add(
                ures_getKey(pluralBundle.getAlias()),
                rawPattern,
                status)) {
            return;
        }
    }
}

static void initRelativeUnit(
        const UResourceBundle *resource,
        QuantityFormatter *relativeUnit,
        UErrorCode &status) {
    LocalUResourceBundlePointer topLevel(
            ures_getByKeyWithFallback(
                    resource, "relativeTime", NULL, &status));
    if (U_FAILURE(status)) {
        return;
    }
    LocalUResourceBundlePointer futureBundle(ures_getByKeyWithFallback(
            topLevel.getAlias(), "future", NULL, &status));
    if (U_FAILURE(status)) {
        return;
    }
    initQuantityFormatter(
            futureBundle.getAlias(),
            relativeUnit[1],
            status);
    LocalUResourceBundlePointer pastBundle(ures_getByKeyWithFallback(
            topLevel.getAlias(), "past", NULL, &status));
    if (U_FAILURE(status)) {
        return;
    }
    initQuantityFormatter(
            pastBundle.getAlias(),
            relativeUnit[0],
            status);
}

static void initRelativeUnit(
        const UResourceBundle *resource,
        const char *path,
        QuantityFormatter *relativeUnit,
        UErrorCode &status) {
    LocalUResourceBundlePointer topLevel(
            ures_getByKeyWithFallback(resource, path, NULL, &status));
    if (U_FAILURE(status)) {
        return;
    }
    initRelativeUnit(topLevel.getAlias(), relativeUnit, status);
}

static void addTimeUnit(
        const UResourceBundle *resource,
        const char *path,
        QuantityFormatter *relativeUnit,
        UnicodeString *absoluteUnit,
        UErrorCode &status) {
    LocalUResourceBundlePointer topLevel(
            ures_getByKeyWithFallback(resource, path, NULL, &status));
    if (U_FAILURE(status)) {
        return;
    }
    initRelativeUnit(topLevel.getAlias(), relativeUnit, status);
    UnicodeString unitName;
    if (!getStringWithFallback(topLevel.getAlias(), "dn", unitName, status)) {
        return;
    }
    // TODO(Travis Keep): This is a hack to get around CLDR bug 6818.
    const char *localeId = ures_getLocaleByType(
            topLevel.getAlias(), ULOC_ACTUAL_LOCALE, &status);
    if (U_FAILURE(status)) {
        return;
    }
    Locale locale(localeId);
    if (uprv_strcmp("en", locale.getLanguage()) == 0) {
         unitName.toLower();
    }
    // end hack
    ures_getByKeyWithFallback(
            topLevel.getAlias(), "relative", topLevel.getAlias(), &status);
    if (U_FAILURE(status)) {
        return;
    }
    initAbsoluteUnit(
            topLevel.getAlias(),
            unitName,
            absoluteUnit,
            status);
}

static void readDaysOfWeek(
        const UResourceBundle *resource,
        const char *path,
        UnicodeString *daysOfWeek,
        UErrorCode &status) {
    LocalUResourceBundlePointer topLevel(
            ures_getByKeyWithFallback(resource, path, NULL, &status));
    if (U_FAILURE(status)) {
        return;
    }
    int32_t size = ures_getSize(topLevel.getAlias());
    if (size != 7) {
        status = U_INTERNAL_PROGRAM_ERROR;
        return;
    }
    for (int32_t i = 0; i < size; ++i) {
        if (!getStringByIndex(topLevel.getAlias(), i, daysOfWeek[i], status)) {
            return;
        }
    }
}

static void addWeekDay(
        const UResourceBundle *resource,
        const char *path,
        const UnicodeString *daysOfWeek,
        UDateAbsoluteUnit absoluteUnit,
        UnicodeString absoluteUnits[][UDAT_DIRECTION_COUNT],
        UErrorCode &status) {
    LocalUResourceBundlePointer topLevel(
            ures_getByKeyWithFallback(resource, path, NULL, &status));
    if (U_FAILURE(status)) {
        return;
    }
    initAbsoluteUnit(
            topLevel.getAlias(),
            daysOfWeek[absoluteUnit - UDAT_ABSOLUTE_SUNDAY],
            absoluteUnits[absoluteUnit],
            status);
}

static void addTimeUnits(
        const UResourceBundle *resource,
        const char *path, const char *pathShort, const char *pathNarrow,
        UDateRelativeUnit relativeUnit,
        UDateAbsoluteUnit absoluteUnit,
        RelativeDateTimeCacheData &cacheData,
        UErrorCode &status) {
    addTimeUnit(
        resource,
        path,
        cacheData.relativeUnits[0][relativeUnit],
        cacheData.absoluteUnits[0][absoluteUnit],
        status);
    addTimeUnit(
        resource,
        pathShort,
        cacheData.relativeUnits[1][relativeUnit],
        cacheData.absoluteUnits[1][absoluteUnit],
        status);
    addTimeUnit(
        resource,
        pathNarrow,
        cacheData.relativeUnits[2][relativeUnit],
        cacheData.absoluteUnits[2][absoluteUnit],
        status);
}

static void initRelativeUnits(
        const UResourceBundle *resource,
        const char *path, const char *pathShort, const char *pathNarrow,
        UDateRelativeUnit relativeUnit,
        QuantityFormatter relativeUnits[][UDAT_RELATIVE_UNIT_COUNT][2],
        UErrorCode &status) {
    initRelativeUnit(
            resource,
            path,
            relativeUnits[0][relativeUnit],
            status);
    initRelativeUnit(
            resource,
            pathShort,
            relativeUnits[1][relativeUnit],
            status);
    initRelativeUnit(
            resource,
            pathNarrow,
            relativeUnits[2][relativeUnit],
            status);
}

static void addWeekDays(
        const UResourceBundle *resource,
        const char *path, const char *pathShort, const char *pathNarrow,
        const UnicodeString daysOfWeek[][7],
        UDateAbsoluteUnit absoluteUnit,
        UnicodeString absoluteUnits[][UDAT_ABSOLUTE_UNIT_COUNT][UDAT_DIRECTION_COUNT],
        UErrorCode &status) {
    addWeekDay(
            resource,
            path,
            daysOfWeek[0],
            absoluteUnit,
            absoluteUnits[0],
            status);
    addWeekDay(
            resource,
            pathShort,
            daysOfWeek[1],
            absoluteUnit,
            absoluteUnits[1],
            status);
    addWeekDay(
            resource,
            pathNarrow,
            daysOfWeek[2],
            absoluteUnit,
            absoluteUnits[2],
            status);
}

static UBool loadUnitData(
        const UResourceBundle *resource,
        RelativeDateTimeCacheData &cacheData,
        UErrorCode &status) {
    addTimeUnits(
            resource,
            "fields/day", "fields/day-short", "fields/day-narrow",
            UDAT_RELATIVE_DAYS,
            UDAT_ABSOLUTE_DAY,
            cacheData,
            status);
    addTimeUnits(
            resource,
            "fields/week", "fields/week-short", "fields/week-narrow",
            UDAT_RELATIVE_WEEKS,
            UDAT_ABSOLUTE_WEEK,
            cacheData,
            status);
    addTimeUnits(
            resource,
            "fields/month", "fields/month-short", "fields/month-narrow",
            UDAT_RELATIVE_MONTHS,
            UDAT_ABSOLUTE_MONTH,
            cacheData,
            status);
    addTimeUnits(
            resource,
            "fields/year", "fields/year-short", "fields/year-narrow",
            UDAT_RELATIVE_YEARS,
            UDAT_ABSOLUTE_YEAR,
            cacheData,
            status);
    initRelativeUnits(
            resource,
            "fields/second", "fields/second-short", "fields/second-narrow",
            UDAT_RELATIVE_SECONDS,
            cacheData.relativeUnits,
            status);
    initRelativeUnits(
            resource,
            "fields/minute", "fields/minute-short", "fields/minute-narrow",
            UDAT_RELATIVE_MINUTES,
            cacheData.relativeUnits,
            status);
    initRelativeUnits(
            resource,
            "fields/hour", "fields/hour-short", "fields/hour-narrow",
            UDAT_RELATIVE_HOURS,
            cacheData.relativeUnits,
            status);
    getStringWithFallback(
            resource,
            "fields/second/relative/0",
            cacheData.absoluteUnits[0][UDAT_ABSOLUTE_NOW][UDAT_DIRECTION_PLAIN],
            status);
    getStringWithFallback(
            resource,
            "fields/second-short/relative/0",
            cacheData.absoluteUnits[1][UDAT_ABSOLUTE_NOW][UDAT_DIRECTION_PLAIN],
            status);
    getStringWithFallback(
            resource,
            "fields/second-narrow/relative/0",
            cacheData.absoluteUnits[2][UDAT_ABSOLUTE_NOW][UDAT_DIRECTION_PLAIN],
            status);
    UnicodeString daysOfWeek[3][7];
    readDaysOfWeek(
            resource,
            "calendar/gregorian/dayNames/stand-alone/wide",
            daysOfWeek[0],
            status);
    readDaysOfWeek(
            resource,
            "calendar/gregorian/dayNames/stand-alone/short",
            daysOfWeek[1],
            status);
    readDaysOfWeek(
            resource,
            "calendar/gregorian/dayNames/stand-alone/narrow",
            daysOfWeek[2],
            status);
    addWeekDays(
            resource,
            "fields/mon/relative",
            "fields/mon-short/relative",
            "fields/mon-narrow/relative",
            daysOfWeek,
            UDAT_ABSOLUTE_MONDAY,
            cacheData.absoluteUnits,
            status);
    addWeekDays(
            resource,
            "fields/tue/relative",
            "fields/tue-short/relative",
            "fields/tue-narrow/relative",
            daysOfWeek,
            UDAT_ABSOLUTE_TUESDAY,
            cacheData.absoluteUnits,
            status);
    addWeekDays(
            resource,
            "fields/wed/relative",
            "fields/wed-short/relative",
            "fields/wed-narrow/relative",
            daysOfWeek,
            UDAT_ABSOLUTE_WEDNESDAY,
            cacheData.absoluteUnits,
            status);
    addWeekDays(
            resource,
            "fields/thu/relative",
            "fields/thu-short/relative",
            "fields/thu-narrow/relative",
            daysOfWeek,
            UDAT_ABSOLUTE_THURSDAY,
            cacheData.absoluteUnits,
            status);
    addWeekDays(
            resource,
            "fields/fri/relative",
            "fields/fri-short/relative",
            "fields/fri-narrow/relative",
            daysOfWeek,
            UDAT_ABSOLUTE_FRIDAY,
            cacheData.absoluteUnits,
            status);
    addWeekDays(
            resource,
            "fields/sat/relative",
            "fields/sat-short/relative",
            "fields/sat-narrow/relative",
            daysOfWeek,
            UDAT_ABSOLUTE_SATURDAY,
            cacheData.absoluteUnits,
            status);
    addWeekDays(
            resource,
            "fields/sun/relative",
            "fields/sun-short/relative",
            "fields/sun-narrow/relative",
            daysOfWeek,
            UDAT_ABSOLUTE_SUNDAY,
            cacheData.absoluteUnits,
            status);
    return U_SUCCESS(status);
}

static UBool getDateTimePattern(
        const UResourceBundle *resource,
        UnicodeString &result,
        UErrorCode &status) {
    UnicodeString defaultCalendarName;
    if (!getStringWithFallback(
            resource,
            "calendar/default",
            defaultCalendarName,
            status)) {
        return FALSE;
    }
    CharString pathBuffer;
    pathBuffer.append("calendar/", status)
            .appendInvariantChars(defaultCalendarName, status)
            .append("/DateTimePatterns", status);
    LocalUResourceBundlePointer topLevel(
            ures_getByKeyWithFallback(
                    resource, pathBuffer.data(), NULL, &status));
    if (U_FAILURE(status)) {
        return FALSE;
    }
    int32_t size = ures_getSize(topLevel.getAlias());
    if (size <= 8) {
        // Oops, size is to small to access the index that we want, fallback
        // to a hard-coded value.
        result = UNICODE_STRING_SIMPLE("{1} {0}");
        return TRUE;
    }
    return getStringByIndex(topLevel.getAlias(), 8, result, status);
}

// Creates RelativeDateTimeFormatter specific data for a given locale
static SharedObject *U_CALLCONV createData(
        const char *localeId, UErrorCode &status) {
    LocalUResourceBundlePointer topLevel(ures_open(NULL, localeId, &status));
    if (U_FAILURE(status)) {
        return NULL;
    }
    LocalPointer<RelativeDateTimeCacheData> result(
            new RelativeDateTimeCacheData());
    if (result.isNull()) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if (!loadUnitData(
            topLevel.getAlias(),
            *result,
            status)) {
        return NULL;
    }
    UnicodeString dateTimePattern;
    if (!getDateTimePattern(topLevel.getAlias(), dateTimePattern, status)) {
        return NULL;
    }
    result->adoptCombinedDateAndTime(
            new MessageFormat(dateTimePattern, localeId, status));
    if (U_FAILURE(status)) {
        return NULL;
    }
    return result.orphan();
}

static void U_CALLCONV cacheInit(UErrorCode &status) {
    U_ASSERT(gCache == NULL);
    ucln_i18n_registerCleanup(UCLN_I18N_RELDATEFMT, reldatefmt_cleanup);
    gCache = new SimpleLRUCache(100, &createData, status);
    if (U_FAILURE(status)) {
        delete gCache;
        gCache = NULL;
    }
}

static UBool getFromCache(
        const char *locale,
        const RelativeDateTimeCacheData *&ptr,
        UErrorCode &status) {
    umtx_initOnce(gCacheInitOnce, &cacheInit, status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    Mutex lock(&gCacheMutex);
    gCache->get(locale, ptr, status);
    return U_SUCCESS(status);
}

RelativeDateTimeFormatter::RelativeDateTimeFormatter(UErrorCode& status)
        : cache(NULL), numberFormat(NULL), pluralRules(NULL), style(UDAT_FULL) {
    init(Locale::getDefault(), NULL, status);
}

RelativeDateTimeFormatter::RelativeDateTimeFormatter(
        const Locale& locale, UErrorCode& status)
        : cache(NULL), numberFormat(NULL), pluralRules(NULL), style(UDAT_FULL) {
    init(locale, NULL, status);
}

RelativeDateTimeFormatter::RelativeDateTimeFormatter(
        const Locale& locale, NumberFormat *nfToAdopt, UErrorCode& status)
        : cache(NULL), numberFormat(NULL), pluralRules(NULL), style(UDAT_FULL) {
    init(locale, nfToAdopt, status);
}

RelativeDateTimeFormatter::RelativeDateTimeFormatter(
        const Locale& locale,
        NumberFormat *nfToAdopt,
        UDateFormatStyle styl,
        UDisplayContext capitalizationContext,
        UErrorCode& status)
        : cache(NULL), numberFormat(NULL), pluralRules(NULL), style(styl) {
  init(locale, nfToAdopt, status);
}

RelativeDateTimeFormatter::RelativeDateTimeFormatter(
        const RelativeDateTimeFormatter& other)
        : cache(other.cache),
          numberFormat(other.numberFormat),
          pluralRules(other.pluralRules),
          style(other.style) {
    cache->addRef();
    numberFormat->addRef();
    pluralRules->addRef();
}

RelativeDateTimeFormatter& RelativeDateTimeFormatter::operator=(
        const RelativeDateTimeFormatter& other) {
    if (this != &other) {
        SharedObject::copyPtr(other.cache, cache);
        SharedObject::copyPtr(other.numberFormat, numberFormat);
        SharedObject::copyPtr(other.pluralRules, pluralRules);
        style = other.style;
    }
    return *this;
}

RelativeDateTimeFormatter::~RelativeDateTimeFormatter() {
    if (cache != NULL) {
        cache->removeRef();
    }
    if (numberFormat != NULL) {
        numberFormat->removeRef();
    }
    if (pluralRules != NULL) {
        pluralRules->removeRef();
    }
}

const NumberFormat& RelativeDateTimeFormatter::getNumberFormat() const {
    return **numberFormat;
}

UDisplayContext RelativeDateTimeFormatter::getCapitalizationContext() const {
    return (UDisplayContext) 0;
}

// TODO: add test.
UDateFormatStyle RelativeDateTimeFormatter::getFormatStyle() const {
    return style;
}

UnicodeString& RelativeDateTimeFormatter::format(
        double quantity, UDateDirection direction, UDateRelativeUnit unit,
        UnicodeString& appendTo, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    if (direction != UDAT_DIRECTION_LAST && direction != UDAT_DIRECTION_NEXT) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    int32_t bFuture = direction == UDAT_DIRECTION_NEXT ? 1 : 0;
    FieldPosition pos(FieldPosition::DONT_CARE);
    return cache->relativeUnits[getStyleIndex(style)][unit][bFuture].format(
            quantity,
            **numberFormat,
            **pluralRules,
            appendTo,
            pos,
            status);
}

UnicodeString& RelativeDateTimeFormatter::format(
        UDateDirection direction, UDateAbsoluteUnit unit,
        UnicodeString& appendTo, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    if (unit == UDAT_ABSOLUTE_NOW && direction != UDAT_DIRECTION_PLAIN) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    return appendTo.append(cache->absoluteUnits[getStyleIndex(style)][unit][direction]);
}

UnicodeString& RelativeDateTimeFormatter::combineDateAndTime(
    const UnicodeString& relativeDateString, const UnicodeString& timeString,
    UnicodeString& appendTo, UErrorCode& status) const {
    Formattable args[2] = {timeString, relativeDateString};
    FieldPosition fpos(0);
    return cache->getCombinedDateAndTime()->format(
            args, 2, appendTo, fpos, status);
}

void RelativeDateTimeFormatter::init(
        const Locale &locale, NumberFormat *nfToAdopt, UErrorCode &status) {
    LocalPointer<NumberFormat> nf(nfToAdopt);
    if (!getFromCache(locale.getName(), cache, status)) {
        return;
    }
    SharedObject::copyPtr(
            PluralRules::createSharedInstance(
                    locale, UPLURAL_TYPE_CARDINAL, status),
            pluralRules);
    if (U_FAILURE(status)) {
        return;
    }
    pluralRules->removeRef();
    if (nf.isNull()) {
       SharedObject::copyPtr(
               NumberFormat::createSharedInstance(
                       locale, UNUM_DECIMAL, status),
               numberFormat);
        if (U_FAILURE(status)) {
            return;
        }
        numberFormat->removeRef();
    } else {
        SharedNumberFormat *shared = new SharedNumberFormat(nf.getAlias());
        if (shared == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        nf.orphan();
        SharedObject::copyPtr(shared, numberFormat);
    }
}


U_NAMESPACE_END

#endif /* !UCONFIG_NO_FORMATTING */

