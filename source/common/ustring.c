/*
*******************************************************************************
*
*   Copyright (C) 1998-1999, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File ustring.h
*
* Modification History:
*
*   Date        Name        Description
*   12/07/98    bertrand    Creation.
*******************************************************************************
*/

#include "unicode/ustring.h"
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ucnv.h"
#include "cstring.h"
#include "cmemory.h"
#include "umutex.h"
#include "ustr_imp.h"

/* forward declaractions of definitions for the shared default converter */

static UConverter *fgDefaultConverter = NULL;

static UConverter*
getDefaultConverter(void);

static void
releaseDefaultConverter(UConverter *converter);

/* ANSI string.h - style functions ------------------------------------------ */

#define MAX_STRLEN 0x0FFFFFFF

/* ---- String searching functions ---- */

UChar*
u_strchr(const UChar *s, UChar c) 
{
  while (*s && *s != c) {
    ++s;
  }
  if (*s == c)
    return (UChar *)s;
  return NULL;
}

/* A Boyer-Moore algorithm would be better, but that would require a hashtable
   because UChar is so big. This algorithm doesn't use a lot of extra memory.
 */
U_CAPI UChar * U_EXPORT2
u_strstr(const UChar *s, const UChar *substring) {

  UChar *strItr, *subItr;

  if (*substring == 0) {
    return (UChar *)s;
  }

  do {
    strItr = (UChar *)s;
    subItr = (UChar *)substring;

    /* Only one string iterator needs checking for null terminator */
    while ((*strItr != 0) && (*strItr == *subItr)) {
      strItr++;
      subItr++;
    }

    if (*subItr == 0) {             /* Was the end of the substring reached? */
      return (UChar *)s;
    }

    s++;
  } while (*strItr != 0);           /* Was the end of the string reached? */

  return NULL;                      /* No match */
}

U_CAPI UChar * U_EXPORT2
u_strchr32(const UChar *s, UChar32 c) {
  if(!UTF_NEED_MULTIPLE_UCHAR(c)) {
    return u_strchr(s, (UChar)c);
  } else {
    UChar buffer[UTF_MAX_CHAR_LENGTH + 1];
    UTextOffset i = 0;
    UTF_APPEND_CHAR_UNSAFE(buffer, i, c);
    buffer[i] = 0;
    return u_strstr(s, buffer);
  }
}

/* Search for a codepoint in a string that matches one of the matchSet codepoints. */
UChar *
u_strpbrk(const UChar *string, const UChar *matchSet)
{
    int32_t matchLen;
    UBool single = TRUE;

    for (matchLen = 0; matchSet[matchLen]; matchLen++)
    {
        if (!UTF_IS_SINGLE(matchSet[matchLen]))
        {
            single = FALSE;
        }
    }

    if (single)
    {
        const UChar *matchItr;
        const UChar *strItr;

        for (strItr = string; *strItr; strItr++)
        {
            for (matchItr = matchSet; *matchItr; matchItr++)
            {
                if (*matchItr == *strItr)
                {
                    return (UChar *)strItr;
                }
            }
        }
    }
    else
    {
        int32_t matchItr;
        int32_t strItr;
        UChar32 stringCh, matchSetCh;
        int32_t stringLen = u_strlen(string);

        for (strItr = 0; strItr < stringLen; strItr++)
        {
            UTF_GET_CHAR_SAFE(string, 0, strItr, stringLen, stringCh, TRUE);
            for (matchItr = 0; matchItr < matchLen; matchItr++)
            {
                UTF_GET_CHAR_SAFE(matchSet, 0, matchItr, matchLen, matchSetCh, TRUE);
                if (stringCh == matchSetCh && (stringCh != UTF_ERROR_VALUE
                    || string[strItr] == UTF_ERROR_VALUE
					|| (matchSetCh == UTF_ERROR_VALUE && !UTF_IS_SINGLE(matchSet[matchItr]))))
                {
                    return (UChar *)string + strItr;
                }
            }
        }
    }

    /* Didn't find it. */
    return NULL;
}

/* Search for a codepoint in a string that matches one of the matchSet codepoints. */
int32_t
u_strcspn(const UChar *string, const UChar *matchSet)
{
    const UChar *foundStr = u_strpbrk(string, matchSet);
    if (foundStr == NULL)
    {
        return u_strlen(string);
    }
    return foundStr - string;
}

/* Search for a codepoint in a string that does not match one of the matchSet codepoints. */
int32_t
u_strspn(const UChar *string, const UChar *matchSet)
{
    UBool single = TRUE;
    UBool match = TRUE;
    int32_t matchLen;
    int32_t retValue;

    for (matchLen = 0; matchSet[matchLen]; matchLen++)
    {
        if (!UTF_IS_SINGLE(matchSet[matchLen]))
        {
            single = FALSE;
        }
    }

    if (single)
    {
        const UChar *matchItr;
        const UChar *strItr;

        for (strItr = string; *strItr && match; strItr++)
        {
            match = FALSE;
            for (matchItr = matchSet; *matchItr; matchItr++)
            {
                if (*matchItr == *strItr)
                {
                    match = TRUE;
                    break;
                }
            }
        }
        retValue = strItr - string - (match == FALSE);
    }
    else
    {
        int32_t matchItr;
        int32_t strItr;
        UChar32 stringCh, matchSetCh;
        int32_t stringLen = u_strlen(string);

        for (strItr = 0; strItr < stringLen && match; strItr++)
        {
            match = FALSE;
            UTF_GET_CHAR_SAFE(string, 0, strItr, stringLen, stringCh, TRUE);
            for (matchItr = 0; matchItr < matchLen; matchItr++)
            {
                UTF_GET_CHAR_SAFE(matchSet, 0, matchItr, matchLen, matchSetCh, TRUE);
                if (stringCh == matchSetCh && (stringCh != UTF_ERROR_VALUE
                    || string[strItr] == UTF_ERROR_VALUE
					|| (matchSetCh == UTF_ERROR_VALUE && !UTF_IS_SINGLE(matchSet[matchItr]))))
                {
                    match = TRUE;
                    break;
                }
            }
        }
        retValue = strItr - (match == FALSE);
    }

    /* Found a mismatch or didn't find it. */
    return retValue;
}

/* ----- Text manipulation functions --- */

UChar*
u_strtok_r(UChar    *src, 
     const UChar    *delim,
           UChar   **saveState)
{
    UChar *tokSource;
    UChar *nextToken;
    uint32_t nonDelimIdx;

    if (src != NULL) {
        tokSource = src;
    }
    else if (saveState && *saveState) {
        tokSource = *saveState;
    }
    else {
        return NULL;
    }

    /* Skip initial delimiters */
    nonDelimIdx = u_strspn(tokSource, delim);
    tokSource = &tokSource[nonDelimIdx];

    nextToken = u_strpbrk(tokSource, delim);
    if (nextToken != NULL) {
        *(nextToken++) = 0;
        *saveState = nextToken;
        return tokSource;
    }
    else if (saveState && *saveState) {
        *saveState = NULL;
        return tokSource;
    }
    return NULL;
}

UChar*
u_strcat(UChar     *dst, 
    const UChar     *src)
{
  UChar *anchor = dst;            /* save a pointer to start of dst */

  while(*dst != 0) {              /* To end of first string          */
    ++dst;
  }
  while((*dst = *src) != 0) {     /* copy string 2 over              */
    ++dst;
    ++src;
  }

  return anchor;
}

UChar* 
u_strncat(UChar     *dst, 
     const UChar     *src, 
     int32_t     n ) 
{
  if(n > 0) {
    UChar *anchor = dst;            /* save a pointer to start of dst */

    while(*dst != 0) {              /* To end of first string          */
      ++dst;
    }
    while((*dst = *src) != 0) {     /* copy string 2 over              */
      ++dst;
      if(--n == 0) {
        *dst = 0;
        break;
      }
      ++src;
    }
  
    return anchor;
  } else {
    return dst;
  }
}

/* ----- Text property functions --- */

int32_t  
u_strcmp(const UChar *s1, 
    const UChar *s2) 
{
  int32_t rc;
  for(;;) {
    rc = (int32_t)*s1 - (int32_t)*s2;
    if(rc != 0 || *s1 == 0) {
      return rc;
    }
    ++s1;
    ++s2;
  }
}

/* String compare in code point order - u_strcmp() compares in code unit order. */
U_CAPI int32_t U_EXPORT2
u_strcmpCodePointOrder(const UChar *s1, const UChar *s2) {
  static const UChar utf16Fixup[32]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x2000, 0xf800, 0xf800, 0xf800, 0xf800
  };
  UChar c1, c2;
  int32_t diff;

  /* rotate each code unit's value so that surrogates get the highest values */
  for(;;) {
    c1=*s1;
    c1+=utf16Fixup[c1>>11]; /* additional "fix-up" line */
    c2=*s2;
    c2+=utf16Fixup[c2>>11]; /* additional "fix-up" line */

    /* now c1 and c2 are in UTF-32-compatible order */
    diff=(int32_t)c1-(int32_t)c2;
    if(diff!=0 || c1==0 /* redundant: || c2==0 */) {
      return diff;
    }
    ++s1;
    ++s2;
  }
}

int32_t  
u_strncmp(const UChar     *s1, 
     const UChar     *s2, 
     int32_t     n) 
{
  if(n > 0) {
    int32_t rc;
    for(;;) {
      rc = (int32_t)*s1 - (int32_t)*s2;
      if(rc != 0 || *s1 == 0 || --n == 0) {
        return rc;
      }
      ++s1;
      ++s2;
    }
  } else {
    return 0;
  }
}

UChar*
u_strcpy(UChar     *dst, 
    const UChar     *src) 
{
  UChar *anchor = dst;            /* save a pointer to start of dst */

  while((*dst = *src) != 0) {     /* copy string 2 over              */
    ++dst;
    ++src;
  }

  return anchor;
}

UChar* 
u_strncpy(UChar     *dst, 
     const UChar     *src, 
     int32_t     n) 
{
  UChar *anchor = dst;            /* save a pointer to start of dst */

  if(n > 0) {
    while((*dst = *src) != 0) {   /* copy string 2 over              */
      ++dst;
      if(--n == 0) {
        *dst = 0;
        break;
      }
      ++src;
    }
  } else {
    *dst = 0;
  }

  return anchor;
}

int32_t  
u_strlen(const UChar *s) 
{
# if U_SIZEOF_WCHAR_T == U_SIZEOF_UCHAR
    return uprv_wcslen(s);
# else
    const UChar *t = s;
    while(*t != 0) {
      ++t;
    }
    return t - s;
#endif
}

/* string casing ------------------------------------------------------------ */

/*
 * Implement argument checking and buffer handling
 * for string case mapping as a common function.
 */
enum {
    TO_LOWER,
    TO_UPPER
};

static int32_t
u_strCaseMap(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             int32_t toWhichCase,
             UErrorCode *pErrorCode) {
    UChar buffer[300];
    UChar *temp;
    int32_t destLength;

    /* check argument values */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if( destCapacity<0 ||
        (dest==NULL && destCapacity>0) ||
        src==NULL ||
        srcLength<-1
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* get the string length */
    if(srcLength==-1) {
        srcLength=u_strlen(src);
    }

    /* check for overlapping source and destination */
    if( (src>=dest && src<(dest+destCapacity)) ||
        (dest>=src && dest<(src+srcLength))
    ) {
        /* overlap: provide a temporary destination buffer and later copy the result */
        if(destCapacity<=(sizeof(buffer)/U_SIZEOF_UCHAR)) {
            /* the stack buffer is large enough */
            temp=buffer;
        } else {
            /* allocate a buffer */
            temp=uprv_malloc(destCapacity*U_SIZEOF_UCHAR);
            if(temp==NULL) {
                *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
        }
    } else {
        temp=dest;
    }

    if(toWhichCase==TO_LOWER) {
        destLength=u_internalStrToLower(temp, destCapacity, src, srcLength,
                                        locale, NULL, NULL, pErrorCode);
    } else {
        destLength=u_internalStrToUpper(temp, destCapacity, src, srcLength,
                                        locale, NULL, NULL, pErrorCode);
    }
    if(temp!=dest) {
        /* copy the result string to the destination buffer */
        uprv_memcpy(dest, temp, destLength*U_SIZEOF_UCHAR);
        if(temp!=buffer) {
            uprv_free(temp);
        }
    }

    /* zero-terminate if possible */
    if(destLength<destCapacity) {
        dest[destLength]=0;
    }
    return destLength;
}

U_CAPI int32_t U_EXPORT2
u_strToLower(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode) {
    return u_strCaseMap(dest, destCapacity, src, srcLength, locale, TO_LOWER, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
u_strToUpper(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode) {
    return u_strCaseMap(dest, destCapacity, src, srcLength, locale, TO_UPPER, pErrorCode);
}

/* conversions between char* and UChar* ------------------------------------- */

UChar* u_uastrcpy(UChar *ucs1,
          const char *s2 )
{
  UConverter *cnv = getDefaultConverter();
  if(cnv != NULL) {
    UErrorCode err = U_ZERO_ERROR;
    ucnv_toUChars(cnv,
                    ucs1,
                    MAX_STRLEN,
                    s2,
                    uprv_strlen(s2),
                    &err);
    releaseDefaultConverter(cnv);
    if(U_FAILURE(err)) {
      *ucs1 = 0;
    }
  } else {
    *ucs1 = 0;
  }
  return ucs1;
}

/*
 returns the minimum of (the length of the null-terminated string) and n.
*/
static int32_t u_astrnlen(const char *ucs1, int32_t n)
{
    int32_t len = 0;

    if (ucs1)
    {
        while (*(ucs1++) && n--)
        {
            len++;
        }
    }
    return len;
}

UChar* u_uastrncpy(UChar *ucs1,
           const char *s2 ,
           int32_t n)
{
  UChar *target = ucs1;
  UConverter *cnv = getDefaultConverter();
  if(cnv != NULL) {
    UErrorCode err = U_ZERO_ERROR;
    ucnv_reset(cnv);
    ucnv_toUnicode(cnv,
                   &target,
                   ucs1+n,
                   &s2,
                   s2+u_astrnlen(s2, n),
                   NULL,
                   TRUE,
                   &err);
    ucnv_reset(cnv); /* be good citizens */
    releaseDefaultConverter(cnv);
    if(U_FAILURE(err) && (err != U_BUFFER_OVERFLOW_ERROR) ) {
      *ucs1 = 0; /* failure */
    }
    if(target < (ucs1+n)) { /* U_BUFFER_OVERFLOW_ERROR isn't an err, just means no termination will happen. */
      *target = 0;  /* terminate */
    }
  } else {
    *ucs1 = 0;
  }
  return ucs1;
}

char* u_austrcpy(char *s1,
         const UChar *ucs2 )
{
  UConverter *cnv = getDefaultConverter();
  if(cnv != NULL) {
    UErrorCode err = U_ZERO_ERROR;
    int32_t len = ucnv_fromUChars(cnv,
                  s1,
                  MAX_STRLEN,
                  ucs2,
                  -1,
                  &err);
    releaseDefaultConverter(cnv);
    s1[len] = 0;
  } else {
    *s1 = 0;
  }
  return s1;
}

/* mutexed access to a shared default converter ----------------------------- */

/* this is the same implementation as in unistr.cpp */

static UConverter*
getDefaultConverter()
{
  UConverter *converter = NULL;

  if(fgDefaultConverter != NULL) {
    umtx_lock(NULL);

    /* need to check to make sure it wasn't taken out from under us */
    if(fgDefaultConverter != NULL) {
      converter = fgDefaultConverter;
      fgDefaultConverter = NULL;
    }
    umtx_unlock(NULL);
  }

  /* if the cache was empty, create a converter */
  if(converter == NULL) {
    UErrorCode status = U_ZERO_ERROR;
    converter = ucnv_open(NULL, &status);
    if(U_FAILURE(status)) {
      return NULL;
    }
  }

  return converter;
}

static void
releaseDefaultConverter(UConverter *converter)
{
  if(fgDefaultConverter == NULL) {
    umtx_lock(NULL);

    if(fgDefaultConverter == NULL) {
      fgDefaultConverter = converter;
      converter = NULL;
    }
    umtx_unlock(NULL);
  }

  if(converter != NULL) {
    ucnv_close(converter);
  }
}

/* u_unescape & support fns ------------------------------------------------- */

/* This map must be in ASCENDING ORDER OF THE ESCAPE CODE */
static const UChar UNESCAPE_MAP[] = {
    /*"   0x22, 0x22 */
    /*'   0x27, 0x27 */
    /*?   0x3F, 0x3F */
    /*\   0x5C, 0x5C */
    /*a*/ 0x61, 0x07,
    /*b*/ 0x62, 0x08,
    /*f*/ 0x66, 0x0c,
    /*n*/ 0x6E, 0x0a,
    /*r*/ 0x72, 0x0d,
    /*t*/ 0x74, 0x09,
    /*v*/ 0x76, 0x0b
};
enum { UNESCAPE_MAP_LENGTH = sizeof(UNESCAPE_MAP) / sizeof(UNESCAPE_MAP[0]) };

/* Convert one octal digit to a numeric value 0..7, or -1 on failure */
static int8_t _digit8(UChar c) {
    if (c >= 0x0030 && c <= 0x0037) {
        return (int8_t)(c - 0x0030);
    }
    return -1;
}

/* Convert one hex digit to a numeric value 0..F, or -1 on failure */
static int8_t _digit16(UChar c) {
    if (c >= 0x0030 && c <= 0x0039) {
        return (int8_t)(c - 0x0030);
    }
    if (c >= 0x0041 && c <= 0x0046) {
        return (int8_t)(c - (0x0041 - 10));
    }
    if (c >= 0x0061 && c <= 0x0066) {
        return (int8_t)(c - (0x0061 - 10));
    }
    return -1;
}

/* Parse a single escape sequence.  Although this method deals in
 * UChars, it does not use C++ or UnicodeString.  This allows it to
 * be used from C contexts. */
U_CAPI UChar32 U_EXPORT2
u_unescapeAt(UNESCAPE_CHAR_AT charAt,
             int32_t *offset,
             int32_t length,
             void *context) {

    int32_t start = *offset;
    UChar c;
    UChar32 result = 0;
    int8_t n = 0;
    int8_t minDig = 0;
    int8_t maxDig = 0;
    int8_t bitsPerDigit = 4; 
    int8_t dig;
    int32_t i;

    /* Check that offset is in range */
    if (*offset < 0 || *offset >= length) {
        goto err;
    }

    /* Fetch first UChar after '\\' */
    c = charAt((*offset)++, context);

    /* Convert hexadecimal and octal escapes */
    switch (c) {
    case 0x0075 /*'u'*/:
        minDig = maxDig = 4;
        break;
    case 0x0055 /*'U'*/:
        minDig = maxDig = 8;
        break;
    case 0x0078 /*'x'*/:
        minDig = 1;
        maxDig = 2;
        break;
    default:
        dig = _digit8(c);
        if (dig >= 0) {
            minDig = 1;
            maxDig = 3;
            n = 1; /* Already have first octal digit */
            bitsPerDigit = 3;
            result = dig;
        }
        break;
    }
    if (minDig != 0) {
        while (*offset < length && n < maxDig) {
            c = charAt(*offset, context);
            dig = (int8_t)((bitsPerDigit == 3) ? _digit8(c) : _digit16(c));
            if (dig < 0) {
                break;
            }
            result = (result << bitsPerDigit) | dig;
            ++(*offset);
            ++n;
        }
        if (n < minDig) {
            goto err;
        }
        return result;
    }

    /* Convert C-style escapes in table */
    for (i=0; i<UNESCAPE_MAP_LENGTH; i+=2) {
        if (c == UNESCAPE_MAP[i]) {
            return UNESCAPE_MAP[i+1];
        } else if (c < UNESCAPE_MAP[i]) {
            break;
        }
    }

    /* If no special forms are recognized, then consider
     * the backslash to generically escape the next character.
     * Deal with surrogate pairs. */
    if (UTF_IS_FIRST_SURROGATE(c) && *offset < length) {
        UChar c2 = charAt(*offset, context);
        if (UTF_IS_SECOND_SURROGATE(c2)) {
            ++(*offset);
            return UTF16_GET_PAIR_VALUE(c, c2);
        }
    }
    return c;

 err:
    /* Invalid escape sequence */
    *offset = start; /* Reset to initial value */
    return (UChar32)0xFFFFFFFF;
}

/* u_unescapeAt() callback to return a UChar from a char* */
static UChar _charPtr_charAt(int32_t offset, void *context) {
    UChar c16;
    /* It would be more efficient to access the invariant tables
     * directly but there is no API for that. */
    u_charsToUChars(((char*) context) + offset, &c16, 1);
    return c16;
}

/* Append an escape-free segment of the text; used by u_unescape() */
static void _appendUChars(UChar *dest, int32_t destCapacity,
                          const char *src, int32_t srcLen) {
    if (destCapacity < 0) {
        destCapacity = 0;
    }
    if (srcLen > destCapacity) {
        srcLen = destCapacity;
    }
    u_charsToUChars(src, dest, srcLen);
}

/* Do an invariant conversion of char* -> UChar*, with escape parsing */
U_CAPI int32_t U_EXPORT2
u_unescape(const char *src, UChar *dest, int32_t destCapacity) {
    const char *segment = src;
    int32_t i = 0;
    char c;

    while ((c=*src) != 0) {
        /* '\\' intentionally written as compiler-specific
         * character constant to correspond to compiler-specific
         * char* constants. */
        if (c == '\\') {
            int32_t lenParsed = 0;
            UChar32 c32;
            if (src != segment) {
                if (dest != NULL) {
                    _appendUChars(dest + i, destCapacity - i,
                                  segment, src - segment);
                }
                i += src - segment;
            }
            ++src; /* advance past '\\' */
            c32 = u_unescapeAt(_charPtr_charAt, &lenParsed, uprv_strlen(src), (void*)src);
            if (lenParsed == 0) {
                goto err;
            }
            src += lenParsed; /* advance past escape seq. */
            if (dest != NULL && UTF_CHAR_LENGTH(c32) <= (destCapacity - i)) {
                UTF_APPEND_CHAR_UNSAFE(dest, i, c32);
            } else {
                i += UTF_CHAR_LENGTH(c32);
            }
            segment = src;
        } else {
            ++src;
        }
    }
    if (src != segment) {
        if (dest != NULL) {
            _appendUChars(dest + i, destCapacity - i,
                          segment, src - segment);
        }
        i += src - segment;
    }
    if (dest != NULL && i < destCapacity) {
        dest[i] = 0;
    }
    return i + 1; /* add 1 for zero term */

 err:
    if (dest != NULL && destCapacity > 0) {
        *dest = 0;
    }
    return 0;
}
