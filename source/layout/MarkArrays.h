/*
 * @(#)MarkArrays.h	1.5 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000, 2001 - All Rights Reserved
 *
 */

#ifndef __MARKARRAYS_H
#define __MARKARRAYS_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

struct MarkRecord
{
    le_uint16	markClass;
    Offset		markAnchorTableOffset;
};

struct MarkArray
{
    le_uint16	markCount;
    MarkRecord	markRecordArray[ANY_NUMBER];

    le_int32 getMarkClass(LEGlyphID glyphID, le_int32 coverageIndex, const LEFontInstance *fontInstance,
        LEPoint &anchor) const;
};

U_NAMESPACE_END
#endif


