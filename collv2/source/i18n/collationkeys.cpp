/*
*******************************************************************************
* Copyright (C) 2012, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* collationkeys.cpp
*
* created on: 2012sep02
* created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/bytestream.h"
#include "collation.h"
#include "collationdata.h"
#include "collationiterator.h"
#include "collationkeys.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

SortKeyByteSink2::~SortKeyByteSink2() {}

void
SortKeyByteSink2::Append(const char *bytes, int32_t n) {
    if (n <= 0 || bytes == NULL) {
        return;
    }
    if (ignore_ > 0) {
        int32_t ignoreRest = ignore_ - n;
        if (ignoreRest >= 0) {
            ignore_ = ignoreRest;
            return;
        } else {
            bytes += ignore_;
            n = -ignoreRest;
            ignore_ = 0;
        }
    }
    int32_t length = appended_;
    appended_ += n;
    if ((buffer_ + length) == bytes) {
        return;  // the caller used GetAppendBuffer() and wrote the bytes already
    }
    int32_t available = capacity_ - length;
    if (n <= available) {
        uprv_memcpy(buffer_ + length, bytes, n);
    } else {
        AppendBeyondCapacity(bytes, n, length);
    }
}

char *
SortKeyByteSink2::GetAppendBuffer(int32_t min_capacity,
                                 int32_t desired_capacity_hint,
                                 char *scratch,
                                 int32_t scratch_capacity,
                                 int32_t *result_capacity) {
    if (min_capacity < 1 || scratch_capacity < min_capacity) {
        *result_capacity = 0;
        return NULL;
    }
    if (ignore_ > 0) {
        // Do not write ignored bytes right at the end of the buffer.
        *result_capacity = scratch_capacity;
        return scratch;
    }
    int32_t available = capacity_ - appended_;
    if (available >= min_capacity) {
        *result_capacity = available;
        return buffer_ + appended_;
    } else if (Resize(desired_capacity_hint, appended_)) {
        *result_capacity = capacity_ - appended_;
        return buffer_ + appended_;
    } else {
        *result_capacity = scratch_capacity;
        return scratch;
    }
}

namespace {

/**
 * uint8_t byte buffer, similar to CharString but simpler.
 */
class SortKeyLevel : public UMemory {
public:
    SortKeyLevel() : len(0), ok(TRUE) {}
    ~SortKeyLevel() {}

    /** @return FALSE if memory allocation failed */
    UBool isOk() const { return ok; }
    UBool isEmpty() const { return len == 0; }
    int32_t length() const { return len; }
    const uint8_t *data() const { return buffer.getAlias(); }
    uint8_t operator[](int32_t index) const { return buffer[index]; }

    uint8_t *data() { return buffer.getAlias(); }

    void appendByte(uint32_t b);
    void appendWeight16(uint32_t w);
    void appendWeight32(uint32_t w);
    void appendReverseWeight16(uint32_t w);

    /** Appends all but the last byte to the sink. The last byte should be the 01 terminator. */
    void appendTo(ByteSink &sink) const {
        U_ASSERT(len > 0 && buffer[len - 1] == 1);
        sink.Append(reinterpret_cast<const char *>(buffer.getAlias()), len - 1);
    }

private:
    MaybeStackArray<uint8_t, 40> buffer;
    int32_t len;
    UBool ok;

    UBool ensureCapacity(int32_t appendCapacity);

    SortKeyLevel(const SortKeyLevel &other); // forbid copying of this class
    SortKeyLevel &operator=(const SortKeyLevel &other); // forbid copying of this class
};

void SortKeyLevel::appendByte(uint32_t b) {
    if(len < buffer.getCapacity() || ensureCapacity(1)) {
        buffer[len++] = (uint8_t)b;
    }
}

void
SortKeyLevel::appendWeight16(uint32_t w) {
    U_ASSERT((w & 0xffff) != 0);
    uint8_t b0 = (uint8_t)(w >> 8);
    uint8_t b1 = (uint8_t)w;
    int32_t appendLength = (b1 == 0) ? 1 : 2;
    if((len + appendLength) <= buffer.getCapacity() || ensureCapacity(appendLength)) {
        buffer[len++] = b0;
        if(b1 != 0) {
            buffer[len++] = b1;
        }
    }
}

void
SortKeyLevel::appendWeight32(uint32_t w) {
    U_ASSERT(w != 0);
    uint8_t bytes[4] = { (uint8_t)(w >> 24), (uint8_t)(w >> 16), (uint8_t)(w >> 8), (uint8_t)w };
    int32_t appendLength = (bytes[1] == 0) ? 1 : (bytes[2] == 0) ? 2 : (bytes[3] == 0) ? 3 : 4;
    if((len + appendLength) <= buffer.getCapacity() || ensureCapacity(appendLength)) {
        buffer[len++] = bytes[0];
        if(bytes[1] != 0) {
            buffer[len++] = bytes[1];
            if(bytes[2] != 0) {
                buffer[len++] = bytes[2];
                if(bytes[3] != 0) {
                    buffer[len++] = bytes[3];
                }
            }
        }
    }
}

void
SortKeyLevel::appendReverseWeight16(uint32_t w) {
    U_ASSERT((w & 0xffff) != 0);
    uint8_t b0 = (uint8_t)(w >> 8);
    uint8_t b1 = (uint8_t)w;
    int32_t appendLength = (b1 == 0) ? 1 : 2;
    if((len + appendLength) <= buffer.getCapacity() || ensureCapacity(appendLength)) {
        if(b1 == 0) {
            buffer[len++] = b0;
        } else {
            buffer[len] = b1;
            buffer[len + 1] = b0;
            len += 2;
        }
    }
}

UBool SortKeyLevel::ensureCapacity(int32_t appendCapacity) {
    if(!ok) {
        return FALSE;
    }
    int32_t newCapacity = 2 * buffer.getCapacity();
    int32_t altCapacity = len + 2 * appendCapacity;
    if (newCapacity < altCapacity) {
        newCapacity = altCapacity;
    }
    if (newCapacity < 200) {
        newCapacity = 200;
    }
    if(buffer.resize(newCapacity, len)==NULL) {
        return ok = FALSE;
    }
    return TRUE;
}

}  // namespace

CollationKeys::LevelCallback::~LevelCallback() {}

UBool
CollationKeys::LevelCallback::needToWrite(Collation::Level /*level*/) { return TRUE; }

/**
 * Map from collation strength (UColAttributeValue)
 * to a mask of Collation::Level bits up to that strength,
 * excluding the CASE_LEVEL which is independent of the strength,
 * and excluding IDENTICAL_LEVEL which this function does not write.
 */
static const uint32_t levelMasks[UCOL_STRENGTH_LIMIT] = {
    2,          // UCOL_PRIMARY -> PRIMARY_LEVEL
    6,          // UCOL_SECONDARY -> up to SECONDARY_LEVEL
    0x16,       // UCOL_TERTIARY -> up to TERTIARY_LEVEL
    0x36,       // UCOL_QUATERNARY -> up to QUATERNARY_LEVEL
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0,
    0x36        // UCOL_IDENTICAL -> up to QUATERNARY_LEVEL
};

void
CollationKeys::writeSortKeyUpToQuaternary(CollationIterator &iter, SortKeyByteSink2 &sink,
                                          Collation::Level minLevel, LevelCallback &callback,
                                          UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }

    const CollationData *data = iter.getData();
    int32_t options = data->options;
    // Set of levels to process and write.
    uint32_t levels = levelMasks[CollationData::getStrength(options)];
    if((options & CollationData::CASE_LEVEL) != 0) {
        levels |= Collation::CASE_LEVEL_FLAG;
    }
    // Minus the levels below minLevel.
    levels &= ~(((uint32_t)1 << minLevel) - 1);
    if(levels == 0) { return; }

    uint32_t variableTop;
    if((options & CollationData::ALTERNATE_MASK) == 0) {
        variableTop = 0;
    } else {
        // +1 so that we can use "<" and primary ignorables test out early.
        variableTop = data->variableTop + 1;
    }
    const uint8_t *reorderTable = data->reorderTable;

    uint32_t tertiaryMask = CollationData::getTertiaryMask(options);

    SortKeyLevel cases;
    SortKeyLevel secondaries;
    SortKeyLevel tertiaries;
    SortKeyLevel quaternaries;

    uint32_t compressedP1 = 0;
    int32_t commonCases = 0;
    int32_t commonSecondaries = 0;
    int32_t commonTertiaries = 0;
    int32_t commonQuaternaries = 0;

    uint32_t prevSecondary = 0;
    UBool anyMergeSeparators = FALSE;

    for(;;) {
        int64_t ce = iter.nextCE(errorCode);
        uint32_t p = (uint32_t)(ce >> 32);
        uint32_t p1 = p >> 24;
        if(reorderTable != NULL) { p1 = reorderTable[p1]; }
        if(p < variableTop && p > Collation::MERGE_SEPARATOR_PRIMARY) {
            // Variable CE, shift it to quaternary level.
            // Ignore all following primary ignorables, and shift further variable CEs.
            if(commonQuaternaries != 0) {
                --commonQuaternaries;
                while(commonQuaternaries >= QUAT_COMMON_MAX_COUNT) {
                    quaternaries.appendByte(QUAT_COMMON_MIDDLE);
                    commonQuaternaries -= QUAT_COMMON_MAX_COUNT;
                }
                // Shifted primary weights are lower than the common weight.
                quaternaries.appendByte(QUAT_COMMON_LOW + commonQuaternaries);
                commonQuaternaries = 0;
            }
            do {
                if((levels & Collation::QUATERNARY_LEVEL_FLAG) != 0) {
                    if(p1 >= QUAT_SHIFTED_LIMIT_BYTE) {
                        // Prevent shifted primary lead bytes from
                        // overlapping with the common compression range.
                        quaternaries.appendByte(QUAT_SHIFTED_LIMIT_BYTE);
                    }
                    quaternaries.appendWeight32((p1 << 24) | (p & 0xffffff));
                }
                do {
                    ce = iter.nextCE(errorCode);
                    p = (uint32_t)(ce >> 32);
                } while(p == 0);
                p1 = p >> 24;
                if(reorderTable != NULL) { p1 = reorderTable[p1]; }
            } while(p < variableTop && p > Collation::MERGE_SEPARATOR_PRIMARY);
        }
        // ce could be primary ignorable, or NO_CE, or the merge separator,
        // or a regular primary CE, but it is not variable.
        // If ce==NO_CE, then write nothing for the primary level but
        // terminate compression on all levels and then exit the loop.
        if(p > Collation::NO_CE_PRIMARY && (levels & Collation::PRIMARY_LEVEL_FLAG) != 0) {
            if(p1 != compressedP1) {
                if(compressedP1 != 0) {
                    if(p1 < compressedP1) {
                        // No primary compression terminator
                        // at the end of the level or merged segment.
                        if(p1 > Collation::MERGE_SEPARATOR_BYTE) {
                            sink.Append(Collation::PRIMARY_COMPRESSION_LOW_BYTE);
                        }
                    } else {
                        sink.Append(Collation::PRIMARY_COMPRESSION_HIGH_BYTE);
                    }
                }
                sink.Append(p1);
                if(data->isCompressibleLeadByte(p1)) {
                    compressedP1 = p1;
                } else {
                    compressedP1 = 0;
                }
            }
            char p2 = (char)(p >> 16);
            if(p2 != 0) {
                char buffer[3] = { p2, (char)(p >> 8), (char)p };
                sink.Append(buffer, (buffer[1] == 0) ? 1 : (buffer[2] == 0) ? 2 : 3);
            }
        }

        uint32_t lower32 = (uint32_t)ce;
        if((levels & Collation::SECONDARY_LEVEL_FLAG) != 0) {
            uint32_t s = lower32 >> 16;
            if(s == 0) {
                // secondary ignorable
            } else if(s == Collation::COMMON_WEIGHT16) {
                ++commonSecondaries;
            } else if((options & CollationData::BACKWARD_SECONDARY) == 0) {
                if(commonSecondaries != 0) {
                    --commonSecondaries;
                    while(commonSecondaries >= SEC_COMMON_MAX_COUNT) {
                        secondaries.appendByte(SEC_COMMON_MIDDLE);
                        commonSecondaries -= SEC_COMMON_MAX_COUNT;
                    }
                    uint32_t b;
                    if(s < Collation::COMMON_WEIGHT16) {
                        b = SEC_COMMON_LOW + commonSecondaries;
                    } else {
                        b = SEC_COMMON_HIGH - commonSecondaries;
                    }
                    secondaries.appendByte(b);
                    commonSecondaries = 0;
                }
                secondaries.appendWeight16(s);
            } else {
                if(commonSecondaries != 0) {
                    --commonSecondaries;
                    // Append reverse weights. The level will be re-reversed later.
                    int32_t remainder = commonSecondaries % SEC_COMMON_MAX_COUNT;
                    uint32_t b;
                    if(prevSecondary < Collation::COMMON_WEIGHT16) {
                        b = SEC_COMMON_LOW + remainder;
                    } else {
                        b = SEC_COMMON_HIGH - remainder;
                    }
                    secondaries.appendByte(b);
                    commonSecondaries -= remainder;
                    // commonSecondaries is now a multiple of SEC_COMMON_MAX_COUNT.
                    while(commonSecondaries > 0) {  // same as >= SEC_COMMON_MAX_COUNT
                        secondaries.appendByte(SEC_COMMON_MIDDLE);
                        commonSecondaries -= SEC_COMMON_MAX_COUNT;
                    }
                    // commonSecondaries == 0
                }
                // Reduce separators so that we can look for byte<=1 later.
                if(s <= Collation::MERGE_SEPARATOR_WEIGHT16) {
                    if(s == Collation::MERGE_SEPARATOR_WEIGHT16) {
                        anyMergeSeparators = TRUE;
                    }
                    secondaries.appendByte((s >> 8) - 1);
                } else {
                    secondaries.appendReverseWeight16(s);
                }
                prevSecondary = s;
            }
        }

        if(lower32 == 0) { continue; }  // completely ignorable, no case/tertiary/quaternary

        if((levels & Collation::CASE_LEVEL_FLAG) != 0) {
            if((CollationData::getStrength(options) == UCOL_PRIMARY) ?
                    p == 0 : lower32 <= 0xffff) {
                // Primary+case: Ignore case level weights of primary ignorables.
                // Otherwise: Ignore case level weights of secondary ignorables.
                // For details see the comments in the CollationCompare class.
            } else {
                uint32_t c = (lower32 >> 8) & 0xff;  // case bits & tertiary lead byte
                U_ASSERT((c & 0xc0) != 0xc0);
                if((c & 0xc0) == 0 && c > Collation::MERGE_SEPARATOR_BYTE) {
                    ++commonCases;
                } else if((options & CollationData::UPPER_FIRST) == 0) {
                    // lowerFirst: Compress common weights to nibbles 1..7..13, mixed=14, upper=15.
                    if(commonCases != 0) {
                        --commonCases;
                        while(commonCases >= CASE_LOWER_FIRST_COMMON_MAX_COUNT) {
                            cases.appendByte(CASE_LOWER_FIRST_COMMON_MIDDLE << 4);
                            commonCases -= CASE_LOWER_FIRST_COMMON_MAX_COUNT;
                        }
                        uint32_t b;
                        if(c <= Collation::MERGE_SEPARATOR_BYTE) {
                            b = CASE_LOWER_FIRST_COMMON_LOW + commonCases;
                        } else {
                            b = CASE_LOWER_FIRST_COMMON_HIGH - commonCases;
                        }
                        cases.appendByte(b << 4);
                        commonCases = 0;
                    }
                    if(c > Collation::MERGE_SEPARATOR_BYTE) {
                        c = (CASE_LOWER_FIRST_COMMON_HIGH + (c >> 6)) << 4;  // 14 or 15
                    }
                } else {
                    // upperFirst: Compress common weights to nibbles 3..15, mixed=2, upper=1.
                    // The compressed common case weights only go down from the "high" value
                    // because with upperFirst the common weight is the highest one.
                    if(commonCases != 0) {
                        --commonCases;
                        while(commonCases >= CASE_UPPER_FIRST_COMMON_MAX_COUNT) {
                            cases.appendByte(CASE_UPPER_FIRST_COMMON_LOW << 4);
                            commonCases -= CASE_UPPER_FIRST_COMMON_MAX_COUNT;
                        }
                        cases.appendByte((CASE_UPPER_FIRST_COMMON_HIGH - commonCases) << 4);
                        commonCases = 0;
                    }
                    if(c > Collation::MERGE_SEPARATOR_BYTE) {
                        c = (CASE_UPPER_FIRST_COMMON_LOW - (c >> 6)) << 4;  // 2 or 1
                    }
                }
                // c is a separator byte 01 or 02,
                // or a left-shifted nibble 0x10, 0x20, ... 0xf0.
                cases.appendByte(c);
            }
        }

        if((levels & Collation::TERTIARY_LEVEL_FLAG) != 0) {
            uint32_t t = lower32 & tertiaryMask;
            U_ASSERT((lower32 & 0xc000) != 0xc000);
            if(t == Collation::COMMON_WEIGHT16) {
                ++commonTertiaries;
            } else if((tertiaryMask & 0x8000) == 0) {
                // Tertiary weights without case bits.
                // Move lead bytes 06..3F to C6..FF for a large common-weight range.
                if(commonTertiaries != 0) {
                    --commonTertiaries;
                    while(commonTertiaries >= TER_ONLY_COMMON_MAX_COUNT) {
                        tertiaries.appendByte(TER_ONLY_COMMON_MIDDLE);
                        commonTertiaries -= TER_ONLY_COMMON_MAX_COUNT;
                    }
                    uint32_t b;
                    if(t < Collation::COMMON_WEIGHT16) {
                        b = TER_ONLY_COMMON_LOW + commonTertiaries;
                    } else {
                        b = TER_ONLY_COMMON_HIGH - commonTertiaries;
                    }
                    tertiaries.appendByte(b);
                    commonTertiaries = 0;
                }
                if(t > Collation::COMMON_WEIGHT16) { t += 0xc000; }
                tertiaries.appendWeight16(t);
            } else if((options & CollationData::UPPER_FIRST) == 0) {
                // Tertiary weights with caseFirst=lowerFirst.
                // Move lead bytes 06..BF to 46..FF for the common-weight range.
                if(commonTertiaries != 0) {
                    --commonTertiaries;
                    while(commonTertiaries >= TER_LOWER_FIRST_COMMON_MAX_COUNT) {
                        tertiaries.appendByte(TER_LOWER_FIRST_COMMON_MIDDLE);
                        commonTertiaries -= TER_LOWER_FIRST_COMMON_MAX_COUNT;
                    }
                    uint32_t b;
                    if(t < Collation::COMMON_WEIGHT16) {
                        b = TER_LOWER_FIRST_COMMON_LOW + commonTertiaries;
                    } else {
                        b = TER_LOWER_FIRST_COMMON_HIGH - commonTertiaries;
                    }
                    tertiaries.appendByte(b);
                    commonTertiaries = 0;
                }
                if(t > Collation::COMMON_WEIGHT16) { t += 0x4000; }
                tertiaries.appendWeight16(t);
            } else {
                // Tertiary weights with caseFirst=upperFirst.
                // Do not change the artificial uppercase weight of a tertiary CE (0.0.ut),
                // to keep tertiary CEs well-formed.
                // Their case+tertiary weights must be greater than those of
                // primary and secondary CEs.
                //
                // Separators    01..02 -> 01..02  (unchanged)
                // Lowercase     03..04 -> 83..84  (includes uncased)
                // Common weight     05 -> 85..C5  (common-weight compression range)
                // Lowercase     06..3F -> C6..FF
                // Mixed case    43..7F -> 43..7F
                // Uppercase     83..BF -> 03..3F
                // Tertiary CE   86..BF -> C6..FF
                if(t <= Collation::MERGE_SEPARATOR_WEIGHT16) {
                    // Keep separators unchanged.
                } else if(lower32 > 0xffff) {
                    // Invert case bits of primary & secondary CEs.
                    t ^= 0xc000;
                    if(t < (TER_UPPER_FIRST_COMMON_HIGH << 8)) {
                        t -= 0x4000;
                    }
                } else {
                    // Keep uppercase bits of tertiary CEs.
                    U_ASSERT(0x8600 <= t && t <= 0xbfff);
                    t += 0x4000;
                }
                // TODO: does v1 handle 03 & 04 tertiary weights properly? 02?
                //       try &[before 3]a<<<x and U+FFFE with all case settings
                if(commonTertiaries != 0) {
                    --commonTertiaries;
                    while(commonTertiaries >= TER_UPPER_FIRST_COMMON_MAX_COUNT) {
                        tertiaries.appendByte(TER_UPPER_FIRST_COMMON_MIDDLE);
                        commonTertiaries -= TER_UPPER_FIRST_COMMON_MAX_COUNT;
                    }
                    uint32_t b;
                    if(t < (TER_UPPER_FIRST_COMMON_LOW << 8)) {
                        b = TER_UPPER_FIRST_COMMON_LOW + commonTertiaries;
                    } else {
                        b = TER_UPPER_FIRST_COMMON_HIGH - commonTertiaries;
                    }
                    tertiaries.appendByte(b);
                    commonTertiaries = 0;
                }
                tertiaries.appendWeight16(t);
            }
        }

        if((levels & Collation::QUATERNARY_LEVEL_FLAG) != 0) {
            uint32_t q = lower32 & 0xffff;
            if((q & 0xc0) == 0 && q > Collation::MERGE_SEPARATOR_WEIGHT16) {
                ++commonQuaternaries;
            } else if(q <= Collation::MERGE_SEPARATOR_WEIGHT16 &&
                    (options & CollationData::ALTERNATE_MASK) == 0 &&
                    (quaternaries.isEmpty() ||
                        quaternaries[quaternaries.length() - 1] == Collation::MERGE_SEPARATOR_BYTE)) {
                // If alternate=non-ignorable and there are only
                // common quaternary weights between two separators,
                // then we need not write anything between these separators.
                // The only weights greater than the merge separator and less than the common weight
                // are shifted primary weights, which are not generated for alternate=non-ignorable.
                // There are also exactly as many quaternary weights as tertiary weights,
                // so level length differences are handled already on tertiary level.
                // Any above-common quaternary weight will compare greater regardless.
                quaternaries.appendByte(q >> 8);
            } else {
                if(q <= Collation::MERGE_SEPARATOR_WEIGHT16) {
                    q >>= 8;
                } else {
                    q = 0xfc + ((q >> 6) & 3);
                }
                if(commonQuaternaries != 0) {
                    --commonQuaternaries;
                    while(commonQuaternaries >= QUAT_COMMON_MAX_COUNT) {
                        quaternaries.appendByte(QUAT_COMMON_MIDDLE);
                        commonQuaternaries -= QUAT_COMMON_MAX_COUNT;
                    }
                    uint32_t b;
                    if(q < QUAT_COMMON_LOW) {
                        b = QUAT_COMMON_LOW + commonQuaternaries;
                    } else {
                        b = QUAT_COMMON_HIGH - commonQuaternaries;
                    }
                    quaternaries.appendByte(b);
                    commonQuaternaries = 0;
                }
                quaternaries.appendByte(q);
            }
        }

        if((lower32 >> 24) == Collation::LEVEL_SEPARATOR_BYTE) { break; }  // ce == NO_CE
    }

    if(U_FAILURE(errorCode)) { return; }

    // Append the beyond-primary levels.
    UBool ok = TRUE;
    if((levels & Collation::SECONDARY_LEVEL_FLAG) != 0) {
        if(!callback.needToWrite(Collation::SECONDARY_LEVEL)) { return; }
        ok &= secondaries.isOk();
        sink.Append(Collation::LEVEL_SEPARATOR_BYTE);
        uint8_t *secs = secondaries.data();
        int32_t length = secondaries.length() - 1;  // Ignore the trailing NO_CE.
        if((options & CollationData::BACKWARD_SECONDARY) != 0) {
            // The backwards secondary level compares secondary weights backwards
            // within segments separated by the merge separator (U+FFFE, weight 02).
            // The separator weights 01 & 02 were reduced to 00 & 01 so that
            // we do not accidentally separate at a _second_ weight byte of 02.
            int32_t start = 0;
            for(;;) {
                // Find the merge separator or the NO_CE terminator.
                int32_t limit;
                if(anyMergeSeparators) {
                    limit = start;
                    while(secs[limit] > 1) { ++limit; }
                } else {
                    limit = length;
                }
                // Reverse this segment.
                if(start < limit) {
                    uint8_t *p = secs + start;
                    uint8_t *q = secs + limit - 1;
                    while(p < q) {
                        uint8_t s = *p;
                        *p++ = *q;
                        *q-- = s;
                    }
                }
                // Did we reach the end of the string?
                if(secs[limit] == 0) { break; }
                // Restore the merge separator.
                secs[limit] = 2;
                // Skip the merge separator and continue.
                start = limit + 1;
            }
        }
        sink.Append(reinterpret_cast<char *>(secs), length);
    }

    if((levels & Collation::CASE_LEVEL_FLAG) != 0) {
        if(!callback.needToWrite(Collation::CASE_LEVEL)) { return; }
        ok &= cases.isOk();
        sink.Append(Collation::LEVEL_SEPARATOR_BYTE);
        // Write pairs of nibbles as bytes, except separator bytes as themselves.
        int32_t length = cases.length() - 1;  // Ignore the trailing NO_CE.
        uint8_t b = 0;
        for(int32_t i = 0; i < length; ++i) {
            uint8_t c = (uint8_t)cases[i];
            if(c <= Collation::MERGE_SEPARATOR_BYTE) {
                if(b != 0) {
                    sink.Append(b);
                    b = 0;
                }
                sink.Append(c);
            } else {
                if(b == 0) {
                    b = c;
                } else {
                    sink.Append(b | (c >> 4));
                    b = 0;
                }
            }
        }
        if(b != 0) {
            sink.Append(b);
        }
    }

    if((levels & Collation::TERTIARY_LEVEL_FLAG) != 0) {
        if(!callback.needToWrite(Collation::TERTIARY_LEVEL)) { return; }
        ok &= tertiaries.isOk();
        sink.Append(Collation::LEVEL_SEPARATOR_BYTE);
        tertiaries.appendTo(sink);
    }

    if((levels & Collation::QUATERNARY_LEVEL_FLAG) != 0) {
        if(!callback.needToWrite(Collation::QUATERNARY_LEVEL)) { return; }
        ok &= quaternaries.isOk();
        sink.Append(Collation::LEVEL_SEPARATOR_BYTE);
        quaternaries.appendTo(sink);
    }

    if(!ok || !sink.IsOk()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
}

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
