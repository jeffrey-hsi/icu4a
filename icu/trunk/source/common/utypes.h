/*
*******************************************************************************
*                                                                             *
* COPYRIGHT:                                                                  *
*   (C) Copyright Taligent, Inc.,  1996, 1997                                 *
*   (C) Copyright International Business Machines Corporation,  1996-1999     *
*   Licensed Material - Program-Property of IBM - All Rights Reserved.        *
*   US Government Users Restricted Rights - Use, duplication, or disclosure   *
*   restricted by GSA ADP Schedule Contract with IBM Corp.                    *
*                                                                             *
*******************************************************************************
*
*  FILE NAME : UTYPES.H (formerly ptypes.h)
*
*   Date        Name        Description
*   12/11/96    helena      Creation.
*   02/27/97    aliu        Added typedefs for ClassID, int8, int16, int32,
*                           uint8, uint16, and uint32.
*   04/01/97    aliu        Added XP_CPLUSPLUS and modified to work under C as
*                            well as C++.
*                           Modified to use memcpy() for icu_arrayCopy() fns.
*   04/14/97    aliu        Added TPlatformUtilities.
*   05/07/97    aliu        Added import/export specifiers (replacing the old
*                           broken EXT_CLASS).  Added version number for our
*                           code.  Cleaned up header.
*    6/20/97    helena      Java class name change.
*   08/11/98    stephen     UErrorCode changed from typedef to enum
*   08/12/98    erm         Changed T_ANALYTIC_PACKAGE_VERSION to 3
*   08/14/98    stephen     Added icu_arrayCopy() for int8_t, int16_t, int32_t
*   12/09/98    jfitz       Added BUFFER_OVERFLOW_ERROR (bug 1100066)
*   04/20/99    stephen     Cleaned up & reworked for autoconf.
*                           Renamed to utypes.h.
*   05/05/99    stephen     Changed to use <inttypes.h>
*******************************************************************************
*/

#ifndef UTYPES_H
#define UTYPES_H

#include <memory.h>
#include <wchar.h>
#include <stdlib.h>
#include "cmemory.h"

/*===========================================================================*/
/* Include platform-dependent definitions                                    */
/* which are contained in the platform-specific file platform.h              */
/*===========================================================================*/

#if defined(WIN32) || defined(_WIN32)
#   include "pwin32.h"
#elif defined(__OS2__)
#   include "pos2.h"
#elif defined(__OS400__)
#   include "pos400.h"
#else
#   include "platform.h"
#endif

/* XP_CPLUSPLUS is a cross-platform symbol which should be defined when 
   using C++.  It should not be defined when compiling under C. */
#ifdef __cplusplus
#   ifndef XP_CPLUSPLUS
#       define XP_CPLUSPLUS
#   endif
#else
#   undef XP_CPLUSPLUS
#endif

/*===========================================================================*/
/* Boolean data type                                                         */
/*===========================================================================*/

#if ! HAVE_BOOL_T
typedef int8_t bool_t;
#endif

#ifndef TRUE
#   define TRUE  1
#endif
#ifndef FALSE
#   define FALSE 0
#endif

/*===========================================================================*/
/* Unicode string offset                                                     */
/*===========================================================================*/
typedef int32_t UTextOffset;

/*===========================================================================*/
/* Unicode character                                                         */
/*===========================================================================*/
typedef uint16_t UChar;

/*===========================================================================*/
/* ICU version number                                                        */
/*===========================================================================*/

/**
 * ICU package code version number.
 * This version number is incremented if and only if the code has changed
 * in a binary incompatible way.  For example, if the algorithm for generating
 * sort keys has changed, this code version must be incremented.
 *
 * This is for internal use only.  Clients should use
 * ResourceBundle::getVersionNumber().
 *
 * ResourceBundle::getVersionNumber() returns a full version number
 * for a resource, which consists of this code version number concatenated
 * with the ResourceBundle data file version number.
 */
#define ICU_VERSION "3"


/*===========================================================================*/
/* For C wrappers, we use the symbol CAPI.                                   */
/* This works properly if the includer is C or C++.                          */
/* Functions are declared   CAPI return-type U_EXPORT2 function-name() ...   */
/*===========================================================================*/

#ifdef XP_CPLUSPLUS
#   define C_FUNC extern "C"
#else
#   define C_FUNC
#endif
#define CAPI C_FUNC U_EXPORT


/* Define NULL pointer value  if it isn't already defined */

#ifndef NULL
#ifdef XP_CPLUSPLUS
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


/*===========================================================================*/
/* Calendar/TimeZone data types                                              */
/*===========================================================================*/

typedef double UDate;

/* Common time manipulation constants */
#define kMillisPerSecond        (1000)
#define kMillisPerMinute       (60000)
#define kMillisPerHour       (3600000)
#define kMillisPerDay       (86400000)


/** A struct representing a range of text containing a specific field */
struct UFieldPosition {
  /** The field */
  int32_t field;
  /** The start of the text range containing field */
  int32_t beginIndex;
  /** The limit of the text range containing field */
  int32_t endIndex;
};
typedef struct UFieldPosition UFieldPosition;

/*===========================================================================*/
/* ClassID-based RTTI */
/*===========================================================================*/

/**
 * ClassID is used to identify classes without using RTTI, since RTTI
 * is not yet supported by all C++ compilers.  Each class hierarchy which needs
 * to implement polymorphic clone() or operator==() defines two methods,
 * described in detail below.  ClassID values can be compared using
 * operator==(). Nothing else should be done with them.
 *
 * getDynamicClassID() is declared in the base class of the hierarchy as
 * a pure virtual.  Each concrete subclass implements it in the same way:
 *
 *      class Base {
 *      public:
 *          virtual ClassID getDynamicClassID() const = 0;
 *      }
 *
 *      class Derived {
 *      public:
 *          virtual ClassID getDynamicClassID() const
 *            { return Derived::getStaticClassID(); }
 *      }
 *
 * Each concrete class implements getStaticClassID() as well, which allows
 * clients to test for a specific type.
 *
 *      class Derived {
 *      public:
 *          static ClassID getStaticClassID();
 *      private:
 *          static char fgClassID;
 *      }
 *
 *      // In Derived.cpp:
 *      ClassID Derived::getStaticClassID()
 *        { return (ClassID)&Derived::fgClassID; }
 *      char Derived::fgClassID = 0; // Value is irrelevant
 */

typedef void* ClassID;

/*===========================================================================*/
/* Shared library/DLL import-export API control                              */
/*===========================================================================*/

/**
 * Control of symbol import/export.
 * The ICU is separated into two libraries.
 */


#ifdef U_COMMON_IMPLEMENTATION
#define U_COMMON_API  U_EXPORT
#define U_I18N_API    U_IMPORT
#elif defined(U_I18N_IMPLEMENTATION)
#define U_COMMON_API  U_IMPORT
#define U_I18N_API    U_EXPORT
#else
#define U_COMMON_API  U_IMPORT
#define U_I18N_API    U_IMPORT
#endif
/*===========================================================================*/
/* UErrorCode */
/*===========================================================================*/

/** Error code to replace exception handling */
enum UErrorCode {
    U_ERROR_INFO_START        = -128,     /* Start of information results (semantically successful) */
    U_USING_FALLBACK_ERROR    = -128,
    U_USING_DEFAULT_ERROR     = -127,
    U_ERROR_INFO_LIMIT,

    U_ZERO_ERROR              =  0,       /* success */

    U_ILLEGAL_ARGUMENT_ERROR  =  1,       /* Start of codes indicating failure */
    U_MISSING_RESOURCE_ERROR  =  2,
    U_INVALID_FORMAT_ERROR    =  3,
    U_FILE_ACCESS_ERROR       =  4,
    U_INTERNAL_PROGRAM_ERROR  =  5,       /* Indicates a bug in the library code */
    U_MESSAGE_PARSE_ERROR     =  6,
    U_MEMORY_ALLOCATION_ERROR =  7,       /* Memory allocation error */
    U_INDEX_OUTOFBOUNDS_ERROR =  8,
    U_PARSE_ERROR             =  9,       /* Equivalent to Java ParseException */
    U_INVALID_CHAR_FOUND      = 10,       /* In the Character conversion routines: Invalid character or sequence was encountered*/
    U_TRUNCATED_CHAR_FOUND    = 11,       /* In the Character conversion routines: More bytes are required to complete the conversion successfully*/
    U_ILLEGAL_CHAR_FOUND      = 12,       /* In codeset conversion: a sequence that does NOT belong in the codepage has been encountered*/
    U_INVALID_TABLE_FORMAT    = 13,       /* Conversion table file found, but corrupted*/
    U_INVALID_TABLE_FILE      = 14,       /* Conversion table file not found*/
    U_BUFFER_OVERFLOW_ERROR   = 15,       /* A result would not fit in the supplied buffer */
    U_UNSUPPORTED_ERROR       = 16,       /* Requested operation not supported in current context */
    U_ERROR_LIMIT
};

#ifndef XP_CPLUSPLUS
typedef enum UErrorCode UErrorCode;
#endif

/* Use the following to determine if an UErrorCode represents */
/* operational success or failure. */
#ifdef XP_CPLUSPLUS
inline bool_t SUCCESS(UErrorCode code) { return (bool_t)(code<=U_ZERO_ERROR); }
inline bool_t FAILURE(UErrorCode code) { return (bool_t)(code>U_ZERO_ERROR); }
#else
#define SUCCESS(x) ((x)<=U_ZERO_ERROR)
#define FAILURE(x) ((x)>U_ZERO_ERROR)
#endif


/* Casting function for int32_t (backward compatibility version, here until
   T_INT32 is replaced) */
#define T_INT32(i) ((int32_t)i)


/*===========================================================================*/
/* Wide-character functions                                                  */
/*===========================================================================*/
#define icu_wcscat(dst, src) wcscat(dst, src)
#define icu_wcscpy(dst, src) wcscpy(dst, src)
#define icu_wcslen(src) wcslen(src)
#define icu_wcstombs(mbstr, wcstr, count) wcstombs(mbstr, wcstr, count)
#define icu_mbstowcs(wcstr, mbstr, count) mbstowcs(wcstr, mbstr, count)

/*===========================================================================*/
/* Array copy utility functions */
/*===========================================================================*/

#ifdef XP_CPLUSPLUS
inline void icu_arrayCopy(const double* src, double* dst, int32_t count)
{ icu_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

inline void icu_arrayCopy(const double* src, int32_t srcStart,
              double* dst, int32_t dstStart, int32_t count)
{ icu_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

inline void icu_arrayCopy(const int8_t* src, int8_t* dst, int32_t count)
    { icu_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

inline void icu_arrayCopy(const int8_t* src, int32_t srcStart,
              int8_t* dst, int32_t dstStart, int32_t count)
{ icu_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

inline void icu_arrayCopy(const int16_t* src, int16_t* dst, int32_t count)
{ icu_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

inline void icu_arrayCopy(const int16_t* src, int32_t srcStart,
              int16_t* dst, int32_t dstStart, int32_t count)
{ icu_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

inline void icu_arrayCopy(const int32_t* src, int32_t* dst, int32_t count)
{ icu_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

inline void icu_arrayCopy(const int32_t* src, int32_t srcStart,
              int32_t* dst, int32_t dstStart, int32_t count)
{ icu_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

inline void
icu_arrayCopy(const UChar *src, int32_t srcStart,
        UChar *dst, int32_t dstStart, int32_t count)
{ icu_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

#endif

/*===========================================================================*/
/* Debugging */
/*===========================================================================*/

/* remove this */

/* This function is useful for debugging; it returns the text name */
/* of an UErrorCode result.  This is not the most efficient way of */
/* doing this but it's just for Debug builds anyway. */

/* Do not use these arrays directly: they will move to a .c file! */
static const char *
_uErrorInfoName[U_ERROR_INFO_LIMIT-U_ERROR_INFO_START]={
    "U_USING_FALLBACK_ERROR",
    "U_USING_DEFAULT_ERROR"
};

static const char *
_uErrorName[U_ERROR_LIMIT]={
    "U_ZERO_ERROR",

    "U_ILLEGAL_ARGUMENT_ERROR",
    "U_MISSING_RESOURCE_ERROR",
    "U_INVALID_FORMAT_ERROR",
    "U_FILE_ACCESS_ERROR",
    "U_INTERNAL_PROGRAM_ERROR",
    "U_MESSAGE_PARSE_ERROR",
    "U_MEMORY_ALLOCATION_ERROR",
    "U_INDEX_OUTOFBOUNDS_ERROR",
    "U_PARSE_ERROR",
    "U_INVALID_CHAR_FOUND",
    "U_TRUNCATED_CHAR_FOUND",
    "U_ILLEGAL_CHAR_FOUND",
    "U_INVALID_TABLE_FORMAT",
    "U_INVALID_TABLE_FILE",
    "U_BUFFER_OVERFLOW_ERROR",
    "U_UNSUPPORTED_ERROR"
};

#ifdef XP_CPLUSPLUS
inline const char *
errorName(UErrorCode code)
{
    if(code>=0 && code<U_ERROR_LIMIT) {
        return _uErrorName[code];
    } else if(code>=U_ERROR_INFO_START && code<U_ERROR_INFO_LIMIT) {
        return _uErrorInfoName[code];
    } else {
        return "[BOGUS UErrorCode]";
    }
}
#else
#   define errorName(code) \
        ((code)>=0 && (code)<U_ERROR_LIMIT) ? \
            _uErrorName[code] : \
            ((code)>=U_ERROR_INFO_START && (code)<U_ERROR_INFO_LIMIT) ? \
                _uErrorInfoName[code] : \
                "[BOGUS UErrorCode]"
#endif

/*===========================================================================*/
/* Include header for platform utilies */
/*===========================================================================*/

#include "putil.h"

#endif /* _UTYPES */
