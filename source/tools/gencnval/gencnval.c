/*
*******************************************************************************
*
*   Copyright (C) 1999, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  gencnval.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999nov05
*   created by: Markus W. Scherer
*
*   This program reads convrtrs.txt and writes a memory-mappable
*   converter name alias table to cnvalias.dat .
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "unicode/utypes.h"
#include "cmemory.h"
#include "cstring.h"
#include "ucnv_io.h" /* charsetNameCmp */
#include "filestrm.h"
#include "unewdata.h"
#include "uoptions.h"

#define STRING_STORE_SIZE 100000
#define MAX_ALIAS_COUNT 2000

#define DATA_NAME "cnvalias"
#define DATA_TYPE "dat"

/* UDataInfo cf. udata.h */
static const UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    0x43, 0x76, 0x41, 0x6c,     /* dataFormat="CvAl" */
    2, 0, 0, 0,                 /* formatVersion */
    1, 4, 2, 0                  /* dataVersion */
};

static char stringStore[STRING_STORE_SIZE];
static uint32_t stringTop=0;

typedef struct {
    const char *alias;
    uint16_t converter;
} Alias;

static Alias aliases[MAX_ALIAS_COUNT];
static uint16_t aliasCount=0;

typedef struct {
    const char *converter;
    uint16_t aliasCount;
} Converter;

static Converter converters[MAX_ALIAS_COUNT];
static uint16_t converterCount=0;

/* prototypes --------------------------------------------------------------- */

static void
parseLine(const char *line);

static void
addAlias(const char *alias, uint16_t converter);

static uint16_t
addConverter(const char *converter);

static char *
allocString(uint32_t length);

static int
compareAliases(const void *file1, const void *file2);

static int
compareConverters(const void *converter1, const void *converter2);

/* -------------------------------------------------------------------------- */

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_COPYRIGHT,
    UOPTION_DESTDIR,
    UOPTION_SOURCEDIR
};

extern int
main(int argc, const char *argv[]) {
    char line[512];
    const char *path;
    FileStream *in;
    UNewDataMemory *out;
    char *s;
    UErrorCode errorCode=U_ZERO_ERROR;
    int i;
    uint16_t stringOffset;

    /* preset then read command line options */
    options[3].value=options[4].value=u_getDataDirectory();
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }
    if(argc<0 || options[0].doesOccur || options[1].doesOccur) {
        fprintf(stderr,
            "usage: %s [-options] [convrtrs.txt]\n"
            "\tread convrtrs.txt and create " DATA_NAME "." DATA_TYPE "\n"
            "\toptions:\n"
            "\t\t-h or -? or --help  this usage text\n"
            "\t\t-c or --copyright   include a copyright notice\n"
            "\t\t-d or --destdir     destination directory, followed by the path\n"
            "\t\t-s or --sourcedir   source directory, followed by the path\n",
            argv[0]);
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    if(argc>=2) {
        path=argv[1];
    } else {
        path=options[4].value;
        if(path!=NULL && *path!=0) {
            uprv_strcpy(line, path);
            path=line+uprv_strlen(line);
            if(*(path-1)!=U_FILE_SEP_CHAR) {
                *((char *)path)=U_FILE_SEP_CHAR;
                ++path;
            }
            uprv_strcpy((char *)path, "convrtrs.txt");
            path=line;
        } else {
            path="convrtrs.txt";
        }
    }
    in=T_FileStream_open(path, "r");
    if(in==NULL) {
        fprintf(stderr, "gencnval: unable to open input file convrtrs.txt\n");
        exit(U_FILE_ACCESS_ERROR);
    }

    /* read the list of aliases */
    while(T_FileStream_readLine(in, line, sizeof(line))!=NULL) {
        /* remove trailing newline characters */
        s=line;
        while(*s!=0) {
            if(*s=='\r' || *s=='\n') {
                *s=0;
                break;
            }
            ++s;
        }

        parseLine(line);
    }

    T_FileStream_close(in);

    /* sort the aliases */
    qsort(aliases, aliasCount, sizeof(Alias), compareAliases);

    /* create the output file */
    out=udata_create(options[3].value, DATA_TYPE, DATA_NAME, &dataInfo,
                     options[2].doesOccur ? U_COPYRIGHT_STRING : NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gencnval: unable to open output file - error %s\n", u_errorName(errorCode));
        exit(errorCode);
    }

    /* determine the length of tables for the data offset of the strings */
    stringOffset=2+4*aliasCount+2+4*converterCount;

    /* write the table of aliases */
    udata_write16(out, aliasCount);
    for(i=0; i<aliasCount; ++i) {
        udata_write16(out, (uint16_t)(stringOffset+(aliases[i].alias-stringStore)));
    }
    for(i=0; i<aliasCount; ++i) {
        udata_write16(out, aliases[i].converter);
    }

    /* write the table of converters */
    udata_write16(out, converterCount);
    for(i=0; i<converterCount; ++i) {
        udata_write16(out, (uint16_t)(stringOffset+(converters[i].converter-stringStore)));
        udata_write16(out, converters[i].aliasCount);
    }

    /* write the strings */
    udata_writeString(out, stringStore, stringTop);

    /* finish */
    udata_finish(out, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gencnval: error finishing output file - %s\n", u_errorName(errorCode));
        exit(errorCode);
    }

    return 0;
}

static void
parseLine(const char *line) {
    uint16_t pos=0, start, limit, length, cnv;
    char *converter, *alias;

    /* skip leading white space */
    while(line[pos]!=0 && isspace((unsigned char)line[pos])) {
        ++pos;
    }

    /* is there only a comment on this line? */
    if(line[pos]==0 || line[pos]=='#') {
        return;
    }

    /* get the converter name */
    start=pos;
    while(line[pos]!=0 && line[pos]!='{' && line[pos]!='#' && !isspace((unsigned char)line[pos])) {
        ++pos;
    }
    limit=pos;

    if (line[pos] == '{') {   /* skip over tags */
        ++pos;
        while (line[pos] && line[pos] != '}' && line[pos] != '#') {
            ++pos;
        }
        if (line[pos] == '}') {
            ++pos;
        } else {
            fprintf(stderr, "unterminated tag list: %s\n", line);
            exit(U_PARSE_ERROR);
        }
    }
    /* store the converter name */
    length=limit-start;
    converter=allocString(length+1);
    uprv_memcpy(converter, line+start, length);
    converter[length]=0;

    /* add the converter to the converter table */
    cnv=addConverter(converter);

    /* add the converter as its own alias to the alias table */
    addAlias(converter, cnv);

    /* get all the real aliases */
    for(;;) {
        /* skip white space */
        while(line[pos]!=0 && isspace((unsigned char)line[pos])) {
            ++pos;
        }

        /* is there no more alias name on this line? */
        if(line[pos]==0 || line[pos]=='#') {
            break;
        }

        /* get an alias name */
        start=pos;
        while(line[pos]!=0 && line[pos]!='{' && line[pos]!='#' && !isspace((unsigned char)line[pos])) {
            ++pos;
        }
        limit=pos;

        /* store the alias name */
        length=limit-start;
        alias=allocString(length+1);
        uprv_memcpy(alias, line+start, length);
        alias[length]=0;

        /* add the alias/converter pair to the alias table */
        addAlias(alias, cnv);

        /* skip whitespace */
        while(line[pos]!=0 && isspace((unsigned char)line[pos])) {
            ++pos;
        }
        if (line[pos] == '{') {   /* skip over tags */
            start = ++pos;
            while (line[pos] && line[pos] != '}' && line[pos] != '#') {
                ++pos;
            }
            if (line[pos] == '}') {
                ++pos;
            } else {
                fprintf(stderr, "unterminated tag list: %s\n", line);
                exit(U_PARSE_ERROR);
            }
        }
    }
}

static void
addAlias(const char *alias, uint16_t converter) {
    if(aliasCount==MAX_ALIAS_COUNT) {
        fprintf(stderr, "gencnval: too many aliases\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    aliases[aliasCount].alias=alias;
    aliases[aliasCount].converter=converter;

    ++aliasCount;

    ++converters[converter].aliasCount;
}

static uint16_t
addConverter(const char *converter) {
    if(converterCount==MAX_ALIAS_COUNT) {
        fprintf(stderr, "gencnval: too many converters\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    converters[converterCount].converter=converter;
    converters[converterCount].aliasCount=0;

    return converterCount++;
}

static char *
allocString(uint32_t length) {
    uint32_t top=stringTop+length;
    char *p;

    if(top>STRING_STORE_SIZE) {
        fprintf(stderr, "gencnval: out of memory\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
    p=stringStore+stringTop;
    stringTop=top;
    return p;
}

static int
compareAliases(const void *alias1, const void *alias2) {
    return charsetNameCmp(((Alias*)alias1)->alias, ((Alias*)alias2)->alias);
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
