/*
 ******************************************************************************
 *   Copyright (C) 1996-2008, International Business Machines                 *
 *   Corporation and others.  All Rights Reserved.                            *
 ******************************************************************************
 */

/**
 * \file 
 * \brief C++ API: Boyer-Moore StringSearch prototype
 * \internal ICU 4.0.1 technology preview
 */
 
#ifndef B_M_SEARCH_H
#define B_M_SEARCH_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"

#include "unicode/colldata.h"

U_NAMESPACE_BEGIN

class BadCharacterTable;
class GoodSuffixTable;
class Target;

/*
 * BoyerMooreSearch
 *
 * This object holds the information needed to do a Collation sensitive Boyer-Moore search. It encapulates
 * the "bad character" and "good suffix" tables, the Collator-based data needed to compute them, and a reference
 * to the text being searched.
 *
 * NOTE: This is a technology preview. The final version of this API may not bear any resenblence to this API.
 *
 * @internal ICU 4.0.1 technology preview
 */
class U_I18N_API BoyerMooreSearch : public UObject
{
public:
    /**
     * Construct a <code>BoyerMooreSearch</code> object.
     *
     * @param theData - A <code>CollData</code> object holding the Collator-sensitive data
     * @param patternString - the string for which to search
     * @param targetString - the string in which to search
     *
     * @internal ICU 4.0.1 technology preview
     */
    BoyerMooreSearch(CollData *theData, const UnicodeString &patternString, const UnicodeString *targetString);

    /**
     * The desstructor
     *
     * @internal ICU 4.0.1 technology preview
     */
    ~BoyerMooreSearch();

    /**
     * Test the pattern to see if it generates any CEs.
     *
     * @return <code>TRUE</code> if the pattern string did not generate any CEs
     *
     * @internal ICU 4.0.1 technology preview
     */
    UBool empty();

    /**
     * Search for the pattern string in the target string.
     *
     * @param offset - the offset in the target string at which to begin the search
     * @param start - will be set to the starting offset of the match, or -1 if there's no match
     * @param end - will be set to the ending offset of the match, or -1 if there's no match
     *
     * @return <code>TRUE</code> if the match succeeds, <code>FALSE</code> otherwise.
     *
     * @internal ICU 4.0.1 technology preview
     */
    UBool search(int32_t offset, int32_t &start, int32_t &end);

    /**
     * Set the target string for the match.
     *
     * @param targetString - the new target string
     *
     * @internal ICU 4.0.1 technology preview
     */
    void setTargetString(const UnicodeString *targetString);

    // **** no longer need these? ****
    /**
     * Return the <code>CollData</code> object used for searching
     *
     * @return the <code>CollData</code> object used for searching
     *
     * @internal ICU 4.0.1 technology preview
     */
    CollData *getData();

    /**
     * Return the CEs generated by the pattern string.
     *
     * @return a <code>CEList</code> object holding the CEs generated by the pattern string.
     *
     * @internal ICU 4.0.1 technology preview
     */
    CEList   *getPatternCEs();

    /**
     * Return the <code>BadCharacterTable</code> object computed for the pattern string.
     *
     * @return the <code>BadCharacterTable</code> object.
     *
     * @internal ICU 4.0.1 technology preview
     */
    BadCharacterTable *getBadCharacterTable();

    /**
     * Return the <code>GoodSuffixTable</code> object computed for the pattern string.
     *
     * @return the <code>GoodSuffixTable</code> object computed for the pattern string.
     *
     * @internal ICU 4.0.1 technology preview
     */
    GoodSuffixTable   *getGoodSuffixTable();

    /*
     * UObject glue...
     */
    virtual UClassID getDynamicClassID() const;
    static UClassID getStaticClassID();
    
private:
    UBool ownData;
    CollData *data;
    CEList *patCEs;
    BadCharacterTable *badCharacterTable;
    GoodSuffixTable   *goodSuffixTable;
    Target *target;
};

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_COLLATION
#endif // #ifndef B_M_SEARCH_H
