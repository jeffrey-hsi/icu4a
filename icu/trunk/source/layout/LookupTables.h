/*
 * @(#)LookupTables.h	1.5 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000 - All Rights Reserved
 *
 */

#ifndef __LOOKUPTABLES_H
#define __LOOKUPTABLES_H

#include "LETypes.h"
#include "LayoutTables.h"

enum LookupTableFormat
{
    ltfSimpleArray      = 0,
    ltfSegmentSingle    = 2,
    ltfSegmentArray     = 4,
    ltfSingleTable      = 6,
    ltfTrimmedArray     = 8
};

typedef le_int16 LookupValue;

struct LookupTable
{
    le_int16 format;
};

struct LookupSegment
{
    le_int16 lastGlyph;
    le_int16 firstGlyph;
    LookupValue value;
};

struct LookupSingle
{
    le_int16 glyph;
    LookupValue value;
};

struct BinarySearchLookupTable : LookupTable
{
    le_int16 unitSize;
    le_int16 nUnits;
    le_int16 searchRange;
    le_int16 entrySelector;
    le_int16 rangeShift;

    const LookupSegment *lookupSegment(const LookupSegment *segments, le_uint32 glyph) const;

    const LookupSingle *lookupSingle(const LookupSingle *entries, le_uint32 glyph) const;
};

struct SimpleArrayLookupTable : LookupTable
{
    LookupValue valueArray[ANY_NUMBER];
};

struct SegmentSingleLookupTable : BinarySearchLookupTable
{
    LookupSegment segments[ANY_NUMBER];
};

struct SegmentArrayLookupTable : BinarySearchLookupTable
{
    LookupSegment segments[ANY_NUMBER];
};

struct SingleTableLookupTable : BinarySearchLookupTable
{
    LookupSingle entries[ANY_NUMBER];
};

struct TrimmedArrayLookupTable : LookupTable
{
    le_int16 firstGlyph;
    le_int16 glyphCount;
    LookupValue valueArray[ANY_NUMBER];
};

#endif
