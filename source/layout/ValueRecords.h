/*
 * @(#)ValueRecords.h	1.6 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000 - All Rights Reserved
 *
 */

#ifndef __VALUERECORDS_H
#define __VALUERECORDS_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphIterator.h"
#include "GlyphPositionAdjustments.h"

typedef le_uint16 ValueFormat;
typedef le_int16 ValueRecordField;

struct ValueRecord
{
    le_int16   values[ANY_NUMBER];

    le_int16   getFieldValue(ValueFormat valueFormat, ValueRecordField field);
    le_int16   getFieldValue(le_int16 index, ValueFormat valueFormat, ValueRecordField field);
    void    adjustPosition(ValueFormat valueFormat, char *base, GlyphPositionAdjustment &positionAdjustment,
                const LEFontInstance *fontInstance);
    void    adjustPosition(le_int16 index, ValueFormat valueFormat, char *base, GlyphPositionAdjustment &positionAdjustment,
                const LEFontInstance *fontInstance);

    static le_int16    getSize(ValueFormat valueFormat);

private:
    static le_int16    getFieldCount(ValueFormat valueFormat);
    static le_int16    getFieldIndex(ValueFormat valueFormat, ValueRecordField field);
};

enum ValueRecordFields
{
    vrfXPlacement   = 0,
    vrfYPlacement   = 1,
    vrfXAdvance     = 2,
    vrfYAdvance     = 3,
    vrfXPlaDevice   = 4,
    vrfYPlaDevice   = 5,
    vrfXAdvDevice   = 6,
    vrfYAdvDevice   = 7
};

enum ValueFormatBits
{
    vfbXPlacement   = 0x0001,
    vfbYPlacement   = 0x0002,
    vfbXAdvance     = 0x0004,
    vfbYAdvance     = 0x0008,
    vfbXPlaDevice   = 0x0010,
    vfbYPlaDevice   = 0x0020,
    vfbXAdvDevice   = 0x0040,
    vfbYAdvDevice   = 0x0080,
    vfbReserved     = 0xFF00,
    vfbAnyDevice    = vfbXPlaDevice + vfbYPlaDevice + vfbXAdvDevice + vfbYAdvDevice
};


#endif


