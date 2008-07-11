/*
 *******************************************************************************
 * Copyright (C) 2008, Google, International Business Machines Corporation and *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */

#ifndef __TMUTFMT_H__
#define __TMUTFMT_H__

#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: Format and parse duration in single time unit
 */


#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/tmunit.h"
#include "unicode/tmutamt.h"
#include "unicode/measfmt.h"
#include "unicode/numfmt.h"
#include "unicode/plurrule.h"

U_NAMESPACE_BEGIN

class Hashtable;


/**
 * Format or parse a TimeUnitAmount, using plural rules for the units where available.
 *
 * <P>
 * Code Sample: 
 * <pre>
 *   // create time unit amount instance - a combination of Number and time unit
 *   UErrorCode status = U_ZERO_ERROR;
 *   TimeUnitAmount* source = new TimeUnitAmount(2, TimeUnit::UTIMEUNIT_YEAR, status);
 *   // create time unit format instance
 *   TimeUnitFormat* format = new TimeUnitFormat(Locale("en"), status);
 *   // format a time unit amount
 *   UnicodeString formatted;
 *   Formattable formattable;
 *   if (U_SUCCESS(status)) {
 *       formattable.adoptObject(source);
 *       formatted = ((Format*)format)->format(formattable, formatted, status);
 *       Formattable result;
 *       ((Format*)format)->parseObject(formatted, result, status);
 *       if (U_SUCCESS(status)) {
 *           assert (result == formattable); 
 *       }
 *   }
 * </pre>
 *
 * <P>
 * @see TimeUnitAmount
 * @see TimeUnitFormat
 * @draft ICU 4.2
 */
class U_I18N_API TimeUnitFormat: public MeasureFormat {
public:
    /**
     * Create TimeUnitFormat with default locale. 
     * Use setLocale and/or setFormat to modify.
     * @draft ICU 4.2
     */
    TimeUnitFormat(UErrorCode& status);

    /**
     * Create TimeUnitFormat given locale.
     * @draft ICU 4.2
     */
    TimeUnitFormat(const Locale& locale, UErrorCode& status);

    /**
     * Copy constructor.
     * @draft ICU 4.2
     */
    TimeUnitFormat(const TimeUnitFormat&);

    /**
     * deconstructor
     * @draft ICU 4.2
     */
    virtual ~TimeUnitFormat();

    /**
     * Clone this Format object polymorphically. The caller owns the result and
     * should delete it when done.
     * @return    A copy of the object.
     * @draft ICU 4.2
     */
    virtual Format* clone(void) const;

    /**
     * Assignment operator
     * @draft ICU 4.2
     */
    TimeUnitFormat& operator=(const TimeUnitFormat& other);


    /**
     * Return true if the given Format objects are semantically equal. Objects
     * of different subclasses are considered unequal.
     * @param other    the object to be compared with.
     * @return         true if the given Format objects are semantically equal.
     * @draft ICU 4.2
     */
    virtual UBool operator==(const Format& other) const;

    /**
     * Return true if the given Format objects are not semantically equal. 
     * Objects of different subclasses are considered unequal.
     * @param other the object to be compared with.
     * @return      true if the given Format objects are not semantically equal.
     * @draft ICU 4.2
     */
    UBool operator!=(const Format& other) const;

    /**
     * Set the locale used for formatting or parsing.
     * @param locale  the locale to be set
     * @param status  output param set to success/failure code on exit
     * @draft ICU 4.2
     */
    void setLocale(const Locale& locale, UErrorCode& status);


    /**
     * Set the number format used for formatting or parsing. 
     * @param format  the number formatter to be set
     * @param status  output param set to success/failure code on exit
     * @draft ICU 4.2
     */
    void setNumberFormat(const NumberFormat& format, UErrorCode& status);

    /**
     * Format a TimeUnitAmount.
     * If the formattable object is not a time unit amount object,
     * or the number in time unit amount is not a double type or long type
     * numeric, it returns a failing status: U_ILLEGAL_ARGUMENT_ERROR.
     * @see Format#format(const Formattable&, UnicodeString&, FieldPosition&,  UErrorCode&) const
     * @draft ICU 4.2
     */
    virtual UnicodeString& format(const Formattable& obj, 
                                  UnicodeString& toAppendTo,
                                  FieldPosition& pos, 
                                  UErrorCode& status) const; 

    /**
     * Parse a TimeUnitAmount.
     * @see Format#parseObject(const UnicodeString&, Formattable&, ParsePosition&) const;
     * @draft ICU 4.2
     */
    virtual void parseObject(const UnicodeString& source, 
                             Formattable& result,
                             ParsePosition& pos) const;

    /**
     * Return the class ID for this class. This is useful only for comparing to
     * a return value from getDynamicClassID(). For example:
     * <pre>
     * .   Base* polymorphic_pointer = createPolymorphicObject();
     * .   if (polymorphic_pointer->getDynamicClassID() ==
     * .       erived::getStaticClassID()) ...
     * </pre>
     * @return          The class ID for all objects of this class.
     * @draft ICU 4.2
     */
    static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * Returns a unique class ID POLYMORPHICALLY. Pure virtual override. This
     * method is to implement a simple version of RTTI, since not all C++
     * compilers support genuine RTTI. Polymorphic operator==() and clone()
     * methods call this method.
     *
     * @return          The class ID for this object. All objects of a
     *                  given class have the same class ID.  Objects of
     *                  other classes have different class IDs.
     * @draft ICU 4.2
     */
    virtual UClassID getDynamicClassID(void) const;

private:
    NumberFormat* fNumberFormat;
    Locale        fLocale;
    Hashtable*    fTimeUnitToCountToPatterns[TimeUnit::UTIMEUNIT_FIELD_COUNT];
    PluralRules*  fPluralRules;
    
    
    // it might actually be simpler to make them Decimal Formats later.
    // initialize all private data members
    void setup(UErrorCode& status); 
 
    // initialize data member without fill in data for fTimeUnitToCountToPattern
    void initDataMembers(UErrorCode& status);

    // initialize fTimeUnitToCountToPatterns from current locale's resource.
    void readFromCurrentLocale(UErrorCode& status);

    // check completeness of fTimeUnitToCountToPatterns against all time units,
    // and all plural rules, fill in fallback as necessary.
    void checkConsistency(UErrorCode& status);

    // fill in fTimeUnitToCountToPatterns from locale fall-back chain
    void searchInLocaleChain(TimeUnit::UTimeUnitFields field, const char*, const char*, Hashtable*, UErrorCode&);

    // initialize hash table
    Hashtable* initHash(UErrorCode& status);

    // delete hash table
    void deleteHash(Hashtable* htable);

    // copy hash table
    void copyHash(const Hashtable* source, Hashtable* target, UErrorCode& status);
    // get time unit name, such as "year", from time unit field enum, such as
    // UTIMEUNIT_YEAR.
    static const char* getTimeUnitName(TimeUnit::UTimeUnitFields field, UErrorCode& status);
};



inline UBool
TimeUnitFormat::operator!=(const Format& other) const  {
    return !operator==(other);
}



U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // __TMUTFMT_H__
//eof
