/*
 ********************************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 ********************************************************************************
 */

#include "unicode/utypes.h"

#include "unicode/ucsdet.h"
#include "csdetect.h"
#include "csmatch.h"

#include "unicode/putil.h"

#include "cmemory.h"
#include "uenumimp.h"

U_NAMESPACE_USE

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define NEW_ARRAY(type,count) (type *) uprv_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) uprv_free((void *) (array))

U_CDECL_BEGIN

U_DRAFT UCharsetDetector * U_EXPORT2
ucsdet_open(UErrorCode   *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }
    CharsetDetector* csd = new CharsetDetector();
    return (UCharsetDetector *) csd;
}

U_DRAFT void U_EXPORT2
ucsdet_close(UCharsetDetector *ucsd)
{
    CharsetDetector *csd = (CharsetDetector *) ucsd;
    delete csd;
}

U_DRAFT void U_EXPORT2
ucsdet_setText(UCharsetDetector *ucsd, const char *textIn, int32_t len, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return;
    }

    CharsetDetector *csd = (CharsetDetector *) ucsd;

    csd->setText(textIn, len);
}

U_DRAFT const char * U_EXPORT2
ucsdet_getName(const UCharsetMatch *ucsm, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }

    CharsetMatch *csm = (CharsetMatch *) ucsm;

    return csm->getName();
}

U_DRAFT int32_t U_EXPORT2
ucsdet_getConfidence(const UCharsetMatch *ucsm, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return -1;
    }

    CharsetMatch *csm = (CharsetMatch *) ucsm;

    return csm->getConfidence();
}

U_DRAFT const char * U_EXPORT2
ucsdet_getLanguage(const UCharsetMatch *ucsm, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }

    CharsetMatch *csm = (CharsetMatch *) ucsm;

    return csm->getLanguage();
}

U_DRAFT const UCharsetMatch * U_EXPORT2
ucsdet_detect(UCharsetDetector *ucsd, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }

    CharsetDetector *csd = (CharsetDetector *) ucsd;

    return (const UCharsetMatch *) csd->detect(*status);
}

U_DRAFT void U_EXPORT2
ucsdet_setDeclaredEncoding(UCharsetDetector *ucsd, const char *encoding, int32_t length, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return;
    }

    CharsetDetector *csd = (CharsetDetector *) ucsd;

    csd->setDeclaredEncoding(encoding,length);
}

U_DRAFT const UCharsetMatch**
ucsdet_detectAll(UCharsetDetector *ucsd,
                 int32_t *maxMatchesFound, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }

    CharsetDetector *csd = (CharsetDetector *) ucsd;

    return (const UCharsetMatch**)csd->detectAll(*maxMatchesFound,*status);
}

// U_DRAFT  const char * U_EXPORT2
// ucsdet_getDetectableCharsetName(const UCharsetDetector *csd, int32_t index, UErrorCode *status)
// {
//     if(U_FAILURE(*status)) {
// 	return 0;
//     }
//     return csd->getCharsetName(index,*status);
// }

// U_DRAFT  int32_t U_EXPORT2
// ucsdet_getDetectableCharsetsCount(const UCharsetDetector *csd, UErrorCode *status)
// {
//     if(U_FAILURE(*status)) {
// 	return -1;
//     }
//     return UCharsetDetector::getDetectableCount();
// }
U_CDECL_END

typedef struct {
    int32_t currIndex;
    int32_t maxIndex;
    char *currChar;
    UChar *currUChar;
    char **array;
} Context;

#define cont ((Context *)en->context)

static void U_CALLCONV
enumClose(UEnumeration *en) {
    if(cont->currUChar != NULL) {
        DELETE_ARRAY(cont->currUChar);
        cont->currUChar = NULL;
    }

    delete en;
}

static int32_t U_CALLCONV
enumCount(UEnumeration *en, UErrorCode *status) {
    return cont->maxIndex;
}

static const UChar* U_CALLCONV 
enumUNext(UEnumeration *en, int32_t *resultLength, UErrorCode *status) {
    if(cont->currIndex >= cont->maxIndex) {
        return NULL;
    }

    if(cont->currUChar == NULL) {
        cont->currUChar = NEW_ARRAY(UChar, 1024);
    }

    cont->currChar = (cont->array)[cont->currIndex];
    *resultLength = (int32_t)strlen(cont->currChar);
    u_charsToUChars(cont->currChar, cont->currUChar, *resultLength);
    cont->currIndex++;

    return cont->currUChar;
}

static const char* U_CALLCONV
enumNext(UEnumeration *en, int32_t *resultLength, UErrorCode *status) {
    if(cont->currIndex >= cont->maxIndex) {
        return NULL;
    }

    cont->currChar = (cont->array)[cont->currIndex];
    *resultLength = (int32_t)strlen(cont->currChar);
    cont->currIndex++;

    return cont->currChar;
}

static void U_CALLCONV
enumReset(UEnumeration *en, UErrorCode *status) {
    cont->currIndex = 0;
}

Context enumContext = {
    0, 0,
    NULL, NULL,
    NULL
};

UEnumeration enumeration = {
    NULL,
    &enumContext,
    enumClose,
    enumCount,
    enumUNext,
    enumNext,
    enumReset
};


static UEnumeration *getEnum(const char** source, int32_t size) {
    UEnumeration *en = new UEnumeration;
    memcpy(en, &enumeration, sizeof(UEnumeration));
    cont->array = (char **)source;
    cont->maxIndex = size;
    return en;
}

static const char** charsets = NEW_ARRAY(const char *, CharsetDetector::getDetectableCount());

U_CDECL_BEGIN
U_DRAFT  UEnumeration * U_EXPORT2
ucsdet_getAllDetectableCharsets(const UCharsetDetector *ucsd,  UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }

    CharsetDetector *csd = (CharsetDetector *) ucsd;
    int32_t size = CharsetDetector::getDetectableCount();

    for(int32_t index=0; index<size; index += 1) {
        charsets[index] = csd->getCharsetName(index, *status);
    }

    return getEnum(charsets, size);
}

U_DRAFT  UBool U_EXPORT2
ucsdet_isInputFilterEnabled(const UCharsetDetector *ucsd)
{
    CharsetDetector *csd = (CharsetDetector *) ucsd;

    return csd->getStripTagsFlag();
}

U_DRAFT  UBool U_EXPORT2
ucsdet_enableInputFilter(UCharsetDetector *ucsd, UBool filter)
{
    CharsetDetector *csd = (CharsetDetector *) ucsd;
    UBool prev = csd->getStripTagsFlag();
    csd->setStripTagsFlag(filter);

    return prev;
}

U_DRAFT  int32_t U_EXPORT2
ucsdet_getUChars(const UCharsetMatch *ucsm,
                 UChar *buf, int32_t cap, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return -1;
    }

    CharsetMatch *csm = (CharsetMatch *) ucsm;
    
    return csm->getUChars(buf, cap, status);
}
U_CDECL_END

