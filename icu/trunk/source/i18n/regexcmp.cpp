
//
//  file:  regexcmp.cpp
//
//  Copyright (C) 2002, International Business Machines Corporation and others.
//  All Rights Reserved.
//
//  This file contains the ICU regular expression scanner, which is responsible
//  for preprocessing a regular expression pattern into the tokenized form that
//  is used by the match finding engine.
//


#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/uchriter.h"
#include "unicode/parsepos.h"
#include "unicode/parseerr.h"
#include "unicode/regex.h"
#include "regeximp.h"
#include "upropset.h"
#include "cmemory.h"
#include "cstring.h"

#include "stdio.h"    // TODO:  Get rid of this

#include "regexcst.h"   // Contains state table for the regex pattern parser.
                       //   generated by a Perl script.
#include "regexcmp.h"

#include "uassert.h"


U_NAMESPACE_BEGIN

const char RegexCompile::fgClassID=0;
static const int RESCAN_DEBUG = 0;

//----------------------------------------------------------------------------------------
//
// Unicode Sets for each of the character classes needed for parsing a regex pattern.
//               (Initialized with hex values for portability to EBCDIC based machines.
//                Really ugly, but there's no good way to avoid it.)
//
//              The sets are referred to by name in the regexcst.txt, which is the
//              source form of the state transition table.  These names are converted
//              to indicies in regexcst.h by the perl state table building script.
//              The indices are used to access the array gRuleSets.
//
//----------------------------------------------------------------------------------------

// Characters that have no special meaning, and thus do not need to be escaped.  Expressed
//    as the inverse of those needing escaping --  [^\*\?\+\[\(\)\{\}\^\$\|\\\.]
static const UChar gRuleSet_rule_char_pattern[]       = { 
 //   [    ^      \     *     \     ?     \     +     \     [     \     (     /     )
    0x5b, 0x5e, 0x5c, 0x2a, 0x5c, 0x3f, 0x5c, 0x2b, 0x5c, 0x5b, 0x5c, 0x28, 0x5c, 0x29, 
 //   \     {    \     }     \     ^     \     $     \     |     \     \     \     .     ]   
    0x5c, 0x7b,0x5c, 0x7d, 0x5c, 0x5e, 0x5c, 0x24, 0x5c, 0x7c, 0x5c, 0x5c, 0x5c, 0x2e, 0x5d, 0};


static const UChar gRuleSet_digit_char_pattern[] = {
//    [    0      -    9     ]
    0x5b, 0x30, 0x2d, 0x39, 0x5d, 0};


static UnicodeSet  *gRuleSets[10];         // Array of ptrs to the actual UnicodeSet objects.
static UnicodeSet  *gUnescapeCharSet;

//
//   These are the backslash escape characters that ICU's unescape
//    will handle.
//
static const UChar gUnescapeCharPattern[] = {
//    [     a     b     c     e     f     n     r     t     u     U     ] 
    0x5b, 0x61, 0x62, 0x63, 0x65, 0x66, 0x6e, 0x72, 0x74, 0x75, 0x55, 0x5d};


//----------------------------------------------------------------------------------------
//
//  Constructor.
//
//----------------------------------------------------------------------------------------
RegexCompile::RegexCompile(UErrorCode &status) : fParenStack(status)
{
    fStatus             = &status;

    fScanIndex = 0;
    fNextIndex = 0;
    fPeekChar  = -1;
    fLineNum    = 1;
    fCharNum    = 0;
    fQuoteMode  = FALSE;
    fFreeForm   = FALSE;

    fMatchOpenParen  = -1;
    fMatchCloseParen = -1;

    if (U_FAILURE(status)) {
        return;
    }

    //
    //  Set up the constant Unicode Sets.
    //    
    if (gRuleSets[kRuleSet_rule_char-128] == NULL) {
        //  TODO:  Make thread safe.
        //  TODO:  Memory Cleanup on ICU shutdown.
        gRuleSets[kRuleSet_rule_char-128]       = new UnicodeSet(gRuleSet_rule_char_pattern,       status);
        gRuleSets[kRuleSet_white_space-128]     = new UnicodeSet(UnicodePropertySet::getRuleWhiteSpaceSet(status));
        gRuleSets[kRuleSet_digit_char-128]      = new UnicodeSet(gRuleSet_digit_char_pattern,      status);
        gUnescapeCharSet                        = new UnicodeSet(gUnescapeCharPattern,             status);
        if (U_FAILURE(status)) {
            delete gRuleSets[kRuleSet_rule_char-128];
            delete gRuleSets[kRuleSet_white_space-128];
            delete gRuleSets[kRuleSet_digit_char-128];
            delete gUnescapeCharSet;
            gRuleSets[kRuleSet_rule_char-128]   = NULL;
            gRuleSets[kRuleSet_white_space-128] = NULL;
            gRuleSets[kRuleSet_digit_char-128]  = NULL;
            gUnescapeCharSet = NULL;
            return;
        }
    }
}



//----------------------------------------------------------------------------------------
//
//  Destructor
//
//----------------------------------------------------------------------------------------
RegexCompile::~RegexCompile() {
}

//---------------------------------------------------------------------------------
//
//  Compile regex pattern.   The state machine for rules parsing is here.
//                         The state tables are hand-written in the file regexcst.txt,
//                         and converted to the form used here by a perl
//                         script regexcst.pl
//
//---------------------------------------------------------------------------------
void    RegexCompile::compile(                    
                         RegexPattern &rxp,          // User level pattern object to receive
                                                     //    the compiled pattern.
                         const UnicodeString &pat,   // Source pat to be compiled.
                         UParseError &pp,            // Error position info
                         UErrorCode &e)              // Error Code
{
    fStatus             = &e;
    fRXPat              = &rxp;
    fParseErr           = &pp;
    fStackPtr           = 0;
    fStack[fStackPtr]   = 0;

    if (U_FAILURE(*fStatus)) {
        return;
    }

    // There should be no pattern stuff in the RegexPattern object.  They can not be reused.
    U_ASSERT(fRXPat->fPattern.length() == 0);

    // Prepare the RegexPattern object to receive the compiled pattern.
    fRXPat->fPattern        = pat;


    // Initialize the pattern scanning state machine
    fPatternLength = pat.length();
    uint16_t                state = 1;
    const RegexTableEl      *tableEl;
    nextChar(fC);                        // Fetch the first char from the pattern string.

    //
    // Main loop for the regex pattern parsing state machine.
    //   Runs once per state transition.
    //   Each time through optionally performs, depending on the state table,
    //      - an advance to the the next pattern char
    //      - an action to be performed.
    //      - pushing or popping a state to/from the local state return stack.
    //   file regexcst.txt is the source for the state table.  The logic behind
    //     recongizing the pattern syntax is there, not here.
    //
    for (;;) {
        //  Bail out if anything has gone wrong.
        //  Regex pattern parsing stops on the first error encountered.
        if (U_FAILURE(*fStatus)) {
            break;
        }

        U_ASSERT(state != 0);

        // Find the state table element that matches the input char from the rule, or the
        //    class of the input character.  Start with the first table row for this
        //    state, then linearly scan forward until we find a row that matches the
        //    character.  The last row for each state always matches all characters, so
        //    the search will stop there, if not before.
        //
        tableEl = &gRuleParseStateTable[state];
        if (RESCAN_DEBUG) {
            printf("char, line, col = (\'%c\', %d, %d)    state=%s ",
                fC.fChar, fLineNum, fCharNum, RegexStateNames[state]);
        }

        for (;;) {    // loop through table rows belonging to this state, looking for one
                      //   that matches the current input char.
            if (RESCAN_DEBUG) { printf(".");}
            if (tableEl->fCharClass < 127 && fC.fQuoted == FALSE &&   tableEl->fCharClass == fC.fChar) {
                // Table row specified an individual character, not a set, and
                //   the input character is not quoted, and
                //   the input character matched it.
                break;
            }
            if (tableEl->fCharClass == 255) {
                // Table row specified default, match anything character class.
                break;
            }
            if (tableEl->fCharClass == 254 && fC.fQuoted)  {
                // Table row specified "quoted" and the char was quoted.
                break;
            }
            if (tableEl->fCharClass == 253 && fC.fChar == (UChar32)-1)  {
                // Table row specified eof and we hit eof on the input.
                break;
            }

            if (tableEl->fCharClass >= 128 && tableEl->fCharClass < 240 &&   // Table specs a char class &&
                fC.fQuoted == FALSE &&                                      //   char is not escaped &&
                fC.fChar != (UChar32)-1) {                                   //   char is not EOF
                UnicodeSet *uniset = gRuleSets[tableEl->fCharClass-128];
                if (uniset->contains(fC.fChar)) {
                    // Table row specified a character class, or set of characters,
                    //   and the current char matches it.
                    break;
                }
            }

            // No match on this row, advance to the next  row for this state,
            tableEl++;
        }
        if (RESCAN_DEBUG) { printf("\n");}

        //
        // We've found the row of the state table that matches the current input
        //   character from the rules string.
        // Perform any action specified  by this row in the state table.
        if (doParseActions((EParseAction)tableEl->fAction) == FALSE) {
            // Break out of the state machine loop if the
            //   the action signalled some kind of error, or
            //   the action was to exit, occurs on normal end-of-rules-input.
            break;
        }

        if (tableEl->fPushState != 0) {
            fStackPtr++;
            if (fStackPtr >= kStackSize) {
                error(U_BRK_INTERNAL_ERROR);
                printf("RegexCompile::parse() - state stack overflow.\n");
                fStackPtr--;
            }
            fStack[fStackPtr] = tableEl->fPushState;
        }

        if (tableEl->fNextChar) {
            nextChar(fC);
        }

        // Get the next state from the table entry, or from the
        //   state stack if the next state was specified as "pop".
        if (tableEl->fNextState != 255) {
            state = tableEl->fNextState;
        } else {
            state = fStack[fStackPtr];
            fStackPtr--;
            if (fStackPtr < 0) {
                error(U_BRK_INTERNAL_ERROR);
                printf("RegexCompile::compile() - state stack underflow.\n");
                fStackPtr++;
            }
        }

    }

}





//----------------------------------------------------------------------------------------
//
//  doParseAction        Do some action during regex pattern parsing.
//                       Called by the parse state machine.
//
//
//----------------------------------------------------------------------------------------
UBool RegexCompile::doParseActions(EParseAction action)
{
    UBool   returnVal = TRUE;

    switch ((Regex_PatternParseAction)action) {

    case doPatStart:
        // Start of pattern compiles to:
        //0   SAVE   2        Fall back to position of FAIL
        //1   jmp    3
        //2   FAIL            Stop if we ever reach here.
        //3   NOP             Dummy, so start of pattern looks the same as
        //                    the start of an ( grouping.
        //4   NOP             Resreved, will be replaced by a save if there are
        //                    OR | operators at the top level
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_STATE_SAVE, 2), *fStatus);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_JMP,  3), *fStatus);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_FAIL, 0), *fStatus);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP,  0), *fStatus);
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP,  0), *fStatus);

        fParenStack.push(-1, *fStatus);     // Begin a Paren Stack Frame
        fParenStack.push( 3, *fStatus);     // Push location of first NOP
        break;

    case doPatFinish:
        // We've scanned to the end of the pattern
        //  The end of pattern compiles to:
        //        URX_END
        //    which will top the runtime match engine.
        //  Encountering end of pattern also behaves like a close paren,
        //   and forces fixups of the State Save at the beginning of the compiled pattern
        //   and of any OR operations at the top level.
        // 
        handleCloseParen();
        
        // add the END operation to the compiled pattern.
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_END, 0), *fStatus);
        
        // Terminate the pattern compilation state machine.
        returnVal = FALSE;
        break;



    case doOrOperator:
        // Scanning a '|', as in (A|B)
        {
            // Insert a SAVE operation at the start of the pattern section preceding
            //   this OR at this level.  This SAVE will branch the match forward
            //   to the right hand side of the OR in the event that the left hand
            //   side fails to match and backtracks.  Locate the position for the
            //   save from the location on the top of the parentheses stack.
            int32_t savePosition = fParenStack.popi();
            int32_t op = fRXPat->fCompiledPat->elementAti(savePosition);
            U_ASSERT(URX_TYPE(op) == URX_NOP);  // original contents of reserved location
            op = URX_BUILD(URX_STATE_SAVE, fRXPat->fCompiledPat->size()+1);
            fRXPat->fCompiledPat->setElementAt(op, savePosition);

            // Append an JMP operation into the compiled pattern.  The operand for
            //  the OR will eventually be the location following the ')' for the
            //  group.  This will be patched in later, when the ')' is encountered.
            op = URX_BUILD(URX_JMP, 0);
            fRXPat->fCompiledPat->addElement(op, *fStatus);

            // Push the position of the newly added JMP op onto the parentheses stack.
            // This registers if for fixup when this block's close paren is encountered.
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);

            // Append a NOP to the compiled pattern.  This is the slot reserved
            //   for a SAVE in the event that there is yet another '|' following
            //   this one.
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);
        }
        break;


    case doOpenCaptureParen:
        // Open Paren.
        //   Compile to a 
        //      - NOP, which later may be replaced by a save-state if the
        //         parenthesized group gets a * quantifier, followed by
        //      - START_CAPTURE
        //      - NOP, which may later be replaced by a save-state if there
        //             is an '|' alternation within the parens.
        {
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);
            fRXPat->fNumCaptureGroups++;
            int32_t  cop = URX_BUILD(URX_START_CAPTURE, fRXPat->fNumCaptureGroups);
            fRXPat->fCompiledPat->addElement(cop, *fStatus);
            fRXPat->fCompiledPat->addElement(URX_BUILD(URX_NOP, 0), *fStatus);

            // On the Parentheses stack, start a new frame and add the postions
            //   of the two NOPs.  Depending on what follows in the pattern, the
            //   NOPs may be changed to SAVE_STATE or JMP ops, with a target
            //   address of the end of the parenthesized group.
            fParenStack.push(-2, *fStatus);           // Begin a new frame.
            fParenStack.push(fRXPat->fCompiledPat->size()-3, *fStatus);   // The first NOP
            fParenStack.push(fRXPat->fCompiledPat->size()-1, *fStatus);   // The second NOP
        }
         break;

    case doOpenNonCaptureParen:
        // Open Paren.
        break;

    case doOpenAtomicParen:
        // Open Paren.
        break;

    case doOpenLookAhead:
        // Open Paren.
        break;

    case doOpenLookAheadNeg:
        // Open Paren.
        break;

    case doOpenLookBehind:
        // Open Paren.
        break;

    case doOpenLookBehindNeg:
        // Open Paren.
        break;

    case doExprRParen:
        break;

    case doCloseParen:
        handleCloseParen();
        break;

    case doNOP:
        break;


    case doBadOpenParenType:
    case doRuleError:
        error(U_BRK_RULE_SYNTAX);
        returnVal = FALSE;
        break;


    case doPlus:
        //  Normal '+'  compiles to
        //     1.   stuff to be repeated  (already built)
        //     2.   state-save  4
        //     3.   jmp 1
        //     4.   ...
        {
            int32_t   topLoc;        // location of item #1, the start of the stuff to repeat

            if (fRXPat->fCompiledPat->size() == fMatchCloseParen)    
            {
                // The thing being repeated (item 1) is a parenthesized block.
                //   Pick up the location of the top of the block.
                topLoc = fMatchOpenParen+1;   
            } else {
                // Repeating just a single item, the last thing in the compiled patternn so far.
                topLoc = fRXPat->fCompiledPat->size()-1;
            }

            // Locate the position in the compiled pattern where the match will continue
            //   after completing the +   (4 in the comment above)
            int32_t continueLoc = fRXPat->fCompiledPat->size()+2;

            // Emit the STATE_SAVE
            int32_t saveStateOp = URX_BUILD(URX_STATE_SAVE, continueLoc);
            fRXPat->fCompiledPat->addElement(saveStateOp, *fStatus);

            // Emit the JMP
            int32_t jmpOp = URX_BUILD(URX_JMP, topLoc);
            fRXPat->fCompiledPat->addElement(jmpOp, *fStatus);
        }
        break;

    case doOpt:
        // Normal (greedy) ? quantifier.
        //  Compiles to
        //     1. state save 3
        //     2.    body of optional stuff
        //     3. ...
        // Insert the state save into the compiled pattern, and we're done.
        {
            int32_t   saveStateLoc = blockTopLoc();      
            int32_t   saveStateOp  = URX_BUILD(URX_STATE_SAVE, fRXPat->fCompiledPat->size());
            fRXPat->fCompiledPat->setElementAt(saveStateOp, saveStateLoc);
        }
        break;



    case doStar:
        // Normal (greedy) * quantifier.
        // Compiles to
        //       1.   STATE_SAVE   3
        //       2.      body of stuff being iterated over
        //       3.   JMP  0
        //       4.   ...
        //
        { 
            // location of item #1, the STATE_SAVE
            int32_t   saveStateLoc = blockTopLoc();       

            // Locate the position in the compiled pattern where the match will continue
            //   after completing the *.   (4 in the comment above)
            int32_t continueLoc = fRXPat->fCompiledPat->size()+1;

            // Put together the save state op store it into the compiled code.
            int32_t saveStateOp = URX_BUILD(URX_STATE_SAVE, continueLoc);
            fRXPat->fCompiledPat->setElementAt(saveStateOp, saveStateLoc);

            // Append the URX_JMP operation to the compiled pattern.  Its target
            // is the locaton of the state-save, above.
            int32_t jmpOp = URX_BUILD(URX_JMP, saveStateLoc);
            fRXPat->fCompiledPat->addElement(jmpOp, *fStatus);
        }
        break;


    case doStartString:
        // We've just scanned a single "normal" character from the pattern,
        // which is a character without special meaning that will need to be
        // matched literally.   Save it away.  It may be the start of a string.
        {
            fStringOpStart = fRXPat->fLiteralText.length();
            fRXPat->fLiteralText.append(fC.fChar);
            break;
        }

    case doStringChar:
        // We've just scanned a "normal" character from the pattern, which now
        //   needs to be appended the the literal match string being that is
        //   already being assembled.
        {
            fRXPat->fLiteralText.append(fC.fChar);
            break;
        }



    case doSplitString:
        // We've just peeked at a quantifier, e.g. a *, following a scanned string.
        //   Separate the last character from the string, because the quantifier
        //   only applies to it, not to the entire string.  Emit into the compiled
        //   pattern:
        //      -  string chars[0..n-2]     (as a string, assuming more than one char)
        //      -  string char [n-1]        (as a single character)
        {
            // Locate the positions of the last and next-to-last characters
            //  in the string.  Requires a bit of futzing around to account for
            //  surrogate pairs, since we want 32 bit code points, not 16 bit code units.
            int32_t  strLength = fRXPat->fLiteralText.length() - fStringOpStart;
            U_ASSERT(strLength > 0);
            int32_t  lastCharIdx = fRXPat->fLiteralText.length()-1;
            lastCharIdx = fRXPat->fLiteralText.getChar32Start(lastCharIdx);
            int32_t nextToLastCharIdx = lastCharIdx-1;
            if (nextToLastCharIdx > fStringOpStart) {
                nextToLastCharIdx = fRXPat->fLiteralText.getChar32Start(nextToLastCharIdx);
            }

            if (nextToLastCharIdx > fStringOpStart) {
                // The string contains three or more code units.
                // emit the first through the next-to-last as a string.
                int32_t  stringToken = URX_BUILD(URX_STRING, fStringOpStart);
                fRXPat->fCompiledPat->addElement(stringToken, *fStatus);
                stringToken = URX_BUILD(URX_STRING_LEN, lastCharIdx - fStringOpStart);
                fRXPat->fCompiledPat->addElement(stringToken, *fStatus);
            }
            else if (nextToLastCharIdx == fStringOpStart) {
                // The string contains exactly two code units.
                // emit the first into the compiled pattern as a single char
                UChar32  c = fRXPat->fLiteralText.char32At(nextToLastCharIdx);
                int32_t  charToken = URX_BUILD(URX_ONECHAR, c);
                fRXPat->fCompiledPat->addElement(charToken, *fStatus);
            }
            // In all cases emit the last char as a single character.
            UChar32  c = fRXPat->fLiteralText.char32At(lastCharIdx);
            int32_t  charToken = URX_BUILD(URX_ONECHAR, c);
            fRXPat->fCompiledPat->addElement(charToken, *fStatus);
        }
        break;

    case doEndString:
        // We have reached the end of a literal string in the pattern.
        // Emit the string token into the compiled pattern, or if the string
        //   has only one character, emit the single character token instead.
        {
            int32_t   strLength = fRXPat->fLiteralText.length() - fStringOpStart;
            U_ASSERT(strLength > 0);  
            int32_t  lastCharIdx = fRXPat->fLiteralText.length()-1;
            lastCharIdx = fRXPat->fLiteralText.getChar32Start(lastCharIdx);
            if (lastCharIdx == fStringOpStart) {
                // The string contains exactly one character.
                //  Emit it into the compiled pattern as a single char.
                int32_t  charToken = URX_BUILD(URX_ONECHAR, fRXPat->fLiteralText.char32At(fStringOpStart));
                fRXPat->fCompiledPat->addElement(charToken, *fStatus);
            } else {
                // The string contains two or more chars.  Emit as a string.
                // Compiled string consumes two tokens in the compiled pattern, one
                //   for the index of the start-of-string, and one for the length.
                int32_t  stringToken = URX_BUILD(URX_STRING, fStringOpStart);
                fRXPat->fCompiledPat->addElement(stringToken, *fStatus);
                stringToken = URX_BUILD(URX_STRING_LEN, strLength);
                fRXPat->fCompiledPat->addElement(stringToken, *fStatus);
            }
        }
        break;


    case doDotAny:
        // scanned a ".",  match any single character.
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_DOTANY, 0), *fStatus);
        break;


    case doBackslashA:
        // Scanned a "\A".
        fRXPat->fCompiledPat->addElement(URX_BUILD(URX_BACKSLASH_A, 0), *fStatus);
        break;

    case doExit:
        returnVal = FALSE;
        break;

    case doScanUnicodeSet:
        {
            UnicodeSet *theSet = scanSet();
            if (theSet == NULL) {
                break;
            }
            if (theSet->size() > 1) {
                //  The set contains two or more chars.
                //  Put it into the compiled pattern as a set.
                int32_t setNumber = fRXPat->fSets->size();
                fRXPat->fSets->addElement(theSet, *fStatus);
                int32_t setOp = URX_BUILD(URX_SETREF, setNumber);
                fRXPat->fCompiledPat->addElement(setOp, *fStatus);
            }
            else
            {
                // The set contains only a single code point.  Put it into
                //   the compiled pattern as a single char operation rather
                //   than a set, and discard the set itself.
                UChar32  c = theSet->charAt(0);
                if (c == -1) {
                    // Set contained no chars.  Stuff an invalid char that can't match.
                    c = 0x1fffff;
                }
                int32_t  charToken = URX_BUILD(URX_ONECHAR, c);
                fRXPat->fCompiledPat->addElement(charToken, *fStatus);
                delete theSet;
            }
        }
        break;

    default:
        error(U_BRK_INTERNAL_ERROR);
        returnVal = FALSE;
        break;
    }
    return returnVal;
};


//------------------------------------------------------------------------------
//
//   blockTopLoc()          Find or create a location in the compiled pattern
//                          at the start of the operation or block that has
//                          just been compiled.  Needed when a quantifier (* or
//                          whatever) appears, and we need to add an operation
//                          at the start of the thing being quantified.
//
//                          (Parenthesized Blocks) have a slot with a NOP that
//                          is reserved for this purpose.  .* or similar don't
//                          and a slot needs to be added.
//
//------------------------------------------------------------------------------
int32_t   RegexCompile::blockTopLoc() {
    int32_t   theLoc;
    if (fRXPat->fCompiledPat->size() == fMatchCloseParen)    
    {
        // The item just processed is a parenthesized block.
        theLoc = fMatchOpenParen;   // A slot is already reserved for us.
        U_ASSERT(theLoc > 0);
        uint32_t  opAtTheLoc = fRXPat->fCompiledPat->elementAti(theLoc);
        U_ASSERT(URX_TYPE(opAtTheLoc) == URX_NOP);
    }
    else {
        // Item just compiled is a single thing, a ".", or a single char, or a set reference.
        // No slot for STATE_SAVE was pre-reserved in the compiled code.
        // We need to make space now.
        theLoc = fRXPat->fCompiledPat->size()-1;
        int32_t opAtTheLoc = fRXPat->fCompiledPat->elementAti(theLoc);
        int32_t prevType = URX_TYPE(opAtTheLoc);
        U_ASSERT(prevType==URX_ONECHAR || prevType==URX_SETREF || prevType==URX_DOTANY);
        int32_t  nop = URX_BUILD(URX_NOP, 0);
        fRXPat->fCompiledPat->insertElementAt(nop, theLoc, *fStatus);
    }
    return theLoc;
}



//------------------------------------------------------------------------------
//
//    handleCloseParen      When compiling a close paren, we need to go back
//                          and fix up any JMP or SAVE operations within the
//                          parenthesized block that need to target the end
//                          of the block.  The locations of these are kept on
//                          the paretheses stack.
//
//                          This function is called both when encountering a
//                          real ) and at the end of the pattern.
//
//-------------------------------------------------------------------------------
void  RegexCompile::handleCloseParen() {
    int32_t   patIdx;
    int32_t   patOp;
    U_ASSERT(fParenStack.size() >= 1);
    
    // Fixup any operations within the just-closed parenthesized group
    //    that need to reference the end of the (block).
    //    (The first one on popped from the stack is an unused slot for
    //     alternation (OR) state save, but applying the fixup to it does no harm.)
    for (;;) {
        patIdx = fParenStack.popi();
        if (patIdx < 0) {
            break;
        }
        U_ASSERT(patIdx>0 && patIdx <= fRXPat->fCompiledPat->size());
        patOp = fRXPat->fCompiledPat->elementAti(patIdx);
        U_ASSERT(URX_VAL(patOp) == 0);          // Branch target for JMP should not be set.
        patOp |= fRXPat->fCompiledPat->size();  // Set it now.
        fRXPat->fCompiledPat->setElementAt(patOp, patIdx);
        fMatchOpenParen     = patIdx;
    }
    
    // DO any additional fixups, depending on the specific kind of
    // parentesized grouping this is
    
    switch (patIdx) {
    case -1:
        // No additional fixups required.
        //   This is the case with most kinds of groupings.
        break;
    case -2:
        // Capturing Parentheses.  
        //   Insert a End Capture op into the pattern.
        //   Grab the group number from the start capture op
        //      and put it into the end-capture op.
        {
            int32_t   captureOp = fRXPat->fCompiledPat->elementAti(fMatchOpenParen+1);
            U_ASSERT(URX_TYPE(captureOp) == URX_START_CAPTURE);
            int32_t   captureGroupNumber = URX_VAL(captureOp);
            U_ASSERT(captureGroupNumber > 0);
            int32_t   endCaptureOp = URX_BUILD(URX_END_CAPTURE, captureGroupNumber);
            fRXPat->fCompiledPat->addElement(endCaptureOp, *fStatus);
        }
        break;
    default:
        U_ASSERT(FALSE);
    }

    // remember the next location in the compiled pattern.
    // The compilation of Quantifiers will look at this to see whether its looping
    //   over a parenthesized block or a single item
    fMatchCloseParen = fRXPat->fCompiledPat->size();
}


//----------------------------------------------------------------------------------------
//
//  Error         Report a rule parse error.
//                Only report it if no previous error has been recorded.
//
//----------------------------------------------------------------------------------------
void RegexCompile::error(UErrorCode e) {
    if (U_SUCCESS(*fStatus)) {
        *fStatus = e;
        fParseErr->line  = fLineNum;
        fParseErr->offset = fCharNum;
        fParseErr->preContext[0] = 0;
        fParseErr->preContext[0] = 0;
    }
}









//
//  Assorted Unicode character constants.
//     Numeric because there is no portable way to enter them as literals.
//     (Think EBCDIC).
//
static const UChar      chCR        = 0x0d;      // New lines, for terminating comments.
static const UChar      chLF        = 0x0a;
static const UChar      chNEL       = 0x85;      //    NEL newline variant
static const UChar      chLS        = 0x2028;    //    Unicode Line Separator
static const UChar      chApos      = 0x27;      //  single quote, for quoted chars.
static const UChar      chPound     = 0x23;      // '#', introduces a comment.
static const UChar      chBackSlash = 0x5c;      // '\'  introduces a char escape
static const UChar      chLParen    = 0x28;
static const UChar      chRParen    = 0x29;


//----------------------------------------------------------------------------------------
//
//  nextCharLL    Low Level Next Char from the regex pattern.
//                Get a char from the string,
//                keep track of input position for error reporting.
//
//----------------------------------------------------------------------------------------
UChar32  RegexCompile::nextCharLL() {
    UChar32       ch;
    UnicodeString &pattern = fRXPat->fPattern;

    if (fPeekChar != -1) {
        ch = fPeekChar;
        fPeekChar = -1;
        return ch;
    }
    if (fPatternLength==0 || fNextIndex >= fPatternLength) {
        return (UChar32)-1;
    }
    ch         = pattern.char32At(fNextIndex);
    fNextIndex = pattern.moveIndex32(fNextIndex, 1);

    if (ch == chCR ||
        ch == chNEL ||
        ch == chLS   ||
        ch == chLF && fLastChar != chCR) {
        // Character is starting a new line.  Bump up the line number, and
        //  reset the column to 0.
        fLineNum++;
        fCharNum=0;
        if (fQuoteMode) {
            error(U_BRK_NEW_LINE_IN_QUOTED_STRING);
            fQuoteMode = FALSE;
        }
    }
    else {
        // Character is not starting a new line.  Except in the case of a
        //   LF following a CR, increment the column position.
        if (ch != chLF) {
            fCharNum++;
        }
    }
    fLastChar = ch;
    return ch;
}

//---------------------------------------------------------------------------------
//
//   peekCharLL    Low Level Character Scanning, sneak a peek at the next
//                 character without actually getting it.
//
//---------------------------------------------------------------------------------
UChar32  RegexCompile::peekCharLL() {
    if (fPeekChar == -1) {
        fPeekChar = nextCharLL();
    }
    return fPeekChar;
}


//---------------------------------------------------------------------------------
//
//   nextChar     for pattern scanning.  At this level, we handle stripping
//                out comments and processing some backslash character escapes.
//                The rest of the pattern grammar is handled at the next level up.
//
//---------------------------------------------------------------------------------
void RegexCompile::nextChar(RegexPatternChar &c) {

    // Unicode Character constants needed for the processing done by nextChar(),
    //   in hex because literals wont work on EBCDIC machines.

    fScanIndex = fNextIndex;
    c.fChar    = nextCharLL();
    c.fQuoted  = FALSE;

    if (fQuoteMode) {
        c.fQuoted = TRUE;
    }
    else
    {
        // We are not in a 'quoted region' of the source.
        //
        if (fFreeForm && c.fChar == chPound) {
            // Start of a comment.  Consume the rest of it.
            //  The new-line char that terminates the comment is always returned.
            //  It will be treated as white-space, and serves to break up anything
            //    that might otherwise incorrectly clump together with a comment in
            //    the middle (a variable name, for example.)
            for (;;) {
                c.fChar = nextCharLL();
                if (c.fChar == (UChar32)-1 ||  // EOF
                    c.fChar == chCR     ||
                    c.fChar == chLF     ||
                    c.fChar == chNEL    ||
                    c.fChar == chLS)       {break;}
            }
        }
        if (c.fChar == (UChar32)-1) {
            return;
        }

        //
        //  check for backslash escaped characters.
        //  Use UnicodeString::unescapeAt() to handle those that it can.
        //  Otherwise just return the '\', and let the pattern parser deal with it.
        //
        int32_t startX = fNextIndex;  // start and end positions of the 
        int32_t endX   = fNextIndex;  //   sequence following the '\'
        if (c.fChar == chBackSlash) {
            if (gUnescapeCharSet->contains(peekCharLL())) {
                nextCharLL();     // get & discard the peeked char.
                c.fQuoted = TRUE;
                c.fChar = fRXPat->fPattern.unescapeAt(endX);
                if (startX == endX) {
                    error(U_REGEX_BAD_ESCAPE_SEQUENCE);
                }
                fCharNum += endX - startX;
                fNextIndex = endX;
            }
        }
    }
    // putc(c.fChar, stdout);
}



//---------------------------------------------------------------------------------
//
//  scanSet    Construct a UnicodeSet from the text at the current scan
//             position.  Advance the scan position to the first character
//             after the set.
//
//             The scan position is normally under the control of the state machine
//             that controls pattern parsing.  UnicodeSets, however, are parsed by
//             the UnicodeSet constructor, not by the Regex pattern parser.  
//
//---------------------------------------------------------------------------------
UnicodeSet *RegexCompile::scanSet() {
    UnicodeSet    *uset = NULL;
    ParsePosition  pos;
    int            startPos;
    int            i;

    if (U_FAILURE(*fStatus)) {
        return NULL;
    }

    pos.setIndex(fScanIndex);
    startPos = fScanIndex;
    UErrorCode localStatus = U_ZERO_ERROR;
    uset = new UnicodeSet(fRXPat->fPattern, pos,
                         localStatus);
    if (U_FAILURE(localStatus)) {
        //  TODO:  Get more accurate position of the error from UnicodeSet's return info.
        //         UnicodeSet appears to not be reporting correctly at this time.
        printf("UnicodeSet parse postion.ErrorIndex = %d\n", pos.getIndex());
        error(localStatus);
        delete uset;
        return NULL;
    }

    // Advance the current scan postion over the UnicodeSet.
    //   Don't just set fScanIndex because the line/char positions maintained
    //   for error reporting would be thrown off.
    i = pos.getIndex();
    for (;;) {
        if (fNextIndex >= i) {
            break;
        }
        nextCharLL();
    }

    return uset;
};


U_NAMESPACE_END

