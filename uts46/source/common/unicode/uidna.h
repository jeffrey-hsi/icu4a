/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  uidna.h
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003feb1
 *   created by: Ram Viswanadha
 */

#ifndef __UIDNA_H__
#define __UIDNA_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/parseerr.h"

/**
 * \file
 * \brief C API: Internationalized Domain Names in Applications Tranformation
 *
 * IDNA2008 is implemented according to UTS #46, see the IDNA C++ class in idna.h.
 * The old and new APIs share some of the option bit set values.
 * The old C API functions below which do not take a service object pointer
 * implement IDNA2003.
 *
 * IDNA2003 API Overview:
 *
 * UIDNA API implements the IDNA protocol as defined in the IDNA RFC 
 * (http://www.ietf.org/rfc/rfc3490.txt).
 * The RFC defines 2 operations: ToASCII and ToUnicode. Domain labels 
 * containing non-ASCII code points are required to be processed by
 * ToASCII operation before passing it to resolver libraries. Domain names
 * that are obtained from resolver libraries are required to be processed by
 * ToUnicode operation before displaying the domain name to the user.
 * IDNA requires that implementations process input strings with Nameprep
 * (http://www.ietf.org/rfc/rfc3491.txt), 
 * which is a profile of Stringprep (http://www.ietf.org/rfc/rfc3454.txt), 
 * and then with Punycode (http://www.ietf.org/rfc/rfc3492.txt). 
 * Implementations of IDNA MUST fully implement Nameprep and Punycode; 
 * neither Nameprep nor Punycode are optional.
 * The input and output of ToASCII and ToUnicode operations are Unicode 
 * and are designed to be chainable, i.e., applying ToASCII or ToUnicode operations
 * multiple times to an input string will yield the same result as applying the operation
 * once.
 * ToUnicode(ToUnicode(ToUnicode...(ToUnicode(string)))) == ToUnicode(string) 
 * ToASCII(ToASCII(ToASCII...(ToASCII(string))) == ToASCII(string).
 */

/*
 * IDNA option bit set values.
 */
enum {
    /**
     * Default options value: None of the other options are set.
     * @stable ICU 2.6
     */
    UIDNA_DEFAULT=0,
    /**
     * Option to allow unassigned code points in domain names and labels.
     * This option is ignored by the UTS46 implementation.
     * (UTS #46 disallows unassigned code points.)
     * @stable ICU 2.6
     */
    UIDNA_ALLOW_UNASSIGNED=1,
    /**
     * Option to check whether the input conforms to the STD3 ASCII rules,
     * for example the restriction of labels to LDH characters
     * (ASCII Letters, Digits and Hyphen-Minus).
     * @stable ICU 2.6
     */
    UIDNA_USE_STD3_RULES=2,
    /**
     * IDNA option to check for whether the input conforms to the BiDi rules.
     * This option is ignored by the IDNA2003 implementation.
     * (IDNA2003 always performs a BiDi check.)
     * @draft ICU 4.6
     */
    UIDNA_CHECK_BIDI=4,
    /**
     * IDNA option to check for whether the input conforms to the CONTEXTJ rules.
     * This option is ignored by the IDNA2003 implementation.
     * (The CONTEXTJ check is new in IDNA2008.)
     * @draft ICU 4.6
     */
    UIDNA_CHECK_CONTEXTJ=8,
    /**
     * IDNA option for nontransitional processing in ToASCII().
     * By default, ToASCII() uses transitional processing.
     * This option is ignored by the IDNA2003 implementation.
     * (This is only relevant for compatibility of newer IDNA implementations with IDNA2003.)
     * @draft ICU 4.6
     */
    UIDNA_NONTRANSITIONAL_TO_ASCII=0x10,
    /**
     * IDNA option for nontransitional processing in ToUnicode().
     * By default, ToUnicode() uses transitional processing.
     * This option is ignored by the IDNA2003 implementation.
     * (This is only relevant for compatibility of newer IDNA implementations with IDNA2003.)
     * @draft ICU 4.6
     */
    UIDNA_NONTRANSITIONAL_TO_UNICODE=0x20
};

/**
 * IDNA2003: This function implements the ToASCII operation as defined in the IDNA RFC.
 * This operation is done on <b>single labels</b> before sending it to something that expects
 * ASCII names. A label is an individual part of a domain name. Labels are usually
 * separated by dots; e.g. "www.example.com" is composed of 3 labels "www","example", and "com".
 *
 * @param src               Input UChar array containing label in Unicode.
 * @param srcLength         Number of UChars in src, or -1 if NUL-terminated.
 * @param dest              Output UChar array with ASCII (ACE encoded) label.
 * @param destCapacity      Size of dest.
 * @param options           A bit set of options:
 *
 *  - UIDNA_DEFAULT             Use default options, i.e., do not process unassigned code points
 *                              and do not use STD3 ASCII rules
 *                              If unassigned code points are found the operation fails with 
 *                              U_UNASSIGNED_ERROR error code.
 *
 *  - UIDNA_ALLOW_UNASSIGNED    Unassigned values can be converted to ASCII for query operations
 *                              If this option is set, the unassigned code points are in the input 
 *                              are treated as normal Unicode code points.
 *
 *  - UIDNA_USE_STD3_RULES      Use STD3 ASCII rules for host name syntax restrictions
 *                              If this option is set and the input does not satisfy STD3 rules,  
 *                              the operation will fail with U_IDNA_STD3_ASCII_RULES_ERROR
 *
 * @param parseError        Pointer to UParseError struct to receive information on position 
 *                          of error if an error is encountered. Can be NULL.
 * @param status            ICU in/out error code parameter.
 *                          U_INVALID_CHAR_FOUND if src contains
 *                          unmatched single surrogates.
 *                          U_INDEX_OUTOFBOUNDS_ERROR if src contains
 *                          too many code points.
 *                          U_BUFFER_OVERFLOW_ERROR if destCapacity is not enough
 * @return The length of the result string, if successful - or in case of a buffer overflow,
 *         in which case it will be greater than destCapacity.
 * @stable ICU 2.6
 */
U_STABLE int32_t U_EXPORT2
uidna_toASCII(const UChar* src, int32_t srcLength, 
              UChar* dest, int32_t destCapacity,
              int32_t options,
              UParseError* parseError,
              UErrorCode* status);


/**
 * IDNA2003: This function implements the ToUnicode operation as defined in the IDNA RFC.
 * This operation is done on <b>single labels</b> before sending it to something that expects
 * Unicode names. A label is an individual part of a domain name. Labels are usually
 * separated by dots; for e.g. "www.example.com" is composed of 3 labels "www","example", and "com".
 *
 * @param src               Input UChar array containing ASCII (ACE encoded) label.
 * @param srcLength         Number of UChars in src, or -1 if NUL-terminated.
 * @param dest Output       Converted UChar array containing Unicode equivalent of label.
 * @param destCapacity      Size of dest.
 * @param options           A bit set of options:
 *
 *  - UIDNA_DEFAULT             Use default options, i.e., do not process unassigned code points
 *                              and do not use STD3 ASCII rules
 *                              If unassigned code points are found the operation fails with 
 *                              U_UNASSIGNED_ERROR error code.
 *
 *  - UIDNA_ALLOW_UNASSIGNED      Unassigned values can be converted to ASCII for query operations
 *                              If this option is set, the unassigned code points are in the input 
 *                              are treated as normal Unicode code points. <b> Note: </b> This option is 
 *                              required on toUnicode operation because the RFC mandates 
 *                              verification of decoded ACE input by applying toASCII and comparing
 *                              its output with source
 *
 *  - UIDNA_USE_STD3_RULES      Use STD3 ASCII rules for host name syntax restrictions
 *                              If this option is set and the input does not satisfy STD3 rules,  
 *                              the operation will fail with U_IDNA_STD3_ASCII_RULES_ERROR
 *
 * @param parseError        Pointer to UParseError struct to receive information on position 
 *                          of error if an error is encountered. Can be NULL.
 * @param status            ICU in/out error code parameter.
 *                          U_INVALID_CHAR_FOUND if src contains
 *                          unmatched single surrogates.
 *                          U_INDEX_OUTOFBOUNDS_ERROR if src contains
 *                          too many code points.
 *                          U_BUFFER_OVERFLOW_ERROR if destCapacity is not enough
 * @return The length of the result string, if successful - or in case of a buffer overflow,
 *         in which case it will be greater than destCapacity.
 * @stable ICU 2.6
 */
U_STABLE int32_t U_EXPORT2
uidna_toUnicode(const UChar* src, int32_t srcLength,
                UChar* dest, int32_t destCapacity,
                int32_t options,
                UParseError* parseError,
                UErrorCode* status);


/**
 * IDNA2003: Convenience function that implements the IDNToASCII operation as defined in the IDNA RFC.
 * This operation is done on complete domain names, e.g: "www.example.com". 
 * It is important to note that this operation can fail. If it fails, then the input 
 * domain name cannot be used as an Internationalized Domain Name and the application
 * should have methods defined to deal with the failure.
 *
 * <b>Note:</b> IDNA RFC specifies that a conformant application should divide a domain name
 * into separate labels, decide whether to apply allowUnassigned and useSTD3ASCIIRules on each, 
 * and then convert. This function does not offer that level of granularity. The options once  
 * set will apply to all labels in the domain name
 *
 * @param src               Input UChar array containing IDN in Unicode.
 * @param srcLength         Number of UChars in src, or -1 if NUL-terminated.
 * @param dest              Output UChar array with ASCII (ACE encoded) IDN.
 * @param destCapacity      Size of dest.
 * @param options           A bit set of options:
 *
 *  - UIDNA_DEFAULT             Use default options, i.e., do not process unassigned code points
 *                              and do not use STD3 ASCII rules
 *                              If unassigned code points are found the operation fails with 
 *                              U_UNASSIGNED_CODE_POINT_FOUND error code.
 *
 *  - UIDNA_ALLOW_UNASSIGNED    Unassigned values can be converted to ASCII for query operations
 *                              If this option is set, the unassigned code points are in the input 
 *                              are treated as normal Unicode code points.
 *
 *  - UIDNA_USE_STD3_RULES      Use STD3 ASCII rules for host name syntax restrictions
 *                              If this option is set and the input does not satisfy STD3 rules,  
 *                              the operation will fail with U_IDNA_STD3_ASCII_RULES_ERROR
 *
 * @param parseError        Pointer to UParseError struct to receive information on position 
 *                          of error if an error is encountered. Can be NULL.
 * @param status            ICU in/out error code parameter.
 *                          U_INVALID_CHAR_FOUND if src contains
 *                          unmatched single surrogates.
 *                          U_INDEX_OUTOFBOUNDS_ERROR if src contains
 *                          too many code points.
 *                          U_BUFFER_OVERFLOW_ERROR if destCapacity is not enough
 * @return The length of the result string, if successful - or in case of a buffer overflow,
 *         in which case it will be greater than destCapacity.
 * @stable ICU 2.6
 */
U_STABLE int32_t U_EXPORT2
uidna_IDNToASCII(  const UChar* src, int32_t srcLength,
                   UChar* dest, int32_t destCapacity,
                   int32_t options,
                   UParseError* parseError,
                   UErrorCode* status);

/**
 * IDNA2003: Convenience function that implements the IDNToUnicode operation as defined in the IDNA RFC.
 * This operation is done on complete domain names, e.g: "www.example.com". 
 *
 * <b>Note:</b> IDNA RFC specifies that a conformant application should divide a domain name
 * into separate labels, decide whether to apply allowUnassigned and useSTD3ASCIIRules on each, 
 * and then convert. This function does not offer that level of granularity. The options once  
 * set will apply to all labels in the domain name
 *
 * @param src               Input UChar array containing IDN in ASCII (ACE encoded) form.
 * @param srcLength         Number of UChars in src, or -1 if NUL-terminated.
 * @param dest Output       UChar array containing Unicode equivalent of source IDN.
 * @param destCapacity      Size of dest.
 * @param options           A bit set of options:
 *
 *  - UIDNA_DEFAULT             Use default options, i.e., do not process unassigned code points
 *                              and do not use STD3 ASCII rules
 *                              If unassigned code points are found the operation fails with 
 *                              U_UNASSIGNED_CODE_POINT_FOUND error code.
 *
 *  - UIDNA_ALLOW_UNASSIGNED    Unassigned values can be converted to ASCII for query operations
 *                              If this option is set, the unassigned code points are in the input 
 *                              are treated as normal Unicode code points.
 *
 *  - UIDNA_USE_STD3_RULES      Use STD3 ASCII rules for host name syntax restrictions
 *                              If this option is set and the input does not satisfy STD3 rules,  
 *                              the operation will fail with U_IDNA_STD3_ASCII_RULES_ERROR
 *
 * @param parseError        Pointer to UParseError struct to receive information on position 
 *                          of error if an error is encountered. Can be NULL.
 * @param status            ICU in/out error code parameter.
 *                          U_INVALID_CHAR_FOUND if src contains
 *                          unmatched single surrogates.
 *                          U_INDEX_OUTOFBOUNDS_ERROR if src contains
 *                          too many code points.
 *                          U_BUFFER_OVERFLOW_ERROR if destCapacity is not enough
 * @return The length of the result string, if successful - or in case of a buffer overflow,
 *         in which case it will be greater than destCapacity.
 * @stable ICU 2.6
 */
U_STABLE int32_t U_EXPORT2
uidna_IDNToUnicode(  const UChar* src, int32_t srcLength,
                     UChar* dest, int32_t destCapacity,
                     int32_t options,
                     UParseError* parseError,
                     UErrorCode* status);

/**
 * IDNA2003: Compare two IDN strings for equivalence.
 * This function splits the domain names into labels and compares them.
 * According to IDN RFC, whenever two labels are compared, they are 
 * considered equal if and only if their ASCII forms (obtained by 
 * applying toASCII) match using an case-insensitive ASCII comparison.
 * Two domain names are considered a match if and only if all labels 
 * match regardless of whether label separators match.
 *
 * @param s1                First source string.
 * @param length1           Length of first source string, or -1 if NUL-terminated.
 *
 * @param s2                Second source string.
 * @param length2           Length of second source string, or -1 if NUL-terminated.
 * @param options           A bit set of options:
 *
 *  - UIDNA_DEFAULT             Use default options, i.e., do not process unassigned code points
 *                              and do not use STD3 ASCII rules
 *                              If unassigned code points are found the operation fails with 
 *                              U_UNASSIGNED_CODE_POINT_FOUND error code.
 *
 *  - UIDNA_ALLOW_UNASSIGNED    Unassigned values can be converted to ASCII for query operations
 *                              If this option is set, the unassigned code points are in the input 
 *                              are treated as normal Unicode code points.
 *
 *  - UIDNA_USE_STD3_RULES      Use STD3 ASCII rules for host name syntax restrictions
 *                              If this option is set and the input does not satisfy STD3 rules,  
 *                              the operation will fail with U_IDNA_STD3_ASCII_RULES_ERROR
 *
 * @param status            ICU error code in/out parameter.
 *                          Must fulfill U_SUCCESS before the function call.
 * @return <0 or 0 or >0 as usual for string comparisons
 * @stable ICU 2.6
 */
U_STABLE int32_t U_EXPORT2
uidna_compare(  const UChar *s1, int32_t length1,
                const UChar *s2, int32_t length2,
                int32_t options,
                UErrorCode* status);

#endif /* #if !UCONFIG_NO_IDNA */

#endif
