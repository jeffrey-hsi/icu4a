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

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/locid.h"

/**
 * \file
 * \brief C++ API: Index Characters
 */


U_CDECL_BEGIN

/**
 * Constants for Alphabetic Index Label Types.
 * The form of these enum constants anticipates having a plain C API
 * for Alphabetic Indexes that will also use them.
 * @draft ICU 4.6
 */
typedef enum UAlphabeticIndexLabelType {
         /**
          *  Normal Label, typically the starting letter of the names
          *  in the bucket with this label.
          * @draft ICU 4.6
          */
         U_ALPHAINDEX_NORMAL    = 0,

         /**
          * Undeflow Label.  The bucket with this label contains names
          * in scripts that sort before any of the bucket labels in this index.
          * @draft ICU 4.6
          */
         U_ALPHAINDEX_UNDERFLOW = 1,

         /**
          * Inflow Label.  The bucket with this label contains names
          * in scripts that sort between two of the bucket labels in this index.
          * Inflow labels are created when an index contains normal labels for
          * multiple scripts, and skips other scripts that sort between some of the
          * included scripts.
          * @draft ICU 4.6
          */
         U_ALPHAINDEX_INFLOW    = 2,

         /**
          * Overflow Label. Te bucket with this label contains names in scripts
          * that sort after all of the bucket labels in this index.
          * @draft ICU 4.6
          */
         U_ALPHAINDEX_OVERFLOW  = 3
     } UAlphabeticIndexLabelType;


struct UHashtable;
U_CDECL_END

U_NAMESPACE_BEGIN

// Forward Declarations

class Collator;
class RuleBasedCollator;
class StringEnumeration;
class UnicodeSet;
class UVector;



/**
  * A class that supports the creation of a UI index appropriate for a given language, such as:
 *
 * <pre>
 *  <b>… A B C D E F G H I J K L M N O P Q R S T U V W X Y Z Æ Ø Å …</b>
 *
 *  <b>A</b>
 *     Addison
 *     Albertson
 *     Azensky
 *  <b>B</b>
 *     Bäcker
 *  ...
 * </pre>
 *
 * The class can generate a list of labels for use as a UI "index", that is, a list of
 * clickable characters (or character sequences) that allow the user to see a segment
 * (bucket) of a larger "target" list. That is, each label corresponds to a bucket in
 * the target list, where everything in the bucket is greater than or equal to the character
 * (according to the locale's collation). Strings can be added to the index;
 * they will be in sorted order in the right bucket.
 * <p>
 * The class also supports having buckets for strings before the first (underflow),
 * after the last (overflow), and between scripts (inflow). For example, if the index
 * is constructed with labels for Russian and English, Greek characters would fall
 * into an inflow bucket between the other two scripts.
 * <p>
 * The AlphabeticIndex class is not intended for public subclassing.
 * <p>
 * <i>Example</i>
 * <p>
 * The "show..." methods below are just to illustrate usage.
 *
 * <pre>
 * // Create a simple index.  "Item" is assumed to be an application
 * // defined type that the application's UI and other processing knows about,
 * //  and that has a name.
 *
 * UErrorCode status = U_ZERO_ERROR;
 * AlphabeticIndex index = new AlphabeticIndex(desiredLocale, status);
 * index->addLabels(additionalLocale, status);
 * for (Item *item in some source of Items ) {
 *     index->addRecord(item->name(), item, status);
 * }
 * ...
 * // Show index at top. We could skip or gray out empty buckets
 *
 * while (index->nextBucket(status)) {
 *     if (showAll || index->getBucketRecordCount() != 0) {
 *         showLabelAtTop(UI, index->getBucketLabel());
 *     }
 * }
 *  ...
 * // Show the buckets with their contents, skipping empty buckets
 *
 * index->resetBucketIterator(status);
 * while (index->nextBucket(status)) {
 *    if (index->getBucketRecordCount() != 0) {
 *        showLabelInList(UI, index->getBucketLabel());
 *        while (index->nextRecord(status)) {
 *            showIndexedItem(UI, static_cast<Item *>(index->getRecordData()))
 * </pre>
 *
 * The caller can build different UIs using this class.
 * For example, an index character could be omitted or grayed-out
 * if its bucket is empty. Small buckets could also be combined based on size, such as:
 *
 * <pre>
 * <b>… A-F G-N O-Z …</b>
 * </pre>
 *
 * <p>
 * <b>Notes:</b>
 * <ul>
 * <li>Additional collation parameters can be passed in as part of the locale name.
 *     For example, German plus numeric
 *     sorting would be "de@kn-true".
 *
 * @draft ICU 4.6
 * @provisional This API might change or be removed in a future release.
 */


class U_I18N_API AlphabeticIndex: public UObject {

  public:

    /**
     * Construct an AlphabeticIndex object for the specified locale.  If the locale's
     * data does not include index characters, a set of them will be
     * synthesized based on the locale's exemplar characters.  The locale
     * determines the sorting order for both the index characters and the
     * user item names appearing under each Index character.
     *
     * @param locale the desired locale.
     * @param status Error code, will be set with the reason if the construction
     *               of the AlphabeticIndex object fails.
     * @draft ICU 4.6
     */
     AlphabeticIndex(const Locale &locale, UErrorCode &status);



    /**
     * Add Labels to this Index.  The labels are additions those
     * that are already in the index; they do not replace the existing
     * ones.
     * @param additions The additional characters to add to the index, such as A-Z.
     * @return this, for chaining
     * @draft ICU 4.6
     */
     AlphabeticIndex &addLabels(const UnicodeSet &additions, UErrorCode &status);

    /**
     * Add the index characters from a Locale to the index.  The labels
     * are added to those that are already in the index; they do not replace the
     * existing index characters.  The collation order for this index is not
     * changed; it remains that of the locale that was originally specified
     * when creating this Index.
     *
     * @param locale The locale whose index characters are to be added.
     * @param status Error code, will be set with the reason if the construction
     *               of the AlphabeticIndex object fails.
     * @return this, for chaining
     * @draft ICU 4.6
     */
     AlphabeticIndex &addLabels(const Locale &locale, UErrorCode &status);

     /**
      * Destructor
      */
     virtual ~AlphabeticIndex();


    /**
     * Get the Collator that establishes the ordering of the items in this index.
     * Ownership of the collator remains with the AlphabeticIndex instance.
     *
     * The returned collator is a reference to the internal collator used by this
     * index.  It may be safely used to compare the names of items or to get
     * sort keys for names.  However if any settings need to be changed,
     * or other non-const methods called, a cloned copy must be made first.
     *
     * @return The collator
     * @draft ICU 4.6
     */
    virtual const RuleBasedCollator &getCollator() const;


   /**
     * Get the default label used for abbreviated buckets <i>between</i> other index characters.
     * For example, consider the index characters for Latin and Greek are used:
     *     X Y Z ... &#x0391; &#x0392; &#x0393;.
     *
     * @return inflow label
     * @draft ICU 4.6
     */
    virtual const UnicodeString &getInflowLabel() const;

   /**
     * Set the default label used for abbreviated buckets <i>between</i> other index characters.
     * An inflow label will be automatically inserted if two otherwise-adjacent label characters
     * are from different scripts, e.g. Latin and Cyrillic, and a third script, e.g. Greek,
     * sorts between the two.  The default inflow character is an ellipsis (...)
     *
     * @param inflowLabel the new Inflow label.
     * @param status Error code, will be set with the reason if the operation fails.
     * @return this
     * @draft ICU 4.6
     */
    virtual AlphabeticIndex &setInflowLabel(const UnicodeString &inflowLabel, UErrorCode &status);



   /**
     * Get the label used for items that sort after the last normal index character,
     * and that would not otherwise have an appropriate label.
     *
     * @return overflow label
     * @draft ICU 4.6
     */
    virtual const UnicodeString &getOverflowLabel() const;


   /**
     * Set the label used for items that sort after the last normal index character,
     * and that would not otherwise have an appropriate label.
     *
     * @param overflowLabel the new overflow label.
     * @param status Error code, will be set with the reason if the operation fails.
     * @return this
     * @draft ICU 4.6
     */
    virtual AlphabeticIndex &setOverflowLabel(const UnicodeString &overflowLabel, UErrorCode &status);

   /**
     * Get the label used for items that sort before the first normal index character,
     * and that would not otherwise have an appropriate label.
     *
     * @return underflow label
     * @draft ICU 4.6
     */
    virtual const UnicodeString &getUnderflowLabel() const;

   /**
     * Set the label used for items that sort before the first normal index character,
     * and that would not otherwise have an appropriate label.
     *
     * @param underflowLabel the new underflow label.
     * @param status Error code, will be set with the reason if the operation fails.
     * @return this
     * @draft ICU 4.6
     */
    virtual AlphabeticIndex &setUnderflowLabel(const UnicodeString &underflowLabel, UErrorCode &status);


    /**
     * Get the limit on the number of labels permitted in the index.
     * The number does not include over, under and inflow labels.
     *
     * @return maxLabelCount maximum number of labels.
     * @draft ICU 4.6
     */
    virtual int32_t getMaxLabelCount() const;

    /**
     * Set a limit on the number of labels permitted in the index.
     * The number does not include over, under and inflow labels.
     * Currently, if the number is exceeded, then every
     * nth item is removed to bring the count down.
     * A more sophisticated mechanism may be available in the future.
     *
     * @param maxLabelCount the maximum number of labels.
     * @return This, for chaining
     * @draft ICU 4.6
     */
    virtual AlphabeticIndex &setMaxLabelCount(int32_t maxLabelCount, UErrorCode &status);


    /**
     * Get the Unicode character (or tailored string) that defines an overflow bucket;
     * that is anything greater than or equal to that string should go in that bucket,
     * instead of with the last character. Normally that is the first character of the script
     * after lowerLimit. Thus in X Y Z ... <i>Devanagari-ka</i>, the overflow character for Z
     * would be the <i>Greek-alpha</i>.
     *
     * @param lowerLimit   The character below the overflow (or inflow) bucket
     * @return string that defines top of the overflow buck for lowerLimit, or null if there is none
     * @internal
     */
    virtual const UnicodeString &getOverflowComparisonString(const UnicodeString &lowerLimit,
                                                             UErrorCode &status);


    /**
     * Add an record to the index.  Each record will be associated with an index Bucket
     *  based on the record's name.  The list of records for each bucket will be sorted
     *  based on the collation ordering of the names in the index's locale.
     *  Records with duplicate names are permitted; they will be kept in the order
     *  that they were added.
     *
     * @param name The display name for the Record.  The Record will be placed in
     *             a bucket based on this name.
     * @param data An optional pointer to user data associated with this
     *             item.  When iterating the contents of a bucket, both the
     *             data pointer the name will be available for each Record.
     * @param status  Error code, will be set with the reason if the operation fails.
     * @return        This, for chaining.
     * @draft ICU 4.6
     */
    virtual AlphabeticIndex &addRecord(const UnicodeString &name, const void *data, UErrorCode &status);

    /**
     * Remove all Records from the Index.  The set of Buckets, which define the headings under
     * which records are classified, is not altered.
     *
     * @param status  Error code, will be set with the reason if the operation fails.
     * @return        This, for chaining.
     * @draft ICU 4.6
     */
    virtual AlphabeticIndex &clearRecords(UErrorCode &status);


    /**  Get the number of labels in this index.
     *      Note: may trigger lazy index construction.
     *
     * @param status  Error code, will be set with the reason if the operation fails.
     * @return        The number of labels in this index, including any under, over or
     *                in-flow labels.
     * @draft ICU 4.6
     */
    virtual int32_t  getBucketCount(UErrorCode &status);


    /**  Get the total number of Records in this index, that is, the number
     *   of <name, data> pairs added.
     *
     * @param status  Error code, will be set with the reason if the operation fails.
     * @return        The number of items in this index, including any under, over or
     *                in-flow labels.
     * @draft ICU 4.6
     */
    virtual int32_t  getRecordCount(UErrorCode &status);



    /**
     *   Given the name of a record, return the zero-based index of the Bucket
     *   in which the item should appear.  The name need not be in the index.
     *   A Record will not be added to the index by this function.
     *   Bucket numbers are zero-based, in Bucket iteration order.
     *
     * @param name  The name whose bucket position in the index is to be determined.
     * @param status  Error code, will be set with the reason if the operation fails.
     * @return The bucket number for this name.
     * @draft ICU 4.6
     *
     */
    virtual int32_t  getBucketIndex(const UnicodeString &itemName, UErrorCode &status);


    /**
     *   Get the zero based index of the current Bucket from an iteration
     *   over the Buckets of this index.  Return -1 if no iteration is in process.
     *   @return  the index of the current Bucket
     *   @draft ICU 4.6
     */
    virtual int32_t  getBucketIndex() const;


    /**
     *   Advance the iteration over the Buckets of this index.  Return FALSE if
     *   there are no more Buckets.
     *
     *   @param status  Error code, will be set with the reason if the operation fails.
     *   U_ENUM_OUT_OF_SYNC_ERROR will be reported if the index is modified while
     *   an enumeration of its contents are in process.
     *   @draft ICU 4.6
     */
    virtual UBool nextBucket(UErrorCode &status);

    /**
     *   Return the name of the Label of the current bucket.
     *   If the iteration is before the first Bucket (nextBucket() has not been called),
     *   or after the last, return an empty string.
     */
    virtual const UnicodeString &getBucketLabel() const;

    /**
     *  Return the type of the label for the current Bucket (selected by the
     *  iteration over Buckets.)
     *
     * @return the label type.
     * @draft ICU 4.6
     */
    virtual UAlphabeticIndexLabelType getBucketLabelType() const;

    /**
      * Get the number of <name, data> Records in the current Bucket.
      * If the current bucket iteration position is before the first label or after the
      * last, return 0.
      *
      *  @return the number of Records.
      *  @draft ICU 4.6
      */
    virtual int32_t getBucketRecordCount() const;


    /**
     *  Reset the Bucket iteration for this index.  The next call to nextBucket()
     *  will restart the iteration at the first label.
     *
     * @param status  Error code, will be set with the reason if the operation fails.
     * @return        this, for chaining.
     * @draft ICU 4.6
     */
    virtual AlphabeticIndex &resetBucketIterator(UErrorCode &status);

    /**
     * Advance to the next record in the current Bucket.
     * When nextBucket() is called, Record iteration is reset to just before the
     * first Record in the new Bucket.
     *
     *   @param status  Error code, will be set with the reason if the operation fails.
     *   U_ENUM_OUT_OF_SYNC_ERROR will be reported if the index is modified while
     *   an enumeration of its contents are in process.
     *   @return TRUE if successful, FALSE when the iteration advances past the last item.
     *   @draft ICU 4.6
     */
    virtual UBool nextRecord(UErrorCode &status);

    /**
     * Get the name of the current Record.
     * Return an empty string if the Record iteration position is before first
     * or after the last.
     *
     *  @return The name of the current index item.
     *  @draft ICU 4.6
     */
    virtual const UnicodeString &getRecordName() const;


    /**
     * Return the data pointer of the Record currently being iterated over.
     * Return NULL if the current iteration position before the first item in this Bucket,
     * or after the last.
     *
     *  @return The current Record's data pointer.
     *  @draft ICU 4.6
     */
    virtual const void *getRecordData() const;


    /**
     * Reset the Record iterator position to before the first Record in the current Bucket.
     *
     *  @return This, for chaining.
     *  @draft ICU 4.6
     */
    virtual AlphabeticIndex &resetRecordIterator();

private:
    // No ICU "poor man's RTTI" for this class nor its subclasses.
    virtual UClassID getDynamicClassID() const;

     /**
      * No Copy constructor.
      * @internal
      */
     AlphabeticIndex(const AlphabeticIndex &other);

     /**
      *   No assignment.
      */
     AlphabeticIndex &operator =(const AlphabeticIndex & /*other*/) { return *this;};

    /**
     * No Equality operators.
     * @internal
     */
     virtual UBool operator==(const AlphabeticIndex& other) const;

    /**
     * Inequality operator.
     * @internal
     */
     virtual UBool operator!=(const AlphabeticIndex& other) const;

     // Common initialization, for use from all constructors.
     void init(UErrorCode &status);

     // Initialize & destruct static constants used by this class.
     static void staticInit(UErrorCode &status);

   public:
     /**
      *   Delete all shared (static) data associated with an AlphabeticIndex.
      *   Internal function, not intended for direct use.
      *   @internal.
      */
     static void staticCleanup();
   private:

     // Add index characters from the specified locale to the dest set.
     // Does not remove any previous contents from dest.
     static void getIndexExemplars(UnicodeSet &dest, const Locale &locale, UErrorCode &status);

     static UVector *firstStringsInScript(Collator *coll, UErrorCode &status);
     static UVector *hackFirstStringsInScript(Collator *coll, UErrorCode &status);

     static UnicodeString separated(const UnicodeString &item);

     static UnicodeSet *getScriptSet(UnicodeSet &dest, const UnicodeString &codePoint, UErrorCode &status);

     void buildIndex(UErrorCode &status);
     void buildBucketList(UErrorCode &status);
     void bucketItems(UErrorCode &status);


  public:

    //  The following internal items are declared public only to allow access from
    //  implementation code written in plain C.  They are not intended for
    //  public use.

    /**
     * A record, or item, in the index.
     * @internal
     */
     struct Record: public UMemory {
         const UnicodeString  *name_;
         const void           *data_;
         int32_t              serialNumber_;
         ~Record() {delete name_;};
     };

     /**
       * Holds all user records before they are distributed into buckets.
       * Type of contents is (Record *)
       * @internal
       */
     UVector  *inputRecords_;

     /**
      *   A Bucket holds an index label and references to everything belonging to that label.
      *   For implementation use only.  Declared public because pure C implementation code needs access.
      *   @internal
      */
     struct Bucket: public UMemory {
         UnicodeString     label_;
         UnicodeString     lowerBoundary_;
         UAlphabeticIndexLabelType labelType_;
         UVector           *records_;  // Records are owned by inputRecords_ vector.

         Bucket(const UnicodeString &label,         // Parameter strings are copied.
                const UnicodeString &lowerBoundary,
                UAlphabeticIndexLabelType type, UErrorCode &status);
         ~Bucket();
     };

   private:

     // Holds the contents of this index, buckets of user items.
     // UVector elements are of type (Bucket *)
     UVector *bucketList_;

     int32_t  labelsIterIndex_;      // Index of next item to return.
     int32_t  itemsIterIndex_;
     Bucket   *currentBucket_;       // While an iteration of the index in underway,
                                     //   point to the bucket for the current label.
                                     // NULL when no iteration underway.

     UBool    indexBuildRequired_;   //  Caller has made changes to the index that
                                     //  require rebuilding & bucketing before the
                                     //  contents can be iterated.

     int32_t    maxLabelCount_;      // Limit on # of labels permitted in the index.

     UHashtable *alreadyIn_;         // Key=UnicodeString, value=UnicodeSet

     UnicodeSet *rawIndexChars_;     // Set of index characters.  Union
                                     //   of those explicitly set by the user plus
                                     //   those from locales.  Raw values, before
                                     //   crunching into bucket labels.

     UVector    *indexCharacters_;   // Set of index characters, after processing, sorting.

     UnicodeSet *noDistinctSorting_;
     UnicodeSet *notAlphabetic_;
     UVector    *firstScriptCharacters_;  // The first character from each script,
                                          //   in collation order.

     Locale    locale_;
     Collator  *comparator_;
     Collator  *comparatorPrimary_;

     UnicodeString  inflowLabel_;
     UnicodeString  overflowLabel_;
     UnicodeString  underflowLabel_;
     UnicodeString  overflowComparisonString_;

     int32_t    recordCounter_;         // Counts Records created.  For minting record serial numbers.

// Constants.  Lazily initialized the first time an AlphabeticIndex object is created.

     static UnicodeSet *ALPHABETIC;
     static UnicodeSet *CORE_LATIN;
     static UnicodeSet *ETHIOPIC;
     static UnicodeSet *HANGUL;
     static UnicodeSet *IGNORE_SCRIPTS;
     static UnicodeSet *TO_TRY;
     static const UnicodeString *EMPTY_STRING;

};

U_NAMESPACE_END
#endif

