/*
 *****************************************************************************
 * Copyright (C) 1996-2012, International Business Machines Corporation and others.
 * All Rights Reserved.
 *****************************************************************************
 *
 * File sortkey.h
 *
 * Created by: Helena Shih
 *
 * Modification History:
 *
 *  Date         Name          Description
 *
 *  6/20/97     helena      Java class name change.
 *  8/18/97     helena      Added internal API documentation.
 *  6/26/98     erm         Changed to use byte arrays and memcmp.
 *****************************************************************************
 */

#ifndef SORTKEY_H
#define SORTKEY_H

#include "unicode/utypes.h"

/**
 * \file 
 * \brief C++ API: Keys for comparing strings multiple times. 
 */
 
#if !UCONFIG_NO_COLLATION
#ifndef U_HIDE_DEPRECATED_API

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/coll.h"

U_NAMESPACE_BEGIN

/* forward declaration */
class RuleBasedCollator;
class RuleBasedCollator2;
class CollationKeyByteSink;
class CollationKeyByteSink2;

/**
 *
 * Collation keys are generated by the Collator class.  Use the CollationKey objects
 * instead of Collator to compare strings multiple times.  A CollationKey
 * preprocesses the comparison information from the Collator object to
 * make the comparison faster.  If you are not going to comparing strings
 * multiple times, then using the Collator object is generally faster,
 * since it only processes as much of the string as needed to make a
 * comparison.
 * <p> For example (with strength == tertiary)
 * <p>When comparing "Abernathy" to "Baggins-Smythworthy", Collator
 * only needs to process a couple of characters, while a comparison
 * with CollationKeys will process all of the characters.  On the other hand,
 * if you are doing a sort of a number of fields, it is much faster to use
 * CollationKeys, since you will be comparing strings multiple times.
 * <p>Typical use of CollationKeys are in databases, where you store a CollationKey
 * in a hidden field, and use it for sorting or indexing.
 *
 * <p>Example of use:
 * <pre>
 * \code
 *     UErrorCode success = U_ZERO_ERROR;
 *     Collator* myCollator = Collator::createInstance(success);
 *     CollationKey* keys = new CollationKey [3];
 *     myCollator->getCollationKey("Tom", keys[0], success );
 *     myCollator->getCollationKey("Dick", keys[1], success );
 *     myCollator->getCollationKey("Harry", keys[2], success );
 *
 *     // Inside body of sort routine, compare keys this way:
 *     CollationKey tmp;
 *     if(keys[0].compareTo( keys[1] ) > 0 ) {
 *         tmp = keys[0]; keys[0] = keys[1]; keys[1] = tmp;
 *     }
 *     //...
 * \endcode
 * </pre>
 * <p>Because Collator::compare()'s algorithm is complex, it is faster to sort
 * long lists of words by retrieving collation keys with Collator::getCollationKey().
 * You can then cache the collation keys and compare them using CollationKey::compareTo().
 * <p>
 * <strong>Note:</strong> <code>Collator</code>s with different Locale,
 * CollationStrength and DecompositionMode settings will return different
 * CollationKeys for the same set of strings. Locales have specific
 * collation rules, and the way in which secondary and tertiary differences
 * are taken into account, for example, will result in different CollationKeys
 * for same strings.
 * <p>

 * @see          Collator
 * @see          RuleBasedCollator
 * @version      1.3 12/18/96
 * @author       Helena Shih
 * @stable ICU 2.0
 */
class U_I18N_API CollationKey : public UObject {
public:
    /**
    * This creates an empty collation key based on the null string.  An empty
    * collation key contains no sorting information.  When comparing two empty
    * collation keys, the result is Collator::EQUAL.  Comparing empty collation key
    * with non-empty collation key is always Collator::LESS.
    * @stable ICU 2.0
    */
    CollationKey();


    /**
    * Creates a collation key based on the collation key values.
    * @param values the collation key values
    * @param count number of collation key values, including trailing nulls.
    * @stable ICU 2.0
    */
    CollationKey(const  uint8_t*    values,
                int32_t     count);

    /**
    * Copy constructor.
    * @param other    the object to be copied.
    * @stable ICU 2.0
    */
    CollationKey(const CollationKey& other);

    /**
    * Sort key destructor.
    * @stable ICU 2.0
    */
    virtual ~CollationKey();

    /**
    * Assignment operator
    * @param other    the object to be copied.
    * @stable ICU 2.0
    */
    const   CollationKey&   operator=(const CollationKey& other);

    /**
    * Compare if two collation keys are the same.
    * @param source the collation key to compare to.
    * @return Returns true if two collation keys are equal, false otherwise.
    * @stable ICU 2.0
    */
    UBool                   operator==(const CollationKey& source) const;

    /**
    * Compare if two collation keys are not the same.
    * @param source the collation key to compare to.
    * @return Returns TRUE if two collation keys are different, FALSE otherwise.
    * @stable ICU 2.0
    */
    UBool                   operator!=(const CollationKey& source) const;


    /**
    * Test to see if the key is in an invalid state. The key will be in an
    * invalid state if it couldn't allocate memory for some operation.
    * @return Returns TRUE if the key is in an invalid, FALSE otherwise.
    * @stable ICU 2.0
    */
    UBool                   isBogus(void) const;

    /**
    * Returns a pointer to the collation key values. The storage is owned
    * by the collation key and the pointer will become invalid if the key
    * is deleted.
    * @param count the output parameter of number of collation key values,
    * including any trailing nulls.
    * @return a pointer to the collation key values.
    * @stable ICU 2.0
    */
    const    uint8_t*       getByteArray(int32_t& count) const;

#ifdef U_USE_COLLATION_KEY_DEPRECATES
    /**
    * Extracts the collation key values into a new array. The caller owns
    * this storage and should free it.
    * @param count the output parameter of number of collation key values,
    * including any trailing nulls.
    * @obsolete ICU 2.6. Use getByteArray instead since this API will be removed in that release.
    */
    uint8_t*                toByteArray(int32_t& count) const;
#endif

    /**
    * Convenience method which does a string(bit-wise) comparison of the
    * two collation keys.
    * @param target target collation key to be compared with
    * @return Returns Collator::LESS if sourceKey &lt; targetKey,
    * Collator::GREATER if sourceKey > targetKey and Collator::EQUAL
    * otherwise.
    * @deprecated ICU 2.6 use the overload with error code
    */
    Collator::EComparisonResult compareTo(const CollationKey& target) const;

    /**
    * Convenience method which does a string(bit-wise) comparison of the
    * two collation keys.
    * @param target target collation key to be compared with
    * @param status error code
    * @return Returns UCOL_LESS if sourceKey &lt; targetKey,
    * UCOL_GREATER if sourceKey > targetKey and UCOL_EQUAL
    * otherwise.
    * @stable ICU 2.6
    */
    UCollationResult compareTo(const CollationKey& target, UErrorCode &status) const;

    /**
    * Creates an integer that is unique to the collation key.  NOTE: this
    * is not the same as String.hashCode.
    * <p>Example of use:
    * <pre>
    * .    UErrorCode status = U_ZERO_ERROR;
    * .    Collator *myCollation = Collator::createInstance(Locale::US, status);
    * .    if (U_FAILURE(status)) return;
    * .    CollationKey key1, key2;
    * .    UErrorCode status1 = U_ZERO_ERROR, status2 = U_ZERO_ERROR;
    * .    myCollation->getCollationKey("abc", key1, status1);
    * .    if (U_FAILURE(status1)) { delete myCollation; return; }
    * .    myCollation->getCollationKey("ABC", key2, status2);
    * .    if (U_FAILURE(status2)) { delete myCollation; return; }
    * .    // key1.hashCode() != key2.hashCode()
    * </pre>
    * @return the hash value based on the string's collation order.
    * @see UnicodeString#hashCode
    * @stable ICU 2.0
    */
    int32_t                 hashCode(void) const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     * @stable ICU 2.2
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     * @stable ICU 2.2
     */
    static UClassID U_EXPORT2 getStaticClassID();

private:
    /**
     * Replaces the current bytes buffer with a new one of newCapacity
     * and copies length bytes from the old buffer to the new one.
     * @return the new buffer, or NULL if the allocation failed
     */
    uint8_t *reallocate(int32_t newCapacity, int32_t length);
    /**
     * Set a new length for a new sort key in the existing fBytes.
     */
    void setLength(int32_t newLength);

    uint8_t *getBytes() {
        return (fFlagAndLength >= 0) ? fUnion.fStackBuffer : fUnion.fFields.fBytes;
    }
    const uint8_t *getBytes() const {
        return (fFlagAndLength >= 0) ? fUnion.fStackBuffer : fUnion.fFields.fBytes;
    }
    int32_t getCapacity() const {
        return (fFlagAndLength >= 0) ? (int32_t)sizeof(fUnion) : fUnion.fFields.fCapacity;
    }
    int32_t getLength() const { return fFlagAndLength & 0x7fffffff; }

    /**
    * Set the CollationKey to a "bogus" or invalid state
    * @return this CollationKey
    */
    CollationKey&           setToBogus(void);
    /**
    * Resets this CollationKey to an empty state
    * @return this CollationKey
    */
    CollationKey&           reset(void);

    /**
    * Allow private access to RuleBasedCollator
    */
    friend  class           RuleBasedCollator;
    friend  class           RuleBasedCollator2;
    friend  class           CollationKeyByteSink;
    friend  class           CollationKeyByteSink2;

    // Class fields. sizeof(CollationKey) is intended to be 48 bytes
    // on a machine with 64-bit pointers.
    // We use a union to maximize the size of the internal buffer,
    // similar to UnicodeString but not as tight and complex.

    // (implicit) *vtable;
    /**
     * Sort key length and flag.
     * Bit 31 is set if the buffer is heap-allocated.
     * Bits 30..0 contain the sort key length.
     */
    int32_t fFlagAndLength;
    /**
    * Unique hash value of this CollationKey.
    * Special value 2 if the key is bogus.
    */
    mutable int32_t fHashCode;
    /**
     * fUnion provides 32 bytes for the internal buffer or for
     * pointer+capacity.
     */
    union StackBufferOrFields {
        /** fStackBuffer is used iff fFlagAndLength>=0, else fFields is used */
        uint8_t fStackBuffer[32];
        struct {
            uint8_t *fBytes;
            int32_t fCapacity;
        } fFields;
    } fUnion;
};

inline UBool
CollationKey::operator!=(const CollationKey& other) const
{
    return !(*this == other);
}

inline UBool
CollationKey::isBogus() const
{
    return fHashCode == 2;  // kBogusHashCode
}

inline const uint8_t*
CollationKey::getByteArray(int32_t &count) const
{
    count = getLength();
    return getBytes();
}

U_NAMESPACE_END

#endif  /* U_HIDE_DEPRECATED_API */
#endif /* #if !UCONFIG_NO_COLLATION */

#endif
