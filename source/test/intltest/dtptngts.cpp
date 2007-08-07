/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#if !UCONFIG_NO_FORMATTING

#include <stdio.h>
#include <stdlib.h>
#include "dtptngts.h" 

#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/dtptngen.h"
#include "unicode/utypes.h"
#include "loctest.h"

static const UnicodeString patternData[] = {
    UnicodeString("yM"),
    UnicodeString("yMMM"),
    UnicodeString("yMd"),
    UnicodeString("yMMMd"),
    UnicodeString("Md"),
    UnicodeString("MMMd"),
    UnicodeString("yQQQ"),
    UnicodeString("hhmm"),
    UnicodeString("HHmm"),
    UnicodeString("mmss"),
    UnicodeString(""),
 };
 
#define MAX_LOCALE   4  
static const char* testLocale[MAX_LOCALE][3] = {
    {"en", "US","\0"},
    {"zh", "Hans", "CN"},
    {"de","DE", "\0"},
    {"fi","\0", "\0"},
 };
 
static const UnicodeString patternResults[] = {
    UnicodeString("1/1999"),  // en_US
    UnicodeString("Jan 1999"),
    UnicodeString("1/13/1999"),
    UnicodeString("Jan/13/1999"),
    UnicodeString("1/13"),
    UnicodeString("Jan 13"),
    UnicodeString("Q1 1999"),
    UnicodeString("11:58 PM"),
    UnicodeString("23:58"),
    UnicodeString("58:59"),
    UnicodeString("1999-1"),  // zh_Hans_CN
    UnicodeString("1999 1"),
    CharsToUnicodeString("1999\\u5E741\\u670813\\u65E5"),
    CharsToUnicodeString("1999\\u5E741\\u670813\\u65E5"),
    UnicodeString("1-13"),
    UnicodeString("1 13"),
    CharsToUnicodeString("1999 Q1"),
    CharsToUnicodeString("\\u4E0B\\u534811:58"),
    CharsToUnicodeString("23:58"),
    UnicodeString("58:59"),
    UnicodeString("1.1999"),  // de_DE
    UnicodeString("Jan 1999"),
    UnicodeString("13.1.1999"),
    UnicodeString("13. Jan 1999"),
    UnicodeString("13.1."),
    UnicodeString("13. Jan"),
    UnicodeString("Q1 1999"),
    UnicodeString("23:58"),
    UnicodeString("23:58"),
    UnicodeString("58:59"),
    UnicodeString("1/1999"),  // fi
    UnicodeString("tammi 1999"),
    UnicodeString("13.1.1999"),
    UnicodeString("13. tammita 1999"),
    UnicodeString("13.1."),
    UnicodeString("13. tammita"),
    UnicodeString("1. nelj./1999"),
    UnicodeString("23.58"),
    UnicodeString("23.58"),
    UnicodeString("58.59"),
    UnicodeString(""),
};

// results for getSkeletons() and getPatternForSkeleton()
static const UnicodeString testSkeletonsResults[] = { 
    UnicodeString("HH:mm"), 
    UnicodeString("MMMMd"), 
    UnicodeString("MMMMMd"), 
};
      
static const UnicodeString testBaseSkeletonsResults[] = {        
    UnicodeString("Hm"),  
    UnicodeString("MMMd"), 
    UnicodeString("MMMd"),
};

static const UnicodeString newDecimal=UnicodeString(" "); // space
static const UnicodeString newAppendItemName = UnicodeString("hrs.");
static const UnicodeString newAppendItemFormat = UnicodeString("{1} {0}");
static const UnicodeString newDateTimeFormat = UnicodeString("{1} {0}");

// This is an API test, not a unit test.  It doesn't test very many cases, and doesn't
// try to test the full functionality.  It just calls each function in the class and
// verifies that it works on a basic level.

void IntlTestDateTimePatternGeneratorAPI::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite DateTimePatternGeneratorAPI");
    switch (index) {
        TESTCASE(0, testAPI);
        default: name = ""; break;
    }
}

/**
 * Test various generic API methods of DateTimePatternGenerator for API coverage.
 */
void IntlTestDateTimePatternGeneratorAPI::testAPI(/*char *par*/)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString conflictingPattern;
    UDateTimePatternConflict conflictingStatus;

    // ======= Test CreateInstance with default locale
    logln("Testing DateTimePatternGenerator createInstance from default locale");
    
    DateTimePatternGenerator *instFromDefaultLocale=DateTimePatternGenerator::createInstance(status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (default) - exitting");
        return;
    }
    else {
        delete instFromDefaultLocale;
    }

    // ======= Test CreateInstance with given locale    
    logln("Testing DateTimePatternGenerator createInstance from French locale");
    status = U_ZERO_ERROR;
    DateTimePatternGenerator *instFromLocale=DateTimePatternGenerator::createInstance(Locale::getFrench(), status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (Locale::getFrench()) - exitting");
        return;
    }

    // ======= Test clone DateTimePatternGenerator    
    logln("Testing DateTimePatternGenerator::clone()");
    status = U_ZERO_ERROR;
    

    UnicodeString decimalSymbol = instFromLocale->getDecimal();
    UnicodeString newDecimalSymbol = UnicodeString("*");
    decimalSymbol = instFromLocale->getDecimal();
    instFromLocale->setDecimal(newDecimalSymbol);
    DateTimePatternGenerator *cloneDTPatternGen=instFromLocale->clone();
    decimalSymbol = cloneDTPatternGen->getDecimal();
    if (decimalSymbol != newDecimalSymbol) {
        errln("ERROR: inconsistency is found in cloned object.");
    }
    delete instFromLocale;
    delete cloneDTPatternGen;
    
    // ======= Test simple use cases    
    logln("Testing simple use cases");
    status = U_ZERO_ERROR;
    Locale deLocale=Locale::getGermany();
    UDate sampleDate=LocaleTest::date(99, 9, 13, 23, 58, 59);
    DateTimePatternGenerator *gen = DateTimePatternGenerator::createInstance(deLocale, status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (Locale::getGermany()) - exitting");
        return;
    }
    UnicodeString findPattern = gen->getBestPattern(UnicodeString("MMMddHmm"), status);
    SimpleDateFormat *format = new SimpleDateFormat(findPattern, deLocale, status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create SimpleDateFormat (Locale::getGermany())");
    }
    TimeZone *zone = TimeZone::createTimeZone(UnicodeString("ECT"));
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create TimeZone ECT");
    }
    format->setTimeZone(*zone);
    UnicodeString dateReturned, expectedResult;
    dateReturned="";
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=UnicodeString("8:58 14. Okt");
    if ( dateReturned != expectedResult ) {
        errln("ERROR: Simple test in getBestPattern with Locale::getGermany()).");
    }
    // add new pattern
    status = U_ZERO_ERROR;
    conflictingStatus = gen->addPattern(UnicodeString("d'. von' MMMM"), true, conflictingPattern, status); 
    if (U_FAILURE(status)) {
        errln("ERROR: Could not addPattern - d\'. von\' MMMM");
    }
    status = U_ZERO_ERROR;
    UnicodeString testPattern=gen->getBestPattern(UnicodeString("MMMMdd"), status);
    testPattern=gen->getBestPattern(UnicodeString("MMMddHmm"), status);
    format->applyPattern(gen->getBestPattern(UnicodeString("MMMMddHmm"), status));
    dateReturned="";
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=UnicodeString("8:58 14. von Oktober");
    if ( dateReturned != expectedResult ) {
        errln("ERROR: Simple test addPattern failed!: d\'. von\' MMMM  ");
    }
    delete format;
    
    // get a pattern and modify it
    format = (SimpleDateFormat *)DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, 
                                                                  deLocale);
    format->setTimeZone(*zone);
    UnicodeString pattern;
    pattern = format->toPattern(pattern);
    dateReturned="";
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=UnicodeString("Donnerstag, 14. Oktober 1999 08:58:59 Frankreich");
    if ( dateReturned != expectedResult ) {
        errln("ERROR: Simple test uses full date format.");
        errln(UnicodeString(" Got: ") + dateReturned + UnicodeString(" Expected: ") + expectedResult);
    }
     
    // modify it to change the zone.  
    UnicodeString newPattern = gen->replaceFieldTypes(pattern, UnicodeString("vvvv"), status);
    format->applyPattern(newPattern);
    dateReturned="";
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=UnicodeString("Donnerstag, 14. Oktober 1999 08:58:59 Frankreich");
    if ( dateReturned != expectedResult ) {
        errln("ERROR: Simple test modify the timezone!");
        errln(UnicodeString(" Got: ")+ dateReturned + UnicodeString(" Expected: ") + expectedResult);
    }
    
    // setDeciaml(), getDeciaml()
    gen->setDecimal(newDecimal);
    if (newDecimal != gen->getDecimal()) {
        errln("ERROR: unexpected result from setDecimal() and getDecimal()!.\n");
    }
    
    // setAppenItemName() , getAppendItemName()
    gen->setAppendItemName(UDATPG_HOUR_FIELD, newAppendItemName);
    if (newAppendItemName != gen->getAppendItemName(UDATPG_HOUR_FIELD)) {
        errln("ERROR: unexpected result from setAppendItemName() and getAppendItemName()!.\n");
    }
    
    // setAppenItemFormat() , getAppendItemFormat()
    gen->setAppendItemFormat(UDATPG_HOUR_FIELD, newAppendItemFormat);
    if (newAppendItemFormat != gen->getAppendItemFormat(UDATPG_HOUR_FIELD)) {
        errln("ERROR: unexpected result from setAppendItemFormat() and getAppendItemFormat()!.\n");
    }
    
    // setDateTimeFormat() , getDateTimeFormat()
    gen->setDateTimeFormat(newDateTimeFormat);
    if (newDateTimeFormat != gen->getDateTimeFormat()) {
        errln("ERROR: unexpected result from setDateTimeFormat() and getDateTimeFormat()!.\n");
    }

    delete format;
    delete zone;
    delete gen;
    
    // ======= Test various skeletons.
    logln("Testing DateTimePatternGenerator with various skeleton");
   
    status = U_ZERO_ERROR;
    int32_t localeIndex=0;
    int32_t resultIndex=0;
    UnicodeString resultDate;
    UDate testDate= LocaleTest::date(99, 0, 13, 23, 58, 59);
    while (localeIndex < MAX_LOCALE )
    {       
        int32_t dataIndex=0;
        UnicodeString bestPattern;
        
        Locale loc(testLocale[localeIndex][0], testLocale[localeIndex][1], testLocale[localeIndex][2], "");
        //printf("\n\n Locale: %s_%s_%s", testLocale[localeIndex][0], testLocale[localeIndex][1], testLocale[localeIndex][2]);
        DateTimePatternGenerator *patGen=DateTimePatternGenerator::createInstance(loc, status);
        if(U_FAILURE(status)) {
            dataerrln("ERROR: Could not create DateTimePatternGenerator with locale index:%d . - exitting\n", localeIndex);
            return;
        }
        while (patternData[dataIndex].length() > 0) {
            bestPattern = patGen->getBestPattern(patternData[dataIndex++], status);
            
            SimpleDateFormat* sdf = new SimpleDateFormat(bestPattern, loc, status);
            resultDate = "";
            resultDate = sdf->format(testDate, resultDate);
            if ( resultDate != patternResults[resultIndex] ) {
                errln(UnicodeString("\nERROR: Test various skeletons[") + (dataIndex-1)
                    + UnicodeString("]. Got: ") + resultDate + UnicodeString(" Expected: ") + patternResults[resultIndex] );
            }
            
            resultIndex++;
            delete sdf;
        }
        delete patGen;
        localeIndex++;
    }



    // ======= Test random skeleton 
    /*const char randomChars[80] = {
     '1','2','3','4','5','6','7','8','9','0','!','@','#','$','%','^','&','*','(',')',
     '`',' ','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r',
     's','t','u','v','w','x','y','z','A','B','C','D','F','G','H','I','J','K','L','M',
     'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',':',';','<','.','?',';','\\'};*/
    DateTimePatternGenerator *randDTGen= DateTimePatternGenerator::createInstance(status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (Locale::getFrench()) - exitting");
        return;
    }

    for (int32_t i=0; i<10; ++i) {
        UnicodeString randomSkeleton="";
        int32_t len = rand() % 20;
        for (int32_t j=0; j<len; ++j ) {
           randomSkeleton += (UChar)(rand()%80);
        }
        UnicodeString bestPattern = randDTGen->getBestPattern(randomSkeleton, status);
    }
    delete randDTGen;
    
    // UnicodeString randomString=Unicode
    // ======= Test getStaticClassID()

    logln("Testing getStaticClassID()");
    status = U_ZERO_ERROR;
    DateTimePatternGenerator *test= DateTimePatternGenerator::createInstance(status);
    
    if(test->getDynamicClassID() != DateTimePatternGenerator::getStaticClassID()) {
        errln("ERROR: getDynamicClassID() didn't return the expected value");
    }
    delete test;
    
    // ====== Test createEmptyInstance()
    
    logln("Testing createEmptyInstance()");
    status = U_ZERO_ERROR;
    
    test = DateTimePatternGenerator::createEmptyInstance(status);
    if(U_FAILURE(status)) {
         errln("ERROR: Fail to create an empty instance ! - exitting.\n");
         delete test;
         return;
    }
    
    conflictingStatus = test->addPattern(UnicodeString("MMMMd"), true, conflictingPattern, status); 
    status = U_ZERO_ERROR;
    testPattern=test->getBestPattern(UnicodeString("MMMMdd"), status);
    conflictingStatus = test->addPattern(UnicodeString("HH:mm"), true, conflictingPattern, status); 
    conflictingStatus = test->addPattern(UnicodeString("MMMMMd"), true, conflictingPattern, status); //duplicate pattern
    StringEnumeration *output=NULL;
    output = test->getRedundants(status);
    expectedResult=UnicodeString("MMMMd");
    if (output != NULL) {
        output->reset(status);
        const UnicodeString *dupPattern=output->snext(status);
        if ( (dupPattern==NULL) || (*dupPattern != expectedResult) ) {
            errln("ERROR: Fail in getRedundants !\n");
        }
    }
    
    // ======== Test getSkeletons and getBaseSkeletons
     StringEnumeration* ptrSkeletonEnum = test->getSkeletons(status);
     if(U_FAILURE(status)) {
          errln("ERROR: Fail to get skeletons !\n");
     }
     UnicodeString returnPattern, *ptrSkeleton;
     ptrSkeletonEnum->reset(status);
     int32_t count=ptrSkeletonEnum->count(status);
     for (int32_t i=0; i<count; ++i) {
         ptrSkeleton = (UnicodeString *)ptrSkeletonEnum->snext(status);
         returnPattern = test->getPatternForSkeleton(*ptrSkeleton);
         if ( returnPattern != testSkeletonsResults[i] ) {
             errln("ERROR: Unexpected result from getSkeletons() and getPatternForSkeleton() !\n");
         }
     }
     StringEnumeration* ptrBaseSkeletonEnum = test->getBaseSkeletons(status);
     if(U_FAILURE(status)) {
          errln("ERROR: Fail to get base skeletons !\n");
      }   
     count=ptrBaseSkeletonEnum->count(status);
     for (int32_t i=0; i<count; ++i) {
         ptrSkeleton = (UnicodeString *)ptrBaseSkeletonEnum->snext(status);
         if ( *ptrSkeleton != testBaseSkeletonsResults[i] ) {
             errln("ERROR: Unexpected result from getBaseSkeletons() !\n");
         }
     }
     
     delete output;
     delete ptrSkeletonEnum;
     delete ptrBaseSkeletonEnum;
     delete test;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
