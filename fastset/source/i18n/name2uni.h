/*
**********************************************************************
*   Copyright (C) 2001-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   06/07/01    aliu        Creation.
**********************************************************************
*/
#ifndef NAME2UNI_H
#define NAME2UNI_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "unicode/uniset.h"

U_NAMESPACE_BEGIN

/**
 * A transliterator that performs name to character mapping.
 * It recognizes the Perl syntax \N{name}.
 * @author Alan Liu
 */
class U_I18N_API NameUnicodeTransliterator : public Transliterator {
public:

    /**
     * Constructs a transliterator.
     * @param adoptedFilter    the filter for this transliterator.
     */
    NameUnicodeTransliterator(UnicodeFilter* adoptedFilter = 0);

    /**
     * Destructor.
     */
    virtual ~NameUnicodeTransliterator();

    /**
     * Copy constructor.
     */
    NameUnicodeTransliterator(const NameUnicodeTransliterator&);

    /**
     * Assignment operator.
     */
    NameUnicodeTransliterator& operator=(const NameUnicodeTransliterator&);

    /**
     * Transliterator API.
     * @return    A copy of the object.
     */
    virtual Transliterator* clone(void) const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     */
    static UClassID U_EXPORT2 getStaticClassID();

 protected:

    /**
     * Implements {@link Transliterator#handleTransliterate}.
     * @param text          the buffer holding transliterated and
     *                      untransliterated text
     * @param offset        the start and limit of the text, the position
     *                      of the cursor, and the start and limit of transliteration.
     * @param incremental   if true, assume more text may be coming after
     *                      pos.contextLimit. Otherwise, assume the text is complete.
     */
    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

    /**
     * Set of characters which occur in Unicode character names.
     */
    UnicodeSet legal;
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
