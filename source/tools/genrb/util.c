/*
*******************************************************************************
*
*   Copyright (C) 1998-2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File util.c
*
* Modification History:
*
*   Date        Name        Description
*   06/10/99    stephen     Creation.
*******************************************************************************
*/

#include "unicode/putil.h"
#include "util.h"
#include "cmemory.h"
#include "cstring.h"


/* go from "/usr/local/include/curses.h" to "/usr/local/include" */
void
get_dirname(char *dirname,
            const char *filename)
{
  const char *lastSlash = uprv_strrchr(filename, U_FILE_SEP_CHAR) + 1;

  if(lastSlash>filename) {
    uprv_strncpy(dirname, filename, (lastSlash - filename));
    *(dirname + (lastSlash - filename)) = '\0';
  } else {
    *dirname = '\0';
  }
}

/* go from "/usr/local/include/curses.h" to "curses" */
void
get_basename(char *basename,
             const char *filename)
{
  /* strip off any leading directory portions */
  const char *lastSlash = uprv_strrchr(filename, U_FILE_SEP_CHAR) + 1;
  char *lastDot;

  if(lastSlash>filename) {
    uprv_strcpy(basename, lastSlash);
  } else {
    uprv_strcpy(basename, filename);
  }

  /* strip off any suffix */
  lastDot = uprv_strrchr(basename, '.');

  if(lastDot != NULL) {
    *lastDot = '\0';
  }
}

#define MAX_DIGITS 10
int32_t 
itostr(char * buffer, int32_t i, uint32_t radix, int32_t pad)
{
    int32_t length = 0;
    int32_t num = 0;
    int32_t save = i;
    int digit;
    int32_t j;
    char temp;
    
    /* if i is negative make it positive */
    if(i<0){
        i=-i;
    }
    
    do{
        digit = (int)(i % radix);
        buffer[length++]=(char)(digit<=9?(0x0030+digit):(0x0030+digit+7));
        i=i/radix;
    } while(i);

    while (length < pad){
        buffer[length++] = 0x0030;/*zero padding */
    }
    
    /* if i is negative add the negative sign */
    if(save < 0){
        buffer[length++]='-';
    }

    /* null terminate the buffer */
    if(length<MAX_DIGITS){
        buffer[length] =  0x0000;
    }

    num= (pad>=length) ? pad :length;
 

    /* Reverses the string */
    for (j = 0; j < (num / 2); j++){
        temp = buffer[(length-1) - j];
        buffer[(length-1) - j] = buffer[j];
        buffer[j] = temp;
    }
    return length;
}