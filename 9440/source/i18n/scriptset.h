/*
**********************************************************************
*   Copyright (C) 2013, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* scriptset.h
*
* created on: 2013 Jan 7
* created by: Andy Heninger
*/

#ifndef __SCRIPTSET_H__
#define __SCRIPTSET_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/uscript.h"

#include "uelement.h"

U_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------
//
//  ScriptSet - A bit set representing a set of scripts.
//
//              This class was originally used exclusively with script sets appearing
//              as part of the spoof check whole script confusable binary data. Its
//              use has since become more general, but the continued use to wrap
//              prebuilt binary data does constrain the design.
//
//-------------------------------------------------------------------------------
class U_I18N_API ScriptSet: public UMemory {
  public:
    ScriptSet();
    ~ScriptSet();

    UBool operator == (const ScriptSet &other) const;
    ScriptSet & operator = (const ScriptSet &other);

    ScriptSet &Union(const ScriptSet &other);
    ScriptSet &set(UScriptCode script, UErrorCode &status);
    ScriptSet &reset(UScriptCode script, UErrorCode &status);
    ScriptSet &intersect(const ScriptSet &other); // TODO: remove this one.
    ScriptSet &intersect(UScriptCode script);
    UBool      intersects(const ScriptSet &other) const;
    ScriptSet &setAll();
    ScriptSet &resetAll();
    int32_t countMembers() const;
    int32_t hashCode() const;

  private:
    uint32_t  bits[6];
};

U_NAMESPACE_END

U_CAPI UBool U_EXPORT2
uhash_compareScriptSet(const UElement key1, const UElement key2);

U_CAPI int32_t U_EXPORT2
uhash_hashScriptSet(const UElement key);

U_CAPI void U_EXPORT2
uhash_deleteScriptSet(void *obj);

#endif // __SCRIPTSET_H__
