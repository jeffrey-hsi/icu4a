/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2008, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _INTLTESTDATEINTERVALFORMAT
#define _INTLTESTDATEINTERVALFORMAT

#include "unicode/utypes.h"
#include "unicode/locid.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

/**
 * Test basic functionality of various API functions
 **/
class DateIntervalFormatTest: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

public:
    /**
     * Performs tests on many API functions, see detailed comments in source code
     **/
    void testAPI();

    /**
     * test formatting
     */
    void testFormat();

private:
    void expect(const char** data, int32_t data_length, const Locale& loc, 
                const char* locName);
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
