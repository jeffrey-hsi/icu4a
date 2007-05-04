/*
 **********************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */


#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/putil.h"
#include "unicode/usearch.h"

#include "intltest.h"
#include "ssearch.h"

#include "xmlparser.h"

#include <stdlib.h>
#include <string.h>

char testId[100];

#define TEST_ASSERT(x) {if (!(x)) { \
    errln("Failure in file %s, line %d, test ID \"%s\"", __FILE__, __LINE__, testId);}}

#define TEST_ASSERT_SUCCESS(errcode) {if (U_FAILURE(errcode)) { \
    errln("Failure in file %s, line %d, test ID \"%s\", status = \"%s\"", \
          __FILE__, __LINE__, testId, u_errorName(errcode));}}


//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
SSearchTest::SSearchTest()
{
}


SSearchTest::~SSearchTest()
{
}



void SSearchTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CharsetDetectionTest: ");
    switch (index) {
       case 0: name = "searchTest";
            if (exec) searchTest();
            break;

        default: name = "";
            break; //needed to end loop
    }
}



#define PATH_BUFFER_SIZE 2048
const char *SSearchTest::getPath(char buffer[2048], const char *filename) {
    UErrorCode status = U_ZERO_ERROR;
    const char *testDataDirectory = IntlTest::getSourceTestData(status);

    if (U_FAILURE(status) || strlen(testDataDirectory) + strlen(filename) + 1 >= PATH_BUFFER_SIZE) {
        errln("ERROR: getPath() failed - %s", u_errorName(status));
        return NULL;
    }

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);
    return buffer;
}


void SSearchTest::searchTest()
{
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    UErrorCode status = U_ZERO_ERROR;
    char path[PATH_BUFFER_SIZE];
    const char *testFilePath = getPath(path, "ssearch.xml");

    if (testFilePath == NULL) {
        return; /* Couldn't get path: error message already output. */
    }

    UXMLParser  *parser = UXMLParser::createParser(status);
    TEST_ASSERT_SUCCESS(status);
    UXMLElement *root   = parser->parseFile(testFilePath, status);
    TEST_ASSERT_SUCCESS(status);
    if (U_FAILURE(status)) {
        return;
    }

    const UXMLElement *testCase;
    int32_t tc = 0;

    while((testCase = root->nextChildElement(tc)) != NULL) {

        if (testCase->getTagName().compare("test-case") != 0) {
            errln("ssearch, unrecognized XML Element in test file");
            continue;
        }
        const UnicodeString *id       = testCase->getAttribute("id");
        *testId = 0;
        if (id != NULL) {
            id->extract(0, id->length(), testId,  sizeof(testId), US_INV);
        }

        const UnicodeString *strength = testCase->getAttribute("strength");

        const UnicodeString defLocale("en");
        char  clocale[100];
        const UnicodeString *locale   = testCase->getAttribute("locale");
        if (locale == NULL || locale->length()==0) {
            locale = &defLocale;
        };
        locale->extract(0, locale->length(), clocale, sizeof(clocale), US_INV);


        UnicodeString  text;
        UnicodeString  target;
        UnicodeString  pattern;
        int32_t        expectedMatchStart = -1;
        int32_t        expectedMatchLimit = -1;
        const UXMLElement  *n;
        int                nodeCount = 0;

        n = testCase->getChildElement("pattern");
        TEST_ASSERT(n != NULL);
        if (n==NULL) {
            continue;
        }
        text = n->getText(FALSE);
        text = text.unescape();
        pattern.append(text);
        nodeCount++;

        n = testCase->getChildElement("pre");
        if (n!=NULL) {
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            nodeCount++;
        }
        
        n = testCase->getChildElement("m");
        if (n!=NULL) {
            expectedMatchStart = target.length();
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            expectedMatchLimit = target.length();
            nodeCount++;
        }

        n = testCase->getChildElement("post");
        if (n!=NULL) {
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            nodeCount++;
        }

        //  Check that there weren't extra things in the XML
        TEST_ASSERT(nodeCount == testCase->countChildren());

        status = U_ZERO_ERROR;
        UStringSearch *uss = usearch_open(pattern.getBuffer(), pattern.length(),
                                         target.getBuffer(), target.length(),
                                         clocale,
                                         NULL,     // the break iterator
                                         &status);
        TEST_ASSERT_SUCCESS(status);
        if (U_FAILURE(status)) {
            continue;
        }

        // TODO:  get strength in here somehow.

        int32_t foundStart = 0;
        int32_t foundLimit = 0;
        UBool   foundMatch;

        foundMatch= usearch_search(uss, 0, &foundStart, &foundLimit, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(foundStart == expectedMatchStart);
        TEST_ASSERT(foundLimit == expectedMatchLimit);
        TEST_ASSERT (foundMatch == (expectedMatchStart >= 0));

        usearch_close(uss);
    }

    delete root;
    delete parser;
#endif
}


