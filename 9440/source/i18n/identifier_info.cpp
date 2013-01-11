/*
**********************************************************************
*   Copyright (C) 2008-2011, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*/

#include "unicode/utypes.h"

#include "unicode/uchar.h"
#include "unicode/utf16.h"

#include "identifier_info.h"
#include "mutex.h"
#include "scriptset.h"


U_NAMESPACE_BEGIN

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static UMutex gInitMutex = U_MUTEX_INITIALIZER;
static UBool gStaticsAreInitialized = FALSE;

IdentifierInfo::IdentifierInfo(UErrorCode &status):
         fIdentifier(NULL), fRequiredScripts(NULL), fScriptSetSet(NULL), 
         fCommonAmongAlternates(NULL), fNumerics(NULL), fIdentifierProfile(NULL) {
    if (U_FAILURE(status)) {
        return;
    }
    {
        Mutex lock(&gInitMutex);
        if (!gStaticsAreInitialized) {
            JAPANESE = new ScriptSet();
            CHINESE  = new ScriptSet();
            KOREAN   = new ScriptSet();
            CONFUSABLE_WITH_LATIN = new ScriptSet();
            if (JAPANESE == NULL || CHINESE == NULL || KOREAN == NULL || CONFUSABLE_WITH_LATIN == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            JAPANESE->set(USCRIPT_LATIN).set(USCRIPT_HAN).set(USCRIPT_HIRAGANA).set(USCRIPT_KATAKANA);
            CHINESE->set(USCRIPT_LATIN).set(USCRIPT_HAN).set(USCRIPT_BOPOMOFO);
            KOREAN->set(USCRIPT_LATIN).set(USCRIPT_HAN).set(USCRIPT_HANGUL);
            CONFUSABLE_WITH_LATIN->set(USCRIPT_CYRILLIC).set(USCRIPT_GREEK).set(USCRIPT_CHEROKEE);
            gStaticsAreInitialized = TRUE;
        }
    }
    fIdentifier = new UnicodeString();
    fRequiredScripts = new ScriptSet();
    fScriptSetSet = uhash_open(uhash_hashScriptSet, uhash_compareScriptSet, NULL, &status);
    uhash_setKeyDeleter(fScriptSetSet, uhash_deleteScriptSet);
    fCommonAmongAlternates = new ScriptSet();
    fNumerics = new UnicodeSet();
    fIdentifierProfile = new UnicodeSet(0, 0x10FFFF);

    if (U_SUCCESS(status) && (fIdentifier == NULL || fRequiredScripts == NULL || fScriptSetSet == NULL ||
                              fCommonAmongAlternates == NULL || fNumerics == NULL || fIdentifierProfile == NULL)) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
};

IdentifierInfo::~IdentifierInfo() {
    delete fIdentifier;
    delete fRequiredScripts;
    uhash_close(fScriptSetSet);
    delete fCommonAmongAlternates;
    delete fNumerics;
    delete fIdentifierProfile;
};


IdentifierInfo &IdentifierInfo::clear() {
    fRequiredScripts->resetAll();
    uhash_removeAll(fScriptSetSet);
    fNumerics->clear();
    fCommonAmongAlternates->resetAll();
    return *this;
}


IdentifierInfo &IdentifierInfo::setIdentifierProfile(const UnicodeSet &identifierProfile) {
    *fIdentifierProfile = identifierProfile;
    return *this;
}


const UnicodeSet &IdentifierInfo::getIdentifierProfile() const {
    return *fIdentifierProfile;
}


IdentifierInfo &IdentifierInfo::setIdentifier(const UnicodeString &identifier, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    *fIdentifier = identifier;
    clear();
    ScriptSet *temp = new ScriptSet(); // Will reuse this.
    UChar32 cp;
    for (int32_t i = 0; i < identifier.length(); i += U16_LENGTH(cp)) {
        cp = identifier.char32At(i);
        // Store a representative character for each kind of decimal digit
        if (u_charType(cp) == U_DECIMAL_DIGIT_NUMBER) {
            // Just store the zero character as a representative for comparison. Unicode guarantees it is cp - value
            numerics.add(cp - u_getNumericValue(cp));
        }
        UScriptCode extensions[500];
        int32_t extensionsCount = uscript_getScriptExtensions(c, extensions, LENGTHOF(buf), status);
        if (U_FAILURE(status)) {
            return;
        }
        temp->resetAll();
        for (int32_t j=0; j<extensionsCount; j++) {
            temp->set(extensions[j], status);
        }
        temp->reset(USCRIPT_COMMON, status);
        temp->reset(USCRIPT_INHERITED, status);
        //            if (temp.cardinality() == 0) {
        //                // HACK for older version of ICU
        //                requiredScripts.set(UScript.getScript(cp));
        //            } else 
        switch (temp->countMembers()) {
        case 0: break;
        case 1:
            // Single script, record it.
            fRequiredScripts->Union(temp);
            break;
        default:
            if (!fRequiredScripts.intersects(temp) 
                    && !uhash_geti(fScriptSetSet, temp)) {
                // If the set hasn't been added already, add it and create new temporary for the next pass,
                // so we don't rewrite what's already in the set.
                uhash_puti(fScriptSetSet, temp, 1, status);  // Takes ownership of temp.
                temp = new BitSet();
            }
            break;
        }
    }
    // Now make a final pass through to remove alternates that came before singles.
    // [Kana], [Kana Hira] => [Kana]
    // This is relatively infrequent, so doesn't have to be optimized.
    // We also compute any commonalities among the alternates.
    if (scriptSetSet->countMembers() == 0) {
        commonAmongAlternates->resetAll();
    } else {
        commonAmongAlternates->setAll();
        // for (Iterator<BitSet> it = scriptSetSet.iterator(); it.hasNext();) {
        int32_t pos = -1;
        for (;;) {
            UHashElement hashEl = uhash_nextElement(scriptSetSet, &pos);
            nextSS = static_cast<ScriptSet *>(hashEl->value.pointer);
            if (nextSS == NULL) {
                break;
            }
            // final BitSet next = it.next();
            if (fRequiredScripts->intersects(*nextSS)) {
                uhash_removeElement(scriptSetSet, hashEl);
            } else {
                // [[Arab Syrc Thaa]; [Arab Syrc]] => [[Arab Syrc]]
                for (BitSet other : scriptSetSet) {
                    if (next != other && contains(next, other)) {
                        it.remove();
                        break;
                    }
                }
                commonAmongAlternates.and(next); // get the intersection.
            }
        }
        if (scriptSetSet.size() == 0) {
            commonAmongAlternates.clear();
        }
    }
    // Note that the above code doesn't minimize alternatives. That is, it does not collapse
    // [[Arab Syrc Thaa]; [Arab Syrc]] to [[Arab Syrc]]
    // That would be a possible optimization, but is probably not worth the extra processing
    return this;
}

ScriptSet *IdentifierInfo::JAPANESE;
ScriptSet *IdentifierInfo::CHINESE;
ScriptSet *IdentifierInfo::KOREAN;
ScriptSet *IdentifierInfo::CONFUSABLE_WITH_LATIN;



U_NAMESPACE_END


