/*
******************************************************************************
*
*   Copyright (C) 1999-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************/


/*----------------------------------------------------------------------------------
 *
 *   UCommonData   An abstract interface for dealing with ICU Common Data Files.
 *                 ICU Common Data Files are a grouping of a number of individual
 *                 data items (resources, converters, tables, anything) into a
 *                 single file or dll.  The combined format includes a table of
 *                 contents for locating the individual items by name.
 *
 *                 Two formats for the table of contents are supported, which is
 *                 why there is an abstract inteface involved.
 *
 *                 These functions are part of the ICU internal implementation, and
 *                 are not inteded to be used directly by applications.
 */

#ifndef __UCMNDATA_H__
#define __UCMNDATA_H__

#include "unicode/udata.h"
#include "umapfile.h"


#define COMMON_DATA_NAME U_ICUDATA_NAME

typedef struct  {
    uint16_t    headerSize;
    uint8_t     magic1;
    uint8_t     magic2;
} MappedData;


typedef struct  {
    MappedData  dataHeader;
    UDataInfo   info;
} DataHeader;

typedef struct {
    uint32_t nameOffset;
    uint32_t dataOffset;
} UDataOffsetTOCEntry;

typedef struct {
    uint32_t count;
    UDataOffsetTOCEntry entry[2];    /* Acutal size of array is from count. */
} UDataOffsetTOC;


/*
 *  "Virtual" functions for data lookup.
 *  To call one, given a UDataMemory *p, the code looks like this:
 *     p->vFuncs.Lookup(p, tocEntryName, pErrorCode);
 *          (I sure do wish this was written in C++, not C)
 */

typedef const DataHeader *
(* LookupFn)(const UDataMemory *pData,
             const char *tocEntryName,
             UErrorCode *pErrorCode);

typedef uint32_t
(* NumEntriesFn)(const UDataMemory *pData);

typedef struct {
    LookupFn      Lookup;
    NumEntriesFn  NumEntries; 
} commonDataFuncs;


/*
 *  Functions to check whether a UDataMemory refers to memory containing 
 *     a recognizable header and table of contents a Common Data Format
 *
 *     If a valid header and TOC are found,
 *         set the CommonDataFuncs function dispatch vector in the UDataMemory
 *             to point to the right functions for the TOC type.
 *     otherwise
 *         set an errorcode.
 */
void   udata_checkCommonData(UDataMemory *pData, UErrorCode *pErrorCode);


#endif
