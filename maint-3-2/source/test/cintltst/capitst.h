/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2004, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CAPITEST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Converted to C
*********************************************************************************
*//* C API TEST For COLLATOR */

#ifndef _CCOLLAPITST
#define _CCOLLAPITST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cintltst.h"
#define MAX_TOKEN_LEN 16


    /**
     * error reporting utility method
     **/

    static void doAssert(int condition, const char *message);
    /**
     * Collator Class Properties
     * ctor, dtor, createInstance, compare, getStrength/setStrength
     * getDecomposition/setDecomposition, getDisplayName
     */
    void TestProperty(void);
    /**
     * Test RuleBasedCollator and getRules
     **/
    void TestRuleBasedColl(void);
    
    /**
     * Test compare
     **/
    void TestCompare(void);
    /**
     * Test hashCode functionality
     **/
    void TestHashCode(void);
    /**
     * Tests the constructor and numerous other methods for CollationKey
     **/
   void TestSortKey(void);
    /**
     * test the CollationElementIterator methods
     **/
   void TestElemIter(void);
    /**
     * Test ucol_getAvailable and ucol_countAvailable()
     **/
    void TestGetAll(void);
    /**
     * Test ucol_GetDefaultRules ()
    void TestGetDefaultRules(void);
     **/

    void TestDecomposition(void);
    /**
     * Test ucol_safeClone ()
     **/    
    void TestSafeClone(void);

    /**
     * Test getting bounds for a sortkey
     */
    void TestBounds(void);

    /**
     * Test ucol_getLocale function
     */
    void TestGetLocale(void);

    /**
     * Test buffer overrun while having smaller buffer for sortkey (j1865)
     */
    void TestSortKeyBufferOverrun(void);
    /**
     * Test getting and setting of attributes
     */
    void TestGetSetAttr(void);
    /**
     * Test getTailoredSet
     */
    void TestGetTailoredSet(void);

    /**
     * Test mergeSortKeys
     */
    void TestMergeSortKeys(void);

    /**
     * utility function, defined in cmsccoll.c
     */
    void genericLocaleStarter(const char *locale, const char *s[], uint32_t size);


    /** 
     * test short string and collator identifier functions
     */
    static void TestShortString(void);

    /** 
     * test getContractions and getUnsafeSet
     */
    static void TestGetContractionsAndUnsafes(void);


#endif /* #if !UCONFIG_NO_COLLATION */

#endif
