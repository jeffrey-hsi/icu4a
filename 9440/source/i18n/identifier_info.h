/*
**********************************************************************
*   Copyright (C) 2013, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* indentifier_info.h
* 
* created on: 2013 Jan 7
* created by: Andy Heninger
*/

#ifndef __IDENTIFIER_INFO_H__
#define __IDENTIFIER_INFO_H__

#include "unicode/utypes.h"

#include "unicode/uniset.h"
#include "unicode/uspoof.h"

U_NAMESPACE_BEGIN

class ScriptSet;

/**
 * This class analyzes a possible identifier for script and identifier status. Use it by calling setIdentifierProfile
 * then setIdentifier. Available methods include:
 * <ol>
 * <li>call getScripts for the specific scripts in the identifier. The identifier contains at least one character in
 * each of these.
 * <li>call getAlternates to get cases where a character is not limited to a single script. For example, it could be
 * either Katakana or Hiragana.
 * <li>call getCommonAmongAlternates to find out if any scripts are common to all the alternates.
 * <li>call getNumerics to get a representative character (with value zero) for each of the decimal number systems in
 * the identifier.
 * <li>call getRestrictionLevel to see what the UTS36 restriction level is.
 * </ol>
 * 
 * This is a port from ICU4J of class com.ibm.icu.text.IdentifierInfo
 */
class U_I18N_API IdentifierInfo : public UMemory {

  public:
    /**
     * Create an identifier info object. Subsequently, call setIdentifier(), etc.
     * @internal
     */
    IdentifierInfo();
     
    /**
     * Set the identifier profile: the characters that are to be allowed in the identifier.
     * 
     * @param identifierProfile the characters that are to be allowed in the identifier
     * @return this
     * @internal
     */
    IdentifierInfo &setIdentifierProfile(const UnicodeSet &identifierProfile);

    /**
     * Get the identifier profile: the characters that are to be allowed in the identifier.
     * 
     * @return The characters that are to be allowed in the identifier.
     * @internal
     */
    const UnicodeSet &getIdentifierProfile() const;


    /**
     * Set an identifier to analyze. Afterwards, call methods like getScripts()
     * 
     * @param identifier the identifier to analyze
     * @return this
     * @internal
     */
    IdentifierInfo &setIdentifier(const UnicodeString &identifier);


    /**
     * Get the identifier that was analyzed.
     * 
     * @return the identifier that was analyzed.
     * @internal
     */
    const UnicodeString &getIdentifier() const;
    

    /**
     * Get the scripts found in the identifiers.
     * 
     * @return the set of explicit scripts.
     * @internal
     */
    const ScriptSet &getScripts() const;

    /**
     * Get the set of alternate scripts found in the identifiers. That is, when a character can be in two scripts, then
     * the set consisting of those scripts will be returned.
     * 
     * @return a vector, with each element being of type (ScriptSet *). 
     *         Ownership remains with the IndetifierInfo object, and remains valid
     *         until a new identifer is set or until the object is deleted.
     * @internal
     */
    const UVector &getAlternates() const;

    /**
     * Get the representative characters (zeros) for the numerics found in the identifier.
     * 
     * @return the set of explicit scripts.
     * @internal
     */
    const UnicodeSet &getNumerics() const;

    /**
     * Find out which scripts are in common among the alternates.
     * 
     * @return the set of scripts that are in common among the alternates.
     * @internal
     */
    const ScriptSet &getCommonAmongAlternates() const;

    /**
     * Find the "tightest" restriction level that the identifier satisfies.
     * 
     * @return the restriction level.
     * @internal
     */
    URestrictionLevel getRestrictionLevel() const;

    UnicodeString toString() const;

    /**
     * Produce a readable string of alternates.
     * 
     * @param alternates a vector of UScriptSets.
     * @return display form
     * @internal
     */
    static UnicodeString displayAlternates(UVector alternates);

};

U_NAMESPACE_END

#endif // __IDENTIFIER_INFO_H__

