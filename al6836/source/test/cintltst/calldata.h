/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef COLLATION_TEST_DATA
#define COLLATION_TEST_DATA

#define COUNT_TEST_CASES 13

const UChar testSourceCases[][16] = {
    {0x61, 0x62, 0x27, 0x63, 0},
    {0x63, 0x6f, 0x2d, 0x6f, 0x70, 0},
    {0x61, 0x62, 0},
    {0x61, 0x6d, 0x70, 0x65, 0x72, 0x73, 0x61, 0x64, 0},
    {0x61, 0x6c, 0x6c, 0},
    {0x66, 0x6f, 0x75, 0x72, 0},
    {0x66, 0x69, 0x76, 0x65, 0},
    {0x31, 0},
    {0x31, 0},
    {0x31, 0},                                            /*  10 */
    {0x32, 0},
    {0x32, 0},
    {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0},
    {0x61, 0x3c, 0x62, 0},
    {0x61, 0x3c, 0x62, 0},
    {0x61, 0x63, 0x63, 0},
    {0x61, 0x63, 0x48, 0x63, 0},  /*  simple test */
    {0x70, 0x00EA, 0x63, 0x68, 0x65, 0},
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x62, 0x63, 0},                                  /*  20 */
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x00E6, 0x63, 0},
    {0x61, 0x63, 0x48, 0x63, 0},  /*  primary test */
    {0x62, 0x6c, 0x61, 0x63, 0x6b, 0},
    {0x66, 0x6f, 0x75, 0x72, 0},
    {0x66, 0x69, 0x76, 0x65, 0},
    {0x31, 0},
    {0x61, 0x62, 0x63, 0},                                        /*  30 */
    {0x61, 0x62, 0x63, 0},                                  
    {0x61, 0x62, 0x63, 0x48, 0},
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x63, 0x48, 0x63, 0},                              /*  34 */
    {0x61, 0x63, 0x65, 0x30},
    {0x31, 0x30},
    {0x70, 0x00EA,0x30}                                    /* 37     */
};

const UChar testTargetCases[][16] = {
    {0x61, 0x62, 0x63, 0x27, 0},
    {0x43, 0x4f, 0x4f, 0x50, 0},
    {0x61, 0x62, 0x63, 0},
    {0x26, 0},
    {0x26, 0},
    {0x34, 0},
    {0x35, 0},
    {0x6f, 0x6e, 0x65, 0},
    {0x6e, 0x6e, 0x65, 0},
    {0x70, 0x6e, 0x65, 0},                                  /*  10 */
    {0x74, 0x77, 0x6f, 0},
    {0x75, 0x77, 0x6f, 0},
    {0x68, 0x65, 0x6c, 0x6c, 0x4f, 0},
    {0x61, 0x3c, 0x3d, 0x62, 0},
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x43, 0x48, 0x63, 0},
    {0x61, 0x43, 0x48, 0x63, 0},  /*  simple test */
    {0x70, (UChar)0x00E9, 0x63, 0x68, 0x00E9, 0},
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x42, 0x43, 0},                                  /*  20 */
    {0x61, 0x62, 0x63, 0x68, 0},
    {0x61, 0x62, 0x64, 0},
    {(UChar)0x00E4, 0x62, 0x63, 0},
    {0x61, (UChar)0x00C6, 0x63, 0},
    {0x61, 0x43, 0x48, 0x63, 0},  /*  primary test */
    {0x62, 0x6c, 0x61, 0x63, 0x6b, 0x2d, 0x62, 0x69, 0x72, 0x64, 0},
    {0x34, 0},
    {0x35, 0},
    {0x6f, 0x6e, 0x65, 0},
    {0x61, 0x62, 0x63, 0},
    {0x61, 0x42, 0x63, 0},                                  /*  30 */
    {0x61, 0x62, 0x63, 0x68, 0},
    {0x61, 0x62, 0x64, 0},
    {0x61, 0x43, 0x48, 0x63, 0},                                /*  34 */
    {0x61, 0x63, 0x65, 0x30},
    {0x31, 0x30},
    {0x70, (UChar)0x00EB,0x30}                                    /* 37 */
};

const UChar testCases[][4] =
{
    {0x61, 0},
    {0x41, 0},
    {0x00e4, 0},
    {0x00c4, 0},
    {0x61, 0x65, 0},
    {0x61, 0x45, 0},
    {0x41, 0x65, 0},
    {0x41, 0x45, 0},
    {(UChar)0x00e6, 0},
    {(UChar)0x00c6, 0},
    {0x62, 0},
    {0x63, 0},
    {0x7a, 0}
};

#endif  /* #ifndef COLLATION_TEST_DATA */
