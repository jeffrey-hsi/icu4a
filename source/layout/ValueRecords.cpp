/*
 * @(#)ValueRecords.cpp	1.6 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "ValueRecords.h"
#include "DeviceTables.h"
#include "GlyphIterator.h"
#include "GlyphPositionAdjustments.h"
#include "LESwaps.h"

#define Nibble(value, nibble) ((value >> (nibble * 4)) & 0xF)
#define NibbleBits(value, nibble) (bitsInNibble[Nibble(value, nibble)])

le_int16 ValueRecord::getFieldValue(ValueFormat valueFormat, ValueRecordField field)
{
    le_int16 valueIndex = getFieldIndex(valueFormat, field);
    le_int16 value = values[valueIndex];

    return SWAPW(value);
}

le_int16 ValueRecord::getFieldValue(le_int16 index, ValueFormat valueFormat, ValueRecordField field)
{
    le_int16 baseIndex = getFieldCount(valueFormat) * index;
    le_int16 valueIndex = getFieldIndex(valueFormat, field);
    le_int16 value = values[baseIndex + valueIndex];

    return SWAPW(value);
}

void ValueRecord::adjustPosition(ValueFormat valueFormat, char *base, GlyphPositionAdjustment &positionAdjustment,
                                 const LEFontInstance *fontInstance)
{
    if ((valueFormat & vfbXPlacement) != 0) {
        le_int16 value = getFieldValue(valueFormat, vrfXPlacement);
        LEPoint pixels;

        fontInstance->transformFunits(value, 0, pixels);

        positionAdjustment.adjustXPlacement(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYPlacement(fontInstance->yPixelsToUnits(pixels.fY));
    }

    if ((valueFormat & vfbYPlacement) != 0) {
        le_int16 value = getFieldValue(valueFormat, vrfYPlacement);
        LEPoint pixels;

        fontInstance->transformFunits(0, value, pixels);

        positionAdjustment.adjustXPlacement(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYPlacement(fontInstance->yPixelsToUnits(pixels.fY));
    }

    if ((valueFormat & vfbXAdvance) != 0) {
        le_int16 value = getFieldValue(valueFormat, vrfXAdvance);
        LEPoint pixels;

        fontInstance->transformFunits(value, 0, pixels);

        positionAdjustment.adjustXAdvance(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYAdvance(fontInstance->yPixelsToUnits(pixels.fY));
    }

    if ((valueFormat & vfbYAdvance) != 0) {
        le_int16 value = getFieldValue(valueFormat, vrfYAdvance);
        LEPoint pixels;

        fontInstance->transformFunits(0, value, pixels);

        positionAdjustment.adjustXAdvance(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYAdvance(fontInstance->yPixelsToUnits(pixels.fY));
    }

    // FIXME: The device adjustments should really be transformed, but
    // the only way I know how to do that is to convert them to le_int16 units,
    // transform them, and then convert them back to pixels. Sigh...
    if ((valueFormat & vfbAnyDevice) != 0) {
        le_int16 xppem = (le_int16) fontInstance->getXPixelsPerEm();
        le_int16 yppem = (le_int16) fontInstance->getYPixelsPerEm();

        if ((valueFormat & vfbXPlaDevice) != 0) {
            Offset dtOffset = getFieldValue(valueFormat, vrfXPlaDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 xAdj = dt->getAdjustment(xppem);

                positionAdjustment.adjustXPlacement(fontInstance->xPixelsToUnits(xAdj));
            }
        }

        if ((valueFormat & vfbYPlaDevice) != 0) {
            Offset dtOffset = getFieldValue(valueFormat, vrfYPlaDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 yAdj = dt->getAdjustment(yppem);

                positionAdjustment.adjustYPlacement(fontInstance->yPixelsToUnits(yAdj));
            }
        }

        if ((valueFormat & vfbXAdvDevice) != 0) {
            Offset dtOffset = getFieldValue(valueFormat, vrfXAdvDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 xAdj = dt->getAdjustment(xppem);

                positionAdjustment.adjustXAdvance(fontInstance->xPixelsToUnits(xAdj));
            }
        }

        if ((valueFormat & vfbYAdvDevice) != 0) {
            Offset dtOffset = getFieldValue(valueFormat, vrfYAdvDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 yAdj = dt->getAdjustment(yppem);

                positionAdjustment.adjustYAdvance(fontInstance->yPixelsToUnits(yAdj));
            }
        }
    }
}

void ValueRecord::adjustPosition(le_int16 index, ValueFormat valueFormat, char *base, GlyphPositionAdjustment &positionAdjustment,
                                 const LEFontInstance *fontInstance)
{
    if ((valueFormat & vfbXPlacement) != 0) {
        le_int16 value = getFieldValue(index, valueFormat, vrfXPlacement);
        LEPoint pixels;

        fontInstance->transformFunits(value, 0, pixels);

        positionAdjustment.adjustXPlacement(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYPlacement(fontInstance->yPixelsToUnits(pixels.fY));
    }

    if ((valueFormat & vfbYPlacement) != 0) {
        le_int16 value = getFieldValue(index, valueFormat, vrfYPlacement);
        LEPoint pixels;

        fontInstance->transformFunits(0, value, pixels);

        positionAdjustment.adjustXPlacement(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYPlacement(fontInstance->yPixelsToUnits(pixels.fY));
    }

    if ((valueFormat & vfbXAdvance) != 0) {
        le_int16 value = getFieldValue(index, valueFormat, vrfXAdvance);
        LEPoint pixels;

        fontInstance->transformFunits(value, 0, pixels);

        positionAdjustment.adjustXAdvance(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYAdvance(fontInstance->yPixelsToUnits(pixels.fY));
    }

    if ((valueFormat & vfbYAdvance) != 0) {
        le_int16 value = getFieldValue(index, valueFormat, vrfYAdvance);
        LEPoint pixels;

        fontInstance->transformFunits(0, value, pixels);

        positionAdjustment.adjustXAdvance(fontInstance->xPixelsToUnits(pixels.fX));
        positionAdjustment.adjustYAdvance(fontInstance->yPixelsToUnits(pixels.fY));
    }

    // FIXME: The device adjustments should really be transformed, but
    // the only way I know how to do that is to convert them to le_int16 units,
    // transform them, and then convert them back to pixels. Sigh...
    if ((valueFormat & vfbAnyDevice) != 0) {
        le_int16 xppem = (le_int16) fontInstance->getXPixelsPerEm();
        le_int16 yppem = (le_int16) fontInstance->getYPixelsPerEm();

        if ((valueFormat & vfbXPlaDevice) != 0) {
            Offset dtOffset = getFieldValue(index, valueFormat, vrfXPlaDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 xAdj = dt->getAdjustment(xppem);

                positionAdjustment.adjustXAdvance(fontInstance->xPixelsToUnits(xAdj));
            }
        }

        if ((valueFormat & vfbYPlaDevice) != 0) {
            Offset dtOffset = getFieldValue(index, valueFormat, vrfYPlaDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 yAdj = dt->getAdjustment(yppem);

                positionAdjustment.adjustYAdvance(fontInstance->yPixelsToUnits(yAdj));
            }
        }

        if ((valueFormat & vfbXAdvDevice) != 0) {
            Offset dtOffset = getFieldValue(index, valueFormat, vrfXAdvDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 xAdj = dt->getAdjustment(xppem);

                positionAdjustment.adjustXAdvance(fontInstance->xPixelsToUnits(xAdj));
            }
        }

        if ((valueFormat & vfbYAdvDevice) != 0) {
            Offset dtOffset = getFieldValue(index, valueFormat, vrfYAdvDevice);

            if (dtOffset != 0) {
                DeviceTable *dt = (DeviceTable *) (base + dtOffset);
                le_int16 yAdj = dt->getAdjustment(yppem);

                positionAdjustment.adjustYAdvance(fontInstance->yPixelsToUnits(yAdj));
            }
        }
    }
}

le_int16 ValueRecord::getSize(ValueFormat valueFormat)
{
    return getFieldCount(valueFormat) * sizeof(le_int16);
}

le_int16 ValueRecord::getFieldCount(ValueFormat valueFormat)
{
    static le_int16 bitsInNibble[] =
    {
        0 + 0 + 0 + 0,
        0 + 0 + 0 + 1,
        0 + 0 + 1 + 0,
        0 + 0 + 1 + 1,
        0 + 1 + 0 + 0,
        0 + 1 + 0 + 1,
        0 + 1 + 1 + 0,
        0 + 1 + 1 + 1,
        1 + 0 + 0 + 0,
        1 + 0 + 0 + 1,
        1 + 0 + 1 + 0,
        1 + 0 + 1 + 1,
        1 + 1 + 0 + 0,
        1 + 1 + 0 + 1,
        1 + 1 + 1 + 0,
        1 + 1 + 1 + 1
    };

    valueFormat &= ~vfbReserved;

    return NibbleBits(valueFormat, 0) + NibbleBits(valueFormat, 1) +
           NibbleBits(valueFormat, 2) + NibbleBits(valueFormat, 3);
}

le_int16 ValueRecord::getFieldIndex(ValueFormat valueFormat, ValueRecordField field)
{
    static le_uint16 beforeMasks[] = 
    {
        0x0000,
        0x0001,
        0x0003,
        0x0007,
        0x000F,
        0x001F,
        0x003F,
        0x007F,
        0x00FF,
        0x01FF,
        0x03FF,
        0x07FF,
        0x0FFF,
        0x1FFF,
        0x3FFF,
        0x7FFF,
        0xFFFF
    };

    return getFieldCount(valueFormat & beforeMasks[field]);
}

