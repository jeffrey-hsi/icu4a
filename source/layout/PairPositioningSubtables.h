/*
 * @(#)PairPositioningSubtables.h	1.7 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000 - All Rights Reserved
 *
 */

#ifndef __PAIRPOSITIONINGSUBTABLES_H
#define __PAIRPOSITIONINGSUBTABLES_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphPositioningTables.h"
#include "ValueRecords.h"
#include "GlyphIterator.h"

// NOTE: ValueRecord has a variable size
struct PairValueRecord
{
    LEGlyphID     secondGlyph;
    ValueRecord valueRecord1;
//  ValueRecord valueRecord2;
};

struct PairSetTable
{
    le_uint16       pairValueCount;
    PairValueRecord pairValueRecordArray[ANY_NUMBER];
};

struct PairPositioningSubtable : GlyphPositioningSubtable
{
    ValueFormat valueFormat1;
    ValueFormat valueFormat2;

    le_uint32  process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance);
};

struct PairPositioningFormat1Subtable : PairPositioningSubtable
{
    le_uint16   pairSetCount;
    Offset      pairSetTableOffsetArray[ANY_NUMBER];

    le_uint32  process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance);

private:
    PairValueRecord *findPairValueRecord(LEGlyphID glyphID, PairValueRecord *records,
        le_uint16 recordCount, le_uint16 recordSize);
};

// NOTE: ValueRecord has a variable size
struct Class2Record
{
    ValueRecord valueRecord1;
//  ValueRecord valurRecord2;
};

struct Class1Record
{
    Class2Record class2RecordArray[ANY_NUMBER];
};

struct PairPositioningFormat2Subtable : PairPositioningSubtable
{
    Offset       classDef1Offset;
    Offset       classDef2Offset;
    le_uint16    class1Count;
    le_uint16    class2Count;
    Class1Record class1RecordArray[ANY_NUMBER];

    le_uint32  process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance);
};

#endif


