/*
* Copyright � {1999}, International Business Machines Corporation and others. All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/17/99    aliu        Creation.
**********************************************************************
*/
#ifndef RBT_PARS_H
#define RBT_PARS_H

#include "unicode/rbt.h"

class TransliterationRuleData;
class UnicodeSet;

class TransliterationRuleParser {

    /**
     * This is a reference to external data we don't own.  This works because
     * we only hold this for the duration of the call to parse().
     */
    const UnicodeString& rules;

    RuleBasedTransliterator::Direction direction;

    TransliterationRuleData* data;

    /**
     * We use a single error code during parsing.  Rather than pass it
     * through each API, we keep it here.
     */
    UErrorCode status;

    /**
     * The next available stand-in for variables.  This starts at some point in
     * the private use area (discovered dynamically) and increments up toward
     * <code>variableLimit</code>.  At any point during parsing, available
     * variables are <code>variableNext..variableLimit-1</code>.
     */
    UChar variableNext;

    /**
     * The last available stand-in for variables.  This is discovered
     * dynamically.  At any point during parsing, available variables are
     * <code>variableNext..variableLimit-1</code>.
     */
    UChar variableLimit;

    // Operators
    static const UChar VARIABLE_DEF_OP;
    static const UChar FORWARD_RULE_OP;
    static const UChar REVERSE_RULE_OP;
    static const UChar FWDREV_RULE_OP; // internal rep of <> op
    static const UnicodeString OPERATORS;

    // Other special characters
    static const UChar QUOTE;
    static const UChar ESCAPE;
    static const UChar END_OF_RULE;
    static const UChar RULE_COMMENT_CHAR;
    static const UChar VARIABLE_REF_OPEN;
    static const UChar VARIABLE_REF_CLOSE;
    static const UChar CONTEXT_OPEN;
    static const UChar CONTEXT_CLOSE;
    static const UChar SET_OPEN;
    static const UChar SET_CLOSE;
    static const UChar CURSOR_POS;

public:

    static TransliterationRuleData*
        parse(const UnicodeString& rules,
              RuleBasedTransliterator::Direction direction);
    
private:

    /**
     * @param rules list of rules, separated by newline characters
     * @exception IllegalArgumentException if there is a syntax error in the
     * rules
     */
    TransliterationRuleParser(const UnicodeString& rules,
                              RuleBasedTransliterator::Direction direction);

    /**
     * Parse the given string as a sequence of rules, separated by newline
     * characters ('\n'), and cause this object to implement those rules.  Any
     * previous rules are discarded.  Typically this method is called exactly
     * once, during construction.
     * @exception IllegalArgumentException if there is a syntax error in the
     * rules
     */
    void parseRules(void);

    /**
     * MAIN PARSER.  Parse the next rule in the given rule string, starting
     * at pos.  Return the index after the last character parsed.  Do not
     * parse characters at or after limit.
     *
     * Important:  The character at pos must be a non-whitespace character
     * that is not the comment character.
     *
     * This method handles quoting, escaping, and whitespace removal.  It
     * parses the end-of-rule character.  It recognizes context and cursor
     * indicators.  Once it does a lexical breakdown of the rule at pos, it
     * creates a rule object and adds it to our rule list.
     */
    int32_t parseRule(int32_t pos, int32_t limit);

    /**
     * Called by main parser upon syntax error.  Search the rule string
     * for the probable end of the rule.  Of course, if the error is that
     * the end of rule marker is missing, then the rule end will not be found.
     * In any case the rule start will be correctly reported.
     * @param msg error description
     * @param rule pattern string
     * @param start position of first character of current rule
     */
    int32_t syntaxError(const char* msg, const UnicodeString&, int32_t start);

    /**
     * Allocate a private-use substitution character for the given set,
     * register it in the setVariables hash, and return the substitution
     * character.
     */
    UChar registerSet(UnicodeSet* adoptedSet);
 
    /**
     * Determines what part of the private use region of Unicode we can use for
     * variable stand-ins.  The correct way to do this is as follows: Parse each
     * rule, and for forward and reverse rules, take the FROM expression, and
     * make a hash of all characters used.  The TO expression should be ignored.
     * When done, everything not in the hash is available for use.  In practice,
     * this method may employ some other algorithm for improved speed.
     */
    void determineVariableRange(void);

    /**
     * Returns the index of the first character in a set, ignoring quoted text.
     * For example, in the string "abc'hide'h", the 'h' in "hide" will not be
     * found by a search for "h".  Unlike String.indexOf(), this method searches
     * not for a single character, but for any character of the string
     * <code>setOfChars</code>.
     * @param text text to be searched
     * @param start the beginning index, inclusive; <code>0 <= start
     * <= limit</code>.
     * @param limit the ending index, exclusive; <code>start <= limit
     * <= text.length()</code>.
     * @param setOfChars string with one or more distinct characters
     * @return Offset of the first character in <code>setOfChars</code>
     * found, or -1 if not found.
     * @see #indexOf
     */
    static int32_t quotedIndexOf(const UnicodeString& text,
                                 int32_t start, int32_t limit,
                                 const UnicodeString& setOfChars);
};

#endif
