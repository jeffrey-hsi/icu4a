/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1998-1999, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*
* File test.c
*
* Modification History:
*
*   Date          Name        Description
*   07/24/2000    Madhu       Creation 
*******************************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/utf8.h"
#include "cmemory.h"
#include "cintltst.h"
#include <stdio.h>


static void printUChars(uint8_t *uchars, int16_t len);

static void TestCodeUnitValues(void);
static void TestCharLength(void);
static void TestGetChar(void);
static void TestNextPrevChar(void);
static void TestFwdBack(void);
static void TestSetChar(void);
static void TestAppendChar(void);



void
addUTF8Test(TestNode** root)
{
  addTest(root, &TestCodeUnitValues,    "utf8tst/TestCodeUnitValues");
  addTest(root, &TestCharLength,        "utf8tst/TestCharLength"    );
  addTest(root, &TestGetChar,           "utf8tst/TestGetChar"       );
  addTest(root, &TestNextPrevChar,      "utf8tst/TestNextPrevChar"  );
  addTest(root, &TestFwdBack,           "utf8tst/TestFwdBack"       );
  addTest(root, &TestSetChar,           "utf8tst/TestSetChar"       );
  addTest(root, &TestAppendChar,        "utf8tst/TestAppendChar"    );
}

static void TestCodeUnitValues()
{
    static uint8_t codeunit[]={0x00, 0x65, 0x7e, 0x7f, 0xc0, 0xc4, 0xf0, 0xfd, 0x80, 0x81, 0xbc, 0xbf,};
    
    int16_t i;
    for(i=0; i<sizeof(codeunit)/sizeof(codeunit[0]); i++){
        uint8_t c=codeunit[i];
        log_verbose("Testing code unit value of %x\n", c);
        if(i<4){
            if(!UTF8_IS_SINGLE(c) || UTF8_IS_LEAD(c) || UTF8_IS_TRAIL(c)){
                log_err("ERROR: 0x%02x is a single byte but results in single: %c lead: %c trail: %c\n",
                    c, UTF8_IS_SINGLE(c) ? 'y' : 'n', UTF8_IS_LEAD(c) ? 'y' : 'n', UTF8_IS_TRAIL(c) ? 'y' : 'n');
            }
        } else if(i< 8){
            if(!UTF8_IS_LEAD(c) || UTF8_IS_SINGLE(c) || UTF8_IS_TRAIL(c)){
                log_err("ERROR: 0x%02x is a lead byte but results in single: %c lead: %c trail: %c\n",
                    c, UTF8_IS_SINGLE(c) ? 'y' : 'n', UTF8_IS_LEAD(c) ? 'y' : 'n', UTF8_IS_TRAIL(c) ? 'y' : 'n');
            }
        } else if(i< 12){
            if(!UTF8_IS_TRAIL(c) || UTF8_IS_SINGLE(c) || UTF8_IS_LEAD(c)){
                log_err("ERROR: 0x%02x is a trail byte but results in single: %c lead: %c trail: %c\n",
                    c, UTF8_IS_SINGLE(c) ? 'y' : 'n', UTF8_IS_LEAD(c) ? 'y' : 'n', UTF8_IS_TRAIL(c) ? 'y' : 'n');
            }
        }
    }
}

static void TestCharLength()
{
    static uint32_t codepoint[]={
        1, 0x0061,
        1, 0x007f,
        2, 0x016f,
        2, 0x07ff,
        3, 0x0865,
        3, 0x20ac,
        4, 0x20402,
        4, 0x23456,
        4, 0x24506,
        4, 0x20402,
        4, 0x10402,
        3, 0xd7ff,
        3, 0xe000,
        
    };
    
    int16_t i;
    UBool multiple;
    for(i=0; i<sizeof(codepoint)/sizeof(codepoint[0]); i=(int16_t)(i+2)){
        UChar32 c=codepoint[i+1];
        if(UTF8_CHAR_LENGTH(c) != (uint16_t)codepoint[i]){
              log_err("The no: of code units for %lx:- Expected: %d Got: %d\n", c, codepoint[i], UTF8_CHAR_LENGTH(c));
        }else{
              log_verbose("The no: of code units for %lx is %d\n",c, UTF8_CHAR_LENGTH(c) ); 
        }
        multiple=(UBool)(codepoint[i] == 1 ? FALSE : TRUE);
        if(UTF8_NEED_MULTIPLE_UCHAR(c) != multiple){
              log_err("ERROR: UTF8_NEED_MULTIPLE_UCHAR failed for %lx\n", c);
        }
    }
}

static void TestGetChar()
{
    static uint8_t input[]={
    /*  code unit,*/
        0x61,
        0x7f,
        0xe4,
        0xba, 
        0x8c,
        0xF0, 
        0x90, 
        0x90, 
        0x81,
        0xc0,
        0x65,
        0x31,
        0x9a,
        0xc9,
    };
    static uint32_t result[]={
     /*codepoint-unsafe,  codepoint-safe(not strict)  codepoint-safe(strict)*/
        0x61,             0x61,                       0x61, 
        0x7f,             0x7f,                       0x7f, 
        0x4e8c,           0x4e8c,                     0x4e8c,
        0x4e8c,           0x4e8c,                     0x4e8c ,
        0x4e8c,           0x4e8c,                     0x4e8c,
        0x10401,          0x10401,                    0x10401 ,
        0x10401,          0x10401,                    0x10401 ,
        0x10401,          0x10401,                    0x10401 ,
        0x10401,          0x10401,                    0x10401,
        0x25,             UTF8_ERROR_VALUE_1,         UTF8_ERROR_VALUE_1,
        0x65,             0x65,                       0x65,  
        0x31,             0x31,                       0x31,  
        0x31,             0x9a,                       0x9a,
        0x240,            UTF8_ERROR_VALUE_1,         UTF8_ERROR_VALUE_1,
    };
    uint16_t i=0;
    UChar32 c;
    uint32_t offset=0;

    for(offset=0; offset<sizeof(input); offset++) {
        if (offset < sizeof(input) - 1) {
            UTF8_GET_CHAR_UNSAFE(input, offset, c);
            if(c != result[i]){
                log_err("ERROR: UTF8_GET_CHAR_UNSAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, result[i], c);
           
            }
        }
        UTF8_GET_CHAR_SAFE(input, 0, offset, sizeof(input), c, FALSE);
        if(c != result[i+1]){
            log_err("ERROR: UTF8_GET_CHAR_SAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, result[i+1], c);
        }
        UTF8_GET_CHAR_SAFE(input, 0, offset, sizeof(input), c, TRUE);
        if(c != result[i+2]){
            log_err("ERROR: UTF8_GET_CHAR_SAFE(strict) failed for offset=%ld. Expected:%lx Got:%lx\n", offset, result[i+2], c);
        }
         
         i=(uint16_t)(i+3);
    }

}

static void TestNextPrevChar(){

    static uint8_t input[]={0x61, 0xf0, 0x90, 0x90, 0x81, 0xc0, 0x80, 0xfd, 0xbf, 0xc0, 0x61, 0x81, 0x90, 0x90, 0xf0, 0x00};
    static uint32_t result[]={
    /*next_unsafe    next_safe_ns        next_safe_s          prev_unsafe   prev_safe_ns         prev_safe_s*/
        0x0061,        0x0061,             0x0061,              0x0000,       0x0000,             0x0000,
        0x10401,       0x10401,            0x10401,             0xf0,         0xf0,               0xf0, 
        0x90,          0x90,               0x90,                0x841410,     UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_1,
        0x90,          0x90,               0x90,                0x21050,      UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_1,
        0x81,          0x81,               0x81,                0x841,        UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_1,
        0x00,          0x00,               UTF8_ERROR_VALUE_2,  0x61,         0x61,               0x61,
        0x80,          0x80,               0x80,                0xc0,         0xc0,               0xc0,
        0xfd,          UTF8_ERROR_VALUE_2, UTF8_ERROR_VALUE_2,  0x77f,        UTF8_ERROR_VALUE_2, UTF8_ERROR_VALUE_2,
        0xbf,          0xbf,               0xbf,                0xfd,         0xfd,               0xfd,
        0x21,          UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_1,  0x00,         0x00,               UTF8_ERROR_VALUE_2, 
        0x61,          0x61,               0x61,                0xc0,         0xc0,               0xc0, 
        0x81,          0x81,               0x81,                0x10401,      0x10401,            0x10401, 
        0x90,          0x90,               0x90,                0x410,        UTF_ERROR_VALUE,    UTF_ERROR_VALUE,
        0x90,          0x90,               0x90,                0x410,        UTF8_ERROR_VALUE_2, UTF8_ERROR_VALUE_2, 
        0x0840,        UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_1,  0xf0,         0xf0,               0xf0, 
        0x0000,        0x0000,             0x0000,              0x0061,       0x0061,             0x0061,

    };
    static UTextOffset movedOffset[]={
   /*next_unsafe    next_safe_ns  next_safe_s       prev_unsafe   prev_safe_ns     prev_safe_s*/
        1,            1,           1,                15,           15,               15,
        5,            5,           5,                14,           14 ,              14, 
        3,            3,           3,                9,            13,               13, 
        4,            4,           4,                9,            12,               12,
        5,            5,           5,                9,            11,               11, 
        7,            7,           7,                10,           10,               10,  
        7,            7,           7,                9,            9,                9,  
        8,            9,           9,                7,            7,                7, 
        9,            9,           9,                7,            7,                7,  
        11,           10,          10,               5,            5,                5,    
        11,           11,          11,               5,            5,                5,   
        12,           12,          12,               1,            1,                1, 
        13,           13,          13,               1,            1,                1,   
        14,           14,          14,               1,            1,                1,      
        14,           15,          15,               1,            1,                1,  
        14,           16,          16,               0,            0,                0, 


    };


    UChar32 c=0x0000;
    uint32_t i=0;
    uint32_t offset=0;
    UTextOffset setOffset=0;
    for(offset=0; offset<sizeof(input); offset++){
         if (offset < sizeof(input) - 2) { /* Can't have it go off the end of the array based on input */
             setOffset=offset;
             UTF8_NEXT_CHAR_UNSAFE(input, setOffset, c);
             if(setOffset != movedOffset[i]){
                 log_err("ERROR: UTF8_NEXT_CHAR_UNSAFE failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                     offset, movedOffset[i], setOffset);
             }
             if(c != result[i]){
                 log_err("ERROR: UTF8_NEXT_CHAR_UNSAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, result[i], c);
             }
         }
         setOffset=offset;
         UTF8_NEXT_CHAR_SAFE(input, setOffset, sizeof(input), c, FALSE);
         if(setOffset != movedOffset[i+1]){
             log_err("ERROR: UTF8_NEXT_CHAR_SAFE failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                 offset, movedOffset[i+1], setOffset);
         }
         if(c != result[i+1]){
             log_err("ERROR: UTF8_NEXT_CHAR_SAFE failed for input=%ld. Expected:%lx Got:%lx\n", offset, result[i+1], c);
         }
         setOffset=offset;
         UTF8_NEXT_CHAR_SAFE(input, setOffset, sizeof(input), c, TRUE);
         if(setOffset != movedOffset[i+1]){
             log_err("ERROR: UTF8_NEXT_CHAR_SAFE(strict) failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                 offset, movedOffset[i+2], setOffset);
         }
         if(c != result[i+2]){
             log_err("ERROR: UTF8_NEXT_CHAR_SAFE(strict) failed for input=%ld. Expected:%lx Got:%lx\n", offset, result[i+2], c);
         }
         /*call the API instead of MACRO
         setOffset=offset;
         (c)=(input)[(setOffset)++]; 
         if(UTF8_IS_LEAD(c)) { 
             (c)=utf8_nextCharSafeBody(input, &(setOffset), sizeof(input), c, TRUE); 
         } 
         if(setOffset != movedOffset[i+1]){
             log_err("ERROR: utf8_nextCharSafeBody(strict) failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                 offset, movedOffset[i+2], setOffset);
         }
         if(c != result[i+2]){
             log_err("ERROR: utf8_nextCharSafeBody(strict) failed for input=%ld. Expected:%lx Got:%lx\n", offset, result[i+2], c);
         }
         */

         i=i+6;
    }
    i=0;
    for(offset=sizeof(input); offset > 0; --offset){
         setOffset=offset;
         UTF8_PREV_CHAR_UNSAFE(input, setOffset, c);
         if(setOffset != movedOffset[i+3]){
             log_err("ERROR: UTF8_PREV_CHAR_UNSAFE failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                 offset, movedOffset[i+3], setOffset);
         }
         if(c != result[i+3]){
             log_err("ERROR: UTF8_PREV_CHAR_UNSAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, result[i+3], c);
         }
         setOffset=offset;
         UTF8_PREV_CHAR_SAFE(input, 0, setOffset, c, FALSE);
         if(setOffset != movedOffset[i+4]){
             log_err("ERROR: UTF8_PREV_CHAR_SAFE failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                 offset, movedOffset[i+4], setOffset);
         }
         if(c != result[i+4]){
             log_err("ERROR: UTF8_PREV_CHAR_SAFE failed for input=%ld. Expected:%lx Got:%lx\n", offset, result[i+4], c);
         }
         setOffset=offset;
         UTF8_PREV_CHAR_SAFE(input, 0,  setOffset, c, TRUE);
         if(setOffset != movedOffset[i+5]){
             log_err("ERROR: UTF8_PREV_CHAR_SAFE(strict) failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                 offset, movedOffset[i+5], setOffset);
         } 
         if(c != result[i+5]){
             log_err("ERROR: UTF8_PREV_CHAR_SAFE(strict) failed for input=%ld. Expected:%lx Got:%lx\n", offset, result[i+5], c);
         }
         /*call the API instead of MACRO
         setOffset=offset;
         (c)=(input)[--(setOffset)]; 
         if(UTF8_IS_TRAIL((c))) { 
             (c)=utf8_prevCharSafeBody(input, 0, &(setOffset), c, TRUE); 
         } 
         if(setOffset != movedOffset[i+5]){
             log_err("ERROR: utf8_prevCharSafeBody(strict) failed to move the offset correctly at %d\n ExpectedOffset:%d Got %d\n",
                 offset, movedOffset[i+5], setOffset);
         } 
         if(c != result[i+5]){
             log_err("ERROR: utf8_prevCharSafeBody(strict) failed for input=%ld. Expected:%lx Got:%lx\n", offset, result[i+5], c);
         }
         */
         i=i+6;
    }

}

static void TestFwdBack(){ 
    static uint8_t input[]={0x61, 0xF0, 0x90, 0x90, 0x81, 0xff, 0x62, 0xc0, 0x80, 0x7f, 0x8f, 0xc0, 0x63, 0x81, 0x90, 0x90, 0xF0, 0x00};
    static uint16_t fwd_unsafe[] ={1, 5, 6, 7,  9, 10, 11, 13, 14, 15, 16,  20, };
    static uint16_t fwd_safe[]   ={1, 5, 6, 7, 9, 10, 11,  12, 13, 14, 15, 16, 17, 18};
    static uint16_t back_unsafe[]={17, 16, 12, 11, 9, 7, 6, 5, 1, 0};
    static uint16_t back_safe[]  ={17, 16, 15, 14, 13, 12, 11, 10, 9, 7, 6, 5, 1, 0};

    static uint16_t Nvalue[]= {0, 1, 2, 3, 1, 2, 1, 5};
    static uint16_t fwd_N_unsafe[] ={0, 1, 6, 10, 11, 14, 15};
    static uint16_t fwd_N_safe[]   ={0, 1, 6, 10, 11, 13, 14, 18}; /*safe macro keeps it at the end of the string */
    static uint16_t back_N_unsafe[]={18, 17, 12, 7, 6, 1, 0};
    static uint16_t back_N_safe[]  ={18, 17, 15, 12, 11, 9, 7, 0};   


    uint32_t offunsafe=0, offsafe=0;
    uint32_t i=0;
    while(offunsafe < sizeof(input)){
        UTF8_FWD_1_UNSAFE(input, offunsafe);
        if(offunsafe != fwd_unsafe[i]){
            log_err("ERROR: Forward_unsafe offset expected:%d, Got:%d\n", fwd_unsafe[i], offunsafe);
        }
        i++;
    }
    i=0;
    while(offsafe < sizeof(input)){
        UTF8_FWD_1_SAFE(input, offsafe, sizeof(input));
        if(offsafe != fwd_safe[i]){
            log_err("ERROR: Forward_safe offset expected:%d, Got:%d\n", fwd_safe[i], offsafe);
        }
        i++;
    }
    offunsafe=sizeof(input);
    i=0;
    while(offunsafe > 0){
        UTF8_BACK_1_UNSAFE(input, offunsafe);
        if(offunsafe != back_unsafe[i]){
            log_err("ERROR: Backward_unsafe offset expected:%d, Got:%d\n", back_unsafe[i], offunsafe);
        }
        i++;
    }
    i=0;
    offsafe=sizeof(input);
    while(offsafe > 0){
        UTF8_BACK_1_SAFE(input, 0,  offsafe);
        if(offsafe != back_safe[i]){
            log_err("ERROR: Backward_safe offset expected:%d, Got:%d\n", back_unsafe[i], offsafe);
        }
        i++;
    }
    offunsafe=0;
    offsafe=0;
    for(i=0; i<sizeof(Nvalue)/sizeof(Nvalue[0])-2; i++){  
        UTF8_FWD_N_UNSAFE(input, offunsafe, Nvalue[i]);
        if(offunsafe != fwd_N_unsafe[i]){
            log_err("ERROR: Forward_N_unsafe offset=%d expected:%d, Got:%d\n", i, fwd_N_unsafe[i], offunsafe);
        }
    }
    for(i=0; i<sizeof(Nvalue)/sizeof(Nvalue[0]); i++){
        UTF8_FWD_N_SAFE(input, offsafe, sizeof(input), Nvalue[i]);
        if(offsafe != fwd_N_safe[i]){
            log_err("ERROR: Forward_N_safe offset=%d expected:%d, Got:%d\n", i, fwd_N_safe[i], offsafe);
        }
    
    }
    offunsafe=sizeof(input);
    offsafe=sizeof(input);
    for(i=0; i<sizeof(Nvalue)/sizeof(Nvalue[0])-2; i++){
        UTF8_BACK_N_UNSAFE(input, offunsafe, Nvalue[i]);
        if(offunsafe != back_N_unsafe[i]){
            log_err("ERROR: backward_N_unsafe offset=%d expected:%d, Got:%d\n", i, back_N_unsafe[i], offunsafe);
        }
    }
    for(i=0; i<sizeof(Nvalue)/sizeof(Nvalue[0]); i++){
        UTF8_BACK_N_SAFE(input, 0, offsafe, Nvalue[i]);
        if(offsafe != back_N_safe[i]){
            log_err("ERROR: backward_N_safe offset=%d expected:%d, Got:%ld\n", i, back_N_safe[i], offsafe);
        }
    }
}

static void TestSetChar(){
    static uint8_t input[]={0x61, 0xe4, 0xba, 0x8c, 0x7f, 0xfe, 0x62, 0xc5, 0x7f, 0x61, 0x80, 0x80, 0xe0, 0x00};
    static uint16_t start_unsafe[]={0, 1, 1, 1, 4, 5, 6, 7, 8, 9, 9, 9, 12, 13};
    static uint16_t start_safe[]  ={0, 1, 1, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    static uint16_t limit_unsafe[]={0, 1, 4, 4, 4, 5, 6, 7, 9, 9, 10, 10, 10, 15};
    static uint16_t limit_safe[]  ={0, 1, 4, 4, 4, 5, 6, 7, 9, 9, 10, 11, 12, 14};
    
    uint32_t i=0;
    uint32_t offset=0, setOffset=0;
    for(offset=0; offset<sizeof(input); offset++){
         setOffset=offset;
         UTF8_SET_CHAR_START_UNSAFE(input, setOffset);
         if(setOffset != start_unsafe[i]){
             log_err("ERROR: UTF8_SET_CHAR_START_UNSAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, start_unsafe[i], setOffset);
         }
         setOffset=offset;
         UTF8_SET_CHAR_START_SAFE(input, 0, setOffset);
         if(setOffset != start_safe[i]){
             log_err("ERROR: UTF8_SET_CHAR_START_SAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, start_safe[i], setOffset);
         }
         /*call the API instead of MACRO
         if(UTF8_IS_TRAIL((input)[(setOffset)])) { 
              (setOffset)=utf8_back1SafeBody(input, 0, (UTextOffset)(setOffset)); 
         } 
         if(setOffset != start_safe[i]){
             log_err("ERROR: utf8_back1SafeBody failed for offset=%ld. Expected:%lx Got:%lx\n", offset, start_safe[i], setOffset);
         }
         */
         if (offset != 0) { /* Can't have it go off the end of the array */
             setOffset=offset; 
             UTF8_SET_CHAR_LIMIT_UNSAFE(input, setOffset);
             if(setOffset != limit_unsafe[i]){
                 log_err("ERROR: UTF8_SET_CHAR_LIMIT_UNSAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, limit_unsafe[i], setOffset);
             }
         }
         setOffset=offset; 
         UTF8_SET_CHAR_LIMIT_SAFE(input,0, setOffset, sizeof(input));
         if(setOffset != limit_safe[i]){
             log_err("ERROR: UTF8_SET_CHAR_LIMIT_SAFE failed for offset=%ld. Expected:%lx Got:%lx\n", offset, limit_safe[i], setOffset);
         }
         i++;
    }
}

static void TestAppendChar(){
    static uint8_t s[11]={0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00};
    static uint32_t test[]={
     /*append-position(unsafe),  CHAR to be appended  */
        0,                        0x10401,
        2,                        0x0028,
        2,                        0x007f, 
        3,                        0xd801,
        1,                        0x20402,
        8,                        0x10401,
        5,                        0xc0,
        5,                        0xc1,
        5,                        0xfd,
        6,                        0x80,
        6,                        0x81,
        6,                        0xbf,
        7,                        0xfe,

    /*append-position(safe),     CHAR to be appended */
        0,                        0x10401,
        2,                        0x0028, 
        3,                        0x7f,
        3,                        0xd801,
        1,                        0x20402,
        9,                        0x10401,
        5,                        0xc0,
        5,                        0xc1,
        5,                        0xfd,
        6,                        0x80,
        6,                        0x81,
        6,                        0xbf,
        7,                        0xfe,
   
    };
    static uint16_t movedOffset[]={
        /*offset-moved-to(unsafe)*/
          4,              /*for append-pos: 0 , CHAR 0x10401*/
          3,              
          3,
          6,
          5,
          12,
          7,
          7, 
          7,
          8,
          8,
          8,
          9,

          /*offse-moved-to(safe)*/
          4,              /*for append-pos: 0, CHAR  0x10401*/
          3,
          4,
          6,
          5,
          11,
          7,
          7, 
          7,
          8,
          8,
          8,
          9,
        
    };
          
    static uint8_t result[][11]={
        /*unsafe*/
        {0xF0, 0x90, 0x90, 0x81, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00},    
        {0x61, 0x62, 0x28, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00}, 
        {0x61, 0x62, 0x7f, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00}, 
        {0x61, 0x62, 0x63, 0xed, 0xa0, 0x81, 0x67, 0x68, 0x69, 0x6a, 0x00}, 
        {0x61, 0xF0, 0xa0, 0x90, 0x82, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00}, 
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0xF0, 0x90, 0x90},
        
        {0x61, 0x62, 0x63, 0x64, 0x65, 0xc3, 0x80, 0x68, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0xc3, 0x81, 0x68, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0xc3, 0xbd, 0x68, 0x69, 0x6a, 0x00},
        
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0xc2, 0x80, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0xc2, 0x81, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0xc2, 0xbf, 0x69, 0x6a, 0x00},

        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0xc3, 0xbe, 0x6a, 0x00},
        /*safe*/
        {0xF0, 0x90, 0x90, 0x81, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00},     
        {0x61, 0x62, 0x28, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00}, 
        {0x61, 0x62, 0x63, 0x7f, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0xed, 0xa0, 0x81, 0x67, 0x68, 0x69, 0x6a, 0x00}, 
        {0x61, 0xF0, 0xa0, 0x90, 0x82, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x00}, 
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0xc2, 0x9f}, /*gets UTF8_ERROR_VALUE_2 which takes 2 bytes 0xc0, 0x9f*/
        
        {0x61, 0x62, 0x63, 0x64, 0x65, 0xc3, 0x80, 0x68, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0xc3, 0x81, 0x68, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0xc3, 0xbd, 0x68, 0x69, 0x6a, 0x00},
        
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0xc2, 0x80, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0xc2, 0x81, 0x69, 0x6a, 0x00},
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0xc2, 0xbf, 0x69, 0x6a, 0x00},
        
        {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0xc3, 0xbe, 0x6a, 0x00},
             
    };
    uint16_t i, count=0;
    uint8_t str[12];
    uint32_t offset;
/*    UChar32 c=0;*/
    uint16_t size=sizeof(s)/sizeof(s[0]);
    for(i=0; i<sizeof(test)/sizeof(test[0]); i=(uint16_t)(i+2)){
        uprv_memcpy(str, s, size);
        offset=test[i];  
        if(count<13){
            UTF8_APPEND_CHAR_UNSAFE(str, offset, test[i+1]);
            if(offset != movedOffset[count]){
                log_err("ERROR: UTF8_APPEND_CHAR_UNSAFE failed to move the offset correctly for count=%d.\nExpectedOffset=%d  currentOffset=%d\n", 
                    count, movedOffset[count], offset);
                     
            }
            if(uprv_memcmp(str, result[count], size) !=0){
                log_err("ERROR: UTF8_APPEND_CHAR_UNSAFE failed for count=%d. \nExpected:", count);
                printUChars(result[count], size);
                printf("\nGot:      ");
                printUChars(str, size);
                printf("\n");
            }
        }else{
            UTF8_APPEND_CHAR_SAFE(str, offset, size, test[i+1]);
            if(offset != movedOffset[count]){
                log_err("ERROR: UTF8_APPEND_CHAR_SAFE failed to move the offset correctly for count=%d.\nExpectedOffset=%d  currentOffset=%d\n", 
                    count, movedOffset[count], offset);
                     
            }
            if(uprv_memcmp(str, result[count], size) !=0){
                log_err("ERROR: UTF8_APPEND_CHAR_SAFE failed for count=%d. \nExpected:", count);
                printUChars(result[count], size);
                printf("\nGot:     ");
                printUChars(str, size);
                printf("\n");
            }
            /*call the API instead of MACRO
            uprv_memcpy(str, s, size);
            offset=test[i]; 
            c=test[i+1];
            if((uint32_t)(c)<=0x7f) { 
                  (str)[(offset)++]=(uint8_t)(c); 
            } else { 
                 (offset)=utf8_appendCharSafeBody(str, (UTextOffset)(offset), (UTextOffset)(size), c); 
            }
            if(offset != movedOffset[count]){
                log_err("ERROR: utf8_appendCharSafeBody() failed to move the offset correctly for count=%d.\nExpectedOffset=%d  currentOffset=%d\n", 
                    count, movedOffset[count], offset);
                     
            }
            if(uprv_memcmp(str, result[count], size) !=0){
                log_err("ERROR: utf8_appendCharSafeBody() failed for count=%d. \nExpected:", count);
                printUChars(result[count], size);
                printf("\nGot:     ");
                printUChars(str, size);
                printf("\n");
            }
            */
        }
        count++;
    }  
   

}

static void printUChars(uint8_t *uchars, int16_t len){
    int16_t i=0;
    for(i=0; i<len; i++){
        printf("0x%02x ", *(uchars+i));
    }
}


