
/*
********************************************************************
* COPYRIGHT: 
* (C) Copyright Taligent, Inc., 1997
* (C) Copyright International Business Machines Corporation, 1997 - 1998
* Licensed Material - Program-Property of IBM - All Rights Reserved. 
* US Government Users Restricted Rights - Use, duplication, or disclosure 
* restricted by GSA ADP Schedule Contract with IBM Corp. 
*
********************************************************************
*/

#ifndef _INTLTESTDATEFORMAT
#define _INTLTESTDATEFORMAT


#include "utypes.h"
#include "intltest.h"


class DateFormat;

/**
 *  Performs some tests in many variations on DateFormat
 **/
class IntlTestDateFormat: public IntlTest {
    void runIndexedTest( int32_t index, bool_t exec, char* &name, char* par = NULL );
    
private:

    /**
     *  test DateFormat::getAvailableLocales
     **/
    void testAvailableLocales(char *par);
    /**
     *  call testLocale for all locales
     **/
    void monsterTest(char *par);

    /**
     *  call tryDate with variations, called by testLocale
     **/
    void testFormat(char *par);
    /**
     *  perform tests using date and fFormat, called in many variations
     **/
    void tryDate(UDate date);
    /**
     *  call testFormat for different DateFormat::EStyle's, etc
     **/
    void testLocale(char *par, const Locale&, const UnicodeString&);
    /**
     *  return a random number
     **/
    double randDouble(void);
    /**
     * generate description for verbose test output
     **/
    void describeTest(void);

    DateFormat *fFormat;
    UnicodeString fTestName;
    int32_t fLimit; // How many iterations it should take to reach convergence

    enum
    {
        // Values in milliseconds (== Date)
        ONESECOND = 1000,
        ONEMINUTE = 60 * ONESECOND,
        ONEHOUR = 60 * ONEMINUTE,
        ONEDAY = 24 * ONEHOUR
    };
    static const double ONEYEAR;
    enum EMode
    {
        GENERIC,
        TIME,
        DATE,
        DATE_TIME
    };
};

#endif
