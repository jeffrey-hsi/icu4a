/*
******************************************************************************
*
*   Copyright (C) 1997-2011, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
*  FILE NAME : platform.h
*
*   Date        Name        Description
*   05/13/98    nos         Creation (content moved here from ptypes.h).
*   03/02/99    stephen     Added AS400 support.
*   03/30/99    stephen     Added Linux support.
*   04/13/99    stephen     Reworked for autoconf.
******************************************************************************
*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

/**
 * \file
 * \brief Basic types for the platform.
 *
 * This file used to be generated by autoconf/configure.
 * Starting with ICU 49, platform.h is a normal source file,
 * to simplify cross-compiling and working with non-autoconf/make build systems.
 *
 * When a value in this file does not work on a platform, then please
 * try to derive it from the U_PLATFORM value
 * (for which we might need a new value constant in rare cases)
 * and/or from other macros that are predefined by the compiler
 * or defined in standard (POSIX or platform or compiler) headers.
 *
 * Note: Some compilers provide ways to show the predefined macros.
 * For example, with gcc you can compile an empty .c file and have the compiler
 * print the predefined macros with
 * \code
 * gcc -E -dM -x c /dev/null | sort
 * \endcode
 * (You can provide an actual empty .c file rather than /dev/null.
 * <code>-x c++</code> is for C++.)
 */

/* This file should be included before uvernum.h. */
#if defined(UVERNUM_H)
# error Do not include unicode/uvernum.h before #including unicode/platform.h.  Instead of unicode/uvernum.h, #include unicode/uversion.h
#endif

/**
 * Determines wheter to enable auto cleanup of libraries. 
 * @internal
 */
#ifndef UCLN_NO_AUTO_CLEANUP
#define UCLN_NO_AUTO_CLEANUP 0
#endif

/**
 * \def U_PLATFORM
 * The U_PLATFORM macro defines the platform we're on.
 *
 * We used to define one different, value-less macro per platform.
 * That made it hard to know the set of relevant platforms and macros,
 * and hard to deal with variants of platforms.
 *
 * Starting with ICU 49, we define platforms as numeric macros,
 * with ranges of values for related platforms and their variants.
 * The U_PLATFORM macro is set to one of these values.
 *
 * Historical note from the Solaris Wikipedia article:
 * AT&T and Sun collaborated on a project to merge the most popular Unix variants
 * on the market at that time: BSD, System V, and Xenix.
 * This became Unix System V Release 4 (SVR4).
 *
 * @internal
 */

/** Unknown platform. @internal */
#define U_PF_UNKNOWN 0
/** Windows @internal */
#define U_PF_WINDOWS 1000
/** MinGW. Windows, calls to Win32 API, but using GNU gcc and binutils. @internal */
#define U_PF_MINGW 1800
/** Cygwin. Windows, calls to cygwin1.dll for Posix functions, using GNU gcc and binutils. @internal */
#define U_PF_CYGWIN 1900
/* Reserve 2000 for U_PF_UNIX? */
/** HP-UX is based on UNIX System V. @internal */
#define U_PF_HPUX 2100
/** Solaris is a Unix operating system based on SVR4. @internal */
#define U_PF_SOLARIS 2600
/** BSD is a UNIX operating system derivative. @internal */
#define U_PF_BSD 3000
/** AIX is based on UNIX System V Releases and 4.3 BSD. @internal */
#define U_PF_AIX 3100
/** IRIX is based on UNIX System V with BSD extensions. @internal */
#define U_PF_IRIX 3200
/**
 * Darwin is a POSIX-compliant operating system, composed of code developed by Apple,
 * as well as code derived from NeXTSTEP, BSD, and other projects,
 * built around the Mach kernel.
 * Darwin forms the core set of components upon which Mac OS X, Apple TV, and iOS are based.
 * (Original description modified from WikiPedia.)
 * @internal
 */
#define U_PF_DARWIN 3500
/** iPhone OS (iOS) is a derivative of Mac OS X. @internal */
#define U_PF_IPHONE 3550
/** QNX is a commercial Unix-like real-time operating system related to BSD. @internal */
#define U_PF_QNX 3700
/** Linux is a Unix-like operating system. @internal */
#define U_PF_LINUX 4000
/** Android is based on Linux. @internal */
#define U_PF_ANDROID 4050
/** "Classic" Mac OS (1984–2001) @internal */
#define U_PF_CLASSIC_MACOS 8000
/** z/OS is the successor to OS/390 which was the successor to MVS. @internal */
#define U_PF_OS390 9000
/** "IBM i" is the current name of what used to be i5/OS and earlier OS/400. @internal */
#define U_PF_OS400 9400

#ifdef U_PLATFORM
    /* Use the predefined value. */
#elif defined(__MINGW32__)
#   define U_PLATFORM U_PF_MINGW
#elif defined(__CYGWIN__)
#   define U_PLATFORM U_PF_CYGWIN
#elif defined(_WIN32)
#   define U_PLATFORM U_PF_WINDOWS
#elif defined(__ANDROID__)
#   define U_PLATFORM U_PF_ANDROID
    /* Android wchar_t support depends on the API level. */
#   include <android/api-level.h>
#elif defined(linux) || defined(__linux__) || defined(__linux)
#   define U_PLATFORM U_PF_LINUX
#elif defined(BSD)
#   define U_PLATFORM U_PF_BSD
#elif defined(sun) || defined(__sun)
    /* Check defined(__SVR4) || defined(__svr4__) to distinguish Solaris from SunOS? */
#   define U_PLATFORM U_PF_SOLARIS
#elif defined(_AIX) || defined(__TOS_AIX__)
#   define U_PLATFORM U_PF_AIX
#elif defined(_hpux) || defined(hpux) || defined(__hpux)
#   define U_PLATFORM U_PF_HPUX
#elif defined(sgi) || defined(__sgi)
#   define U_PLATFORM U_PF_IRIX
#elif defined(__APPLE__) && defined(__MACH__)
#   include "TargetConditionals.h"
#   ifdef TARGET_OS_IPHONE  /* variant of TARGET_OS_MAC */
#       define U_PLATFORM U_PF_IPHONE
#   else
#       define U_PLATFORM U_PF_DARWIN
#   endif
#elif defined(macintosh)
#   define U_PLATFORM U_PF_CLASSIC_MACOS
#elif defined(__QNX__) || defined(__QNXNTO__)
#   define U_PLATFORM U_PF_QNX
#elif defined(__TOS_MVS__)
#   define U_PLATFORM U_PF_OS390
#elif defined(__OS400__) || defined(__TOS_OS400__)
#   define U_PLATFORM U_PF_OS400
#else
#   define U_PLATFORM U_PF_UNKNOWN
#endif

/**
 * \def U_HAVE_DIRENT_H
 * Defines whether dirent.h is available.
 * @internal
 */
#ifdef U_HAVE_DIRENT_H
    /* Use the predefined value. */
#elif U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#   define U_HAVE_DIRENT_H 0
#else
#   define U_HAVE_DIRENT_H 1
#endif

/**
 * \def U_HAVE_INTTYPES_H
 * Defines whether stdint.h is available. It is a C99 standard header.
 * We used to include inttypes.h which includes stdint.h but we usually do not need
 * the additional definitions from inttypes.h.
 * @internal
 */
#ifdef U_HAVE_INTTYPES_H
    /* Use the predefined value. */
#elif U_PLATFORM == U_WINDOWS
#   if defined(__BORLANDC__) || (defined(_MSC_VER) && _MSC_VER>=1600)
        /* Windows Visual Studio 9 and below do not have stdint.h & inttypes.h, but VS 2010 adds them. */
#       define U_HAVE_INTTYPES_H 1
#   else
#       define U_HAVE_INTTYPES_H 0
#   endif
#else
#   define U_HAVE_INTTYPES_H 1
#endif

/**
 * \def U_IOSTREAM_SOURCE
 * Defines what support for C++ streams is available.
 *
 * If U_IOSTREAM_SOURCE is set to 199711, then &lt;iostream&gt; is available
 * (the ISO/IEC C++ FDIS was published in November 1997), and then
 * one should qualify streams using the std namespace in ICU header
 * files.
 * Starting with ICU 49, this is the only supported version.
 *
 * If U_IOSTREAM_SOURCE is set to 198506, then &lt;iostream.h&gt; is
 * available instead (in June 1985 Stroustrup published
 * "An Extensible I/O Facility for C++" at the summer USENIX conference).
 * Starting with ICU 49, this version is not supported any more.
 *
 * If U_IOSTREAM_SOURCE is 0 (or any value less than 199711),
 * then C++ streams are not available and
 * support for them will be silently suppressed in ICU.
 *
 * @internal
 */
#ifndef U_IOSTREAM_SOURCE
#define U_IOSTREAM_SOURCE 199711
#endif

/**
 * \def U_HAVE_STD_STRING
 * Defines whether the standard C++ (STL) &lt;string&gt; header is available.
 * @internal
 */
#ifdef U_HAVE_STD_STRING
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_ANDROID
#   define U_HAVE_STD_STRING 0
#else
#   define U_HAVE_STD_STRING 1
#endif

/*===========================================================================*/
/** @{ Compiler and environment features                                     */
/*===========================================================================*/

/**
 * \def U_IS_BIG_ENDIAN
 * Determines the endianness of the platform.
 * @internal
 */
#ifdef U_IS_BIG_ENDIAN
    /* Use the predefined value. */
#elif defined(BYTE_ORDER) && defined(BIG_ENDIAN)
#   define U_IS_BIG_ENDIAN (BYTE_ORDER == BIG_ENDIAN)
#elif defined(__BIG_ENDIAN__)
#   define U_IS_BIG_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__)
#   define U_IS_BIG_ENDIAN 0
#else
#   define U_IS_BIG_ENDIAN 0
#endif

/* 1 or 0 to enable or disable threads.  If undefined, default is: enable threads. */
#ifndef ICU_USE_THREADS
#define ICU_USE_THREADS 1
#endif

#ifdef U_DEBUG
    /* Use the predefined value. */
#elif defined(_DEBUG)
    /*
     * _DEBUG is defined by Visual Studio debug compilation.
     * Do *not* test for its NDEBUG macro: It is an orthogonal macro
     * which disables assert().
     */
#   define U_DEBUG 1
#elif defined(U_RELEASE)
#   define U_DEBUG (!U_RELEASE)
# else
#   define U_DEBUG 0
#endif

#ifndef U_RELEASE
#define U_RELEASE (!U_DEBUG)
#endif

/**
 * \def U_DISABLE_RENAMING
 * Determines whether to disable renaming or not.
 * @internal
 */
#ifndef U_DISABLE_RENAMING
#define U_DISABLE_RENAMING 0
#endif

/* Determine whether to override new and delete. */
#ifndef U_OVERRIDE_CXX_ALLOCATION
#define U_OVERRIDE_CXX_ALLOCATION @U_OVERRIDE_CXX_ALLOCATION@
#endif
/* Determine whether to override placement new and delete for STL. */
#ifndef U_HAVE_PLACEMENT_NEW
#define U_HAVE_PLACEMENT_NEW @U_HAVE_PLACEMENT_NEW@
#endif

/* Determine whether to enable tracing. */
#ifndef U_ENABLE_TRACING
#define U_ENABLE_TRACING @U_ENABLE_TRACING@
#endif

/**
 * Whether to enable Dynamic loading in ICU
 * @internal
 */
#ifndef U_ENABLE_DYLOAD
#define U_ENABLE_DYLOAD @U_ENABLE_DYLOAD@
#endif

/**
 * Whether to test Dynamic loading as an OS capabilty
 * @internal
 */
#ifndef U_CHECK_DYLOAD
#define U_CHECK_DYLOAD @U_CHECK_DYLOAD@
#endif


/** Do we allow ICU users to use the draft APIs by default? */
#ifndef U_DEFAULT_SHOW_DRAFT
#define U_DEFAULT_SHOW_DRAFT @U_DEFAULT_SHOW_DRAFT@
#endif

/** @} */

/*===========================================================================*/
/** @{ Character data types                                                      */
/*===========================================================================*/

#ifdef U_CHARSET_FAMILY
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_OS390 && (!defined(__CHARSET_LIB) || !__CHARSET_LIB)
    /* U_EBCDIC_FAMILY, see utypes.h */
#   define U_CHARSET_FAMILY 1
#elif U_PLATFORM == U_PF_OS400 && !defined(__UTF32__)
    /* U_EBCDIC_FAMILY, see utypes.h */
#   define U_CHARSET_FAMILY 1
#else
    /* U_ASCII_FAMILY, see utypes.h */
#   define U_CHARSET_FAMILY 0
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about wchar support                                           */
/*===========================================================================*/

#ifdef U_HAVE_WCHAR_H
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_ANDROID && __ANDROID_API__ < 9
    /*
     * Android before Gingerbread (Android 2.3, API level 9) did not support wchar_t.
     * The type and header existed, but the library functions did not work as expected.
     * The size of wchar_t was 1 but L"xyz" string literals had 32-bit units anyway.
     */
#   define U_HAVE_WCHAR_H 0
#else
#   define U_HAVE_WCHAR_H 1
#endif

#ifdef U_SIZEOF_WCHAR_T
    /* Use the predefined value. */
#elif U_PLATFORM == U_PF_ANDROID && __ANDROID_API__ < 9
#   define U_SIZEOF_WCHAR_T 1
#elif U_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#   define U_SIZEOF_WCHAR_T 2
#elif (U_PF_DARWIN <= U_PLATFORM && U_PLATFORM <= U_PF_IPHONE) || U_PLATFORM == U_PF_CLASSIC_MACOS
#   define U_SIZEOF_WCHAR_T 2
#elif U_PLATFORM == U_PF_AIX
    /*
     * AIX 6.1 information, section "Wide character data representation":
     * "... the wchar_t datatype is 32–bit in the 64–bit environment and
     * 16–bit in the 32–bit environment."
     * and
     * "All locales use Unicode for their wide character code values (process code),
     * except the IBM-eucTW codeset."
     */
#   ifdef __64BIT__
#       define U_SIZEOF_WCHAR_T 4
#   else
#       define U_SIZEOF_WCHAR_T 2
#   endif
#elif U_PLATFORM == U_PF_OS390
    /*
     * z/OS V1R11 information center, section "LP64 | ILP32":
     * "In 31-bit mode, the size of long and pointers is 4 bytes and the size of wchar_t is 2 bytes.
     * Under LP64, the size of long and pointer is 8 bytes and the size of wchar_t is 4 bytes."
     */
#   ifdef _LP64
#       define U_SIZEOF_WCHAR_T 4
#   else
#       define U_SIZEOF_WCHAR_T 2
#   endif
#elif U_PLATFORM == U_PF_OS400
#   if defined(__UTF32__)
        /*
         * LOCALETYPE(*LOCALEUTF) is specified.
         * Wide-character strings are in UTF-32,
         * narrow-character strings are in UTF-8.
         */
#       define U_SIZEOF_WCHAR_T 4
#   elif defined(__UCS2__)
        /*
         * LOCALETYPE(*LOCALEUCS2) is specified.
         * Wide-character strings are in UCS-2,
         * narrow-character strings are in EBCDIC.
         */
#       define U_SIZEOF_WCHAR_T 2
#else
        /*
         * LOCALETYPE(*CLD) or LOCALETYPE(*LOCALE) is specified.
         * Wide-character strings are in 16-bit EBCDIC,
         * narrow-character strings are in EBCDIC.
         */
#       define U_SIZEOF_WCHAR_T 2
#   endif
#else
#   define U_SIZEOF_WCHAR_T 4
#endif

#ifndef U_HAVE_WCSCPY
#define U_HAVE_WCSCPY U_HAVE_WCHAR_H
#endif

/** @} */

/**
 * @{
 * \def U_DECLARE_UTF16
 * Do not use this macro. Use the UNICODE_STRING or U_STRING_DECL macros
 * instead.
 * @internal
 *
 * \def U_GNUC_UTF16_STRING
 * @internal
 */
#ifndef U_GNUC_UTF16_STRING
#define U_GNUC_UTF16_STRING @U_CHECK_GNUC_UTF16_STRING@
#endif
#if @U_CHECK_UTF16_STRING@ || defined(U_CHECK_UTF16_STRING)
#if (defined(__xlC__) && defined(__IBM_UTF_LITERAL) && U_SIZEOF_WCHAR_T != 2) \
    || (defined(__HP_aCC) && __HP_aCC >= 035000) \
    || (defined(__HP_cc) && __HP_cc >= 111106) \
    || U_GNUC_UTF16_STRING
#define U_DECLARE_UTF16(string) u ## string
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550)
/* || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x580) */
/* Sun's C compiler has issues with this notation, and it's unreliable. */
#define U_DECLARE_UTF16(string) U ## string
#elif U_SIZEOF_WCHAR_T == 2 \
    && (U_CHARSET_FAMILY == 0 || ((defined(OS390) || defined(OS400)) && defined(__UCS2__)))
#define U_DECLARE_UTF16(string) L ## string
#endif
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about POSIX support                                           */
/*===========================================================================*/

#ifndef U_HAVE_NL_LANGINFO_CODESET
#define U_HAVE_NL_LANGINFO_CODESET  @U_HAVE_NL_LANGINFO_CODESET@
#endif

#ifndef U_NL_LANGINFO_CODESET
#define U_NL_LANGINFO_CODESET       @U_NL_LANGINFO_CODESET@
#endif

#if @U_HAVE_TZSET@
#define U_TZSET         @U_TZSET@
#endif
#if @U_HAVE_TIMEZONE@
#define U_TIMEZONE      @U_TIMEZONE@
#endif
#if @U_HAVE_TZNAME@
#define U_TZNAME        @U_TZNAME@
#endif

#define U_HAVE_MMAP     @HAVE_MMAP@
#define U_HAVE_POPEN    @U_HAVE_POPEN@

/** @} */

/*===========================================================================*/
/** @{ Symbol import-export control                                          */
/*===========================================================================*/

#ifdef U_STATIC_IMPLEMENTATION
#define U_EXPORT
#elif @U_USE_GCC_VISIBILITY_ATTRIBUTE@
#define U_EXPORT __attribute__((visibility("default")))
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550) \
   || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x550) 
#define U_EXPORT __global
/*#elif defined(__HP_aCC) || defined(__HP_cc)
#define U_EXPORT __declspec(dllexport)*/
#else
#define U_EXPORT
#endif

/* U_CALLCONV is releated to U_EXPORT2 */
#define U_EXPORT2

/* cygwin needs to export/import data */
#if defined(U_CYGWIN) && !defined(__GNUC__)
#define U_IMPORT __declspec(dllimport)
#else
#define U_IMPORT 
#endif

/* @} */

/*===========================================================================*/
/** @{ Code alignment                                                        */
/*===========================================================================*/

#ifndef U_ALIGN_CODE
#define U_ALIGN_CODE(n) 
#endif

/** @} */

/*===========================================================================*/
/** @{ GCC built in functions for atomic memory operations                   */
/*===========================================================================*/

/**
 * \def U_HAVE_GCC_ATOMICS
 * @internal
 */
#ifndef U_HAVE_GCC_ATOMICS
#define U_HAVE_GCC_ATOMICS @U_HAVE_GCC_ATOMICS@
#endif

/** @} */

/*===========================================================================*/
/** @{ Programs used by ICU code                                                 */
/*===========================================================================*/

/**
 * \def U_MAKE
 * What program to execute to run 'make'
 */
#ifndef U_MAKE
#define U_MAKE  "@U_MAKE@"
#endif

/** @} */

/*===========================================================================*/
/* Custom icu entry point renaming                                                  */
/*===========================================================================*/

/**
 * Define the library suffix with C syntax.
 * @internal
 */
# define U_LIB_SUFFIX_C_NAME @ICULIBSUFFIXCNAME@
/**
 * Define the library suffix as a string with C syntax
 * @internal
 */
# define U_LIB_SUFFIX_C_NAME_STRING "@ICULIBSUFFIXCNAME@"
/**
 * 1 if a custom library suffix is set
 * @internal
 */
# define U_HAVE_LIB_SUFFIX @U_HAVE_LIB_SUFFIX@

#if U_HAVE_LIB_SUFFIX
# ifndef U_ICU_ENTRY_POINT_RENAME
/* Renaming pattern:    u_strcpy_41_suffix */
#  define U_ICU_ENTRY_POINT_RENAME(x)    x ## _ ## @LIB_VERSION_MAJOR@ ## @ICULIBSUFFIXCNAME@
#  define U_DEF_ICUDATA_ENTRY_POINT(major) icudt##@ICULIBSUFFIXCNAME@##major##_dat

# endif
#endif

#endif
