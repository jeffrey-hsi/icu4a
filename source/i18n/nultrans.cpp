/*
**********************************************************************
*   Copyright (C) 2016 and later: Unicode, Inc. and others.
*   License & terms of use: http://www.unicode.org/copyright.html
**********************************************************************
*   Date        Name        Description
*   01/11/2000  aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "nultrans.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NullTransliterator)

NullTransliterator::NullTransliterator() : Transliterator(UNICODE_STRING_SIMPLE("Any-Null"), 0) {}

NullTransliterator::~NullTransliterator() {}

Transliterator* NullTransliterator::clone(void) const {
    return new NullTransliterator();
}

void NullTransliterator::handleTransliterate(Replaceable& /*text*/, UTransPosition& offsets,
                                             UBool /*isIncremental*/) const {
    offsets.start = offsets.limit;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
