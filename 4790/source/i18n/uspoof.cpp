/*
***************************************************************************
* Copyright (C) 2008-2009, International Business Machines Corporation
* and others. All Rights Reserved.
***************************************************************************
*   file name:  uspoof.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2008Feb13
*   created by: Andy Heninger
*
*   Unicode Spoof Detection
*/
#include "unicode/utypes.h"
#include "unicode/uspoof.h"
#include "unicode/unorm.h"
#include "cmemory.h"
#include "uspoof_impl.h"

U_NAMESPACE_BEGIN


U_CAPI USpoofChecker * U_EXPORT2
uspoof_open(UErrorCode *status) {
	if (U_FAILURE(*status)) {
	    return NULL;
	}
    SpoofImpl *si = new SpoofImpl(SpoofData::getDefault(*status), *status);
	if (U_FAILURE(*status)) {
		delete si;
		si = NULL;
	}
	return (USpoofChecker *)si;
}

U_CAPI void U_EXPORT2
uspoof_close(USpoofChecker *sc) {
    UErrorCode status = U_ZERO_ERROR;
	SpoofImpl *This = SpoofImpl::validateThis(sc, status);
    delete This;
}

    
U_CAPI void U_EXPORT2
uspoof_setChecks(USpoofChecker *sc, int32_t checks, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
	if (This == NULL) {
		return;
	}

    // Verify that the requested checks are all ones (bits) that 
    //   are acceptable, known values.
    if (checks && ~USPOOF_ALL_CHECKS) {
        *status = U_ILLEGAL_ARGUMENT_ERROR; 
        return;
    }

    This->fChecks = checks;
}


U_CAPI int32_t U_EXPORT2
uspoof_getChecks(const USpoofChecker *sc, UErrorCode *status) {
    const SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
	if (This == NULL) {
		return 0;
	}
    return This->fChecks;
}

U_CAPI void U_EXPORT2
uspoof_setAllowedLocales(USpoofChecker *sc, const char *localesList, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
	if (This == NULL) {
		return;
	}
    // TODO:
}


U_CAPI int32_t U_EXPORT2
uspoof_getSkeleton(const USpoofChecker *sc,
                   USpoofChecks type,
                   const UChar *s,  int32_t length,
                   UChar *dest, int32_t destCapacity,
                   UErrorCode *status) {

    const SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }
    if (length<-1 || destCapacity<0 || (destCapacity==0 && dest!=NULL)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    // NFKD transform of the user supplied input
    
    UChar nfkdBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar *nfkdInput = nfkdBuf;
    int32_t normalizedLen = unorm_normalize(
        s, length, UNORM_NFKD, 0, nfkdInput, USPOOF_STACK_BUFFER_SIZE, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        nfkdInput = (UChar *)uprv_malloc((normalizedLen+1)*sizeof(UChar));
        if (nfkdInput == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        normalizedLen = unorm_normalize(s, length, UNORM_NFKD, 0,
                                        nfkdInput, normalizedLen+1, status);
    }
    if (U_FAILURE(*status)) {
        return 0;
    }

    // buffer to hold the Unicode defined mappings for a single code point
    UChar buf[USPOOF_MAX_SKELETON_EXPANSION];

    // Apply the mapping to the NFKD form string
    
    int32_t inputIndex = 0;
    int32_t resultLen = 0;
    while (inputIndex < normalizedLen) {
        UChar32 c;
        U16_NEXT(nfkdInput, inputIndex, normalizedLen, c);
        int32_t replaceLen = This->ConfusableLookup(c, buf);
        if (resultLen + replaceLen < destCapacity) {
            int i;
            for (i=0; i<replaceLen; i++) {
                dest[resultLen++] = buf[i];
            }
        } else {
            // Storing the transformed string would overflow the dest buffer.
            //   Don't bother storing anything, just sum up the required buffer size.
            //   (We dont guarantee that a truncated buffer is filled to it's end)
            resultLen += replaceLen;
        }
    }
    
    if (resultLen < destCapacity) {
        dest[resultLen] = 0;
    } else if (resultLen == destCapacity) {
        *status = U_STRING_NOT_TERMINATED_WARNING;
    } else {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    if (nfkdInput != nfkdBuf) {
        uprv_free(nfkdInput);
    }
    return resultLen;
}


U_CAPI UnicodeString &  U_EXPORT2
uspoof_getSkeletonUnicodeString(const USpoofChecker *sc,
                                USpoofChecks type,
                                const UnicodeString &s,
                                UnicodeString &dest,
                                UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return dest;
    }
    dest.remove();
    
    const UChar *str = s.getBuffer();
    int32_t      strLen = s.length();
    UChar        smallBuf[100];
    UChar       *buf = smallBuf;
    int32_t outputSize = uspoof_getSkeleton(sc, type, str, strLen, smallBuf, 100, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        buf = static_cast<UChar *>(uprv_malloc(outputSize+1));
        if (buf == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        }
        uspoof_getSkeleton(sc, type, str, strLen, buf, outputSize+1, status);
    }
    if (U_SUCCESS(*status)) {
        dest.setTo(buf, outputSize);
    }

    if (buf != smallBuf) {
        uprv_free(buf);
    }
    return dest;
}


U_CAPI void U_EXPORT2
uspoof_setAllowedChars(USpoofChecker *sc, const USet *chars, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
	if (This == NULL) {
		return;
	}

    // Cast the USet to a UnicodeSet.  Slightly dicey - relies on knowing that
    //    a USet * is actually a UnicodeSet.  But we have both USet and
    //    UnicodeSet interfaces, and need a common internal representation.
    UnicodeSet *uniset = (UnicodeSet *)chars;

    if (uniset->isBogus()) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    // Clone & freeze the caller's set
    UnicodeSet *clonedSet = (UnicodeSet *)uniset->clone();
    if (clonedSet == NULL || clonedSet->isBogus()) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    clonedSet->freeze();

    // Any existing set is frozen, meaning it can't be reused.
    //   Just delete it and create another.
    delete This->fAllowedCharsSet;
    This->fAllowedCharsSet = clonedSet;

    // Set the bit that enables the allowed chars test
    This->fChecks |= USPOOF_CHAR_LIMIT;
}

U_NAMESPACE_END
