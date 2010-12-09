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
 */

#include <stdio.h>
#include <stdlib.h>
#include "unicode/uperf.h"
#include "bytetrie.h"
#include "bytetriebuilder.h"
#include "charstr.h"
#include "package.h"
#include "toolutil.h"
#include "uoptions.h"
#include "uvectr32.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

// Test object.
class DictionaryTriePerfTest : public UPerfTest {
public:
    DictionaryTriePerfTest(int32_t argc, const char *argv[], UErrorCode &status)
            : UPerfTest(argc, argv, NULL, 0, "", status) {
    }

    virtual UPerfFunction *runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

    const char *getSourceDir() const { return sourceDir; }
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
    if(trie.hasValue(s, -1)) {
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

UPerfFunction *DictionaryTriePerfTest::runIndexedTest(int32_t index, UBool exec,
                                                      const char *&name, char * /*par*/) {
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
