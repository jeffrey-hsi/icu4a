// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2016 International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _INTLTESTDATETIMEPATTERNGENERATORAPI
#define _INTLTESTDATETIMEPATTERNGENERATORAPI

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

/**
 * Test basic functionality of various API functions
 **/
class IntlTestDateTimePatternGeneratorAPI : public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

private:
    /**
     * Performs tests on many API functions, see detailed comments in source code
     **/
    void testAPI(/* char* par */);
    void testOptions(/* char* par */);
    void testAllFieldPatterns(/* char* par */);
    void testStaticGetSkeleton(/* char* par */);
    void testC();
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
