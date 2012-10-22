/*
*******************************************************************************
* Copyright (C) 1997-2012, International Business Machines Corporation and others.
* All Rights Reserved.
*******************************************************************************
*/

#ifndef RBNF_H
#define RBNF_H

#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: Rule Based Number Format
 */

/**
 * \def U_HAVE_RBNF
 * This will be 0 if RBNF support is not included in ICU
 * and 1 if it is.
 *
 * @stable ICU 2.4
 */
#if UCONFIG_NO_FORMATTING
#define U_HAVE_RBNF 0
#else
#define U_HAVE_RBNF 1

#include "unicode/coll.h"
#include "unicode/dcfmtsym.h"
#include "unicode/fmtable.h"
#include "unicode/locid.h"
#include "unicode/numfmt.h"
#include "unicode/unistr.h"
#include "unicode/strenum.h"

U_NAMESPACE_BEGIN

class NFRuleSet;
class LocalizationInfo;

/**
 * Tags for the predefined rulesets.
 *
 * @stable ICU 2.2
 */
enum URBNFRuleSetTag {
    URBNF_SPELLOUT,
    URBNF_ORDINAL,
    URBNF_DURATION,
    URBNF_NUMBERING_SYSTEM,
    URBNF_COUNT
};

#if UCONFIG_NO_COLLATION
class Collator;
#endif

/**
 * The RuleBasedNumberFormat class formats numbers according to a set of rules. This number formatter is
 * typically used for spelling out numeric values in words (e.g., 25,3476 as
 * &quot;twenty-five thousand three hundred seventy-six&quot; or &quot;vingt-cinq mille trois
 * cents soixante-seize&quot; or
 * &quot;f&uuml;nfundzwanzigtausenddreihundertsechsundsiebzig&quot;), but can also be used for
 * other complicated formatting tasks, such as formatting a number of seconds as hours,
 * minutes and seconds (e.g., 3,730 as &quot;1:02:10&quot;).
 *
 * <p>The resources contain three predefined formatters for each locale: spellout, which
 * spells out a value in words (123 is &quot;one hundred twenty-three&quot;); ordinal, which
 * appends an ordinal suffix to the end of a numeral (123 is &quot;123rd&quot;); and
 * duration, which shows a duration in seconds as hours, minutes, and seconds (123 is
 * &quot;2:03&quot;).&nbsp; The client can also define more specialized <tt>RuleBasedNumberFormat</tt>s
 * by supplying programmer-defined rule sets.</p>
 *
 * <p>The behavior of a <tt>RuleBasedNumberFormat</tt> is specified by a textual description
 * that is either passed to the constructor as a <tt>String</tt> or loaded from a resource
 * bundle. In its simplest form, the description consists of a semicolon-delimited list of <em>rules.</em>
 * Each rule has a string of output text and a value or range of values it is applicable to.
 * In a typical spellout rule set, the first twenty rules are the words for the numbers from
 * 0 to 19:</p>
 *
 * <pre>zero; one; two; three; four; five; six; seven; eight; nine;
 * ten; eleven; twelve; thirteen; fourteen; fifteen; sixteen; seventeen; eighteen; nineteen;</pre>
 *
 * <p>For larger numbers, we can use the preceding set of rules to format the ones place, and
 * we only have to supply the words for the multiples of 10:</p>
 *
 * <pre> 20: twenty[-&gt;&gt;];
 * 30: thirty[-&gt;&gt;];
 * 40: forty[-&gt;&gt;];
 * 50: fifty[-&gt;&gt;];
 * 60: sixty[-&gt;&gt;];
 * 70: seventy[-&gt;&gt;];
 * 80: eighty[-&gt;&gt;];
 * 90: ninety[-&gt;&gt;];</pre>
 *
 * <p>In these rules, the <em>base value</em> is spelled out explicitly and set off from the
 * rule's output text with a colon. The rules are in a sorted list, and a rule is applicable
 * to all numbers from its own base value to one less than the next rule's base value. The
 * &quot;&gt;&gt;&quot; token is called a <em>substitution</em> and tells the fomatter to
 * isolate the number's ones digit, format it using this same set of rules, and place the
 * result at the position of the &quot;&gt;&gt;&quot; token. Text in brackets is omitted if
 * the number being formatted is an even multiple of 10 (the hyphen is a literal hyphen; 24
 * is &quot;twenty-four,&quot; not &quot;twenty four&quot;).</p>
 *
 * <p>For even larger numbers, we can actually look up several parts of the number in the
 * list:</p>
 *
 * <pre>100: &lt;&lt; hundred[ &gt;&gt;];</pre>
 *
 * <p>The &quot;&lt;&lt;&quot; represents a new kind of substitution. The &lt;&lt; isolates
 * the hundreds digit (and any digits to its left), formats it using this same rule set, and
 * places the result where the &quot;&lt;&lt;&quot; was. Notice also that the meaning of
 * &gt;&gt; has changed: it now refers to both the tens and the ones digits. The meaning of
 * both substitutions depends on the rule's base value. The base value determines the rule's <em>divisor,</em>
 * which is the highest power of 10 that is less than or equal to the base value (the user
 * can change this). To fill in the substitutions, the formatter divides the number being
 * formatted by the divisor. The integral quotient is used to fill in the &lt;&lt;
 * substitution, and the remainder is used to fill in the &gt;&gt; substitution. The meaning
 * of the brackets changes similarly: text in brackets is omitted if the value being
 * formatted is an even multiple of the rule's divisor. The rules are applied recursively, so
 * if a substitution is filled in with text that includes another substitution, that
 * substitution is also filled in.</p>
 *
 * <p>This rule covers values up to 999, at which point we add another rule:</p>
 *
 * <pre>1000: &lt;&lt; thousand[ &gt;&gt;];</pre>
 *
 * <p>Again, the meanings of the brackets and substitution tokens shift because the rule's
 * base value is a higher power of 10, changing the rule's divisor. This rule can actually be
 * used all the way up to 999,999. This allows us to finish out the rules as follows:</p>
 *
 * <pre> 1,000,000: &lt;&lt; million[ &gt;&gt;];
 * 1,000,000,000: &lt;&lt; billion[ &gt;&gt;];
 * 1,000,000,000,000: &lt;&lt; trillion[ &gt;&gt;];
 * 1,000,000,000,000,000: OUT OF RANGE!;</pre>
 *
 * <p>Commas, periods, and spaces can be used in the base values to improve legibility and
 * are ignored by the rule parser. The last rule in the list is customarily treated as an
 * &quot;overflow rule,&quot; applying to everything from its base value on up, and often (as
 * in this example) being used to print out an error message or default representation.
 * Notice also that the size of the major groupings in large numbers is controlled by the
 * spacing of the rules: because in English we group numbers by thousand, the higher rules
 * are separated from each other by a factor of 1,000.</p>
 *
 * <p>To see how these rules actually work in practice, consider the following example:
 * Formatting 25,430 with this rule set would work like this:</p>
 *
 * <table border="0" width="100%">
 *   <tr>
 *     <td><strong>&lt;&lt; thousand &gt;&gt;</strong></td>
 *     <td>[the rule whose base value is 1,000 is applicable to 25,340]</td>
 *   </tr>
 *   <tr>
 *     <td><strong>twenty-&gt;&gt;</strong> thousand &gt;&gt;</td>
 *     <td>[25,340 over 1,000 is 25. The rule for 20 applies.]</td>
 *   </tr>
 *   <tr>
 *     <td>twenty-<strong>five</strong> thousand &gt;&gt;</td>
 *     <td>[25 mod 10 is 5. The rule for 5 is &quot;five.&quot;</td>
 *   </tr>
 *   <tr>
 *     <td>twenty-five thousand <strong>&lt;&lt; hundred &gt;&gt;</strong></td>
 *     <td>[25,340 mod 1,000 is 340. The rule for 100 applies.]</td>
 *   </tr>
 *   <tr>
 *     <td>twenty-five thousand <strong>three</strong> hundred &gt;&gt;</td>
 *     <td>[340 over 100 is 3. The rule for 3 is &quot;three.&quot;]</td>
 *   </tr>
 *   <tr>
 *     <td>twenty-five thousand three hundred <strong>forty</strong></td>
 *     <td>[340 mod 100 is 40. The rule for 40 applies. Since 40 divides
 *     evenly by 10, the hyphen and substitution in the brackets are omitted.]</td>
 *   </tr>
 * </table>
 *
 * <p>The above syntax suffices only to format positive integers. To format negative numbers,
 * we add a special rule:</p>
 *
 * <pre>-x: minus &gt;&gt;;</pre>
 *
 * <p>This is called a <em>negative-number rule,</em> and is identified by &quot;-x&quot;
 * where the base value would be. This rule is used to format all negative numbers. the
 * &gt;&gt; token here means &quot;find the number's absolute value, format it with these
 * rules, and put the result here.&quot;</p>
 *
 * <p>We also add a special rule called a <em>fraction rule </em>for numbers with fractional
 * parts:</p>
 *
 * <pre>x.x: &lt;&lt; point &gt;&gt;;</pre>
 *
 * <p>This rule is used for all positive non-integers (negative non-integers pass through the
 * negative-number rule first and then through this rule). Here, the &lt;&lt; token refers to
 * the number's integral part, and the &gt;&gt; to the number's fractional part. The
 * fractional part is formatted as a series of single-digit numbers (e.g., 123.456 would be
 * formatted as &quot;one hundred twenty-three point four five six&quot;).</p>
 *
 * <p>To see how this rule syntax is applied to various languages, examine the resource data.</p>
 *
 * <p>There is actually much more flexibility built into the rule language than the
 * description above shows. A formatter may own multiple rule sets, which can be selected by
 * the caller, and which can use each other to fill in their substitutions. Substitutions can
 * also be filled in with digits, using a DecimalFormat object. There is syntax that can be
 * used to alter a rule's divisor in various ways. And there is provision for much more
 * flexible fraction handling. A complete description of the rule syntax follows:</p>
 *
 * <hr>
 *
 * <p>The description of a <tt>RuleBasedNumberFormat</tt>'s behavior consists of one or more <em>rule
 * sets.</em> Each rule set consists of a name, a colon, and a list of <em>rules.</em> A rule
 * set name must begin with a % sign. Rule sets with names that begin with a single % sign
 * are <em>public:</em> the caller can specify that they be used to format and parse numbers.
 * Rule sets with names that begin with %% are <em>private:</em> they exist only for the use
 * of other rule sets. If a formatter only has one rule set, the name may be omitted.</p>
 *
 * <p>The user can also specify a special &quot;rule set&quot; named <tt>%%lenient-parse</tt>.
 * The body of <tt>%%lenient-parse</tt> isn't a set of number-formatting rules, but a <tt>RuleBasedCollator</tt>
 * description which is used to define equivalences for lenient parsing. For more information
 * on the syntax, see <tt>RuleBasedCollator</tt>. For more information on lenient parsing,
 * see <tt>setLenientParse()</tt>.  <em>Note:</em> symbols that have syntactic meaning
 * in collation rules, such as '&amp;', have no particular meaning when appearing outside
 * of the <tt>lenient-parse</tt> rule set.</p>
 *
 * <p>The body of a rule set consists of an ordered, semicolon-delimited list of <em>rules.</em>
 * Internally, every rule has a base value, a divisor, rule text, and zero, one, or two <em>substitutions.</em>
 * These parameters are controlled by the description syntax, which consists of a <em>rule
 * descriptor,</em> a colon, and a <em>rule body.</em></p>
 *
 * <p>A rule descriptor can take one of the following forms (text in <em>italics</em> is the
 * name of a token):</p>
 *
 * <table border="0" width="100%">
 *   <tr>
 *     <td><em>bv</em>:</td>
 *     <td><em>bv</em> specifies the rule's base value. <em>bv</em> is a decimal
 *     number expressed using ASCII digits. <em>bv</em> may contain spaces, period, and commas,
 *     which are ignored. The rule's divisor is the highest power of 10 less than or equal to
 *     the base value.</td>
 *   </tr>
 *   <tr>
 *     <td><em>bv</em>/<em>rad</em>:</td>
 *     <td><em>bv</em> specifies the rule's base value. The rule's divisor is the
 *     highest power of <em>rad</em> less than or equal to the base value.</td>
 *   </tr>
 *   <tr>
 *     <td><em>bv</em>&gt;:</td>
 *     <td><em>bv</em> specifies the rule's base value. To calculate the divisor,
 *     let the radix be 10, and the exponent be the highest exponent of the radix that yields a
 *     result less than or equal to the base value. Every &gt; character after the base value
 *     decreases the exponent by 1. If the exponent is positive or 0, the divisor is the radix
 *     raised to the power of the exponent; otherwise, the divisor is 1.</td>
 *   </tr>
 *   <tr>
 *     <td><em>bv</em>/<em>rad</em>&gt;:</td>
 *     <td><em>bv</em> specifies the rule's base value. To calculate the divisor,
 *     let the radix be <em>rad</em>, and the exponent be the highest exponent of the radix that
 *     yields a result less than or equal to the base value. Every &gt; character after the radix
 *     decreases the exponent by 1. If the exponent is positive or 0, the divisor is the radix
 *     raised to the power of the exponent; otherwise, the divisor is 1.</td>
 *   </tr>
 *   <tr>
 *     <td>-x:</td>
 *     <td>The rule is a negative-number rule.</td>
 *   </tr>
 *   <tr>
 *     <td>x.x:</td>
 *     <td>The rule is an <em>improper fraction rule.</em></td>
 *   </tr>
 *   <tr>
 *     <td>0.x:</td>
 *     <td>The rule is a <em>proper fraction rule.</em></td>
 *   </tr>
 *   <tr>
 *     <td>x.0:</td>
 *     <td>The rule is a <em>master rule.</em></td>
 *   </tr>
 *   <tr>
 *     <td><em>nothing</em></td>
 *     <td>If the rule's rule descriptor is left out, the base value is one plus the
 *     preceding rule's base value (or zero if this is the first rule in the list) in a normal
 *     rule set.&nbsp; In a fraction rule set, the base value is the same as the preceding rule's
 *     base value.</td>
 *   </tr>
 * </table>
 *
 * <p>A rule set may be either a regular rule set or a <em>fraction rule set,</em> depending
 * on whether it is used to format a number's integral part (or the whole number) or a
 * number's fractional part. Using a rule set to format a rule's fractional part makes it a
 * fraction rule set.</p>
 *
 * <p>Which rule is used to format a number is defined according to one of the following
 * algorithms: If the rule set is a regular rule set, do the following:
 *
 * <ul>
 *   <li>If the rule set includes a master rule (and the number was passed in as a <tt>double</tt>),
 *     use the master rule.&nbsp; (If the number being formatted was passed in as a <tt>long</tt>,
 *     the master rule is ignored.)</li>
 *   <li>If the number is negative, use the negative-number rule.</li>
 *   <li>If the number has a fractional part and is greater than 1, use the improper fraction
 *     rule.</li>
 *   <li>If the number has a fractional part and is between 0 and 1, use the proper fraction
 *     rule.</li>
 *   <li>Binary-search the rule list for the rule with the highest base value less than or equal
 *     to the number. If that rule has two substitutions, its base value is not an even multiple
 *     of its divisor, and the number <em>is</em> an even multiple of the rule's divisor, use the
 *     rule that precedes it in the rule list. Otherwise, use the rule itself.</li>
 * </ul>
 *
 * <p>If the rule set is a fraction rule set, do the following:
 *
 * <ul>
 *   <li>Ignore negative-number and fraction rules.</li>
 *   <li>For each rule in the list, multiply the number being formatted (which will always be
 *     between 0 and 1) by the rule's base value. Keep track of the distance between the result
 *     the nearest integer.</li>
 *   <li>Use the rule that produced the result closest to zero in the above calculation. In the
 *     event of a tie or a direct hit, use the first matching rule encountered. (The idea here is
 *     to try each rule's base value as a possible denominator of a fraction. Whichever
 *     denominator produces the fraction closest in value to the number being formatted wins.) If
 *     the rule following the matching rule has the same base value, use it if the numerator of
 *     the fraction is anything other than 1; if the numerator is 1, use the original matching
 *     rule. (This is to allow singular and plural forms of the rule text without a lot of extra
 *     hassle.)</li>
 * </ul>
 *
 * <p>A rule's body consists of a string of characters terminated by a semicolon. The rule
 * may include zero, one, or two <em>substitution tokens,</em> and a range of text in
 * brackets. The brackets denote optional text (and may also include one or both
 * substitutions). The exact meanings of the substitution tokens, and under what conditions
 * optional text is omitted, depend on the syntax of the substitution token and the context.
 * The rest of the text in a rule body is literal text that is output when the rule matches
 * the number being formatted.</p>
 *
 * <p>A substitution token begins and ends with a <em>token character.</em> The token
 * character and the context together specify a mathematical operation to be performed on the
 * number being formatted. An optional <em>substitution descriptor </em>specifies how the
 * value resulting from that operation is used to fill in the substitution. The position of
 * the substitution token in the rule body specifies the location of the resultant text in
 * the original rule text.</p>
 *
 * <p>The meanings of the substitution token characters are as follows:</p>
 *
 * <table border="0" width="100%">
 *   <tr>
 *     <td>&gt;&gt;</td>
 *     <td>in normal rule</td>
 *     <td>Divide the number by the rule's divisor and format the remainder</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in negative-number rule</td>
 *     <td>Find the absolute value of the number and format the result</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in fraction or master rule</td>
 *     <td>Isolate the number's fractional part and format it.</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in rule in fraction rule set</td>
 *     <td>Not allowed.</td>
 *   </tr>
 *   <tr>
 *     <td>&gt;&gt;&gt;</td>
 *     <td>in normal rule</td>
 *     <td>Divide the number by the rule's divisor and format the remainder,
 *       but bypass the normal rule-selection process and just use the
 *       rule that precedes this one in this rule list.</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in all other rules</td>
 *     <td>Not allowed.</td>
 *   </tr>
 *   <tr>
 *     <td>&lt;&lt;</td>
 *     <td>in normal rule</td>
 *     <td>Divide the number by the rule's divisor and format the quotient</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in negative-number rule</td>
 *     <td>Not allowed.</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in fraction or master rule</td>
 *     <td>Isolate the number's integral part and format it.</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in rule in fraction rule set</td>
 *     <td>Multiply the number by the rule's base value and format the result.</td>
 *   </tr>
 *   <tr>
 *     <td>==</td>
 *     <td>in all rule sets</td>
 *     <td>Format the number unchanged</td>
 *   </tr>
 *   <tr>
 *     <td>[]</td>
 *     <td>in normal rule</td>
 *     <td>Omit the optional text if the number is an even multiple of the rule's divisor</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in negative-number rule</td>
 *     <td>Not allowed.</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in improper-fraction rule</td>
 *     <td>Omit the optional text if the number is between 0 and 1 (same as specifying both an
 *     x.x rule and a 0.x rule)</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in master rule</td>
 *     <td>Omit the optional text if the number is an integer (same as specifying both an x.x
 *     rule and an x.0 rule)</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in proper-fraction rule</td>
 *     <td>Not allowed.</td>
 *   </tr>
 *   <tr>
 *     <td></td>
 *     <td>in rule in fraction rule set</td>
 *     <td>Omit the optional text if multiplying the number by the rule's base value yields 1.</td>
 *   </tr>
 * </table>
 *
 * <p>The substitution descriptor (i.e., the text between the token characters) may take one
 * of three forms:</p>
 *
 * <table border="0" width="100%">
 *   <tr>
 *     <td>a rule set name</td>
 *     <td>Perform the mathematical operation on the number, and format the result using the
 *     named rule set.</td>
 *   </tr>
 *   <tr>
 *     <td>a DecimalFormat pattern</td>
 *     <td>Perform the mathematical operation on the number, and format the result using a
 *     DecimalFormat with the specified pattern.&nbsp; The pattern must begin with 0 or #.</td>
 *   </tr>
 *   <tr>
 *     <td>nothing</td>
 *     <td>Perform the mathematical operation on the number, and format the result using the rule
 *     set containing the current rule, except:
 *     <ul>
 *       <li>You can't have an empty substitution descriptor with a == substitution.</li>
 *       <li>If you omit the substitution descriptor in a &gt;&gt; substitution in a fraction rule,
 *         format the result one digit at a time using the rule set containing the current rule.</li>
 *       <li>If you omit the substitution descriptor in a &lt;&lt; substitution in a rule in a
 *         fraction rule set, format the result using the default rule set for this formatter.</li>
 *     </ul>
 *     </td>
 *   </tr>
 * </table>
 *
 * <p>Whitespace is ignored between a rule set name and a rule set body, between a rule
 * descriptor and a rule body, or between rules. If a rule body begins with an apostrophe,
 * the apostrophe is ignored, but all text after it becomes significant (this is how you can
 * have a rule's rule text begin with whitespace). There is no escape function: the semicolon
 * is not allowed in rule set names or in rule text, and the colon is not allowed in rule set
 * names. The characters beginning a substitution token are always treated as the beginning
 * of a substitution token.</p>
 *
 * <p>See the resource data and the demo program for annotated examples of real rule sets
 * using these features.</p>
 *
 * <p><em>User subclasses are not supported.</em> While clients may write
 * subclasses, such code will not necessarily work and will not be
 * guaranteed to work stably from release to release.
 *
 * <p><b>Localizations</b></p>
 * <p>Constructors are available that allow the specification of localizations for the
 * public rule sets (and also allow more control over what public rule sets are available).
 * Localization data is represented as a textual description.  The description represents
 * an array of arrays of string.  The first element is an array of the public rule set names,
 * each of these must be one of the public rule set names that appear in the rules.  Only
 * names in this array will be treated as public rule set names by the API.  Each subsequent
 * element is an array of localizations of these names.  The first element of one of these
 * subarrays is the locale name, and the remaining elements are localizations of the
 * public rule set names, in the same order as they were listed in the first arrray.</p>
 * <p>In the syntax, angle brackets '<', '>' are used to delimit the arrays, and comma ',' is used
 * to separate elements of an array.  Whitespace is ignored, unless quoted.</p>
 * <p>For example:<pre>
 * < < %foo, %bar, %baz >,
 *   < en, Foo, Bar, Baz >,
 *   < fr, 'le Foo', 'le Bar', 'le Baz' >
 *   < zh, \\u7532, \\u4e59, \\u4e19 > >
 * </pre></p>
 * @author Richard Gillam
 * @see NumberFormat
 * @see DecimalFormat
 * @stable ICU 2.0
 */
class U_I18N_API RuleBasedNumberFormat : public NumberFormat {
public:

  //-----------------------------------------------------------------------
  // constructors
  //-----------------------------------------------------------------------

    /**
     * Creates a RuleBasedNumberFormat that behaves according to the description
     * passed in.  The formatter uses the default locale.
     * @param rules A description of the formatter's desired behavior.
     * See the class documentation for a complete explanation of the description
     * syntax.
     * @param perror The parse error if an error was encountered.
     * @param status The status indicating whether the constructor succeeded.
     * @stable ICU 3.2
     */
    RuleBasedNumberFormat(const UnicodeString& rules, UParseError& perror, UErrorCode& status);

    /**
     * Creates a RuleBasedNumberFormat that behaves according to the description
     * passed in.  The formatter uses the default locale.
     * <p>
     * The localizations data provides information about the public
     * rule sets and their localized display names for different
     * locales. The first element in the list is an array of the names
     * of the public rule sets.  The first element in this array is
     * the initial default ruleset.  The remaining elements in the
     * list are arrays of localizations of the names of the public
     * rule sets.  Each of these is one longer than the initial array,
     * with the first String being the ULocale ID, and the remaining
     * Strings being the localizations of the rule set names, in the
     * same order as the initial array.  Arrays are NULL-terminated.
     * @param rules A description of the formatter's desired behavior.
     * See the class documentation for a complete explanation of the description
     * syntax.
     * @param localizations the localization information.
     * names in the description.  These will be copied by the constructor.
     * @param perror The parse error if an error was encountered.
     * @param status The status indicating whether the constructor succeeded.
     * @stable ICU 3.2
     */
    RuleBasedNumberFormat(const UnicodeString& rules, const UnicodeString& localizations,
                        UParseError& perror, UErrorCode& status);

  /**
   * Creates a RuleBasedNumberFormat that behaves according to the rules
   * passed in.  The formatter uses the specified locale to determine the
   * characters to use when formatting numerals, and to define equivalences
   * for lenient parsing.
   * @param rules The formatter rules.
   * See the class documentation for a complete explanation of the rule
   * syntax.
   * @param locale A locale that governs which characters are used for
   * formatting values in numerals and which characters are equivalent in
   * lenient parsing.
   * @param perror The parse error if an error was encountered.
   * @param status The status indicating whether the constructor succeeded.
   * @stable ICU 2.0
   */
  RuleBasedNumberFormat(const UnicodeString& rules, const Locale& locale,
                        UParseError& perror, UErrorCode& status);

    /**
     * Creates a RuleBasedNumberFormat that behaves according to the description
     * passed in.  The formatter uses the default locale.
     * <p>
     * The localizations data provides information about the public
     * rule sets and their localized display names for different
     * locales. The first element in the list is an array of the names
     * of the public rule sets.  The first element in this array is
     * the initial default ruleset.  The remaining elements in the
     * list are arrays of localizations of the names of the public
     * rule sets.  Each of these is one longer than the initial array,
     * with the first String being the ULocale ID, and the remaining
     * Strings being the localizations of the rule set names, in the
     * same order as the initial array.  Arrays are NULL-terminated.
     * @param rules A description of the formatter's desired behavior.
     * See the class documentation for a complete explanation of the description
     * syntax.
     * @param localizations a list of localizations for the rule set
     * names in the description.  These will be copied by the constructor.
     * @param locale A locale that governs which characters are used for
     * formatting values in numerals and which characters are equivalent in
     * lenient parsing.
     * @param perror The parse error if an error was encountered.
     * @param status The status indicating whether the constructor succeeded.
     * @stable ICU 3.2
     */
    RuleBasedNumberFormat(const UnicodeString& rules, const UnicodeString& localizations,
                        const Locale& locale, UParseError& perror, UErrorCode& status);

  /**
   * Creates a RuleBasedNumberFormat from a predefined ruleset.  The selector
   * code choosed among three possible predefined formats: spellout, ordinal,
   * and duration.
   * @param tag A selector code specifying which kind of formatter to create for that
   * locale.  There are four legal values: URBNF_SPELLOUT, which creates a formatter that
   * spells out a value in words in the desired language, URBNF_ORDINAL, which attaches
   * an ordinal suffix from the desired language to the end of a number (e.g. "123rd"),
   * URBNF_DURATION, which formats a duration in seconds as hours, minutes, and seconds,
   * and URBNF_NUMBERING_SYSTEM, which is used to invoke rules for alternate numbering
   * systems such as the Hebrew numbering system, or for Roman Numerals, etc.
   * @param locale The locale for the formatter.
   * @param status The status indicating whether the constructor succeeded.
   * @stable ICU 2.0
   */
  RuleBasedNumberFormat(URBNFRuleSetTag tag, const Locale& locale, UErrorCode& status);

  //-----------------------------------------------------------------------
  // boilerplate
  //-----------------------------------------------------------------------

  /**
   * Copy constructor
   * @param rhs    the object to be copied from.
   * @stable ICU 2.6
   */
  RuleBasedNumberFormat(const RuleBasedNumberFormat& rhs);

  /**
   * Assignment operator
   * @param rhs    the object to be copied from.
   * @stable ICU 2.6
   */
  RuleBasedNumberFormat& operator=(const RuleBasedNumberFormat& rhs);

  /**
   * Release memory allocated for a RuleBasedNumberFormat when you are finished with it.
   * @stable ICU 2.6
   */
  virtual ~RuleBasedNumberFormat();

  /**
   * Clone this object polymorphically.  The caller is responsible
   * for deleting the result when done.
   * @return  A copy of the object.
   * @stable ICU 2.6
   */
  virtual Format* clone(void) const;

  /**
   * Return true if the given Format objects are semantically equal.
   * Objects of different subclasses are considered unequal.
   * @param other    the object to be compared with.
   * @return        true if the given Format objects are semantically equal.
   * @stable ICU 2.6
   */
  virtual UBool operator==(const Format& other) const;

//-----------------------------------------------------------------------
// public API functions
//-----------------------------------------------------------------------

  /**
   * return the rules that were provided to the RuleBasedNumberFormat.
   * @return the result String that was passed in
   * @stable ICU 2.0
   */
  virtual UnicodeString getRules() const;

  /**
   * Return the number of public rule set names.
   * @return the number of public rule set names.
   * @stable ICU 2.0
   */
  virtual int32_t getNumberOfRuleSetNames() const;

  /**
   * Return the name of the index'th public ruleSet.  If index is not valid,
   * the function returns null.
   * @param index the index of the ruleset
   * @return the name of the index'th public ruleSet.
   * @stable ICU 2.0
   */
  virtual UnicodeString getRuleSetName(int32_t index) const;

  /**
   * Return the number of locales for which we have localized rule set display names.
   * @return the number of locales for which we have localized rule set display names.
   * @stable ICU 3.2
   */
  virtual int32_t getNumberOfRuleSetDisplayNameLocales(void) const;

  /**
   * Return the index'th display name locale.
   * @param index the index of the locale
   * @param status set to a failure code when this function fails
   * @return the locale
   * @see #getNumberOfRuleSetDisplayNameLocales
   * @stable ICU 3.2
   */
  virtual Locale getRuleSetDisplayNameLocale(int32_t index, UErrorCode& status) const;

    /**
     * Return the rule set display names for the provided locale.  These are in the same order
     * as those returned by getRuleSetName.  The locale is matched against the locales for
     * which there is display name data, using normal fallback rules.  If no locale matches,
     * the default display names are returned.  (These are the internal rule set names minus
     * the leading '%'.)
     * @param index the index of the rule set
     * @param locale the locale (returned by getRuleSetDisplayNameLocales) for which the localized
     * display name is desired
     * @return the display name for the given index, which might be bogus if there is an error
     * @see #getRuleSetName
     * @stable ICU 3.2
     */
  virtual UnicodeString getRuleSetDisplayName(int32_t index,
                          const Locale& locale = Locale::getDefault());

    /**
     * Return the rule set display name for the provided rule set and locale.
     * The locale is matched against the locales for which there is display name data, using
     * normal fallback rules.  If no locale matches, the default display name is returned.
     * @return the display name for the rule set
     * @stable ICU 3.2
     * @see #getRuleSetDisplayName
     */
  virtual UnicodeString getRuleSetDisplayName(const UnicodeString& ruleSetName,
                          const Locale& locale = Locale::getDefault());


  using NumberFormat::format;

  /**
   * Formats the specified 32-bit number using the default ruleset.
   * @param number The number to format.
   * @param toAppendTo the string that will hold the (appended) result
   * @param pos the fieldposition
   * @return A textual representation of the number.
   * @stable ICU 2.0
   */
  virtual UnicodeString& format(int32_t number,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos) const;

  /**
   * Formats the specified 64-bit number using the default ruleset.
   * @param number The number to format.
   * @param toAppendTo the string that will hold the (appended) result
   * @param pos the fieldposition
   * @return A textual representation of the number.
   * @stable ICU 2.1
   */
  virtual UnicodeString& format(int64_t number,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos) const;
  /**
   * Formats the specified number using the default ruleset.
   * @param number The number to format.
   * @param toAppendTo the string that will hold the (appended) result
   * @param pos the fieldposition
   * @return A textual representation of the number.
   * @stable ICU 2.0
   */
  virtual UnicodeString& format(double number,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos) const;

  /**
   * Formats the specified number using the named ruleset.
   * @param number The number to format.
   * @param ruleSetName The name of the rule set to format the number with.
   * This must be the name of a valid public rule set for this formatter.
   * @param toAppendTo the string that will hold the (appended) result
   * @param pos the fieldposition
   * @param status the status
   * @return A textual representation of the number.
   * @stable ICU 2.0
   */
  virtual UnicodeString& format(int32_t number,
                                const UnicodeString& ruleSetName,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;
  /**
   * Formats the specified 64-bit number using the named ruleset.
   * @param number The number to format.
   * @param ruleSetName The name of the rule set to format the number with.
   * This must be the name of a valid public rule set for this formatter.
   * @param toAppendTo the string that will hold the (appended) result
   * @param pos the fieldposition
   * @param status the status
   * @return A textual representation of the number.
   * @stable ICU 2.1
   */
  virtual UnicodeString& format(int64_t number,
                                const UnicodeString& ruleSetName,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;
  /**
   * Formats the specified number using the named ruleset.
   * @param number The number to format.
   * @param ruleSetName The name of the rule set to format the number with.
   * This must be the name of a valid public rule set for this formatter.
   * @param toAppendTo the string that will hold the (appended) result
   * @param pos the fieldposition
   * @param status the status
   * @return A textual representation of the number.
   * @stable ICU 2.0
   */
  virtual UnicodeString& format(double number,
                                const UnicodeString& ruleSetName,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;

  /**
   * Formats the specified number using the default ruleset.
   * @param obj The number to format.
   * @param toAppendTo the string that will hold the (appended) result
   * @param pos the fieldposition
   * @param status the status
   * @return A textual representation of the number.
   * @stable ICU 2.0
   */
  virtual UnicodeString& format(const Formattable& obj,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;
  /**
   * Redeclared Format method.
   * @param obj    the object to be formatted.
   * @param result Output param which will receive the formatted string.
   * @param status Output param set to success/failure code
   * @return       A reference to 'result'.
   * @stable ICU 2.0
   */
  UnicodeString& format(const Formattable& obj,
                        UnicodeString& result,
                        UErrorCode& status) const;

  /**
   * Redeclared NumberFormat method.
   * @param number    the double value to be formatted.
   * @param output    Output param which will receive the formatted string.
   * @return          A reference to 'output'.
   * @stable ICU 2.0
   */
   UnicodeString& format(double number,
                         UnicodeString& output) const;

  /**
   * Redeclared NumberFormat method.
   * @param number    the long value to be formatted.
   * @param output    Output param which will receive the formatted string.
   * @return          A reference to 'output'.
   * @stable ICU 2.0
   */
   UnicodeString& format(int32_t number,
                         UnicodeString& output) const;

  /**
   * Parses the specfied string, beginning at the specified position, according
   * to this formatter's rules.  This will match the string against all of the
   * formatter's public rule sets and return the value corresponding to the longest
   * parseable substring.  This function's behavior is affected by the lenient
   * parse mode.
   * @param text The string to parse
   * @param result the result of the parse, either a double or a long.
   * @param parsePosition On entry, contains the position of the first character
   * in "text" to examine.  On exit, has been updated to contain the position
   * of the first character in "text" that wasn't consumed by the parse.
   * @see #setLenient
   * @stable ICU 2.0
   */
  virtual void parse(const UnicodeString& text,
                     Formattable& result,
                     ParsePosition& parsePosition) const;


  /**
   * Redeclared Format method.
   * @param text   The string to parse
   * @param result the result of the parse, either a double or a long.
   * @param status Output param set to failure code when a problem occurs.
   * @stable ICU 2.0
   */
  virtual inline void parse(const UnicodeString& text,
                      Formattable& result,
                      UErrorCode& status) const;

#if !UCONFIG_NO_COLLATION

  /**
   * Turns lenient parse mode on and off.
   *
   * When in lenient parse mode, the formatter uses a Collator for parsing the text.
   * Only primary differences are treated as significant.  This means that case
   * differences, accent differences, alternate spellings of the same letter
   * (e.g., ae and a-umlaut in German), ignorable characters, etc. are ignored in
   * matching the text.  In many cases, numerals will be accepted in place of words
   * or phrases as well.
   *
   * For example, all of the following will correctly parse as 255 in English in
   * lenient-parse mode:
   * <br>"two hundred fifty-five"
   * <br>"two hundred fifty five"
   * <br>"TWO HUNDRED FIFTY-FIVE"
   * <br>"twohundredfiftyfive"
   * <br>"2 hundred fifty-5"
   *
   * The Collator used is determined by the locale that was
   * passed to this object on construction.  The description passed to this object
   * on construction may supply additional collation rules that are appended to the
   * end of the default collator for the locale, enabling additional equivalences
   * (such as adding more ignorable characters or permitting spelled-out version of
   * symbols; see the demo program for examples).
   *
   * It's important to emphasize that even strict parsing is relatively lenient: it
   * will accept some text that it won't produce as output.  In English, for example,
   * it will correctly parse "two hundred zero" and "fifteen hundred".
   *
   * @param enabled If true, turns lenient-parse mode on; if false, turns it off.
   * @see RuleBasedCollator
   * @stable ICU 2.0
   */
  virtual void setLenient(UBool enabled);

  /**
   * Returns true if lenient-parse mode is turned on.  Lenient parsing is off
   * by default.
   * @return true if lenient-parse mode is turned on.
   * @see #setLenient
   * @stable ICU 2.0
   */
  virtual inline UBool isLenient(void) const;

#endif

  /**
   * Override the default rule set to use.  If ruleSetName is null, reset
   * to the initial default rule set.  If the rule set is not a public rule set name,
   * U_ILLEGAL_ARGUMENT_ERROR is returned in status.
   * @param ruleSetName the name of the rule set, or null to reset the initial default.
   * @param status set to failure code when a problem occurs.
   * @stable ICU 2.6
   */
  virtual void setDefaultRuleSet(const UnicodeString& ruleSetName, UErrorCode& status);

  /**
   * Return the name of the current default rule set.  If the current rule set is
   * not public, returns a bogus (and empty) UnicodeString.
   * @return the name of the current default rule set
   * @stable ICU 3.0
   */
  virtual UnicodeString getDefaultRuleSetName() const;

public:
    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 2.8
     */
    static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 2.8
     */
    virtual UClassID getDynamicClassID(void) const;

    /**
     * Sets the decimal format symbols, which is generally not changed
     * by the programmer or user. The formatter takes ownership of
     * symbolsToAdopt; the client must not delete it.
     *
     * @param symbolsToAdopt DecimalFormatSymbols to be adopted.
     * @draft ICU 49
     */
    virtual void adoptDecimalFormatSymbols(DecimalFormatSymbols* symbolsToAdopt);

    /**
     * Sets the decimal format symbols, which is generally not changed
     * by the programmer or user. A clone of the symbols is created and
     * the symbols is _not_ adopted; the client is still responsible for
     * deleting it.
     *
     * @param symbols DecimalFormatSymbols.
     * @draft ICU 49
     */
    virtual void setDecimalFormatSymbols(const DecimalFormatSymbols& symbols);

private:
    RuleBasedNumberFormat(); // default constructor not implemented

    // this will ref the localizations if they are not NULL
    // caller must deref to get adoption
    RuleBasedNumberFormat(const UnicodeString& description, LocalizationInfo* localizations,
              const Locale& locale, UParseError& perror, UErrorCode& status);

    void init(const UnicodeString& rules, LocalizationInfo* localizations, UParseError& perror, UErrorCode& status);
    void dispose();
    void stripWhitespace(UnicodeString& src);
    void initDefaultRuleSet();
    void format(double number, NFRuleSet& ruleSet);
    NFRuleSet* findRuleSet(const UnicodeString& name, UErrorCode& status) const;

    /* friend access */
    friend class NFSubstitution;
    friend class NFRule;
    friend class FractionalPartSubstitution;

    inline NFRuleSet * getDefaultRuleSet() const;
    Collator * getCollator() const;
    DecimalFormatSymbols * getDecimalFormatSymbols() const;

private:
    NFRuleSet **ruleSets;
    UnicodeString* ruleSetDescriptions;
    int32_t numRuleSets;
    NFRuleSet *defaultRuleSet;
    Locale locale;
    Collator* collator;
    DecimalFormatSymbols* decimalFormatSymbols;
    UBool lenient;
    UnicodeString* lenientParseRules;
    LocalizationInfo* localizations;
};

// ---------------

inline UnicodeString&
RuleBasedNumberFormat::format(const Formattable& obj,
                              UnicodeString& result,
                              UErrorCode& status) const
{
    // Don't use Format:: - use immediate base class only,
    // in case immediate base modifies behavior later.
    // dlf - the above comment is bogus, if there were a reason to modify
    // it, it would be virtual, and there's no reason because it is
    // a one-line macro in NumberFormat anyway, just like this one.
    return NumberFormat::format(obj, result, status);
}

inline UnicodeString&
RuleBasedNumberFormat::format(double number, UnicodeString& output) const {
    FieldPosition pos(0);
    return format(number, output, pos);
}

inline UnicodeString&
RuleBasedNumberFormat::format(int32_t number, UnicodeString& output) const {
    FieldPosition pos(0);
    return format(number, output, pos);
}

inline void
RuleBasedNumberFormat::parse(const UnicodeString& text, Formattable& result, UErrorCode& status) const
{
    NumberFormat::parse(text, result, status);
}

#if !UCONFIG_NO_COLLATION

inline UBool
RuleBasedNumberFormat::isLenient(void) const {
    return lenient;
}

#endif

inline NFRuleSet*
RuleBasedNumberFormat::getDefaultRuleSet() const {
    return defaultRuleSet;
}

U_NAMESPACE_END

/* U_HAVE_RBNF */
#endif

/* RBNF_H */
#endif
