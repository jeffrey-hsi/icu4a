/*
 * @(#)GDEFMarkFilter.cpp	1.5 00/03/15
 *
 * (C) Copyright IBM Corp. 1998, 1999, 2000 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "GDEFMarkFilter.h"
#include "GlyphDefinitionTables.h"

GDEFMarkFilter::GDEFMarkFilter(GlyphDefinitionTableHeader *gdefTable)
{
    classDefTable = gdefTable->getGlyphClassDefinitionTable();
}

GDEFMarkFilter::~GDEFMarkFilter()
{
    // nothing to do?
}

le_bool GDEFMarkFilter::accept(LEGlyphID glyph)
{
    le_int32 glyphClass = classDefTable->getGlyphClass(glyph);

    return glyphClass == gcdMarkGlyph;
}
