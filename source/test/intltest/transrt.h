/*
**********************************************************************
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/23/00    aliu        Creation.
**********************************************************************
*/
#ifndef TRANSRT_H
#define TRANSRT_H

#include "unicode/translit.h"
#include "intltest.h"

/**
 * @test
 * @summary Round trip test of Transliterator
 */
class TransliteratorRoundTripTest : public IntlTest {

    void runIndexedTest(int32_t index, UBool exec, const char* &name,
                        char* par=NULL);

    void TestHiragana(void);
    void TestKatakana(void);
    void TestArabic(void);
    void TestHebrew(void);
    void TestGreek(void);
    void Testel(void);
    void TestCyrillic(void);
    void TestDevanagari(void);
    void TestDevanagariTamil(void);
    void TestJamo(void);
    void TestJamoHangul(void);
};

#endif
