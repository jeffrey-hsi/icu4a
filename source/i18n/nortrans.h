/*
**********************************************************************
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   07/03/01    aliu        Creation.
**********************************************************************
*/
#ifndef NORTRANS_H
#define NORTRANS_H

#include "unicode/utypes.h"
#include "unicode/translit.h"
#include "unicode/normlzr.h"

U_NAMESPACE_BEGIN

/**
 * A transliterator that performs normalization.
 * @author Alan Liu
 * @version $RCSfile: nortrans.h,v $ $Revision: 1.2 $ $Date: 2001/12/03 20:50:11 $
 */
class U_I18N_API NormalizationTransliterator : public Transliterator {

    /**
     * The normalization mode of this transliterator.
     */
    UNormalizationMode fMode;

    /**
     * Normalization options for this transliterator.
     */
    int32_t options;

    /**
     * Alias to skippables set.  NOT OWNED.
     */
    UnicodeSet* skippable;

 public:

    /**
     * Destructor.
     */
    virtual ~NormalizationTransliterator();

    /**
     * Copy constructor.
     */
    NormalizationTransliterator(const NormalizationTransliterator&);

    /**
     * Assignment operator.
     */
    NormalizationTransliterator& operator=(const NormalizationTransliterator&);

    /**
     * Transliterator API.
     */
    Transliterator* clone(void) const;

 protected:

    /**
     * Implements {@link Transliterator#handleTransliterate}.
     */
    void handleTransliterate(Replaceable& text, UTransPosition& offset,
                             UBool isIncremental) const;
 public:

    /**
     * System registration hook.  Public to Transliterator only.
     */
    static void registerIDs();

    /**
     * Static memory cleanup function.
     */
    static void cleanup();

 private:

    // Transliterator::Factory methods
    static Transliterator* _create(const UnicodeString& ID,
                                   Token context);

    /**
     * Constructs a transliterator.  This method is private.
     * Public users must use the factory method createInstance().
     */
    NormalizationTransliterator(const UnicodeString& id,
                                UNormalizationMode mode, int32_t opt);

    static void initStatics();
};

U_NAMESPACE_END

#endif
