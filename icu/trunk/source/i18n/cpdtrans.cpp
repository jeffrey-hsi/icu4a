/*
**********************************************************************
*   Copyright (C) 1999, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/17/99    aliu        Creation.
**********************************************************************
*/
#include "unicode/cpdtrans.h"
#include "unicode/unifilt.h"
#include "unicode/unifltlg.h"
#include "unicode/uniset.h"
#include "uvector.h"

// keep in sync with Transliterator
static const UChar ID_SEP   = 0x002D; /*-*/
static const UChar ID_DELIM = 0x003B; /*;*/
static const UChar NEWLINE  = 10;

// Empty string
static const UChar EMPTY[] = {0}; //""

U_NAMESPACE_BEGIN

/**
 * Constructs a new compound transliterator given an array of
 * transliterators.  The array of transliterators may be of any
 * length, including zero or one, however, useful compound
 * transliterators have at least two components.
 * @param transliterators array of <code>Transliterator</code>
 * objects
 * @param transliteratorCount The number of
 * <code>Transliterator</code> objects in transliterators.
 * @param filter the filter.  Any character for which
 * <tt>filter.contains()</tt> returns <tt>false</tt> will not be
 * altered by this transliterator.  If <tt>filter</tt> is
 * <tt>null</tt> then no filtering is applied.
 */
CompoundTransliterator::CompoundTransliterator(
                           Transliterator* const transliterators[],
                           int32_t transliteratorCount,
                           UnicodeFilter* adoptedFilter) :
    Transliterator(joinIDs(transliterators, transliteratorCount), adoptedFilter),
    trans(0), count(0), compoundRBTIndex(-1)  {
    setTransliterators(transliterators, transliteratorCount);
}

/**
 * Splits an ID of the form "ID;ID;..." into a compound using each
 * of the IDs. 
 * @param id of above form
 * @param forward if false, does the list in reverse order, and
 * takes the inverse of each ID.
 */
CompoundTransliterator::CompoundTransliterator(const UnicodeString& id,
                              UTransDirection direction,
                              UnicodeFilter* adoptedFilter,
                              UParseError& parseError,
                              UErrorCode& status) :
    Transliterator(id, adoptedFilter),
    trans(0), compoundRBTIndex(-1) {
    init(id, direction, -1, 0, TRUE,parseError,status);
}

CompoundTransliterator::CompoundTransliterator(const UnicodeString& id,
                              UParseError& parseError,
                              UErrorCode& status) :
    Transliterator(id, 0), // set filter to 0 here!
    trans(0), compoundRBTIndex(-1) {
    init(id, UTRANS_FORWARD, -1, 0, TRUE,parseError,status);
}

/**
 * Private constructor for compound RBTs.  Construct a compound
 * transliterator using the given idBlock, with the adoptedTrans
 * inserted at the idSplitPoint.
 */
CompoundTransliterator::CompoundTransliterator(const UnicodeString& ID,
                                               const UnicodeString& idBlock,
                                               int32_t idSplitPoint,
                                               Transliterator *adoptedTrans,
                                               UParseError& parseError,
                                               UErrorCode& status) :
    Transliterator(ID, 0),
    trans(0), compoundRBTIndex(-1) {
    init(idBlock, UTRANS_FORWARD, idSplitPoint, adoptedTrans, FALSE,parseError,status);
}

/**
 * Private constructor for Transliterator from a vector of
 * transliterators.  The vector order is FORWARD, so if dir is REVERSE
 * then the vector order will be reversed.  The caller is responsible
 * for fixing up the ID.
 */
CompoundTransliterator::CompoundTransliterator(UTransDirection dir,
                                               UVector& list,
                                               UErrorCode& status) :
    Transliterator(UnicodeString("", ""), 0),
    trans(0), compoundRBTIndex(-1) {
    init(list, dir, FALSE, status);
    // assume caller will fixup ID
}

/**
 * Finish constructing a transliterator: only to be called by
 * constructors.  Before calling init(), set trans and filter to NULL.
 * @param id the id containing ';'-separated entries
 * @param direction either FORWARD or REVERSE
 * @param idSplitPoint the index into id at which the
 * adoptedSplitTransliterator should be inserted, if there is one, or
 * -1 if there is none.
 * @param adoptedSplitTransliterator a transliterator to be inserted
 * before the entry at offset idSplitPoint in the id string.  May be
 * NULL to insert no entry.
 * @param fixReverseID if TRUE, then reconstruct the ID of reverse
 * entries by calling getID() of component entries.  Some constructors
 * do not require this because they apply a facade ID anyway.
 * @param status the error code indicating success or failure
 */
void CompoundTransliterator::init(const UnicodeString& id,
                                  UTransDirection direction,
                                  int32_t idSplitPoint,
                                  Transliterator *adoptedSplitTrans,
                                  UBool fixReverseID,
                                  UParseError& parseError,
                                  UErrorCode& status) {
    // assert(trans == 0);

    if (U_FAILURE(status)) {
        delete adoptedSplitTrans;
        return;
    }

    UVector list(status);
    UnicodeSet* compoundFilter = NULL;
    UnicodeString regenID;
    Transliterator::parseCompoundID(id, regenID, direction,
                                    idSplitPoint, adoptedSplitTrans,
                                    list, compoundRBTIndex, compoundFilter,
                                    parseError, status);

    init(list, direction, fixReverseID, status);

    if (compoundFilter != NULL) {
        adoptFilter(compoundFilter);
    }
}

/**
 * Finish constructing a transliterator: only to be called by
 * constructors.  Before calling init(), set trans and filter to NULL.
 * @param list a vector of transliterator objects to be adopted.  It
 * should NOT be empty.  The list should be in declared order.  That
 * is, it should be in the FORWARD order; if direction is REVERSE then
 * the list order will be reversed.
 * @param direction either FORWARD or REVERSE
 * @param fixReverseID if TRUE, then reconstruct the ID of reverse
 * entries by calling getID() of component entries.  Some constructors
 * do not require this because they apply a facade ID anyway.
 * @param status the error code indicating success or failure
 */
void CompoundTransliterator::init(UVector& list,
                                  UTransDirection direction,
                                  UBool fixReverseID,
                                  UErrorCode& status) {
    // assert(trans == 0);

    // Allocate array
    if (U_SUCCESS(status)) {
        count = list.size();
        trans = new Transliterator*[count];
    }

    if (U_FAILURE(status) || trans == 0) {
         // assert(trans == 0);
        return;
    }

    // Move the transliterators from the vector into an array.
    // Reverse the order if necessary.
    int32_t i;
    for (i=0; i<count; ++i) {
        int32_t j = (direction == UTRANS_FORWARD) ? i : count - 1 - i;
        trans[i] = (Transliterator*) list.elementAt(j);
    }

    // Fix compoundRBTIndex for REVERSE transliterators
    if (compoundRBTIndex >= 0 && direction == UTRANS_REVERSE) {
        compoundRBTIndex = count - 1 - compoundRBTIndex;
    }

    // If the direction is UTRANS_REVERSE then we may need to fix the
    // ID.
    if (direction == UTRANS_REVERSE && fixReverseID) {
        UnicodeString newID;
        for (i=0; i<count; ++i) {
            if (i > 0) {
                newID.append(ID_DELIM);
            }
            newID.append(trans[i]->getID());
        }
        setID(newID);
    }

    computeMaximumContextLength();
}

/**
 * Return the IDs of the given list of transliterators, concatenated
 * with ID_DELIM delimiting them.  Equivalent to the perlish expression
 * join(ID_DELIM, map($_.getID(), transliterators).
 */
UnicodeString CompoundTransliterator::joinIDs(Transliterator* const transliterators[],
                                              int32_t transCount) {
    UnicodeString id;
    for (int32_t i=0; i<transCount; ++i) {
        if (i > 0) {
            id.append(ID_DELIM);
        }
        id.append(transliterators[i]->getID());
    }
    return id; // Return temporary
}

/**
 * Copy constructor.
 */
CompoundTransliterator::CompoundTransliterator(const CompoundTransliterator& t) :
    Transliterator(t), trans(0), count(0), compoundRBTIndex(-1) {
    *this = t;
}

/**
 * Destructor
 */
CompoundTransliterator::~CompoundTransliterator() {
    freeTransliterators();
}

void CompoundTransliterator::freeTransliterators(void) {
    if (trans != 0) {
        for (int32_t i=0; i<count; ++i) {
            delete trans[i];
        }
        delete[] trans;
    }
    trans = 0;
    count = 0;
}

/**
 * Assignment operator.
 */
CompoundTransliterator& CompoundTransliterator::operator=(
                                             const CompoundTransliterator& t) {
    Transliterator::operator=(t);
    int32_t i;
    for (i=0; i<count; ++i) {
        delete trans[i];
        trans[i] = 0;
    }
    if (t.count > count) {
        delete[] trans;
        trans = new Transliterator*[t.count];
    }
    count = t.count;
    for (i=0; i<count; ++i) {
        trans[i] = t.trans[i]->clone();
    }
    compoundRBTIndex = t.compoundRBTIndex;
    return *this;
}

/**
 * Transliterator API.
 */
Transliterator* CompoundTransliterator::clone(void) const {
    return new CompoundTransliterator(*this);
}

/**
 * Returns the number of transliterators in this chain.
 * @return number of transliterators in this chain.
 */
int32_t CompoundTransliterator::getCount(void) const {
    return count;
}

/**
 * Returns the transliterator at the given index in this chain.
 * @param index index into chain, from 0 to <code>getCount() - 1</code>
 * @return transliterator at the given index
 */
const Transliterator& CompoundTransliterator::getTransliterator(int32_t index) const {
    return *trans[index];
}

void CompoundTransliterator::setTransliterators(Transliterator* const transliterators[],
                                                int32_t transCount) {
    Transliterator** a = new Transliterator*[transCount];
    for (int32_t i=0; i<transCount; ++i) {
        a[i] = transliterators[i]->clone();
    }
    adoptTransliterators(a, transCount);
}

void CompoundTransliterator::adoptTransliterators(Transliterator* adoptedTransliterators[],
                                                  int32_t transCount) {
    // First free trans[] and set count to zero.  Once this is done,
    // orphan the filter.  Set up the new trans[].
    freeTransliterators();
    trans = adoptedTransliterators;
    count = transCount;
    computeMaximumContextLength();
    setID(joinIDs(trans, count));
}

/**
 * Append c to buf, unless buf is empty or buf already ends in c.
 */
static void _smartAppend(UnicodeString& buf, UChar c) {
    if (buf.length() != 0 &&
        buf.charAt(buf.length() - 1) != c) {
        buf.append(c);
    }
}

UnicodeString& CompoundTransliterator::toRules(UnicodeString& rulesSource,
                                               UBool escapeUnprintable) const {
    // We do NOT call toRules() on our component transliterators, in
    // general.  If we have several rule-based transliterators, this
    // yields a concatenation of the rules -- not what we want.  We do
    // handle compound RBT transliterators specially -- those for which
    // compoundRBTIndex >= 0.  For the transliterator at compoundRBTIndex,
    // we do call toRules() recursively.
    rulesSource.truncate(0);
    if (compoundRBTIndex >= 0 && getFilter() != NULL) {
        // If we are a compound RBT and if we have a global
        // filter, then emit it at the top.
        UnicodeString pat;
        rulesSource.append("::").append(getFilter()->toPattern(pat, escapeUnprintable)).append(ID_DELIM);
    }
    for (int32_t i=0; i<count; ++i) {
        UnicodeString rule;
        if (i == compoundRBTIndex) {
            trans[i]->toRules(rule, escapeUnprintable);
        } else {
            trans[i]->Transliterator::toRules(rule, escapeUnprintable);
        }
        _smartAppend(rulesSource, NEWLINE);
        rulesSource.append(rule);
        _smartAppend(rulesSource, ID_DELIM);
    }
    return rulesSource;
}

/**
 * Implements {@link Transliterator#handleTransliterate}.
 */
void CompoundTransliterator::handleTransliterate(Replaceable& text, UTransPosition& index,
                                                 UBool incremental) const {
    /* Call each transliterator with the same contextStart and
     * start, but with the limit as modified
     * by preceding transliterators.  The start index must be
     * reset for each transliterator to give each a chance to
     * transliterate the text.  The initial contextStart index is known
     * to still point to the same place after each transliterator
     * is called because each transliterator will not change the
     * text between contextStart and the initial start index.
     *
     * IMPORTANT: After the first transliterator, each subsequent
     * transliterator only gets to transliterate text committed by
     * preceding transliterators; that is, the start (output
     * value) of transliterator i becomes the limit (input value)
     * of transliterator i+1.  Finally, the overall limit is fixed
     * up before we return.
     *
     * Assumptions we make here:
     * (1) contextStart <= start <= limit <= contextLimit <= text.length()
     * (2) start <= start' <= limit'  ;cursor doesn't move back
     * (3) start <= limit'            ;text before cursor unchanged
     * - start' is the value of start after calling handleKT
     * - limit' is the value of limit after calling handleKT
     */
    
    /**
     * Example: 3 transliterators.  This example illustrates the
     * mechanics we need to implement.  C, S, and L are the contextStart,
     * start, and limit.  gl is the globalLimit.  contextLimit is
     * equal to limit throughout.
     *
     * 1. h-u, changes hex to Unicode
     *
     *    4  7  a  d  0      4  7  a
     *    abc/u0061/u    =>  abca/u    
     *    C  S       L       C   S L   gl=f->a
     *
     * 2. upup, changes "x" to "XX"
     *
     *    4  7  a       4  7  a
     *    abca/u    =>  abcAA/u    
     *    C  SL         C    S   
     *                       L    gl=a->b
     * 3. u-h, changes Unicode to hex
     *
     *    4  7  a        4  7  a  d  0  3
     *    abcAA/u    =>  abc/u0041/u0041/u    
     *    C  S L         C              S
     *                                  L   gl=b->15
     * 4. return
     *
     *    4  7  a  d  0  3
     *    abc/u0041/u0041/u    
     *    C S L
     */

    if (count < 1) {
        index.start = index.limit;
        return; // Short circuit for empty compound transliterators
    }

    // compoundLimit is the limit value for the entire compound
    // operation.  We overwrite index.limit with the previous
    // index.start.  After each transliteration, we update
    // compoundLimit for insertions or deletions that have happened.
    int32_t compoundLimit = index.limit;

    // compoundStart is the start for the entire compound
    // operation.
    int32_t compoundStart = index.start;
    
    int32_t delta = 0; // delta in length

    // Give each transliterator a crack at the run of characters.
    // See comments at the top of the method for more detail.
    for (int32_t i=0; i<count; ++i) {
        index.start = compoundStart; // Reset start
        int32_t limit = index.limit;
        
        trans[i]->filteredTransliterate(text, index, incremental);
        
        // Cumulative delta for insertions/deletions
        delta += index.limit - limit;
        
        if (incremental) {
            // In the incremental case, only allow subsequent
            // transliterators to modify what has already been
            // completely processed by prior transliterators.  In the
            // non-incrmental case, allow each transliterator to
            // process the entire text.
            index.limit = index.start;
        }
    }

    compoundLimit += delta;

    // Start is good where it is -- where the last transliterator left
    // it.  Limit needs to be put back where it was, modulo
    // adjustments for deletions/insertions.
    index.limit = compoundLimit;
}

/**
 * Sets the length of the longest context required by this transliterator.
 * This is <em>preceding</em> context.
 */
void CompoundTransliterator::computeMaximumContextLength(void) {
    int32_t max = 0;
    for (int32_t i=0; i<count; ++i) {
        int32_t len = trans[i]->getMaximumContextLength();
        if (len > max) {
            max = len;
        }
    }
    setMaximumContextLength(max);
}

U_NAMESPACE_END

/* eof */
