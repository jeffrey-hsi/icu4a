/*
*******************************************************************************
* Copyright (C) 2014, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File MEASFMTTEST.CPP
*
*******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>

#include "intltest.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/measfmt.h"
#include "unicode/measunit.h"

#define LENGTHOF(array) (int32_t)(sizeof(array) / sizeof((array)[0]))

class MeasureFormatTest : public IntlTest {
public:
    MeasureFormatTest() {
    }

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=0);
private:
    void TestBasic();
    void TestGetAvailable();
};

void MeasureFormatTest::runIndexedTest(
        int32_t index, UBool exec, const char *&name, char *) {
    if (exec) {
        logln("TestSuite MeasureFormatTest: ");
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(TestBasic);
    TESTCASE_AUTO(TestGetAvailable);
    TESTCASE_AUTO_END;
}

void MeasureFormatTest::TestBasic() {
    UErrorCode status = U_ZERO_ERROR;
    MeasureUnit *ptr1 = MeasureUnit::createArcminute(status);
    MeasureUnit *ptr2 = MeasureUnit::createArcminute(status);
    if (!(*ptr1 == *ptr2)) {
        errln("Expect == to work.");
    }
    if (*ptr1 != *ptr2) {
        errln("Expect != to work.");
    }
    MeasureUnit *ptr3 = MeasureUnit::createMeter(status);
    if (*ptr1 == *ptr3) {
        errln("Expect == to work.");
    }
    if (!(*ptr1 != *ptr3)) {
        errln("Expect != to work.");
    }
    MeasureUnit *ptr4 = (MeasureUnit *) ptr1->clone();
    if (*ptr1 != *ptr4) {
        errln("Expect clone to work.");
    }
    MeasureUnit stack;
    stack = *ptr1;
    if (*ptr1 != stack) {
        errln("Expect assignment to work.");
    }

    delete ptr1;
    delete ptr2;
    delete ptr3;
    delete ptr4;
}

void MeasureFormatTest::TestGetAvailable() {
    MeasureUnit *units = NULL;
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = MeasureUnit::getAvailable(0, units, status);
    while (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;
        delete [] units;
        units = new MeasureUnit[len];
        len = MeasureUnit::getAvailable(len, units, status);
    }
    if (U_FAILURE(status)) {
        dataerrln("Failure creating format object - %s", u_errorName(status));
        delete [] units;
        return;
    }
    for (int i = 0; i < len; ++i) {
      errln("Type: %s Subtype: %s", units[i].getType(), units[i].getSubtype());
    }
    delete [] units;
    const char **types = NULL;
    int32_t typeCount = MeasureUnit::getAvailableTypes(0, types, status);
    while (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;
        delete [] types;
        types = new const char *[typeCount];
        typeCount = MeasureUnit::getAvailableTypes(typeCount, types, status);
    }
    if (U_FAILURE(status)) {
        dataerrln("Failure getting types - %s", u_errorName(status));
        delete [] types;
        return;
    }
    units = NULL;
    int32_t unitCapacity = 0;
    for (int i = 0; i < typeCount; ++i) {
        errln("Type: %s", types[i]);
        int32_t unitCount = MeasureUnit::getAvailable(types[i], unitCapacity, units, status);
        while (status == U_BUFFER_OVERFLOW_ERROR) {
            status = U_ZERO_ERROR;
            delete [] units;
            units = new MeasureUnit[unitCount];
            unitCapacity = unitCount;
            unitCount = MeasureUnit::getAvailable(types[i], unitCapacity, units, status);
        }
        if (U_FAILURE(status)) {
            dataerrln("Failure getting units - %s", u_errorName(status));
            delete [] units;
            delete[] types;
            return;
        }
        for (int j = 0; j < unitCount; ++j) {
           errln("Type: %s Subtype: %s index: %d", units[j].getType(), units[j].getSubtype(), units[j].getIndex());
        }
    }
    delete [] units;
    delete[] types;
    MeasureUnit *ptr = MeasureUnit::createArcminute(status);
    errln("Type: %s Subtype: %s", ptr->getType(), ptr->getSubtype());
    delete ptr;
}

extern IntlTest *createMeasureFormatTest() {
    return new MeasureFormatTest();
}

#endif

