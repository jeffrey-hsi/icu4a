/*
*******************************************************************************
*   Copyright (C) 1997-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   Date        Name        Description
*   06/21/00    aliu        Creation.
*******************************************************************************
*/

#ifndef UTRANS_H
#define UTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/urep.h"
#include "unicode/parseerr.h"

/********************************************************************
 * General Notes
 ********************************************************************
 */
/**
 * \file
 * \brief C API: Transliterator
 *
 * <h2> Transliteration </h2>
 * The data structures and functions described in this header provide
 * transliteration services.  Transliteration services are implemented
 * as C++ classes.  The comments and documentation in this header
 * assume the reader is familiar with the C++ headers translit.h and
 * rbt.h and associated documentation.
 *
 * A significant but incomplete subset of the C++ transliteration
 * services are available to C code through this header.  In order to
 * access more complex transliteration services, refer to the C++
 * headers and documentation.
 */

/********************************************************************
 * Data Structures
 ********************************************************************/

/**
 * An opaque transliterator for use in C.  Open with utrans_openxxx()
 * and close with utrans_close() when done.  Equivalent to the C++ class
 * Transliterator and its subclasses.
 * @see Transliterator
 * @stable
 */
typedef void* UTransliterator;

/**
 * Direction constant indicating the direction in a transliterator,
 * e.g., the forward or reverse rules of a RuleBasedTransliterator.
 * Specified when a transliterator is opened.  An "A-B" transliterator
 * transliterates A to B when operating in the forward direction, and
 * B to A when operating in the reverse direction.
 * @stable
 */
typedef enum UTransDirection {
    
    /**
     * UTRANS_FORWARD means from <source> to <target> for a
     * transliterator with ID <source>-<target>.  For a transliterator
     * opened using a rule, it means forward direction rules, e.g.,
     * "A > B".
     */
    UTRANS_FORWARD,

    /**
     * UTRANS_REVERSE means from <target> to <source> for a
     * transliterator with ID <source>-<target>.  For a transliterator
     * opened using a rule, it means reverse direction rules, e.g.,
     * "A < B".
     */
    UTRANS_REVERSE

} UTransDirection;

/**
 * Position structure for utrans_transIncremental() incremental
 * transliteration.  This structure defines two substrings of the text
 * being transliterated.  The first region, [contextStart,
 * contextLimit), defines what characters the transliterator will read
 * as context.  The second region, [start, limit), defines what
 * characters will actually be transliterated.  The second region
 * should be a subset of the first.
 *
 * <p>After a transliteration operation, some of the indices in this
 * structure will be modified.  See the field descriptions for
 * details.
 *
 * <p>contextStart <= start <= limit <= contextLimit
 *
 * <p>Note: All index values in this structure must be at code point
 * boundaries.  That is, none of them may occur between two code units
 * of a surrogate pair.  If any index does split a surrogate pair,
 * results are unspecified.
 *
 * @stable
 */
typedef struct UTransPosition {

    /**
     * Beginning index, inclusive, of the context to be considered for
     * a transliteration operation.  The transliterator will ignore
     * anything before this index.  INPUT/OUTPUT parameter: This parameter
     * is updated by a transliteration operation to reflect the maximum
     * amount of antecontext needed by a transliterator.
     */
    int32_t contextStart;
    
    /**
     * Ending index, exclusive, of the context to be considered for a
     * transliteration operation.  The transliterator will ignore
     * anything at or after this index.  INPUT/OUTPUT parameter: This
     * parameter is updated to reflect changes in the length of the
     * text, but points to the same logical position in the text.
     */
    int32_t contextLimit;
    
    /**
     * Beginning index, inclusive, of the text to be transliteratd.
     * INPUT/OUTPUT parameter: This parameter is advanced past
     * characters that have already been transliterated by a
     * transliteration operation.
     */
    int32_t start;
    
    /**
     * Ending index, exclusive, of the text to be transliteratd.
     * INPUT/OUTPUT parameter: This parameter is updated to reflect
     * changes in the length of the text, but points to the same
     * logical position in the text.
     */
    int32_t limit;

} UTransPosition;

/********************************************************************
 * General API
 ********************************************************************/

/**
 * Open a custom transliterator, given a custom rules string 
 * OR 
 * a system transliterator, given its ID.  
 * Any non-NULL result from this function should later be closed with
 * utrans_close().
 *
 * @param id a valid ID, as returned by utrans_getAvailableID()
 * @param dir the desired direction
 * @param rules the transliterator rules.  See the C++ header rbt.h
 * for rules syntax. If NULL then a system transliterator matching 
 * the ID is returned.
 * @param rulesLength the length of the rules, or -1 if the rules
 * are zero-terminated.
 * @param dir the desired direction
 * @param parseErr a pointer to a UParseError struct to receive the
 * details of any parsing errors. This parameter may be NULL if no
 * parsing error details are desired.
 * @param status a pointer to the UErrorCode
 * @return a transliterator pointer that may be passed to other
 * utrans_xxx() functions, or NULL if the open call fails.
 * @draft ICU 2.0
 */
U_CAPI UTransliterator* U_EXPORT2 
utrans_open(const char* id,
            UTransDirection dir,
            const UChar* rules,         /* may be Null */
            int32_t rulesLength,        /* -1 if null-terminated */ 
            UParseError* parseError,    /* may be Null */
            UErrorCode* status);

/**
 * Open an inverse of an existing transliterator.  For this to work,
 * the inverse must be registered with the system.  For example, if
 * the Transliterator "A-B" is opened, and then its inverse is opened,
 * the result is the Transliterator "B-A", if such a transliterator is
 * registered with the system.  Otherwise the result is NULL and a
 * failing UErrorCode is set.  Any non-NULL result from this function
 * should later be closed with utrans_close().
 *
 * @param trans the transliterator to open the inverse of.
 * @param status a pointer to the UErrorCode
 * @return a pointer to a newly-opened transliterator that is the
 * inverse of trans, or NULL if the open call fails.
 * @stable
 */
U_CAPI UTransliterator* U_EXPORT2 
utrans_openInverse(const UTransliterator* trans,
                   UErrorCode* status);

/**
 * Create a copy of a transliterator.  Any non-NULL result from this
 * function should later be closed with utrans_close().
 *
 * @param trans the transliterator to be copied.
 * @param status a pointer to the UErrorCode
 * @return a transliterator pointer that may be passed to other
 * utrans_xxx() functions, or NULL if the clone call fails.
 * @stable
 */
U_CAPI UTransliterator* U_EXPORT2 
utrans_clone(const UTransliterator* trans,
             UErrorCode* status);

/**
 * Close a transliterator.  Any non-NULL pointer returned by
 * utrans_openXxx() or utrans_clone() should eventually be closed.
 * @param trans the transliterator to be closed.
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_close(UTransliterator* trans);

/**
 * Return the programmatic identifier for this transliterator.
 * If this identifier is passed to utrans_open(), it will open
 * a transliterator equivalent to this one, if the ID has been
 * registered.
 * @param trans the transliterator to return the ID of.
 * @param buf the buffer in which to receive the ID.  This may be
 * NULL, in which case no characters are copied.
 * @param bufCapacity the capacity of the buffer.  Ignored if buf is
 * NULL.
 * @return the actual length of the ID, not including
 * zero-termination.  This may be greater than bufCapacity.
 * @stable
 */
U_CAPI int32_t U_EXPORT2 
utrans_getID(const UTransliterator* trans,
             char* buf,
             int32_t bufCapacity);

/**
 * Register an open transliterator with the system.  When
 * utrans_open() is called with an ID string that is equal to that
 * returned by utrans_getID(adoptedTrans,...), then
 * utrans_clone(adoptedTrans,...) is returned.
 *
 * <p>NOTE: After this call the system owns the adoptedTrans and will
 * close it.  The user must not call utrans_close() on adoptedTrans.
 *
 * @param adoptedTrans a transliterator, typically the result of
 * utrans_openRules(), to be registered with the system.
 * @param status a pointer to the UErrorCode
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_register(UTransliterator* adoptedTrans,
                UErrorCode* status);

/**
 * Unregister a transliterator from the system.  After this call the
 * system will no longer recognize the given ID when passed to
 * utrans_open().  If the id is invalid then nothing is done.
 *
 * @param id a zero-terminated ID
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_unregister(const char* id);

/**
 * Set the filter used by a transliterator.  A filter can be used to
 * make the transliterator pass certain characters through untouched.
 * The filter is expressed using a UnicodeSet pattern.  If the
 * filterPattern is NULL or the empty string, then the transliterator
 * will be reset to use no filter.
 *
 * @param trans the transliterator
 * @param filterPattern a pattern string, in the form accepted by
 * UnicodeSet, specifying which characters to apply the
 * transliteration to.  May be NULL or the empty string to indicate no
 * filter.
 * @param filterPatternLen the length of filterPattern, or -1 if
 * filterPattern is zero-terminated
 * @param status a pointer to the UErrorCode
 * @see UnicodeSet
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_setFilter(UTransliterator* trans,
                 const UChar* filterPattern,
                 int32_t filterPatternLen,
                 UErrorCode* status);

/**
 * Return the number of system transliterators.
 * @return the number of system transliterators.
 * @stable
 */
U_CAPI int32_t U_EXPORT2 
utrans_countAvailableIDs(void);

/**
 * Return the ID of the index-th system transliterator.  The result
 * is placed in the given buffer.  If the given buffer is too small,
 * the initial substring is copied to buf.  The result in buf is
 * always zero-terminated.
 *
 * @param index the number of the transliterator to return.  Must
 * satisfy 0 <= index < utrans_countAvailableIDs().  If index is out
 * of range then it is treated as if it were 0.
 * @param buf the buffer in which to receive the ID.  This may be
 * NULL, in which case no characters are copied.
 * @param bufCapacity the capacity of the buffer.  Ignored if buf is
 * NULL.
 * @return the actual length of the index-th ID, not including
 * zero-termination.  This may be greater than bufCapacity.
 * @stable
 */
U_CAPI int32_t U_EXPORT2 
utrans_getAvailableID(int32_t index,
                      char* buf,
                      int32_t bufCapacity);

/********************************************************************
 * Transliteration API
 ********************************************************************/

/**
 * Transliterate a segment of a UReplaceable string.  The string is
 * passed in as a UReplaceable pointer rep and a UReplaceableCallbacks
 * function pointer struct repFunc.  Functions in the repFunc struct
 * will be called in order to modify the rep string.
 *
 * @param trans the transliterator
 * @param rep a pointer to the string.  This will be passed to the
 * repFunc functions.
 * @param repFunc a set of function pointers that will be used to
 * modify the string pointed to by rep.
 * @param start the beginning index, inclusive; <code>0 <= start <=
 * limit</code>.
 * @param limit pointer to the ending index, exclusive; <code>start <=
 * limit <= repFunc->length(rep)</code>.  Upon return, *limit will
 * contain the new limit index.  The text previously occupying
 * <code>[start, limit)</code> has been transliterated, possibly to a
 * string of a different length, at <code>[start,
 * </code><em>new-limit</em><code>)</code>, where <em>new-limit</em>
 * is the return value.
 * @param status a pointer to the UErrorCode
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_trans(const UTransliterator* trans,
             UReplaceable* rep,
             UReplaceableCallbacks* repFunc,
             int32_t start,
             int32_t* limit,
             UErrorCode* status);

/**
 * Transliterate the portion of the UReplaceable text buffer that can
 * be transliterated unambiguosly.  This method is typically called
 * after new text has been inserted, e.g. as a result of a keyboard
 * event.  The transliterator will try to transliterate characters of
 * <code>rep</code> between <code>index.cursor</code> and
 * <code>index.limit</code>.  Characters before
 * <code>index.cursor</code> will not be changed.
 *
 * <p>Upon return, values in <code>index</code> will be updated.
 * <code>index.start</code> will be advanced to the first
 * character that future calls to this method will read.
 * <code>index.cursor</code> and <code>index.limit</code> will
 * be adjusted to delimit the range of text that future calls to
 * this method may change.
 *
 * <p>Typical usage of this method begins with an initial call
 * with <code>index.start</code> and <code>index.limit</code>
 * set to indicate the portion of <code>text</code> to be
 * transliterated, and <code>index.cursor == index.start</code>.
 * Thereafter, <code>index</code> can be used without
 * modification in future calls, provided that all changes to
 * <code>text</code> are made via this method.
 *
 * <p>This method assumes that future calls may be made that will
 * insert new text into the buffer.  As a result, it only performs
 * unambiguous transliterations.  After the last call to this method,
 * there may be untransliterated text that is waiting for more input
 * to resolve an ambiguity.  In order to perform these pending
 * transliterations, clients should call utrans_trans() with a start
 * of index.start and a limit of index.end after the last call to this
 * method has been made.
 *
 * @param trans the transliterator
 * @param rep a pointer to the string.  This will be passed to the
 * repFunc functions.
 * @param repFunc a set of function pointers that will be used to
 * modify the string pointed to by rep.
 * @param pos a struct containing the start and limit indices of the
 * text to be read and the text to be transliterated
 * @param status a pointer to the UErrorCode
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_transIncremental(const UTransliterator* trans,
                        UReplaceable* rep,
                        UReplaceableCallbacks* repFunc,
                        UTransPosition* pos,
                        UErrorCode* status);

/**
 * Transliterate a segment of a UChar* string.  The string is passed
 * in in a UChar* buffer.  The string is modified in place.  If the
 * result is longer than textCapacity, it is truncated.  The actual
 * length of the result is returned in *textLength, if textLength is
 * non-NULL. *textLength may be greater than textCapacity, but only
 * textCapacity UChars will be written to *text, including the zero
 * terminator.
 *
 * @param trans the transliterator
 * @param text a pointer to a buffer containing the text to be
 * transliterated on input and the result text on output.
 * @param textLength a pointer to the length of the string in text.
 * If the length is -1 then the string is assumed to be
 * zero-terminated.  Upon return, the new length is stored in
 * *textLength.  If textLength is NULL then the string is assumed to
 * be zero-terminated.
 * @param textCapacity a pointer to the length of the text buffer.
 * Upon return, 
 * @param start the beginning index, inclusive; <code>0 <= start <=
 * limit</code>.
 * @param limit pointer to the ending index, exclusive; <code>start <=
 * limit <= repFunc->length(rep)</code>.  Upon return, *limit will
 * contain the new limit index.  The text previously occupying
 * <code>[start, limit)</code> has been transliterated, possibly to a
 * string of a different length, at <code>[start,
 * </code><em>new-limit</em><code>)</code>, where <em>new-limit</em>
 * is the return value.
 * @param status a pointer to the UErrorCode
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_transUChars(const UTransliterator* trans,
                   UChar* text,
                   int32_t* textLength,
                   int32_t textCapacity,
                   int32_t start,
                   int32_t* limit,
                   UErrorCode* status);

/**
 * Transliterate the portion of the UChar* text buffer that can be
 * transliterated unambiguosly.  See utrans_transIncremental().  The
 * string is passed in in a UChar* buffer.  The string is modified in
 * place.  If the result is longer than textCapacity, it is truncated.
 * The actual length of the result is returned in *textLength, if
 * textLength is non-NULL. *textLength may be greater than
 * textCapacity, but only textCapacity UChars will be written to
 * *text, including the zero terminator.  See utrans_transIncremental()
 * for usage details.
 *
 * @param trans the transliterator
 * @param text a pointer to a buffer containing the text to be
 * transliterated on input and the result text on output.
 * @param textLength a pointer to the length of the string in text.
 * If the length is -1 then the string is assumed to be
 * zero-terminated.  Upon return, the new length is stored in
 * *textLength.  If textLength is NULL then the string is assumed to
 * be zero-terminated.
 * @param textCapacity the length of the text buffer
 * @param pos a struct containing the start and limit indices of the
 * text to be read and the text to be transliterated
 * @param status a pointer to the UErrorCode
 * @see utrans_transIncremental
 * @stable
 */
U_CAPI void U_EXPORT2 
utrans_transIncrementalUChars(const UTransliterator* trans,
                              UChar* text,
                              int32_t* textLength,
                              int32_t textCapacity,
                              UTransPosition* pos,
                              UErrorCode* status);


/********************* Deprecated API ************************************/
/**
 *@deprecated Remove after Aug 2002
 */

#ifdef U_USE_DEPRECATED_FORMAT_API

#if ((U_ICU_VERSION_MAJOR_NUM != 2) || (U_ICU_VERSION_MINOR_NUM != 2))
#   error "ICU version has changed. Please redefine the macros under U_USE_DEPRECATED_FORMAT_API pre-processor definition"
#else 
    U_CAPI UTransliterator* U_EXPORT2 
    utrans_openRules(const char* id,
                     const UChar* rules,
                     int32_t rulesLength, /* -1 if null-terminated */
                     UTransDirection dir,
                     UParseError* parseErr, /* may be NULL */
                     UErrorCode* status){
        return utrans_open(id,dir,rules,rulesLength,parseErr,status);
    }

#   define utrans_open_2_2(id,dir,status) utrans_open(id,dir,NULL,0,NULL,status)

#endif
#endif
/********************* End **********************************************/

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
