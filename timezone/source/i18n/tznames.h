/*
*******************************************************************************
* Copyright (C) 2022, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#ifndef __TZNAMES_H
#define __TZNAMES_H

/**
 * \file 
 * \brief C API: Time zone names classe
 */
#include "unicode/utypes.h"
#include "unicode/uloc.h"
#include "unicode/unistr.h"

#if !UCONFIG_NO_FORMATTING


U_CDECL_BEGIN

typedef enum UTimeZoneNameType {
    UTZNM_LONG_GENERIC      = 0x01,
    UTZNM_LONG_STANDARD     = 0x02,
    UTZNM_LONG_DAYLIGHT     = 0x04,
    UTZNM_SHORT_GENERIC     = 0x08,
    UTZNM_SHORT_STANDARD    = 0x10,
    UTZNM_SHORT_DAYLIGHT    = 0x20,
    UTZNM_SHORT_STANDARD_COMMONLY_USED  = 0x40,
    UTZNM_SHORT_DAYLIGHT_COMMONLY_USED  = 0x80
} UTimeZoneNameType;

U_CDECL_END

U_NAMESPACE_BEGIN
class U_I18N_API TimeZoneNames : public UMemory {
public:
    virtual ~TimeZoneNames();

    static TimeZoneNames* U_EXPORT2 createInstance(const Locale& locale, UErrorCode& status);

    virtual StringEnumeration* getAvailableMetaZoneIDs() const = 0;
    virtual StringEnumeration* getAvailableMetaZoneIDs(const UnicodeString& tzID) const = 0;
    virtual UnicodeString& getMetaZoneID(const UnicodeString& tzID, UDate date, UnicodeString& mzID) const = 0;
    virtual UnicodeString& getReferenceZoneID(const UnicodeString& mzID, const char* region, UnicodeString& tzID) const = 0;

    virtual UnicodeString& getMetaZoneDisplayName(const UnicodeString& mzID, UTimeZoneNameType type, UnicodeString& name) const = 0;
    virtual UnicodeString& getTimeZoneDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UnicodeString& name) const = 0;

    virtual UnicodeString& getExemplarLocationName(const UnicodeString& tzID, UnicodeString& name) const;
    virtual UnicodeString& getZoneDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UDate date, UnicodeString& name) const;

    // types - bit flags of UTimeZoneNameType
    virtual UEnumeration* find(const UnicodeString& text, int32_t start, int32_t types) const = 0;
};

U_NAMESPACE_END
#endif
#endif
