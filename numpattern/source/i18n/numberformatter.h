/*
********************************************************************************
*   Copyright (C) 2014, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*/
#ifndef NUMBERFORMATTER_H
#define NUMBERFORMATTER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class NumberFormat;
class IntFormatter;
class NumberIntFormatter;
class Formattable;
class PluralRules;
class FieldPosition;
class SharedNumberFormat;

/**
 * NumberFormatter provides a common interface for delegates of various types
 * that have a
 * format(Formattable &, UnicodeString &, FieldPosition, UErrorCode &) method.
 * NumberFormatter is not designed to be extended. Instead, it wraps a
 * delegate providing a common interface to it.
 *
 * Instances of this class can either borrow a delegate or own a delegate via
 * adoption. Delegates are never copied. Therefore, when an instance of this
 * class borrows a delegate, the delegate must outlive its enclosing instance.
 */
class U_I18N_API NumberFormatter : public UMemory {
public:
    NumberFormatter();
    NumberFormatter(const NumberFormat &nfToBorrow);
    NumberFormatter(const IntFormatter &nfToBorrow);
    NumberFormatter(const NumberIntFormatter &nfToBorrow);
    NumberFormatter(const SharedNumberFormat *borrowed);
    ~NumberFormatter();
    NumberFormatter &borrowNumberFormat(const NumberFormat &nfToBorrow);
    NumberFormatter &borrowIntFormatter(const IntFormatter &nfToBorrow);
    NumberFormatter &borrowNumberIntFormatter(
            const NumberIntFormatter &nfToBorrow);
    NumberFormatter &borrowSharedNumberFormat(
            const SharedNumberFormat *borrowed);
    NumberFormatter &adoptNumberFormat(NumberFormat *nfToAdopt);
    NumberFormatter &adoptIntFormatter(IntFormatter *nfToAdopt);
    NumberFormatter &adoptNumberIntFormatter(NumberIntFormatter *nfToAdopt);

    /**
      * Returns the enclosed IntFormatter or NULL if this instance does not
      * wrap an IntFormatter.
      */
    const IntFormatter *getIntFormatter() const;

    /**
      * Returns the enclosed NumberFormat or NULL if this instance does not
      * wrap an IntFormatter.
      */
    const NumberFormat *getNumberFormat() const;

    /**
      * Returns the enclosed NumberIntFormatter or NULL if this instance does
      * not wrap a NumberIntFormatter.
      */
    const NumberIntFormatter *getNumberIntFormatter() const;

    /**
     * Selects the plural form.
     */
    UnicodeString &select(
            const Formattable &quantity,
            const PluralRules &rules,
            UnicodeString &result,
            UErrorCode &status) const;

    /**
     * Calls format on underlying delegate.
     */
    UnicodeString &format(
            const Formattable &quantity,
            UnicodeString &appendTo, 
            FieldPosition &pos,
            UErrorCode &status) const;
private:
    enum Type {
        kUndefined,
        kNumberFormat,
        kIntFormatter,
        kNumberIntFormatter
    };
    union {
        const NumberFormat *numberFormat;
        const IntFormatter *intFormatter;
        const NumberIntFormatter *numberIntFormatter;
    } fDelegate;
    Type fDelegateType;
    UBool fDelegateOwned;
    NumberFormatter(const NumberFormatter &other);
    NumberFormatter &operator=(const NumberFormatter &other);
    void clear();
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // NUMBERFORMATTER_H
//eof
