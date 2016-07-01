/*
***************************************************************************
*   Copyright (C) 1999-2016 International Business Machines Corporation
*   and others. All rights reserved.
***************************************************************************
*/
//
//  file:  rbbi.cpp  Contains the implementation of the rule based break iterator
//                   runtime engine and the API implementation for
//                   class RuleBasedBreakIterator
//

#include "utypeinfo.h"  // for 'typeid' to work

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/rbbi.h"
#include "unicode/schriter.h"
#include "unicode/uchriter.h"
#include "unicode/uclean.h"
#include "unicode/udata.h"

#include "brkeng.h"
#include "ucln_cmn.h"
#include "cmemory.h"
#include "cstring.h"
#include "rbbidata.h"
#include "rbbi_dict_cache.h"
#include "rbbirb.h"
#include "uassert.h"
#include "umutex.h"
#include "uvectr32.h"

// if U_LOCAL_SERVICE_HOOK is defined, then localsvc.cpp is expected to be included.
#if U_LOCAL_SERVICE_HOOK
#include "localsvc.h"
#endif

#ifdef RBBI_DEBUG
static UBool gTrace = FALSE;
#endif

U_NAMESPACE_BEGIN

// The state number of the starting state
static const int32_t START_STATE = 1;

// The state-transition value indicating "stop"
static const int32_t STOP_STATE = 0;


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedBreakIterator)


//=======================================================================
// constructors
//=======================================================================

/**
 * Constructs a RuleBasedBreakIterator that uses the already-created
 * tables object that is passed in as a parameter.
 */
RuleBasedBreakIterator::RuleBasedBreakIterator(RBBIDataHeader* data, UErrorCode &status)
{
    init(status);
    fData = new RBBIDataWrapper(data, status); // status checked in constructor
    if (U_FAILURE(status)) {return;}
    if(fData == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}

//
//  Construct from precompiled binary rules (tables).  This constructor is public API,
//  taking the rules as a (const uint8_t *) to match the type produced by getBinaryRules().
//
RuleBasedBreakIterator::RuleBasedBreakIterator(const uint8_t *compiledRules,
                       uint32_t       ruleLength,
                       UErrorCode     &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    if (compiledRules == NULL || ruleLength < sizeof(RBBIDataHeader)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    const RBBIDataHeader *data = (const RBBIDataHeader *)compiledRules;
    if (data->fLength > ruleLength) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fData = new RBBIDataWrapper(data, RBBIDataWrapper::kDontAdopt, status);
    if (U_FAILURE(status)) {return;}
    if(fData == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}


//-------------------------------------------------------------------------------
//
//   Constructor   from a UDataMemory handle to precompiled break rules
//                 stored in an ICU data file.
//
//-------------------------------------------------------------------------------
RuleBasedBreakIterator::RuleBasedBreakIterator(UDataMemory* udm, UErrorCode &status)
{
    init(status);
    fData = new RBBIDataWrapper(udm, status); // status checked in constructor
    if (U_FAILURE(status)) {return;}
    if(fData == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}



//-------------------------------------------------------------------------------
//
//   Constructor       from a set of rules supplied as a string.
//
//-------------------------------------------------------------------------------
RuleBasedBreakIterator::RuleBasedBreakIterator( const UnicodeString  &rules,
                                                UParseError          &parseError,
                                                UErrorCode           &status)
{
    init(status);
    if (U_FAILURE(status)) {return;}
    RuleBasedBreakIterator *bi = (RuleBasedBreakIterator *)
        RBBIRuleBuilder::createRuleBasedBreakIterator(rules, &parseError, status);
    // Note:  This is a bit awkward.  The RBBI ruleBuilder has a factory method that
    //        creates and returns a complete RBBI.  From here, in a constructor, we
    //        can't just return the object created by the builder factory, hence
    //        the assignment of the factory created object to "this".
    if (U_SUCCESS(status)) {
        *this = *bi;
        delete bi;
    }
}


//-------------------------------------------------------------------------------
//
// Default Constructor.      Create an empty shell that can be set up later.
//                           Used when creating a RuleBasedBreakIterator from a set
//                           of rules.
//-------------------------------------------------------------------------------
RuleBasedBreakIterator::RuleBasedBreakIterator() {
    UErrorCode status = U_ZERO_ERROR;
    init(status);
}


//-------------------------------------------------------------------------------
//
//   Copy constructor.  Will produce a break iterator with the same behavior,
//                      and which iterates over the same text, as the one passed in.
//
//-------------------------------------------------------------------------------
RuleBasedBreakIterator::RuleBasedBreakIterator(const RuleBasedBreakIterator& other)
: BreakIterator(other)
{
    UErrorCode status = U_ZERO_ERROR;
    this->init(status);
    *this = other;
}


/**
 * Destructor
 */
RuleBasedBreakIterator::~RuleBasedBreakIterator() {
    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        // fCharIter was adopted from the outside.
        delete fCharIter;
    }
    fCharIter = NULL;
    delete fSCharIter;
    fSCharIter = NULL;
    delete fDCharIter;
    fDCharIter = NULL;

    utext_close(fText);

    if (fData != NULL) {
        fData->removeReference();
        fData = NULL;
    }
    delete fDictionaryCache;
    fDictionaryCache = NULL;

    delete fLanguageBreakEngines;
    fLanguageBreakEngines = NULL;

    delete fUnhandledBreakEngine;
    fUnhandledBreakEngine = NULL;
}

/**
 * Assignment operator.  Sets this iterator to have the same behavior,
 * and iterate over the same text, as the one passed in.
 */
RuleBasedBreakIterator&
RuleBasedBreakIterator::operator=(const RuleBasedBreakIterator& that) {
    if (this == &that) {
        return *this;
    }
    fDictionaryCache->reset();    // Delete break cache information
    fBreakType = that.fBreakType;
    if (fLanguageBreakEngines != NULL) {
        delete fLanguageBreakEngines;
        fLanguageBreakEngines = NULL;   // Just rebuild for now
    }
    // TODO: clone fLanguageBreakEngines from "that"
    UErrorCode status = U_ZERO_ERROR;
    fText = utext_clone(fText, that.fText, FALSE, TRUE, &status);

    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        delete fCharIter;
    }
    fCharIter = NULL;

    if (that.fCharIter != NULL ) {
        // This is a little bit tricky - it will intially appear that
        //  this->fCharIter is adopted, even if that->fCharIter was
        //  not adopted.  That's ok.
        fCharIter = that.fCharIter->clone();
    }

    if (fData != NULL) {
        fData->removeReference();
        fData = NULL;
    }
    if (that.fData != NULL) {
        fData = that.fData->addReference();
    }

    return *this;
}



//-----------------------------------------------------------------------------
//
//    init()      Shared initialization routine.   Used by all the constructors.
//                Initializes all fields, leaving the object in a consistent state.
//
//-----------------------------------------------------------------------------
void RuleBasedBreakIterator::init(UErrorCode &status) {
    fText                 = NULL;
    fCharIter             = NULL;
    fSCharIter            = NULL;
    fDCharIter            = NULL;
    fData                 = NULL;
    fLastRuleStatusIndex  = 0;
    fDictionaryCharCount  = 0;
    fBreakType            = UBRK_WORD;  // Defaulting BreakType to word gives reasonable
                                        //   dictionary behavior for Break Iterators that are
                                        //   built from rules.  Even better would be the ability to
                                        //   declare the type in the rules.

    fLanguageBreakEngines = NULL;
    fUnhandledBreakEngine = NULL;
    fDictionaryCache      = NULL;

    if (U_FAILURE(status)) {
        return;
    }

    fText            = utext_openUChars(NULL, NULL, 0, &status);
    fDictionaryCache = new RBBIDictCache(status);
    if (U_SUCCESS(status) && (fText == NULL || fDictionaryCache == NULL)) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }

#ifdef RBBI_DEBUG
    static UBool debugInitDone = FALSE;
    if (debugInitDone == FALSE) {
        char *debugEnv = getenv("U_RBBIDEBUG");
        if (debugEnv && uprv_strstr(debugEnv, "trace")) {
            gTrace = TRUE;
        }
        debugInitDone = TRUE;
    }
#endif
}



//-----------------------------------------------------------------------------
//
//    clone - Returns a newly-constructed RuleBasedBreakIterator with the same
//            behavior, and iterating over the same text, as this one.
//            Virtual function: does the right thing with subclasses.
//
//-----------------------------------------------------------------------------
BreakIterator*
RuleBasedBreakIterator::clone(void) const {
    return new RuleBasedBreakIterator(*this);
}

/**
 * Equality operator.  Returns TRUE if both BreakIterators are of the
 * same class, have the same behavior, and iterate over the same text.
 */
UBool
RuleBasedBreakIterator::operator==(const BreakIterator& that) const {
    if (typeid(*this) != typeid(that)) {
        return FALSE;
    }

    const RuleBasedBreakIterator& that2 = (const RuleBasedBreakIterator&) that;

    if (!utext_equals(fText, that2.fText)) {
        // The two break iterators are operating on different text,
        //   or have a different interation position.
        return FALSE;
    };

    // TODO:  need a check for when in a dictionary region at different offsets.

    if (that2.fData == fData ||
        (fData != NULL && that2.fData != NULL && *that2.fData == *fData)) {
            // The two break iterators are using the same rules.
            return TRUE;
        }
    return FALSE;
}

/**
 * Compute a hash code for this BreakIterator
 * @return A hash code
 */
int32_t
RuleBasedBreakIterator::hashCode(void) const {
    int32_t   hash = 0;
    if (fData != NULL) {
        hash = fData->hashCode();
    }
    return hash;
}


void RuleBasedBreakIterator::setText(UText *ut, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    fDictionaryCache->reset();
    fText = utext_clone(fText, ut, FALSE, TRUE, &status);

    // Set up a dummy CharacterIterator to be returned if anyone
    //   calls getText().  With input from UText, there is no reasonable
    //   way to return a characterIterator over the actual input text.
    //   Return one over an empty string instead - this is the closest
    //   we can come to signaling a failure.
    //   (GetText() is obsolete, this failure is sort of OK)
    if (fDCharIter == NULL) {
        static const UChar c = 0;
        fDCharIter = new UCharCharacterIterator(&c, 0);
        if (fDCharIter == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        // existing fCharIter was adopted from the outside.  Delete it now.
        delete fCharIter;
    }
    fCharIter = fDCharIter;

    this->first();
}


UText *RuleBasedBreakIterator::getUText(UText *fillIn, UErrorCode &status) const {
    UText *result = utext_clone(fillIn, fText, FALSE, TRUE, &status);
    return result;
}



/**
 * Returns the description used to create this iterator
 */
const UnicodeString&
RuleBasedBreakIterator::getRules() const {
    if (fData != NULL) {
        return fData->getRuleSourceString();
    } else {
        static const UnicodeString *s;
        if (s == NULL) {
            // TODO:  something more elegant here.
            //        perhaps API should return the string by value.
            //        Note:  thread unsafe init & leak are semi-ok, better than
            //               what was before.  Sould be cleaned up, though.
            s = new UnicodeString;
        }
        return *s;
    }
}

//=======================================================================
// BreakIterator overrides
//=======================================================================

/**
 * Return a CharacterIterator over the text being analyzed.
 */
CharacterIterator&
RuleBasedBreakIterator::getText() const {
    return *fCharIter;
}

/**
 * Set the iterator to analyze a new piece of text.  This function resets
 * the current iteration position to the beginning of the text.
 * @param newText An iterator over the text to analyze.
 */
void
RuleBasedBreakIterator::adoptText(CharacterIterator* newText) {
    // If we are holding a CharacterIterator adopted from a
    //   previous call to this function, delete it now.
    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        delete fCharIter;
    }

    fCharIter = newText;
    UErrorCode status = U_ZERO_ERROR;
    fDictionaryCache->reset();
    if (newText==NULL || newText->startIndex() != 0) {
        // startIndex !=0 wants to be an error, but there's no way to report it.
        // Make the iterator text be an empty string.
        fText = utext_openUChars(fText, NULL, 0, &status);
    } else {
        fText = utext_openCharacterIterator(fText, newText, &status);
    }
    this->first();
}

/**
 * Set the iterator to analyze a new piece of text.  This function resets
 * the current iteration position to the beginning of the text.
 * @param newText An iterator over the text to analyze.
 */
void
RuleBasedBreakIterator::setText(const UnicodeString& newText) {
    UErrorCode status = U_ZERO_ERROR;
    fDictionaryCache->reset();
    fText = utext_openConstUnicodeString(fText, &newText, &status);

    // Set up a character iterator on the string.
    //   Needed in case someone calls getText().
    //  Can not, unfortunately, do this lazily on the (probably never)
    //  call to getText(), because getText is const.
    if (fSCharIter == NULL) {
        fSCharIter = new StringCharacterIterator(newText);
    } else {
        fSCharIter->setText(newText);
    }

    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        // old fCharIter was adopted from the outside.  Delete it.
        delete fCharIter;
    }
    fCharIter = fSCharIter;

    this->first();
}


/**
 *  Provide a new UText for the input text.  Must reference text with contents identical
 *  to the original.
 *  Intended for use with text data originating in Java (garbage collected) environments
 *  where the data may be moved in memory at arbitrary times.
 */
RuleBasedBreakIterator &RuleBasedBreakIterator::refreshInputText(UText *input, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    if (input == NULL) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return *this;
    }
    int64_t pos = utext_getNativeIndex(fText);
    //  Shallow read-only clone of the new UText into the existing input UText
    fText = utext_clone(fText, input, FALSE, TRUE, &status);
    if (U_FAILURE(status)) {
        return *this;
    }
    utext_setNativeIndex(fText, pos);
    if (utext_getNativeIndex(fText) != pos) {
        // Sanity check.  The new input utext is supposed to have the exact same
        // contents as the old.  If we can't set to the same position, it doesn't.
        // The contents underlying the old utext might be invalid at this point,
        // so it's not safe to check directly.
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return *this;
}


/**
 * Sets the current iteration position to the beginning of the text, position zero.
 * @return The new iterator position, which is zero.
 */
int32_t RuleBasedBreakIterator::first(void) {
    fDictionaryCache->reset();
    fLastRuleStatusIndex  = 0;
    if (fText == NULL) {
        return BreakIterator::DONE;
    }
    utext_setNativeIndex(fText, 0);
    return 0;
}

/**
 * Sets the current iteration position to the end of the text.
 * @return The text's past-the-end offset.
 */
int32_t RuleBasedBreakIterator::last(void) {
    fDictionaryCache->reset();
    if (fText == NULL) {
        fLastRuleStatusIndex  = 0;
        return BreakIterator::DONE;
    }

    // TODO: rule status will not be valid. Fix it somehow.
    int32_t pos = (int32_t)utext_nativeLength(fText);
    utext_setNativeIndex(fText, pos);
    return pos;
}

/**
 * Advances the iterator either forward or backward the specified number of steps.
 * Negative values move backward, and positive values move forward.  This is
 * equivalent to repeatedly calling next() or previous().
 * @param n The number of steps to move.  The sign indicates the direction
 * (negative is backwards, and positive is forwards).
 * @return The character offset of the boundary position n boundaries away from
 * the current one.
 */
int32_t RuleBasedBreakIterator::next(int32_t n) {
    int32_t result = current();
    while (n > 0) {
        result = next();
        --n;
    }
    while (n < 0) {
        result = previous();
        ++n;
    }
    return result;
}

/**
 * Advances the iterator to the next boundary position.
 * @return The position of the first boundary after this one.
 */
int32_t RuleBasedBreakIterator::next(void) {
    // if we have cached break positions and we're still in the range
    // covered by them, just move one step forward in the cache

    int32_t startPos = current();
    int32_t dictResult = 0;
    if (fDictionaryCache->following(startPos, &dictResult, &fLastRuleStatusIndex)) {
        utext_setNativeIndex(fText, dictResult);
        return dictResult;
    }

    int32_t prevStatus = fLastRuleStatusIndex;
    int32_t ruleResult = handleNext(fData->fForwardTable);
    if (fDictionaryCharCount > 0) {
        checkDictionary(startPos, ruleResult, prevStatus, fLastRuleStatusIndex);
        if (fDictionaryCache->following(startPos, &dictResult, &fLastRuleStatusIndex)) {
            utext_setNativeIndex(fText, dictResult);
            return dictResult;
        }
    }
    return ruleResult;
}

/**
 * Move the iterator backwards, to the boundary preceding the current one.
 *
 *         Starts from the current position within fText.
 *         Starting position need not be on a boundary.
 *
 * @return The position of the boundary position immediately preceding the starting position.
 */
int32_t RuleBasedBreakIterator::previous(void) {
    int32_t startPos = current();

    // if we're already positioned at the beginning of the text, return DONE
    if (fText == NULL || startPos == 0) {
        fLastRuleStatusIndex  = 0;
        return BreakIterator::DONE;
    }

    // if we have cached break positions and we're still in the range
    // covered by them, move one step backward in the cache
    int32_t dictResult = 0;
    if (fDictionaryCache->preceding(startPos, &dictResult, &fLastRuleStatusIndex)) {
        utext_setNativeIndex(fText, dictResult);
        return dictResult;
    }


    // Loop, backing up a somewhat arbitrary amount on each interation, until we have
    //       reach a position before the boundary that we are looking for.

    int32_t lastPos = 0;
    int32_t lastTag = 0;
    int32_t nextPos = 0;
    int32_t backupPos = startPos;

    for (;;) {
        utext_setNativeIndex(fText, backupPos - 10);
        backupPos = handlePrevious(fData->fSafeRevTable);

        if (backupPos == UBRK_DONE) {
            backupPos = 0;
            utext_setNativeIndex(fText, 0);
        }
        U_ASSERT(backupPos < startPos);

        // Position is now on a "safe point", from which the forward rules will work correctly.
        // Current position may or may not be an actual boundary.

        lastPos = backupPos;
        for (;;) {
            nextPos = handleNext(fData->fForwardTable);
            if (nextPos == BreakIterator::DONE || nextPos >= startPos) {
                break;
            }
            lastPos = nextPos;
            lastTag = fLastRuleStatusIndex;
        }

        if (backupPos < lastPos) {
            // Good candidate boundary of lastPos
            break;
        }
        if (backupPos == 0 && lastPos == 0) {
            // result is 0. Can't get before 0 to advance forwards to the boundary (at 0)
            // which is why this is a separate case.
            break;
        }
        // Didn't back up enough. Repeat the loop.
    }

    if (fDictionaryCharCount > 0) {
        U_ASSERT(nextPos > lastPos);
        checkDictionary(lastPos, nextPos, lastTag, fLastRuleStatusIndex);
        if (fDictionaryCache->preceding(startPos, &dictResult, &fLastRuleStatusIndex)) {
            utext_setNativeIndex(fText, dictResult);
            return dictResult;
        }
    }

    utext_setNativeIndex(fText, lastPos);
    fLastRuleStatusIndex  = lastTag;       // for use by getRuleStatus()

    return lastPos;
}

/**
 * Sets the iterator to refer to the first boundary position following
 * the specified position.
 * @startPos The position from which to begin searching for a break position.
 * @return The position of the first break after the current position.
 */
int32_t RuleBasedBreakIterator::following(int32_t startPos) {
    // if the offset passed in is already past the end of the text,
    // just return DONE; if it's before the beginning, return the
    // text's starting offset
    if (fText == NULL || startPos >= utext_nativeLength(fText)) {
        last();
        return next();
    }
    if (startPos < 0) {
        return first();
    }

    // Back up to a safe point, then
    utext_setNativeIndex(fText, startPos);
    int32_t backupPos = handlePrevious(fData->fSafeRevTable);
    (void)backupPos;
    int32_t result = 0;
    do {
        result = next();
    } while (result != BreakIterator::DONE && result <= startPos);
    return result;
}

/**
 * Sets the iterator to refer to the last boundary position before the
 * specified position.
 * @offset The position to begin searching for a break from.
 * @return The position of the last boundary before the starting position.
 */
int32_t RuleBasedBreakIterator::preceding(int32_t offset) {
    // if the offset passed in is already past the end of the text,
    // just return DONE; if it's before the beginning, return the
    // text's starting offset
    if (fText == NULL || offset > utext_nativeLength(fText)) {
        return last();
    }
    if (offset <= 0) {
        return first();
    }

    // Move requested offset to a code point start. It might be on a trail surrogate,
    // or on a trail byte if the input is UTF-8.
    utext_setNativeIndex(fText, offset);

    return previous();
}

/**
 * Returns true if the specfied position is a boundary position.  As a side
 * effect, leaves the iterator pointing to the first boundary position at
 * or after "offset".
 * @param offset the offset to check.
 * @return True if "offset" is a boundary position.
 */
UBool RuleBasedBreakIterator::isBoundary(int32_t offset) {
    // the beginning index of the iterator is always a boundary position by definition
    if (offset == 0) {
        first();       // For side effects on current position, tag values.
        return TRUE;
    }

    if (offset == (int32_t)utext_nativeLength(fText)) {
        last();       // For side effects on current position, tag values.
        return TRUE;
    }

    // out-of-range indexes are never boundary positions
    if (offset < 0) {
        first();       // For side effects on current position, tag values.
        return FALSE;
    }

    if (offset > utext_nativeLength(fText)) {
        last();        // For side effects on current position, tag values.
        return FALSE;
    }

    // otherwise, we can use following() on the position before the specified
    // one and return true if the position we get back is the one the user
    // specified
    utext_previous32From(fText, offset);
    int32_t backOne = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    UBool    result  = following(backOne) == offset;
    return result;
}

/**
 * Returns the current iteration position.
 * @return The current iteration position.
 */
int32_t RuleBasedBreakIterator::current(void) const {
    int32_t  pos = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    return pos;
}

//=======================================================================
// implementation
//=======================================================================

//
// RBBIRunMode  -  the state machine runs an extra iteration at the beginning and end
//                 of user text.  A variable with this enum type keeps track of where we
//                 are.  The state machine only fetches user input while in the RUN mode.
//
enum RBBIRunMode {
    RBBI_START,     // state machine processing is before first char of input
    RBBI_RUN,       // state machine processing is in the user text
    RBBI_END        // state machine processing is after end of user text.
};


// Map from look-ahead break states (corresponds to rules) to boundary positions.
// Allows multiple lookahead break rules to be in flight at the same time.
//
// This is a temporary approach for ICU 57. A better fix is to make the look-ahead numbers
// in the state table be sequential, then we can just index an array. And the
// table could also tell us in advance how big that array needs to be.
//
// Before ICU 57 there was just a single simple variable for a look-ahead match that
// was in progress. Two rules at once did not work.

static const int32_t kMaxLookaheads = 8;
struct LookAheadResults {
    int32_t    fUsedSlotLimit;
    int32_t    fPositions[8];
    int16_t    fKeys[8];

    LookAheadResults() : fUsedSlotLimit(0), fPositions(), fKeys() {};

    int32_t getPosition(int16_t key) {
        for (int32_t i=0; i<fUsedSlotLimit; ++i) {
            if (fKeys[i] == key) {
                return fPositions[i];
            }
        }
        U_ASSERT(FALSE);
        return -1;
    }

    void setPosition(int16_t key, int32_t position) {
        int32_t i;
        for (i=0; i<fUsedSlotLimit; ++i) {
            if (fKeys[i] == key) {
                fPositions[i] = position;
                return;
            }
        }
        if (i >= kMaxLookaheads) {
            U_ASSERT(FALSE);
            i = kMaxLookaheads - 1;
        }
        fKeys[i] = key;
        fPositions[i] = position;
        U_ASSERT(fUsedSlotLimit == i);
        fUsedSlotLimit = i + 1;
    }
};


//-----------------------------------------------------------------------------------
//
//  handleNext(stateTable)
//     This method is the actual implementation of the rbbi next() method.
//     This method initializes the state machine to state 1
//     and advances through the text character by character until we reach the end
//     of the text or the state machine transitions to state 0.  We update our return
//     value every time the state machine passes through an accepting state.
//
//-----------------------------------------------------------------------------------
int32_t RuleBasedBreakIterator::handleNext(const RBBIStateTable *statetable) {
    int32_t             state;
    uint16_t            category        = 0;
    RBBIRunMode         mode;

    RBBIStateTableRow  *row;
    UChar32             c;
    LookAheadResults    lookAheadMatches;
    int32_t             result             = 0;
    int32_t             initialPosition    = 0;
    const char         *tableData          = statetable->fTableData;
    uint32_t            tableRowLen        = statetable->fRowLen;

    #ifdef RBBI_DEBUG
        if (gTrace) {
            RBBIDebugPuts("Handle Next   pos   char  state category");
        }
    #endif

    // No matter what, handleNext alway correctly sets the break tag value.
    fLastRuleStatusIndex = 0;

    fDictionaryCharCount = 0;

    // if we're already at the end of the text, return DONE.
    initialPosition = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    result          = initialPosition;
    c               = UTEXT_NEXT32(fText);
    if (fData == NULL || c==U_SENTINEL) {
        return BreakIterator::DONE;
    }

    //  Set the initial state for the state machine
    state = START_STATE;
    row = (RBBIStateTableRow *)
            //(statetable->fTableData + (statetable->fRowLen * state));
            (tableData + tableRowLen * state);


    mode     = RBBI_RUN;
    if (statetable->fFlags & RBBI_BOF_REQUIRED) {
        category = 2;
        mode     = RBBI_START;
    }


    // loop until we reach the end of the text or transition to state 0
    //
    for (;;) {
        if (c == U_SENTINEL) {
            // Reached end of input string.
            if (mode == RBBI_END) {
                // We have already run the loop one last time with the
                //   character set to the psueudo {eof} value.  Now it is time
                //   to unconditionally bail out.
                break;
            }
            // Run the loop one last time with the fake end-of-input character category.
            mode = RBBI_END;
            category = 1;
        }

        //
        // Get the char category.  An incoming category of 1 or 2 means that
        //      we are preset for doing the beginning or end of input, and
        //      that we shouldn't get a category from an actual text input character.
        //
        if (mode == RBBI_RUN) {
            // look up the current character's character category, which tells us
            // which column in the state table to look at.
            // Note:  the 16 in UTRIE_GET16 refers to the size of the data being returned,
            //        not the size of the character going in, which is a UChar32.
            //
            UTRIE_GET16(&fData->fTrie, c, category);

            // Check the dictionary bit in the character's category.
            //    Counter is only used by dictionary based iterators (subclasses).
            //    Chars that need to be handled by a dictionary have a flag bit set
            //    in their category values.
            //
            if ((category & 0x4000) != 0)  {
                fDictionaryCharCount++;
                //  And off the dictionary flag bit.
                category &= ~0x4000;
            }
        }

       #ifdef RBBI_DEBUG
            if (gTrace) {
                RBBIDebugPrintf("             %4ld   ", utext_getNativeIndex(fText));
                if (0x20<=c && c<0x7f) {
                    RBBIDebugPrintf("\"%c\"  ", c);
                } else {
                    RBBIDebugPrintf("%5x  ", c);
                }
                RBBIDebugPrintf("%3d  %3d\n", state, category);
            }
        #endif

        // State Transition - move machine to its next state
        //

        // Note: fNextState is defined as uint16_t[2], but we are casting
        // a generated RBBI table to RBBIStateTableRow and some tables
        // actually have more than 2 categories.
        U_ASSERT(category<fData->fHeader->fCatCount);
        state = row->fNextState[category];  /*Not accessing beyond memory*/
        row = (RBBIStateTableRow *)
            // (statetable->fTableData + (statetable->fRowLen * state));
            (tableData + tableRowLen * state);


        if (row->fAccepting == -1) {
            // Match found, common case.
            if (mode != RBBI_START) {
                result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
            }
            fLastRuleStatusIndex = row->fTagIdx;   // Remember the break status (tag) values.
        }

        int16_t completedRule = row->fAccepting;
        if (completedRule > 0) {
            // Lookahead match is completed.
            int32_t lookaheadResult = lookAheadMatches.getPosition(completedRule);
            if (lookaheadResult >= 0) {
                fLastRuleStatusIndex = row->fTagIdx;
                UTEXT_SETNATIVEINDEX(fText, lookaheadResult);
                return lookaheadResult;
            }
        }
        int16_t rule = row->fLookAhead;
        if (rule != 0) {
            // At the position of a '/' in a look-ahead match. Record it.
            int32_t  pos = (int32_t)UTEXT_GETNATIVEINDEX(fText);
            lookAheadMatches.setPosition(rule, pos);
        }

        if (state == STOP_STATE) {
            // This is the normal exit from the lookup state machine.
            // We have advanced through the string until it is certain that no
            //   longer match is possible, no matter what characters follow.
            break;
        }

        // Advance to the next character.
        // If this is a beginning-of-input loop iteration, don't advance
        //    the input position.  The next iteration will be processing the
        //    first real input character.
        if (mode == RBBI_RUN) {
            c = UTEXT_NEXT32(fText);
        } else {
            if (mode == RBBI_START) {
                mode = RBBI_RUN;
            }
        }


    }

    // The state machine is done.  Check whether it found a match...

    // If the iterator failed to advance in the match engine, force it ahead by one.
    //   (This really indicates a defect in the break rules.  They should always match
    //    at least one character.)
    if (result == initialPosition) {
        UTEXT_SETNATIVEINDEX(fText, initialPosition);
        UTEXT_NEXT32(fText);
        result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    }

    // Leave the iterator at our result position.
    UTEXT_SETNATIVEINDEX(fText, result);
    #ifdef RBBI_DEBUG
        if (gTrace) {
            RBBIDebugPrintf("result = %d\n\n", result);
        }
    #endif
    return result;
}



//-----------------------------------------------------------------------------------
//
//  handlePrevious()
//
//      Iterate backwards, according to the logic of the reverse rules.
//      This version handles the exact style backwards rules.
//
//      The logic of this function is very similar to handleNext(), above.
//
//-----------------------------------------------------------------------------------
int32_t RuleBasedBreakIterator::handlePrevious(const RBBIStateTable *statetable) {
    int32_t             state;
    uint16_t            category        = 0;
    RBBIRunMode         mode;
    RBBIStateTableRow  *row;
    UChar32             c;
    LookAheadResults    lookAheadMatches;
    int32_t             result          = 0;
    int32_t             initialPosition = 0;

    #ifdef RBBI_DEBUG
        if (gTrace) {
            RBBIDebugPuts("Handle Previous   pos   char  state category");
        }
    #endif

    // if we're already at the start of the text, return DONE.
    if (fText == NULL || fData == NULL || UTEXT_GETNATIVEINDEX(fText)==0) {
        return BreakIterator::DONE;
    }

    //  Set up the starting char.
    initialPosition = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    result          = initialPosition;
    c               = UTEXT_PREVIOUS32(fText);

    //  Set the initial state for the state machine
    state = START_STATE;
    row = (RBBIStateTableRow *)
            (statetable->fTableData + (statetable->fRowLen * state));
    category = 3;
    mode     = RBBI_RUN;
    if (statetable->fFlags & RBBI_BOF_REQUIRED) {
        category = 2;
        mode     = RBBI_START;
    }


    // loop until we reach the start of the text or transition to state 0
    //
    for (;;) {
        if (c == U_SENTINEL) {
            // Reached end of input string.
            if (mode == RBBI_END) {
                // We have already run the loop one last time with the
                //   character set to the psueudo {eof} value.  Now it is time
                //   to unconditionally bail out.
                if (result == initialPosition) {
                    // Ran off start, no match found.
                    // move one index one (towards the start, since we are doing a previous())
                    UTEXT_SETNATIVEINDEX(fText, initialPosition);
                    (void)UTEXT_PREVIOUS32(fText);   // TODO:  shouldn't be necessary.  We're already at beginning.  Check.
                }
                break;
            }
            // Run the loop one last time with the fake end-of-input character category.
            mode = RBBI_END;
            category = 1;
        }

        //
        // Get the char category.  An incoming category of 1 or 2 means that
        //      we are preset for doing the beginning or end of input, and
        //      that we shouldn't get a category from an actual text input character.
        //
        if (mode == RBBI_RUN) {
            // look up the current character's character category, which tells us
            // which column in the state table to look at.
            // Note:  the 16 in UTRIE_GET16 refers to the size of the data being returned,
            //        not the size of the character going in, which is a UChar32.
            //
            //  And off the dictionary flag bit. For reverse iteration it is not used.
            UTRIE_GET16(&fData->fTrie, c, category);
            category &= ~0x4000;
        }

        #ifdef RBBI_DEBUG
            if (gTrace) {
                RBBIDebugPrintf("             %4d   ", (int32_t)utext_getNativeIndex(fText));
                if (0x20<=c && c<0x7f) {
                    RBBIDebugPrintf("\"%c\"  ", c);
                } else {
                    RBBIDebugPrintf("%5x  ", c);
                }
                RBBIDebugPrintf("%3d  %3d\n", state, category);
            }
        #endif

        // State Transition - move machine to its next state
        //

        // Note: fNextState is defined as uint16_t[2], but we are casting
        // a generated RBBI table to RBBIStateTableRow and some tables
        // actually have more than 2 categories.
        U_ASSERT(category<fData->fHeader->fCatCount);
        state = row->fNextState[category];  /*Not accessing beyond memory*/
        row = (RBBIStateTableRow *)
            (statetable->fTableData + (statetable->fRowLen * state));

        if (row->fAccepting == -1) {
            // Match found, common case.
            result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
        }

        int16_t completedRule = row->fAccepting;
        if (completedRule > 0) {
            // Lookahead match is completed.
            int32_t lookaheadResult = lookAheadMatches.getPosition(completedRule);
            if (lookaheadResult >= 0) {
                UTEXT_SETNATIVEINDEX(fText, lookaheadResult);
                return lookaheadResult;
            }
        }
        int16_t rule = row->fLookAhead;
        if (rule != 0) {
            // At the position of a '/' in a look-ahead match. Record it.
            int32_t  pos = (int32_t)UTEXT_GETNATIVEINDEX(fText);
            lookAheadMatches.setPosition(rule, pos);
        }

        if (state == STOP_STATE) {
            // This is the normal exit from the lookup state machine.
            // We have advanced through the string until it is certain that no
            //   longer match is possible, no matter what characters follow.
            break;
        }

        // Move (backwards) to the next character to process.
        // If this is a beginning-of-input loop iteration, don't advance
        //    the input position.  The next iteration will be processing the
        //    first real input character.
        if (mode == RBBI_RUN) {
            c = UTEXT_PREVIOUS32(fText);
        } else {
            if (mode == RBBI_START) {
                mode = RBBI_RUN;
            }
        }
    }

    // The state machine is done.  Check whether it found a match...

    // If the iterator failed to advance in the match engine, force it ahead by one.
    //   (This really indicates a defect in the break rules.  They should always match
    //    at least one character.)
    if (result == initialPosition) {
        UTEXT_SETNATIVEINDEX(fText, initialPosition);
        UTEXT_PREVIOUS32(fText);
        result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    }

    // Leave the iterator at our result position.
    UTEXT_SETNATIVEINDEX(fText, result);
    #ifdef RBBI_DEBUG
        if (gTrace) {
            RBBIDebugPrintf("result = %d\n\n", result);
        }
    #endif
    return result;
}


//-------------------------------------------------------------------------------
//
//   getRuleStatus()   Return the break rule tag associated with the current
//                     iterator position.  If the iterator arrived at its current
//                     position by iterating forwards, the value will have been
//                     cached by the handleNext() function.
//
//-------------------------------------------------------------------------------

int32_t  RuleBasedBreakIterator::getRuleStatus() const {

    // fLastRuleStatusIndex indexes to the start of the appropriate status record
    //                                                 (the number of status values.)
    //   This function returns the last (largest) of the array of status values.
    int32_t  idx = fLastRuleStatusIndex + fData->fRuleStatusTable[fLastRuleStatusIndex];
    int32_t  tagVal = fData->fRuleStatusTable[idx];

    return tagVal;
}


int32_t RuleBasedBreakIterator::getRuleStatusVec(
             int32_t *fillInVec, int32_t capacity, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return 0;
    }

    int32_t  numVals = fData->fRuleStatusTable[fLastRuleStatusIndex];
    int32_t  numValsToCopy = numVals;
    if (numVals > capacity) {
        status = U_BUFFER_OVERFLOW_ERROR;
        numValsToCopy = capacity;
    }
    int i;
    for (i=0; i<numValsToCopy; i++) {
        fillInVec[i] = fData->fRuleStatusTable[fLastRuleStatusIndex + i + 1];
    }
    return numVals;
}



//-------------------------------------------------------------------------------
//
//   getBinaryRules        Access to the compiled form of the rules,
//                         for use by build system tools that save the data
//                         for standard iterator types.
//
//-------------------------------------------------------------------------------
const uint8_t  *RuleBasedBreakIterator::getBinaryRules(uint32_t &length) {
    const uint8_t  *retPtr = NULL;
    length = 0;

    if (fData != NULL) {
        retPtr = (const uint8_t *)fData->fHeader;
        length = fData->fHeader->fLength;
    }
    return retPtr;
}


BreakIterator *  RuleBasedBreakIterator::createBufferClone(void * /*stackBuffer*/,
                                   int32_t &bufferSize,
                                   UErrorCode &status)
{
    if (U_FAILURE(status)){
        return NULL;
    }

    if (bufferSize == 0) {
        bufferSize = 1;  // preflighting for deprecated functionality
        return NULL;
    }

    BreakIterator *clonedBI = clone();
    if (clonedBI == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        status = U_SAFECLONE_ALLOCATED_WARNING;
    }
    return (RuleBasedBreakIterator *)clonedBI;
}


//-------------------------------------------------------------------------------
//
//  checkDictionary       This function handles all processing of characters in
//                        the "dictionary" set. It will determine the appropriate
//                        course of action, and possibly set up a cache in the
//                        process.
//
//-------------------------------------------------------------------------------
void RuleBasedBreakIterator::checkDictionary(int32_t startPos, int32_t endPos,
                                             int32_t firstRuleStatus, int32_t otherRuleStatus) {
    // Reset the old break cache first.
    fDictionaryCache->reset();
    fDictionaryCache->fFirstRuleStatusIndex = firstRuleStatus;
    fDictionaryCache->fOtherRuleStatusIndex = otherRuleStatus;

    // note: code segment below assumes that dictionary chars are in the
    // startPos-endPos range
    if ((endPos - startPos) <= 1) {
        return;
    }

    int64_t initialTextPosition = utext_getNativeIndex(fText);

    UBool reverse = FALSE;

    // Starting from the starting point, scan towards the proposed result,
    // looking for the first dictionary character (which may be the one
    // we're on, if we're starting in the middle of a range).
    utext_setNativeIndex(fText, reverse ? endPos : startPos);
    if (reverse) {
        UTEXT_PREVIOUS32(fText);
    }

    int32_t rangeStart = startPos;
    int32_t rangeEnd = endPos;

    uint16_t    category;
    int32_t     current;
    UErrorCode  status = U_ZERO_ERROR;
    fDictionaryCache->reset();
    UVector32   &breaks = *fDictionaryCache->fBreaks;
    int32_t     foundBreakCount = 0;

    // Is the character we're starting on a dictionary character? If so, we
    // need to back up to include the entire run; otherwise the results of
    // the break algorithm will differ depending on where we start. Since
    // the result is cached and there is typically a non-dictionary break
    // within a small number of words, there should be little performance impact.

    utext_setNativeIndex(fText, endPos);
    UChar32     c = utext_current32(fText);
    UTRIE_GET16(&fData->fTrie, c, category);
    if (category & 0x4000) {
        do {
            utext_next32(fText);          // TODO:  recast to work directly with postincrement.
            c = utext_current32(fText);
            UTRIE_GET16(&fData->fTrie, c, category);
        } while (c != U_SENTINEL && (category & 0x4000));
        // Back up to the last dictionary character
        rangeEnd = (int32_t)UTEXT_GETNATIVEINDEX(fText);
        c = UTEXT_PREVIOUS32(fText);
    }

    utext_setNativeIndex(fText, startPos);
    c = utext_current32(fText);
    UTRIE_GET16(&fData->fTrie, c, category);
    if (category & 0x4000) {
        do {
            c = UTEXT_PREVIOUS32(fText);
            UTRIE_GET16(&fData->fTrie, c, category);
        }
        while (c != U_SENTINEL && (category & 0x4000));
        // Back up to the last dictionary character
        if (c == U_SENTINEL) {
            // c = fText->first32();
            c = utext_current32(fText);
        }
        else {
            utext_next32(fText);
            c = utext_current32(fText);
        }
        rangeStart = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    }

    //printf("startPos, endPos = (%d, %d)\n", startPos, endPos);
    //printf("rangeStart, rangeEnd = (%d, %d)\n", rangeStart, rangeEnd);

    // Loop through the text, looking for ranges of dictionary characters.
    // For each span, find the appropriate break engine, and ask it to find
    // any breaks within the span.
    // Note: we always do this in the forward direction, so that the break
    // cache is built in the right order.
    utext_setNativeIndex(fText, rangeStart);
    c = utext_current32(fText);
    UTRIE_GET16(&fData->fTrie, c, category);

    while(U_SUCCESS(status)) {
        while((current = (int32_t)UTEXT_GETNATIVEINDEX(fText)) < rangeEnd && (category & 0x4000) == 0) {
            utext_next32(fText);           // TODO:  tweak for post-increment operation
            c = utext_current32(fText);
            UTRIE_GET16(&fData->fTrie, c, category);
        }
        if (current >= rangeEnd) {
            break;
        }

        // We now have a dictionary character. Get the appropriate language object
        // to deal with it.
        const LanguageBreakEngine *lbe = getLanguageBreakEngine(c);

        // Ask the language object if there are any breaks. It will leave the text
        // pointer on the other side of its range, ready to search for the next one.
        if (lbe != NULL) {
            foundBreakCount += lbe->findBreaks(fText, rangeStart, rangeEnd, fBreakType, breaks);
        }

        // Reload the loop variables for the next go-round
        c = utext_current32(fText);
        UTRIE_GET16(&fData->fTrie, c, category);
    }

    utext_setNativeIndex(fText, initialTextPosition);

    // If we found breaks, build a new break cache. The first and last entries must
    // be the original starting and ending position.
    // printf("foundBreakCount = %d\n", foundBreakCount);
    if (foundBreakCount > 0) {
        U_ASSERT(foundBreakCount == breaks.size());
        if (startPos < breaks.elementAti(0)) {
            // TODO: ensure this can't happen. Slow insert at start of array.
            breaks.insertElementAt(startPos, 0, status);
        }
        if (endPos > breaks.peeki()) {
            breaks.push(endPos, status);
        }
        fDictionaryCache->fNumCachedBreakPositions = breaks.size();
        fDictionaryCache->fPositionInCache = 0;
        // fDictionaryCache->fStart = startPos;
        // fDictionaryCache->fLimit = endPos;
        // Note: Dictionary matching may extend beyond the original limit.
        fDictionaryCache->fStart = breaks.elementAti(0);
        fDictionaryCache->fLimit = breaks.peeki();
        return;
    }

    // If we get here, there were no language-based breaks.
    return;
}

U_NAMESPACE_END


static icu::UStack *gLanguageBreakFactories = NULL;
static icu::UInitOnce gLanguageBreakFactoriesInitOnce = U_INITONCE_INITIALIZER;

/**
 * Release all static memory held by breakiterator.
 */
U_CDECL_BEGIN
static UBool U_CALLCONV breakiterator_cleanup_dict(void) {
    if (gLanguageBreakFactories) {
        delete gLanguageBreakFactories;
        gLanguageBreakFactories = NULL;
    }
    gLanguageBreakFactoriesInitOnce.reset();
    return TRUE;
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV _deleteFactory(void *obj) {
    delete (icu::LanguageBreakFactory *) obj;
}
U_CDECL_END
U_NAMESPACE_BEGIN

static void U_CALLCONV initLanguageFactories() {
    UErrorCode status = U_ZERO_ERROR;
    U_ASSERT(gLanguageBreakFactories == NULL);
    gLanguageBreakFactories = new UStack(_deleteFactory, NULL, status);
    if (gLanguageBreakFactories != NULL && U_SUCCESS(status)) {
        ICULanguageBreakFactory *builtIn = new ICULanguageBreakFactory(status);
        gLanguageBreakFactories->push(builtIn, status);
#ifdef U_LOCAL_SERVICE_HOOK
        LanguageBreakFactory *extra = (LanguageBreakFactory *)uprv_svc_hook("languageBreakFactory", &status);
        if (extra != NULL) {
            gLanguageBreakFactories->push(extra, status);
        }
#endif
    }
    ucln_common_registerCleanup(UCLN_COMMON_BREAKITERATOR_DICT, breakiterator_cleanup_dict);
}


static const LanguageBreakEngine*
getLanguageBreakEngineFromFactory(UChar32 c, int32_t breakType)
{
    umtx_initOnce(gLanguageBreakFactoriesInitOnce, &initLanguageFactories);
    if (gLanguageBreakFactories == NULL) {
        return NULL;
    }

    int32_t i = gLanguageBreakFactories->size();
    const LanguageBreakEngine *lbe = NULL;
    while (--i >= 0) {
        LanguageBreakFactory *factory = (LanguageBreakFactory *)(gLanguageBreakFactories->elementAt(i));
        lbe = factory->getEngineFor(c, breakType);
        if (lbe != NULL) {
            break;
        }
    }
    return lbe;
}


//-------------------------------------------------------------------------------
//
//  getLanguageBreakEngine  Find an appropriate LanguageBreakEngine for the
//                          the character c.
//
//-------------------------------------------------------------------------------
const LanguageBreakEngine *
RuleBasedBreakIterator::getLanguageBreakEngine(UChar32 c) {
    const LanguageBreakEngine *lbe = NULL;
    UErrorCode status = U_ZERO_ERROR;

    if (fLanguageBreakEngines == NULL) {
        fLanguageBreakEngines = new UStack(status);
        if (fLanguageBreakEngines == NULL || U_FAILURE(status)) {
            delete fLanguageBreakEngines;
            fLanguageBreakEngines = 0;
            return NULL;
        }
    }

    int32_t i = fLanguageBreakEngines->size();
    while (--i >= 0) {
        lbe = (const LanguageBreakEngine *)(fLanguageBreakEngines->elementAt(i));
        if (lbe->handles(c, fBreakType)) {
            return lbe;
        }
    }

    // No existing dictionary took the character. See if a factory wants to
    // give us a new LanguageBreakEngine for this character.
    lbe = getLanguageBreakEngineFromFactory(c, fBreakType);

    // If we got one, use it and push it on our stack.
    if (lbe != NULL) {
        fLanguageBreakEngines->push((void *)lbe, status);
        // Even if we can't remember it, we can keep looking it up, so
        // return it even if the push fails.
        return lbe;
    }

    // No engine is forthcoming for this character. Add it to the
    // reject set. Create the reject break engine if needed.
    if (fUnhandledBreakEngine == NULL) {
        fUnhandledBreakEngine = new UnhandledEngine(status);
        if (U_SUCCESS(status) && fUnhandledBreakEngine == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        // Put it last so that scripts for which we have an engine get tried
        // first.
        fLanguageBreakEngines->insertElementAt(fUnhandledBreakEngine, 0, status);
        // If we can't insert it, or creation failed, get rid of it
        if (U_FAILURE(status)) {
            delete fUnhandledBreakEngine;
            fUnhandledBreakEngine = 0;
            return NULL;
        }
    }

    // Tell the reject engine about the character; at its discretion, it may
    // add more than just the one character.
    fUnhandledBreakEngine->handleCharacter(c, fBreakType);

    return fUnhandledBreakEngine;
}



/*int32_t RuleBasedBreakIterator::getBreakType() const {
    return fBreakType;
}*/

void RuleBasedBreakIterator::setBreakType(int32_t type) {
    fBreakType = type;
    fDictionaryCache->reset();
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
