/*
 ******************************************************************************
 *   Copyright (C) 1996-2008, International Business Machines                 *
 *   Corporation and others.  All Rights Reserved.                            *
 ******************************************************************************
 */

/**
 * \file 
 * \brief C++ API: Collation data used to compute minLengthInChars.
 * \internal
 */
 
#ifndef COLL_DATA_H
#define COLL_DATA_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"

U_NAMESPACE_BEGIN

/*
 * The size of the internal buffer for the Collator's short description string.
 */
#define KEY_BUFFER_SIZE 64

 /*
  * The size of the internal CE buffer in a <code>CEList</code> object
  */
#define CELIST_BUFFER_SIZE 4

/*
 * Define this to enable the <code>CEList</code> objects to collect
 * statistics.
 */
#define INSTRUMENT_CELIST

 /*
  * The size of the initial list in a <code>StringList</code> object.
  */
#define STRING_LIST_BUFFER_SIZE 16

/*
 * Define this to enable the <code>StringList</code> objects to
 * collect statistics.
 */
#define INSTRUMENT_STRING_LIST

 /**
  * CEList
  *
  * This object holds a list of CEs generated from a particular
  * <code>UnicodeString</code>
  *
  * @internal ICU 4.0.1 technology preview
  */
class U_I18N_API CEList : public UObject
{
public:
    /**
     * Construct a <code>CEList</code> object.
     *
     * @param coll - the Collator used to collect the CEs.
     * @param string - the string for which to collect the CEs.
     *
     * @internal ICU 4.0.1 technology preview
     */
    CEList(UCollator *coll, const UnicodeString &string);

    /**
     * The destructor.
     */
    ~CEList();

    /**
     * Return the number of CEs in the list.
     *
     * @return the number of CEs in the list.
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t size() const;

    /**
     * Get a particular CE from the list.
     *
     * @param index - the index of the CE to return
     *
     * @return the CE, or <code>0</code> if <code>index</code> is out of range
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t get(int32_t index) const;

    /**
     * Check if the CEs in another <code>CEList</code> match the
     * suffix of this list starting at a give offset.
     *
     * @param offsset - the offset of the suffix
     * @param other - the other <code>CEList</code>
     *
     * @return <code>TRUE</code> if the CEs match, <code>FALSE</code> otherwise.
     *
     * @internal ICU 4.0.1 technology preview
     */
    UBool matchesAt(int32_t offset, const CEList *other) const; 

    /**
     * The index operator.
     *
     * @param index - the index
     *
     * @return a reference to the given CE in the list
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t &operator[](int32_t index) const;

    /*
     * UObject glue...
     */
    virtual UClassID getDynamicClassID() const;
    static UClassID getStaticClassID();

private:
    void add(int32_t ce);

    int32_t ceBuffer[CELIST_BUFFER_SIZE];
    int32_t *ces;
    int32_t listMax;
    int32_t listSize;

#ifdef INSTRUMENT_CELIST
    static int32_t _active;
    static int32_t _histogram[10];
#endif
};

/**
 * StringList
 *
 * This object holds a list of <code>UnicodeString</code> objects.
 *
 * @internal ICU 4.0.1 technology preview
 */
class U_I18N_API StringList : public UObject
{
public:
    /**
     * Construct an empty <code>StringList</code>
     *
     * @internal ICU 4.0.1 technology preview
     */
    StringList();

    /**
     * The destructor.
     *
     * @internal ICU 4.0.1 technology preview
     */
    ~StringList();

    /**
     * Add a string to the list.
     *
     * @param string - the string to add
     *
     * @internal ICU 4.0.1 technology preview
     */
    void add(const UnicodeString *string);

    /**
     * Add an array of Unicode code points to the list.
     *
     * @param chars - the address of the array of code points
     * @param count - the number of code points in the array
     *
     * @internal ICU 4.0.1 technology preview
     */
    void add(const UChar *chars, int32_t count);

    /**
     * Get a particular string from the list.
     *
     * @param index - the index of the string
     *
     * @return a pointer to the <code>UnicodeString</code> or <code>NULL</code> 
     *         if <code>index</code> is out of bounds.
     *
     * @internal ICU 4.0.1 technology preview
     */
    const UnicodeString *get(int32_t index) const;

    /**
     * Get the number of stings in the list.
     *
     * @return the number of strings in the list.
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t size() const;

    /*
     * the UObject glue...
     */
    virtual UClassID getDynamicClassID() const;
    static UClassID getStaticClassID();

private:
    UnicodeString *strings;
    int32_t listMax;
    int32_t listSize;

#ifdef INSTRUMENT_STRING_LIST
    static int32_t _lists;
    static int32_t _strings;
    static int32_t _histogram[101];
#endif
};

/*
 * Forward references to internal classes.
 */
class StringToCEsMap;
class CEToStringsMap;
class CollDataCache;

/**
 * CollData
 *
 * This class holds the Collator-specific data needed to
 * compute the length of the shortest string that can
 * generate a partcular list of CEs.
 *
 * @internal ICU 4.0.1 technology preview
 */
class U_I18N_API CollData : public UObject
{
public:
    /**
     * Construct a <code>CollData</code> object.
     *
     * @param collator - the collator
     *
     * @return the <code>CollData</code> object. You must call
     *         <code>close</code> when you are done using the object.
     *
     * @internal ICU 4.0.1 technology preview
     */
    static CollData *open(UCollator *collator);

    /**
     * Release a <code>CollData</code> object.
     *
     * @param collData - the object
     *
     * @internal ICU 4.0.1 technology preview
     */
    static void close(CollData *collData);

    /**
     * Get the <code>UCollator</code> object used to create this object.
     * The object returned may not be the exact object that was used to
     * create this object, but it will have the same behavior.
     */
    UCollator *getCollator() const;

    /**
     * Get a list of all the strings which generate a list
     * of CEs starting with a given CE.
     *
     * @param ce - the CE
     *
     * return a <code>StringList</code> object containing all
     *        the stirngs, or <code>NULL</code> if there are
     *        no such strings.
     *
     * @internal ICU 4.0.1 technology preview.
     */
    const StringList *getStringList(int32_t ce) const;

    /**
     * Get a list of the CEs generated by a partcular stirng.
     *
     * @param string - the string
     *
     * @return a <code>CEList</code> object containt the CEs. You
     *         must call <code>freeCEList</code> when you are finished
     *         using the <code>CEList</code>/
     *
     * @internal ICU 4.0.1 technology preview.
     */
    const CEList *getCEList(const UnicodeString *string) const;

    /**
     * Release a <code>CEList</code> returned by <code>getCEList</code>.
     *
     * @param list - the <CEList> to free.
     *
     * @internal ICU 4.0.1 technology preview
     */
    void freeCEList(const CEList *list);

    /**
     * Return the length of the shortest string that will generate
     * the given list of CEs.
     *
     * @param ces - the CEs
     * @param offset - the offset of the first CE in the list to use.
     *
     * @return the length of the shortest string.
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t minLengthInChars(const CEList *ces, int32_t offset) const;

 
    /**
     * Return the length of the shortest string that will generate
     * the given list of CEs.
     *
     * Note: the algorithm used to do this computation is recursive. To
     * limit the amount of recursion, a "history" list is used to record
     * the best answer starting at a particular offset in the list of CEs.
     * If the same offset is visited again during the recursion, the answer
     * in the history list is used.
     *
     * @param ces - the CEs
     * @param offset - the offset of the first CE in the list to use.
     * param history - the history list. Must be at least as long as
     *                 the number of cEs in the <code>CEList</code>
     *
     * @return the length of the shortest string.
     *
     * @internal ICU 4.0.1 technology preview
     */
   int32_t minLengthInChars(const CEList *ces, int32_t offset, int32_t *history) const;

   /*
    * UObject glue...
    */
    virtual UClassID getDynamicClassID() const;
    static UClassID getStaticClassID();

    /**
     * <code>CollData</code> objects are expensive to compute, and so
     * may be cached. This routine will free the cached objects and delete
     * the cache.
     *
     * WARNING: Don't call this until you are have called <code>close</code>
     * for each <code>CollData</code> object that you have used.
     *
     * @internal
     */
    static void freeCollDataCache();

private:
    friend class CollDataCache;
    friend class CollDataCacheEntry;

    CollData(UCollator *collator, char *cacheKey, int32_t cachekeyLength);
    ~CollData();

    CollData();

    static char *getCollatorKey(UCollator *collator, char *buffer, int32_t bufferLength);

    static CollDataCache *getCollDataCache();

    UCollator      *coll;
    StringToCEsMap *charsToCEList;
    CEToStringsMap *ceToCharsStartingWith;

    char keyBuffer[KEY_BUFFER_SIZE];
    char *key;

    static CollDataCache *collDataCache;
};

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_COLLATION
#endif // #ifndef COLL_DATA_H
