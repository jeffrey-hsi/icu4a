/*
 *
 * (C) Copyright IBM Corp. 1998-2013 - All Rights Reserved
 *
 */

#ifndef __GDEFMARKFILTER__H
#define __GDEFMARKFILTER__H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "GlyphDefinitionTables.h"

U_NAMESPACE_BEGIN

class GDEFMarkFilter : public UMemory, public LEGlyphFilter
{
private:
    const LEReferenceTo<GlyphClassDefinitionTable> classDefTable;

    GDEFMarkFilter(const GDEFMarkFilter &other); // forbid copying of this class
    GDEFMarkFilter &operator=(const GDEFMarkFilter &other); // forbid copying of this class

public:
    GDEFMarkFilter(const LEReferenceTo<GlyphDefinitionTableHeader> &gdefTable, LEErrorCode &success);
    virtual ~GDEFMarkFilter();

    virtual le_bool accept(LEGlyphID glyph) const;
};

U_NAMESPACE_END
#endif
