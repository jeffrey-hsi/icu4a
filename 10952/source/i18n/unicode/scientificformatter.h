/*
**********************************************************************
* Copyright (c) 2014, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
*/
#ifndef SCIFORMATTER_H
#define SCIFORMATTER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#ifndef U_HIDE_DRAFT_API

#include "unicode/unistr.h"

/**
 * \file 
 * \brief C++ API: Formats in scientific notation.
 */

U_NAMESPACE_BEGIN

class FieldPositionIterator;
class DecimalFormatStaticSets;
class DecimalFormatSymbols;
class DecimalFormat;
class Formattable;

/**
 * A formatter that formats in user-friendly scientific notation.
 *
 * Sample code:
 * <pre>
 * UErrorCode status = U_ZERO_ERROR;
 * LocalPointer<ScientificFormatter> fmt(
 *         ScientificFormatter::createMarkupInstance(
 *                 "en", "<sup>", "</sup>", status));
 * if (U_FAILURE(status)) {
 *     return;
 * }
 * UnicodeString appendTo;
 * // appendTo = "1.23456x10<sup>-78</sup>"
 * fmt->format(1.23456e-78, appendTo, status);
 * </pre>
 *
 * @draft ICU 55
 */
class U_I18N_API ScientificFormatter : public UObject {
public:

    /**
     * Creates a ScientificFormatter instance that uses
     * superscript characters for exponents.
     * @param fmtToAdopt The DecimalFormat which must be configured for
     *   scientific notation.
     * @param status error returned here.
     * @return The new ScientificFormatter instance.
     *
     * @draft ICU 55
     */
    static ScientificFormatter *createSuperscriptInstance(
            DecimalFormat *fmtToAdopt, UErrorCode &status);

    /**
     * Creates a ScientificFormatter instance that uses
     * superscript characters for exponents for this locale.
     * @param locale The locale
     * @param status error returned here.
     * @return The ScientificFormatter instance.
     *
     * @draft ICU 55
     */
    static ScientificFormatter *createSuperscriptInstance(
            const Locale &locale, UErrorCode &status);


    /**
     * Creates a ScientificFormatter instance that uses
     * mark up for exponents.
     * @param fmtToAdopt The DecimalFormat which must be configured for
     *   scientific notation.
     * @param beginMarkup the mark up to start superscript.
     * @param endMarkup the mark up to end superscript.
     * @param status error returned here.
     * @return The new ScientificFormatter instance.
     *
     * @draft ICU 55
     */
    static ScientificFormatter *createMarkupInstance(
            DecimalFormat *fmtToAdopt,
            const UnicodeString &beginMarkup,
            const UnicodeString &endMarkup,
            UErrorCode &status);

    /**
     * Creates a ScientificFormatter instance that uses
     * mark up for exponents for this locale.
     * @param locale The locale
     * @param beginMarkup the mark up to start superscript.
     * @param endMarkup the mark up to end superscript.
     * @param status error returned here.
     * @return The ScientificFormatter instance.
     *
     * @draft ICU 55
     */
    static ScientificFormatter *createMarkupInstance(
            const Locale &locale,
            const UnicodeString &beginMarkup,
            const UnicodeString &endMarkup,
            UErrorCode &status);

    /**
     * Copy constructor.
     * @draft ICU 55
     */
    ScientificFormatter(const ScientificFormatter &other);

    /**
     * Assignment operator.
     * @draft ICU 55
     */
    ScientificFormatter &operator=(const ScientificFormatter &other);

    /**
     * Destructor.
     * @draft ICU 55
     */
    virtual ~ScientificFormatter();

    /**
     * Formats a number into user friendly scientific notation.
     *
     * @param number the number to format.
     * @param appendTo formatted string appended here.
     * @param status any error returned here.
     * @return appendTo
     *
     * @draft ICU 55
     */
    UnicodeString &format(
            const Formattable &number,
            UnicodeString &appendTo,
            UErrorCode &status) const;
 private:
    class U_I18N_API Style : public UObject {
    public:
        virtual Style *clone() const = 0;
    protected:
        virtual UnicodeString &format(
                const UnicodeString &original,
                FieldPositionIterator &fpi,
                const UnicodeString &preExponent,
                const DecimalFormatStaticSets &decimalFormatSets,
                UnicodeString &appendTo,
                UErrorCode &status) const = 0;
    private:
        friend class ScientificFormatter;
    };

    class U_I18N_API SuperscriptStyle : public Style {
    public:
        virtual Style *clone() const;
    protected:
        virtual UnicodeString &format(
                const UnicodeString &original,
                FieldPositionIterator &fpi,
                const UnicodeString &preExponent,
                const DecimalFormatStaticSets &decimalFormatSets,
                UnicodeString &appendTo,
                UErrorCode &status) const;
    private:
        friend class ScientificFormatHelper;
    };

    class U_I18N_API MarkupStyle : public Style {
    public:
        MarkupStyle(
                const UnicodeString &beginMarkup,
                const UnicodeString &endMarkup)
                : Style(),
                  fBeginMarkup(beginMarkup),
                  fEndMarkup(endMarkup) { }
        virtual Style *clone() const;
    protected:
        virtual UnicodeString &format(
                const UnicodeString &original,
                FieldPositionIterator &fpi,
                const UnicodeString &preExponent,
                const DecimalFormatStaticSets &decimalFormatSets,
                UnicodeString &appendTo,
                UErrorCode &status) const;
    private:
        UnicodeString fBeginMarkup;
        UnicodeString fEndMarkup;
        friend class ScientificFormatHelper;
    };

    ScientificFormatter(
            DecimalFormat *fmtToAdopt,
            Style *styleToAdopt,
            UErrorCode &status);

    static void getPreExponent(
            const DecimalFormatSymbols &dfs, UnicodeString &preExponent);

    static ScientificFormatter *createInstance(
            DecimalFormat *fmtToAdopt,
            Style *styleToAdopt,
            UErrorCode &status);

    UnicodeString fPreExponent;
    DecimalFormat *fDecimalFormat;
    Style *fStyle;
    const DecimalFormatStaticSets *fStaticSets;

    friend class ScientificFormatHelper;
};

U_NAMESPACE_END

#endif /* U_HIDE_DRAFT_API */

#endif /* !UCONFIG_NO_FORMATTING */
#endif 
