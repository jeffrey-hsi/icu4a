/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * MajorTestLevel is the top level test class for everything in the directory "IntlWork".
 */

#ifndef _INTLTESTCOLLATOR
#define _INTLTESTCOLLATOR


#include "intltest.h"
#include "unicode/coll.h"
#include "unicode/sortkey.h"
#include "unicode/schriter.h"
#include "unicode/ures.h"
#include "unicode/coleitr.h"
#include "cmemory.h"


class IntlTestCollator: public IntlTest {
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );
protected:
    // These two should probably go down in IntlTest
    void doTest(Collator* col, UnicodeString source, UnicodeString target, Collator::EComparisonResult result);
    void doTestVariant(Collator* col, UnicodeString &source, UnicodeString &target, Collator::EComparisonResult result);
    virtual void reportCResult( UnicodeString &source, UnicodeString &target,
                                CollationKey &sourceKey, CollationKey &targetKey,
                                Collator::EComparisonResult compareResult,
                                Collator::EComparisonResult keyResult,
                                Collator::EComparisonResult incResult,
                                Collator::EComparisonResult expectedResult );

    static UnicodeString &prettify(const CollationKey &source, UnicodeString &target);
    static UnicodeString &appendCompareResult(Collator::EComparisonResult result, UnicodeString &target);
    void backAndForth(CollationElementIterator &iter);
    /**
     * Return an integer array containing all of the collation orders
     * returned by calls to next on the specified iterator
     */
    int32_t *getOrders(CollationElementIterator &iter, int32_t &orderLength);

};


#endif
