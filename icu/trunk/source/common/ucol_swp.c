/*
*******************************************************************************
*
*   Copyright (C) 2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_swp.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003sep10
*   created by: Markus W. Scherer
*
*   Swap collation binaries.
*/

#include "unicode/utypes.h"
#include "unicode/udata.h" /* UDataInfo */
#include "cmemory.h"
#include "utrie.h"
#include "udataswp.h"
#include "ucol_imp.h"
#include "ucol_swp.h"

/* swap a header-less collation binary, inside a resource bundle or ucadata.icu */
U_CAPI int32_t U_EXPORT2
ucol_swapBinary(const UDataSwapper *ds,
                const void *inData, int32_t length, void *outData,
                UErrorCode *pErrorCode) {
    const uint8_t *inBytes;
    uint8_t *outBytes;

    const UCATableHeader *inHeader;
    UCATableHeader *outHeader;
    UCATableHeader header;

    uint32_t count;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || length<-1 || (length>0 && outData==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData;
    outBytes=(uint8_t *)outData;

    inHeader=(const UCATableHeader *)inData;
    outHeader=(UCATableHeader *)outData;

    /*
     * The collation binary must contain at least the UCATableHeader,
     * starting with its size field.
     * sizeof(UCATableHeader)==42*4 in ICU 2.8
     */
    header.size=udata_readInt32(ds, inHeader->size);
    if(length>=0 && (length<(42*4) || length<header.size)) {
        udata_printError(ds, "ucol_swapBinary(): too few bytes (%d after header) for collation data\n",
                         length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    header.magic=ds->readUInt32(inHeader->magic);
    if(!(
        header.magic==UCOL_HEADER_MAGIC &&
        inHeader->formatVersion[0]==2 &&
        inHeader->formatVersion[1]>=3
    )) {
        udata_printError(ds, "ucol_swapBinary(): magic 0x%08x or format version %02x.%02x is not a collation binary\n",
                         header.magic,
                         inHeader->formatVersion[0], inHeader->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    if(inHeader->isBigEndian!=ds->inIsBigEndian || inHeader->charSetFamily!=ds->inCharset) {
        udata_printError(ds, "ucol_swapBinary(): endianness %d or charset %d does not match the swapper\n",
                         inHeader->isBigEndian, inHeader->charSetFamily);
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    if(length>=0) {
        /* copy everything, takes care of data that needs no swapping */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, header.size);
        }

        /* swap the necessary pieces in the order of their occurrence in the data */

        /* read more of the UCATableHeader (the size field was read above) */
        header.options=                 ds->readUInt32(inHeader->options);
        header.UCAConsts=               ds->readUInt32(inHeader->UCAConsts);
        header.contractionUCACombos=    ds->readUInt32(inHeader->contractionUCACombos);
        header.mappingPosition=         ds->readUInt32(inHeader->mappingPosition);
        header.expansion=               ds->readUInt32(inHeader->expansion);
        header.contractionIndex=        ds->readUInt32(inHeader->contractionIndex);
        header.contractionCEs=          ds->readUInt32(inHeader->contractionCEs);
        header.contractionSize=         ds->readUInt32(inHeader->contractionSize);
        header.endExpansionCE=          ds->readUInt32(inHeader->endExpansionCE);
        header.expansionCESize=         ds->readUInt32(inHeader->expansionCESize);
        header.endExpansionCECount=     udata_readInt32(ds, inHeader->endExpansionCECount);
        header.contractionUCACombosSize=udata_readInt32(ds, inHeader->contractionUCACombosSize);

        /* swap the 32-bit integers in the header */
        ds->swapArray32(ds, inHeader, (int32_t)((const char *)&inHeader->jamoSpecial-(const char *)inHeader),
                           outHeader, pErrorCode);

        /* set the output platform properties */
        outHeader->isBigEndian=ds->outIsBigEndian;
        outHeader->charSetFamily=ds->outCharset;

        /* swap the options */
        ds->swapArray32(ds, inBytes+header.options, header.expansion-header.options,
                           outBytes+header.options, pErrorCode);

        /* swap the expansions */
        if(header.contractionIndex!=0) {
            /* expansions bounded by contractions */
            count=header.contractionIndex-header.expansion;
        } else {
            /* no contractions: expansions bounded by the main trie */
            count=header.mappingPosition-header.expansion;
        }
        ds->swapArray32(ds, inBytes+header.expansion, (int32_t)count,
                           outBytes+header.expansion, pErrorCode);

        /* swap the contractions */
        if(header.contractionIndex!=0) {
            /* contractionIndex: UChar[] */
            ds->swapArray16(ds, inBytes+header.contractionIndex, header.contractionSize*2,
                               outBytes+header.contractionIndex, pErrorCode);

            /* contractionCEs: CEs[] */
            ds->swapArray32(ds, inBytes+header.contractionCEs, header.contractionSize*4,
                               outBytes+header.contractionCEs, pErrorCode);
        }

        /* swap the main trie */
        count=header.endExpansionCE-header.mappingPosition;
        utrie_swap(ds, inBytes+header.mappingPosition, (int32_t)count,
                      outBytes+header.mappingPosition, pErrorCode);

        /* swap the max expansion table */
        ds->swapArray32(ds, inBytes+header.endExpansionCE, header.endExpansionCECount*4,
                           outBytes+header.endExpansionCE, pErrorCode);

        /* expansionCESize, unsafeCP, contrEndCP: uint8_t[], no need to swap */

        /* swap UCA constants */
        if(header.UCAConsts!=0) {
            /*
             * if UCAConsts!=0 then contractionUCACombos because we are swapping
             * the UCA data file, and we know that the UCA contains contractions
             */
            count=header.contractionUCACombos-header.UCAConsts;
            ds->swapArray32(ds, inBytes+header.UCAConsts, header.contractionUCACombos-header.UCAConsts,
                               outBytes+header.UCAConsts, pErrorCode);
        }

        /* swap UCA contractions */
        if(header.contractionUCACombosSize!=0) {
            count=header.contractionUCACombosSize*inHeader->contractionUCACombosWidth*U_SIZEOF_UCHAR;
            ds->swapArray16(ds, inBytes+header.contractionUCACombos, (int32_t)count,
                               outBytes+header.contractionUCACombos, pErrorCode);
        }
    }

    return header.size;
}

/* swap ICU collation data like ucadata.icu */
U_CAPI int32_t U_EXPORT2
ucol_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize, collationSize;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x55 &&   /* dataFormat="UCol" */
        pInfo->dataFormat[1]==0x43 &&
        pInfo->dataFormat[2]==0x6f &&
        pInfo->dataFormat[3]==0x6c &&
        pInfo->formatVersion[0]==2 &&
        pInfo->formatVersion[1]>=3
    )) {
        udata_printError(ds, "ucol_swap(): data format %02x.%02x.%02x.%02x (format version %02x.%02x) is not a collation file\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0], pInfo->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    collationSize=ucol_swapBinary(ds,
                        (const char *)inData+headerSize,
                        length>=0 ? length-headerSize : -1,
                        (char *)outData+headerSize,
                        pErrorCode);
    if(U_SUCCESS(*pErrorCode)) {
        return headerSize+collationSize;
    } else {
        return 0;
    }
}
