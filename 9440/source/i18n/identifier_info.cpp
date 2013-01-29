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
#include "uvector.h"

#include "stdio.h"    // TODO: debugging, remove.

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
            ASCII    = new UnicodeSet(0, 0x7f);
            JAPANESE = new ScriptSet();
            CHINESE  = new ScriptSet();
            KOREAN   = new ScriptSet();
            CONFUSABLE_WITH_LATIN = new ScriptSet();
            if (ASCII == NULL || JAPANESE == NULL || CHINESE == NULL || KOREAN == NULL 
                    || CONFUSABLE_WITH_LATIN == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            ASCII->freeze();
            JAPANESE->set(USCRIPT_LATIN, status).set(USCRIPT_HAN, status).set(USCRIPT_HIRAGANA, status)
                     .set(USCRIPT_KATAKANA, status);
            CHINESE->set(USCRIPT_LATIN, status).set(USCRIPT_HAN, status).set(USCRIPT_BOPOMOFO, status);
            KOREAN->set(USCRIPT_LATIN, status).set(USCRIPT_HAN, status).set(USCRIPT_HANGUL, status);
            CONFUSABLE_WITH_LATIN->set(USCRIPT_CYRILLIC, status).set(USCRIPT_GREEK, status)
                      .set(USCRIPT_CHEROKEE, status);
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
        return *this;
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
            fNumerics->add(cp - u_getNumericValue(cp));
        }
        UScriptCode extensions[500];
        int32_t extensionsCount = uscript_getScriptExtensions(cp, extensions, LENGTHOF(extensions), &status);
        if (U_FAILURE(status)) {
            return *this;
        }
        temp->resetAll();
        for (int32_t j=0; j<extensionsCount; j++) {
            temp->set(extensions[j], status);
        }
        temp->reset(USCRIPT_COMMON, status);
        temp->reset(USCRIPT_INHERITED, status);
        switch (temp->countMembers()) {
        case 0: break;
        case 1:
            // Single script, record it.
            fRequiredScripts->Union(*temp);
            break;
        default:
            if (!fRequiredScripts->intersects(*temp) 
                    && !uhash_geti(fScriptSetSet, temp)) {
                // If the set hasn't been added already, add it and create new temporary for the next pass,
                // so we don't rewrite what's already in the set.
                uhash_puti(fScriptSetSet, temp, 1, &status);  // Takes ownership of temp.
                temp = new ScriptSet();
            }
            break;
        }
    }
    // Now make a final pass through to remove alternates that came before singles.
    // [Kana], [Kana Hira] => [Kana]
    // This is relatively infrequent, so doesn't have to be optimized.
    // We also compute any commonalities among the alternates.
    if (uhash_count(fScriptSetSet) == 0) {
        fCommonAmongAlternates->resetAll();
    } else {
        fCommonAmongAlternates->setAll();
        for (int32_t it = -1;;) {
            const UHashElement *nextHashEl = uhash_nextElement(fScriptSetSet, &it);
            if (nextHashEl == NULL) {
                break;
            }
            ScriptSet *next = static_cast<ScriptSet *>(nextHashEl->key.pointer);
            // [Kana], [Kana Hira] => [Kana]
            if (fRequiredScripts->intersects(*next)) {
                uhash_removeElement(fScriptSetSet, nextHashEl);
            } else {
                // [[Arab Syrc Thaa]; [Arab Syrc]] => [[Arab Syrc]]
                fCommonAmongAlternates->intersect(*next);
                for (int32_t otherIt = -1;;) {
                    const UHashElement *otherHashEl = uhash_nextElement(fScriptSetSet, &otherIt);
                    if (otherHashEl == NULL) {
                        break;
                    }
                    ScriptSet *other = static_cast<ScriptSet *>(otherHashEl->key.pointer);
                    if (next != other && next->contains(*other)) {
                        uhash_removeElement(fScriptSetSet, nextHashEl);
                        break;
                    }
                }
            }
        }
        if (uhash_count(fScriptSetSet) == 0) {
            fCommonAmongAlternates->resetAll();
        }
    }
    return *this;
}


const UnicodeString *IdentifierInfo::getIdentifier() const {
    return fIdentifier;
}

const ScriptSet *IdentifierInfo::getScripts() const {
    return fRequiredScripts;
}

const UHashtable *IdentifierInfo::getAlternates() const {
    return fScriptSetSet;
}


const UnicodeSet *IdentifierInfo::getNumerics() const {
    return fNumerics;
}

const ScriptSet *IdentifierInfo::getCommonAmongAlternates() const {
    return fCommonAmongAlternates;
}

URestrictionLevel IdentifierInfo::getRestrictionLevel(UErrorCode &status) const {
    if (!fIdentifierProfile->containsAll(*fIdentifier) || getNumerics()->size() > 1) {
        return USPOOF_UNRESTRICTIVE;
    }
    if (ASCII->containsAll(*fIdentifier)) {
        return USPOOF_ASCII;
    }
    ScriptSet temp;
    temp.Union(*fRequiredScripts);
    temp.reset(USCRIPT_COMMON, status);
    temp.reset(USCRIPT_INHERITED, status);
    // This is a bit tricky. We look at a number of factors.
    // The number of scripts in the text.
    // Plus 1 if there is some commonality among the alternates (eg [Arab Thaa]; [Arab Syrc])
    // Plus number of alternates otherwise (this only works because we only test cardinality up to 2.)
    int32_t cardinalityPlus = temp.countMembers() + 
            (fCommonAmongAlternates->countMembers() == 0 ? uhash_count(fScriptSetSet) : 1);
    if (cardinalityPlus < 2) {
        return USPOOF_HIGHLY_RESTRICTIVE;
    }
    if (containsWithAlternates(*JAPANESE, temp) || containsWithAlternates(*CHINESE, temp)
            || containsWithAlternates(*KOREAN, temp)) {
        return USPOOF_HIGHLY_RESTRICTIVE;
    }
    if (cardinalityPlus == 2 && temp.test(USCRIPT_LATIN, status) && !temp.intersects(*CONFUSABLE_WITH_LATIN)) {
        return USPOOF_MODERATELY_RESTRICTIVE;
    }
    return USPOOF_MINIMALLY_RESTRICTIVE;
}


UBool IdentifierInfo::containsWithAlternates(const ScriptSet &container, const ScriptSet &containee) const {
    if (!container.contains(containee)) {
        return FALSE;
    }
    for (int32_t iter = -1; ;) {
        const UHashElement *hashEl = uhash_nextElement(fScriptSetSet, &iter);
        if (hashEl == NULL) {
            break;
        }
        ScriptSet *alternatives = static_cast<ScriptSet *>(hashEl->key.pointer);
        if (!container.intersects(*alternatives)) {
            return false;
        }
    }
    return true;
}

UnicodeString &IdentifierInfo::displayAlternates(UnicodeString &dest, const UHashtable *alternates, UErrorCode &status) {
    UVector sorted(status);
    if (U_FAILURE(status)) {
        return dest;
    }
    for (int32_t pos = -1; ;) {
        const UHashElement *el = uhash_nextElement(alternates, &pos);
        if (el == NULL) {
            break;
        }
        ScriptSet *ss = static_cast<ScriptSet *>(el->key.pointer);
        sorted.addElement(ss, status);
    }
    sorted.sort(uhash_compareScriptSet, status);
    UnicodeString separator = UNICODE_STRING_SIMPLE("; ");
    for (int32_t i=0; i<sorted.size(); i++) {
        if (i>0) {
            dest.append(separator);
        }
        ScriptSet *ss = static_cast<ScriptSet *>(sorted.elementAt(i));
        ss->displayScripts(dest);
    }
    return dest;
}

UnicodeSet *IdentifierInfo::ASCII;
ScriptSet *IdentifierInfo::JAPANESE;
ScriptSet *IdentifierInfo::CHINESE;
ScriptSet *IdentifierInfo::KOREAN;
ScriptSet *IdentifierInfo::CONFUSABLE_WITH_LATIN;

U_NAMESPACE_END

