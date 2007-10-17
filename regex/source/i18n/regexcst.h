//---------------------------------------------------------------------------------
//
// Generated Header File.  Do not edit by hand.
//    This file contains the state table for the ICU Regular Expression Pattern Parser
//    It is generated by the Perl script "regexcst.pl" from
//    the rule parser state definitions file "regexcst.txt".
//
//   Copyright (C) 2002-2007 International Business Machines Corporation 
//   and others. All rights reserved.  
//
//---------------------------------------------------------------------------------
#ifndef RBBIRPT_H
#define RBBIRPT_H

U_NAMESPACE_BEGIN
//
// Character classes for regex pattern scanning.
//
    static const uint8_t kRuleSet_white_space = 128;
    static const uint8_t kRuleSet_digit_char = 129;
    static const uint8_t kRuleSet_rule_char = 130;
    static const uint8_t kRuleSet_octal_digit = 131;


enum Regex_PatternParseAction {
    doLiteralChar,
    doSetEnd,
    doBackslashA,
    doSetBeginUnion,
    doNOP,
    doSetRange,
    doOctalStart,
    doBackslashG,
    doPerlInline,
    doSetAddDash,
    doIntevalLowerDigit,
    doProperty,
    doBackslashX,
    doOpenAtomicParen,
    doPatFinish,
    doSetDifference2,
    doNGPlus,
    doOpenLookBehindNeg,
    doIntervalError,
    doIntervalSame,
    doBackRef,
    doPlus,
    doOpenCaptureParen,
    doMismatchedParenErr,
    doBeginMatchMode,
    doEscapeError,
    doOpenNonCaptureParen,
    doDollar,
    doSetProp,
    doIntervalUpperDigit,
    doSetBegin,
    doBackslashs,
    doOpenLookBehind,
    doSetMatchMode,
    doOrOperator,
    doCaret,
    doMatchModeParen,
    doStar,
    doOpt,
    doMatchMode,
    doOctalDigit,
    doSuppressComments,
    doPossessiveInterval,
    doOpenLookAheadNeg,
    doBackslashW,
    doCloseParen,
    doSetOpError,
    doIntervalInit,
    doSetFinish,
    doOctalFinish,
    doSetIntersection2,
    doNGStar,
    doEnterQuoteMode,
    doSetAddAmp,
    doBackslashB,
    doBackslashw,
    doPossessiveOpt,
    doSetNegate,
    doRuleError,
    doBackslashb,
    doConditionalExpr,
    doPossessivePlus,
    doBadOpenParenType,
    doNGInterval,
    doSetLiteral,
    doBackslashd,
    doSetBeginDifference1,
    doBackslashD,
    doExit,
    doInterval,
    doSetNoCloseError,
    doNGOpt,
    doSetPosixProp,
    doBackslashS,
    doBackslashZ,
    doSetBeginIntersection1,
    doOpenLookAhead,
    doBadModeFlag,
    doPatStart,
    doPossessiveStar,
    doBackslashz,
    doDotAny,
    rbbiLastAction};

//-------------------------------------------------------------------------------
//
//  RegexTableEl       represents the structure of a row in the transition table
//                     for the pattern parser state machine.
//-------------------------------------------------------------------------------
struct RegexTableEl {
    Regex_PatternParseAction      fAction;
    uint8_t                       fCharClass;       // 0-127:    an individual ASCII character
                                                    // 128-255:  character class index
    uint8_t                       fNextState;       // 0-250:    normal next-state numbers
                                                    // 255:      pop next-state from stack.
    uint8_t                       fPushState;
    UBool                         fNextChar;
};

static const struct RegexTableEl gRuleParseStateTable[] = {
    {doNOP, 0, 0, 0, TRUE}
    , {doPatStart, 255, 2,0,  FALSE}     //  1      start
    , {doLiteralChar, 254, 14,0,  TRUE}     //  2      term
    , {doLiteralChar, 130, 14,0,  TRUE}     //  3 
    , {doSetBegin, 91 /* [ */, 109, 165, TRUE}     //  4 
    , {doNOP, 40 /* ( */, 27,0,  TRUE}     //  5 
    , {doDotAny, 46 /* . */, 14,0,  TRUE}     //  6 
    , {doCaret, 94 /* ^ */, 14,0,  TRUE}     //  7 
    , {doDollar, 36 /* $ */, 14,0,  TRUE}     //  8 
    , {doNOP, 92 /* \ */, 81,0,  TRUE}     //  9 
    , {doOrOperator, 124 /* | */, 2,0,  TRUE}     //  10 
    , {doCloseParen, 41 /* ) */, 255,0,  TRUE}     //  11 
    , {doPatFinish, 253, 2,0,  FALSE}     //  12 
    , {doRuleError, 255, 166,0,  FALSE}     //  13 
    , {doNOP, 42 /* * */, 59,0,  TRUE}     //  14      expr-quant
    , {doNOP, 43 /* + */, 62,0,  TRUE}     //  15 
    , {doNOP, 63 /* ? */, 65,0,  TRUE}     //  16 
    , {doIntervalInit, 123 /* { */, 68,0,  TRUE}     //  17 
    , {doNOP, 40 /* ( */, 23,0,  TRUE}     //  18 
    , {doNOP, 255, 20,0,  FALSE}     //  19 
    , {doOrOperator, 124 /* | */, 2,0,  TRUE}     //  20      expr-cont
    , {doCloseParen, 41 /* ) */, 255,0,  TRUE}     //  21 
    , {doNOP, 255, 2,0,  FALSE}     //  22 
    , {doSuppressComments, 63 /* ? */, 25,0,  TRUE}     //  23      open-paren-quant
    , {doNOP, 255, 27,0,  FALSE}     //  24 
    , {doNOP, 35 /* # */, 47, 14, TRUE}     //  25      open-paren-quant2
    , {doNOP, 255, 29,0,  FALSE}     //  26 
    , {doSuppressComments, 63 /* ? */, 29,0,  TRUE}     //  27      open-paren
    , {doOpenCaptureParen, 255, 2, 14, FALSE}     //  28 
    , {doOpenNonCaptureParen, 58 /* : */, 2, 14, TRUE}     //  29      open-paren-extended
    , {doOpenAtomicParen, 62 /* > */, 2, 14, TRUE}     //  30 
    , {doOpenLookAhead, 61 /* = */, 2, 20, TRUE}     //  31 
    , {doOpenLookAheadNeg, 33 /* ! */, 2, 20, TRUE}     //  32 
    , {doNOP, 60 /* < */, 44,0,  TRUE}     //  33 
    , {doNOP, 35 /* # */, 47, 2, TRUE}     //  34 
    , {doBeginMatchMode, 105 /* i */, 50,0,  FALSE}     //  35 
    , {doBeginMatchMode, 109 /* m */, 50,0,  FALSE}     //  36 
    , {doBeginMatchMode, 115 /* s */, 50,0,  FALSE}     //  37 
    , {doBeginMatchMode, 119 /* w */, 50,0,  FALSE}     //  38 
    , {doBeginMatchMode, 120 /* x */, 50,0,  FALSE}     //  39 
    , {doBeginMatchMode, 45 /* - */, 50,0,  FALSE}     //  40 
    , {doConditionalExpr, 40 /* ( */, 166,0,  TRUE}     //  41 
    , {doPerlInline, 123 /* { */, 166,0,  TRUE}     //  42 
    , {doBadOpenParenType, 255, 166,0,  FALSE}     //  43 
    , {doOpenLookBehind, 61 /* = */, 2, 20, TRUE}     //  44      open-paren-lookbehind
    , {doOpenLookBehindNeg, 33 /* ! */, 2, 20, TRUE}     //  45 
    , {doBadOpenParenType, 255, 166,0,  FALSE}     //  46 
    , {doNOP, 41 /* ) */, 255,0,  TRUE}     //  47      paren-comment
    , {doMismatchedParenErr, 253, 166,0,  FALSE}     //  48 
    , {doNOP, 255, 47,0,  TRUE}     //  49 
    , {doMatchMode, 105 /* i */, 50,0,  TRUE}     //  50      paren-flag
    , {doMatchMode, 109 /* m */, 50,0,  TRUE}     //  51 
    , {doMatchMode, 115 /* s */, 50,0,  TRUE}     //  52 
    , {doMatchMode, 119 /* w */, 50,0,  TRUE}     //  53 
    , {doMatchMode, 120 /* x */, 50,0,  TRUE}     //  54 
    , {doMatchMode, 45 /* - */, 50,0,  TRUE}     //  55 
    , {doSetMatchMode, 41 /* ) */, 2,0,  TRUE}     //  56 
    , {doMatchModeParen, 58 /* : */, 2, 14, TRUE}     //  57 
    , {doBadModeFlag, 255, 166,0,  FALSE}     //  58 
    , {doNGStar, 63 /* ? */, 20,0,  TRUE}     //  59      quant-star
    , {doPossessiveStar, 43 /* + */, 20,0,  TRUE}     //  60 
    , {doStar, 255, 20,0,  FALSE}     //  61 
    , {doNGPlus, 63 /* ? */, 20,0,  TRUE}     //  62      quant-plus
    , {doPossessivePlus, 43 /* + */, 20,0,  TRUE}     //  63 
    , {doPlus, 255, 20,0,  FALSE}     //  64 
    , {doNGOpt, 63 /* ? */, 20,0,  TRUE}     //  65      quant-opt
    , {doPossessiveOpt, 43 /* + */, 20,0,  TRUE}     //  66 
    , {doOpt, 255, 20,0,  FALSE}     //  67 
    , {doNOP, 128, 68,0,  TRUE}     //  68      interval-open
    , {doNOP, 129, 71,0,  FALSE}     //  69 
    , {doIntervalError, 255, 166,0,  FALSE}     //  70 
    , {doIntevalLowerDigit, 129, 71,0,  TRUE}     //  71      interval-lower
    , {doNOP, 44 /* , */, 75,0,  TRUE}     //  72 
    , {doIntervalSame, 125 /* } */, 78,0,  TRUE}     //  73 
    , {doIntervalError, 255, 166,0,  FALSE}     //  74 
    , {doIntervalUpperDigit, 129, 75,0,  TRUE}     //  75      interval-upper
    , {doNOP, 125 /* } */, 78,0,  TRUE}     //  76 
    , {doIntervalError, 255, 166,0,  FALSE}     //  77 
    , {doNGInterval, 63 /* ? */, 20,0,  TRUE}     //  78      interval-type
    , {doPossessiveInterval, 43 /* + */, 20,0,  TRUE}     //  79 
    , {doInterval, 255, 20,0,  FALSE}     //  80 
    , {doBackslashA, 65 /* A */, 2,0,  TRUE}     //  81      backslash
    , {doBackslashB, 66 /* B */, 2,0,  TRUE}     //  82 
    , {doBackslashb, 98 /* b */, 2,0,  TRUE}     //  83 
    , {doBackslashd, 100 /* d */, 14,0,  TRUE}     //  84 
    , {doBackslashD, 68 /* D */, 14,0,  TRUE}     //  85 
    , {doBackslashG, 71 /* G */, 2,0,  TRUE}     //  86 
    , {doProperty, 78 /* N */, 14,0,  FALSE}     //  87 
    , {doProperty, 112 /* p */, 14,0,  FALSE}     //  88 
    , {doProperty, 80 /* P */, 14,0,  FALSE}     //  89 
    , {doEnterQuoteMode, 81 /* Q */, 2,0,  TRUE}     //  90 
    , {doBackslashS, 83 /* S */, 14,0,  TRUE}     //  91 
    , {doBackslashs, 115 /* s */, 14,0,  TRUE}     //  92 
    , {doBackslashW, 87 /* W */, 14,0,  TRUE}     //  93 
    , {doBackslashw, 119 /* w */, 14,0,  TRUE}     //  94 
    , {doBackslashX, 88 /* X */, 14,0,  TRUE}     //  95 
    , {doBackslashZ, 90 /* Z */, 2,0,  TRUE}     //  96 
    , {doBackslashz, 122 /* z */, 2,0,  TRUE}     //  97 
    , {doOctalStart, 48 /* 0 */, 102,0,  TRUE}     //  98 
    , {doBackRef, 129, 14,0,  TRUE}     //  99 
    , {doEscapeError, 253, 166,0,  FALSE}     //  100 
    , {doLiteralChar, 255, 14,0,  TRUE}     //  101 
    , {doOctalDigit, 131, 104,0,  TRUE}     //  102      octal-1
    , {doOctalFinish, 255, 14,0,  FALSE}     //  103 
    , {doOctalDigit, 131, 106,0,  TRUE}     //  104      octal-2
    , {doOctalFinish, 255, 14,0,  FALSE}     //  105 
    , {doOctalDigit, 131, 108,0,  TRUE}     //  106      octal-3
    , {doOctalFinish, 255, 14,0,  FALSE}     //  107 
    , {doOctalFinish, 255, 14,0,  FALSE}     //  108      octal-end
    , {doSetNegate, 94 /* ^ */, 112,0,  TRUE}     //  109      set-open
    , {doSetPosixProp, 58 /* : */, 114,0,  FALSE}     //  110 
    , {doNOP, 255, 112,0,  FALSE}     //  111 
    , {doSetLiteral, 93 /* ] */, 122,0,  TRUE}     //  112      set-open2
    , {doNOP, 255, 117,0,  FALSE}     //  113 
    , {doSetEnd, 93 /* ] */, 255,0,  TRUE}     //  114      set-posix
    , {doNOP, 58 /* : */, 117,0,  FALSE}     //  115 
    , {doRuleError, 255, 166,0,  FALSE}     //  116 
    , {doSetLiteral, 254, 122,0,  TRUE}     //  117      set-start
    , {doSetEnd, 93 /* ] */, 255,0,  TRUE}     //  118 
    , {doSetBeginUnion, 91 /* [ */, 109, 129, TRUE}     //  119 
    , {doNOP, 92 /* \ */, 161,0,  TRUE}     //  120 
    , {doSetLiteral, 255, 122,0,  TRUE}     //  121 
    , {doSetLiteral, 254, 122,0,  TRUE}     //  122      set-after-lit
    , {doSetEnd, 93 /* ] */, 255,0,  TRUE}     //  123 
    , {doSetBeginUnion, 91 /* [ */, 109, 129, TRUE}     //  124 
    , {doNOP, 45 /* - */, 151,0,  TRUE}     //  125 
    , {doNOP, 92 /* \ */, 161,0,  TRUE}     //  126 
    , {doSetNoCloseError, 253, 166,0,  FALSE}     //  127 
    , {doSetLiteral, 255, 122,0,  TRUE}     //  128 
    , {doSetLiteral, 254, 122,0,  TRUE}     //  129      set-after-set
    , {doSetEnd, 93 /* ] */, 255,0,  TRUE}     //  130 
    , {doSetBeginUnion, 91 /* [ */, 109, 129, TRUE}     //  131 
    , {doNOP, 45 /* - */, 144,0,  TRUE}     //  132 
    , {doNOP, 92 /* \ */, 161,0,  TRUE}     //  133 
    , {doNOP, 38 /* & */, 137,0,  TRUE}     //  134 
    , {doSetNoCloseError, 253, 166,0,  FALSE}     //  135 
    , {doSetLiteral, 255, 122,0,  TRUE}     //  136 
    , {doSetAddAmp, 254, 141,0,  FALSE}     //  137      set-amp-S-1
    , {doSetBeginIntersection1, 91 /* [ */, 109, 129, TRUE}     //  138 
    , {doSetIntersection2, 38 /* & */, 156,0,  TRUE}     //  139 
    , {doSetAddAmp, 255, 141,0,  FALSE}     //  140 
    , {doSetLiteral, 254, 122,0,  TRUE}     //  141      set-amp-lit
    , {doSetEnd, 93 /* ] */, 255,0,  TRUE}     //  142 
    , {doSetLiteral, 255, 122,0,  TRUE}     //  143 
    , {doSetAddDash, 254, 148,0,  FALSE}     //  144      set-dash-S-1
    , {doSetBeginDifference1, 91 /* [ */, 109, 129, TRUE}     //  145 
    , {doSetDifference2, 45 /* - */, 156,0,  TRUE}     //  146 
    , {doSetAddDash, 255, 148,0,  FALSE}     //  147 
    , {doSetLiteral, 254, 122,0,  TRUE}     //  148      set-dash-lit
    , {doSetEnd, 93 /* ] */, 255,0,  TRUE}     //  149 
    , {doSetLiteral, 255, 122,0,  TRUE}     //  150 
    , {doSetRange, 254, 122,0,  TRUE}     //  151      set-dash-L-1
    , {doSetDifference2, 45 /* - */, 156,0,  TRUE}     //  152 
    , {doSetLiteral, 91 /* [ */, 122,0,  FALSE}     //  153 
    , {doSetLiteral, 93 /* ] */, 122,0,  FALSE}     //  154 
    , {doSetRange, 255, 122,0,  TRUE}     //  155 
    , {doSetLiteral, 254, 122,0,  TRUE}     //  156      set-after-op
    , {doSetBeginUnion, 91 /* [ */, 109, 129, TRUE}     //  157 
    , {doSetOpError, 93 /* ] */, 166,0,  FALSE}     //  158 
    , {doNOP, 92 /* \ */, 161,0,  TRUE}     //  159 
    , {doSetLiteral, 255, 122,0,  TRUE}     //  160 
    , {doSetProp, 112 /* p */, 129,0,  FALSE}     //  161      set-escape
    , {doSetProp, 80 /* P */, 129,0,  FALSE}     //  162 
    , {doSetProp, 78 /* N */, 129,0,  FALSE}     //  163 
    , {doSetLiteral, 255, 122,0,  TRUE}     //  164 
    , {doSetFinish, 255, 14,0,  FALSE}     //  165      set-finish
    , {doExit, 255, 166,0,  TRUE}     //  166      errorDeath
 };
static const char * const RegexStateNames[] = {    0,
     "start",
     "term",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "expr-quant",
    0,
    0,
    0,
    0,
    0,
     "expr-cont",
    0,
    0,
     "open-paren-quant",
    0,
     "open-paren-quant2",
    0,
     "open-paren",
    0,
     "open-paren-extended",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "open-paren-lookbehind",
    0,
    0,
     "paren-comment",
    0,
    0,
     "paren-flag",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "quant-star",
    0,
    0,
     "quant-plus",
    0,
    0,
     "quant-opt",
    0,
    0,
     "interval-open",
    0,
    0,
     "interval-lower",
    0,
    0,
    0,
     "interval-upper",
    0,
    0,
     "interval-type",
    0,
    0,
     "backslash",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "octal-1",
    0,
     "octal-2",
    0,
     "octal-3",
    0,
     "octal-end",
     "set-open",
    0,
    0,
     "set-open2",
    0,
     "set-posix",
    0,
    0,
     "set-start",
    0,
    0,
    0,
    0,
     "set-after-lit",
    0,
    0,
    0,
    0,
    0,
    0,
     "set-after-set",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
     "set-amp-S-1",
    0,
    0,
    0,
     "set-amp-lit",
    0,
    0,
     "set-dash-S-1",
    0,
    0,
    0,
     "set-dash-lit",
    0,
    0,
     "set-dash-L-1",
    0,
    0,
    0,
    0,
     "set-after-op",
    0,
    0,
    0,
    0,
     "set-escape",
    0,
    0,
    0,
     "set-finish",
     "errorDeath",
    0};

U_NAMESPACE_END
#endif
