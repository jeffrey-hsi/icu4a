/*  
 **********************************************************************
 *   Copyright (C) 2002-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 *  file name:  dicttrieperf.cpp
 *  encoding:   US-ASCII
 *  tab size:   8 (not used)
 *  indentation:4
 *
 *  created on: 2010dec09
 *  created by: Markus W. Scherer
 *
 *  Performance test program for dictionary-type tries.
 *
 * Usage from within <ICU build tree>/test/perf/dicttrieperf/ :
 * (Linux)
 *  make
 *  export LD_LIBRARY_PATH=../../../lib:../../../stubdata:../../../tools/ctestfw
 *  ./dicttrieperf --sourcedir <ICU build tree>/data/out/tmp --passes 3 --iterations 1000
 * or
 *  ./dicttrieperf -f <ICU source tree>/source/data/brkitr/thaidict.txt --passes 3 --iterations 250
 */

#include <stdio.h>
#include <stdlib.h>
#include "unicode/uperf.h"
#include "unicode/utext.h"
#include "bytetrie.h"
#include "bytetriebuilder.h"
#include "charstr.h"
#include "package.h"
#include "toolutil.h"
#include "triedict.h"
#include "ucbuf.h"  // struct ULine
#include "uchartrie.h"
#include "uchartriebuilder.h"
#include "uoptions.h"
#include "uvectr32.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

// Test object.
class DictionaryTriePerfTest : public UPerfTest {
public:
    DictionaryTriePerfTest(int32_t argc, const char *argv[], UErrorCode &status)
            : UPerfTest(argc, argv, NULL, 0, "", status), numTextLines(0) {
        if(hasFile()) {
            getLines(status);
            for(int32_t i=0; i<numLines; ++i) {
                // Skip comment lines (start with a character below 'A').
                if(lines[i].name[0]>=0x41) {
                    ++numTextLines;
                    // Remove trailing CR LF.
                    int32_t len=lines[i].len;
                    UChar c;
                    while(len>0 && ((c=lines[i].name[len-1])==0xa || c==0xd)) {
                        --len;
                    }
                    lines[i].len=len;
                }
            }
        }
    }

    virtual UPerfFunction *runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

    const char *getSourceDir() const { return sourceDir; }

    UBool hasFile() const { return ucharBuf!=NULL; }
    const ULine *getCachedLines() const { return lines; }
    int32_t getNumLines() const { return numLines; }
    int32_t numTextLines;  // excluding comment lines
};

// Performance test function object.
// Loads icudt46l.dat (or whatever its current versioned filename)
// from the -s or --sourcedir path.
class PackageLookup : public UPerfFunction {
protected:
    PackageLookup(const DictionaryTriePerfTest &perf) {
        IcuToolErrorCode errorCode("PackageLookup()");
        CharString filename(perf.getSourceDir(), errorCode);
        int32_t filenameLength=filename.length();
        if(filenameLength>0 && filename[filenameLength-1]!=U_FILE_SEP_CHAR &&
                               filename[filenameLength-1]!=U_FILE_ALT_SEP_CHAR) {
            filename.append(U_FILE_SEP_CHAR, errorCode);
        }
        filename.append(U_ICUDATA_NAME, errorCode);
        filename.append(".dat", errorCode);
        pkg.readPackage(filename.data());
    }

public:
    virtual ~PackageLookup() {}

    // virtual void call(UErrorCode* pErrorCode) { ... }

    virtual long getOperationsPerIteration() {
        return pkg.getItemCount();
    }

    // virtual long getEventsPerIteration();

protected:
    Package pkg;
};

struct TOCEntry {
    int32_t nameOffset, dataOffset;
};

// Similar to ICU 4.6 offsetTOCLookupFn() (in ucmndata.c).
static int32_t simpleBinarySearch(const char *s, const char *names, const TOCEntry *toc, int32_t count) {
    int32_t start=0;
    int32_t limit=count;
    int32_t lastNumber=limit;
    for(;;) {
        int32_t number=(start+limit)/2;
        if(lastNumber==number) {  // have we moved?
            return -1;  // not found
        }
        lastNumber=number;
        int32_t cmp=strcmp(s, names+toc[number].nameOffset);
        if(cmp<0) {
            limit=number;
        } else if(cmp>0) {
            start=number;
        } else {  // found s
            return number;
        }
    }
}

class BinarySearchPackageLookup : public PackageLookup {
public:
    BinarySearchPackageLookup(const DictionaryTriePerfTest &perf)
            : PackageLookup(perf) {
        IcuToolErrorCode errorCode("BinarySearchPackageLookup()");
        int32_t count=pkg.getItemCount();
        toc=new TOCEntry[count];
        for(int32_t i=0; i<count; ++i) {
            toc[i].nameOffset=itemNames.length();
            toc[i].dataOffset=i;  // arbitrary value, see toc comment below
            // The Package class removes the "icudt46l/" prefix.
            // We restore that here for a fair performance test.
            const char *name=pkg.getItem(i)->name;
            itemNames.append("icudt46l/", errorCode);
            itemNames.append(name, strlen(name)+1, errorCode);
        }
        printf("size of item names: %6ld\n", (long)itemNames.length());
        printf("size of TOC:        %6ld\n", (long)(count*8));
        printf("total index size:   %6ld\n", (long)(itemNames.length()+count*8));
    }
    virtual ~BinarySearchPackageLookup() {
        delete[] toc;
    }

    virtual void call(UErrorCode * /*pErrorCode*/) {
        int32_t count=pkg.getItemCount();
        const char *itemNameChars=itemNames.data();
        const char *name=itemNameChars;
        for(int32_t i=0; i<count; ++i) {
            if(simpleBinarySearch(name, itemNameChars, toc, count)<0) {
                fprintf(stderr, "item not found: %s\n", name);
            }
            name=strchr(name, 0)+1;
        }
    }

protected:
    CharString itemNames;
    // toc imitates a .dat file's array of UDataOffsetTOCEntry
    // with nameOffset and dataOffset.
    // We don't need the dataOffsets, but we want to imitate the real
    // memory density, to measure equivalent CPU cache usage.
    TOCEntry *toc;
};

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

// Compare strings where we know the shared prefix length,
// and advance the prefix length as we find that the strings share even more characters.
static int32_t strcmpAfterPrefix(const char *s1, const char *s2, int32_t &prefixLength) {
    int32_t pl=prefixLength;
    s1+=pl;
    s2+=pl;
    int32_t cmp=0;
    for(;;) {
        int32_t c1=(uint8_t)*s1++;
        int32_t c2=(uint8_t)*s2++;
        cmp=c1-c2;
        if(cmp!=0 || c1==0) {  // different or done
            break;
        }
        ++pl;  // increment shared same-prefix length
    }
    prefixLength=pl;
    return cmp;
}

static int32_t prefixBinarySearch(const char *s, const char *names, const TOCEntry *toc, int32_t count) {
    if(count==0) {
        return -1;
    }
    int32_t start=0;
    int32_t limit=count;
    // Remember the shared prefix between s, start and limit,
    // and don't compare that shared prefix again.
    // The shared prefix should get longer as we narrow the [start, limit[ range.
    int32_t startPrefixLength=0;
    int32_t limitPrefixLength=0;
    // Prime the prefix lengths so that we don't keep prefixLength at 0 until
    // both the start and limit indexes have moved.
    // At the same time, we find if s is one of the start and (limit-1) names,
    // and if not, exclude them from the actual binary search.
    if(0==strcmpAfterPrefix(s, names+toc[0].nameOffset, startPrefixLength)) {
        return 0;
    }
    ++start;
    --limit;
    if(0==strcmpAfterPrefix(s, names+toc[limit].nameOffset, limitPrefixLength)) {
        return limit;
    }
    while(start<limit) {
        int32_t i=(start+limit)/2;
        int32_t prefixLength=MIN(startPrefixLength, limitPrefixLength);
        int32_t cmp=strcmpAfterPrefix(s, names+toc[i].nameOffset, prefixLength);
        if(cmp<0) {
            limit=i;
            limitPrefixLength=prefixLength;
        } else if(cmp==0) {
            return i;
        } else {
            start=i;
            startPrefixLength=prefixLength;
        }
    }
    return -1;
}

class PrefixBinarySearchPackageLookup : public BinarySearchPackageLookup {
public:
    PrefixBinarySearchPackageLookup(const DictionaryTriePerfTest &perf)
            : BinarySearchPackageLookup(perf) {}

    virtual void call(UErrorCode * /*pErrorCode*/) {
        int32_t count=pkg.getItemCount();
        const char *itemNameChars=itemNames.data();
        const char *name=itemNameChars;
        for(int32_t i=0; i<count; ++i) {
            if(prefixBinarySearch(name, itemNameChars, toc, count)<0) {
                fprintf(stderr, "item not found: %s\n", name);
            }
            name=strchr(name, 0)+1;
        }
    }
};

static int32_t byteTrieLookup(const char *s, const char *nameTrieBytes) {
    ByteTrie trie(nameTrieBytes);
    if(trie.next(s, -1) && trie.hasValue()) {
        return trie.getValue();
    } else {
        return -1;
    }
}

class ByteTriePackageLookup : public PackageLookup {
public:
    ByteTriePackageLookup(const DictionaryTriePerfTest &perf)
            : PackageLookup(perf) {
        IcuToolErrorCode errorCode("BinarySearchPackageLookup()");
        int32_t count=pkg.getItemCount();
        for(int32_t i=0; i<count; ++i) {
            // The Package class removes the "icudt46l/" prefix.
            // We restore that here for a fair performance test.
            // We store all full names so that we do not have to reconstruct them
            // in the call() function.
            const char *name=pkg.getItem(i)->name;
            int32_t offset=itemNames.length();
            itemNames.append("icudt46l/", errorCode);
            itemNames.append(name, -1, errorCode);
            // As value, set the data item index.
            // In a real implementation, we would use that to get the
            // start and limit offset of the data item.
            StringPiece fullName(itemNames.toStringPiece());
            fullName.remove_prefix(offset);
            builder.add(fullName, i, errorCode);
            // NUL-terminate the name for call() to find the next one.
            itemNames.append(0, errorCode);
        }
        int32_t length=builder.build(errorCode).length();
        printf("size of ByteTrie:   %6ld\n", (long)length);
        // count+1: +1 for the last-item limit offset which we should have always had
        printf("size of dataOffsets:%6ld\n", (long)((count+1)*4));
        printf("total index size:   %6ld\n", (long)(length+(count+1)*4));
    }
    virtual ~ByteTriePackageLookup() {}

    virtual void call(UErrorCode *pErrorCode) {
        int32_t count=pkg.getItemCount();
        const char *nameTrieBytes=builder.build(*pErrorCode).data();
        const char *name=itemNames.data();
        for(int32_t i=0; i<count; ++i) {
            if(byteTrieLookup(name, nameTrieBytes)<0) {
                fprintf(stderr, "item not found: %s\n", name);
            }
            name=strchr(name, 0)+1;
        }
    }

protected:
    ByteTrieBuilder builder;
    CharString itemNames;
};

// Performance test function object.
// Each subclass loads a dictionary text file
// from the -s or --sourcedir path plus -f or --file-name.
// For example, <ICU source dir>/source/data/brkitr/thaidict.txt.
class DictLookup : public UPerfFunction {
public:
    DictLookup(const DictionaryTriePerfTest &perfTest) : perf(perfTest) {}

    virtual long getOperationsPerIteration() {
        return perf.numTextLines;
    }

protected:
    const DictionaryTriePerfTest &perf;
};

class CompactTrieDictLookup : public DictLookup {
public:
    CompactTrieDictLookup(const DictionaryTriePerfTest &perfTest)
            : DictLookup(perfTest), ctd(NULL) {
        IcuToolErrorCode errorCode("UCharTrieDictLookup()");
        // U+0E1C is the median code unit, from
        // the UCharTrie root node (split-branch node) for thaidict.txt.
        MutableTrieDictionary builder(0xe1c, errorCode);
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            builder.addWord(lines[i].name, lines[i].len, errorCode);
        }
        ctd=new CompactTrieDictionary(builder, errorCode);
        int32_t length=(int32_t)ctd->dataSize();
        printf("size of CompactTrieDict:    %6ld bytes\n", (long)length*2);
    }

    virtual ~CompactTrieDictLookup() {
        delete ctd;
    }

    virtual void call(UErrorCode *pErrorCode) {
        UText text=UTEXT_INITIALIZER;
        int32_t lengths[20];
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            utext_openUChars(&text, lines[i].name, lines[i].len, pErrorCode);
            int32_t count;
            ctd->matches(&text, lines[i].len,
                         lengths, count, LENGTHOF(lengths));
            if(count==0 || lengths[count-1]!=lines[i].len) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }

protected:
    CompactTrieDictionary *ctd;
};

// Closely imitate CompactTrieDictionary::matches().
// Note: CompactTrieDictionary::matches() is part of its trie implementation,
// and while it loops over the text, it knows the current state.
// By contrast, this implementation uses UCharTrie API functions that have to
// check the trie state each time and load/store state in the object.
// (Whether it hasNext() and whether it is in the middle of a linear-match node.)
static int32_t
ucharTrieMatches(UCharTrie &trie,
                 UText *text, int32_t textLimit,
                 int32_t *lengths, int &count, int limit ) {
    UChar32 c=utext_next32(text);
    // Notes:
    // a) CompactTrieDictionary::matches() does not check for U_SENTINEL.
    // b) It also ignores non-BMP code points by casting to UChar!
    if(c<0) {
        return 0;
    }
    // Should be firstForCodePoint() but CompactTrieDictionary
    // handles only code units.
    UCharTrie::Result result=trie.first(c, TRUE);
    if(result==UCharTrie::NO_MATCH) {
        return 1;
    }
    int32_t numChars=1;
    count=0;
    for(;;) {
        if(result==UCharTrie::HAS_VALUE) {
            if(count<limit) {
                // lengths[count++]=(int32_t)utext_getNativeIndex(text);
                lengths[count++]=numChars;  // CompactTrieDictionary just counts chars too.
            }
        }
        if(numChars>=textLimit) {
            // Note: Why do we have both a text limit and a UText that knows its length?
            break;
        }
        UChar32 c=utext_next32(text);
        // Notes:
        // a) CompactTrieDictionary::matches() does not check for U_SENTINEL.
        // b) It also ignores non-BMP code points by casting to UChar!
        if(c<0) {
            break;
        }
        ++numChars;
        // Should be nextForCodePoint() but CompactTrieDictionary
        // handles only code units.
        result=trie.next(c, TRUE);
        if(result==UCharTrie::NO_MATCH) {
            break;
        }
    }
#if 0
    // Note: CompactTrieDictionary::matches() comments say that it leaves the UText
    // after the longest prefix match and returns the number of characters
    // that were matched.
    if(index!=lastMatch) {
        utext_setNativeIndex(text, lastMatch);
    }
    return lastMatch-start;
    // However, it does not do either of these, so I am not trying to
    // imitate it (or its docs) 100%.
#endif
    return numChars;
}

class UCharTrieDictLookup : public DictLookup {
public:
    UCharTrieDictLookup(const DictionaryTriePerfTest &perfTest)
            : DictLookup(perfTest) {
        IcuToolErrorCode errorCode("UCharTrieDictLookup()");
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            builder.add(UnicodeString(FALSE, lines[i].name, lines[i].len), 0, errorCode);
        }
        int32_t length=builder.build(errorCode).length();
        printf("size of UCharTrie:          %6ld bytes\n", (long)length*2);
    }

    virtual ~UCharTrieDictLookup() {}

protected:
    UCharTrieBuilder builder;
};

class UCharTrieDictMatches : public UCharTrieDictLookup {
public:
    UCharTrieDictMatches(const DictionaryTriePerfTest &perfTest)
            : UCharTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode *pErrorCode) {
        UnicodeString uchars(builder.build(*pErrorCode));
        UCharTrie trie(uchars.getBuffer());
        UText text=UTEXT_INITIALIZER;
        int32_t lengths[20];
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            utext_openUChars(&text, lines[i].name, lines[i].len, pErrorCode);
            int32_t count=0;
            ucharTrieMatches(trie, &text, lines[i].len,
                             lengths, count, LENGTHOF(lengths));
            if(count==0 || lengths[count-1]!=lines[i].len) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

class UCharTrieDictContains : public UCharTrieDictLookup {
public:
    UCharTrieDictContains(const DictionaryTriePerfTest &perfTest)
            : UCharTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode *pErrorCode) {
        UnicodeString uchars(builder.build(*pErrorCode));
        UCharTrie trie(uchars.getBuffer());
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            if(!trie.reset().next(lines[i].name, lines[i].len) || !trie.hasValue()) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

static inline int32_t thaiCharToByte(UChar32 c) {
    if(0xe00<=c && c<=0xefe) {
        return c&0xff;
    } else if(c==0x2e) {
        return 0xff;
    } else {
        return -1;
    }
}

static UBool thaiWordToBytes(const UChar *s, int32_t length,
                             CharString &str, UErrorCode &errorCode) {
    for(int32_t i=0; i<length; ++i) {
        UChar c=s[i];
        int32_t b=thaiCharToByte(c);
        if(b>=0) {
            str.append((char)b, errorCode);
        } else {
            fprintf(stderr, "thaiWordToBytes(): unable to encode U+%04X as a byte\n", c);
            return FALSE;
        }
    }
    return TRUE;
}

class ByteTrieDictLookup : public DictLookup {
public:
    ByteTrieDictLookup(const DictionaryTriePerfTest &perfTest)
            : DictLookup(perfTest), noDict(FALSE) {
        IcuToolErrorCode errorCode("ByteTrieDictLookup()");
        CharString str;
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            if(!thaiWordToBytes(lines[i].name, lines[i].len, str.clear(), errorCode)) {
                fprintf(stderr, "thaiWordToBytes(): failed for word %ld (0-based)\n", (long)i);
                noDict=TRUE;
                break;
            }
            builder.add(str.toStringPiece(), 0, errorCode);
        }
        if(!noDict) {
            int32_t length=builder.build(errorCode).length();
            printf("size of ByteTrie:           %6ld bytes\n", (long)length);
        }
    }

    virtual ~ByteTrieDictLookup() {}

protected:
    ByteTrieBuilder builder;
    UBool noDict;
};

static int32_t
byteTrieMatches(ByteTrie &trie,
                UText *text, int32_t textLimit,
                int32_t *lengths, int &count, int limit ) {
    UChar32 c=utext_next32(text);
    if(c<0) {
        return 0;
    }
    if(!trie.first(thaiCharToByte(c))) {
        return 1;
    }
    int32_t numChars=1;
    count=0;
    for(;;) {
        if(trie.hasValue()) {
            if(count<limit) {
                // lengths[count++]=(int32_t)utext_getNativeIndex(text);
                lengths[count++]=numChars;  // CompactTrieDictionary just counts chars too.
            }
        }
        if(numChars>=textLimit) {
            break;
        }
        UChar32 c=utext_next32(text);
        if(c<0) {
            break;
        }
        ++numChars;
        if(!trie.next(thaiCharToByte(c))) {
            break;
        }
    }
    return numChars;
}

class ByteTrieDictMatches : public ByteTrieDictLookup {
public:
    ByteTrieDictMatches(const DictionaryTriePerfTest &perfTest)
            : ByteTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode *pErrorCode) {
        if(noDict) {
            return;
        }
        ByteTrie trie(builder.build(*pErrorCode).data());
        UText text=UTEXT_INITIALIZER;
        int32_t lengths[20];
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            utext_openUChars(&text, lines[i].name, lines[i].len, pErrorCode);
            int32_t count=0;
            byteTrieMatches(trie, &text, lines[i].len,
                            lengths, count, LENGTHOF(lengths));
            if(count==0 || lengths[count-1]!=lines[i].len) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

class ByteTrieDictContains : public ByteTrieDictLookup {
public:
    ByteTrieDictContains(const DictionaryTriePerfTest &perfTest)
            : ByteTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode *pErrorCode) {
        if(noDict) {
            return;
        }
        ByteTrie trie(builder.build(*pErrorCode).data());
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            // Skip comment lines (start with a character below 'A').
            if(lines[i].name[0]<0x41) {
                continue;
            }
            trie.reset();
            const UChar *line=lines[i].name;
            int32_t lineLength=lines[i].len;
            for(int32_t j=0; j<lineLength; ++j) {
                if(!trie.next(thaiCharToByte(line[j]))) {
                    fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
                }
            }
            if(!trie.hasValue()) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

UPerfFunction *DictionaryTriePerfTest::runIndexedTest(int32_t index, UBool exec,
                                                      const char *&name, char * /*par*/) {
    if(hasFile()) {
        switch(index) {
        case 0:
            name="compacttriematches";
            if(exec) {
                return new CompactTrieDictLookup(*this);
            }
            break;
        case 1:
            name="uchartriematches";
            if(exec) {
                return new UCharTrieDictMatches(*this);
            }
            break;
        case 2:
            name="uchartriecontains";
            if(exec) {
                return new UCharTrieDictContains(*this);
            }
            break;
        case 3:
            name="bytetriematches";
            if(exec) {
                return new ByteTrieDictMatches(*this);
            }
            break;
        case 4:
            name="bytetriecontains";
            if(exec) {
                return new ByteTrieDictContains(*this);
            }
            break;
        default:
            name="";
            break;
        }
    } else {
        if(index==0 && exec) {
            puts("Running ByteTrie perf tests on the .dat package file from the --sourcedir.\n"
                 "For UCharTrie perf tests on a dictionary text file, specify the -f or --file-name.\n");
        }
        switch(index) {
        case 0:
            name="simplebinarysearch";
            if(exec) {
                return new BinarySearchPackageLookup(*this);
            }
            break;
        case 1:
            name="prefixbinarysearch";
            if(exec) {
                return new PrefixBinarySearchPackageLookup(*this);
            }
            break;
        case 2:
            name="bytetrie";
            if(exec) {
                return new ByteTriePackageLookup(*this);
            }
            break;
        default:
            name="";
            break;
        }
    }
    return NULL;
}

int main(int argc, const char *argv[]) {
    IcuToolErrorCode errorCode("dicttrieperf main()");
    DictionaryTriePerfTest test(argc, argv, errorCode);
    if(errorCode.isFailure()) {
        fprintf(stderr, "DictionaryTriePerfTest() failed: %s\n", errorCode.errorName());
        test.usage();
        return errorCode.reset();
    }
    if(!test.run()) {
        fprintf(stderr, "FAILED: Tests could not be run, please check the arguments.\n");
        return -1;
    }
    return 0;
}
