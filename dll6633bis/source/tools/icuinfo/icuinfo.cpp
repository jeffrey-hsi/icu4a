/*
*******************************************************************************
*
*   Copyright (C) 1999-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  gentest.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000mar03
*   created by: Madhu Katragadda
*
*   This program shows some basic info about the current ICU.
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uclean.h"
#include "unicode/udata.h"
#include "unicode/udbgutil.h"
#include "unewdata.h"
#include "cmemory.h"
#include "cstring.h"
#include "uoptions.h"
#include "toolutil.h"
#include "icuplugimp.h"
#include <unicode/uloc.h>
#include <unicode/ucnv.h>
#include "putilimp.h"


static UOption options[]={
  /*0*/ UOPTION_HELP_H,
  /*1*/ UOPTION_HELP_QUESTION_MARK,
  /*2*/ UOPTION_DEF("interactive", 'i', UOPT_NO_ARG),
  /*3*/ UOPTION_VERBOSE,
  /*4*/ UOPTION_DEF("list-plugins", 'L', UOPT_NO_ARG),
};

void *theLib = NULL;

void printVersion(const uint8_t  *v)
{
  int i;
  for(i=0;i<4;i++)
    fprintf(stdout, "%3d%c", (int)v[i], (i==3?' ':'.'));

  for(i=0;i<4;i++)
    fprintf(stdout, "%c", isprint(v[i])?v[i]:'_');

  fprintf(stdout, "\n");
}

void printInfo(const UDataInfo *info)
{
    printf("Size: %d, Endianness: %c, Charset family: %c, ",
	   (int)info->size,
	   "lB?"[info->isBigEndian],
	   "AE?"[info->charsetFamily]);
    
    printf("UChar=%d bytes.\n", info->sizeofUChar);

    printf("dataFormat   =");
    printVersion(info->dataFormat);
    printf("formatVersion=");
    printVersion(info->formatVersion);
    printf("dataVersion  =");
    printVersion(info->dataVersion);
}

#if 0
void cmd_C(const char */*buf*/, UErrorCode *status)
{
    if(theLib != NULL)
    {
        uplug_closeLibrary(theLib, status);
        if(U_FAILURE(*status))
        {
            fprintf(stderr, "Closed, with err %s\n", u_errorName(*status));
        } else {
            fprintf(stderr, "Closed.\n");
        }
        theLib = NULL;
    }
}

void cmd_O(const char *buf, UErrorCode *status)
{
    void *p;

    if((buf[1]!=' ')||(buf[2]==0))
    {
        fprintf(stderr, "Usage:  O library...\n");
        return;
    }

    /* close the buffer if it is open. */
    cmd_C(buf, status);

    fprintf(stderr, "Opening: [%s]\n", buf+2);
    p = uplug_openLibrary(buf+2, status);
    if(!p || U_FAILURE(*status)) {
        fprintf(stderr, "Didnt' open. %s\n", u_errorName(*status));
    }
    else
    {
        fprintf(stderr, " -> %p\n", p);
    }

    theLib = p;
}

void cmd_S(char *buf, UErrorCode *status)
{
    UErrorCode err = U_ZERO_ERROR;
    char *equals = NULL;
    void *foo;
    const char *lib;
    
    if(buf[0]=='S')
    {
        equals=strchr(buf, '=');
        
        if(!equals||strlen(buf)<5)
        {
            fprintf(stderr, "usage:  S myapp_dat=myapp\n");
            return;
        }
        *equals=0;
        equals++;
    }
    else if(buf[0]=='I')
    {
        if(strlen(buf)<5 || buf[1]!=' ')
        {
            fprintf(stderr, "Usage: I " U_ICUDATA_NAME "_dat\n");
            return;
        }
        equals=strdup("ICUDATA");
    }
    
    lib = (buf + 2);

    if(theLib == NULL)
    {
        fprintf(stderr, "loading global %s as package %s\n", lib, equals);
    }
    else
    {
        fprintf(stderr, "loading %p's %s as package %s\n", theLib, lib, equals);
    }
    fflush(stderr);

    foo = uprv_dl_sym(theLib, lib, status);
    if(!foo || U_FAILURE(*status)) {
        fprintf(stderr, " Couldn't dlsym(%p,%s) - %s\n", theLib, lib, u_errorName(*status));
        return;
    }
    if(buf[0]=='S') {
        udata_setAppData(equals, foo, &err);
        if(U_FAILURE(err))
        {
            fprintf(stderr, " couldn't setAppData(%s, %p, ...) - %s\n",
                    equals, foo, u_errorName(err));
            return;
        }
        fprintf(stderr, " Set app data - %s\n", u_errorName(err));

    } else if(buf[0]=='I') {
        udata_setCommonData(foo, &err);
        if(U_FAILURE(err))
        {
            fprintf(stderr, " couldn't setCommonData(%s, %p, ...) - %s\n",
                    equals, foo, u_errorName(err));
            return;
        }
        fprintf(stderr, " Set cmn data - %s\n", u_errorName(err));
        
    } else
    {
        fprintf(stderr, "Unknown cmd letter '%c'\n", buf[0]);
        return;
    }

}
#endif


#define CAN_DYNAMIC_LOAD 1

void
cmd_help()
{
/*
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\th                     - print the top 256 bytes of the item\n");
    fprintf(stderr, "\ti                     - print info on current item\n");
    fprintf(stderr, "\tl %cpath%cpkg|name.type - load item of path, package, 'name' and 'type'\n", U_FILE_SEP_CHAR, U_FILE_SEP_CHAR);
    fprintf(stderr, "\tl name.type           - load item of 'name' and 'type'\n");
    fprintf(stderr, "\tl pkg|name.type       - load item of package, 'name' and 'type'\n");
    fprintf(stderr, "\tp                     - print the path\n");
    fprintf(stderr, "\tp=                    - set the path to empty\n");
    fprintf(stderr, "\tp=%ca%cb%c%cstuff%cmypkg.dat - set the path\n", U_FILE_SEP_CHAR,U_FILE_SEP_CHAR,U_PATH_SEP_CHAR,U_FILE_SEP_CHAR, U_FILE_SEP_CHAR,U_PATH_SEP_CHAR, U_FILE_SEP_CHAR, U_FILE_SEP_CHAR);
    fprintf(stderr, "\tq                     - quit the program\n");
    fprintf(stderr, "\tu                     - unload data item\n");
    fprintf(stderr, "\tv                     - print version and info (Loads data!)\n");
    fprintf(stderr, "\t?                     - show this help\n");
#if CAN_DYNAMIC_LOAD
    fprintf(stderr, " Dynamic Load Functions:\n");
    fprintf(stderr, "\tO whatever.dll        - DLL load\n");
    fprintf(stderr, "\tC                     - close DLL\n");
    fprintf(stderr, "\tS myapp_dat=myapp     - load app data myapp from package myapp_dat\n");
    fprintf(stderr, "\tI icuwhatever_dat     - load ICU data\n");
    fprintf(stderr, "\tI " U_ICUDATA_NAME "      ( default for 'I')\n");
#endif
*/
    fprintf(stderr, "No help available yet, sorry. \n");
}

const char *prettyDir(const char *d)
{
    if(d == NULL) return "<NULL>";
    if(*d == 0) return "<EMPTY>";
    return d;
}

void cmd_version(UBool noLoad)
{
    UVersionInfo icu;
    UErrorCode status = U_ZERO_ERROR;
    char str[200];
    u_getVersion(icu);
    u_versionToString(icu, str);
    fprintf(stderr, "\nCompiled against ICU " U_ICU_VERSION ", currently running ICU %s\n", str);
    fprintf(stderr, "ICUDATA is %s\n", U_ICUDATA_NAME);
    u_init(&status);
    fprintf(stderr, "u_init returned %s\n", u_errorName(status));
    fprintf(stderr, "plugin file is: %s\n", uplug_getPluginFile());
}

void cmd_cleanup(UBool noLoad)
{
    u_cleanup();
    fprintf(stderr,"u_cleanup() returned.\n");
    fprintf(stderr, "Default locale is %s\n", uloc_getDefault());
    if(noLoad == FALSE)
    {
        fprintf(stderr, "Default converter is %s.\n", ucnv_getDefaultName());
    }
}


void cmd_listplugins() {
    int32_t i;
    UPlugData *plug;
    
    printf("Plugins: \n");
    printf(    "# %6s   %s \n",
                       "Level",
                       "Name" );
    printf(    "    %10s:%-10s\n",
                       "Library",
                       "Symbol"
            );

                       
    printf(    "       config| (configuration string)\n");
    printf(    " >>>   Error          | Explanation \n");
    printf(    "-----------------------------------\n");
        
    for(i=0;(plug=uplug_getPlugInternal(i))!=NULL;i++) {
        UErrorCode libStatus = U_ZERO_ERROR;
        const char *name = uplug_getPlugName(plug);
        const char *sym = uplug_getSymbolName(plug);
        const char *lib = uplug_getLibraryName(plug, &libStatus);
        const char *config = uplug_getConfiguration(plug);
        UErrorCode loadStatus = uplug_getPlugLoadStatus(plug);
        const char *message = NULL;
        
        printf("\n#%d  %-6s %s \n",
            i+1,
            udbg_enumName(UDBG_UPlugLevel,(int32_t)uplug_getPlugLevel(plug)),
            name!=NULL?(*name?name:"this plugin did not call uplug_setPlugName()"):"(null)"
        );
        printf("    plugin| %10s:%-10s\n",
            (U_SUCCESS(libStatus)?(lib!=NULL?lib:"(null)"):u_errorName(libStatus)),
            sym!=NULL?sym:"(null)"
        );
        
        if(config!=NULL&&*config) {
            printf("    config| %s\n", config);
        }
        
        switch(loadStatus) {
            case U_PLUGIN_CHANGED_LEVEL_WARNING:
                message = "Note: This plugin changed the system level (by allocating memory or calling something which does). Later plugins may not load.";
                break;
                
            case U_PLUGIN_DIDNT_SET_LEVEL:
                message = "Error: This plugin did not call uplug_setPlugLevel during QUERY.";
                break;
            
            case U_PLUGIN_TOO_HIGH:
                message = "Error: This plugin couldn't load because the system level was too high. Try loading this plugin earlier.";
                break;
                
            case U_ZERO_ERROR: 
                message = NULL; /* no message */
                break;
            default:
                if(U_FAILURE(loadStatus)) {
                    message = "error loading:";
                } else {
                    message = "warning during load:";
                }            
        }
        
        if(message!=NULL) {
            printf("\\\\\\ status| %s\n"
                   "/// %s\n", u_errorName(loadStatus), message);
        }
        
    }
}


void cmd_path(const char *buf)
{
    if(buf[1]=='=')
    {
        fprintf(stderr, "ICU data path was %s\n", prettyDir(u_getDataDirectory()));
        if((buf[2]==0)||(buf[2]==' '))
        {
            u_setDataDirectory("");
            fprintf(stderr, "ICU data path set to EMPTY\n");
        }
        else
        {
            u_setDataDirectory(buf+2);
            fprintf(stderr, "ICU data path set to %s\n", buf+2);
        }
    }
    fprintf(stderr, "ICU data path is  %s\n", prettyDir(u_getDataDirectory()));
}

void cmd_unload(const char * /*buf*/, UDataMemory *old)
{
    if(old)
    {
        fprintf(stderr, "Unloading data at %p\n", (void*)old);
        udata_close(old);
    }
}

UDataMemory *cmd_load(char *buf, UDataMemory *old)
{
    const char *pkg;
    char *name;
    char *type;
    UDataMemory *data;

    UErrorCode errorCode=U_ZERO_ERROR;
    cmd_unload(buf, old);


    if((buf[1] != ' ')||(buf[2]==0))
    {
        fprintf(stderr, "Load: Load what?\n");
        fprintf(stderr, "Use ? for help.\n");
        return NULL;
    }

    name = strchr(buf+2, '|');
    if(name == NULL)  /* separator not found */
    {
        pkg = NULL;
        name = buf+2;
    }
    else
    {
        pkg = buf+2; /* starts with pkg name */
        *name = 0; /* zap | */
        name++;
    }
    
    type = strchr(name, '.');
    if(type == NULL)
    {
        fprintf(stderr, "Bad type. Use ? for help.\n");
        return NULL;
    }
    
    *type = 0;
    type++;

    if(*type == 0)
    {
        fprintf(stderr, "Bad type. Use ? for help.\n");
        return NULL;
    }

    if(*name == 0)
    {
        fprintf(stderr, "Bad name. use ? for help.\n");
        return NULL;
    }
    
    fprintf(stderr, "Attempting to load::    %s | %s . %s  ", 
           prettyDir(pkg),
           name,
           type);

    fflush(stdout);
    fflush(stderr);
    data = udata_open(pkg, type, name, &errorCode);
    fflush(stdout);
    fflush(stderr);
    fprintf(stderr, " ->  %s (%p)\n", u_errorName(errorCode), (void*)data);
    
    if(U_FAILURE(errorCode) && data)
    {
        fprintf(stderr, "Forcing data %p to close.\n", (void*)data);
        udata_close(data);
        data = NULL;
    }
    
    return data;
}

void cmd_info(const char */*buf*/, UDataMemory *data)
{
    UDataInfo info;

    if(data == NULL) {
        fprintf(stderr, "Err, no data loaded\n");
        return;
    }

    info.size = sizeof(info);
    udata_getInfo(data, &info);
    
    printInfo(&info);
}

void
doInteractive()
{
    UDataMemory *data=NULL;

    char linebuf[1024];
    char loaded[1024];
#ifdef HAVE_READLINE
    char *rl;
#endif

    cmd_version(TRUE);
    cmd_path("p");
    fprintf(stderr, "\nEntering interactive mode. Typing ? gets help.\n");
#if CAN_DYNAMIC_LOAD
    fprintf(stderr, "(Dynamic loading available.)");
#endif
#ifdef HAVE_READLINE
    fprintf(stderr, "(readline mode.)\n");
    while(rl=readline("==> "))
    {
        strcpy(linebuf, rl);
#else
    fprintf(stderr, "==> ");
    while(!feof(stdin) && fgets(linebuf, 1024, stdin))
    {
        UErrorCode status = U_ZERO_ERROR;
#endif
        if(linebuf[strlen(linebuf)-1]=='\n')
        {
            linebuf[strlen(linebuf)-1]=0;
        }

        switch(linebuf[0])
        {
#if 0 && CAN_DYNAMIC_LOAD
        case 'O':
            cmd_O(linebuf, &status);
            break;
        case 'C':
            cmd_C(linebuf, &status);
            break;
        case 'S':
            cmd_S(linebuf, &status);
            break;
        case 'I':
            cmd_S(linebuf, &status);
            break;
#endif

        case '!':
            system(linebuf+1);
            break;

        case '?':
            cmd_help();
            break;

        case 'i':
            cmd_info(linebuf, data);
            break;
            
        case 'v':
            cmd_version(FALSE);
            break;

        case 'l':
            strcpy(loaded, linebuf);
            data = cmd_load(linebuf, data);
            break;
            
        case 'p':
            cmd_path(linebuf);
            break;

        case 'q':
            fprintf(stderr, "Bye.\n");
            cmd_unload(linebuf, data);
            return;
            break;
            
        default: 
            fprintf(stderr, "Unknown option '%c' (use ? for help).\n", linebuf[0]);
            break;

        case 0:
        case ' ':
            ;
        }

#ifndef HAVE_READLINE
        fprintf(stderr, "==> ");
#else
        free(rl);
#endif
    }
}


extern int
main(int argc, char* argv[]) {
    UErrorCode errorCode = U_ZERO_ERROR;
    UBool didSomething = FALSE;
    
    /* preset then read command line options */
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }
    if( options[0].doesOccur || options[1].doesOccur) {
        fprintf(stderr,
            "usage: %s [-options]\n"
            "\toptions:\n"
            "\t\t-h or -? or --help  this usage text\n",
            argv[0]);
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }
    
    if(options[2].doesOccur) {
        doInteractive();
    } else {
        if(options[3].doesOccur) {
            cmd_version(FALSE);
            didSomething = TRUE;
        }
        
        if(options[4].doesOccur) {
            cmd_listplugins();
            didSomething = TRUE;
        }

        if(options[3].doesOccur) {  /* 2nd part of version: cleanup */
            cmd_cleanup(FALSE);
            didSomething = TRUE;
        }
        
        if(!didSomething) {
            cmd_version(FALSE);  /* at least print the version # */
        }
    }

    return U_FAILURE(errorCode);
}
