/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/normlzr.h"
#include "unicode/schriter.h"
#include "cstring.h"
#include "tstnorm.h"

#define ARRAY_LENGTH(array) ((int32_t)(sizeof (array) / sizeof (*array)))

#define CASE(id,test) case id:                          \
                          name = #test;                 \
                          if (exec) {                   \
                              logln(#test "---");       \
                              logln((UnicodeString)""); \
                              test();                   \
                          }                             \
                          break

static UErrorCode status = U_ZERO_ERROR;

void BasicNormalizerTest::runIndexedTest(int32_t index, UBool exec,
                                         const char* &name, char* /*par*/) {
    switch (index) {
        CASE(0,TestDecomp);
        CASE(1,TestCompatDecomp);
        CASE(2,TestCanonCompose);
        CASE(3,TestCompatCompose);
        CASE(4,TestPrevious);
        CASE(5,TestHangulDecomp);
        CASE(6,TestHangulCompose);
        CASE(7,TestTibetan);
        CASE(8,TestCompositionExclusion);
        CASE(9,TestZeroIndex);
        CASE(10,TestVerisign);
        CASE(11,TestPreviousNext);
        CASE(12,TestNormalizerAPI);
        CASE(13,TestConcatenate);
        CASE(14,FindFoldFCDExceptions);
        CASE(15,TestCompare);
        default: name = ""; break;
    }
}

/**
 * Convert Java-style strings with \u Unicode escapes into UnicodeString objects
 */
static UnicodeString str(const char *input)
{
    UnicodeString str(input, ""); // Invariant conversion
    return str.unescape();
}


BasicNormalizerTest::BasicNormalizerTest()
{
  // canonTest
  // Input                    Decomposed                    Composed

    canonTests[0][0] = str("cat");  canonTests[0][1] = str("cat"); canonTests[0][2] =  str("cat");

    canonTests[1][0] = str("\\u00e0ardvark");    canonTests[1][1] = str("a\\u0300ardvark");  canonTests[1][2] = str("\\u00e0ardvark"); 

    canonTests[2][0] = str("\\u1e0a"); canonTests[2][1] = str("D\\u0307"); canonTests[2][2] = str("\\u1e0a");                 // D-dot_above

    canonTests[3][0] = str("D\\u0307");  canonTests[3][1] = str("D\\u0307"); canonTests[3][2] = str("\\u1e0a");            // D dot_above

    canonTests[4][0] = str("\\u1e0c\\u0307"); canonTests[4][1] = str("D\\u0323\\u0307");  canonTests[4][2] = str("\\u1e0c\\u0307");         // D-dot_below dot_above

    canonTests[5][0] = str("\\u1e0a\\u0323"); canonTests[5][1] = str("D\\u0323\\u0307");  canonTests[5][2] = str("\\u1e0c\\u0307");        // D-dot_above dot_below 

    canonTests[6][0] = str("D\\u0307\\u0323"); canonTests[6][1] = str("D\\u0323\\u0307");  canonTests[6][2] = str("\\u1e0c\\u0307");         // D dot_below dot_above 

    canonTests[7][0] = str("\\u1e10\\u0307\\u0323");  canonTests[7][1] = str("D\\u0327\\u0323\\u0307"); canonTests[7][2] = str("\\u1e10\\u0323\\u0307");     // D dot_below cedilla dot_above

    canonTests[8][0] = str("D\\u0307\\u0328\\u0323"); canonTests[8][1] = str("D\\u0328\\u0323\\u0307"); canonTests[8][2] = str("\\u1e0c\\u0328\\u0307");     // D dot_above ogonek dot_below

    canonTests[9][0] = str("\\u1E14"); canonTests[9][1] = str("E\\u0304\\u0300"); canonTests[9][2] = str("\\u1E14");         // E-macron-grave

    canonTests[10][0] = str("\\u0112\\u0300"); canonTests[10][1] = str("E\\u0304\\u0300");  canonTests[10][2] = str("\\u1E14");            // E-macron + grave

    canonTests[11][0] = str("\\u00c8\\u0304"); canonTests[11][1] = str("E\\u0300\\u0304");  canonTests[11][2] = str("\\u00c8\\u0304");         // E-grave + macron
  
    canonTests[12][0] = str("\\u212b"); canonTests[12][1] = str("A\\u030a"); canonTests[12][2] = str("\\u00c5");             // angstrom_sign

    canonTests[13][0] = str("\\u00c5");      canonTests[13][1] = str("A\\u030a");  canonTests[13][2] = str("\\u00c5");            // A-ring
  
    canonTests[14][0] = str("\\u00C4ffin");  canonTests[14][1] = str("A\\u0308ffin");  canonTests[14][2] = str("\\u00C4ffin");

    canonTests[15][0] = str("\\u00C4\\uFB03n"); canonTests[15][1] = str("A\\u0308\\uFB03n"); canonTests[15][2] = str("\\u00C4\\uFB03n");
  
    canonTests[16][0] = str("Henry IV"); canonTests[16][1] = str("Henry IV"); canonTests[16][2] = str("Henry IV");

    canonTests[17][0] = str("Henry \\u2163");  canonTests[17][1] = str("Henry \\u2163");  canonTests[17][2] = str("Henry \\u2163");
  
    canonTests[18][0] = str("\\u30AC");  canonTests[18][1] = str("\\u30AB\\u3099");  canonTests[18][2] = str("\\u30AC");              // ga (Katakana)

    canonTests[19][0] = str("\\u30AB\\u3099"); canonTests[19][1] = str("\\u30AB\\u3099");  canonTests[19][2] = str("\\u30AC");            // ka + ten

    canonTests[20][0] = str("\\uFF76\\uFF9E"); canonTests[20][1] = str("\\uFF76\\uFF9E");  canonTests[20][2] = str("\\uFF76\\uFF9E");       // hw_ka + hw_ten

    canonTests[21][0] = str("\\u30AB\\uFF9E"); canonTests[21][1] = str("\\u30AB\\uFF9E");  canonTests[21][2] = str("\\u30AB\\uFF9E");         // ka + hw_ten

    canonTests[22][0] = str("\\uFF76\\u3099"); canonTests[22][1] = str("\\uFF76\\u3099");  canonTests[22][2] = str("\\uFF76\\u3099");         // hw_ka + ten

    canonTests[23][0] = str("A\\u0300\\u0316"); canonTests[23][1] = str("A\\u0316\\u0300");  canonTests[23][2] = str("\\u00C0\\u0316");     

    /* compatTest */
  // Input                        Decomposed                        Composed
  compatTests[0][0] = str("cat"); compatTests[0][1] = str("cat"); compatTests[0][2] = str("cat") ;
  
  compatTests[1][0] = str("\\uFB4f");  compatTests[1][1] = str("\\u05D0\\u05DC"); compatTests[1][2] = str("\\u05D0\\u05DC");  // Alef-Lamed vs. Alef, Lamed
  
  compatTests[2][0] = str("\\u00C4ffin"); compatTests[2][1] = str("A\\u0308ffin"); compatTests[2][2] = str("\\u00C4ffin") ;

  compatTests[3][0] = str("\\u00C4\\uFB03n"); compatTests[3][1] = str("A\\u0308ffin"); compatTests[3][2] = str("\\u00C4ffin") ; // ffi ligature -> f + f + i
  
  compatTests[4][0] = str("Henry IV"); compatTests[4][1] = str("Henry IV"); compatTests[4][2] = str("Henry IV") ;

  compatTests[5][0] = str("Henry \\u2163"); compatTests[5][1] = str("Henry IV");  compatTests[5][2] = str("Henry IV") ;
  
  compatTests[6][0] = str("\\u30AC"); compatTests[6][1] = str("\\u30AB\\u3099"); compatTests[6][2] = str("\\u30AC") ; // ga (Katakana)

  compatTests[7][0] = str("\\u30AB\\u3099"); compatTests[7][1] = str("\\u30AB\\u3099"); compatTests[7][2] = str("\\u30AC") ; // ka + ten
  
  compatTests[8][0] = str("\\uFF76\\u3099"); compatTests[8][1] = str("\\u30AB\\u3099"); compatTests[8][2] = str("\\u30AC") ; // hw_ka + ten
  
  /* These two are broken in Unicode 2.1.2 but fixed in 2.1.5 and later */
  compatTests[9][0] = str("\\uFF76\\uFF9E"); compatTests[9][1] = str("\\u30AB\\u3099"); compatTests[9][2] = str("\\u30AC") ; // hw_ka + hw_ten

  compatTests[10][0] = str("\\u30AB\\uFF9E"); compatTests[10][1] = str("\\u30AB\\u3099"); compatTests[10][2] = str("\\u30AC") ; // ka + hw_ten

  /* Hangul Canonical */
  // Input                        Decomposed                        Composed
  hangulCanon[0][0] = str("\\ud4db"); hangulCanon[0][1] = str("\\u1111\\u1171\\u11b6"); hangulCanon[0][2] = str("\\ud4db") ;

  hangulCanon[1][0] = str("\\u1111\\u1171\\u11b6"), hangulCanon[1][1] = str("\\u1111\\u1171\\u11b6"),   hangulCanon[1][2] = str("\\ud4db");
}

BasicNormalizerTest::~BasicNormalizerTest()
{
}

void BasicNormalizerTest::TestPrevious() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  
  logln("testing decomp...");
  uint32_t i;
  for (i = 0; i < ARRAY_LENGTH(canonTests); i++) {
    backAndForth(norm, canonTests[i][0]);
  }
  
  logln("testing compose...");
  norm->setMode(UNORM_NFC);
  for (i = 0; i < ARRAY_LENGTH(canonTests); i++) {
    backAndForth(norm, canonTests[i][0]);
  }

  delete norm;
}

void BasicNormalizerTest::TestDecomp() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  iterateTest(norm, canonTests, ARRAY_LENGTH(canonTests), 1);
  staticTest(UNORM_NFD, 0, canonTests, ARRAY_LENGTH(canonTests), 1);
  delete norm;
}

void BasicNormalizerTest::TestCompatDecomp() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFKD);
  iterateTest(norm, compatTests, ARRAY_LENGTH(compatTests), 1);
  
  staticTest(UNORM_NFKD, 0, 
         compatTests, ARRAY_LENGTH(compatTests), 1);
  delete norm;
}

void BasicNormalizerTest::TestCanonCompose() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFC);
  iterateTest(norm, canonTests, ARRAY_LENGTH(canonTests), 2);
  
  staticTest(UNORM_NFC, 0, canonTests,
         ARRAY_LENGTH(canonTests), 2);
  delete norm;
}

void BasicNormalizerTest::TestCompatCompose() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFKC);
  iterateTest(norm, compatTests, ARRAY_LENGTH(compatTests), 2);
  
  staticTest(UNORM_NFKC, 0, 
         compatTests, ARRAY_LENGTH(compatTests), 2);
  delete norm;
}


//-------------------------------------------------------------------------------

void BasicNormalizerTest::TestHangulCompose() 
{
  // Make sure that the static composition methods work
  logln("Canonical composition...");
  staticTest(UNORM_NFC, 0,                    hangulCanon,  ARRAY_LENGTH(hangulCanon),  2);
  logln("Compatibility composition...");
  
  // Now try iterative composition....
  logln("Static composition...");
  Normalizer* norm = new Normalizer("", UNORM_NFC);
  iterateTest(norm, hangulCanon, ARRAY_LENGTH(hangulCanon), 2);
  norm->setMode(UNORM_NFKC);
  
  // And finally, make sure you can do it in reverse too
  logln("Reverse iteration...");
  norm->setMode(UNORM_NFC);
  for (uint32_t i = 0; i < ARRAY_LENGTH(hangulCanon); i++) {
    backAndForth(norm, hangulCanon[i][0]);
  }
  delete norm;
}

void BasicNormalizerTest::TestHangulDecomp() 
{
  // Make sure that the static decomposition methods work
  logln("Canonical decomposition...");
  staticTest(UNORM_NFD, 0,                     hangulCanon,  ARRAY_LENGTH(hangulCanon),  1);
  logln("Compatibility decomposition...");
  
  // Now the iterative decomposition methods...
  logln("Iterative decomposition...");
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  iterateTest(norm, hangulCanon, ARRAY_LENGTH(hangulCanon), 1);
  norm->setMode(UNORM_NFKD);
  
  // And finally, make sure you can do it in reverse too
  logln("Reverse iteration...");
  norm->setMode(UNORM_NFD);
  for (uint32_t i = 0; i < ARRAY_LENGTH(hangulCanon); i++) {
    backAndForth(norm, hangulCanon[i][0]);
  }
  delete norm;
}

/**
 * The Tibetan vowel sign AA, 0f71, was messed up prior to Unicode version 2.1.9.
 */
void BasicNormalizerTest::TestTibetan(void) {
    UnicodeString decomp[1][3];
    decomp[0][0] = str("\\u0f77");
    decomp[0][1] = str("\\u0f77");
    decomp[0][2] = str("\\u0fb2\\u0f71\\u0f80");

    UnicodeString compose[1][3];
    compose[0][0] = str("\\u0fb2\\u0f71\\u0f80");
    compose[0][1] = str("\\u0fb2\\u0f71\\u0f80");
    compose[0][2] = str("\\u0fb2\\u0f71\\u0f80");

    staticTest(UNORM_NFD,         0, decomp, ARRAY_LENGTH(decomp), 1);
    staticTest(UNORM_NFKD,  0, decomp, ARRAY_LENGTH(decomp), 2);
    staticTest(UNORM_NFC,        0, compose, ARRAY_LENGTH(compose), 1);
    staticTest(UNORM_NFKC, 0, compose, ARRAY_LENGTH(compose), 2);
}

/**
 * Make sure characters in the CompositionExclusion.txt list do not get
 * composed to.
 */
void BasicNormalizerTest::TestCompositionExclusion(void) {
    // This list is generated from CompositionExclusion.txt.
    // Update whenever the normalizer tables are updated.  Note
    // that we test all characters listed, even those that can be
    // derived from the Unicode DB and are therefore commented
    // out.
    // ### TODO read composition exclusion from source/data/unidata file
    // and test against that
    UnicodeString EXCLUDED = str(
        "\\u0340\\u0341\\u0343\\u0344\\u0374\\u037E\\u0387\\u0958"
        "\\u0959\\u095A\\u095B\\u095C\\u095D\\u095E\\u095F\\u09DC"
        "\\u09DD\\u09DF\\u0A33\\u0A36\\u0A59\\u0A5A\\u0A5B\\u0A5E"
        "\\u0B5C\\u0B5D\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69"
        "\\u0F73\\u0F75\\u0F76\\u0F78\\u0F81\\u0F93\\u0F9D\\u0FA2"
        "\\u0FA7\\u0FAC\\u0FB9\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79"
        "\\u1F7B\\u1F7D\\u1FBB\\u1FBE\\u1FC9\\u1FCB\\u1FD3\\u1FDB"
        "\\u1FE3\\u1FEB\\u1FEE\\u1FEF\\u1FF9\\u1FFB\\u1FFD\\u2000"
        "\\u2001\\u2126\\u212A\\u212B\\u2329\\u232A\\uF900\\uFA10"
        "\\uFA12\\uFA15\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A\\uFB1F"
        "\\uFB2A\\uFB2B\\uFB2C\\uFB2D\\uFB2E\\uFB2F\\uFB30\\uFB31"
        "\\uFB32\\uFB33\\uFB34\\uFB35\\uFB36\\uFB38\\uFB39\\uFB3A"
        "\\uFB3B\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46"
        "\\uFB47\\uFB48\\uFB49\\uFB4A\\uFB4B\\uFB4C\\uFB4D\\uFB4E"
        );
    for (int32_t i=0; i<EXCLUDED.length(); ++i) {
        UnicodeString a(EXCLUDED.charAt(i));
        UnicodeString b;
        UnicodeString c;
        Normalizer::normalize(a, UNORM_NFKD, 0, b, status);
        Normalizer::normalize(b, UNORM_NFC, 0, c, status);
        if (c == a) {
            errln("FAIL: " + hex(a) + " x DECOMP_COMPAT => " +
                  hex(b) + " x COMPOSE => " +
                  hex(c));
        } else if (verbose) {
            logln("Ok: " + hex(a) + " x DECOMP_COMPAT => " +
                  hex(b) + " x COMPOSE => " +
                  hex(c));                
        }
    }
}

/**
 * Test for a problem that showed up just before ICU 1.6 release
 * having to do with combining characters with an index of zero.
 * Such characters do not participate in any canonical
 * decompositions.  However, having an index of zero means that
 * they all share one typeMask[] entry, that is, they all have to
 * map to the same canonical class, which is not the case, in
 * reality.
 */
void BasicNormalizerTest::TestZeroIndex(void) {
    const char* DATA[] = {
        // Expect col1 x COMPOSE_COMPAT => col2
        // Expect col2 x DECOMP => col3
        "A\\u0316\\u0300", "\\u00C0\\u0316", "A\\u0316\\u0300",
        "A\\u0300\\u0316", "\\u00C0\\u0316", "A\\u0316\\u0300",
        "A\\u0327\\u0300", "\\u00C0\\u0327", "A\\u0327\\u0300",
        "c\\u0321\\u0327", "c\\u0321\\u0327", "c\\u0321\\u0327",
        "c\\u0327\\u0321", "\\u00E7\\u0321", "c\\u0327\\u0321",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    for (int32_t i=0; i<DATA_length; i+=3) {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString a(DATA[i], "");
        a = a.unescape();
        UnicodeString b;
        Normalizer::normalize(a, UNORM_NFKC, 0, b, status);
        UnicodeString exp(DATA[i+1], "");
        exp = exp.unescape();
        if (b == exp) {
            logln((UnicodeString)"Ok: " + hex(a) + " x COMPOSE_COMPAT => " + hex(b));
        } else {
            errln((UnicodeString)"FAIL: " + hex(a) + " x COMPOSE_COMPAT => " + hex(b) +
                  ", expect " + hex(exp));
        }
        Normalizer::normalize(b, UNORM_NFD, 0, a, status);
        exp = UnicodeString(DATA[i+2], "").unescape();
        if (a == exp) {
            logln((UnicodeString)"Ok: " + hex(b) + " x DECOMP => " + hex(a));
        } else {
            errln((UnicodeString)"FAIL: " + hex(b) + " x DECOMP => " + hex(a) +
                  ", expect " + hex(exp));
        }
    }
}

/**
 * Run a few specific cases that are failing for Verisign.
 */
void BasicNormalizerTest::TestVerisign(void) {
    /*
      > Their input:
      > 05B8 05B9 05B1 0591 05C3 05B0 05AC 059F
      > Their output (supposedly from ICU):
      > 05B8 05B1 05B9 0591 05C3 05B0 05AC 059F
      > My output from charlint:
      > 05B1 05B8 05B9 0591 05C3 05B0 05AC 059F
      
      05B8 05B9 05B1 0591 05C3 05B0 05AC 059F => 05B1 05B8 05B9 0591 05C3 05B0
      05AC 059F
      
      U+05B8  18  E HEBREW POINT QAMATS
      U+05B9  19  F HEBREW POINT HOLAM
      U+05B1  11 HEBREW POINT HATAF SEGOL
      U+0591 220 HEBREW ACCENT ETNAHTA
      U+05C3   0 HEBREW PUNCTUATION SOF PASUQ
      U+05B0  10 HEBREW POINT SHEVA
      U+05AC 230 HEBREW ACCENT ILUY
      U+059F 230 HEBREW ACCENT QARNEY PARA
      
      U+05B1  11 HEBREW POINT HATAF SEGOL
      U+05B8  18 HEBREW POINT QAMATS
      U+05B9  19 HEBREW POINT HOLAM
      U+0591 220 HEBREW ACCENT ETNAHTA
      U+05C3   0 HEBREW PUNCTUATION SOF PASUQ
      U+05B0  10 HEBREW POINT SHEVA
      U+05AC 230 HEBREW ACCENT ILUY
      U+059F 230 HEBREW ACCENT QARNEY PARA
      
      Wrong result:
      U+05B8  18 HEBREW POINT QAMATS
      U+05B1  11 HEBREW POINT HATAF SEGOL
      U+05B9  19 HEBREW POINT HOLAM
      U+0591 220 HEBREW ACCENT ETNAHTA
      U+05C3   0 HEBREW PUNCTUATION SOF PASUQ
      U+05B0  10 HEBREW POINT SHEVA
      U+05AC 230 HEBREW ACCENT ILUY
      U+059F 230 HEBREW ACCENT QARNEY PARA

      
      > Their input:
      >0592 05B7 05BC 05A5 05B0 05C0 05C4 05AD
      >Their output (supposedly from ICU):
      >0592 05B0 05B7 05BC 05A5 05C0 05AD 05C4
      >My output from charlint:
      >05B0 05B7 05BC 05A5 0592 05C0 05AD 05C4
      
      0592 05B7 05BC 05A5 05B0 05C0 05C4 05AD => 05B0 05B7 05BC 05A5 0592 05C0
      05AD 05C4
      
      U+0592 230 HEBREW ACCENT SEGOL
      U+05B7  17 HEBREW POINT PATAH
      U+05BC  21 HEBREW POINT DAGESH OR MAPIQ
      U+05A5 220 HEBREW ACCENT MERKHA
      U+05B0  10 HEBREW POINT SHEVA
      U+05C0   0 HEBREW PUNCTUATION PASEQ
      U+05C4 230 HEBREW MARK UPPER DOT
      U+05AD 222 HEBREW ACCENT DEHI
      
      U+05B0  10 HEBREW POINT SHEVA
      U+05B7  17 HEBREW POINT PATAH
      U+05BC  21 HEBREW POINT DAGESH OR MAPIQ
      U+05A5 220 HEBREW ACCENT MERKHA
      U+0592 230 HEBREW ACCENT SEGOL
      U+05C0   0 HEBREW PUNCTUATION PASEQ
      U+05AD 222 HEBREW ACCENT DEHI
      U+05C4 230 HEBREW MARK UPPER DOT

      Wrong result:
      U+0592 230 HEBREW ACCENT SEGOL
      U+05B0  10 HEBREW POINT SHEVA
      U+05B7  17 HEBREW POINT PATAH
      U+05BC  21 HEBREW POINT DAGESH OR MAPIQ
      U+05A5 220 HEBREW ACCENT MERKHA
      U+05C0   0 HEBREW PUNCTUATION PASEQ
      U+05AD 222 HEBREW ACCENT DEHI
      U+05C4 230 HEBREW MARK UPPER DOT
    */
    UnicodeString data[2][3];
    data[0][0] = str("\\u05B8\\u05B9\\u05B1\\u0591\\u05C3\\u05B0\\u05AC\\u059F");
    data[0][1] = str("\\u05B1\\u05B8\\u05B9\\u0591\\u05C3\\u05B0\\u05AC\\u059F");
    data[0][2] = str("");
    data[1][0] = str("\\u0592\\u05B7\\u05BC\\u05A5\\u05B0\\u05C0\\u05C4\\u05AD");
    data[1][1] = str("\\u05B0\\u05B7\\u05BC\\u05A5\\u0592\\u05C0\\u05AD\\u05C4");
    data[1][2] = str("");

    staticTest(UNORM_NFD, 0, data, ARRAY_LENGTH(data), 1);
    staticTest(UNORM_NFC, 0, data, ARRAY_LENGTH(data), 1);
}

//------------------------------------------------------------------------
// Internal utilities
//

UnicodeString BasicNormalizerTest::hex(UChar ch) {
    UnicodeString result;
    return appendHex(ch, 4, result);
}

UnicodeString BasicNormalizerTest::hex(const UnicodeString& s) {
    UnicodeString result;
    for (int i = 0; i < s.length(); ++i) {
        if (i != 0) result += (UChar)0x2c/*,*/;
        appendHex(s[i], 4, result);
    }
    return result;
}


inline static void insert(UnicodeString& dest, int pos, UChar32 ch)
{
    dest.replace(pos, 0, ch);
}

void BasicNormalizerTest::backAndForth(Normalizer* iter, const UnicodeString& input)
{
    UChar32 ch;
    iter->setText(input, status);

    // Run through the iterator forwards and stick it into a StringBuffer
    UnicodeString forward;
    for (ch = iter->first(); ch != iter->DONE; ch = iter->next()) {
        forward += ch;
    }

    // Now do it backwards
    UnicodeString reverse;
    for (ch = iter->last(); ch != iter->DONE; ch = iter->previous()) {
        insert(reverse, 0, ch);
    }
    
    if (forward != reverse) {
        errln("Forward/reverse mismatch for input " + hex(input)
              + ", forward: " + hex(forward) + ", backward: " + hex(reverse));
    }
}

void BasicNormalizerTest::staticTest(UNormalizationMode mode, int options,
                     UnicodeString tests[][3], int length,
                     int outCol)
{
    for (int i = 0; i < length; i++)
    {
        UnicodeString& input = tests[i][0];
        UnicodeString& expect = tests[i][outCol];
        
        logln("Normalizing '" + input + "' (" + hex(input) + ")" );
        
        UnicodeString output;
        Normalizer::normalize(input, mode, options, output, status);
        
        if (output != expect) {
            errln(UnicodeString("ERROR: case ") + i + " normalized " + hex(input) + "\n"
                + "                expected " + hex(expect) + "\n"
                + "              static got " + hex(output) );
        }
    }
}

void BasicNormalizerTest::iterateTest(Normalizer* iter,
                                      UnicodeString tests[][3], int length,
                                      int outCol)
{
    for (int i = 0; i < length; i++)
    {
        UnicodeString& input = tests[i][0];
        UnicodeString& expect = tests[i][outCol];
        
        logln("Normalizing '" + input + "' (" + hex(input) + ")" );
        
        iter->setText(input, status);
        assertEqual(input, expect, iter, UnicodeString("ERROR: case ") + i + " ");
    }
}

void BasicNormalizerTest::assertEqual(const UnicodeString&    input,
                      const UnicodeString&    expected,
                      Normalizer*        iter,
                      const UnicodeString&    errPrefix)
{
    UnicodeString result;

    for (UChar32 ch = iter->first(); ch != iter->DONE; ch = iter->next()) {
        result += ch;
    }
    if (result != expected) {
        errln(errPrefix + "normalized " + hex(input) + "\n"
            + "                expected " + hex(expected) + "\n"
            + "             iterate got " + hex(result) );
    }
}

// helper class for TestPreviousNext()
// simple UTF-32 character iterator
class UChar32Iterator {
public:
    UChar32Iterator(const UChar32 *text, int32_t len, int32_t index) :
        s(text), length(len), i(index) {}

    UChar32 current() {
        if(i<length) {
            return s[i];
        } else {
            return 0xffff;
        }
    }

    UChar32 next() {
        if(i<length) {
            return s[i++];
        } else {
            return 0xffff;
        }
    }

    UChar32 previous() {
        if(i>0) {
            return s[--i];
        } else {
            return 0xffff;
        }
    }

    int32_t getIndex() {
        return i;
    }
private:
    const UChar32 *s;
    int32_t length, i;
};

void
BasicNormalizerTest::TestPreviousNext() {
    // src and expect strings
    static const UChar src[]={
        UTF16_LEAD(0x2f999), UTF16_TRAIL(0x2f999),
        UTF16_LEAD(0x1d15f), UTF16_TRAIL(0x1d15f),
        0xc4,
        0x1ed0
    };
    static const UChar32 expect[]={
        0x831d,
        0x1d158, 0x1d165,
        0x41, 0x308,
        0x4f, 0x302, 0x301
    };

    // expected src indexes corresponding to expect indexes
    static const int32_t expectIndex[]={
        0,
        2, 2,
        4, 4,
        5, 5, 5,
        6 // behind last character
    };

    // initial indexes into the src and expect strings
    enum {
        SRC_MIDDLE=4,
        EXPECT_MIDDLE=3
    };

    // movement vector
    // - for previous(), 0 for current(), + for next()
    // not const so that we can terminate it below for the error message
    static const char *moves="0+0+0--0-0-+++0--+++++++0--------";

    // iterators
    Normalizer iter(src, sizeof(src)/U_SIZEOF_UCHAR, UNORM_NFD);
    UChar32Iterator iter32(expect, sizeof(expect)/4, EXPECT_MIDDLE);

    UChar32 c1, c2;
    char m;

    // initially set the indexes into the middle of the strings
    iter.setIndexOnly(SRC_MIDDLE);

    // move around and compare the iteration code points with
    // the expected ones
    const char *move=moves;
    while((m=*move++)!=0) {
        if(m=='-') {
            c1=iter.previous();
            c2=iter32.previous();
        } else if(m=='0') {
            c1=iter.current();
            c2=iter32.current();
        } else /* m=='+' */ {
            c1=iter.next();
            c2=iter32.next();
        }

        // compare results
        if(c1!=c2) {
            // copy the moves until the current (m) move, and terminate
            char history[64];
            uprv_strcpy(history, moves);
            history[move-moves]=0;
            errln("error: mismatch in Normalizer iteration at %s: "
                  "got c1=U+%04lx != expected c2=U+%04lx\n",
                  history, c1, c2);
            break;
        }

        // compare indexes
        if(iter.getIndex()!=expectIndex[iter32.getIndex()]) {
            // copy the moves until the current (m) move, and terminate
            char history[64];
            uprv_strcpy(history, moves);
            history[move-moves]=0;
            errln("error: index mismatch in Normalizer iteration at %s: "
                  "Normalizer index %ld expected %ld\n",
                  history, iter.getIndex(), expectIndex[iter32.getIndex()]);
            break;
        }
    }
}

// test APIs that are not otherwise used - improve test coverage
void
BasicNormalizerTest::TestNormalizerAPI() {
    // instantiate a Normalizer from a CharacterIterator
    UnicodeString s=UnicodeString("a\\u0308\\uac00\\U0002f800", "").unescape();
    s.append(s); // make s a bit longer and more interesting
    StringCharacterIterator iter(s);
    Normalizer norm(iter, UNORM_NFC);
    if(norm.next()!=0xe4) {
        errln("error in Normalizer(CharacterIterator).next()");
    }

    // test copy constructor
    Normalizer copy(norm);
    if(copy.next()!=0xac00) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).next()");
    }

    // test clone(), ==, and hashCode()
    Normalizer *clone=copy.clone();
    if(*clone!=copy) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).clone()!=copy");
    }
    // clone must have the same hashCode()
    if(clone->hashCode()!=copy.hashCode()) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).clone()->hashCode()!=copy.hashCode()");
    }
    if(clone->next()!=0x4e3d) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).clone()->next()");
    }
    // position changed, must change hashCode()
    if(clone->hashCode()==copy.hashCode()) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).clone()->next().hashCode()==copy.hashCode()");
    }
    delete clone;
    clone=0;

    // test compose() and decompose()
    UnicodeString tel, nfkc, nfkd;
    tel=UnicodeString(1, (UChar32)0x2121, 10);
    tel.insert(1, (UChar)0x301);

    UErrorCode errorCode=U_ZERO_ERROR;
    Normalizer::compose(tel, TRUE, 0, nfkc, errorCode);
    Normalizer::decompose(tel, TRUE, 0, nfkd, errorCode);
    if(U_FAILURE(errorCode)) {
        errln("error in Normalizer::(de)compose(): %s", u_errorName(errorCode));
    } else if(
        nfkc!=UnicodeString("TE\\u0139TELTELTELTELTELTELTELTELTEL", "").unescape() || 
        nfkd!=UnicodeString("TEL\\u0301TELTELTELTELTELTELTELTELTEL", "").unescape()
    ) {
        errln("error in Normalizer::(de)compose(): wrong result(s)");
    }

    // test setIndex()
    if(norm.setIndex(3)!=0x4e3d) {
        errln("error in Normalizer(CharacterIterator).setIndex(3)");
    }

    // test setText(CharacterIterator) and getText()
    UnicodeString out, out2;
    errorCode=U_ZERO_ERROR;
    copy.setText(iter, errorCode);
    if(U_FAILURE(errorCode)) {
        errln("error Normalizer::setText() failed: %s", u_errorName(errorCode));
    } else {
        copy.getText(out);
        iter.getText(out2);
        if( out!=out2 ||
            copy.startIndex()!=iter.startIndex() ||
            copy.endIndex()!=iter.endIndex()
        ) {
            errln("error in Normalizer::setText() or Normalizer::getText()");
        }
    }

    // test setText(UChar *), getUMode() and setMode()
    errorCode=U_ZERO_ERROR;
    copy.setText(s.getBuffer()+1, s.length()-1, errorCode);
    copy.setMode(UNORM_NFD);
    if(copy.getUMode()!=UNORM_NFD) {
        errln("error in Normalizer::setMode() or Normalizer::getUMode()");
    }
    if(copy.next()!=0x308 || copy.next()!=0x1100) {
        errln("error in Normalizer::setText(UChar *) or Normalizer::setMode()");
    }

    // test setText(UChar *, length=-1)
    errorCode=U_ZERO_ERROR;

    // NUL-terminate s
    s.append((UChar)0);         // append NUL
    s.truncate(s.length()-1);   // undo length change

    copy.setText(s.getBuffer()+1, -1, errorCode);
    if(copy.endIndex()!=s.length()-1) {
        errln("error in Normalizer::setText(UChar *, -1)");
    }

    // test setOption() and getOption()
    copy.setOption(0xaa0000, TRUE);
    copy.setOption(0x20000, FALSE);
    if(!copy.getOption(0x880000) || copy.getOption(0x20000)) {
        errln("error in Normalizer::setOption() or Normalizer::getOption()");
    }

    // test last()/previous() with an internal buffer overflow
    errorCode=U_ZERO_ERROR;
    copy.setText(UnicodeString(1000, (UChar32)0x308, 1000), errorCode);
    if(copy.last()!=0x308) {
        errln("error in Normalizer(1000*U+0308).last()");
    }

    // test UNORM_NONE
    norm.setMode(UNORM_NONE);
    if(norm.first()!=0x61 || norm.next()!=0x308 || norm.last()!=0x2f800) {
        errln("error in Normalizer(UNORM_NONE).first()/next()/last()");
    }
    Normalizer::normalize(s, UNORM_NONE, 0, out, status);
    if(out!=s) {
        errln("error in Normalizer::normalize(UNORM_NONE)");
    }
}

void BasicNormalizerTest::TestConcatenate() {
    static const char *const
    cases[][4]={
        /* mode, left, right, result */
        {
            "C",
            "re",
            "\\u0301sum\\u00e9",
            "r\\u00e9sum\\u00e9"
        },
        {
            "C",
            "a\\u1100",
            "\\u1161bcdefghijk",
            "a\\uac00bcdefghijk"
        },
        /* ### TODO: add more interesting cases */
        {
            "D", 
            "\\u0340\\u0341\\u0343\\u0344\\u0374\\u037E\\u0387\\u0958" 
            "\\u0959\\u095A\\u095B\\u095C\\u095D\\u095E\\u095F\\u09DC" 
            "\\u09DD\\u09DF\\u0A33\\u0A36\\u0A59\\u0A5A\\u0A5B\\u0A5E" 
            "\\u0B5C\\u0B5D\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69" 
            "\\u0F73\\u0F75\\u0F76\\u0F78\\u0F81\\u0F93\\u0F9D\\u0FA2" 
            "\\u0FA7\\u0FAC\\u0FB9\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79" 
            "\\u1F7B\\u1F7D\\u1FBB\\u1FBE\\u1FC9\\u1FCB\\u1FD3\\u1FDB",
            
            "\\u1FE3\\u1FEB\\u1FEE\\u1FEF\\u1FF9\\u1FFB\\u1FFD\\u2000" 
            "\\u2001\\u2126\\u212A\\u212B\\u2329\\u232A\\uF900\\uFA10" 
            "\\uFA12\\uFA15\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A\\uFB1F" 
            "\\uFB2A\\uFB2B\\uFB2C\\uFB2D\\uFB2E\\uFB2F\\uFB30\\uFB31" 
            "\\uFB32\\uFB33\\uFB34\\uFB35\\uFB36\\uFB38\\uFB39\\uFB3A" 
            "\\uFB3B\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46" 
            "\\uFB47\\uFB48\\uFB49\\uFB4A\\uFB4B\\uFB4C\\uFB4D\\uFB4E",
           
            "\\u0340\\u0341\\u0343\\u0344\\u0374\\u037E\\u0387\\u0958"
            "\\u0959\\u095A\\u095B\\u095C\\u095D\\u095E\\u095F\\u09DC"
            "\\u09DD\\u09DF\\u0A33\\u0A36\\u0A59\\u0A5A\\u0A5B\\u0A5E"
            "\\u0B5C\\u0B5D\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69"
            "\\u0F73\\u0F75\\u0F76\\u0F78\\u0F81\\u0F93\\u0F9D\\u0FA2"
            "\\u0FA7\\u0FAC\\u0FB9\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79"
            "\\u1F7B\\u1F7D\\u1FBB\\u1FBE\\u1FC9\\u1FCB\\u1FD3\\u0399"
            "\\u0301\\u03C5\\u0308\\u0301\\u1FEB\\u1FEE\\u1FEF\\u1FF9"
            "\\u1FFB\\u1FFD\\u2000\\u2001\\u2126\\u212A\\u212B\\u2329"
            "\\u232A\\uF900\\uFA10\\uFA12\\uFA15\\uFA20\\uFA22\\uFA25"
            "\\uFA26\\uFA2A\\uFB1F\\uFB2A\\uFB2B\\uFB2C\\uFB2D\\uFB2E"
            "\\uFB2F\\uFB30\\uFB31\\uFB32\\uFB33\\uFB34\\uFB35\\uFB36"
            "\\uFB38\\uFB39\\uFB3A\\uFB3B\\uFB3C\\uFB3E\\uFB40\\uFB41"
            "\\uFB43\\uFB44\\uFB46\\uFB47\\uFB48\\uFB49\\uFB4A\\uFB4B"
            "\\uFB4C\\uFB4D\\uFB4E"
        }
    };

    UnicodeString left, right, expect, result, r;
    UErrorCode errorCode;
    UNormalizationMode mode;
    int32_t i;

    /* test concatenation */
    for(i=0; i<(int32_t)(sizeof(cases)/sizeof(cases[0])); ++i) {
        switch(*cases[i][0]) {
        case 'C': mode=UNORM_NFC; break;
        case 'D': mode=UNORM_NFD; break;
        case 'c': mode=UNORM_NFKC; break;
        case 'd': mode=UNORM_NFKD; break;
        default: mode=UNORM_NONE; break;
        }

        left=UnicodeString(cases[i][1], "").unescape();
        right=UnicodeString(cases[i][2], "").unescape();
        expect=UnicodeString(cases[i][3], "").unescape();

        //result=r=UnicodeString();
        errorCode=U_ZERO_ERROR;

        r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
        if(U_FAILURE(errorCode) || /*result!=r ||*/ result!=expect) {
            errln("error in Normalizer::concatenate(), cases[] fails with "+
                UnicodeString(u_errorName(errorCode))+", result==expect: expected: "+
                hex(expect)+" =========> got: " + hex(result));
        }
    }

    /* test error cases */

    /* left.getBuffer()==result.getBuffer() */
    result=r=expect=UnicodeString("zz", "");
    errorCode=U_UNEXPECTED_TOKEN;
    r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
    if(errorCode!=U_UNEXPECTED_TOKEN || result!=r || !result.isBogus()) {
        errln("error in Normalizer::concatenate(), violates UErrorCode protocol");
    }

    left.setToBogus();
    errorCode=U_ZERO_ERROR;
    r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || result!=r || !result.isBogus()) {
        errln("error in Normalizer::concatenate(), does not detect left.isBogus()");
    }
}

// reference implementation of Normalizer::compare
static int32_t
ref_norm_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options, UErrorCode &errorCode) {
    UnicodeString r1, r2, t1, t2;

    // get writable objects
    r1=s1;
    r2=s2;

    if(options&U_COMPARE_IGNORE_CASE) {
        r1.foldCase(options);
        r2.foldCase(options);
    }

    Normalizer::decompose(r1, FALSE, 0, t1, errorCode);
    Normalizer::decompose(r2, FALSE, 0, t2, errorCode);

    if(options&U_COMPARE_CODE_POINT_ORDER) {
        return t1.compareCodePointOrder(t2);
    } else {
        return t1.compare(t2);
    }
}

// test wrapper for Normalizer::compare, sets UNORM_INPUT_IS_FCD appropriately
static int32_t
_norm_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options, UErrorCode &errorCode) {
    if( UNORM_YES==Normalizer::quickCheck(s1, UNORM_FCD, errorCode) &&
        UNORM_YES==Normalizer::quickCheck(s2, UNORM_FCD, errorCode)) {
        options|=UNORM_INPUT_IS_FCD;
    }

    return Normalizer::compare(s1, s2, options, errorCode);
}

// reference implementation of UnicodeString::caseCompare
static int32_t
ref_case_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options) {
    UnicodeString t1, t2;

    t1=s1;
    t2=s2;

    t1.foldCase(options);
    t2.foldCase(options);

    if(options&U_COMPARE_CODE_POINT_ORDER) {
        return t1.compareCodePointOrder(t2);
    } else {
        return t1.compare(t2);
    }
}

// reduce an integer to -1/0/1
static inline int32_t
_sign(int32_t value) {
    if(value==0) {
        return 0;
    } else {
        return (value>>31)|1;
    }
}

void
BasicNormalizerTest::TestCompare() {
    // test Normalizer::compare and unorm_compare (thinly wrapped by the former)
    // by comparing it with its semantic equivalent
    // since we trust the pieces, this is sufficient

    // test each string with itself and each other
    // each time with all options
    static const char *const
    strings[]={
        // some cases from NormalizationTest.txt
        // 0..3
        "D\\u031B\\u0307\\u0323",
        "\\u1E0C\\u031B\\u0307",
        "D\\u031B\\u0323\\u0307",
        "d\\u031B\\u0323\\u0307",

        // 4..6
        "\\u00E4",
        "a\\u0308",
        "A\\u0308",

        // Angstrom sign = A ring
        // 7..10
        "\\u212B",
        "\\u00C5",
        "A\\u030A",
        "a\\u030A",

        // 11.14
        "a\\u059A\\u0316\\u302A\\u032Fb",
        "a\\u302A\\u0316\\u032F\\u059Ab",
        "a\\u302A\\u0316\\u032F\\u059Ab",
        "A\\u059A\\u0316\\u302A\\u032Fb",

        // from ICU case folding tests
        // 15..20
        "A\\u00df\\u00b5\\ufb03\\U0001040c\\u0131",
        "ass\\u03bcffi\\U00010434i",
        "\\u0061\\u0042\\u0131\\u03a3\\u00df\\ufb03\\ud93f\\udfff",
        "\\u0041\\u0062\\u0069\\u03c3\\u0073\\u0053\\u0046\\u0066\\u0049\\ud93f\\udfff",
        "\\u0041\\u0062\\u0131\\u03c3\\u0053\\u0073\\u0066\\u0046\\u0069\\ud93f\\udfff",
        "\\u0041\\u0062\\u0069\\u03c3\\u0073\\u0053\\u0046\\u0066\\u0049\\ud93f\\udffd",

        //     U+d800 U+10001   see implementation comment in unorm_cmpEquivFold
        // vs. U+10000          at bottom - code point order
        // 21..22
        "\\ud800\\ud800\\udc01",
        "\\ud800\\udc00",

        // other code point order tests from ustrtest.cpp
        // 23..31
        "\\u20ac\\ud801",
        "\\u20ac\\ud800\\udc00",
        "\\ud800",
        "\\ud800\\uff61",
        "\\udfff",
        "\\uff61\\udfff",
        "\\uff61\\ud800\\udc02",
        "\\ud800\\udc02",
        "\\ud84d\\udc56",

        // long strings, see cnormtst.c/TestNormCoverage()
        // equivalent if case-insensitive
        // 32..33
        "\\uAD8B\\uAD8B\\uAD8B\\uAD8B"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "aaaaaaaaaaaaaaaaaazzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        "\\uAD8B\\uAD8B\\uAD8B\\uAD8B"
        "d\\u031B\\u0307\\u0323",

        "\\u1100\\u116f\\u11aa\\uAD8B\\uAD8B\\u1100\\u116f\\u11aa"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "aaaaaaaaaaAAAAAAAAZZZZZZZZZZZZZZZZzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        "\\u1100\\u116f\\u11aa\\uAD8B\\uAD8B\\u1100\\u116f\\u11aa"
        "\\u1E0C\\u031B\\u0307",

        // some strings that may make a difference whether the compare function
        // case-folds or decomposes first
        // 34..41
        "\\u0360\\u0345\\u0334",
        "\\u0360\\u03b9\\u0334",

        "\\u0360\\u1f80\\u0334",
        "\\u0360\\u03b1\\u0313\\u03b9\\u0334",

        "\\u0360\\u1ffc\\u0334",
        "\\u0360\\u03c9\\u03b9\\u0334",

        "\\u00cc",
        "\\u0069\\u0300",

        // empty string
        // 42
        ""
    };

    UnicodeString s[100]; // at least as many items as in strings[] !

    // all combinations of options
    // UNORM_INPUT_IS_FCD is set automatically if both input strings fulfill FCD conditions
    static const struct {
        uint32_t options;
        const char *name;
    } opt[6]={
        { 0, "default" },
        { U_COMPARE_CODE_POINT_ORDER, "code point order" },
        { U_COMPARE_IGNORE_CASE, "ignore case" },
        { U_COMPARE_CODE_POINT_ORDER|U_COMPARE_IGNORE_CASE, "code point order & ignore case" },
        { U_COMPARE_IGNORE_CASE|U_FOLD_CASE_EXCLUDE_SPECIAL_I, "ignore case & special i" },
        { U_COMPARE_CODE_POINT_ORDER|U_COMPARE_IGNORE_CASE|U_FOLD_CASE_EXCLUDE_SPECIAL_I, "code point order & ignore case & special i" }
    };

    int32_t i, j, k, count=sizeof(strings)/sizeof(strings[0]);
    int32_t result, refResult;

    UErrorCode errorCode;

    // create the UnicodeStrings
    for(i=0; i<count; ++i) {
        s[i]=UnicodeString(strings[i], "").unescape();
    }

    // test them each with each other
    for(i=0; i<count; ++i) {
        for(j=i; j<count; ++j) {
            for(k=0; k<sizeof(opt)/sizeof(opt[0]); ++k) {
                // test Normalizer::compare
                errorCode=U_ZERO_ERROR;
                result=_norm_compare(s[i], s[j], opt[k].options, errorCode);
                refResult=ref_norm_compare(s[i], s[j], opt[k].options, errorCode);
                if(_sign(result)!=_sign(refResult)) {
                    errln("Normalizer::compare(%d, %d, %s)=%d should be same sign as %d (%s)",
                        i, j, opt[k].name, result, refResult, u_errorName(errorCode));
                }

                // test UnicodeString::caseCompare - same internal implementation function
                if(opt[k].options&U_COMPARE_IGNORE_CASE) {
                    errorCode=U_ZERO_ERROR;
                    result=s[i].caseCompare(s[j], opt[k].options);
                    refResult=ref_case_compare(s[i], s[j], opt[k].options);
                    if(_sign(result)!=_sign(refResult)) {
                        errln("Normalizer::compare(%d, %d, %s)=%d should be same sign as %d (%s)",
                            i, j, opt[k].name, result, refResult, u_errorName(errorCode));
                    }
                }
            }
        }
    }
}

// verify that case-folding does not un-FCD strings
int32_t
BasicNormalizerTest::countFoldFCDExceptions(uint32_t foldingOptions) {
    UnicodeString s, fold, d;
    UChar32 c;
    int32_t count;
    uint8_t cc, trailCC, foldCC, foldTrailCC;
    UNormalizationCheckResult qcResult;
    int8_t category;
    UBool isNFD;
    UErrorCode errorCode;

    logln("Test if case folding may un-FCD a string (folding options %04lx)", foldingOptions);

    count=0;
    for(c=0; c<=0x10ffff; ++c) {
        errorCode = U_ZERO_ERROR;
        category=u_charType(c);
        if(category==U_UNASSIGNED) {
            continue; // skip unassigned code points
        }
        if(c==0xac00) {
            c=0xd7a3; // skip Hangul - no case folding there
            continue;
        }
        // skip Han blocks - no case folding there either
        if(c==0x3400) {
            c=0x4db5;
            continue;
        }
        if(c==0x4e00) {
            c=0x9fa5;
            continue;
        }
        if(c==0x20000) {
            c=0x2a6d6;
            continue;
        }

        s.setTo(c);

        // get leading and trailing cc for c
        Normalizer::decompose(s, FALSE, 0, d, errorCode);
        isNFD= s==d;
        cc=u_getCombiningClass(d.char32At(0));
        trailCC=u_getCombiningClass(d.char32At(d.length()-1));

        // get leading and trailing cc for the case-folding of c
        s.foldCase(foldingOptions);
        Normalizer::decompose(s, FALSE, 0, d, errorCode);
        foldCC=u_getCombiningClass(d.char32At(0));
        foldTrailCC=u_getCombiningClass(d.char32At(d.length()-1));

        qcResult=Normalizer::quickCheck(s, UNORM_FCD, errorCode);

        if (U_FAILURE(errorCode)) {
            ++count;
            errln("U+%04lx: Failed with error %s", u_errorName(errorCode));
        }

        // bad:
        // - character maps to empty string: adjacent characters may then need reordering
        // - folding has different leading/trailing cc's, and they don't become just 0
        // - folding itself is not FCD
        if( qcResult!=UNORM_YES ||
            s.isEmpty() ||
            (cc!=foldCC && foldCC!=0) || (trailCC!=foldTrailCC && foldTrailCC!=0)
        ) {
            ++count;
            errln("U+%04lx: case-folding may un-FCD a string (folding options %04lx)", c, foldingOptions);
            errln("  cc %02x trailCC %02x    foldCC(U+%04lx) %02x foldTrailCC(U+%04lx) %02x   quickCheck(folded)=%d", cc, trailCC, d.char32At(0), foldCC, d.char32At(d.length()-1), foldTrailCC, qcResult);
            continue;
        }

        // also bad:
        // if a code point is in NFD but its case folding is not, then
        // unorm_compare will also fail
        if(isNFD && UNORM_YES!=Normalizer::quickCheck(s, UNORM_NFD, errorCode)) {
            ++count;
            errln("U+%04lx: case-folding un-NFDs this character (folding options %04lx)", c, foldingOptions);
        }
    }

    logln("There are %ld code points for which case-folding may un-FCD a string (folding options %04lx)", count, foldingOptions);
    return count;
}

void
BasicNormalizerTest::FindFoldFCDExceptions() {
    int32_t count;

    count=countFoldFCDExceptions(0);
    count+=countFoldFCDExceptions(U_FOLD_CASE_EXCLUDE_SPECIAL_I);
    if(count>0) {
        /*
         * If case-folding un-FCDs any strings, then unorm_compare() must be
         * re-implemented.
         * It currently assumes that one can check for FCD then case-fold
         * and then still have FCD strings for raw decomposition without reordering.
         */
        errln("error: There are %ld code points for which case-folding may un-FCD a string for all folding options.\n"
              "See comment in BasicNormalizerTest::FindFoldFCDExceptions()!", count);
    }
}
