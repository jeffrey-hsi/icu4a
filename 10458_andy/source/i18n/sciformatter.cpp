/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: sciformatter.cpp
 */

#include "unicode/utypes.h"

#include "sciformatter.h"
#include "digitgrouping.h"
#include "unicode/dcfmtsym.h"
#include "unicode/unum.h"
#include "fphdlimp.h"
#include "numericvalue.h"

U_NAMESPACE_BEGIN

SciFormatter::SciFormatter() : fExponent("E") {
}

SciFormatter::SciFormatter(const DecimalFormatSymbols &symbols) {
    setDecimalFormatSymbols(symbols);
}

void
SciFormatter::setDecimalFormatSymbols(
        const DecimalFormatSymbols &symbols) {
fExponent = symbols.getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
}

UnicodeString &
SciFormatter::format(
        const NumericValue &value,
        const DigitFormatter &formatter,
        const SciFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    if (value.isNaN()) {
        return formatter.formatNaN(handler, appendTo);
    }
    if (value.isInfinite()) {
        return formatter.formatInfinity(handler, appendTo);
    }
    return format(
            value.fValue,
            value.fExponent,
            formatter,
            value.fInterval,
            options,
            handler,
            appendTo);
}

UnicodeString &
SciFormatter::format(
        const DigitList &positiveMantissa,
        int32_t exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    DigitGrouping grouping;
    formatter.format(
            positiveMantissa,
            grouping,
            mantissaInterval,
            options.fMantissa,
            handler,
            appendTo);
    int32_t expBegin = appendTo.length();
    appendTo.append(fExponent);
    handler.addAttribute(
            UNUM_EXPONENT_SYMBOL_FIELD, expBegin, appendTo.length());
    return formatter.formatInt32(
            exponent,
            options.fExponent,
            UNUM_EXPONENT_SIGN_FIELD,
            UNUM_EXPONENT_FIELD,
            handler,
            appendTo);
}

int32_t
SciFormatter::countChar32(
        const NumericValue &value,
        const DigitFormatter &formatter,
        const SciFormatterOptions &options) const {
    if (value.isNaN()) {
        return formatter.countChar32ForNaN();
    }
    if (value.isInfinite()) {
        return formatter.countChar32ForInfinity();
    }
    return countChar32(value.fExponent, formatter, value.fInterval, options);
}

int32_t
SciFormatter::countChar32(
        int32_t exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options) const {
    DigitGrouping grouping;
    int32_t count = formatter.countChar32(
            grouping, mantissaInterval, options.fMantissa);
    count += fExponent.countChar32();
    count += formatter.countChar32ForInt32(
            exponent, options.fExponent);
    return count;
}


U_NAMESPACE_END

