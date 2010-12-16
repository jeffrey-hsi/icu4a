/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  uchartrie.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010nov14
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "uassert.h"
#include "uchartrie.h"

U_NAMESPACE_BEGIN

Appendable &
Appendable::appendCodePoint(UChar32 c) {
    if(c<=0xffff) {
        return append((UChar)c);
    } else {
        return append(U16_LEAD(c)).append(U16_TRAIL(c));
    }
}

Appendable &
Appendable::append(const UChar *s, int32_t length) {
    if(s!=NULL && length!=0) {
        if(length<0) {
            UChar c;
            while((c=*s++)!=0) {
                append(c);
            }
        } else {
            const UChar *limit=s+length;
            while(s<limit) {
                append(*s++);
            }
        }
    }
    return *this;
}

UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(Appendable)

UCharTrie::Result
UCharTrie::branchNext(const UChar *pos, int32_t length, int32_t uchar, UBool checkValue) {
    // Branch according to the current unit.
    if(length==0) {
        length=*pos++;
    }
    ++length;
    // The length of the branch is the number of units to select from.
    // The data structure encodes a binary search.
    while(length>kMaxBranchLinearSubNodeLength) {
        if(uchar<*pos++) {
            length>>=1;
            // int32_t delta=readDelta(pos);
            int32_t delta=*pos++;
            if(delta>=kMinTwoUnitDeltaLead) {
                if(delta==kThreeUnitDeltaLead) {
                    delta=(pos[0]<<16)|pos[1];
                    pos+=2;
                } else {
                    delta=((delta-kMinTwoUnitDeltaLead)<<16)|*pos++;
                }
            }
            // end readDelta()
            pos+=delta;
        } else {
            length=length-(length>>1);
            pos=skipDelta(pos);
        }
    }
    // Drop down to linear search for the last few units.
    // length>=2 because the loop body above sees length>kMaxBranchLinearSubNodeLength>=3
    // and divides length by 2.
    do {
        if(uchar==*pos++) {
            Result result;
            int32_t node=*pos;
            U_ASSERT(node>=kMinValueLead);
            UBool isFinal=(UBool)(node&kValueIsFinal);
            if(isFinal && !checkValue) {
                // Leave the final value for hasValue() to read.
                result=MAYBE_VALUE;
            } else {
                ++pos;
                // int32_t delta=readValue(pos, node>>1);
                node>>=1;
                int32_t value;
                if(node<kMinTwoUnitValueLead) {
                    value=node-kMinOneUnitValueLead;
                } else if(node<kThreeUnitValueLead) {
                    value=((node-kMinTwoUnitValueLead)<<16)|*pos++;
                } else {
                    value=(pos[0]<<16)|pos[1];
                    pos+=2;
                }
                // end readValue()
                if(isFinal) {
                    pos=NULL;
                    value_=value;
                    haveValue_=TRUE;
                    result=HAS_VALUE;
                } else {
                    // Use the non-final value as the jump delta.
                    return matchResult(pos+value, checkValue);
                }
            }
            pos_=pos;
            return result;
        }
        --length;
        pos=skipValueAndFinal(pos);
    } while(length>1);
    if(uchar==*pos++) {
        return matchResult(pos, checkValue);
    } else {
        stop();
        return NO_MATCH;
    }
}

UCharTrie::Result
UCharTrie::nextImpl(const UChar *pos, int32_t uchar, UBool checkValue) {
    for(;;) {
        int32_t node=*pos++;
        if(node<kMinLinearMatch) {
            return branchNext(pos, node, uchar, checkValue);
        } else if(node<kMinValueLead) {
            // Match the first of length+1 units.
            int32_t length=node-kMinLinearMatch;  // Actual match length minus 1.
            if(uchar==*pos++) {
                return linearMatchResult(pos, length-1, checkValue);
            } else {
                // No match.
                break;
            }
        } else if(node&kValueIsFinal) {
            // No further matching units.
            break;
        } else {
            // Skip intermediate value.
            pos=skipValueAndFinal(pos, node);
            // The next node must not also be a value node.
            U_ASSERT(*pos<kMinValueLead);
        }
    }
    stop();
    return NO_MATCH;
}

UCharTrie::Result
UCharTrie::next(int32_t uchar, UBool checkValue) {
    const UChar *pos=pos_;
    if(pos==NULL) {
        return NO_MATCH;
    }
    haveValue_=FALSE;
    int32_t length=remainingMatchLength_;  // Actual remaining match length minus 1.
    if(length>=0) {
        // Remaining part of a linear-match node.
        if(uchar==*pos++) {
            return linearMatchResult(pos, length-1, checkValue);
        } else {
            stop();
            return NO_MATCH;
        }
    }
    return nextImpl(pos, uchar, checkValue);
}

UCharTrie::Result
UCharTrie::next(const UChar *s, int32_t sLength) {
    if(sLength<0 ? *s==0 : sLength==0) {
        // Empty input: Do nothing at all, see API doc.
        return MAYBE_VALUE;
    }
    const UChar *pos=pos_;
    if(pos==NULL) {
        return NO_MATCH;
    }
    haveValue_=FALSE;
    int32_t length=remainingMatchLength_;  // Actual remaining match length minus 1.
    for(;;) {
        // Fetch the next input unit, if there is one.
        // Continue a linear-match node without rechecking sLength<0.
        int32_t uchar;
        if(sLength<0) {
            for(;;) {
                if((uchar=*s++)==0) {
                    remainingMatchLength_=length;
                    pos_=pos;
                    return length>=0 ? NO_VALUE : MAYBE_VALUE;
                }
                if(length<0) {
                    remainingMatchLength_=length;
                    break;
                }
                if(uchar!=*pos) {
                    stop();
                    return NO_MATCH;
                }
                ++pos;
                --length;
            }
        } else {
            for(;;) {
                if(sLength==0) {
                    remainingMatchLength_=length;
                    pos_=pos;
                    return length>=0 ? NO_VALUE : MAYBE_VALUE;
                }
                uchar=*s++;
                --sLength;
                if(length<0) {
                    remainingMatchLength_=length;
                    break;
                }
                if(uchar!=*pos) {
                    stop();
                    return NO_MATCH;
                }
                ++pos;
                --length;
            }
        }
        for(;;) {
            int32_t node=*pos++;
            if(node<kMinLinearMatch) {
                if(NO_MATCH==branchNext(pos, node, uchar, FALSE)) {
                    return NO_MATCH;
                }
                // Fetch the next input unit, if there is one.
                if(sLength<0) {
                    if((uchar=*s++)==0) {
                        return MAYBE_VALUE;
                    }
                } else {
                    if(sLength==0) {
                        return MAYBE_VALUE;
                    }
                    uchar=*s++;
                    --sLength;
                }
                pos=pos_;  // branchNext() advanced pos and wrote it to pos_ .
            } else if(node<kMinValueLead) {
                // Match length+1 units.
                length=node-kMinLinearMatch;  // Actual match length minus 1.
                if(uchar!=*pos) {
                    stop();
                    return NO_MATCH;
                }
                ++pos;
                --length;
                break;
            } else if(node&kValueIsFinal) {
                // No further matching units.
                stop();
                return NO_MATCH;
            } else {
                // Skip intermediate value.
                pos=skipValueAndFinal(pos, node);
                // The next node must not also be a value node.
                U_ASSERT(*pos<kMinValueLead);
            }
        }
    }
}

UBool
UCharTrie::hasValue() {
    int32_t node;
    if(haveValue_) {
        return TRUE;
    }
    const UChar *pos=pos_;
    if(pos!=NULL && remainingMatchLength_<0 && (node=*pos)>=kMinValueLead) {
        // Deliver value for the matching units.
        readValueAndFinal(pos+1, node);
        return TRUE;
    }
    return FALSE;
}

UBool
UCharTrie::hasUniqueValue() {
#if 0
    if(pos==NULL) {
        return FALSE;
    }
    const UChar *originalPos=pos;
    uniqueValue=value;
    haveUniqueValue=haveValue;

    if(remainingMatchLength>=0) {
        // Skip the rest of a pending linear-match node.
        pos+=remainingMatchLength+1;
    }
    haveValue=findUniqueValue();
    // If haveValue is true, then value is already set to the final value
    // of the last-visited branch.
    // Restore original state, except for value/haveValue.
    pos=originalPos;
    return haveValue;
#endif
    return FALSE;
}

UBool
UCharTrie::findUniqueValueFromBranchEntry(int32_t node) {
#if 0
    value=readFixedInt(node);
    if(node&kFixedIntIsFinal) {
        // Final value directly in the branch entry.
        if(!isUniqueValue()) {
            return FALSE;
        }
    } else {
        // Use the non-final value as the jump delta.
        if(!findUniqueValueAt(value)) {
            return FALSE;
        }
    }
    return TRUE;
#endif
    return FALSE;
}

UBool
UCharTrie::findUniqueValue() {
#if 0
    for(;;) {
        int32_t node=*pos++;
        if(node<kMinLinearMatch) {
            while(node>=kMinSplitBranch) {
                // split-branch node
                ++pos;  // ignore the comparison unit
                // less-than branch
                int32_t delta=readFixedInt(node);
                if(!findUniqueValueAt(delta)) {
                    return FALSE;
                }
                // greater-or-equal branch
                node=*pos++;
                U_ASSERT(node<kMinLinearMatch);
            }
            // list-branch node
            int32_t length=(node>>kMaxListBranchLengthShift)+1;  // Actual list length minus 1.
            if(length>=kMaxListBranchSmallLength) {
                // For 7..14 pairs, read the next unit as well.
                node=(node<<16)|*pos++;
            }
            do {
                ++pos;  // ignore a comparison unit
                // handle its value
                if(!findUniqueValueFromBranchEntry(node)) {
                    return FALSE;
                }
                node>>=2;
            } while(--length>0);
            ++pos;  // ignore the last comparison unit
        } else if(node<kMinValueLead) {
            // linear-match node
            pos+=node-kMinLinearMatch+1;  // Ignore the match units.
        } else {
            UBool isFinal=readCompactInt(node);
            if(!isUniqueValue()) {
                return FALSE;
            }
            if(isFinal) {
                return TRUE;
            }
            U_ASSERT(*pos<kMinValueLead);
        }
    }
#endif
    return FALSE;
}

int32_t
UCharTrie::getNextUChars(Appendable &out) {
#if 0
    if(pos==NULL) {
        return 0;
    }
    if(remainingMatchLength>=0) {
        out.append(*pos);  // Next byte of a pending linear-match node.
        return 1;
    }
    const UChar *originalPos=pos;
    int32_t node=*pos;
    if(node>=kMinValueLead) {
        if(node&kValueIsFinal) {
            return 0;
        } else {
            ++pos;  // Skip the lead unit.
            skipValueAndFinal(node);
            node=*pos;
            U_ASSERT(node<kMinValueLead);
        }
    }
    int32_t count;
    if(node<kMinLinearMatch) {
        count=getNextBranchUChars(out);
    } else {
        // First byte of the linear-match node.
        out.append(pos[1]);
        count=1;
    }
    pos=originalPos;
    return count;
#endif
    out.append(0);
    return 0;
}

int32_t
UCharTrie::getNextBranchUChars(Appendable &out) {
#if 0
    int32_t count=0;
    int32_t node=*pos++;
    U_ASSERT(node<kMinLinearMatch);
    while(node>=kMinSplitBranch) {
        // split-branch node
        ++pos;  // ignore the comparison unit
        // less-than branch
        int32_t delta=readFixedInt(node);
        const UChar *currentPos=pos;
        pos+=delta;
        count+=getNextBranchUChars(out);
        pos=currentPos;
        // greater-or-equal branch
        node=*pos++;
        U_ASSERT(node<kMinLinearMatch);
    }
    // list-branch node
    int32_t length=(node>>kMaxListBranchLengthShift)+1;  // Actual list length minus 1.
    if(length>=kMaxListBranchSmallLength) {
        // For 7..14 pairs, read the next unit as well.
        node=(node<<16)|*pos++;
    }
    count+=length+1;
    do {
        out.append(*pos++);
        pos+=(node&kFixedInt32)+1;
        node>>=2;
    } while(--length>0);
    out.append(*pos);
    return count;
#endif
    out.append(0);
    return 0;
}

U_NAMESPACE_END
