/*
**********************************************************************
*   Copyright (C) 1997-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File DIGITLST.CPP
*
* Modification History:
*
*   Date        Name        Description
*   03/21/97    clhuang     Converted from java.
*   03/21/97    clhuang     Implemented with new APIs.
*   03/27/97    helena      Updated to pass the simple test after code review.
*   03/31/97    aliu        Moved isLONG_MIN to here, and fixed it.
*   04/15/97    aliu        Changed MAX_COUNT to DBL_DIG.  Changed Digit to char.
*                           Reworked representation by replacing fDecimalAt
*                           with fExponent.
*   04/16/97    aliu        Rewrote set() and getDouble() to use sprintf/atof
*                           to do digit conversion.
*   09/09/97    aliu        Modified for exponential notation support.
*   08/02/98    stephen     Added nearest/even rounding
*                            Fixed bug in fitsIntoLong
******************************************************************************
*/

#include "digitlst.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "putilimp.h"
#include "uassert.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

// ***************************************************************************
// class DigitList
//    A wrapper onto decNumber.
//    Used to be standalone.
// ***************************************************************************

/**
 * This is the zero digit.  The base for the digits returned by getDigit()
 */
#define kZero '0'

static char gDecimal = 0;

/* Only for 32 bit numbers. Ignore the negative sign. */
static const char LONG_MIN_REP[] = "2147483648";
static const char I64_MIN_REP[] = "9223372036854775808";

enum {
    LONG_MIN_REP_LENGTH = sizeof(LONG_MIN_REP) - 1, //Ignore the NULL at the end
    I64_MIN_REP_LENGTH = sizeof(I64_MIN_REP) - 1,   //Ignore the NULL at the end
    DEFAULT_DIGITS = 40                             // Will grow as needed.
};

U_NAMESPACE_BEGIN

// -------------------------------------
// default constructor

DigitList::DigitList()
{
    uprv_decContextDefault(&fContext, DEC_INIT_BASE);
    fContext.traps  = 0;
    uprv_decContextSetRounding(&fContext, DEC_ROUND_HALF_EVEN);
    fContext.digits = DEFAULT_DIGITS;

    int32_t numSize = sizeof(decNumber) + DEFAULT_DIGITS;
    fDecNumber = static_cast<decNumber *>(uprv_malloc(numSize));
    // TODO:  do something with allocation failures.
    
    uprv_decNumberZero(fDecNumber);
}

// -------------------------------------

DigitList::~DigitList()
{
    uprv_free(fDecNumber);
    fDecNumber = NULL;
}

// -------------------------------------
// copy constructor

DigitList::DigitList(const DigitList &other)
{
    fDecNumber = NULL;
    *this = other;
}

// -------------------------------------
// assignment operator

DigitList&
DigitList::operator=(const DigitList& other)
{
    if (this != &other)
    {
        uprv_memcpy(&fContext, &other.fContext, sizeof(decContext));

        uprv_free(fDecNumber);
        int32_t numSize = sizeof(decNumber) + DEFAULT_DIGITS;
        fDecNumber = static_cast<decNumber *>(uprv_malloc(numSize));
        uprv_decNumberCopy(fDecNumber, other.fDecNumber);
    }
    return *this;
}

// -------------------------------------
//    operator ==  (does not exactly match the old DigitList function)

UBool
DigitList::operator==(const DigitList& that) const
{
    if (this == &that) {
        return TRUE;
    }
    decNumber n;  // Has space for only a none digit value.
    decContext c;
    uprv_decContextDefault(&c, DEC_INIT_BASE);
    c.digits = 1;
    c.traps = 0;

    uprv_decNumberCompare(&n, this->fDecNumber, that.fDecNumber, &c);
    UBool result = decNumberIsZero(&n);
    return result;
}

// -------------------------------------
// Resets the digit list; sets all the digits to zero.

void
DigitList::clear()
{
    uprv_decNumberZero(fDecNumber);
    uprv_decContextSetRounding(&fContext, DEC_ROUND_HALF_EVEN);
}

// -------------------------------------

void 
DigitList::setRoundingMode(DecimalFormat::ERoundingMode m) {
    enum rounding r;

    switch (m) {
      case  DecimalFormat::kRoundCeiling:  r = DEC_ROUND_CEILING;   break;
      case  DecimalFormat::kRoundFloor:    r = DEC_ROUND_FLOOR;     break;
      case  DecimalFormat::kRoundDown:     r = DEC_ROUND_DOWN;      break;
      case  DecimalFormat::kRoundUp:       r = DEC_ROUND_UP;        break;
      case  DecimalFormat::kRoundHalfEven: r = DEC_ROUND_HALF_EVEN; break;
      case  DecimalFormat::kRoundHalfDown: r = DEC_ROUND_HALF_DOWN; break;
      case  DecimalFormat::kRoundHalfUp:   r = DEC_ROUND_HALF_UP;   break;
      default:
         // TODO: how to report the problem?
         // Leave existing mode unchanged.
         r = uprv_decContextGetRounding(&fContext);
    }
    uprv_decContextSetRounding(&fContext, r);
  
}

// -------------------------------------
//  Set to zero, but preserve sign 
void  
DigitList::setToZero() {
   uint8_t signbit = fDecNumber->bits & DECNEG;
   uprv_decNumberZero(fDecNumber);
   fDecNumber->bits |= signbit; 
}

// -------------------------------------

void  
DigitList::setPositive(UBool s) {
    if (s) {
        fDecNumber->bits &= ~DECNEG; 
    } else {
        fDecNumber->bits |= DECNEG;
    }
}
// -------------------------------------

void     
DigitList::setDecimalAt(int32_t d) {
    U_ASSERT((fDecNumber->bits & DECSPECIAL) == 0);  // Not Infinity or NaN
    U_ASSERT(d-1>-999999999);
    U_ASSERT(d-1< 999999999);
    fDecNumber->exponent = d - fDecNumber->digits;
}

int32_t  
DigitList::getDecimalAt() {
    U_ASSERT((fDecNumber->bits & DECSPECIAL) == 0);  // Not Infinity or NaN
    if (decNumberIsZero(fDecNumber) || (fDecNumber->bits & DECSPECIAL != 0)) {
        return fDecNumber->exponent;  // Exponent should be zero for these cases.
    }
    return fDecNumber->exponent + fDecNumber->digits;
}

void     
DigitList::setCount(int32_t c)  {
    U_ASSERT(c <= fContext.digits);
    if (c == 0) {
        // For a value of zero, DigitList sets all fields to zero, while
        // decNumber keeps one digit (with that digit being a zero)
        c = 1;
    }
    fDecNumber->digits = c;
}

int32_t  
DigitList::getCount() {
    if (decNumberIsZero(fDecNumber)) {
       return 0;
    } else {
       return fDecNumber->digits;
    }
}
    
void     
DigitList::setDigit(int32_t i, char v) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    U_ASSERT(v>='0' && v<='9');
    v &= 0x0f;
    fDecNumber->lsu[count-i-1] = v;
}

char     
DigitList::getDigit(int32_t i) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    return fDecNumber->lsu[count-i-1] + '0';
}

// -------------------------------------
// Appends the digit to the digit list if it's not out of scope.
// Ignores the digit, otherwise.
// 
// This function is horribly inefficient to do with decNumber because
// the digits are stored least significant first, which requires moving all
// existing digits down one to make space for the new one to be appended.
//
// TODO:  redo parsing functions to not use this.
//        Do not enable big decimal parsing until then.
void
DigitList::append(char digit)
{
    U_ASSERT(digit>='0' && digit<='9');
    // Ignore digits which exceed the precision we can represent
    //    And don't fix for larger precision.  Fix callers instead.
    int32_t nDigits = fDecNumber->digits;
    if (nDigits < fContext.digits) {
        int i;
        for (i=nDigits; i>0; i--) {
            fDecNumber->lsu[i] = fDecNumber->lsu[i-1];
        }
        fDecNumber->lsu[0] = digit & 0x0f;
    }
}

// -------------------------------------

/**
 * Currently, getDouble() depends on atof() to do its conversion.
 *
 * WARNING!!
 * This is an extremely costly function. ~1/2 of the conversion time
 * can be linked to this function.
 */
double
DigitList::getDouble() /*const*/
{
    double value;

    if (gDecimal == 0) {
        char rep[MAX_DIGITS];
        // For machines that decide to change the decimal on you,
        // and try to be too smart with localization.
        // This normally should be just a '.'.
        sprintf(rep, "%+1.1f", 1.0);
        gDecimal = rep[2];
    }

    if (decNumberIsZero(fDecNumber)) {
        // TODO:  worry about sign?
        value = 0.0;
    }
    else {
        MaybeStackArray<char, MAX_DIGITS+15> s;
           // Note:  14 is a  magic constant from the decNumber library documentation,
           //        the max number of extra characters beyond the number of digits 
           //        needed to represent the number in string form.
        if (fDecNumber->digits > MAX_DIGITS) {
            s.resize(fDecNumber->digits+15);
        }
        uprv_decNumberToString(fDecNumber, s);
        
        if (gDecimal != '.') {
            char *decimalPt = strchr(s, '.');
            if (decimalPt != NULL) {
                *decimalPt = gDecimal;
            }
        }
        char *end = NULL;
        value = uprv_strtod(s, &end);
    }
    return value;
}

// -------------------------------------

/**
 * Make sure that fitsIntoLong() is called before calling this function.
 */
int32_t DigitList::getLong() /*const*/
{
    int32_t n = uprv_decNumberToInt32(fDecNumber, &fContext);
    return n;
}


/**
 *  convert this number to an int64_t.   Round if there is a fractional part.
 *  Return zero if the number cannot be represented.
 */
int64_t DigitList::getInt64() /*const*/ {
    // Round if non-integer.   (Truncate or round?)
    // Return 0 if out of range.
    // Range of in64_t is -9223372036854775808 to 9223372036854775807  (19 digits)
    //
    if (fDecNumber->exponent != 0) {
        DigitList intNum(*this);
        intNum.toIntegralValue();
        if (intNum.fDecNumber->exponent != 0) {
            // Number is too large to represent exactly as a decimal int, and _way_
            // to large to go in an int64.
            return 0;
        } else {
            // Recursive call; this time we know we have an int.
            return intNum.getInt64();
        }
    }

    int32_t numDigits = fDecNumber->digits;
    if (numDigits > 19) {
        // Overflow
        return 0;
    }

    uint64_t value = 0;
    for (int i = numDigits-1; i>=0 ; --i) {
        int v = fDecNumber->lsu[i];
        value = value * (uint64_t)10 + (uint64_t)v;
    }
    if (decNumberIsNegative(fDecNumber)) {
        value = ~value;
        value += 1;
    }
    int64_t svalue = (int64_t)value;

    // Check overflow.  It's convenient that the MSD is 9 only on overflow, the amount of
    //                  overflow can't wrap too far.  The test will also fail -0, but
    //                  that does no harm; the right answer is 0.
    if (numDigits == 15) {
        if (( decNumberIsNegative(fDecNumber) && svalue>0) ||
            (!decNumberIsNegative(fDecNumber) && svalue<0)) {
            value = 0;
        }
    }
        
    return svalue;
}

/**
 * Return true if this is an integer value that can be held
 * by an int32_t type.
 */
UBool
DigitList::fitsIntoLong(UBool ignoreNegativeZero) /*const*/
{
    // Figure out if the result will fit in a long.  We have to
    // first look for nonzero digits after the decimal point;
    // then check the size.

    // Trim trailing zeros after the decimal point. This does not change
    // the represented value.
    while (fCount > fDecimalAt && fCount > 0 && fDigits[fCount - 1] == kZero)
        --fCount;

    if (fCount == 0) {
        // Positive zero fits into a long, but negative zero can only
        // be represented as a double. - bug 4162852
        return fIsPositive || ignoreNegativeZero;
    }

    // If the digit list represents a double or this number is too
    // big for a long.
    if (fDecimalAt < fCount || fDecimalAt > LONG_MIN_REP_LENGTH)
        return FALSE;

    // If number is small enough to fit in a long
    if (fDecimalAt < LONG_MIN_REP_LENGTH)
        return TRUE;

    // At this point we have fDecimalAt == fCount, and fCount == LONG_MIN_REP_LENGTH.
    // The number will overflow if it is larger than LONG_MAX
    // or smaller than LONG_MIN.
    for (int32_t i=0; i<fCount; ++i)
    {
        char dig = fDigits[i],
             max = LONG_MIN_REP[i];
        if (dig > max)
            return FALSE;
        if (dig < max)
            return TRUE;
    }

    // At this point the first count digits match.  If fDecimalAt is less
    // than count, then the remaining digits are zero, and we return true.
    if (fCount < fDecimalAt)
        return TRUE;

    // Now we have a representation of Long.MIN_VALUE, without the leading
    // negative sign.  If this represents a positive value, then it does
    // not fit; otherwise it fits.
    return !fIsPositive;
}

/**
 * Return true if the number represented by this object can fit into
 * a long.
 */
UBool
DigitList::fitsIntoInt64(UBool ignoreNegativeZero) /*const*/
{
    // Figure out if the result will fit in a long.  We have to
    // first look for nonzero digits after the decimal point;
    // then check the size.

    // Trim trailing zeros after the decimal point. This does not change
    // the represented value.
    while (fCount > fDecimalAt && fCount > 0 && fDigits[fCount - 1] == kZero)
        --fCount;

    if (fCount == 0) {
        // Positive zero fits into a long, but negative zero can only
        // be represented as a double. - bug 4162852
        return fIsPositive || ignoreNegativeZero;
    }

    // If the digit list represents a double or this number is too
    // big for a long.
    if (fDecimalAt < fCount || fDecimalAt > I64_MIN_REP_LENGTH)
        return FALSE;

    // If number is small enough to fit in an int64
    if (fDecimalAt < I64_MIN_REP_LENGTH)
        return TRUE;

    // At this point we have fDecimalAt == fCount, and fCount == INT64_MIN_REP_LENGTH.
    // The number will overflow if it is larger than U_INT64_MAX
    // or smaller than U_INT64_MIN.
    for (int32_t i=0; i<fCount; ++i)
    {
        char dig = fDigits[i],
             max = I64_MIN_REP[i];
        if (dig > max)
            return FALSE;
        if (dig < max)
            return TRUE;
    }

    // At this point the first count digits match.  If fDecimalAt is less
    // than count, then the remaining digits are zero, and we return true.
    if (fCount < fDecimalAt)
        return TRUE;

    // Now we have a representation of INT64_MIN_VALUE, without the leading
    // negative sign.  If this represents a positive value, then it does
    // not fit; otherwise it fits.
    return !fIsPositive;
}


// -------------------------------------

void
DigitList::set(int32_t source, int32_t maximumDigits)
{
    set((int64_t)source, maximumDigits);
}

// -------------------------------------
/**
 * @param maximumDigits The maximum digits to be generated.  If zero,
 * there is no maximum -- generate all digits.
 */
void
DigitList::set(int64_t source, int32_t maximumDigits)
{
    fCount = fDecimalAt = formatBase10(source, fDecimalDigits, MAX_DIGITS);

    fIsPositive = (*fDecimalDigits == '+');

    // Don't copy trailing zeros
    while (fCount > 1 && fDigits[fCount - 1] == kZero)
        --fCount;

    if(maximumDigits > 0)
        round(maximumDigits);
}

/**
 * Set the digit list to a representation of the given double value.
 * This method supports both fixed-point and exponential notation.
 * @param source Value to be converted.
 * @param maximumDigits The most fractional or total digits which should
 * be converted.  If total digits, and the value is zero, then
 * there is no maximum -- generate all digits.
 * @param fixedPoint If true, then maximumDigits is the maximum
 * fractional digits to be converted.  If false, total digits.
 */
void
DigitList::set(double source, int32_t maximumDigits, UBool fixedPoint)
{
    // for now, simple implementation; later, do proper IEEE stuff
    char rep[MAX_DIGITS + 8]; // Extra space for '+', '.', e+NNN, and '\0' (actually +8 is enough)

    fIsPositive = !uprv_isNegative(source);    // Allow +0 and -0

    // Generate a representation of the form /[+-][0-9]+e[+-][0-9]+/
    sprintf(rep, "%+1.*e", MAX_DBL_DIGITS - 1, source);

    // Create a decNumber from the string.
    decNumberFromString(fDecNumber, rep, &fContext);

    if (maximumDigits > 0 && maximumDigits < fDecNumber.digits) {
        // Rounding may be needed.
        int32_t requestedDigits = maximumDigits;  // For floating point case.
        if (fixedPoint) {
            // Requested maximumDigits is the digits to the right of
            // the decimal.  Total digits in the rounded number may be greater.
            fractionDigits = -fDecNumber.exponent;
            requestedDigits = fDecNumber.digits;
            if (fractionDigits > 0) {
                requestedDigits = fDecNumber.digits - fractionDigits + requestedDigits;
            }
        }
        if (requestedDigits < fDecNumber.digits) {
            fDecNumber.round(requestedDigits);
        }
    }

    // TODO:  test the behavior of -0
}

// -------------------------------------

/**
 * Round the representation to the given number of digits.
 * @param maximumDigits The maximum number of digits to be shown.
 * Upon return, count will be less than or equal to maximumDigits.
 */
void
DigitList::round(int32_t maximumDigits)
{
    int32_t savedDigits fContext.digits;
    fContext.digits = maximumDigits;
    uprv_decNumberPlus(fDecNumber, fDecNumber, &fContext);
    fContext.digits = savedDigits;
}


// -------------------------------------

void
DigitList::toIntegralValue() {
    uprv_decNumberToIntegralValue(fDecNumber, fDecNumber, &fContext);
}


// -------------------------------------
UBool
DigitList::isZero() const
{
    return uprv_decNumberIsZero(fDecNumber);
}

U_NAMESPACE_END
#endif // #if !UCONFIG_NO_FORMATTING

//eof
