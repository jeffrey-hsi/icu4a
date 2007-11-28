/*
*******************************************************************************
* Copyright (C) 2007, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "zonemeta.h"

#include "unicode/timezone.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"

#include "umutex.h"
#include "uvector.h"
#include "cmemory.h"
#include "gregoimp.h"
#include "cstring.h"
#include "ucln_in.h"

static UBool gZoneMetaInitialized = FALSE;

// Metazone mapping tables
static UMTX gZoneMetaLock = NULL;
static U_NAMESPACE_QUALIFIER Hashtable *gCanonicalMap = NULL;
static U_NAMESPACE_QUALIFIER Hashtable *gOlsonToMeta = NULL;
static U_NAMESPACE_QUALIFIER Hashtable *gMetaToOlson = NULL;

U_CDECL_BEGIN
/**
 * Cleanup callback func
 */
static UBool U_CALLCONV zoneMeta_cleanup(void)
{
     umtx_destroy(&gZoneMetaLock);

    if (gCanonicalMap != NULL) {
        delete (U_NAMESPACE_QUALIFIER Hashtable*) gCanonicalMap;
        gCanonicalMap = NULL;
    }

    if (gOlsonToMeta != NULL) {
        delete (U_NAMESPACE_QUALIFIER Hashtable*) gOlsonToMeta;
        gOlsonToMeta = NULL;
    }

    if (gMetaToOlson != NULL) {
        delete (U_NAMESPACE_QUALIFIER Hashtable*) gMetaToOlson;
        gMetaToOlson = NULL;
    }

    gZoneMetaInitialized = FALSE;

    return TRUE;
}

/**
 * Deleter for UVector
 */
static void U_CALLCONV
deleteUVector(void *obj) {
   delete (U_NAMESPACE_QUALIFIER UVector*) obj;
}

/**
 * Deleter for CanonicalMapEntry
 */
static void U_CALLCONV
deleteCanonicalMapEntry(void *obj) {
    U_NAMESPACE_QUALIFIER CanonicalMapEntry *entry = (U_NAMESPACE_QUALIFIER CanonicalMapEntry*)obj;
    uprv_free(entry->id);
    if (entry->country != NULL) {
        uprv_free(entry->country);
    }
    uprv_free(entry);
}

/**
 * Deleter for OlsonToMetaMappingEntry
 */
static void U_CALLCONV
deleteOlsonToMetaMappingEntry(void *obj) {
    U_NAMESPACE_QUALIFIER OlsonToMetaMappingEntry *entry = (U_NAMESPACE_QUALIFIER OlsonToMetaMappingEntry*)obj;
    uprv_free(entry->mzid);
    uprv_free(entry);
}

/**
 * Deleter for MetaToOlsonMappingEntry
 */
static void U_CALLCONV
deleteMetaToOlsonMappingEntry(void *obj) {
    U_NAMESPACE_QUALIFIER MetaToOlsonMappingEntry *entry = (U_NAMESPACE_QUALIFIER MetaToOlsonMappingEntry*)obj;
    uprv_free(entry->id);
    uprv_free(entry->territory);
    uprv_free(entry);
}
U_CDECL_END

U_NAMESPACE_BEGIN

#define ZID_KEY_MAX 128
static const char gZoneStringsTag[]     = "zoneStrings";
static const char gUseMetazoneTag[]     = "um";

static const char gSupplementalData[]   = "supplementalData";
static const char gMapTimezonesTag[]    = "mapTimezones";
static const char gMetazonesTag[]       = "metazones";
static const char gZoneFormattingTag[]  = "zoneFormatting";
static const char gTerritoryTag[]       = "territory";
static const char gAliasesTag[]         = "aliases";
static const char gMultizoneTag[]       = "multizone";

static const char gMetazoneInfo[]       = "metazoneInfo";
static const char gMetazoneMappings[]   = "metazoneMappings";

#define MZID_PREFIX_LEN 5
static const char gMetazoneIdPrefix[]   = "meta:";

static const UChar gWorld[] = {0x30, 0x30, 0x31, 0x00}; // "001"

#define ASCII_DIGIT(c) (((c)>=0x30 && (c)<=0x39) ? (c)-0x30 : -1)

/*
 * Convert a date string used by metazone mappings to UDate.
 * The format used by CLDR metazone mapping is "yyyy-MM-dd HH:mm".
 */
 static UDate parseDate (const UChar *text, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return 0;
    }
    int32_t year = 0, month = 0, day = 0, hour = 0, min = 0, n;
    int32_t idx;

    // "yyyy" (0 - 3)
    for (idx = 0; idx <= 3 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            year = 10*year + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "MM" (5 - 6)
    for (idx = 5; idx <= 6 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            month = 10*month + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "dd" (8 - 9)
    for (idx = 8; idx <= 9 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            day = 10*day + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "HH" (11 - 12)
    for (idx = 11; idx <= 12 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            hour = 10*hour + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "mm" (14 - 15)
    for (idx = 14; idx <= 15 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            min = 10*min + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }

    if (U_SUCCESS(status)) {
        UDate date = Grego::fieldsToDay(year, month - 1, day) * U_MILLIS_PER_DAY
            + hour * U_MILLIS_PER_HOUR + min * U_MILLIS_PER_MINUTE;
        return date;
    }
    return 0;
}

/*
 * Initialize global objects
 */
void
ZoneMeta::initialize(void) {
    UBool initialized;
    UMTX_CHECK(&gZoneMetaLock, gZoneMetaInitialized, initialized);
    if (initialized) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;

    // Initialize hash tables
    Hashtable *tmpCanonicalMap = createCanonicalMap();
    Hashtable *tmpOlsonToMeta = createOlsonToMetaMap();
    if (tmpOlsonToMeta == NULL) {
        // With ICU 3.8 data
        createOlsonToMetaMapOld();
    }
    Hashtable *tmpMetaToOlson = createMetaToOlsonMap();

    umtx_lock(&gZoneMetaLock);
    if (gZoneMetaInitialized) {
        // Another thread already created mappings
        delete tmpCanonicalMap;
        delete tmpOlsonToMeta;
        delete tmpMetaToOlson;
    } else {
        gZoneMetaInitialized = TRUE;
        gCanonicalMap = tmpCanonicalMap;
        gOlsonToMeta = tmpOlsonToMeta;
        gMetaToOlson = tmpMetaToOlson;
        ucln_i18n_registerCleanup(UCLN_I18N_ZONEMETA, zoneMeta_cleanup);
    }
    umtx_unlock(&gZoneMetaLock);
}

Hashtable*
ZoneMeta::createCanonicalMap(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *canonicalMap = NULL;
    UResourceBundle *supplementalDataBundle = NULL;
    UResourceBundle *zoneFormatting = NULL;
    UResourceBundle *tzitem = NULL;
    UResourceBundle *aliases = NULL;

    canonicalMap = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    canonicalMap->setValueDeleter(deleteCanonicalMapEntry);

    supplementalDataBundle = ures_openDirect(NULL, gSupplementalData, &status);
    zoneFormatting = ures_getByKey(supplementalDataBundle, gZoneFormattingTag, NULL, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    while (ures_hasNext(zoneFormatting)) {
        tzitem = ures_getNextResource(zoneFormatting, NULL, &status);
        if (U_FAILURE(status)) {
            ures_close(tzitem);
            tzitem = NULL;
            status = U_ZERO_ERROR;
            continue;
        }
        if (ures_getType(tzitem) != URES_TABLE) {
            ures_close(tzitem);
            tzitem = NULL;
            continue;
        }

        int32_t territoryLen;
        const UChar *territory = ures_getStringByKey(tzitem, gTerritoryTag, &territoryLen, &status);
        if (U_FAILURE(status)) {
            territory = NULL;
            status = U_ZERO_ERROR;
        }

        int32_t tzidLen = 0;
        char tzid[ZID_KEY_MAX];
        const char *tzkey = ures_getKey(tzitem);
        uprv_strcpy(tzid, tzkey);
        // Replace ':' with '/'
        char *p = tzid;
        while (*p) {
            if (*p == ':') {
                *p = '/';
            }
            p++;
            tzidLen++;
        }

        // Create canonical map entry
        CanonicalMapEntry *entry = (CanonicalMapEntry*)uprv_malloc(sizeof(CanonicalMapEntry));
        if (entry == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            goto error_cleanup;
        }
        entry->id = (UChar*)uprv_malloc((tzidLen + 1) * sizeof(UChar));
        if (entry->id == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(entry);
            goto error_cleanup;
        }
        u_charsToUChars(tzid, entry->id, tzidLen + 1);

        if (territory  == NULL || u_strcmp(territory, gWorld) == 0) {
            entry->country = NULL;
        } else {
            entry->country = (UChar*)uprv_malloc((territoryLen + 1) * sizeof(UChar));
            if (entry->country == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteCanonicalMapEntry(entry);
                goto error_cleanup;
            }
            u_strcpy(entry->country, territory);
        }

        // Put this entry to the table
        canonicalMap->put(UnicodeString(tzid), entry, status);
        if (U_FAILURE(status)) {
            deleteCanonicalMapEntry(entry);
            goto error_cleanup;
        }

        // Get aliases
        aliases = ures_getByKey(tzitem, gAliasesTag, NULL, &status);
        if (U_FAILURE(status)) {
            // No aliases
            status = U_ZERO_ERROR;
            ures_close(tzitem);
            continue;
        }

        while (ures_hasNext(aliases)) {
            int32_t aliasLen;
            const UChar* alias = ures_getNextString(aliases, &aliasLen, NULL, &status);
            if (U_FAILURE(status)) {
                status = U_ZERO_ERROR;
                continue;
            }
            // Create canonical map entry for this alias
            entry = (CanonicalMapEntry*)uprv_malloc(sizeof(CanonicalMapEntry));
            if (entry == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                goto error_cleanup;
            }
            entry->id = (UChar*)uprv_malloc((tzidLen + 1) * sizeof(UChar));
            if (entry->id == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                uprv_free(entry);
                goto error_cleanup;
            }
            u_charsToUChars(tzid, entry->id, tzidLen + 1);

            if (territory  == NULL || u_strcmp(territory, gWorld) == 0) {
                entry->country = NULL;
            } else {
                entry->country = (UChar*)uprv_malloc((territoryLen + 1) * sizeof(UChar));
                if (entry->country == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    deleteCanonicalMapEntry(entry);
                    goto error_cleanup;
                }
                u_strcpy(entry->country, territory);
            }
            canonicalMap->put(UnicodeString(alias), entry, status);
            if (U_FAILURE(status)) {
                deleteCanonicalMapEntry(entry);
                goto error_cleanup;
            }
        }

        ures_close(aliases);
        ures_close(tzitem);
    }

    ures_close(zoneFormatting);
    ures_close(supplementalDataBundle);
    return canonicalMap;

error_cleanup:
    ures_close(aliases);
    ures_close(tzitem);
    ures_close(zoneFormatting);
    ures_close(supplementalDataBundle);
    delete canonicalMap;

    return NULL;
}

/*
 * Creating Olson tzid to metazone mappings from resource (3.8.1 and beyond)
 */
Hashtable*
ZoneMeta::createOlsonToMetaMap(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *olsonToMeta = NULL;
    UResourceBundle *metazoneInfoBundle = NULL;
    UResourceBundle *metazoneMappings = NULL;
    StringEnumeration *tzids = NULL;

    olsonToMeta = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    olsonToMeta->setValueDeleter(deleteUVector);

    // Read metazone mappings from metazoneInfo bundle
    metazoneInfoBundle = ures_openDirect(NULL, gMetazoneInfo, &status);
    metazoneMappings = ures_getByKey(metazoneInfoBundle, gMetazoneMappings, NULL, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    // Walk through all canonical tzids
    char zidkey[ZID_KEY_MAX];

    tzids = TimeZone::createEnumeration();
    const char *tzid;
    while ((tzid = tzids->next(NULL, status))) {
        if (U_FAILURE(status)) {
            goto error_cleanup;
        }
        // We may skip aliases, because the bundle
        // contains only canonical IDs.  For now, try
        // all of them.
        uprv_strcpy(zidkey, tzid);

        // Replace '/' with ':'
        UBool foundSep = FALSE;
        char *p = zidkey;
        while (*p) {
            if (*p == '/') {
                *p = ':';
                foundSep = TRUE;
            }
            p++;
        }
        if (!foundSep) {
            // A valid time zone key has at least one separator
            continue;
        }

        UResourceBundle *zoneItem = ures_getByKey(metazoneMappings, zidkey, NULL, &status);
        if (U_FAILURE(status)) {
            status = U_ZERO_ERROR;
            ures_close(zoneItem);
            continue;
        }

        UVector *mzMappings = NULL;
        while (ures_hasNext(zoneItem)) {
            UResourceBundle *mz = ures_getNextResource(zoneItem, NULL, &status);
            int32_t len;
            const UChar *mz_name = ures_getStringByIndex(mz, 0, &len, &status);
            const UChar *mz_from = ures_getStringByIndex(mz, 1, &len, &status);
            const UChar *mz_to   = ures_getStringByIndex(mz, 2, &len, &status);
            ures_close(mz);

            if(U_FAILURE(status)){
                status = U_ZERO_ERROR;
                continue;
            }
            // We do not want to use SimpleDateformat to parse boundary dates,
            // because this code could be triggered by the initialization code
            // used by SimpleDateFormat.
            UDate from = parseDate(mz_from, status);
            UDate to = parseDate(mz_to, status);
            if (U_FAILURE(status)) {
                status = U_ZERO_ERROR;
                continue;
            }

            OlsonToMetaMappingEntry *entry = (OlsonToMetaMappingEntry*)uprv_malloc(sizeof(OlsonToMetaMappingEntry));
            if (entry == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            entry->mzid = (UChar*)uprv_malloc((u_strlen(mz_name) + 1) * sizeof(UChar));
            if (entry->mzid == NULL) {
                uprv_free(entry);
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            u_strcpy(entry->mzid, mz_name);
            entry->from = from;
            entry->to = to;

            if (mzMappings == NULL) {
                mzMappings = new UVector(deleteOlsonToMetaMappingEntry, NULL, status);
                if (U_FAILURE(status)) {
                    delete mzMappings;
                    deleteOlsonToMetaMappingEntry(entry);
                    uprv_free(entry);
                    break;
                }
            }

            mzMappings->addElement(entry, status);
            if (U_FAILURE(status)) {
                break;
            }
        }

        ures_close(zoneItem);

        if (U_FAILURE(status)) {
            if (mzMappings != NULL) {
                delete mzMappings;
            }
            goto error_cleanup;
        }
        if (mzMappings != NULL) {
            olsonToMeta->put(UnicodeString(tzid), mzMappings, status);
            if (U_FAILURE(status)) {
                delete mzMappings;
                goto error_cleanup;
            }
        }
    }

    delete tzids;
    ures_close(metazoneMappings);
    ures_close(metazoneInfoBundle);
    return olsonToMeta;

error_cleanup:
    if (tzids != NULL) {
        delete tzids;
    }
    ures_close(metazoneMappings);
    ures_close(metazoneInfoBundle);
    if (olsonToMeta != NULL) {
        delete olsonToMeta;
    }
    return NULL;
}

/*
 * Creating Olson tzid to metazone mappings from ICU resource (3.8)
 */
Hashtable*
ZoneMeta::createOlsonToMetaMapOld(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *olsonToMeta = NULL;
    UResourceBundle *rootBundle = NULL;
    UResourceBundle *zoneStringsArray = NULL;
    StringEnumeration *tzids = NULL;

    olsonToMeta = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    olsonToMeta->setValueDeleter(deleteUVector);

    // Read metazone mappings from root bundle
    rootBundle = ures_openDirect(NULL, "", &status);
    zoneStringsArray = ures_getByKey(rootBundle, gZoneStringsTag, NULL, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    // Walk through all canonical tzids
    char zidkey[ZID_KEY_MAX];

    tzids = TimeZone::createEnumeration();
    const char *tzid;
    while ((tzid = tzids->next(NULL, status))) {
        if (U_FAILURE(status)) {
            goto error_cleanup;
        }
        // We may skip aliases, because the bundle
        // contains only canonical IDs.  For now, try
        // all of them.
        uprv_strcpy(zidkey, tzid);

        // Replace '/' with ':'
        UBool foundSep = FALSE;
        char *p = zidkey;
        while (*p) {
            if (*p == '/') {
                *p = ':';
                foundSep = TRUE;
            }
            p++;
        }
        if (!foundSep) {
            // A valid time zone key has at least one separator
            continue;
        }

        UResourceBundle *zoneItem = ures_getByKey(zoneStringsArray, zidkey, NULL, &status);
        UResourceBundle *useMZ = ures_getByKey(zoneItem, gUseMetazoneTag, NULL, &status);
        if (U_FAILURE(status)) {
            status = U_ZERO_ERROR;
            ures_close(zoneItem);
            ures_close(useMZ);
            continue;
        }

        UVector *mzMappings = NULL;
        while (ures_hasNext(useMZ)) {
            UResourceBundle *mz = ures_getNextResource(useMZ, NULL, &status);
            int32_t len;
            const UChar *mz_name = ures_getStringByIndex(mz, 0, &len, &status);
            const UChar *mz_from = ures_getStringByIndex(mz, 1, &len, &status);
            const UChar *mz_to   = ures_getStringByIndex(mz, 2, &len, &status);
            ures_close(mz);

            if(U_FAILURE(status)){
                status = U_ZERO_ERROR;
                continue;
            }
            // We do not want to use SimpleDateformat to parse boundary dates,
            // because this code could be triggered by the initialization code
            // used by SimpleDateFormat.
            UDate from = parseDate(mz_from, status);
            UDate to = parseDate(mz_to, status);
            if (U_FAILURE(status)) {
                status = U_ZERO_ERROR;
                continue;
            }

            OlsonToMetaMappingEntry *entry = (OlsonToMetaMappingEntry*)uprv_malloc(sizeof(OlsonToMetaMappingEntry));
            if (entry == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            entry->mzid = (UChar*)uprv_malloc((u_strlen(mz_name) + 1) * sizeof(UChar));
            if (entry->mzid == NULL) {
                uprv_free(entry);
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            u_strcpy(entry->mzid, mz_name);
            entry->from = from;
            entry->to = to;

            if (mzMappings == NULL) {
                mzMappings = new UVector(deleteOlsonToMetaMappingEntry, NULL, status);
                if (U_FAILURE(status)) {
                    delete mzMappings;
                    deleteOlsonToMetaMappingEntry(entry);
                    uprv_free(entry);
                    break;
                }
            }

            mzMappings->addElement(entry, status);
            if (U_FAILURE(status)) {
                break;
            }
        }

        ures_close(zoneItem);
        ures_close(useMZ);

        if (U_FAILURE(status)) {
            if (mzMappings != NULL) {
                delete mzMappings;
            }
            goto error_cleanup;
        }
        if (mzMappings != NULL) {
            olsonToMeta->put(UnicodeString(tzid), mzMappings, status);
            if (U_FAILURE(status)) {
                delete mzMappings;
                goto error_cleanup;
            }
        }
    }

    delete tzids;
    ures_close(zoneStringsArray);
    ures_close(rootBundle);
    return olsonToMeta;

error_cleanup:
    if (tzids != NULL) {
        delete tzids;
    }
    ures_close(zoneStringsArray);
    ures_close(rootBundle);
    if (olsonToMeta != NULL) {
        delete olsonToMeta;
    }
    return NULL;
}

Hashtable*
ZoneMeta::createMetaToOlsonMap(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *metaToOlson = NULL;
    UResourceBundle *supplementalDataBundle = NULL;
    UResourceBundle *mapTimezones = NULL;
    UResourceBundle *metazones = NULL;

    metaToOlson = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    metaToOlson->setValueDeleter(deleteUVector);

    supplementalDataBundle = ures_openDirect(NULL, gSupplementalData, &status);
    mapTimezones = ures_getByKey(supplementalDataBundle, gMapTimezonesTag, NULL, &status);
    metazones = ures_getByKey(mapTimezones, gMetazonesTag, NULL, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    while (ures_hasNext(metazones)) {
        UResourceBundle *mz = ures_getNextResource(metazones, NULL, &status);
        if (U_FAILURE(status)) {
            ures_close(mz);
            status = U_ZERO_ERROR;
            continue;
        }
        const char *mzkey = ures_getKey(mz);
        if (uprv_strncmp(mzkey, gMetazoneIdPrefix, MZID_PREFIX_LEN) == 0) {
            const char *mzid = mzkey + MZID_PREFIX_LEN;
            const char *territory = uprv_strrchr(mzid, '_');
            int32_t mzidLen = 0;
            int32_t territoryLen = 0;
            if (territory) {
                mzidLen = territory - mzid;
                territory++;
                territoryLen = uprv_strlen(territory);
            }
            if (mzidLen > 0 && territoryLen > 0) {
                int32_t tzidLen;
                const UChar *tzid = ures_getStringByIndex(mz, 0, &tzidLen, &status);
                if (U_SUCCESS(status)) {
                    // Create MetaToOlsonMappingEntry
                    MetaToOlsonMappingEntry *entry = (MetaToOlsonMappingEntry*)uprv_malloc(sizeof(MetaToOlsonMappingEntry));
                    if (entry == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        ures_close(mz);
                        goto error_cleanup;
                    }
                    entry->id = (UChar*)uprv_malloc((tzidLen + 1) * sizeof(UChar));
                    if (entry->id == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        uprv_free(entry);
                        ures_close(mz);
                        goto error_cleanup;
                    }
                    u_strcpy(entry->id, tzid);

                    entry->territory = (UChar*)uprv_malloc((territoryLen + 1) * sizeof(UChar));
                    if (entry->territory == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        uprv_free(entry->id);
                        uprv_free(entry);
                        ures_close(mz);
                        goto error_cleanup;
                    }
                    u_charsToUChars(territory, entry->territory, territoryLen + 1);

                    // Check if mapping entries for metazone is already available
                    UnicodeString mzidStr(mzid, mzidLen);
                    UVector *tzMappings = (UVector*)metaToOlson->get(mzidStr);
                    if (tzMappings == NULL) {
                        // Create new UVector and put it into the hashtable
                        tzMappings = new UVector(deleteMetaToOlsonMappingEntry, NULL, status);
                        metaToOlson->put(mzidStr, tzMappings, status);
                        if (U_FAILURE(status)) {
                            if (tzMappings != NULL) {
                                delete tzMappings;
                            }
                            deleteMetaToOlsonMappingEntry(entry);
                            ures_close(mz);
                            goto error_cleanup;
                        }
                    }
                    tzMappings->addElement(entry, status);
                    if (U_FAILURE(status)) {
                        goto error_cleanup;
                    }
                } else {
                    status = U_ZERO_ERROR;
                }
            }
        }
        ures_close(mz);
    }

    ures_close(metazones);
    ures_close(mapTimezones);
    ures_close(supplementalDataBundle);
    return metaToOlson;

error_cleanup:
    ures_close(metazones);
    ures_close(mapTimezones);
    ures_close(supplementalDataBundle);
    if (metaToOlson != NULL) {
        delete metaToOlson;
    }
    return NULL;
}

UnicodeString&
ZoneMeta::getCanonicalID(const UnicodeString &tzid, UnicodeString &canonicalID) {
    const CanonicalMapEntry *entry = getCanonicalInfo(tzid);
    if (entry != NULL) {
        canonicalID.setTo(entry->id);
    } else {
        // Use the input tzid
        canonicalID.setTo(tzid);
    }
    return canonicalID;
}

UnicodeString&
ZoneMeta::getCanonicalCountry(const UnicodeString &tzid, UnicodeString &canonicalCountry) {
    const CanonicalMapEntry *entry = getCanonicalInfo(tzid);
    if (entry != NULL && entry->country != NULL) {
        canonicalCountry.setTo(entry->country);
    } else {
        // Use the input tzid
        canonicalCountry.remove();
    }
    return canonicalCountry;
}

const CanonicalMapEntry*
ZoneMeta::getCanonicalInfo(const UnicodeString &tzid) {
    initialize();
    CanonicalMapEntry *entry = NULL;
    UnicodeString canonicalOlsonId;
    TimeZone::getOlsonCanonicalID(tzid, canonicalOlsonId);
    if (!canonicalOlsonId.isEmpty()) {
        if (gCanonicalMap != NULL) {
            entry = (CanonicalMapEntry*)gCanonicalMap->get(tzid);
        }
    }
    return entry;
}

UnicodeString&
ZoneMeta::getSingleCountry(const UnicodeString &tzid, UnicodeString &country) {
    UErrorCode status = U_ZERO_ERROR;

    // Get canonical country for the zone
    getCanonicalCountry(tzid, country);

    if (!country.isEmpty()) { 
        UResourceBundle *supplementalDataBundle = ures_openDirect(NULL, gSupplementalData, &status);
        UResourceBundle *zoneFormatting = ures_getByKey(supplementalDataBundle, gZoneFormattingTag, NULL, &status);
        UResourceBundle *multizone = ures_getByKey(zoneFormatting, gMultizoneTag, NULL, &status);

        if (U_SUCCESS(status)) {
            while (ures_hasNext(multizone)) {
                int32_t len;
                const UChar* multizoneCountry = ures_getNextString(multizone, &len, NULL, &status);
                if (country.compare(multizoneCountry, len) == 0) {
                    // Included in the multizone country list
                    country.remove();
                    break;
                }
            }
        }

        ures_close(multizone);
        ures_close(zoneFormatting);
        ures_close(supplementalDataBundle);
    }

    return country;
}

UnicodeString&
ZoneMeta::getMetazoneID(const UnicodeString &tzid, UDate date, UnicodeString &result) {
    UBool isSet = FALSE;
    const UVector *mappings = getMetazoneMappings(tzid);
    if (mappings != NULL) {
        for (int32_t i = 0; i < mappings->size(); i++) {
            OlsonToMetaMappingEntry *mzm = (OlsonToMetaMappingEntry*)mappings->elementAt(i);
            if (mzm->from <= date && mzm->to > date) {
                result.setTo(mzm->mzid, -1);
                isSet = TRUE;
                break;
            }
        }
    }
    if (!isSet) {
        result.remove();
    }
    return result;
}

const UVector*
ZoneMeta::getMetazoneMappings(const UnicodeString &tzid) {
    initialize();
    const UVector *result;
    if (gOlsonToMeta != NULL) {
        result = (UVector*)gOlsonToMeta->get(tzid);
    }
    return result;
}

UnicodeString&
ZoneMeta::getZoneIdByMetazone(const UnicodeString &mzid, const UnicodeString &region, UnicodeString &result) {
    initialize();
    UBool isSet = FALSE;
    if (gMetaToOlson != NULL) {
        UVector *mappings = (UVector*)gMetaToOlson->get(mzid);
        if (mappings != NULL) {
            // Find a preferred time zone for the given region.
            for (int32_t i = 0; i < mappings->size(); i++) {
                MetaToOlsonMappingEntry *olsonmap = (MetaToOlsonMappingEntry*)mappings->elementAt(i);
                if (region.compare(olsonmap->territory, -1) == 0) {
                    result.setTo(olsonmap->id);
                    isSet = TRUE;
                    break;
                } else if (u_strcmp(olsonmap->territory, gWorld) == 0) {
                    result.setTo(olsonmap->id);
                    isSet = TRUE;
                }
            }
        }
    }
    if (!isSet) {
        result.remove();
    }
    return result;
}


U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
