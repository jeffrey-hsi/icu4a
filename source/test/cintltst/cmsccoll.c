/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2001-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*******************************************************************************
*
* File cmsccoll.C
*
*******************************************************************************/
/**
 * These are the tests specific to ICU 1.8 and above, that I didn't know where
 * to fit.
 */

#include <stdio.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "unicode/ustring.h"
#include "string.h"
#include "ucol_imp.h"
#include "ucol_tok.h"
#include "cmemory.h"
#include "cstring.h"
#include "unicode/parseerr.h"
#include "unicode/ucnv.h"
#include "uparse.h"

#define MAX_TOKEN_LEN 16

typedef int tst_strcoll(void *collator, const int object,
                        const UChar *source, const int sLen,
                        const UChar *target, const int tLen);



const static char cnt1[][10] = {

  "AA",
  "AC",
  "AZ",
  "AQ",
  "AB",
  "ABZ",
  "ABQ",
  "Z",
  "ABC",
  "Q",
  "B"
};

const static char cnt2[][10] = {
  "DA",
  "DAD",
  "DAZ",
  "MAR",
  "Z",
  "DAVIS",
  "MARK",
  "DAV",
  "DAVI"
};

static void IncompleteCntTest(void)
{
  UErrorCode status = U_ZERO_ERROR;
  UChar temp[90];
  UChar t1[90];
  UChar t2[90];

  UCollator *coll =  NULL;
  uint32_t i = 0, j = 0;
  uint32_t size = 0;

  u_uastrcpy(temp, " & Z < ABC < Q < B");

  coll = ucol_openRules(temp, u_strlen(temp), UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL,&status);

  if(U_SUCCESS(status)) {
    size = sizeof(cnt1)/sizeof(cnt1[0]);
    for(i = 0; i < size-1; i++) {
      for(j = i+1; j < size; j++) {
        UCollationElements *iter;
        u_uastrcpy(t1, cnt1[i]);
        u_uastrcpy(t2, cnt1[j]);
        doTest(coll, t1, t2, UCOL_LESS);
        /* synwee : added collation element iterator test */
        iter = ucol_openElements(coll, t2, u_strlen(t2), &status);
        if (U_FAILURE(status)) {
          log_err("Creation of iterator failed\n");
          break;
        }
        backAndForth(iter);
        ucol_closeElements(iter);
      }
    }
  }

  ucol_close(coll);


  u_uastrcpy(temp, " & Z < DAVIS < MARK <DAV");
  coll = ucol_openRules(temp, u_strlen(temp), UCOL_OFF, UCOL_DEFAULT_STRENGTH,NULL, &status);

  if(U_SUCCESS(status)) {
    size = sizeof(cnt2)/sizeof(cnt2[0]);
    for(i = 0; i < size-1; i++) {
      for(j = i+1; j < size; j++) {
        UCollationElements *iter;
        u_uastrcpy(t1, cnt2[i]);
        u_uastrcpy(t2, cnt2[j]);
        doTest(coll, t1, t2, UCOL_LESS);

        /* synwee : added collation element iterator test */
        iter = ucol_openElements(coll, t2, u_strlen(t2), &status);
        if (U_FAILURE(status)) {
          log_err("Creation of iterator failed\n");
          break;
        }
        backAndForth(iter);
        ucol_closeElements(iter);
      }
    }
  }

  ucol_close(coll);


}

const static char shifted[][20] = {
  "black bird",
  "black-bird",
  "blackbird",
  "black Bird",
  "black-Bird",
  "blackBird",
  "black birds",
  "black-birds",
  "blackbirds"
};

const static UCollationResult shiftedTert[] = {
  0,
  UCOL_EQUAL,
  UCOL_EQUAL,
  UCOL_LESS,
  UCOL_EQUAL,
  UCOL_EQUAL,
  UCOL_LESS,
  UCOL_EQUAL,
  UCOL_EQUAL
};

const static char nonignorable[][20] = {
  "black bird",
  "black Bird",
  "black birds",
  "black-bird",
  "black-Bird",
  "black-birds",
  "blackbird",
  "blackBird",
  "blackbirds"
};

static void BlackBirdTest(void) {
  UErrorCode status = U_ZERO_ERROR;
  UChar t1[90];
  UChar t2[90];

  uint32_t i = 0, j = 0;
  uint32_t size = 0;
  UCollator *coll = ucol_open("en_US", &status);

  ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_OFF, &status);
  ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, &status);

  if(U_SUCCESS(status)) {
    size = sizeof(nonignorable)/sizeof(nonignorable[0]);
    for(i = 0; i < size-1; i++) {
      for(j = i+1; j < size; j++) {
        u_uastrcpy(t1, nonignorable[i]);
        u_uastrcpy(t2, nonignorable[j]);
        doTest(coll, t1, t2, UCOL_LESS);
      }
    }
  }

  ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
  ucol_setAttribute(coll, UCOL_STRENGTH, UCOL_QUATERNARY, &status);

  if(U_SUCCESS(status)) {
    size = sizeof(shifted)/sizeof(shifted[0]);
    for(i = 0; i < size-1; i++) {
      for(j = i+1; j < size; j++) {
        u_uastrcpy(t1, shifted[i]);
        u_uastrcpy(t2, shifted[j]);
        doTest(coll, t1, t2, UCOL_LESS);
      }
    }
  }

  ucol_setAttribute(coll, UCOL_STRENGTH, UCOL_TERTIARY, &status);
  if(U_SUCCESS(status)) {
    size = sizeof(shifted)/sizeof(shifted[0]);
    for(i = 1; i < size; i++) {
      u_uastrcpy(t1, shifted[i-1]);
      u_uastrcpy(t2, shifted[i]);
      doTest(coll, t1, t2, shiftedTert[i]);
    }
  }

  ucol_close(coll);
}

const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
    {0x0041/*'A'*/, 0x0300, 0x0301, 0x0000},
    {0x0041/*'A'*/, 0x0300, 0x0316, 0x0000},
    {0x0041/*'A'*/, 0x0300, 0x0000},
    {0x00C0, 0x0301, 0x0000},
    /* this would work with forced normalization */
    {0x00C0, 0x0316, 0x0000}
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
    {0x0041/*'A'*/, 0x0301, 0x0300, 0x0000},
    {0x0041/*'A'*/, 0x0316, 0x0300, 0x0000},
    {0x00C0, 0},
    {0x0041/*'A'*/, 0x0301, 0x0300, 0x0000},
    /* this would work with forced normalization */
    {0x0041/*'A'*/, 0x0316, 0x0300, 0x0000}
};

const static UCollationResult results[] = {
    UCOL_GREATER,
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_GREATER,
    UCOL_EQUAL
};

static void FunkyATest(void)
{

    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    UCollator  *myCollation;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing some A letters, for some reason\n");
    ucol_setAttribute(myCollation, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 4 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

UColAttributeValue caseFirst[] = {
    UCOL_OFF,
    UCOL_LOWER_FIRST,
    UCOL_UPPER_FIRST
};


UColAttributeValue alternateHandling[] = {
    UCOL_NON_IGNORABLE,
    UCOL_SHIFTED
};

UColAttributeValue caseLevel[] = {
    UCOL_OFF,
    UCOL_ON
};

UColAttributeValue strengths[] = {
    UCOL_PRIMARY,
    UCOL_SECONDARY,
    UCOL_TERTIARY,
    UCOL_QUATERNARY,
    UCOL_IDENTICAL
};

#if 0
static const char * strengthsC[] = {
    "UCOL_PRIMARY",
    "UCOL_SECONDARY",
    "UCOL_TERTIARY",
    "UCOL_QUATERNARY",
    "UCOL_IDENTICAL"
};

static const char * caseFirstC[] = {
    "UCOL_OFF",
    "UCOL_LOWER_FIRST",
    "UCOL_UPPER_FIRST"
};


static const char * alternateHandlingC[] = {
    "UCOL_NON_IGNORABLE",
    "UCOL_SHIFTED"
};

static const char * caseLevelC[] = {
    "UCOL_OFF",
    "UCOL_ON"
};

/* not used currently - does not test only prints */
static void PrintMarkDavis(void)
{
  UErrorCode status = U_ZERO_ERROR;
  UChar m[256];
  uint8_t sortkey[256];
  UCollator *coll = ucol_open("en_US", &status);
  uint32_t h,i,j,k, sortkeysize;
  uint32_t sizem = 0;
  char buffer[512];
  uint32_t len = 512;

  log_verbose("PrintMarkDavis");

  u_uastrcpy(m, "Mark Davis");
  sizem = u_strlen(m);


  m[1] = 0xe4;

  for(i = 0; i<sizem; i++) {
    fprintf(stderr, "\\u%04X ", m[i]);
  }
  fprintf(stderr, "\n");

  for(h = 0; h<sizeof(caseFirst)/sizeof(caseFirst[0]); h++) {
    ucol_setAttribute(coll, UCOL_CASE_FIRST, caseFirst[i], &status);
    fprintf(stderr, "caseFirst: %s\n", caseFirstC[h]);

    for(i = 0; i<sizeof(alternateHandling)/sizeof(alternateHandling[0]); i++) {
      ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, alternateHandling[i], &status);
      fprintf(stderr, "  AltHandling: %s\n", alternateHandlingC[i]);

      for(j = 0; j<sizeof(caseLevel)/sizeof(caseLevel[0]); j++) {
        ucol_setAttribute(coll, UCOL_CASE_LEVEL, caseLevel[j], &status);
        fprintf(stderr, "    caseLevel: %s\n", caseLevelC[j]);

        for(k = 0; k<sizeof(strengths)/sizeof(strengths[0]); k++) {
          ucol_setAttribute(coll, UCOL_STRENGTH, strengths[k], &status);
          sortkeysize = ucol_getSortKey(coll, m, sizem, sortkey, 256);
          fprintf(stderr, "      strength: %s\n      Sortkey: ", strengthsC[k]);
          fprintf(stderr, "%s\n", ucol_sortKeyToString(coll, sortkey, buffer, &len));
        }

      }

    }

  }
}
#endif

static void BillFairmanTest(void) {
/*
** check for actual locale via ICU resource bundles
**
** lp points to the original locale ("fr_FR_....")
*/

    UResourceBundle *lr,*cr;
    UErrorCode              lec = U_ZERO_ERROR;
    const char *lp = "fr_FR_you_ll_never_find_this_locale";

    log_verbose("BillFairmanTest\n");

    lr = ures_open(NULL,lp,&lec);
    if (lr) {
        cr = ures_getByKey(lr,"CollationElements",0,&lec);
        if (cr) {
            lp = ures_getLocale(cr,&lec);
            if (lp) {
                if (U_SUCCESS(lec)) {
                    if(strcmp(lp, "fr") != 0) {
                        log_err("Wrong locale for French Collation Data, expected \"fr\" got %s", lp);
                    }
                }
            }
            ures_close(cr);
        }
        ures_close(lr);
    }
}

static void testPrimary(UCollator* col, const UChar* p,const UChar* q){
    UChar source[256] = { '\0'};
    UChar target[256] = { '\0'};
    UChar preP = 0x31a3;
    UChar preQ = 0x310d;
/*
    UChar preP = (*p>0x0400 && *p<0x0500)?0x00e1:0x491;
    UChar preQ = (*p>0x0400 && *p<0x0500)?0x0041:0x413;
*/
    /*log_verbose("Testing primary\n");*/

    doTest(col, p, q, UCOL_LESS);
/*
    UCollationResult result = ucol_strcoll(col,p,u_strlen(p),q,u_strlen(q));

    if(result!=UCOL_LESS){
       aescstrdup(p,utfSource,256);
       aescstrdup(q,utfTarget,256);
       fprintf(file,"Primary failed  source: %s target: %s \n", utfSource,utfTarget);
    }
*/
    source[0] = preP;
    u_strcpy(source+1,p);
    target[0] = preQ;
    u_strcpy(target+1,q);
    doTest(col, source, target, UCOL_LESS);
/*
    fprintf(file,"Primary swamps 2nd failed  source: %s target: %s \n", utfSource,utfTarget);
*/
}

static void testSecondary(UCollator* col, const UChar* p,const UChar* q){
    UChar source[256] = { '\0'};
    UChar target[256] = { '\0'};

    /*log_verbose("Testing secondary\n");*/

    doTest(col, p, q, UCOL_LESS);
/*
    fprintf(file,"secondary failed  source: %s target: %s \n", utfSource,utfTarget);
*/
    source[0] = 0x0053;
    u_strcpy(source+1,p);
    target[0]= 0x0073;
    u_strcpy(target+1,q);

    doTest(col, source, target, UCOL_LESS);
/*
    fprintf(file,"secondary swamps 3rd failed  source: %s target: %s \n",utfSource,utfTarget);
*/


    u_strcpy(source,p);
    source[u_strlen(p)] = 0x62;
    source[u_strlen(p)+1] = 0;


    u_strcpy(target,q);
    target[u_strlen(q)] = 0x61;
    target[u_strlen(q)+1] = 0;

    doTest(col, source, target, UCOL_GREATER);

/*
    fprintf(file,"secondary is swamped by 1  failed  source: %s target: %s \n",utfSource,utfTarget);
*/
}

static void testTertiary(UCollator* col, const UChar* p,const UChar* q){
    UChar source[256] = { '\0'};
    UChar target[256] = { '\0'};

    /*log_verbose("Testing tertiary\n");*/

    doTest(col, p, q, UCOL_LESS);
/*
    fprintf(file,"Tertiary failed  source: %s target: %s \n",utfSource,utfTarget);
*/
    source[0] = 0x0020;
    u_strcpy(source+1,p);
    target[0]= 0x002D;
    u_strcpy(target+1,q);

    doTest(col, source, target, UCOL_LESS);
/*
    fprintf(file,"Tertiary swamps 4th failed  source: %s target: %s \n", utfSource,utfTarget);
*/

    u_strcpy(source,p);
    source[u_strlen(p)] = 0xE0;
    source[u_strlen(p)+1] = 0;

    u_strcpy(target,q);
    target[u_strlen(q)] = 0x61;
    target[u_strlen(q)+1] = 0;

    doTest(col, source, target, UCOL_GREATER);

/*
    fprintf(file,"Tertiary is swamped by 3rd failed  source: %s target: %s \n",utfSource,utfTarget);
*/
}

static void testEquality(UCollator* col, const UChar* p,const UChar* q){
/*
    UChar source[256] = { '\0'};
    UChar target[256] = { '\0'};
*/

    doTest(col, p, q, UCOL_EQUAL);
/*
    fprintf(file,"Primary failed  source: %s target: %s \n", utfSource,utfTarget);
*/
}

static void testCollator(UCollator *coll, UErrorCode *status) {
  const UChar *rules = NULL, *current = NULL;
  int32_t ruleLen = 0;
  uint32_t strength = 0;
  uint32_t chOffset = 0; uint32_t chLen = 0;
  uint32_t exOffset = 0; uint32_t exLen = 0;
  uint32_t prefixOffset = 0; uint32_t prefixLen = 0;
  uint32_t firstEx = 0;
/*  uint32_t rExpsLen = 0; */
  uint32_t firstLen = 0;
  UBool varT = FALSE; UBool top_ = TRUE;
  uint16_t specs = 0;
  UBool startOfRules = TRUE;
  UBool lastReset = FALSE;
  UBool before = FALSE;
  UColTokenParser src;
  UColOptionSet opts;

  UChar first[256];
  UChar second[256];
  UChar tempB[256];
  uint32_t tempLen;
  UChar *rulesCopy = NULL;
  UParseError parseError;
  src.opts = &opts;

  rules = ucol_getRules(coll, &ruleLen);
  if(U_SUCCESS(*status) && ruleLen > 0) {
    rulesCopy = (UChar *)malloc((ruleLen+UCOL_TOK_EXTRA_RULE_SPACE_SIZE)*sizeof(UChar));
    uprv_memcpy(rulesCopy, rules, ruleLen*sizeof(UChar));
    src.current = src.source = rulesCopy;
    src.end = rulesCopy+ruleLen;
    src.extraCurrent = src.end;
    src.extraEnd = src.end+UCOL_TOK_EXTRA_RULE_SPACE_SIZE;
    *first = *second = 0;

    while ((current = ucol_tok_parseNextToken(&src, startOfRules,&parseError, status)) != NULL) {
      strength = src.parsedToken.strength;
      chOffset = src.parsedToken.charsOffset;
      chLen = src.parsedToken.charsLen;
      exOffset = src.parsedToken.extensionOffset;
      exLen = src.parsedToken.extensionLen;
      prefixOffset = src.parsedToken.prefixOffset;
      prefixLen = src.parsedToken.prefixLen;
      specs = src.parsedToken.flags;

      startOfRules = FALSE;
      varT = (UBool)((specs & UCOL_TOK_VARIABLE_TOP) != 0);
      top_ = (UBool)((specs & UCOL_TOK_TOP) != 0);
      if(top_) { /* if reset is on top, the sequence is broken. We should have an empty string */
        second[0] = 0;
      } else {
        u_strncpy(second,rulesCopy+chOffset, chLen);
        second[chLen] = 0;

        if(exLen > 0 && firstEx == 0) {
          u_strncat(first, rulesCopy+exOffset, exLen);
          first[firstLen+exLen] = 0;
        }

        if(lastReset == TRUE && prefixLen != 0) {
          u_strncpy(first+prefixLen, first, firstLen);
          u_strncpy(first, rulesCopy+prefixOffset, prefixLen); 
          first[firstLen+prefixLen] = 0;
          firstLen = firstLen+prefixLen;
        }

        if(before == TRUE) { /* swap first and second */
          u_strcpy(tempB, first);
          u_strcpy(first, second);
          u_strcpy(second, tempB);

          tempLen = firstLen;
          firstLen = chLen;
          chLen = tempLen;

          tempLen = firstEx;
          firstEx = exLen;
          exLen = tempLen;
        }
      }
      lastReset = FALSE;

      switch(strength){
      case UCOL_IDENTICAL:
          testEquality(coll,first,second);
          break;
      case UCOL_PRIMARY:
          testPrimary(coll,first,second);
          break;
      case UCOL_SECONDARY:
          testSecondary(coll,first,second);
          break;
      case UCOL_TERTIARY:
          testTertiary(coll,first,second);
          break;
      case UCOL_TOK_RESET:
        lastReset = TRUE;
        before = (UBool)((specs & UCOL_TOK_BEFORE) != 0);
        break;
      default:
          break;
      }

      if(before == TRUE && strength != UCOL_TOK_RESET) { /* first and second were swapped */
        before = FALSE; 
      } else {
        firstLen = chLen;
        firstEx = exLen;
        u_strcpy(first, second);
      }
    }
    free(rulesCopy);
  }
}

static int ucaTest(void *collator, const int object, const UChar *source, const int sLen, const UChar *target, const int tLen) {
  UCollator *UCA = (UCollator *)collator;
  return ucol_strcoll(UCA, source, sLen, target, tLen);
}

/*
static int winTest(void *collator, const int object, const UChar *source, const int sLen, const UChar *target, const int tLen) {
#ifdef WIN32
  LCID lcid = (LCID)collator;
  return CompareString(lcid, 0, source, sLen, target, tLen);
#else
  return 0;
#endif
}
*/

static UCollationResult swampEarlier(tst_strcoll* func, void *collator, int opts,
                                     UChar s1, UChar s2,
                                     const UChar *s, const uint32_t sLen,
                                     const UChar *t, const uint32_t tLen) {
  UChar source[256] = {0};
  UChar target[256] = {0};

  source[0] = s1;
  u_strcpy(source+1, s);
  target[0] = s2;
  u_strcpy(target+1, t);

  return func(collator, opts, source, sLen+1, target, tLen+1);
}

static UCollationResult swampLater(tst_strcoll* func, void *collator, int opts,
                                   UChar s1, UChar s2,
                                   const UChar *s, const uint32_t sLen,
                                   const UChar *t, const uint32_t tLen) {
  UChar source[256] = {0};
  UChar target[256] = {0};

  u_strcpy(source, s);
  source[sLen] = s1;
  u_strcpy(target, t);
  target[tLen] = s2;

  return func(collator, opts, source, sLen+1, target, tLen+1);
}

static uint32_t probeStrength(tst_strcoll* func, void *collator, int opts,
                              const UChar *s, const uint32_t sLen,
                              const UChar *t, const uint32_t tLen,
                              UCollationResult result) {
  /*UChar fPrimary = 0x6d;*/
  /*UChar sPrimary = 0x6e;*/
  UChar fSecondary = 0x310d;
  UChar sSecondary = 0x31a3;
  UChar fTertiary = 0x310f;
  UChar sTertiary = 0x31b7;

  UCollationResult oposite;
  if(result == UCOL_EQUAL) {
    return UCOL_IDENTICAL;
  } else if(result == UCOL_GREATER) {
    oposite = UCOL_LESS;
  } else {
    oposite = UCOL_GREATER;
  }

  if(swampEarlier(func, collator, opts, sSecondary, fSecondary, s, sLen, t, tLen) == result) {
    return UCOL_PRIMARY;
  } else if((swampEarlier(func, collator, opts, sTertiary, 0x310f, s, sLen, t, tLen) == result) &&
    (swampEarlier(func, collator, opts, 0x310f, sTertiary, s, sLen, t, tLen) == result)) {
    return UCOL_SECONDARY;
  } else if((swampLater(func, collator, opts, sTertiary, fTertiary, s, sLen, t, tLen) == result) &&
    (swampLater(func, collator, opts, fTertiary, sTertiary, s, sLen, t, tLen) == result)) {
    return UCOL_TERTIARY;
  } else if((swampLater(func, collator, opts, sTertiary, 0x310f, s, sLen, t, tLen) == oposite) &&
    (swampLater(func, collator, opts, fTertiary, sTertiary, s, sLen, t, tLen) == oposite)) {
    return UCOL_QUATERNARY;
  } else {
    return UCOL_IDENTICAL;
  }
}

static char *getRelationSymbol(UCollationResult res, uint32_t strength, char *buffer) {
  uint32_t i = 0;

  if(res == UCOL_EQUAL || strength == 0xdeadbeef) {
    buffer[0] = '=';
    buffer[1] = '=';
    buffer[2] = '\0';
  } else if(res == UCOL_GREATER) {
    for(i = 0; i<strength+1; i++) {
      buffer[i] = '>';
    }
    buffer[strength+1] = '\0';
  } else {
    for(i = 0; i<strength+1; i++) {
      buffer[i] = '<';
    }
    buffer[strength+1] = '\0';
  }

  return buffer;
}



static void logFailure (const char *platform, const char *test,
                        const UChar *source, const uint32_t sLen,
                        const UChar *target, const uint32_t tLen,
                        UCollationResult realRes, uint32_t realStrength,
                        UCollationResult expRes, uint32_t expStrength, UBool error) {

  uint32_t i = 0;

  char sEsc[256], s[256], tEsc[256], t[256], b[256], output[256], relation[256];

  *sEsc = *tEsc = *s = *t = 0;
  if(error == TRUE) {
    log_err("Difference between expected and generated order. Run test with -v for more info\n");
  }
  for(i = 0; i<sLen; i++) {
    sprintf(b, "%04X", source[i]);
    strcat(sEsc, "\\u");
    strcat(sEsc, b);
    strcat(s, b);
    strcat(s, " ");
    if(source[i] < 0x80) {
      sprintf(b, "(%c)", source[i]);
      strcat(sEsc, b);
    }
  }
  for(i = 0; i<tLen; i++) {
    sprintf(b, "%04X", target[i]);
    strcat(tEsc, "\\u");
    strcat(tEsc, b);
    strcat(t, b);
    strcat(t, " ");
    if(target[i] < 0x80) {
      sprintf(b, "(%c)", target[i]);
      strcat(tEsc, b);
    }
  }
/*
  strcpy(output, "[[ ");
  strcat(output, sEsc);
  strcat(output, getRelationSymbol(expRes, expStrength, relation));
  strcat(output, tEsc);

  strcat(output, " : ");

  strcat(output, sEsc);
  strcat(output, getRelationSymbol(realRes, realStrength, relation));
  strcat(output, tEsc);
  strcat(output, " ]] ");

  log_verbose("%s", output);
*/


  strcpy(output, "DIFF: ");

  strcat(output, s);
  strcat(output, " : ");
  strcat(output, t);

  strcat(output, test);
  strcat(output, ": ");

  strcat(output, sEsc);
  strcat(output, getRelationSymbol(expRes, expStrength, relation));
  strcat(output, tEsc);

  strcat(output, " ");

  strcat(output, platform);
  strcat(output, ": ");

  strcat(output, sEsc);
  strcat(output, getRelationSymbol(realRes, realStrength, relation));
  strcat(output, tEsc);

  log_verbose("%s\n", output);

}

/*
static void printOutRules(const UChar *rules) {
  uint32_t len = u_strlen(rules);
  uint32_t i = 0;
  char toPrint;
  uint32_t line = 0;

  fprintf(stdout, "Rules:");

  for(i = 0; i<len; i++) {
    if(rules[i]<0x7f && rules[i]>=0x20) {
      toPrint = (char)rules[i];
      if(toPrint == '&') {
        line = 1;
        fprintf(stdout, "\n&");
      } else if(toPrint == ';') {
        fprintf(stdout, "<<");
        line+=2;
      } else if(toPrint == ',') {
        fprintf(stdout, "<<<");
        line+=3;
      } else {
        fprintf(stdout, "%c", toPrint);
        line++;
      }
    } else if(rules[i]<0x3400 || rules[i]>=0xa000) {
      fprintf(stdout, "\\u%04X", rules[i]);
      line+=6;
    }
    if(line>72) {
      fprintf(stdout, "\n");
      line = 0;
    }
  }

  log_verbose("\n");

}
*/

static uint32_t testSwitch(tst_strcoll* func, void *collator, int opts, uint32_t strength, const UChar *first, const UChar *second, const char* msg, UBool error) {
  uint32_t diffs = 0;
  UCollationResult realResult;
  uint32_t realStrength;

  uint32_t sLen = u_strlen(first);
  uint32_t tLen = u_strlen(second);

  realResult = func(collator, opts, first, sLen, second, tLen);
  realStrength = probeStrength(func, collator, opts, first, sLen, second, tLen, realResult);

  if(strength == UCOL_IDENTICAL && realResult != UCOL_IDENTICAL) {
    logFailure(msg, "tailoring", first, sLen, second, tLen, realResult, realStrength, UCOL_EQUAL, strength, error);
    diffs++;
  } else if(realResult != UCOL_LESS || realStrength != strength) {
    logFailure(msg, "tailoring", first, sLen, second, tLen, realResult, realStrength, UCOL_LESS, strength, error);
    diffs++;
  }
  return diffs;
}


static void testAgainstUCA(UCollator *coll, UCollator *UCA, const char *refName, UBool error, UErrorCode *status) {
  const UChar *rules = NULL, *current = NULL;
  int32_t ruleLen = 0;
  uint32_t strength = 0;
  uint32_t chOffset = 0; uint32_t chLen = 0;
  uint32_t exOffset = 0; uint32_t exLen = 0;
  uint32_t prefixOffset = 0; uint32_t prefixLen = 0;
/*  uint32_t rExpsLen = 0; */
  uint32_t firstLen = 0, secondLen = 0;
  UBool varT = FALSE; UBool top_ = TRUE;
  uint16_t specs = 0;
  UBool startOfRules = TRUE;
  UColTokenParser src;
  UColOptionSet opts;

  UChar first[256];
  UChar second[256];
  UChar *rulesCopy = NULL;

  uint32_t UCAdiff = 0;
  uint32_t Windiff = 1;
  UParseError parseError;

  src.opts = &opts;

  rules = ucol_getRules(coll, &ruleLen);

  /*printOutRules(rules);*/

  if(U_SUCCESS(*status) && ruleLen > 0) {
    rulesCopy = (UChar *)malloc((ruleLen+UCOL_TOK_EXTRA_RULE_SPACE_SIZE)*sizeof(UChar));
    uprv_memcpy(rulesCopy, rules, ruleLen*sizeof(UChar));
    src.current = src.source = rulesCopy;
    src.end = rulesCopy+ruleLen;
    src.extraCurrent = src.end;
    src.extraEnd = src.end+UCOL_TOK_EXTRA_RULE_SPACE_SIZE;
    *first = *second = 0;

    while ((current = ucol_tok_parseNextToken(&src, startOfRules, &parseError,status)) != NULL) {
      strength = src.parsedToken.strength;
      chOffset = src.parsedToken.charsOffset;
      chLen = src.parsedToken.charsLen;
      exOffset = src.parsedToken.extensionOffset;
      exLen = src.parsedToken.extensionLen;
      prefixOffset = src.parsedToken.prefixOffset;
      prefixLen = src.parsedToken.prefixLen;
      specs = src.parsedToken.flags;

      startOfRules = FALSE;
      varT = (UBool)((specs & UCOL_TOK_VARIABLE_TOP) != 0);
      top_ = (UBool)((specs & UCOL_TOK_TOP) != 0);

      u_strncpy(second,rulesCopy+chOffset, chLen);
      second[chLen] = 0;
      secondLen = chLen;

      if(exLen > 0) {
        u_strncat(first, rulesCopy+exOffset, exLen);
        first[firstLen+exLen] = 0;
        firstLen += exLen;
      }

      if(strength != UCOL_TOK_RESET) {
        if((*first<0x3400 || *first>=0xa000) && (*second<0x3400 || *second>=0xa000)) {
          UCAdiff += testSwitch(&ucaTest, (void *)UCA, 0, strength, first, second, refName, error);
          /*Windiff += testSwitch(&winTest, (void *)lcid, 0, strength, first, second, "Win32");*/
        }
      }


      firstLen = chLen;
      u_strcpy(first, second);

    }
    if(UCAdiff != 0 && Windiff != 0) {
      log_verbose("\n");
    }
    if(UCAdiff == 0) {
      log_verbose("No immediate difference with %s!\n", refName);
    }
    if(Windiff == 0) {
      log_verbose("No immediate difference with Win32!\n");
    }
    free(rulesCopy);
  }
}

/* 
 * Takes two CEs (lead and continuation) and 
 * compares them as CEs should be compared:
 * primary vs. primary, secondary vs. secondary
 * tertiary vs. tertiary
 */
static int32_t compareCEs(uint32_t s1, uint32_t s2,
                   uint32_t t1, uint32_t t2) {
  uint32_t s = 0, t = 0;
  if(s1 == t1 && s2 == t2) {
    return 0;
  }
  s = (s1 & 0xFFFF0000)|((s2 & 0xFFFF0000)>>16); 
  t = (t1 & 0xFFFF0000)|((t2 & 0xFFFF0000)>>16); 
  if(s < t) {
    return -1;
  } else if(s > t) {
    return 1;
  } else {
    s = (s1 & 0x0000FF00) | (s2 & 0x0000FF00)>>8;
    t = (t1 & 0x0000FF00) | (t2 & 0x0000FF00)>>8;
    if(s < t) {
      return -1;
    } else if(s > t) {
      return 1;
    } else {
      s = (s1 & 0x000000FF)<<8 | (s2 & 0x000000FF);
      t = (t1 & 0x000000FF)<<8 | (t2 & 0x000000FF);
      if(s < t) {
        return -1;
      } else {
        return 1;
      }
    }
  }
}

typedef struct {
  uint32_t startCE;
  uint32_t startContCE;
  uint32_t limitCE;
  uint32_t limitContCE;
} indirectBoundaries;

/* these values are used for finding CE values for indirect positioning. */
/* Indirect positioning is a mechanism for allowing resets on symbolic   */
/* values. It only works for resets and you cannot tailor indirect names */
/* An indirect name can define either an anchor point or a range. An     */
/* anchor point behaves in exactly the same way as a code point in reset */
/* would, except that it cannot be tailored. A range (we currently only  */
/* know for the [top] range will explicitly set the upper bound for      */
/* generated CEs, thus allowing for better control over how many CEs can */
/* be squeezed between in the range without performance penalty.         */
/* In that respect, we use [top] for tailoring of locales that use CJK   */
/* characters. Other indirect values are currently a pure convenience,   */
/* they can be used to assure that the CEs will be always positioned in  */
/* the same place relative to a point with known properties (e.g. first  */
/* primary ignorable). */
static indirectBoundaries ucolIndirectBoundaries[15];
static UBool indirectBoundariesSet = FALSE;
static void setIndirectBoundaries(uint32_t indexR, uint32_t *start, uint32_t *end) { 
  
  /* Set values for the top - TODO: once we have values for all the indirects, we are going */
  /* to initalize here. */
  ucolIndirectBoundaries[indexR].startCE = start[0];
  ucolIndirectBoundaries[indexR].startContCE = start[1];
  if(end) {
    ucolIndirectBoundaries[indexR].limitCE = end[0];
    ucolIndirectBoundaries[indexR].limitContCE = end[1];
  } else {
    ucolIndirectBoundaries[indexR].limitCE = 0;
    ucolIndirectBoundaries[indexR].limitContCE = 0;
  }
}

static void testCEs(UCollator *coll, UErrorCode *status) {

  const UChar *rules = NULL, *current = NULL;
  int32_t ruleLen = 0;

  uint32_t strength = 0;
  uint32_t maxStrength = UCOL_IDENTICAL;
  uint32_t baseCE, baseContCE, nextCE, nextContCE, currCE, currContCE;
  uint32_t lastCE;
  uint32_t lastContCE;

  int32_t result = 0;
  uint32_t chOffset = 0; uint32_t chLen = 0;
  uint32_t exOffset = 0; uint32_t exLen = 0;
  uint32_t prefixOffset = 0; uint32_t prefixLen = 0;
  uint32_t oldOffset = 0;

  /* uint32_t rExpsLen = 0; */
  /* uint32_t firstLen = 0; */
  uint16_t specs = 0;
  UBool varT = FALSE; UBool top_ = TRUE;
  UBool startOfRules = TRUE;
  UBool before = FALSE;
  UColTokenParser src;
  UColOptionSet opts;
  UParseError parseError;
  UChar *rulesCopy = NULL;
  collIterate c;
  UCollator *UCA = ucol_open("root", status);
  UCAConstants *consts = (UCAConstants *)((uint8_t *)UCA->image + UCA->image->UCAConsts);
  uint32_t UCOL_RESET_TOP_VALUE = consts->UCA_LAST_NON_VARIABLE[0], UCOL_RESET_TOP_CONT = consts->UCA_LAST_NON_VARIABLE[1], 
           UCOL_NEXT_TOP_VALUE = consts->UCA_FIRST_IMPLICIT[0], UCOL_NEXT_TOP_CONT = consts->UCA_FIRST_IMPLICIT[1];
  
  baseCE=baseContCE=nextCE=nextContCE=currCE=currContCE=lastCE=lastContCE = UCOL_NOT_FOUND;

  src.opts = &opts;

  rules = ucol_getRules(coll, &ruleLen);

  ucol_initInverseUCA(status);

  if(indirectBoundariesSet == FALSE) {
    /* UCOL_RESET_TOP_VALUE */
    setIndirectBoundaries(0, consts->UCA_LAST_NON_VARIABLE, consts->UCA_FIRST_IMPLICIT); 
    /* UCOL_FIRST_PRIMARY_IGNORABLE */
    setIndirectBoundaries(1, consts->UCA_FIRST_PRIMARY_IGNORABLE, 0);
    /* UCOL_LAST_PRIMARY_IGNORABLE */
    setIndirectBoundaries(2, consts->UCA_LAST_PRIMARY_IGNORABLE, 0);
    /* UCOL_FIRST_SECONDARY_IGNORABLE */
    setIndirectBoundaries(3, consts->UCA_FIRST_SECONDARY_IGNORABLE, 0);
    /* UCOL_LAST_SECONDARY_IGNORABLE */
    setIndirectBoundaries(4, consts->UCA_LAST_SECONDARY_IGNORABLE, 0);
    /* UCOL_FIRST_TERTIARY_IGNORABLE */
    setIndirectBoundaries(5, consts->UCA_FIRST_TERTIARY_IGNORABLE, 0);
    /* UCOL_LAST_TERTIARY_IGNORABLE */
    setIndirectBoundaries(6, consts->UCA_LAST_TERTIARY_IGNORABLE, 0);
    /* UCOL_FIRST_VARIABLE */
    setIndirectBoundaries(7, consts->UCA_FIRST_VARIABLE, 0);
    /* UCOL_LAST_VARIABLE */
    setIndirectBoundaries(8, consts->UCA_LAST_VARIABLE, 0);
    /* UCOL_FIRST_NON_VARIABLE */
    setIndirectBoundaries(9, consts->UCA_FIRST_NON_VARIABLE, 0);
    /* UCOL_LAST_NON_VARIABLE */
    setIndirectBoundaries(10, consts->UCA_LAST_NON_VARIABLE, consts->UCA_FIRST_IMPLICIT);
    /* UCOL_FIRST_IMPLICIT */
    setIndirectBoundaries(11, consts->UCA_FIRST_IMPLICIT, 0);
    /* UCOL_LAST_IMPLICIT */
    setIndirectBoundaries(12, consts->UCA_LAST_IMPLICIT, consts->UCA_FIRST_TRAILING);
    /* UCOL_FIRST_TRAILING */
    setIndirectBoundaries(13, consts->UCA_FIRST_TRAILING, 0);
    /* UCOL_LAST_TRAILING */
    setIndirectBoundaries(14, consts->UCA_LAST_TRAILING, 0);
    ucolIndirectBoundaries[14].limitCE = (consts->UCA_PRIMARY_SPECIAL_MIN<<24);
    indirectBoundariesSet = TRUE;
  }


  if(U_SUCCESS(*status) && ruleLen > 0) {
    rulesCopy = (UChar *)malloc((ruleLen+UCOL_TOK_EXTRA_RULE_SPACE_SIZE)*sizeof(UChar));
    uprv_memcpy(rulesCopy, rules, ruleLen*sizeof(UChar));
    src.current = src.source = rulesCopy;
    src.end = rulesCopy+ruleLen;
    src.extraCurrent = src.end;
    src.extraEnd = src.end+UCOL_TOK_EXTRA_RULE_SPACE_SIZE;

    while ((current = ucol_tok_parseNextToken(&src, startOfRules, &parseError,status)) != NULL) {
      strength = src.parsedToken.strength;
      chOffset = src.parsedToken.charsOffset;
      chLen = src.parsedToken.charsLen;
      exOffset = src.parsedToken.extensionOffset;
      exLen = src.parsedToken.extensionLen;
      prefixOffset = src.parsedToken.prefixOffset;
      prefixLen = src.parsedToken.prefixLen;
      specs = src.parsedToken.flags;

      startOfRules = FALSE;
      varT = (UBool)((specs & UCOL_TOK_VARIABLE_TOP) != 0);
      top_ = (UBool)((specs & UCOL_TOK_TOP) != 0);

      uprv_init_collIterate(coll, rulesCopy+chOffset, chLen, &c);

      currCE = ucol_getNextCE(coll, &c, status);
      if(currCE == 0 && UCOL_ISTHAIPREVOWEL(*(rulesCopy+chOffset))) {
        log_verbose("Thai prevowel detected. Will pick next CE\n");
        currCE = ucol_getNextCE(coll, &c, status);
      }

      currContCE = ucol_getNextCE(coll, &c, status);
      if(!isContinuation(currContCE)) {
        currContCE = 0;
      }

      /* we need to repack CEs here */

      if(strength == UCOL_TOK_RESET) {
        before = (UBool)((specs & UCOL_TOK_BEFORE) != 0);
        if(top_ == TRUE) {
          int32_t index = src.parsedToken.indirectIndex;
          
          nextCE = baseCE = currCE = ucolIndirectBoundaries[index].startCE;
          nextContCE = baseContCE = currContCE = ucolIndirectBoundaries[index].startContCE;
        } else {
          nextCE = baseCE = currCE;
          nextContCE = baseContCE = currContCE;
        }
        maxStrength = UCOL_IDENTICAL;
      } else {
        if(strength < maxStrength) {
          maxStrength = strength;
          if(baseCE == UCOL_RESET_TOP_VALUE) {
              log_verbose("Resetting to [top]\n");
              nextCE = UCOL_NEXT_TOP_VALUE;
              nextContCE = UCOL_NEXT_TOP_CONT;
          } else {
            result = ucol_inv_getNextCE(baseCE & 0xFFFFFF3F, baseContCE, &nextCE, &nextContCE, maxStrength);
          }
          if(result < 0) {
            if(ucol_isTailored(coll, *(rulesCopy+oldOffset), status)) {
              log_verbose("Reset is tailored codepoint %04X, don't know how to continue, taking next test\n", *(rulesCopy+oldOffset));
              return;
            } else {
              log_err("couldn't find the CE\n");
              return;
            }
          }
        }

        currCE &= 0xFFFFFF3F;
        currContCE &= 0xFFFFFFBF;

        if(maxStrength == UCOL_IDENTICAL) {
          if(baseCE != currCE || baseContCE != currContCE) {
            log_err("current CE  (initial strength UCOL_EQUAL)\n");
          }
        } else {
          if(strength == UCOL_IDENTICAL) {
            if(lastCE != currCE || lastContCE != currContCE) {
              log_err("current CE  (initial strength UCOL_EQUAL)\n");
            }
          } else {
            if(compareCEs(currCE, currContCE, nextCE, nextContCE) > 0) {
            /*if(currCE > nextCE || (currCE == nextCE && currContCE >= nextContCE)) {*/
              log_err("current CE is not less than base CE\n");
            }
            if(!before) {
              if(compareCEs(currCE, currContCE, lastCE, lastContCE) < 0) {
              /*if(currCE < lastCE || (currCE == lastCE && currContCE <= lastContCE)) {*/
                log_err("sequence of generated CEs is broken\n");
              }
            } else {
              before = FALSE;
              if(compareCEs(currCE, currContCE, lastCE, lastContCE) > 0) {
              /*if(currCE < lastCE || (currCE == lastCE && currContCE <= lastContCE)) {*/
                log_err("sequence of generated CEs is broken\n");
              }
            }
          }
        }

      }

      oldOffset = chOffset;
      lastCE = currCE & 0xFFFFFF3F;
      lastContCE = currContCE & 0xFFFFFFBF;
    }
    free(rulesCopy);
  }
  ucol_close(UCA);
}

#if 0
/* these locales are now picked from index RB */
static const char* localesToTest[] = {
"ar", "bg", "ca", "cs", "da",
"el", "en_BE", "en_US_POSIX",
"es", "et", "fi", "fr", "hi",
"hr", "hu", "is", "iw", "ja",
"ko", "lt", "lv", "mk", "mt",
"nb", "nn", "nn_NO", "pl", "ro",
"ru", "sh", "sk", "sl", "sq",
"sr", "sv", "th", "tr", "uk",
"vi", "zh", "zh_TW"
};
#endif

static const char* rulesToTest[] = {
  /* Funky fa rule */
  "&\\u0622 < \\u0627 << \\u0671 < \\u0621",
  /*"& Z < p, P",*/
    /* Cui Mins rules */
    "&[top]<o,O<p,P<q,Q<'?'/u<r,R<u,U", /*"<o,O<p,P<q,Q<r,R<u,U & Qu<'?'",*/
    "&[top]<o,O<p,P<q,Q;'?'/u<r,R<u,U", /*"<o,O<p,P<q,Q<r,R<u,U & Qu;'?'",*/
    "&[top]<o,O<p,P<q,Q,'?'/u<r,R<u,U", /*"<o,O<p,P<q,Q<r,R<u,U&'Qu','?'",*/
    "&[top]<3<4<5<c,C<f,F<m,M<o,O<p,P<q,Q;'?'/u<r,R<u,U",  /*"<'?'<3<4<5<a,A<f,F<m,M<o,O<p,P<q,Q<r,R<u,U & Qu;'?'",*/
    "&[top]<'?';Qu<3<4<5<c,C<f,F<m,M<o,O<p,P<q,Q<r,R<u,U",  /*"<'?'<3<4<5<a,A<f,F<m,M<o,O<p,P<q,Q<r,R<u,U & '?';Qu",*/
    "&[top]<3<4<5<c,C<f,F<m,M<o,O<p,P<q,Q;'?'/um<r,R<u,U", /*"<'?'<3<4<5<a,A<f,F<m,M<o,O<p,P<q,Q<r,R<u,U & Qum;'?'",*/
    "&[top]<'?';Qum<3<4<5<c,C<f,F<m,M<o,O<p,P<q,Q<r,R<u,U"  /*"<'?'<3<4<5<a,A<f,F<m,M<o,O<p,P<q,Q<r,R<u,U & '?';Qum"*/
};

static UBool hasCollationElements(const char *locName) {

  UErrorCode status = U_ZERO_ERROR;
  UResourceBundle *ColEl = NULL;

  UResourceBundle *loc = ures_open(NULL, locName, &status);;

  if(U_SUCCESS(status)) {
    status = U_ZERO_ERROR;
    ColEl = ures_getByKey(loc, "CollationElements", ColEl, &status);
    if(status == U_ZERO_ERROR) { /* do the test - there are real elements */
      ures_close(ColEl);
      ures_close(loc);
      return TRUE;
    }
    ures_close(ColEl);
    ures_close(loc);
  }
  return FALSE;
}


static void TestCollations(void) {
  int32_t noOfLoc = uloc_countAvailable();
  int32_t i = 0, j = 0;

  UErrorCode status = U_ZERO_ERROR;
  char cName[256];
  UChar name[256];
  int32_t nameSize;


  const char *locName = NULL;
  UCollator *coll = NULL;
  UCollator *UCA = ucol_open("", &status);
  UColAttributeValue oldStrength = ucol_getAttribute(UCA, UCOL_STRENGTH, &status);
  ucol_setAttribute(UCA, UCOL_STRENGTH, UCOL_QUATERNARY, &status);

  for(i = 0; i<noOfLoc; i++) {
    status = U_ZERO_ERROR;
    locName = uloc_getAvailable(i);
    if(uprv_strcmp("ja", locName) == 0) {
      log_verbose("Don't know how to test prefixes\n");
      continue;
    }
    if(hasCollationElements(locName)) {
        nameSize = uloc_getDisplayName(locName, NULL, name, 256, &status);
        for(j = 0; j<nameSize; j++) {
          cName[j] = (char)name[j];
        }
        cName[nameSize] = 0;
        log_verbose("\nTesting locale %s (%s)\n", locName, cName);
        coll = ucol_open(locName, &status);
        testAgainstUCA(coll, UCA, "UCA", FALSE, &status);
        ucol_close(coll);
    }
  }
  ucol_setAttribute(UCA, UCOL_STRENGTH, oldStrength, &status);
  ucol_close(UCA);
}

static void RamsRulesTest(void) {
  UErrorCode status = U_ZERO_ERROR;
  int32_t i = 0;
  UCollator *coll = NULL;
  UChar rule[2048];
  uint32_t ruleLen;
  int32_t noOfLoc = uloc_countAvailable();
  const char *locName = NULL;

  log_verbose("RamsRulesTest\n");

  for(i = 0; i<noOfLoc; i++) {
    status = U_ZERO_ERROR;
    locName = uloc_getAvailable(i);
    if(hasCollationElements(locName)) {
      if (uprv_strcmp("ja", locName)==0) {
        log_verbose("Don't know how to test Japanese because of prefixes\n");
        continue;
      }
      log_verbose("Testing locale %s\n", locName);
      coll = ucol_open(locName, &status);
      if(U_SUCCESS(status)) {
        if(coll->image->jamoSpecial == TRUE) {
          log_err("%s has special JAMOs\n", locName);
        }
        ucol_setAttribute(coll, UCOL_CASE_FIRST, UCOL_OFF, &status);
        testCollator(coll, &status);
        testCEs(coll, &status);
        ucol_close(coll);
      }
    }
  }

  for(i = 0; i<sizeof(rulesToTest)/sizeof(rulesToTest[0]); i++) {
    log_verbose("Testing rule: %s\n", rulesToTest[i]);
    ruleLen = u_unescape(rulesToTest[i], rule, 2048);
    coll = ucol_openRules(rule, ruleLen, UCOL_OFF, UCOL_TERTIARY, NULL,&status);
    if(U_SUCCESS(status)) {
      testCollator(coll, &status);
      testCEs(coll, &status);
      ucol_close(coll);
    }
  }

}

static void IsTailoredTest(void) {
  UErrorCode status = U_ZERO_ERROR;
  uint32_t i = 0;
  UCollator *coll = NULL;
  UChar rule[2048];
  UChar tailored[2048];
  UChar notTailored[2048];
  uint32_t ruleLen, tailoredLen, notTailoredLen;

  log_verbose("IsTailoredTest\n");

  u_uastrcpy(rule, "&Z < A, B, C;c < d");
  ruleLen = u_strlen(rule);

  u_uastrcpy(tailored, "ABCcd");
  tailoredLen = u_strlen(tailored);

  u_uastrcpy(notTailored, "ZabD");
  notTailoredLen = u_strlen(notTailored);

  coll = ucol_openRules(rule, ruleLen, UCOL_OFF, UCOL_TERTIARY, NULL,&status);
  if(U_SUCCESS(status)) {
    for(i = 0; i<tailoredLen; i++) {
      if(!ucol_isTailored(coll, tailored[i], &status)) {
        log_err("%i: %04X should be tailored - it is reported as not\n", i, tailored[i]);
      }
    }
    for(i = 0; i<notTailoredLen; i++) {
      if(ucol_isTailored(coll, notTailored[i], &status)) {
        log_err("%i: %04X should not be tailored - it is reported as it is\n", i, notTailored[i]);
      }
    }
    ucol_close(coll);
  }
}


const static char chTest[][20] = {
  "c",
  "C",
  "ca", "cb", "cx", "cy", "CZ",
  "c\\u030C", "C\\u030C",
  "h",
  "H",
  "ha", "Ha", "harly", "hb", "HB", "hx", "HX", "hy", "HY",
  "ch", "cH", "Ch", "CH",
  "cha", "charly", "che", "chh", "chch", "chr",
  "i", "I", "iarly",
  "r", "R",
  "r\\u030C", "R\\u030C",
  "s",
  "S",
  "s\\u030C", "S\\u030C",
  "z", "Z",
  "z\\u030C", "Z\\u030C"
};

static void TestChMove(void) {
  UChar t1[256] = {0};
  UChar t2[256] = {0};

  uint32_t i = 0, j = 0;
  uint32_t size = 0;
  UErrorCode status = U_ZERO_ERROR;

  UCollator *coll = ucol_open("cs", &status);

  if(U_SUCCESS(status)) {
    size = sizeof(chTest)/sizeof(chTest[0]);
    for(i = 0; i < size-1; i++) {
      for(j = i+1; j < size; j++) {
        u_unescape(chTest[i], t1, 256);
        u_unescape(chTest[j], t2, 256);
        doTest(coll, t1, t2, UCOL_LESS);
      }
    }
  }
  else {
    log_err("Can't open collator");
  }
  ucol_close(coll);
}

const static char impTest[][20] = {
  "\\u4e00",
    "a",
    "A",
    "b",
    "B",
    "\\u4e01"
};


static void TestImplicitTailoring(void) {
  UChar t1[256] = {0};
  UChar t2[256] = {0};

  const char *rule = "&\\u4e00 < a <<< A < b <<< B";

  uint32_t i = 0, j = 0;
  uint32_t size = 0;
  uint32_t ruleLen = 0;
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = NULL;
  ruleLen = u_unescape(rule, t1, 256);

  coll = ucol_openRules(t1, ruleLen, UCOL_OFF, UCOL_TERTIARY,NULL, &status);

  if(U_SUCCESS(status)) {
    size = sizeof(impTest)/sizeof(impTest[0]);
    for(i = 0; i < size-1; i++) {
      for(j = i+1; j < size; j++) {
        u_unescape(impTest[i], t1, 256);
        u_unescape(impTest[j], t2, 256);
        doTest(coll, t1, t2, UCOL_LESS);
      }
    }
  }
  else {
    log_err("Can't open collator");
  }
  ucol_close(coll);
}

static void TestFCDProblem(void) {
  UChar t1[256] = {0};
  UChar t2[256] = {0};

  const char *s1 = "\\u0430\\u0306\\u0325";
  const char *s2 = "\\u04D1\\u0325";

  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open("", &status);
  u_unescape(s1, t1, 256);
  u_unescape(s2, t2, 256);

  ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_OFF, &status);
  doTest(coll, t1, t2, UCOL_EQUAL);

  ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
  doTest(coll, t1, t2, UCOL_EQUAL);

  ucol_close(coll);
}

#define NORM_BUFFER_TEST_LEN 32
typedef struct {
  UChar32 u;
  UChar NFC[NORM_BUFFER_TEST_LEN];
  UChar NFD[NORM_BUFFER_TEST_LEN];
} tester;

static void TestComposeDecompose(void) {
    int32_t noOfLoc;
    int32_t i = 0, j = 0;

    UErrorCode status = U_ZERO_ERROR;

    const char *locName = NULL;

    uint32_t nfcSize;
    uint32_t nfdSize;
    tester **t;
    uint32_t noCases = 0;
    UCollator *coll = NULL;
    UChar32 u = 0;
    UChar comp[NORM_BUFFER_TEST_LEN];
    uint32_t len = 0;
    UCollationElements *iter;

    noOfLoc = uloc_countAvailable();

    t = malloc(0x30000 * sizeof(tester *));
    t[0] = (tester *)malloc(sizeof(tester));
    log_verbose("Testing UCA extensively\n");
    coll = ucol_open("", &status);
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening collator\n");
      return;
    }


    for(u = 0; u < 0x30000; u++) {
      len = 0;
      UTF_APPEND_CHAR_UNSAFE(comp, len, u);
        nfcSize = unorm_normalize(comp, len, UNORM_NFC, 0, t[noCases]->NFC, NORM_BUFFER_TEST_LEN, &status);
        nfdSize = unorm_normalize(comp, len, UNORM_NFD, 0, t[noCases]->NFD, NORM_BUFFER_TEST_LEN, &status);

        if(nfcSize != nfdSize || (uprv_memcmp(t[noCases]->NFC, t[noCases]->NFD, nfcSize * sizeof(UChar)) != 0) 
          || (len != nfdSize || (uprv_memcmp(comp, t[noCases]->NFD, nfdSize * sizeof(UChar)) != 0))) {
            t[noCases]->u = u;
            if(len != nfdSize || (uprv_memcmp(comp, t[noCases]->NFD, nfdSize * sizeof(UChar)) != 0)) {
              u_strncpy(t[noCases]->NFC, comp, len);
              t[noCases]->NFC[len] = 0;
            }
            noCases++;
            t[noCases] = (tester *)malloc(sizeof(tester));
            uprv_memset(t[noCases], 0, sizeof(tester));
        } 
    }

    for(u=0; u<(UChar32)noCases; u++) {
      if(!ucol_equal(coll, t[u]->NFC, -1, t[u]->NFD, -1)) {
        log_err("Failure: codePoint %05X fails TestComposeDecompose in the UCA\n", t[u]->u);
        doTest(coll, t[u]->NFC, t[u]->NFD, UCOL_EQUAL);
      }
    }
    /*
    for(u = 0; u < 0x30000; u++) {
      if(!(u&0xFFFF)) {
        log_verbose("%08X ", u);
      }
      uprv_memset(t[noCases], 0, sizeof(tester));
      t[noCases]->u = u;
      len = 0;
      UTF_APPEND_CHAR_UNSAFE(comp, len, u);
      comp[len] = 0;
      nfcSize = unorm_normalize(comp, len, UNORM_NFC, 0, t[noCases]->NFC, NORM_BUFFER_TEST_LEN, &status);
      nfdSize = unorm_normalize(comp, len, UNORM_NFD, 0, t[noCases]->NFD, NORM_BUFFER_TEST_LEN, &status);
      doTest(coll, comp, t[noCases]->NFD, UCOL_EQUAL);
      doTest(coll, comp, t[noCases]->NFC, UCOL_EQUAL);
    }
    */

    ucol_close(coll);

    log_verbose("Testing locales, number of cases = %i\n", noCases);
    iter = ucol_openElements(coll, t[u]->NFD, u_strlen(t[u]->NFD), &status);
    for(i = 0; i<noOfLoc; i++) {
        status = U_ZERO_ERROR;
        locName = uloc_getAvailable(i);
        if(hasCollationElements(locName)) {
            char cName[256];
            UChar name[256];
            int32_t nameSize = uloc_getDisplayName(locName, NULL, name, sizeof(cName), &status);

            for(j = 0; j<nameSize; j++) {
                cName[j] = (char)name[j];
            }
            cName[nameSize] = 0;
            log_verbose("\nTesting locale %s (%s)\n", locName, cName);

            coll = ucol_open(locName, &status);
            ucol_setStrength(coll, UCOL_IDENTICAL);

            for(u=0; u<(UChar32)noCases; u++) {
              if(!ucol_equal(coll, t[u]->NFC, -1, t[u]->NFD, -1)) {
                log_err("Failure: codePoint %05X fails TestComposeDecompose for locale %s\n", t[u]->u, cName);
                doTest(coll, t[u]->NFC, t[u]->NFD, UCOL_EQUAL);
                log_verbose("Testing NFC\n");
                ucol_setText(iter, t[u]->NFC, u_strlen(t[u]->NFC), &status);
                  backAndForth(iter);
                log_verbose("Testing NFD\n");
                  ucol_setText(iter, t[u]->NFD, u_strlen(t[u]->NFD), &status);
                  backAndForth(iter);
              }
            }
            ucol_close(coll);
        }
    }
    for(u = 0; u <= (UChar32)noCases; u++) {
        free(t[u]);
    }
    free(t);
    ucol_closeElements(iter);
}

static void TestEmptyRule(void) {
  UErrorCode status = U_ZERO_ERROR;
  UChar rulez[] = { 0 };
  UCollator *coll = ucol_openRules(rulez, 0, UCOL_OFF, UCOL_TERTIARY,NULL, &status);

  ucol_close(coll);
}

static void TestUCARules(void) {
  UErrorCode status = U_ZERO_ERROR;
  UChar b[256];
  UChar *rules = b;
  uint32_t ruleLen = 0;
  UCollator *UCAfromRules = NULL;
  UCollator *coll = ucol_open("", &status);
  if(status == U_FILE_ACCESS_ERROR) {
    log_data_err("Is your data around?\n");
    return;
  } else if(U_FAILURE(status)) {
    log_err("Error opening collator\n");
    return;
  }
  ruleLen = ucol_getRulesEx(coll, UCOL_FULL_RULES, rules, 256);

  log_verbose("TestUCARules\n");
  if(ruleLen > 256) {
    rules = (UChar *)malloc((ruleLen+1)*sizeof(UChar));
    ruleLen = ucol_getRulesEx(coll, UCOL_FULL_RULES, rules, ruleLen);
  }
  log_verbose("Rules length is %d\n", ruleLen);
  UCAfromRules = ucol_openRules(rules, ruleLen, UCOL_OFF, UCOL_TERTIARY, NULL,&status);
  if(U_SUCCESS(status)) {
    ucol_close(UCAfromRules);
  } else {
    log_verbose("Unable to create a collator from UCARules!\n");
  }
/*
  u_unescape(blah, b, 256);
  ucol_getSortKey(coll, b, 1, res, 256);
*/
  ucol_close(coll);
  if(rules != b) {
    free(rules);
  }
}


/* Pinyin tonal order */
/*
    A < .. (\u0101) < .. (\u00e1) < .. (\u01ce) < .. (\u00e0)
          (w/macron)<  (w/acute)<   (w/caron)<   (w/grave)
    E < .. (\u0113) < .. (\u00e9) < .. (\u011b) < .. (\u00e8)
    I < .. (\u012b) < .. (\u00ed) < .. (\u01d0) < .. (\u00ec)
    O < .. (\u014d) < .. (\u00f3) < .. (\u01d2) < .. (\u00f2)
    U < .. (\u016b) < .. (\u00fa) < .. (\u01d4) < .. (\u00f9)
      < .. (\u01d6) < .. (\u01d8) < .. (\u01da) < .. (\u01dc) <
.. (\u00fc)

However, in testing we got the following order:
    A < .. (\u00e1) < .. (\u00e0) < .. (\u01ce) < .. (\u0101)
          (w/acute)<   (w/grave)<   (w/caron)<   (w/macron)
    E < .. (\u00e9) < .. (\u00e8) < .. (\u00ea) < .. (\u011b) <
.. (\u0113)
    I < .. (\u00ed) < .. (\u00ec) < .. (\u01d0) < .. (\u012b)
    O < .. (\u00f3) < .. (\u00f2) < .. (\u01d2) < .. (\u014d)
    U < .. (\u00fa) < .. (\u00f9) < .. (\u01d4) < .. (\u00fc) <
.. (\u01d8)
      < .. (\u01dc) < .. (\u01da) < .. (\u01d6) < .. (\u016b)
*/

static void TestBefore(void) {
  const static char *data[] = {
      "\\u0101", "\\u00e1", "\\u01ce", "\\u00e0", "A",
      "\\u0113", "\\u00e9", "\\u011b", "\\u00e8", "E",
      "\\u012b", "\\u00ed", "\\u01d0", "\\u00ec", "I",
      "\\u014d", "\\u00f3", "\\u01d2", "\\u00f2", "O",
      "\\u016b", "\\u00fa", "\\u01d4", "\\u00f9", "U",
      "\\u01d6", "\\u01d8", "\\u01da", "\\u01dc", "\\u00fc"
  };
  genericRulesStarter(
    "&[before 1]a<\\u0101<\\u00e1<\\u01ce<\\u00e0"
    "&[before 1]e<\\u0113<\\u00e9<\\u011b<\\u00e8"
    "&[before 1]i<\\u012b<\\u00ed<\\u01d0<\\u00ec"
    "&[before 1]o<\\u014d<\\u00f3<\\u01d2<\\u00f2"
    "&[before 1]u<\\u016b<\\u00fa<\\u01d4<\\u00f9"
    "&u<\\u01d6<\\u01d8<\\u01da<\\u01dc<\\u00fc",
    data, sizeof(data)/sizeof(data[0]));
}

static void TestJ784(void) {
  const static char *data[] = {
      "A", "\\u0101", "\\u00e1", "\\u01ce", "\\u00e0",
      "E", "\\u0113", "\\u00e9", "\\u011b", "\\u00e8",
      "I", "\\u012b", "\\u00ed", "\\u01d0", "\\u00ec",
      "O", "\\u014d", "\\u00f3", "\\u01d2", "\\u00f2",
      "U", "\\u016b", "\\u00fa", "\\u01d4", "\\u00f9",
      "\\u00fc",
           "\\u01d6", "\\u01d8", "\\u01da", "\\u01dc"
  };
  genericLocaleStarter("zh", data, sizeof(data)/sizeof(data[0]));
}


static void TestJ831(void) {
  const static char *data[] = {
    "I",
      "i",
      "Y",
      "y"
  };
  genericLocaleStarter("lv", data, sizeof(data)/sizeof(data[0]));
}

static void TestJ815(void) {
  const static char *data[] = {
    "aa",
      "Aa",
      "ab",
      "Ab",
      "ad",
      "Ad",
      "ae",
      "Ae",
      "\\u00e6",
      "\\u00c6",
      "af",
      "Af",
      "b",
      "B"
  };
  genericLocaleStarter("fr", data, sizeof(data)/sizeof(data[0]));
  genericRulesStarter("[backwards 2]&A<<\\u00e6/e<<<\\u00c6/E", data, sizeof(data)/sizeof(data[0]));
}


/*
"& a < b < c < d& r < c",                                   "& a < b < d& r < c",
"& a < b < c < d& c < m",                                   "& a < b < c < m < d",
"& a < b < c < d& a < m",                                   "& a < m < b < c < d",
"& a <<< b << c < d& a < m",                                "& a <<< b << c < m < d",
"& a < b < c < d& [before 1] c < m",                        "& a < b < m < c < d",
"& a < b <<< c << d <<< e& [before 3] e <<< x",            "& a < b <<< c << d <<< x <<< e",
"& a < b <<< c << d <<< e& [before 2] e <<< x",            "& a < b <<< c <<< x << d <<< e",
"& a < b <<< c << d <<< e& [before 1] e <<< x",            "& a <<< x < b <<< c << d <<< e",
"& a < b <<< c << d <<< e <<< f < g& [before 1] g < x",    "& a < b <<< c << d <<< e <<< f < x < g",
*/
static void TestRedundantRules(void) {
  int32_t i;

  const static char *rules[] = {
    "& a <<< b <<< c << d <<< e& [before 1] e <<< x",
    "& a < b <<< c << d <<< e& [before 1] e <<< x",
    "& a < b < c < d& [before 1] c < m",
    "& a < b <<< c << d <<< e& [before 3] e <<< x",
    "& a < b <<< c << d <<< e& [before 2] e <<< x",
    "& a < b <<< c << d <<< e <<< f < g& [before 1] g < x",
    "& a <<< b << c < d& a < m",
    "&a<b<<b\\u0301 &z<b",
    "&z<m<<<q<<<m",
    "&z<<<m<q<<<m",
    "& a < b < c < d& r < c",
    "& a < b < c < d& r < c",
    "& a < b < c < d& c < m",
    "& a < b < c < d& a < m"
  };

  const static char *expectedRules[] = {
    /*"&\\u3029<<<x",*/
    "&\\u2089<<<x",
    "& a <<< x < b <<< c << d <<< e",
    "& a < b < m < c < d",
    "& a < b <<< c << d <<< x <<< e",
    "& a < b <<< c <<< x << d <<< e",
    "& a < b <<< c << d <<< e <<< f < x < g",
    "& a <<< b << c < m < d",
    "&a<b\\u0301 &z<b",
    "&z<q<<<m",
    "&z<q<<<m",
    "& a < b < d& r < c",
    "& a < b < d& r < c",
    "& a < b < c < m < d",
    "& a < m < b < c < d"
  };

  const static char *testdata[][8] = {
    /*{"\\u3029", "x"},*/
    {"\\u2089", "x"},
    {"a", "x", "b", "c", "d", "e"},
    {"a", "b", "m", "c", "d"},
    {"a", "b", "c", "d", "x", "e"},
    {"a", "b", "c", "x", "d", "e"},
    {"a", "b", "c", "d", "e", "f", "x", "g"},
    {"a", "b", "c", "m", "d"},
    {"a", "b\\u0301", "z", "b"},
    {"z", "q", "m"},
    {"z", "q", "m"},
    {"a", "b", "d"},
    {"r", "c"},
    {"a", "b", "c", "m", "d"},
    {"a", "m", "b", "c", "d"}
  };

  const static uint32_t testdatalen[] = {
    2,
      6,
      5,
      6,
      6,
      8,
      5,
      4,
      3,
      3,
      3,
      2,
      5,
      5
  };



  UCollator *credundant = NULL;
  UCollator *cresulting = NULL;
  UErrorCode status = U_ZERO_ERROR;
  UChar rlz[2048] = { 0 };
  uint32_t rlen = 0;

  for(i = 0; i<sizeof(rules)/sizeof(rules[0]); i++) {
    log_verbose("testing rule %s, expected to be %s\n", rules[i], expectedRules[i]);
    rlen = u_unescape(rules[i], rlz, 2048);

    credundant = ucol_openRules(rlz, rlen, UCOL_DEFAULT, UCOL_DEFAULT, NULL,&status);
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening collator\n");
      return;
    }

    rlen = u_unescape(expectedRules[i], rlz, 2048);
    cresulting = ucol_openRules(rlz, rlen, UCOL_DEFAULT, UCOL_DEFAULT, NULL,&status);

    testAgainstUCA(cresulting, credundant, "expected", TRUE, &status);

    ucol_close(credundant);
    ucol_close(cresulting);

    log_verbose("testing using data\n");

    genericRulesStarter(rules[i], testdata[i], testdatalen[i]);
  }

}

static void TestExpansionSyntax(void) {
  int32_t i;

  const static char *rules[] = {
    "&AE <<< a << b <<< c &d <<< f",
    "&AE <<< a <<< b << c << d < e < f <<< g",
    "&AE <<< B <<< C / D <<< F"
  };

  const static char *expectedRules[] = {
    "&A <<< a / E << b / E <<< c /E  &d <<< f",
    "&A <<< a / E <<< b / E << c / E << d / E < e < f <<< g",
    "&A <<< B / E <<< C / ED <<< F / E"
  };

  const static char *testdata[][8] = {
    {"AE", "a", "b", "c"},
    {"AE", "a", "b", "c", "d", "e", "f", "g"},
    {"AE", "B", "C"} /* / ED <<< F / E"},*/
  };

  const static uint32_t testdatalen[] = {
      4,
      8,
      3
  };



  UCollator *credundant = NULL;
  UCollator *cresulting = NULL;
  UErrorCode status = U_ZERO_ERROR;
  UChar rlz[2048] = { 0 };
  uint32_t rlen = 0;

  for(i = 0; i<sizeof(rules)/sizeof(rules[0]); i++) {
    log_verbose("testing rule %s, expected to be %s\n", rules[i], expectedRules[i]);
    rlen = u_unescape(rules[i], rlz, 2048);

    credundant = ucol_openRules(rlz, rlen, UCOL_DEFAULT, UCOL_DEFAULT, NULL, &status);
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening collator\n");
      return;
    }
    rlen = u_unescape(expectedRules[i], rlz, 2048);
    cresulting = ucol_openRules(rlz, rlen, UCOL_DEFAULT, UCOL_DEFAULT, NULL,&status);

    /* testAgainstUCA still doesn't handle expansions correctly, so this is not run */
    /* as a hard error test, but only in information mode */
    testAgainstUCA(cresulting, credundant, "expected", FALSE, &status);

    ucol_close(credundant);
    ucol_close(cresulting);

    log_verbose("testing using data\n");

    genericRulesStarter(rules[i], testdata[i], testdatalen[i]);
  }
}

static void TestCase(void)
{
    const static UChar gRules[MAX_TOKEN_LEN] =
    /*" & 0 < 1,\u2461<a,A"*/
    { 0x0026, 0x0030, 0x003C, 0x0031, 0x002C, 0x2460, 0x003C, 0x0061, 0x002C, 0x0041, 0x0000 };

    const static UChar testCase[][MAX_TOKEN_LEN] =
    {
        /*0*/ {0x0031 /*'1'*/, 0x0061/*'a'*/, 0x0000},
        /*1*/ {0x0031 /*'1'*/, 0x0041/*'A'*/, 0x0000},
        /*2*/ {0x2460 /*circ'1'*/, 0x0061/*'a'*/, 0x0000},
        /*3*/ {0x2460 /*circ'1'*/, 0x0041/*'A'*/, 0x0000}
    };

    const static UCollationResult caseTestResults[][9] =
    {
            { UCOL_LESS, UCOL_LESS, UCOL_LESS, 0, UCOL_LESS, UCOL_LESS, 0, 0, UCOL_LESS },
            { UCOL_GREATER, UCOL_LESS, UCOL_LESS, 0, UCOL_LESS, UCOL_LESS, 0, 0, UCOL_GREATER },
            { UCOL_LESS, UCOL_LESS, UCOL_LESS, 0, UCOL_GREATER, UCOL_LESS, 0, 0, UCOL_LESS },
            { UCOL_GREATER, UCOL_LESS, UCOL_GREATER, 0, UCOL_LESS, UCOL_LESS, 0, 0, UCOL_GREATER }

    };

    const static UColAttributeValue caseTestAttributes[][2] =
    {
            { UCOL_LOWER_FIRST, UCOL_OFF},
            { UCOL_UPPER_FIRST, UCOL_OFF},
            { UCOL_LOWER_FIRST, UCOL_ON},
            { UCOL_UPPER_FIRST, UCOL_ON}

    };
    int32_t i,j,k;
    UErrorCode status = U_ZERO_ERROR;
    UCollator  *myCollation;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing different case settings\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);

    for(k = 0; k<4; k++) {
      ucol_setAttribute(myCollation, UCOL_CASE_FIRST, caseTestAttributes[k][0], &status);
      ucol_setAttribute(myCollation, UCOL_CASE_LEVEL, caseTestAttributes[k][1], &status);
      log_verbose("Case first = %d, Case level = %d\n", caseTestAttributes[k][0], caseTestAttributes[k][1]);
      for (i = 0; i < 3 ; i++) {
        for(j = i+1; j<4; j++) {
          doTest(myCollation, testCase[i], testCase[j], caseTestResults[k][3*i+j-1]);
        }
      }
    }
    ucol_close(myCollation);

    myCollation = ucol_openRules(gRules, u_strlen(gRules), UCOL_OFF, UCOL_TERTIARY,NULL, &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing different case settings with custom rules\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);

    for(k = 0; k<4; k++) {
      ucol_setAttribute(myCollation, UCOL_CASE_FIRST, caseTestAttributes[k][0], &status);
      ucol_setAttribute(myCollation, UCOL_CASE_LEVEL, caseTestAttributes[k][1], &status);
      for (i = 0; i < 3 ; i++) {
        for(j = i+1; j<4; j++) {
          log_verbose("k:%d, i:%d, j:%d\n", k, i, j);
          doTest(myCollation, testCase[i], testCase[j], caseTestResults[k][3*i+j-1]);
        }
      }
    }
    ucol_close(myCollation);
    {
      const static char *lowerFirst[] = {
        "h",
        "H",
        "ch",
        "Ch",
        "CH",
        "cha",
        "chA",
        "Cha",
        "ChA",
        "CHa",
        "CHA",
        "i",
        "I"
      };

      const static char *upperFirst[] = {
        "H",
        "h",
        "CH",
        "Ch",
        "ch",
        "CHA",
        "CHa",
        "ChA",
        "Cha",
        "chA",
        "cha",
        "I",
        "i"
      };
      log_verbose("mixed case test\n");
      log_verbose("lower first, case level off\n");
      genericRulesStarter("[casefirst lower]&H<ch<<<Ch<<<CH", lowerFirst, sizeof(lowerFirst)/sizeof(lowerFirst[0]));
      log_verbose("upper first, case level off\n");
      genericRulesStarter("[casefirst upper]&H<ch<<<Ch<<<CH", upperFirst, sizeof(upperFirst)/sizeof(upperFirst[0]));
      log_verbose("lower first, case level on\n");
      genericRulesStarter("[casefirst lower][caselevel on]&H<ch<<<Ch<<<CH", lowerFirst, sizeof(lowerFirst)/sizeof(lowerFirst[0]));
      log_verbose("upper first, case level on\n");
      genericRulesStarter("[casefirst upper][caselevel on]&H<ch<<<Ch<<<CH", upperFirst, sizeof(upperFirst)/sizeof(upperFirst[0]));
    }

}

static void TestIncrementalNormalize(void) {

    /*UChar baseA     =0x61;*/
    UChar baseA     =0x41;
/*    UChar baseB     = 0x42;*/
    UChar ccMix[]   = {0x316, 0x321, 0x300};
    /*UChar ccMix[]   = {0x61, 0x61, 0x61};*/
    /*
        0x316 is combining grave accent below, cc=220
        0x321 is combining palatalized hook below, cc=202
        0x300 is combining grave accent, cc=230
    */

    /*int          maxSLen   = 2000;*/
    int          maxSLen   = 64000;
    int          sLen;
    int          i;

    UCollator        *coll;
    UErrorCode       status = U_ZERO_ERROR;
    UCollationResult result;

    int32_t myQ = QUICK;

    if(QUICK < 0) {
      QUICK = 1;
    }

    {
        /* Test 1.  Run very long unnormalized strings, to force overflow of*/
        /*          most buffers along the way.*/
        UChar            *strA;
        UChar            *strB;

        strA = malloc((maxSLen+1) * sizeof(UChar));
        strB = malloc((maxSLen+1) * sizeof(UChar));

        coll = ucol_open("en_US", &status);
        if(status == U_FILE_ACCESS_ERROR) {
          log_data_err("Is your data around?\n");
          return;
        } else if(U_FAILURE(status)) {
          log_err("Error opening collator\n");
          return;
        }
        ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);

        /*for (sLen = 257; sLen<maxSLen; sLen++) {*/
        /*for (sLen = 4; sLen<maxSLen; sLen++) {*/
        /*for (sLen = 1000; sLen<1001; sLen++) {*/
        for (sLen = 500; sLen<501; sLen++) {
        /*for (sLen = 40000; sLen<65000; sLen+=1000) {*/
            strA[0] = baseA;
            strB[0] = baseA;
            for (i=1; i<=sLen-1; i++) {
                strA[i] = ccMix[i % 3];
                strB[sLen-i] = ccMix[i % 3];
            }
            strA[sLen]   = 0;
            strB[sLen]   = 0;

            ucol_setStrength(coll, UCOL_TERTIARY);   /* Do test with default strength, which runs*/
            doTest(coll, strA, strB, UCOL_EQUAL);    /*   optimized functions in the impl*/
            ucol_setStrength(coll, UCOL_IDENTICAL);   /* Do again with the slow, general impl.*/
            doTest(coll, strA, strB, UCOL_EQUAL);
        }
        free(strA);
        free(strB);
    }

    QUICK = myQ;


    /*  Test 2:  Non-normal sequence in a string that extends to the last character*/
    /*         of the string.  Checks a couple of edge cases.*/

    {
        UChar strA[] = {0x41, 0x41, 0x300, 0x316, 0};
        UChar strB[] = {0x41, 0xc0, 0x316, 0};
        ucol_setStrength(coll, UCOL_TERTIARY);
        doTest(coll, strA, strB, UCOL_EQUAL);
    }

    /*  Test 3:  Non-normal sequence is terminated by a surrogate pair.*/

    {
      /* New UCA  3.1.1. 
       * test below used a code point from Desseret, which sorts differently 
       * than d800 dc00
       */
        /*UChar strA[] = {0x41, 0x41, 0x300, 0x316, 0xD801, 0xDC00, 0};*/
        UChar strA[] = {0x41, 0x41, 0x300, 0x316, 0xD800, 0xDC01, 0};
        UChar strB[] = {0x41, 0xc0, 0x316, 0xD800, 0xDC00, 0};
        ucol_setStrength(coll, UCOL_TERTIARY);
        doTest(coll, strA, strB, UCOL_GREATER);
    }

    /*  Test 4:  Imbedded nulls do not terminate a string when length is specified.*/

    {
        UChar strA[] = {0x41, 0x00, 0x42, 0x00};
        UChar strB[] = {0x41, 0x00, 0x00, 0x00};
        char  sortKeyA[50];
        char  sortKeyAz[50];
        char  sortKeyB[50];
        char  sortKeyBz[50];
        int   r;

        /* there used to be -3 here. Hmmmm.... */
        /*result = ucol_strcoll(coll, strA, -3, strB, -3);*/
        result = ucol_strcoll(coll, strA, 3, strB, 3);
        if (result != UCOL_GREATER) {
            log_err("ERROR 1 in test 4\n");
        }
        result = ucol_strcoll(coll, strA, -1, strB, -1);
        if (result != UCOL_EQUAL) {
            log_err("ERROR 2 in test 4\n");
        }

        ucol_getSortKey(coll, strA,  3, (uint8_t *)sortKeyA, sizeof(sortKeyA));
        ucol_getSortKey(coll, strA, -1, (uint8_t *)sortKeyAz, sizeof(sortKeyAz));
        ucol_getSortKey(coll, strB,  3, (uint8_t *)sortKeyB, sizeof(sortKeyB));
        ucol_getSortKey(coll, strB, -1, (uint8_t *)sortKeyBz, sizeof(sortKeyBz));

        r = strcmp(sortKeyA, sortKeyAz);
        if (r <= 0) {
            log_err("Error 3 in test 4\n");
        }
        r = strcmp(sortKeyA, sortKeyB);
        if (r <= 0) {
            log_err("Error 4 in test 4\n");
        }
        r = strcmp(sortKeyAz, sortKeyBz);
        if (r != 0) {
            log_err("Error 5 in test 4\n");
        }

        ucol_setStrength(coll, UCOL_IDENTICAL);
        ucol_getSortKey(coll, strA,  3, (uint8_t *)sortKeyA, sizeof(sortKeyA));
        ucol_getSortKey(coll, strA, -1, (uint8_t *)sortKeyAz, sizeof(sortKeyAz));
        ucol_getSortKey(coll, strB,  3, (uint8_t *)sortKeyB, sizeof(sortKeyB));
        ucol_getSortKey(coll, strB, -1, (uint8_t *)sortKeyBz, sizeof(sortKeyBz));

        r = strcmp(sortKeyA, sortKeyAz);
        if (r <= 0) {
            log_err("Error 6 in test 4\n");
        }
        r = strcmp(sortKeyA, sortKeyB);
        if (r <= 0) {
            log_err("Error 7 in test 4\n");
        }
        r = strcmp(sortKeyAz, sortKeyBz);
        if (r != 0) {
            log_err("Error 8 in test 4\n");
        }
        ucol_setStrength(coll, UCOL_TERTIARY);
    }


    /*  Test 5:  Null characters in non-normal source strings.*/

    {
        UChar strA[] = {0x41, 0x41, 0x300, 0x316, 0x00, 0x42, 0x00};
        UChar strB[] = {0x41, 0x41, 0x300, 0x316, 0x00, 0x00, 0x00};
        char  sortKeyA[50];
        char  sortKeyAz[50];
        char  sortKeyB[50];
        char  sortKeyBz[50];
        int   r;

        result = ucol_strcoll(coll, strA, 6, strB, 6);
        if (result != UCOL_GREATER) {
            log_err("ERROR 1 in test 5\n");
        }
        result = ucol_strcoll(coll, strA, -1, strB, -1);
        if (result != UCOL_EQUAL) {
            log_err("ERROR 2 in test 5\n");
        }

        ucol_getSortKey(coll, strA,  6, (uint8_t *)sortKeyA, sizeof(sortKeyA));
        ucol_getSortKey(coll, strA, -1, (uint8_t *)sortKeyAz, sizeof(sortKeyAz));
        ucol_getSortKey(coll, strB,  6, (uint8_t *)sortKeyB, sizeof(sortKeyB));
        ucol_getSortKey(coll, strB, -1, (uint8_t *)sortKeyBz, sizeof(sortKeyBz));

        r = strcmp(sortKeyA, sortKeyAz);
        if (r <= 0) {
            log_err("Error 3 in test 5\n");
        }
        r = strcmp(sortKeyA, sortKeyB);
        if (r <= 0) {
            log_err("Error 4 in test 5\n");
        }
        r = strcmp(sortKeyAz, sortKeyBz);
        if (r != 0) {
            log_err("Error 5 in test 5\n");
        }

        ucol_setStrength(coll, UCOL_IDENTICAL);
        ucol_getSortKey(coll, strA,  6, (uint8_t *)sortKeyA, sizeof(sortKeyA));
        ucol_getSortKey(coll, strA, -1, (uint8_t *)sortKeyAz, sizeof(sortKeyAz));
        ucol_getSortKey(coll, strB,  6, (uint8_t *)sortKeyB, sizeof(sortKeyB));
        ucol_getSortKey(coll, strB, -1, (uint8_t *)sortKeyBz, sizeof(sortKeyBz));

        r = strcmp(sortKeyA, sortKeyAz);
        if (r <= 0) {
            log_err("Error 6 in test 5\n");
        }
        r = strcmp(sortKeyA, sortKeyB);
        if (r <= 0) {
            log_err("Error 7 in test 5\n");
        }
        r = strcmp(sortKeyAz, sortKeyBz);
        if (r != 0) {
            log_err("Error 8 in test 5\n");
        }
        ucol_setStrength(coll, UCOL_TERTIARY);
    }


    /*  Test 6:  Null character as base of a non-normal combining sequence.*/

    {
        UChar strA[] = {0x41, 0x0, 0x300, 0x316, 0x41, 0x302, 0x00};
        UChar strB[] = {0x41, 0x0, 0x302, 0x316, 0x41, 0x300, 0x00};

        result = ucol_strcoll(coll, strA, 5, strB, 5);
        if (result != UCOL_LESS) {
            log_err("Error 1 in test 6\n");
        }
        result = ucol_strcoll(coll, strA, -1, strB, -1);
        if (result != UCOL_EQUAL) {
            log_err("Error 2 in test 6\n");
        }
    }

    ucol_close(coll);
}



#if 0
static void TestGetCaseBit(void) {
  static const char *caseBitData[] = {
    "a", "A", "ch", "Ch", "CH",
      "\\uFF9E", "\\u0009"
  };

  static const uint8_t results[] = {
    UCOL_LOWER_CASE, UCOL_UPPER_CASE, UCOL_LOWER_CASE, UCOL_MIXED_CASE, UCOL_UPPER_CASE,
      UCOL_UPPER_CASE, UCOL_LOWER_CASE
  };

  uint32_t i, blen = 0;
  UChar b[256] = {0};
  UErrorCode status = U_ZERO_ERROR;
  UCollator *UCA = ucol_open("", &status);
  uint8_t res = 0;

  for(i = 0; i<sizeof(results)/sizeof(results[0]); i++) {
    blen = u_unescape(caseBitData[i], b, 256);
    res = ucol_uprv_getCaseBits(UCA, b, blen, &status);
    if(results[i] != res) {
      log_err("Expected case = %02X, got %02X for %04X\n", results[i], res, b[0]);
    }
  }
}
#endif

static void TestHangulTailoring(void) {
    static const char *koreanData[] = {
        "\\uac00", "\\u4f3d", "\\u4f73", "\\u5047", "\\u50f9", "\\u52a0", "\\u53ef", "\\u5475",
            "\\u54e5", "\\u5609", "\\u5ac1", "\\u5bb6", "\\u6687", "\\u67b6", "\\u67b7", "\\u67ef",
            "\\u6b4c", "\\u73c2", "\\u75c2", "\\u7a3c", "\\u82db", "\\u8304", "\\u8857", "\\u8888",
            "\\u8a36", "\\u8cc8", "\\u8dcf", "\\u8efb", "\\u8fe6", "\\u99d5",
            "\\u4EEE", "\\u50A2", "\\u5496", "\\u54FF", "\\u5777", "\\u5B8A", "\\u659D", "\\u698E",
            "\\u6A9F", "\\u73C8", "\\u7B33", "\\u801E", "\\u8238", "\\u846D", "\\u8B0C"
    };

    const char *rules =
        "&\\uac00 <<< \\u4f3d <<< \\u4f73 <<< \\u5047 <<< \\u50f9 <<< \\u52a0 <<< \\u53ef <<< \\u5475 "
        "<<< \\u54e5 <<< \\u5609 <<< \\u5ac1 <<< \\u5bb6 <<< \\u6687 <<< \\u67b6 <<< \\u67b7 <<< \\u67ef "
        "<<< \\u6b4c <<< \\u73c2 <<< \\u75c2 <<< \\u7a3c <<< \\u82db <<< \\u8304 <<< \\u8857 <<< \\u8888 "
        "<<< \\u8a36 <<< \\u8cc8 <<< \\u8dcf <<< \\u8efb <<< \\u8fe6 <<< \\u99d5 "
        "<<< \\u4EEE <<< \\u50A2 <<< \\u5496 <<< \\u54FF <<< \\u5777 <<< \\u5B8A <<< \\u659D <<< \\u698E "
        "<<< \\u6A9F <<< \\u73C8 <<< \\u7B33 <<< \\u801E <<< \\u8238 <<< \\u846D <<< \\u8B0C";


  UErrorCode status = U_ZERO_ERROR;
  UChar rlz[2048] = { 0 };
  uint32_t rlen = u_unescape(rules, rlz, 2048);

  UCollator *coll = ucol_openRules(rlz, rlen, UCOL_DEFAULT, UCOL_DEFAULT,NULL, &status);
  if(status == U_FILE_ACCESS_ERROR) {
    log_data_err("Is your data around?\n");
    return;
  } else if(U_FAILURE(status)) {
    log_err("Error opening collator\n");
    return;
  }

  log_verbose("Using start of korean rules\n");

  if(U_SUCCESS(status)) {
    genericOrderingTest(coll, koreanData, sizeof(koreanData)/sizeof(koreanData[0]));
  } else {
    log_err("Unable to open collator with rules %s\n", rules);
  }

  log_verbose("Setting jamoSpecial to TRUE and testing once more\n");
  ((UCATableHeader *)coll->image)->jamoSpecial = TRUE; /* don't try this at home  */
  genericOrderingTest(coll, koreanData, sizeof(koreanData)/sizeof(koreanData[0]));

  ucol_close(coll);

  log_verbose("Using ko__LOTUS locale\n");
  genericLocaleStarter("ko__LOTUS", koreanData, sizeof(koreanData)/sizeof(koreanData[0]));
}

static void TestCompressOverlap(void) {
    UChar       secstr[150];
    UChar       tertstr[150];
    UErrorCode  status = U_ZERO_ERROR;
    UCollator  *coll;
    char        result[200];
    uint32_t    resultlen;
    int         count = 0;
    char       *tempptr;

    coll = ucol_open("", &status);

    if (U_FAILURE(status)) {
        log_err("Collator can't be created\n");
        return;
    }
    while (count < 149) {
        secstr[count] = 0x0020; /* [06, 05, 05] */
        tertstr[count] = 0x0020;
        count ++;
    }

    /* top down compression ----------------------------------- */
    secstr[count] = 0x0332; /* [, 87, 05] */
    tertstr[count] = 0x3000; /* [06, 05, 07] */

    /* no compression secstr should have 150 secondary bytes, tertstr should
    have 150 tertiary bytes.
    with correct overlapping compression, secstr should have 4 secondary
    bytes, tertstr should have > 2 tertiary bytes */
    resultlen = ucol_getSortKey(coll, secstr, 150, (uint8_t *)result, 250);
    tempptr = uprv_strchr(result, 1) + 1;
    while (*(tempptr + 1) != 1) {
        /* the last secondary collation element is not checked since it is not
        part of the compression */
        if (*tempptr < UCOL_COMMON_TOP2 - UCOL_TOP_COUNT2) {
            log_err("Secondary compression overlapped\n");
        }
        tempptr ++;
    }

    /* tertiary top/bottom/common for en_US is similar to the secondary
    top/bottom/common */
    resultlen = ucol_getSortKey(coll, tertstr, 150, (uint8_t *)result, 250);
    tempptr = uprv_strrchr(result, 1) + 1;
    while (*(tempptr + 1) != 0) {
        /* the last secondary collation element is not checked since it is not
        part of the compression */
        if (*tempptr < coll->tertiaryTop - coll->tertiaryTopCount) {
            log_err("Tertiary compression overlapped\n");
        }
        tempptr ++;
    }

    /* bottom up compression ------------------------------------- */
    secstr[count] = 0;
    tertstr[count] = 0;
    resultlen = ucol_getSortKey(coll, secstr, 150, (uint8_t *)result, 250);
    tempptr = uprv_strchr(result, 1) + 1;
    while (*(tempptr + 1) != 1) {
        /* the last secondary collation element is not checked since it is not
        part of the compression */
        if (*tempptr > UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2) {
            log_err("Secondary compression overlapped\n");
        }
        tempptr ++;
    }

    /* tertiary top/bottom/common for en_US is similar to the secondary
    top/bottom/common */
    resultlen = ucol_getSortKey(coll, tertstr, 150, (uint8_t *)result, 250);
    tempptr = uprv_strrchr(result, 1) + 1;
    while (*(tempptr + 1) != 0) {
        /* the last secondary collation element is not checked since it is not
        part of the compression */
        if (*tempptr > coll->tertiaryBottom + coll->tertiaryBottomCount) {
            log_err("Tertiary compression overlapped\n");
        }
        tempptr ++;
    }

    ucol_close(coll);
}

static void TestCyrillicTailoring(void) {
  static const char *test[] = {
    "\\u0410b",
      "\\u0410\\u0306a",
      "\\u04d0A"
  };

    /* Russian overrides contractions, so this test is not valid anymore */
    /*genericLocaleStarter("ru", test, 3);*/ 

    genericLocaleStarter("root", test, 3);
    genericRulesStarter("&\\u0410 = \\u0410", test, 3);
    genericRulesStarter("&Z < \\u0410", test, 3);
    genericRulesStarter("&\\u0410 = \\u0410 < \\u04d0", test, 3);
    genericRulesStarter("&Z < \\u0410 < \\u04d0", test, 3);
    genericRulesStarter("&\\u0410 = \\u0410 < \\u0410\\u0301", test, 3);
    genericRulesStarter("&Z < \\u0410 < \\u0410\\u0301", test, 3);
}

static void TestSuppressContractions(void) {

  static const char *testNoCont2[] = {
      "\\u0410\\u0302a",
      "\\u0410\\u0306b",
      "\\u0410c"            
  };
  static const char *testNoCont[] = {
      "a\\u0410",            
      "A\\u0410\\u0306",
      "\\uFF21\\u0410\\u0302"
  };
    
  genericRulesStarter("[suppressContractions [\\u0400-\\u047f]]", testNoCont, 3);
  genericRulesStarter("[suppressContractions [\\u0400-\\u047f]]", testNoCont2, 3);
}

static void TestContraction(void) {
    const static char *testrules[] = {
        "&A = AB / B",
        "&A = A\\u0306/\\u0306",
        "&c = ch / h"
    };
    const static UChar testdata[][2] = {
        {0x0041 /* 'A' */, 0x0042 /* 'B' */},
        {0x0041 /* 'A' */, 0x0306 /* combining breve */},
        {0x0063 /* 'c' */, 0x0068 /* 'h' */}
    };
    const static UChar testdata2[][2] = {
        {0x0063 /* 'c' */, 0x0067 /* 'g' */},
        {0x0063 /* 'c' */, 0x0068 /* 'h' */},
        {0x0063 /* 'c' */, 0x006C /* 'l' */}
    };
    const static char *testrules3[] = {
        "&z < xyz &xyzw << B",
        "&z < xyz &xyz << B / w",
        "&z < ch &achm << B",
        "&z < ch &a << B / chm",
        "&\\ud800\\udc00w << B",
        "&\\ud800\\udc00 << B / w",
        "&a\\ud800\\udc00m << B",
        "&a << B / \\ud800\\udc00m",
    };

    UErrorCode  status   = U_ZERO_ERROR;
    UCollator  *coll;
    UChar       rule[256] = {0};
    uint32_t    rlen     = 0;
    int         i;

    for (i = 0; i < sizeof(testrules) / sizeof(testrules[0]); i ++) {
        UCollationElements *iter1;
        int j = 0;
        log_verbose("Rule %s for testing\n", testrules[i]);
        rlen = u_unescape(testrules[i], rule, 32);
        coll = ucol_openRules(rule, rlen, UCOL_ON, UCOL_TERTIARY,NULL, &status);
        if (U_FAILURE(status)) {
            log_err("Collator creation failed %s\n", testrules[i]);
            return;
        }
        iter1 = ucol_openElements(coll, testdata[i], 2, &status);
        if (U_FAILURE(status)) {
            log_err("Collation iterator creation failed\n");
            return;
        }
        while (j < 2) {
            UCollationElements *iter2 = ucol_openElements(coll,
                                                         &(testdata[i][j]),
                                                         1, &status);
            uint32_t ce;
            if (U_FAILURE(status)) {
                log_err("Collation iterator creation failed\n");
                return;
            }
            ce = ucol_next(iter2, &status);
            while (ce != UCOL_NULLORDER) {
                if ((uint32_t)ucol_next(iter1, &status) != ce) {
                    log_err("Collation elements in contraction split does not match\n");
                    return;
                }
                ce = ucol_next(iter2, &status);
            }
            j ++;
            ucol_closeElements(iter2);
        }
        if (ucol_next(iter1, &status) != UCOL_NULLORDER) {
            log_err("Collation elements not exhausted\n");
            return;
        }
        ucol_closeElements(iter1);
        ucol_close(coll);
    }

    rlen = u_unescape("& a < b < c < ch < d & c = ch / h", rule, 256);
    coll = ucol_openRules(rule, rlen, UCOL_ON, UCOL_TERTIARY,NULL, &status);
    if (ucol_strcoll(coll, testdata2[0], 2, testdata2[1], 2) != UCOL_LESS) {
        log_err("Expected \\u%04x\\u%04x < \\u%04x\\u%04x\n",
                testdata2[0][0], testdata2[0][1], testdata2[1][0],
                testdata2[1][1]);
        return;
    }
    if (ucol_strcoll(coll, testdata2[1], 2, testdata2[2], 2) != UCOL_LESS) {
        log_err("Expected \\u%04x\\u%04x < \\u%04x\\u%04x\n",
                testdata2[1][0], testdata2[1][1], testdata2[2][0],
                testdata2[2][1]);
        return;
    }
    ucol_close(coll);

    for (i = 0; i < sizeof(testrules3) / sizeof(testrules3[0]); i += 2) {
        UCollator          *coll1,
                           *coll2;
        UCollationElements *iter1,
                           *iter2;
        UChar               ch = 0x0042 /* 'B' */;
        uint32_t            ce;
        rlen = u_unescape(testrules3[i], rule, 32);
        coll1 = ucol_openRules(rule, rlen, UCOL_ON, UCOL_TERTIARY,NULL, &status);
        rlen = u_unescape(testrules3[i + 1], rule, 32);
        coll2 = ucol_openRules(rule, rlen, UCOL_ON, UCOL_TERTIARY,NULL, &status);
        if (U_FAILURE(status)) {
            log_err("Collator creation failed %s\n", testrules[i]);
            return;
        }
        iter1 = ucol_openElements(coll1, &ch, 1, &status);
        iter2 = ucol_openElements(coll2, &ch, 1, &status);
        if (U_FAILURE(status)) {
            log_err("Collation iterator creation failed\n");
            return;
        }
        ce = ucol_next(iter1, &status);
        if (U_FAILURE(status)) {
            log_err("Retrieving ces failed\n");
            return;
        }
        while (ce != UCOL_NULLORDER) {
            if (ce != (uint32_t)ucol_next(iter2, &status)) {
                log_err("CEs does not match\n");
                return;
            }
            ce = ucol_next(iter1, &status);
            if (U_FAILURE(status)) {
                log_err("Retrieving ces failed\n");
                return;
            }
        }
        if (ucol_next(iter2, &status) != UCOL_NULLORDER) {
            log_err("CEs not exhausted\n");
            return;
        }
        ucol_closeElements(iter1);
        ucol_closeElements(iter2);
        ucol_close(coll1);
        ucol_close(coll2);
    }
}

static void TestExpansion(void) {
    const static char *testrules[] = {
        "&J << K / B & K << M",
        "&J << K / B << M"
    };
    const static UChar testdata[][3] = {
        {0x004A /*'J'*/, 0x0041 /*'A'*/, 0},
        {0x004D /*'M'*/, 0x0041 /*'A'*/, 0},
        {0x004B /*'K'*/, 0x0041 /*'A'*/, 0},
        {0x004B /*'K'*/, 0x0043 /*'C'*/, 0},
        {0x004A /*'J'*/, 0x0043 /*'C'*/, 0},
        {0x004D /*'M'*/, 0x0043 /*'C'*/, 0}
    };

    UErrorCode  status   = U_ZERO_ERROR;
    UCollator  *coll;
    UChar       rule[256] = {0};
    uint32_t    rlen     = 0;
    int         i;

    for (i = 0; i < sizeof(testrules) / sizeof(testrules[0]); i ++) {
        int j = 0;
        log_verbose("Rule %s for testing\n", testrules[i]);
        rlen = u_unescape(testrules[i], rule, 32);
        coll = ucol_openRules(rule, rlen, UCOL_ON, UCOL_TERTIARY,NULL, &status);
        if (U_FAILURE(status)) {
            log_err("Collator creation failed %s\n", testrules[i]);
            return;
        }

        for (j = 0; j < 5; j ++) {
            doTest(coll, testdata[j], testdata[j + 1], UCOL_LESS);
        }
        ucol_close(coll);
    }
}

#if 0
/* this test tests the current limitations of the engine */
/* it always fail, so it is disabled by default */
static void TestLimitations(void) {
  /* recursive expansions */
  {
    static const char *rule = "&a=b/c&d=c/e";
    static const char *tlimit01[] = {"add","b","adf"};
    static const char *tlimit02[] = {"aa","b","af"};
    log_verbose("recursive expansions\n");
    genericRulesStarter(rule, tlimit01, sizeof(tlimit01)/sizeof(tlimit01[0]));
    genericRulesStarter(rule, tlimit02, sizeof(tlimit02)/sizeof(tlimit02[0]));
  }
  /* contractions spanning expansions */
  {
    static const char *rule = "&a<<<c/e&g<<<eh";
    static const char *tlimit01[] = {"ad","c","af","f","ch","h"};
    static const char *tlimit02[] = {"ad","c","ch","af","f","h"};
    log_verbose("contractions spanning expansions\n");
    genericRulesStarter(rule, tlimit01, sizeof(tlimit01)/sizeof(tlimit01[0]));
    genericRulesStarter(rule, tlimit02, sizeof(tlimit02)/sizeof(tlimit02[0]));
  }
  /* normalization: nulls in contractions */
  {
    static const char *rule = "&a<<<\\u0000\\u0302";
    static const char *tlimit01[] = {"a","\\u0000\\u0302\\u0327"};
    static const char *tlimit02[] = {"\\u0000\\u0302\\u0327","a"};
    static const UColAttribute att[] = { UCOL_DECOMPOSITION_MODE };
    static const UColAttributeValue valOn[] = { UCOL_ON };
    static const UColAttributeValue valOff[] = { UCOL_OFF };

    log_verbose("NULL in contractions\n");
    genericRulesStarterWithOptions(rule, tlimit01, 2, att, valOn, 1);
    genericRulesStarterWithOptions(rule, tlimit02, 2, att, valOn, 1);
    genericRulesStarterWithOptions(rule, tlimit01, 2, att, valOff, 1);
    genericRulesStarterWithOptions(rule, tlimit02, 2, att, valOff, 1);

  }
  /* normalization: contractions spanning normalization */
  {
    static const char *rule = "&a<<<\\u0000\\u0302";
    static const char *tlimit01[] = {"a","\\u0000\\u0302\\u0327"};
    static const char *tlimit02[] = {"\\u0000\\u0302\\u0327","a"};
    static const UColAttribute att[] = { UCOL_DECOMPOSITION_MODE };
    static const UColAttributeValue valOn[] = { UCOL_ON };
    static const UColAttributeValue valOff[] = { UCOL_OFF };

    log_verbose("contractions spanning normalization\n");
    genericRulesStarterWithOptions(rule, tlimit01, 2, att, valOn, 1);
    genericRulesStarterWithOptions(rule, tlimit02, 2, att, valOn, 1);
    genericRulesStarterWithOptions(rule, tlimit01, 2, att, valOff, 1);
    genericRulesStarterWithOptions(rule, tlimit02, 2, att, valOff, 1);

  }
  /* variable top:  */
  {
    /*static const char *rule2 = "&\\u2010<x=[variable top]<z";*/
    static const char *rule = "&\\u2010<x<[variable top]=z";
    /*static const char *rule3 = "&' '<x<[variable top]=z";*/
    static const char *tlimit01[] = {" ", "z", "zb", "a", " b", "xb", "b", "c" };
    static const char *tlimit02[] = {"-", "-x", "x","xb", "-z", "z", "zb", "-a", "a", "-b", "b", "c"};
    static const char *tlimit03[] = {" ", "xb", "z", "zb", "a", " b", "b", "c" };
    static const UColAttribute att[] = { UCOL_ALTERNATE_HANDLING, UCOL_STRENGTH };
    static const UColAttributeValue valOn[] = { UCOL_SHIFTED, UCOL_QUATERNARY };
    static const UColAttributeValue valOff[] = { UCOL_NON_IGNORABLE, UCOL_TERTIARY };

    log_verbose("variable top\n");
    genericRulesStarterWithOptions(rule, tlimit03, sizeof(tlimit03)/sizeof(tlimit03[0]), att, valOn, sizeof(att)/sizeof(att[0]));
    genericRulesStarterWithOptions(rule, tlimit01, sizeof(tlimit01)/sizeof(tlimit01[0]), att, valOn, sizeof(att)/sizeof(att[0]));
    genericRulesStarterWithOptions(rule, tlimit02, sizeof(tlimit02)/sizeof(tlimit02[0]), att, valOn, sizeof(att)/sizeof(att[0]));
    genericRulesStarterWithOptions(rule, tlimit01, sizeof(tlimit01)/sizeof(tlimit01[0]), att, valOff, sizeof(att)/sizeof(att[0]));
    genericRulesStarterWithOptions(rule, tlimit02, sizeof(tlimit02)/sizeof(tlimit02[0]), att, valOff, sizeof(att)/sizeof(att[0]));

  }
  /* case level */
  {
    static const char *rule = "&c<ch<<<cH<<<Ch<<<CH";
    static const char *tlimit01[] = {"c","CH","Ch","cH","ch"};
    static const char *tlimit02[] = {"c","CH","cH","Ch","ch"};
    static const UColAttribute att[] = { UCOL_CASE_FIRST};
    static const UColAttributeValue valOn[] = { UCOL_UPPER_FIRST};
    /*static const UColAttributeValue valOff[] = { UCOL_OFF};*/
    log_verbose("case level\n");
    genericRulesStarterWithOptions(rule, tlimit01, sizeof(tlimit01)/sizeof(tlimit01[0]), att, valOn, sizeof(att)/sizeof(att[0]));
    genericRulesStarterWithOptions(rule, tlimit02, sizeof(tlimit02)/sizeof(tlimit02[0]), att, valOn, sizeof(att)/sizeof(att[0]));
    /*genericRulesStarterWithOptions(rule, tlimit01, sizeof(tlimit01)/sizeof(tlimit01[0]), att, valOff, sizeof(att)/sizeof(att[0]));*/
    /*genericRulesStarterWithOptions(rule, tlimit02, sizeof(tlimit02)/sizeof(tlimit02[0]), att, valOff, sizeof(att)/sizeof(att[0]));*/
  }

}
#endif

static void TestBocsuCoverage(void) {
  UErrorCode status = U_ZERO_ERROR;
  const char *testString = "\\u0041\\u0441\\u4441\\U00044441\\u4441\\u0441\\u0041";
  UChar       test[256] = {0};
  uint32_t    tlen     = u_unescape(testString, test, 32);
  uint8_t key[256]     = {0};
  uint32_t klen         = 0;

  UCollator *coll = ucol_open("", &status);
  if(U_SUCCESS(status)) {
  ucol_setAttribute(coll, UCOL_STRENGTH, UCOL_IDENTICAL, &status);

  klen = ucol_getSortKey(coll, test, tlen, key, 256);

  ucol_close(coll);
  } else {
    log_data_err("Couldn't open UCA\n");
  }
}

static void TestVariableTopSetting(void) {
  UErrorCode status = U_ZERO_ERROR;
  const UChar *current = NULL;
  uint32_t varTopOriginal = 0, varTop1, varTop2;
  UCollator *coll = ucol_open("", &status);
  if(U_SUCCESS(status)) {

  uint32_t strength = 0;
  uint16_t specs = 0;
  uint32_t chOffset = 0;
  uint32_t chLen = 0;
  uint32_t exOffset = 0;
  uint32_t exLen = 0;
  uint32_t oldChOffset = 0;
  uint32_t oldChLen = 0;
  uint32_t oldExOffset = 0;
  uint32_t oldExLen = 0;
  uint32_t prefixOffset = 0;
  uint32_t prefixLen = 0;

  UBool startOfRules = TRUE;
  UColTokenParser src;
  UColOptionSet opts;

  UChar *rulesCopy = NULL;
  uint32_t rulesLen;

  UCollationResult result;

  UChar first[256] = { 0 };
  UChar second[256] = { 0 };
  UParseError parseError;
  int32_t myQ = QUICK;

  src.opts = &opts;

  if(QUICK <= 0) {
    QUICK = 1;
  }

  /* this test will fail when normalization is turned on */
  /* therefore we always turn off exhaustive mode for it */
  if(1) { /* QUICK > 0*/ 
    log_verbose("Slide variable top over UCARules\n");
    rulesLen = ucol_getRulesEx(coll, UCOL_FULL_RULES, rulesCopy, 0);
    rulesCopy = (UChar *)malloc((rulesLen+UCOL_TOK_EXTRA_RULE_SPACE_SIZE)*sizeof(UChar));
    rulesLen = ucol_getRulesEx(coll, UCOL_FULL_RULES, rulesCopy, rulesLen+UCOL_TOK_EXTRA_RULE_SPACE_SIZE);

    if(U_SUCCESS(status) && rulesLen > 0) {
      ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
      src.current = src.source = rulesCopy;
      src.end = rulesCopy+rulesLen;
      src.extraCurrent = src.end;
      src.extraEnd = src.end+UCOL_TOK_EXTRA_RULE_SPACE_SIZE; 

      while ((current = ucol_tok_parseNextToken(&src, startOfRules, &parseError,&status)) != NULL) {
        strength = src.parsedToken.strength;
        chOffset = src.parsedToken.charsOffset;
        chLen = src.parsedToken.charsLen;
        exOffset = src.parsedToken.extensionOffset;
        exLen = src.parsedToken.extensionLen;
        prefixOffset = src.parsedToken.prefixOffset;
        prefixLen = src.parsedToken.prefixLen;
        specs = src.parsedToken.flags;

        startOfRules = FALSE;
        if(0) {
          log_verbose("%04X %d ", *(rulesCopy+chOffset), chLen);
        }
        if(strength == UCOL_PRIMARY) {
          status = U_ZERO_ERROR;
          varTopOriginal = ucol_getVariableTop(coll, &status);
          varTop1 = ucol_setVariableTop(coll, rulesCopy+oldChOffset, oldChLen, &status);
          if(U_FAILURE(status)) {
            char buffer[256];
            char *buf = buffer;
            uint32_t i = 0, j;
            uint32_t CE = UCOL_NO_MORE_CES;

            /* before we start screaming, let's see if there is a problem with the rules */
            collIterate s;
            uprv_init_collIterate(coll, rulesCopy+oldChOffset, oldChLen, &s);

            CE = ucol_getNextCE(coll, &s, &status);

            for(i = 0; i < oldChLen; i++) {
              j = sprintf(buf, "%04X ", *(rulesCopy+oldChOffset+i));
              buf += j;
            }
            if(status == U_PRIMARY_TOO_LONG_ERROR) {
              log_verbose("= Expected failure for %s =", buffer);
            } else {
              if(s.pos == s.endp) {
                log_err("Unexpected failure setting variable top at offset %d. Error %s. Codepoints: %s\n", 
                  oldChOffset, u_errorName(status), buffer);
              } else {
                log_verbose("There is a goofy contraction in UCA rules that does not appear in the fractional UCA. Codepoints: %s\n", 
                  buffer);
              }
            }
          }
          varTop2 = ucol_getVariableTop(coll, &status);
          if((varTop1 & 0xFFFF0000) != (varTop2 & 0xFFFF0000)) {
            log_err("cannot retrieve set varTop value!\n");
            continue;
          }

          if((varTop1 & 0xFFFF0000) > 0 && oldExLen == 0) {

            u_strncpy(first, rulesCopy+oldChOffset, oldChLen);
            u_strncpy(first+oldChLen, rulesCopy+chOffset, chLen);
            u_strncpy(first+oldChLen+chLen, rulesCopy+oldChOffset, oldChLen);
            first[2*oldChLen+chLen] = 0;

            if(oldExLen == 0) {
              u_strncpy(second, rulesCopy+chOffset, chLen);
              second[chLen] = 0;
            } else { /* This is skipped momentarily, but should work once UCARules are fully UCA conformant */
              u_strncpy(second, rulesCopy+oldExOffset, oldExLen);
              u_strncpy(second+oldChLen, rulesCopy+chOffset, chLen);
              u_strncpy(second+oldChLen+chLen, rulesCopy+oldExOffset, oldExLen);
              second[2*oldExLen+chLen] = 0;
            }
            result = ucol_strcoll(coll, first, -1, second, -1);
            if(result == UCOL_EQUAL) {
              doTest(coll, first, second, UCOL_EQUAL);
            } else {
              log_verbose("Suspicious strcoll result for %04X and %04X\n", *(rulesCopy+oldChOffset), *(rulesCopy+chOffset));
            }
          }
        }
        if(strength != UCOL_TOK_RESET) {
          oldChOffset = chOffset;
          oldChLen = chLen;
          oldExOffset = exOffset;
          oldExLen = exLen;
        }
      }
      status = U_ZERO_ERROR;
    }
    else {
      log_err("Unexpected failure getting rules %s\n", u_errorName(status));
      return;
    }
    if (U_FAILURE(status)) {
        log_err("Error parsing rules %s\n", u_errorName(status));
        return;
    }
    status = U_ZERO_ERROR;
  }

  QUICK = myQ;

  log_verbose("Testing setting variable top to contractions\n");
  {
    /* uint32_t tailoredCE = UCOL_NOT_FOUND; */
    /*UChar *conts = (UChar *)((uint8_t *)coll->image + coll->image->UCAConsts+sizeof(UCAConstants));*/
    UChar *conts = (UChar *)((uint8_t *)coll->image + coll->image->contractionUCACombos);
    while(*conts != 0) {
      if(*(conts+2) == 0) {
        varTop1 = ucol_setVariableTop(coll, conts, -1, &status);
      } else {
        varTop1 = ucol_setVariableTop(coll, conts, 3, &status);
      }
      if(U_FAILURE(status)) {
        log_err("Couldn't set variable top to a contraction %04X %04X %04X\n",
          *conts, *(conts+1), *(conts+2));
        status = U_ZERO_ERROR;
      }
      conts+=3;
    }

    status = U_ZERO_ERROR;

    first[0] = 0x0040;
    first[1] = 0x0050;
    first[2] = 0x0000;

    ucol_setVariableTop(coll, first, -1, &status);

    if(U_SUCCESS(status)) {
      log_err("Invalid contraction succeded in setting variable top!\n");
    }

  }

  log_verbose("Test restoring variable top\n");

  status = U_ZERO_ERROR;
  ucol_restoreVariableTop(coll, varTopOriginal, &status);
  if(varTopOriginal != ucol_getVariableTop(coll, &status)) {
    log_err("Couldn't restore old variable top\n");
  }

  log_verbose("Testing calling with error set\n");

  status = U_INTERNAL_PROGRAM_ERROR;
  varTop1 = ucol_setVariableTop(coll, first, 1, &status);
  varTop2 = ucol_getVariableTop(coll, &status);
  ucol_restoreVariableTop(coll, varTop2, &status);
  varTop1 = ucol_setVariableTop(NULL, first, 1, &status);
  varTop2 = ucol_getVariableTop(NULL, &status);
  ucol_restoreVariableTop(NULL, varTop2, &status);
  if(status != U_INTERNAL_PROGRAM_ERROR) {
    log_err("Bad reaction to passed error!\n");
  }
  free(rulesCopy);
  ucol_close(coll);
  } else {
    log_data_err("Couldn't open UCA collator\n");
  }

}

static void TestNonChars(void) {
  static const char *test[] = {
    "\\u0000",
    "\\uFFFE", "\\uFFFF",
      "\\U0001FFFE", "\\U0001FFFF",
      "\\U0002FFFE", "\\U0002FFFF",
      "\\U0003FFFE", "\\U0003FFFF",
      "\\U0004FFFE", "\\U0004FFFF",
      "\\U0005FFFE", "\\U0005FFFF",
      "\\U0006FFFE", "\\U0006FFFF",
      "\\U0007FFFE", "\\U0007FFFF",
      "\\U0008FFFE", "\\U0008FFFF",
      "\\U0009FFFE", "\\U0009FFFF",
      "\\U000AFFFE", "\\U000AFFFF",
      "\\U000BFFFE", "\\U000BFFFF",
      "\\U000CFFFE", "\\U000CFFFF",
      "\\U000DFFFE", "\\U000DFFFF",
      "\\U000EFFFE", "\\U000EFFFF",
      "\\U000FFFFE", "\\U000FFFFF",
      "\\U0010FFFE", "\\U0010FFFF"
  };
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open("en_US", &status);

  log_verbose("Test non characters\n");

  if(U_SUCCESS(status)) {
    genericOrderingTestWithResult(coll, test, 35, UCOL_EQUAL);
  } else {
    log_err("Unable to open collator\n");
  }

  ucol_close(coll);
}

static void TestExtremeCompression(void) {
  static char *test[4];
  int32_t j = 0, i = 0;

  for(i = 0; i<4; i++) {
    test[i] = (char *)malloc(2048*sizeof(char));
  }

  for(j = 20; j < 500; j++) {
    for(i = 0; i<4; i++) {
      uprv_memset(test[i], 'a', (j-1)*sizeof(char));
      test[i][j-1] = (char)('a'+i);
      test[i][j] = 0;
    }
    genericLocaleStarter("en_US", (const char **)test, 4);
  }


  for(i = 0; i<4; i++) {
    free(test[i]);
  }
}

#if 0
static void TestExtremeCompression(void) {
  static char *test[4];
  int32_t j = 0, i = 0;
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open("en_US", status);
  for(i = 0; i<4; i++) {
    test[i] = (char *)malloc(2048*sizeof(char));
  }
  for(j = 10; j < 2048; j++) {
    for(i = 0; i<4; i++) {
      uprv_memset(test[i], 'a', (j-2)*sizeof(char));
      test[i][j-1] = (char)('a'+i);
      test[i][j] = 0;
    }
  }
  genericLocaleStarter("en_US", (const char **)test, 4);

  for(j = 10; j < 2048; j++) {
    for(i = 0; i<1; i++) {
      uprv_memset(test[i], 'a', (j-1)*sizeof(char));
      test[i][j] = 0;
    }
  }
  for(i = 0; i<4; i++) {
    free(test[i]);
  }
}
#endif

static void TestSurrogates(void) {
  static const char *test[] = {
    "z","\\ud900\\udc25",  "\\ud805\\udc50",
       "\\ud800\\udc00y",  "\\ud800\\udc00r",
       "\\ud800\\udc00f",  "\\ud800\\udc00",
       "\\ud800\\udc00c", "\\ud800\\udc00b",
       "\\ud800\\udc00fa", "\\ud800\\udc00fb",
       "\\ud800\\udc00a",
       "c", "b"
  };

  static const char *rule =
    "&z < \\ud900\\udc25   < \\ud805\\udc50"
       "< \\ud800\\udc00y  < \\ud800\\udc00r"
       "< \\ud800\\udc00f  << \\ud800\\udc00"
       "< \\ud800\\udc00fa << \\ud800\\udc00fb"
       "< \\ud800\\udc00a  < c < b" ;

  genericRulesStarter(rule, test, 14);
}

/* This is a test for prefix implementation, used by JIS X 4061 collation rules */
static void TestPrefix(void) {
  uint32_t i;

  static struct {
    const char *rules;
    const char *data[50];
    const uint32_t len;
  } tests[] = {  
    { "&z <<< z|a", 
      {"zz", "za"}, 2 },

    { "&z <<< z|   a", 
      {"zz", "za"}, 2 },
    { "[strength I]"
      "&a=\\ud900\\udc25"
      "&z<<<\\ud900\\udc25|a", 
      {"aa", "az", "\\ud900\\udc25z", "\\ud900\\udc25a", "zz"}, 4 },
  };


  for(i = 0; i<(sizeof(tests)/sizeof(tests[0])); i++) {
    genericRulesStarter(tests[i].rules, tests[i].data, tests[i].len);
  }
}

/* This test uses data suplied by Masashiko Maedera to test the implementation */
/* JIS X 4061 collation order implementation                                   */
static void TestNewJapanese(void) {

  static const char *test1[] = {
      "\\u30b7\\u30e3\\u30fc\\u30ec",
      "\\u30b7\\u30e3\\u30a4",
      "\\u30b7\\u30e4\\u30a3",
      "\\u30b7\\u30e3\\u30ec",
      "\\u3061\\u3087\\u3053",
      "\\u3061\\u3088\\u3053",
      "\\u30c1\\u30e7\\u30b3\\u30ec\\u30fc\\u30c8",
      "\\u3066\\u30fc\\u305f",
      "\\u30c6\\u30fc\\u30bf",
      "\\u30c6\\u30a7\\u30bf",
      "\\u3066\\u3048\\u305f",
      "\\u3067\\u30fc\\u305f", 
      "\\u30c7\\u30fc\\u30bf",
      "\\u30c7\\u30a7\\u30bf",
      "\\u3067\\u3048\\u305f",
      "\\u3066\\u30fc\\u305f\\u30fc",
      "\\u30c6\\u30fc\\u30bf\\u30a1",
      "\\u30c6\\u30a7\\u30bf\\u30fc",
      "\\u3066\\u3047\\u305f\\u3041",
      "\\u3066\\u3048\\u305f\\u30fc",
      "\\u3067\\u30fc\\u305f\\u30fc",
      "\\u30c7\\u30fc\\u30bf\\u30a1",
      "\\u3067\\u30a7\\u305f\\u30a1",
      "\\u30c7\\u3047\\u30bf\\u3041",
      "\\u30c7\\u30a8\\u30bf\\u30a2",
      "\\u3072\\u3086",
      "\\u3073\\u3085\\u3042",
      "\\u3074\\u3085\\u3042",
      "\\u3073\\u3085\\u3042\\u30fc",
      "\\u30d3\\u30e5\\u30a2\\u30fc",
      "\\u3074\\u3085\\u3042\\u30fc",
      "\\u30d4\\u30e5\\u30a2\\u30fc",
      "\\u30d2\\u30e5\\u30a6",
      "\\u30d2\\u30e6\\u30a6",
      "\\u30d4\\u30e5\\u30a6\\u30a2",
      "\\u3073\\u3085\\u30fc\\u3042\\u30fc", 
      "\\u30d3\\u30e5\\u30fc\\u30a2\\u30fc",
      "\\u30d3\\u30e5\\u30a6\\u30a2\\u30fc",
      "\\u3072\\u3085\\u3093",
      "\\u3074\\u3085\\u3093",
      "\\u3075\\u30fc\\u308a",
      "\\u30d5\\u30fc\\u30ea",
      "\\u3075\\u3045\\u308a",
      "\\u3075\\u30a5\\u308a",
      "\\u3075\\u30a5\\u30ea",
      "\\u30d5\\u30a6\\u30ea",
      "\\u3076\\u30fc\\u308a",
      "\\u30d6\\u30fc\\u30ea",
      "\\u3076\\u3045\\u308a",
      "\\u30d6\\u30a5\\u308a",
      "\\u3077\\u3046\\u308a",
      "\\u30d7\\u30a6\\u30ea",
      "\\u3075\\u30fc\\u308a\\u30fc",
      "\\u30d5\\u30a5\\u30ea\\u30fc",
      "\\u3075\\u30a5\\u308a\\u30a3",
      "\\u30d5\\u3045\\u308a\\u3043",
      "\\u30d5\\u30a6\\u30ea\\u30fc",
      "\\u3075\\u3046\\u308a\\u3043",
      "\\u30d6\\u30a6\\u30ea\\u30a4",
      "\\u3077\\u30fc\\u308a\\u30fc",
      "\\u3077\\u30a5\\u308a\\u30a4",
      "\\u3077\\u3046\\u308a\\u30fc",
      "\\u30d7\\u30a6\\u30ea\\u30a4",
      "\\u30d5\\u30fd",
      "\\u3075\\u309e",
      "\\u3076\\u309d",
      "\\u3076\\u3075",
      "\\u3076\\u30d5",
      "\\u30d6\\u3075",
      "\\u30d6\\u30d5",
      "\\u3076\\u309e",
      "\\u3076\\u3077",
      "\\u30d6\\u3077",
      "\\u3077\\u309d",
      "\\u30d7\\u30fd",
      "\\u3077\\u3075",
};

  static const char *test2[] = {
    "\\u306f\\u309d", /* H\\u309d */
    /*"\\u30cf\\u30fd",*/ /* K\\u30fd */
    "\\u306f\\u306f", /* HH */
    "\\u306f\\u30cf", /* HK */
    "\\u30cf\\u30cf", /* KK */
    "\\u306f\\u309e", /* H\\u309e */
    "\\u30cf\\u30fe", /* K\\u30fe */
    "\\u306f\\u3070", /* HH\\u309b */
    "\\u30cf\\u30d0", /* KK\\u309b */
    "\\u306f\\u3071", /* HH\\u309c */
    "\\u30cf\\u3071", /* KH\\u309c */
    "\\u30cf\\u30d1", /* KK\\u309c */
    "\\u3070\\u309d", /* H\\u309b\\u309d */
    "\\u30d0\\u30fd", /* K\\u309b\\u30fd */
    "\\u3070\\u306f", /* H\\u309bH */
    "\\u30d0\\u30cf", /* K\\u309bK */
    "\\u3070\\u309e", /* H\\u309b\\u309e */
    "\\u30d0\\u30fe", /* K\\u309b\\u30fe */
    "\\u3070\\u3070", /* H\\u309bH\\u309b */
    "\\u30d0\\u3070", /* K\\u309bH\\u309b */
    "\\u30d0\\u30d0", /* K\\u309bK\\u309b */
    "\\u3070\\u3071", /* H\\u309bH\\u309c */
    "\\u30d0\\u30d1", /* K\\u309bK\\u309c */
    "\\u3071\\u309d", /* H\\u309c\\u309d */
    "\\u30d1\\u30fd", /* K\\u309c\\u30fd */
    "\\u3071\\u306f", /* H\\u309cH */
    "\\u30d1\\u30cf", /* K\\u309cK */
    "\\u3071\\u3070", /* H\\u309cH\\u309b */
    "\\u3071\\u30d0", /* H\\u309cK\\u309b */
    "\\u30d1\\u30d0", /* K\\u309cK\\u309b */
    "\\u3071\\u3071", /* H\\u309cH\\u309c */
    "\\u30d1\\u30d1", /* K\\u309cK\\u309c */
  };
  /*
  static const char *test3[] = {
    "\\u221er\\u221e",
    "\\u221eR#",
    "\\u221et\\u221e",
    "#r\\u221e",
    "#R#",
    "#t%",
    "#T%",
    "8t\\u221e",
    "8T\\u221e",
    "8t#",
    "8T#",
    "8t%",
    "8T%",
    "8t8",
    "8T8",
    "\\u03c9r\\u221e",
    "\\u03a9R%",
    "rr\\u221e",
    "rR\\u221e",
    "Rr\\u221e",
    "RR\\u221e",
    "RT%",
    "rt8",
    "tr\\u221e",
    "tr8",
    "TR8",
    "tt8",
    "\\u30b7\\u30e3\\u30fc\\u30ec",
  };
  */
  static const UColAttribute att[] = { UCOL_STRENGTH };
  static const UColAttributeValue val[] = { UCOL_QUATERNARY };

  static const UColAttribute attShifted[] = { UCOL_STRENGTH, UCOL_ALTERNATE_HANDLING};
  static const UColAttributeValue valShifted[] = { UCOL_QUATERNARY, UCOL_SHIFTED };

  genericLocaleStarterWithOptions("ja", test1, sizeof(test1)/sizeof(test1[0]), att, val, 1);
  genericLocaleStarterWithOptions("ja", test2, sizeof(test2)/sizeof(test2[0]), att, val, 1);
  /*genericLocaleStarter("ja", test3, sizeof(test3)/sizeof(test3[0]));*/
  genericLocaleStarterWithOptions("ja", test1, sizeof(test1)/sizeof(test1[0]), attShifted, valShifted, 2);
  genericLocaleStarterWithOptions("ja", test2, sizeof(test2)/sizeof(test2[0]), attShifted, valShifted, 2);
}

static void TestStrCollIdenticalPrefix(void) {
  const char* rule = "&\\ud9b0\\udc70=\\ud9b0\\udc71";
  const char* test[] = {
    "ab\\ud9b0\\udc70",
    "ab\\ud9b0\\udc71"
  };
  genericRulesTestWithResult(rule, test, sizeof(test)/sizeof(test[0]), UCOL_EQUAL);
}
/* Contractions should have all their canonically equivalent */
/* strings included */
static void TestContractionClosure(void) {
  static struct {
    const char *rules;
    const char *data[50];
    const uint32_t len;
  } tests[] = {  
    {   "&b=\\u00e4\\u00e4",
      { "b", "\\u00e4\\u00e4", "a\\u0308a\\u0308", "\\u00e4a\\u0308", "a\\u0308\\u00e4" }, 5},
    {   "&b=\\u00C5",
      { "b", "\\u00C5", "A\\u030A", "\\u212B" }, 4},
  };
  uint32_t i;


  for(i = 0; i<(sizeof(tests)/sizeof(tests[0])); i++) {
    genericRulesTestWithResult(tests[i].rules, tests[i].data, tests[i].len, UCOL_EQUAL);
  }
}

/* This tests also fails*/
static void TestBeforePrefixFailure(void) {
  static struct {
    const char *rules;
    const char *data[50];
    const uint32_t len;
  } tests[] = {  
    { "&g <<< a"
      "&[before 3]\\uff41 <<< x",
      {"x", "\\uff41"}, 2 },
    {   "&\\u30A7=\\u30A7=\\u3047=\\uff6a"
        "&\\u30A8=\\u30A8=\\u3048=\\uff74"
        "&[before 3]\\u30a7<<<\\u30a9", 
      {"\\u30a9", "\\u30a7"}, 2 },
    {   "&[before 3]\\u30a7<<<\\u30a9"
        "&\\u30A7=\\u30A7=\\u3047=\\uff6a"
        "&\\u30A8=\\u30A8=\\u3048=\\uff74",
      {"\\u30a9", "\\u30a7"}, 2 },
  };
  uint32_t i;


  for(i = 0; i<(sizeof(tests)/sizeof(tests[0])); i++) {
    genericRulesStarter(tests[i].rules, tests[i].data, tests[i].len);
  }

#if 0
  const char* rule1 = 
        "&\\u30A7=\\u30A7=\\u3047=\\uff6a"
        "&\\u30A8=\\u30A8=\\u3048=\\uff74"
        "&[before 3]\\u30a7<<<\\u30c6|\\u30fc";
  const char* rule2 = 
        "&[before 3]\\u30a7<<<\\u30c6|\\u30fc"
        "&\\u30A7=\\u30A7=\\u3047=\\uff6a"
        "&\\u30A8=\\u30A8=\\u3048=\\uff74";
  const char* test[] = {
      "\\u30c6\\u30fc\\u30bf", 
      "\\u30c6\\u30a7\\u30bf",
  };
  genericRulesStarter(rule1, test, sizeof(test)/sizeof(test[0]));
  genericRulesStarter(rule2, test, sizeof(test)/sizeof(test[0]));
/* this piece of code should be in some sort of verbose mode     */
/* it gets the collation elements for elements and prints them   */
/* This is useful when trying to see whether the problem is      */
  { 
    UErrorCode status = U_ZERO_ERROR;
    uint32_t i = 0;
    UCollationElements *it = NULL;
    uint32_t CE;
    UChar string[256];
    uint32_t uStringLen;
    UCollator *coll = NULL;

    uStringLen = u_unescape(rule1, string, 256);

    coll = ucol_openRules(string, uStringLen, UCOL_DEFAULT, UCOL_DEFAULT, NULL, &status);

    /*coll = ucol_open("ja_JP_JIS", &status);*/
    it = ucol_openElements(coll, string, 0, &status);

    for(i = 0; i < sizeof(test)/sizeof(test[0]); i++) {
      log_verbose("%s\n", test[i]);
      uStringLen = u_unescape(test[i], string, 256);
      ucol_setText(it, string, uStringLen, &status);

      while((CE=ucol_next(it, &status)) != UCOL_NULLORDER) {
        log_verbose("%08X\n", CE);
      }
      log_verbose("\n");

    }

    ucol_closeElements(it);
    ucol_close(coll);
  }
#endif
}

static void TestPrefixCompose(void) {
  const char* rule1 = 
        "&\\u30a7<<<\\u30ab|\\u30fc=\\u30ac|\\u30fc";
  /*
  const char* test[] = {
      "\\u30c6\\u30fc\\u30bf", 
      "\\u30c6\\u30a7\\u30bf",
  };
  */
  { 
    UErrorCode status = U_ZERO_ERROR;
    /*uint32_t i = 0;*/
    /*UCollationElements *it = NULL;*/
/*    uint32_t CE;*/
    UChar string[256];
    uint32_t uStringLen;
    UCollator *coll = NULL;

    uStringLen = u_unescape(rule1, string, 256);

    coll = ucol_openRules(string, uStringLen, UCOL_DEFAULT, UCOL_DEFAULT, NULL, &status);
    ucol_close(coll);
  }


}

/*
[last variable] last variable value 
[last primary ignorable] largest CE for primary ignorable 
[last secondary ignorable] largest CE for secondary ignorable 
[last tertiary ignorable] largest CE for tertiary ignorable 
[top] guaranteed to be above all implicit CEs, for now and in the future (in 1.8) 
*/

static void TestRuleOptions(void) {
  /* values here are hardcoded and are correct for the current UCA 
   * when the UCA changes, one might be forced to change these 
   * values. (\\u02d0, \\U00010FFFC etc...)
   */
  static struct {
    const char *rules;
    const char *data[50];
    const uint32_t len;
  } tests[] = {  
    /* - all befores here amount to zero */
    { "&[before 1][first tertiary ignorable]<<<a", 
        { "\\u0000", "a"}, 2
    }, /* you cannot go before first tertiary ignorable */

    { "&[before 1][last tertiary ignorable]<<<a", 
        { "\\u0000", "a"}, 2
    }, /* you cannot go before last tertiary ignorable */

    { "&[before 1][first secondary ignorable]<<<a", 
        { "\\u0000", "a"}, 2
    }, /* you cannot go before first secondary ignorable */

    { "&[before 1][last secondary ignorable]<<<a", 
        { "\\u0000", "a"}, 2
    }, /* you cannot go before first secondary ignorable */

    /* 'normal' befores */

    { "&[before 1][first primary ignorable]<<<c<<<b &[first primary ignorable]<a", 
        {  "c", "b", "\\u0332", "a" }, 4
    },

    /* we don't have a code point that corresponds to  
     * the last primary ignorable
     */
    { "&[before 2][last primary ignorable]<<<c<<<b &[last primary ignorable]<a", 
        {  "\\u0332", "\\u20e3", "c", "b", "a" }, 5
    }, 

    { "&[before 1][first variable]<<<c<<<b &[first variable]<a", 
        {  "c", "b", "\\u0009", "a", "\\u000a" }, 5
    }, 

    { "&[last variable]<a &[before 1][last variable]<<<c<<<b ", 
        {  "c", "b", "\\uD800\\uDD33", "a", "\\u02d0" }, 5
    }, 

    { "&[first regular]<a"
      "&[before 1][first regular]<b",
      { "b", "\\u02d0", "a", "\\u02d1"}, 4
    },

    { "&[before 1][last regular]<b"
      "&[last regular]<a",
        { "b", "\\uD800\\uDF9D", "a", "\\u4e00" }, 4
    },     

    { "&[before 1][first implicit]<b"
      "&[first implicit]<a",
        { "b", "\\u4e00", "a", "\\u4e01"}, 4
    },

    { "&[before 1][last implicit]<b"
      "&[last implicit]<a",
        { "b", "\\U0010FFFC", "a" }, 3
    },     

    { "&[last variable]<z"
      "&[last primary ignorable]<x"
      "&[last secondary ignorable]<<y"
      "&[last tertiary ignorable]<<<w"
      "&[top]<u",
      {"\\ufffb",  "w", "y", "\\u20e3", "x", "\\u137c", "z", "u"}, 7 
    }

  };
  uint32_t i;


  for(i = 0; i<(sizeof(tests)/sizeof(tests[0])); i++) {
    genericRulesStarter(tests[i].rules, tests[i].data, tests[i].len);
  }
}


static void TestOptimize(void) {
  /* this is not really a test - just trying out 
   * whether copying of UCA contents will fail 
   * Cannot really test, since the functionality 
   * remains the same.
   */
  static struct {
    const char *rules;
    const char *data[50];
    const uint32_t len;
  } tests[] = {  
    /* - all befores here amount to zero */
    { "[optimize [\\uAC00-\\uD7FF]]", 
    { "a", "b"}, 2} 
  };
  uint32_t i;

  for(i = 0; i<(sizeof(tests)/sizeof(tests[0])); i++) {
    genericRulesStarter(tests[i].rules, tests[i].data, tests[i].len);
  }
}

/*
cycheng@ca.ibm.c...	we got inconsistent results when using the UTF-16BE iterator and the UTF-8 iterator.
weiv	ucol_strcollIter?
cycheng@ca.ibm.c...	e.g. s1 = 0xfffc0062, and s2 = d8000021
weiv	these are the input strings?
cycheng@ca.ibm.c...	yes, using the utf-16 iterator and UCA with normalization on, we have s1 > s2
weiv	will check - could be a problem with utf-8 iterator
cycheng@ca.ibm.c...	but if we use the utf-8 iterator, i.e. s1 = efbfbc62 and s2 = eda08021, we have s1 < s2
weiv	hmmm
cycheng@ca.ibm.c...	note that we have a standalone high surrogate
weiv	that doesn't sound right
cycheng@ca.ibm.c...	we got the same inconsistent results on AIX and Win2000
weiv	so you have two strings, you convert them to utf-8 and to utf-16BE
cycheng@ca.ibm.c...	yes
weiv	and then do the comparison
cycheng@ca.ibm.c...	in one case, the input strings are in utf8, and in the other case the input strings are in utf-16be
weiv	utf-16 strings look like a little endian ones in the example you sent me
weiv	It could be a bug - let me try to test it out
cycheng@ca.ibm.c...	ok
cycheng@ca.ibm.c...	we can wait till the conf. call
cycheng@ca.ibm.c...	next weke
weiv	that would be great
weiv	hmmm
weiv	I might be wrong
weiv	let me play with it some more
cycheng@ca.ibm.c...	ok
cycheng@ca.ibm.c...	also please check s3 = 0x0e3a0062  and s4 = 0x0e400021. both are in utf-16be
cycheng@ca.ibm.c...	seems with icu 2.2 we have s3 > s4, but not in icu 2.4 that's built for db2
cycheng@ca.ibm.c...	also s1 & s2 that I sent you earlier are also in utf-16be
weiv	ok
cycheng@ca.ibm.c...	i ask sherman to send you more inconsistent data 
weiv	thanks
cycheng@ca.ibm.c...	the 4 strings we sent are just samples
*/
#if 0
static void Alexis(void) {
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open("", &status);


  const char utf16be[2][4] = {
    { (char)0xd8, (char)0x00, (char)0x00, (char)0x21 },
    { (char)0xff, (char)0xfc, (char)0x00, (char)0x62 }
  };

  const char utf8[2][4] = {
    { (char)0xed, (char)0xa0, (char)0x80, (char)0x21 },
    { (char)0xef, (char)0xbf, (char)0xbc, (char)0x62 },
  };

  UCharIterator iterU161, iterU162;
  UCharIterator iterU81, iterU82;

  UCollationResult resU16, resU8;

  uiter_setUTF16BE(&iterU161, utf16be[0], 4);
  uiter_setUTF16BE(&iterU162, utf16be[1], 4);

  uiter_setUTF8(&iterU81, utf8[0], 4);
  uiter_setUTF8(&iterU82, utf8[1], 4);

  ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);

  resU16 = ucol_strcollIter(coll, &iterU161, &iterU162, &status);
  resU8 = ucol_strcollIter(coll, &iterU81, &iterU82, &status);


  if(resU16 != resU8) {
    log_err("different results\n");
  }

  ucol_close(coll);
}
#endif

#define CMSCOLL_ALEXIS2_BUFFER_SIZE 256
static void Alexis2(void) {
  UErrorCode status = U_ZERO_ERROR;
  UChar U16Source[CMSCOLL_ALEXIS2_BUFFER_SIZE], U16Target[CMSCOLL_ALEXIS2_BUFFER_SIZE];
  char U16BESource[CMSCOLL_ALEXIS2_BUFFER_SIZE], U16BETarget[CMSCOLL_ALEXIS2_BUFFER_SIZE];
  char U8Source[CMSCOLL_ALEXIS2_BUFFER_SIZE], U8Target[CMSCOLL_ALEXIS2_BUFFER_SIZE]; 
  int32_t U16LenS = 0, U16LenT = 0, U16BELenS = 0, U16BELenT = 0, U8LenS = 0, U8LenT = 0;

  UConverter *conv = NULL;

  UCharIterator U16BEItS, U16BEItT;
  UCharIterator U8ItS, U8ItT;

  UCollationResult resU16, resU16BE, resU8;

  const char* pairs[][2] = {
    { "\\ud800\\u0021", "\\uFFFC\\u0062"},
    { "\\u0435\\u0308\\u0334", "\\u0415\\u0334\\u0340" },
    { "\\u0E40\\u0021", "\\u00A1\\u0021"},
    { "\\u0E40\\u0021", "\\uFE57\\u0062"},
    { "\\u5F20", "\\u5F20\\u4E00\\u8E3F"},
    { "\\u0000\\u0020", "\\u0000\\u0020\\u0000"},
    { "\\u0020", "\\u0020\\u0000"}
/*
5F20 (my result here)
5F204E008E3F
5F20 (your result here)
*/
  };

  int32_t i = 0;

  UCollator *coll = ucol_open("", &status);
  if(status == U_FILE_ACCESS_ERROR) {
    log_data_err("Is your data around?\n");
    return;
  } else if(U_FAILURE(status)) {
    log_err("Error opening collator\n");
    return;
  }
  ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
  conv = ucnv_open("UTF16BE", &status);
  for(i = 0; i < sizeof(pairs)/sizeof(pairs[0]); i++) {
    U16LenS = u_unescape(pairs[i][0], U16Source, CMSCOLL_ALEXIS2_BUFFER_SIZE);
    U16LenT = u_unescape(pairs[i][1], U16Target, CMSCOLL_ALEXIS2_BUFFER_SIZE);

    resU16 = ucol_strcoll(coll, U16Source, U16LenS, U16Target, U16LenT);

    log_verbose("Result of strcoll is %i\n", resU16);

    U16BELenS = ucnv_fromUChars(conv, U16BESource, CMSCOLL_ALEXIS2_BUFFER_SIZE, U16Source, U16LenS, &status);
    U16BELenT = ucnv_fromUChars(conv, U16BETarget, CMSCOLL_ALEXIS2_BUFFER_SIZE, U16Target, U16LenT, &status);

    /* use the original sizes, as the result from converter is in bytes */
    uiter_setUTF16BE(&U16BEItS, U16BESource, U16LenS);
    uiter_setUTF16BE(&U16BEItT, U16BETarget, U16LenT);

    resU16BE = ucol_strcollIter(coll, &U16BEItS, &U16BEItT, &status);

    log_verbose("Result of U16BE is %i\n", resU16BE);

    if(resU16 != resU16BE) {
      log_verbose("Different results between UTF16 and UTF16BE for %s & %s\n", pairs[i][0], pairs[i][1]);
    }

    u_strToUTF8(U8Source, CMSCOLL_ALEXIS2_BUFFER_SIZE, &U8LenS, U16Source, U16LenS, &status);
    u_strToUTF8(U8Target, CMSCOLL_ALEXIS2_BUFFER_SIZE, &U8LenT, U16Target, U16LenT, &status);

    uiter_setUTF8(&U8ItS, U8Source, U8LenS);
    uiter_setUTF8(&U8ItT, U8Target, U8LenT);

    resU8 = ucol_strcollIter(coll, &U8ItS, &U8ItT, &status);

    if(resU16 != resU8) {
      log_verbose("Different results between UTF16 and UTF8 for %s & %s\n", pairs[i][0], pairs[i][1]);
    }

  }

  ucol_close(coll);
  ucnv_close(conv);
}

static void TestHebrewUCA(void) {
  UErrorCode status = U_ZERO_ERROR;
  const char *first[] = {
    "d790d6b8d79cd795d6bcd7a9",
    "d790d79cd79ed7a7d799d799d7a1",
    "d790d6b4d79ed795d6bcd7a9",
  };

  char utf8String[3][256];
  UChar utf16String[3][256];

  int32_t i = 0, j = 0;
  int32_t sizeUTF8[3];
  int32_t sizeUTF16[3];

  UCollator *coll = ucol_open("", &status);
  /*ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);*/

  for(i = 0; i < sizeof(first)/sizeof(first[0]); i++) {
    sizeUTF8[i] = u_parseUTF8(first[i], -1, utf8String[i], 256, &status);
    u_strFromUTF8(utf16String[i], 256, &sizeUTF16[i], utf8String[i], sizeUTF8[i], &status);
    log_verbose("%i: ");
    for(j = 0; j < sizeUTF16[i]; j++) {
      /*log_verbose("\\u%04X", utf16String[i][j]);*/
      log_verbose("%04X", utf16String[i][j]);
    }
    log_verbose("\n");
  }
  for(i = 0; i < sizeof(first)/sizeof(first[0])-1; i++) {
    for(j = i + 1; j < sizeof(first)/sizeof(first[0]); j++) {
      doTest(coll, utf16String[i], utf16String[j], UCOL_LESS);
    }
  }

  ucol_close(coll);

}

static void TestPartialSortKeyTermination(void) {
  const char* cases[] = {
    "\\u1234\\u1234\\udc00",
    "\\udc00\\ud800\\ud800" 
  };

  int32_t i = sizeof(UCollator);

  UErrorCode status = U_ZERO_ERROR;

  UCollator *coll = ucol_open("", &status);

  UCharIterator iter;

  UChar currCase[256];
  int32_t length = 0;
  int32_t pKeyLen = 0;

  uint8_t key[256];

  for(i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
    uint32_t state[2] = {0, 0};
    length = u_unescape(cases[i], currCase, 256);
    uiter_setString(&iter, currCase, length);
    pKeyLen = ucol_nextSortKeyPart(coll, &iter, state, key, 256, &status);

    log_verbose("Done\n");

  }
  ucol_close(coll);
}

static void TestSettings(void) {
  const char* cases[] = {
    "apple",
      "Apple"
  };

  const char* locales[] = {
    "",
      "en"
  };

  UErrorCode status = U_ZERO_ERROR;

  int32_t i = 0, j = 0;

  UChar source[256], target[256];
  int32_t sLen = 0, tLen = 0;

  UCollator *collateObject = NULL;
  for(i = 0; i < sizeof(locales)/sizeof(locales[0]); i++) {
    collateObject = ucol_open(locales[i], &status);
    ucol_setStrength(collateObject, UCOL_PRIMARY);
    ucol_setAttribute(collateObject, UCOL_CASE_LEVEL , UCOL_OFF, &status);
    for(j = 1; j < sizeof(cases)/sizeof(cases[0]); j++) {
      sLen = u_unescape(cases[j-1], source, 256);
      source[sLen] = 0;
      tLen = u_unescape(cases[j], target, 256);
      source[tLen] = 0;
      doTest(collateObject, source, target, UCOL_EQUAL);
    }
    ucol_close(collateObject);
  }
}

static int32_t TestEqualsForCollator(const char* locName, UCollator *source, UCollator *target) {
  UErrorCode status = U_ZERO_ERROR;
  int32_t errorNo = 0;
  /*const UChar *sourceRules = NULL;*/
  /*int32_t sourceRulesLen = 0;*/
  UColAttributeValue french = UCOL_OFF;
  int32_t cloneSize = 0;

  if(!ucol_equals(source, target)) {
    log_err("Same collators, different address not equal\n");
    errorNo++;
  }
  ucol_close(target);
  if(uprv_strcmp(ucol_getLocale(source, ULOC_REQUESTED_LOCALE, &status), ucol_getLocale(source, ULOC_ACTUAL_LOCALE, &status)) == 0) {
    /* currently, safeClone is implemented through getRules/openRules
     * so it is the same as the test below - I will comment that test out.
     */
    /* real thing */
    target = ucol_safeClone(source, NULL, &cloneSize, &status);
    if(U_FAILURE(status)) {
      log_err("Error creating clone\n");
      errorNo++;
      return errorNo;
    }
    if(!ucol_equals(source, target)) {
      log_err("Collator different from it's clone\n");
      errorNo++;
    }
    french = ucol_getAttribute(source, UCOL_FRENCH_COLLATION, &status);
    if(french == UCOL_ON) {
      ucol_setAttribute(target, UCOL_FRENCH_COLLATION, UCOL_OFF, &status);
    } else {
      ucol_setAttribute(target, UCOL_FRENCH_COLLATION, UCOL_ON, &status);
    }
    if(U_FAILURE(status)) {
      log_err("Error setting attributes\n");
      errorNo++;
      return errorNo;
    }
    if(ucol_equals(source, target)) {
      log_err("Collators same even when options changed\n");
      errorNo++;
    }
    ucol_close(target);
    /* commented out since safeClone uses exactly the same technique */
    /*
    sourceRules = ucol_getRules(source, &sourceRulesLen);
    target = ucol_openRules(sourceRules, sourceRulesLen, UCOL_DEFAULT, UCOL_DEFAULT, &parseError, &status);
    if(U_FAILURE(status)) {
      log_err("Error instantiating target from rules\n");
      errorNo++;
      return errorNo;
    }
    if(!ucol_equals(source, target)) {
      log_err("Collator different from collator that was created from the same rules\n");
      errorNo++;
    }
    ucol_close(target);
    */
  }
  return errorNo;
}


static void TestEquals(void) {
  /* ucol_equals is not currently a public API. There is a chance that it will become
   * something like this, but currently it is only used by RuleBasedCollator::operator==
   */
  /* test whether the two collators instantiated from the same locale are equal */
  UErrorCode status = U_ZERO_ERROR;
  UParseError parseError;
  int32_t noOfLoc = uloc_countAvailable();
  const char *locName = NULL;
  UCollator *source = NULL, *target = NULL;
  int32_t i = 0;

  const char* rules[] = {
    "&l < lj <<< Lj <<< LJ",
      "&n < nj <<< Nj <<< NJ",
      "&ae <<< \\u00e4",
      "&AE <<< \\u00c4"
  };
  /*
  const char* badRules[] = {
    "&l <<< Lj",
      "&n < nj <<< nJ <<< NJ",
      "&a <<< \\u00e4",
      "&AE <<< \\u00c4 <<< x"
  };
  */

  UChar sourceRules[1024], targetRules[1024];
  int32_t sourceRulesSize = 0, targetRulesSize = 0;
  int32_t rulesSize = sizeof(rules)/sizeof(rules[0]);

  for(i = 0; i < rulesSize; i++) {
    sourceRulesSize += u_unescape(rules[i], sourceRules+sourceRulesSize, 1024 - sourceRulesSize);
    targetRulesSize += u_unescape(rules[rulesSize-i-1], targetRules+targetRulesSize, 1024 - targetRulesSize);
  }

  source = ucol_openRules(sourceRules, sourceRulesSize, UCOL_DEFAULT, UCOL_DEFAULT, &parseError, &status);
  if(status == U_FILE_ACCESS_ERROR) {
    log_data_err("Is your data around?\n");
    return;
  } else if(U_FAILURE(status)) {
    log_err("Error opening collator\n");
    return;
  }
  target = ucol_openRules(targetRules, targetRulesSize, UCOL_DEFAULT, UCOL_DEFAULT, &parseError, &status);
  if(!ucol_equals(source, target)) {
    log_err("Equivalent collators not equal!\n");
  }
  ucol_close(source);
  ucol_close(target);

  source = ucol_open("root", &status);
  target = ucol_open("root", &status);
  log_verbose("Testing root\n");
  if(!ucol_equals(source, source)) {
    log_err("Same collator not equal\n");
  }
  if(TestEqualsForCollator(locName, source, target)) {
    log_err("Errors for root\n", locName);
  }
  ucol_close(source);

  for(i = 0; i<noOfLoc; i++) {
    status = U_ZERO_ERROR;
    locName = uloc_getAvailable(i);
    /*if(hasCollationElements(locName)) {*/
      log_verbose("Testing equality for locale %s\n", locName);
      source = ucol_open(locName, &status);
      target = ucol_open(locName, &status);
      if(TestEqualsForCollator(locName, source, target)) {
        log_err("Errors for locale %s\n", locName);
      }
      ucol_close(source);
    /*}*/
  }
}

static void TestJ2726(void) {
  UChar a[2] = { 0x61, 0x00 }; /*"a"*/
  UChar aSpace[3] = { 0x61, 0x20, 0x00 }; /*"a "*/
  UChar spaceA[3] = { 0x20, 0x61, 0x00 }; /*" a"*/
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open("en", &status);
  ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
  ucol_setAttribute(coll, UCOL_STRENGTH, UCOL_PRIMARY, &status);
  doTest(coll, a, aSpace, UCOL_EQUAL);
  doTest(coll, aSpace, a, UCOL_EQUAL);
  doTest(coll, a, spaceA, UCOL_EQUAL);
  doTest(coll, spaceA, a, UCOL_EQUAL);
  doTest(coll, spaceA, aSpace, UCOL_EQUAL);
  doTest(coll, aSpace, spaceA, UCOL_EQUAL);
  ucol_close(coll);
}

static void NullRule(void) {
  UChar r[3] = {0};
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_openRules(r, 1, UCOL_DEFAULT, UCOL_DEFAULT, NULL, &status);
  if(U_SUCCESS(status)) {
    log_err("This should have been an error!\n");
    ucol_close(coll);
  } else {
    status = U_ZERO_ERROR;
  }
  coll = ucol_openRules(r, 0, UCOL_DEFAULT, UCOL_DEFAULT, NULL, &status);
  if(U_FAILURE(status)) {
    log_err("Empty rules should have produced a valid collator\n");
  } else {
    ucol_close(coll);
  }
}

/**
 * Test for CollationElementIterator previous and next for the whole set of
 * unicode characters with normalization on.
 */
static void TestNumericCollation(void)
{
    UCollationElements *iter;
    UErrorCode status = U_ZERO_ERROR;

	int i = 0, j = 0, size = 0;
	/*UCollationResult collResult;*/
	UChar t1[100], t2[100];

	const static char *basicTestStrings[]={
	"hello1",
	"hello2",
	"hello123456"
	};
	
	const static char *preZeroTestStrings[]={
	"avery1",
	"avery01",
	"avery001",
	"avery0001"
	};
	
	const static char *thirtyTwoBitNumericStrings[]={
	"avery42949672960",
	"avery42949672961",
	"avery42949672962",
	"avery429496729610"
	};

    const static char *supplementaryDigits[] = {
      "\\uD835\\uDFCE", /* 0 */
      "\\uD835\\uDFCF", /* 1 */
      "\\uD835\\uDFD0", /* 2 */
      "\\uD835\\uDFD1", /* 3 */
      "\\uD835\\uDFCF\\uD835\\uDFCE", /* 10 */
      "\\uD835\\uDFCF\\uD835\\uDFCF", /* 11 */
      "\\uD835\\uDFCF\\uD835\\uDFD0", /* 12 */
      "\\uD835\\uDFD0\\uD835\\uDFCE", /* 20 */
      "\\uD835\\uDFD0\\uD835\\uDFCF", /* 21 */
      "\\uD835\\uDFD0\\uD835\\uDFD0" /* 22 */
    };

    const static char *foreignDigits[] = {
      "\\u0661",
        "\\u0662",
        "\\u0663",
      "\\u0661\\u0660",
      "\\u0661\\u0662",
      "\\u0661\\u0663",
      "\\u0662\\u0660",
      "\\u0662\\u0662",
      "\\u0662\\u0663",
      "\\u0663\\u0660",
      "\\u0663\\u0662",
      "\\u0663\\u0663"
    };

    UColAttribute att = UCOL_NUMERIC_COLLATION;
    UColAttributeValue val = UCOL_ON;

	/* Open our collator. */
    UCollator* coll = ucol_open("root", &status);
    if (U_FAILURE(status)){
        log_err("ERROR: in using ucol_open()\n %s\n",
              myErrorName(status));
        return;
    }
    genericLocaleStarterWithOptions("root", basicTestStrings, sizeof(basicTestStrings)/sizeof(basicTestStrings[0]), &att, &val, 1);
    genericLocaleStarterWithOptions("root", thirtyTwoBitNumericStrings, sizeof(thirtyTwoBitNumericStrings)/sizeof(thirtyTwoBitNumericStrings[0]), &att, &val, 1);
    genericLocaleStarterWithOptions("root", foreignDigits, sizeof(foreignDigits)/sizeof(foreignDigits[0]), &att, &val, 1);
    genericLocaleStarterWithOptions("root", supplementaryDigits, sizeof(supplementaryDigits)/sizeof(supplementaryDigits[0]), &att, &val, 1);	

	/* Setting up our collator to do digits. */
	ucol_setAttribute(coll, UCOL_NUMERIC_COLLATION, UCOL_ON, &status);
    if (U_FAILURE(status)){
        log_err("ERROR: in setting UCOL_NUMERIC_COLLATION as an attribute\n %s\n",
              myErrorName(status));
        return;
    }

    /* 
       Testing that prepended zeroes still yield the correct collation behavior. 
       We expect that every element in our strings array will be equal.
    */
    size = sizeof(preZeroTestStrings)/sizeof(preZeroTestStrings[0]);
    for(i = 0; i < size-1; i++) {
      for(j = i+1; j < size; j++) {
        u_uastrcpy(t1, preZeroTestStrings[i]);
        u_uastrcpy(t2, preZeroTestStrings[j]);
        doTest(coll, t1, t2, UCOL_EQUAL);
        iter=ucol_openElements(coll, t2, u_strlen(t2), &status);
        backAndForth(iter);
        ucol_closeElements(iter);
      }
    }

	/* 
      Testing collation element iterator. Running backAndForth on 
      a string with numbers in it should be sufficient.
     */
	u_uastrcpy(t1, basicTestStrings[2]);	
	iter=ucol_openElements(coll, t1, u_strlen(t1), &status);
	backAndForth(iter);
    ucol_closeElements(iter);
   
    ucol_close(coll);
}


#define TEST(x) addTest(root, &x, "tscoll/cmsccoll/" # x)

void addMiscCollTest(TestNode** root)
{
    addTest(root, &TestRuleOptions, "tscoll/cmsccoll/TestRuleOptions");
    addTest(root, &TestBeforePrefixFailure, "tscoll/cmsccoll/TestBeforePrefixFailure");
    addTest(root, &TestContractionClosure, "tscoll/cmsccoll/TestContractionClosure");
    addTest(root, &TestPrefixCompose, "tscoll/cmsccoll/TestPrefixCompose");
    addTest(root, &TestStrCollIdenticalPrefix, "tscoll/cmsccoll/TestStrCollIdenticalPrefix");
    addTest(root, &TestPrefix, "tscoll/cmsccoll/TestPrefix");
    addTest(root, &TestNewJapanese, "tscoll/cmsccoll/TestNewJapanese"); 
    /*addTest(root, &TestLimitations, "tscoll/cmsccoll/TestLimitations");*/
    addTest(root, &TestNonChars, "tscoll/cmsccoll/TestNonChars");
    addTest(root, &TestExtremeCompression, "tscoll/cmsccoll/TestExtremeCompression");
    addTest(root, &TestSurrogates, "tscoll/cmsccoll/TestSurrogates");
    addTest(root, &TestVariableTopSetting, "tscoll/cmsccoll/TestVariableTopSetting");
    addTest(root, &TestBocsuCoverage, "tscoll/cmsccoll/TestBocsuCoverage");
    addTest(root, &TestCyrillicTailoring, "tscoll/cmsccoll/TestCyrillicTailoring");
    addTest(root, &TestCase, "tscoll/cmsccoll/TestCase");
    addTest(root, &IncompleteCntTest, "tscoll/cmsccoll/IncompleteCntTest");
    addTest(root, &BlackBirdTest, "tscoll/cmsccoll/BlackBirdTest");
    addTest(root, &FunkyATest, "tscoll/cmsccoll/FunkyATest");
    addTest(root, &BillFairmanTest, "tscoll/cmsccoll/BillFairmanTest");
    addTest(root, &RamsRulesTest, "tscoll/cmsccoll/RamsRulesTest");
    addTest(root, &IsTailoredTest, "tscoll/cmsccoll/IsTailoredTest");
    addTest(root, &TestCollations, "tscoll/cmsccoll/TestCollations");
    addTest(root, &TestChMove, "tscoll/cmsccoll/TestChMove");
    addTest(root, &TestImplicitTailoring, "tscoll/cmsccoll/TestImplicitTailoring");
    addTest(root, &TestFCDProblem, "tscoll/cmsccoll/TestFCDProblem");
    addTest(root, &TestEmptyRule, "tscoll/cmsccoll/TestEmptyRule");
    addTest(root, &TestJ784, "tscoll/cmsccoll/TestJ784");
    addTest(root, &TestJ815, "tscoll/cmsccoll/TestJ815");
    addTest(root, &TestJ831, "tscoll/cmsccoll/TestJ831");
    addTest(root, &TestBefore, "tscoll/cmsccoll/TestBefore");
    addTest(root, &TestRedundantRules, "tscoll/cmsccoll/TestRedundantRules");
    addTest(root, &TestExpansionSyntax, "tscoll/cmsccoll/TestExpansionSyntax");
    addTest(root, &TestHangulTailoring, "tscoll/cmsccoll/TestHangulTailoring");
    addTest(root, &TestUCARules, "tscoll/cmsccoll/TestUCARules");
    addTest(root, &TestIncrementalNormalize, "tscoll/cmsccoll/TestIncrementalNormalize");
    addTest(root, &TestComposeDecompose, "tscoll/cmsccoll/TestComposeDecompose");
    addTest(root, &TestCompressOverlap, "tscoll/cmsccoll/TestCompressOverlap");
    addTest(root, &TestContraction, "tscoll/cmsccoll/TestContraction");
    addTest(root, &TestExpansion, "tscoll/cmsccoll/TestExpansion");
    /*addTest(root, &PrintMarkDavis, "tscoll/cmsccoll/PrintMarkDavis");*/ /* this test doesn't test - just prints sortkeys */
    /*addTest(root, &TestGetCaseBit, "tscoll/cmsccoll/TestGetCaseBit");*/ /*this one requires internal things to be exported */
    TEST(TestOptimize);
    TEST(TestSuppressContractions);
    TEST(Alexis2);
    TEST(TestHebrewUCA);
    TEST(TestPartialSortKeyTermination);
    TEST(TestSettings);
    TEST(TestEquals);
    TEST(TestJ2726);
    TEST(NullRule);
    TEST(TestNumericCollation);
}

#endif /* #if !UCONFIG_NO_COLLATION */

