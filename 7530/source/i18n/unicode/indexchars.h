/*
*******************************************************************************
*
*   Copyright (C) 2010 International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*/

#ifndef INDEXCHARS_H
#define INDEXCHARS_H

#include "unicode/uobject.h"
#include "unicode/locid.h"

/**
 * \file 
 * \brief C++ API: Index Characters
 */
 
struct UHashtable;

U_NAMESPACE_BEGIN

// Forward Declarations

class Collator;
class RuleBasedCollator;
class StringEnumeration;
class UnicodeSet;
class UVector;

/**
 * A set of characters for use as a UI "index", that is, a
 * list of clickable characters (or character sequences) that allow the user to
 * see a segment of a larger "target" list. That is, each character corresponds
 * to a bucket in the target list, where everything in the bucket is greater
 * than or equal to the character (according to the locale's collation). The
 * intention is to have two main functions; one that produces an index list that
 * is relatively static, and the other is a list that produces roughly
 * equally-sized buckets. Only the first is currently provided.
 * <p>
 * The static list would be presented as something like
 * 
 * <pre>
 *  A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
 * </pre>
 * 
 * In the UI, an index character could be omitted if its bucket is empty. For
 * example, if there is nothing in the bucket for Q, then Q could be omitted.
 * <p>
 * <b>Important Notes:</b>
 * <ul>
 * <li>Although we say "character" above, the index character could be a
 * sequence, like "CH".</li>
 * <li>There could be items in a target list that are less than the first or
 * (much) greater than the last; examples include words from other scripts. The
 * UI could bucket them with the first or last respectively, or have some symbol
 * for those categories.</li>
 * <li>The use of the list requires that the target list be sorted according to
 * the locale that is used to create that list.</li>
 * <li>For languages without widely accepted sorting methods (eg Chinese/Japanese)
 * the results may appear arbitrary, and it may be best not to use these methods.</li>
 * <li>In the initial version, an arbitrary limit of 100 is placed on these lists.</li>
 * </ul>
 * 
 * @draft ICU 4.6
 * @provisional This API might change or be removed in a future release.
 */
class U_I18N_API IndexCharacters: public UObject {

  public:

    /**
     * Construct an IndexCharacters object for the specified locale.  If the locale's
     * data does not include index characters, a set of them will be
     * synthesized based on the locale's exemplar characters.
     *
     * @param locale the desired locale.
     * @param status Error code, will be set with the reason if the construction
     *               of the IndexCharacters object fails.
     * @draft ICU 4.6
     */
     IndexCharacters(const Locale &locale, UErrorCode &status);


    /**
     * Construct an IndexCharacters object for the specified locale, and
     * add an additional set of index characters.  If the locale's
     * data does not include index characters, a set of them will be
     * synthesized based on the locale's exemplar characters.
     * 
     * @param locale
     *            The locale to be passed.
     * @param additions
     *            Additional characters to be added, eg A-Z for non-Latin locales.
     * @param status Error code, will be set with the reason if the construction
     *               of the IndexCharacters object fails.
     * @draft ICU 4.6
     * @provisional This API might change or be removed in a future release.
     */
     IndexCharacters(const Locale &locale, const UnicodeSet &additions, UErrorCode &status);


#if 0
    TODO:  Late addtion to Java.  How best to do this here?

     /**
     * Create the index object.
     * 
     * @param locale
     *            The locale to be passed.
     * @param additions
     *            Additional characters to be added, eg A-Z for non-Latin locales.
     * @draft ICU 4.4
     * @provisional This API might change or be removed in a future release.
     */
    public IndexCharacters(ULocale locale, ULocale... additionalLocales) {
#endif

    /**
     * Internal constructor.  Has much implementation for others.
     * @internal
     * @param exemplarChars
     *            TODO
     */
    IndexCharacters(const Locale &locale, RuleBasedCollator *collator, 
                    const UnicodeSet *exemplarChars, const UnicodeSet *additions, 
                    UErrorCode &status);


     /**
      * Copy constructor
      *
      * @param other  The source IndexCharacters object.
      * @param status Error code, will be set with the reason if the construction fails.
      * @draft ICU 4.6
      */
     IndexCharacters(const IndexCharacters &other, UErrorCode &status);

     /**
      * Destructor
      */
     virtual ~IndexCharacters();


    /**
     * Equality operator.
     * @draft ICU 4.6
     */
     virtual UBool operator==(const IndexCharacters& other) const;

    /**
     * Inequality operator.
     * @draft ICU 4.6
     */
     virtual UBool operator!=(const IndexCharacters& other) const;



    /**
     * Get a String Enumeration that will produce the index characters.
     * @return A String Enumeration over the index characters.
     * @draft ICU 4.6
     */
     virtual StringEnumeration *getIndexCharacters() const;

   /**
     * Get the locale.
     * TODO: The one specified?  The one actually used?  For collation or exepmplars?
     * @return The locale.
     * @draft ICU 4.6
     */
    virtual Locale getLocale() const;


    /**
     * Get the Collator that establishes the ordering of the index characters.
     * Ownership of the collator remains with the IndexCharacters instance.
     * @return The collator
     * @draft ICU 4.6
     */
    virtual const Collator &getCollator() const;

   /**
     * Get the default label used for abbreviated buckets <i>between</i> other index characters. 
     * For example, consider the index characters for Latin and Greek are used: 
     *     X Y Z ... &#x0391; &#x0392; &#x0393;.
     * 
     * @param status Error code, will be set with the reason if the operation fails.
     * @return inflow label
     * @draft ICU 4.6
     */
    virtual UnicodeString getInflowLabel(UErrorCode &status) const;


   /**
     * Get the default label used in the IndexCharacters' locale for overflow, eg the first item in:
     *     ... A B C
     * 
     * @param status Error code, will be set with the reason if the operation fails.
     * @return overflow label
     * @draft ICU 4.6
     */
    virtual UnicodeString getOverflowLabel(UErrorCode &status) const;


   /**
     * Get the default label used in the IndexCharacters' locale for underflow, eg the last item in:
     *    X Y Z ...
     * 
     * @param status Error code, will be set with the reason if the operation fails.
     * @return underflow label
     * @draft ICU 4.6
     */
    virtual UnicodeString getUnderflowLabel(UErrorCode &status) const;


    /**
     * Get the Unicode character (or tailored string) that defines an overflow bucket; that is anything greater than or
     * equal to that string should go in that bucket, instead of with the last character. Normally that is the first
     * character of the script after lowerLimit. Thus in X Y Z ... <i>Devanagari-ka</i>, the overflow character for Z
     * would be the <i>Greek-alpha</i>.
     * 
     * @param lowerLimit   The character below the overflow (or inflow) bucket
     * @return string that defines top of the overflow buck for lowerLimit, or null if there is none
     * @draft ICU 4.6
     */
    virtual UnicodeString getOverflowComparisonString(const UnicodeString &lowerLimit);
    

    /**
     * Add an item to an index.
     *
     * @param name The display name for the item.  The item will be placed under
     *             an index label based on this name.
     * @param context An optional pointer to user data associated with this
     *                item.  When interating the contents of an index, the
     *                context pointer will be returned along with the name for
     *                each item.
     * @param status  Error code, will be set with the reason if the operation fails.
     * @draft ICU 4.6
     */
    virtual void addItem(const UnicodeString &name, void *context, UErrorCode &status);

    virtual UBool nextLabel();
    virtual const UnicodeString &getLabel();
    virtual void resetLabelIterator();

    // and for each label, iterate over its items
    virtual UBool nextItem();
    virtual const UnicodeString &getItemName();
    virtual const void *getItemContext();
    virtual void resetItemIterator();

  private:
     /**
      *   No assignment.  IndexCharacters objects are const after creation/
      */
     IndexCharacters &operator =(const IndexCharacters & /*other*/) { return *this;};

     // Common initialization, for use from all constructors.
     void init(UErrorCode &status);

     UnicodeSet *getIndexExemplars(const Locale &locale, UBool &explicitIndexChars,
                                          UErrorCode &status);
         // TODO:  change getIndexExemplars() to static once the constant UnicodeSets
         //         are factored out into a singleton.

     UVector *firstStringsInScript(Collator *coll, UErrorCode &status);

     UnicodeString separated(const UnicodeString &item);

     static UnicodeSet *getScriptSet(const UnicodeString &codePoint);

     void buildIndex();


     // TODO:  coordinate with Java. 
     //        Names anticipate an eventual plain C API.
     enum LabelType {
         ALPHABETIC_INDEX_NORMAL   = 0,
         ALPHABETIC_INDEX_UNDERFLOW = 1,
         ALPHABETIC_INDEX_INFLOW    = 2,
         ALPHABETIC_INDEX_OVERFLOW  = 3
     };


     /*
     * A record to be sorted into buckets with getIndexBucketCharacters.
     */
     struct Record: public UMemory {
         const UnicodeString  *name_;
         void                 *context_;
         ~Record() {delete name_;};
     };

     // Holds all user items before they are distributed into buckets.
     UVector  *inputRecords_;

     struct Bucket: public UMemory {
         UnicodeString     label_;
         UnicodeString     lowerBoundary_;
         LabelType         labelType_;
         UVector           *records_;
     };

     // Holds the contents of this index, buckets of user items.
     // UVector elements are of type (Bucket *)
     UVector *buckets_;
     int32_t  labelsIterIndex_;    // Index of next item to return.

     
// Constants.  TODO:  move into a singleton and init constants only once.
//
     UChar32 OVERFLOW_MARKER;
     UChar32 INFLOW_MARKER;
     UnicodeSet *ALPHABETIC;
     UnicodeSet *HANGUL;
     UnicodeSet *ETHIOPIC;
     UnicodeSet *CORE_LATIN;
     UnicodeSet *IGNORE_SCRIPTS;
     UnicodeSet *TO_TRY;
     UVector    *FIRST_CHARS_IN_SCRIPTS;

     // LinkedHashMap<String, Set<String>> alreadyIn = new LinkedHashMap<String, Set<String>>();
     UHashtable *alreadyIn_;   // Key=UnicodeString, value=UnicodeSet

     UVector    *indexCharacters_;   // Contents are (UnicodeString *)
     UnicodeSet *noDistinctSorting_;
     UnicodeSet *notAlphabetic_;
     UVector    *firstScriptCharacters_;

     Locale    locale_;
     Collator  *comparator_;

     UnicodeString  inflowLabel_;
     UnicodeString  overflowLabel_;
     UnicodeString  underflowLabel_;
     UnicodeString  overflowComparisonString_;


};

U_NAMESPACE_END
#endif

