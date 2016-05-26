/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2016 and later: Unicode, Inc. and others.
 * License & terms of use: http://www.unicode.org/copyright.html
 ********************************************************************/

#include "unicode/translit.h"
#include "unicode/normlzr.h"

class UnaccentTransliterator : public Transliterator {
    
 public:
    
    /**
     * Constructor
     */
    UnaccentTransliterator();

    /**
     * Destructor
     */
    virtual ~UnaccentTransliterator();

 protected:

    /**
     * Implement Transliterator API
     */
    virtual void handleTransliterate(Replaceable& text,
                                     UTransPosition& index,
                                     UBool incremental) const;

 private:

    /**
     * Unaccent a single character using normalizer.
     */
    UChar unaccent(UChar c) const;

    Normalizer normalizer;
};
