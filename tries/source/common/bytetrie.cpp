/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  bytetrie.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010sep25
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "unicode/bytestream.h"
#include "unicode/uobject.h"
#include "uassert.h"
#include "bytetrie.h"

U_NAMESPACE_BEGIN

// lead byte already shifted right by 1.
int32_t
ByteTrie::readValue(const uint8_t *pos, int32_t leadByte) {
    int32_t value;
    if(leadByte<kMinTwoByteValueLead) {
        value=leadByte-kMinOneByteValueLead;
    } else if(leadByte<kMinThreeByteValueLead) {
        value=((leadByte-kMinTwoByteValueLead)<<8)|*pos;
    } else if(leadByte<kFourByteValueLead) {
        value=((leadByte-kMinThreeByteValueLead)<<16)|(pos[0]<<8)|pos[1];
    } else if(leadByte==kFourByteValueLead) {
        value=(pos[0]<<16)|(pos[1]<<8)|pos[2];
    } else {
        value=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
    }
    return value;
}

const uint8_t *
ByteTrie::jumpByDelta(const uint8_t *pos) {
    int32_t delta=*pos++;
    if(delta<kMinTwoByteDeltaLead) {
        // nothing to do
    } else if(delta<kMinThreeByteDeltaLead) {
        delta=((delta-kMinTwoByteDeltaLead)<<8)|*pos++;
    } else if(delta<kFourByteDeltaLead) {
        delta=((delta-kMinThreeByteDeltaLead)<<16)|(pos[0]<<8)|pos[1];
        pos+=2;
    } else if(delta==kFourByteDeltaLead) {
        delta=(pos[0]<<16)|(pos[1]<<8)|pos[2];
        pos+=3;
    } else {
        delta=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
        pos+=4;
    }
    return pos+delta;
}

UDictTrieResult
ByteTrie::current() const {
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return UDICTTRIE_NO_MATCH;
    } else {
        int32_t node;
        return (remainingMatchLength_<0 && (node=*pos)>=kMinValueLead) ?
            (UDictTrieResult)(UDICTTRIE_HAS_VALUE+(node&kValueIsFinal)) : UDICTTRIE_NO_VALUE;
    }
}

UDictTrieResult
ByteTrie::branchNext(const uint8_t *pos, int32_t length, int32_t inByte) {
    // Branch according to the current byte.
    if(length==0) {
        length=*pos++;
    }
    ++length;
    // The length of the branch is the number of bytes to select from.
    // The data structure encodes a binary search.
    while(length>kMaxBranchLinearSubNodeLength) {
        if(inByte<*pos++) {
            length>>=1;
            pos=jumpByDelta(pos);
        } else {
            length=length-(length>>1);
            pos=skipDelta(pos);
        }
    }
    // Drop down to linear search for the last few bytes.
    // length>=2 because the loop body above sees length>kMaxBranchLinearSubNodeLength>=3
    // and divides length by 2.
    do {
        if(inByte==*pos++) {
            UDictTrieResult result;
            int32_t node=*pos;
            U_ASSERT(node>=kMinValueLead);
            if(node&kValueIsFinal) {
                // Leave the final value for getValue() to read.
                result=UDICTTRIE_HAS_FINAL_VALUE;
            } else {
                // Use the non-final value as the jump delta.
                ++pos;
                // int32_t delta=readValue(pos, node>>1);
                node>>=1;
                int32_t delta;
                if(node<kMinTwoByteValueLead) {
                    delta=node-kMinOneByteValueLead;
                } else if(node<kMinThreeByteValueLead) {
                    delta=((node-kMinTwoByteValueLead)<<8)|*pos++;
                } else if(node<kFourByteValueLead) {
                    delta=((node-kMinThreeByteValueLead)<<16)|(pos[0]<<8)|pos[1];
                    pos+=2;
                } else if(node==kFourByteValueLead) {
                    delta=(pos[0]<<16)|(pos[1]<<8)|pos[2];
                    pos+=3;
                } else {
                    delta=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
                    pos+=4;
                }
                // end readValue()
                pos+=delta;
                node=*pos;
                result= node>=kMinValueLead ?
                    (UDictTrieResult)(UDICTTRIE_HAS_VALUE+(node&kValueIsFinal)) : UDICTTRIE_NO_VALUE;
            }
            pos_=pos;
            return result;
        }
        --length;
        pos=skipValue(pos);
    } while(length>1);
    if(inByte==*pos++) {
        pos_=pos;
        int32_t node=*pos;
        return node>=kMinValueLead ?
            (UDictTrieResult)(UDICTTRIE_HAS_VALUE+(node&kValueIsFinal)) : UDICTTRIE_NO_VALUE;
    } else {
        stop();
        return UDICTTRIE_NO_MATCH;
    }
}

UDictTrieResult
ByteTrie::nextImpl(const uint8_t *pos, int32_t inByte) {
    for(;;) {
        int32_t node=*pos++;
        if(node<kMinLinearMatch) {
            return branchNext(pos, node, inByte);
        } else if(node<kMinValueLead) {
            // Match the first of length+1 bytes.
            int32_t length=node-kMinLinearMatch;  // Actual match length minus 1.
            if(inByte==*pos++) {
                remainingMatchLength_=--length;
                pos_=pos;
                return (length<0 && (node=*pos)>=kMinValueLead) ?
                    (UDictTrieResult)(UDICTTRIE_HAS_VALUE+(node&kValueIsFinal)) : UDICTTRIE_NO_VALUE;
            } else {
                // No match.
                break;
            }
        } else if(node&kValueIsFinal) {
            // No further matching bytes.
            break;
        } else {
            // Skip intermediate value.
            pos=skipValue(pos, node);
            // The next node must not also be a value node.
            U_ASSERT(*pos<kMinValueLead);
        }
    }
    stop();
    return UDICTTRIE_NO_MATCH;
}

UDictTrieResult
ByteTrie::next(int32_t inByte) {
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return UDICTTRIE_NO_MATCH;
    }
    int32_t length=remainingMatchLength_;  // Actual remaining match length minus 1.
    if(length>=0) {
        // Remaining part of a linear-match node.
        if(inByte==*pos++) {
            remainingMatchLength_=--length;
            pos_=pos;
            int32_t node;
            return (length<0 && (node=*pos)>=kMinValueLead) ?
                (UDictTrieResult)(UDICTTRIE_HAS_VALUE+(node&kValueIsFinal)) : UDICTTRIE_NO_VALUE;
        } else {
            stop();
            return UDICTTRIE_NO_MATCH;
        }
    }
    return nextImpl(pos, inByte);
}

UDictTrieResult
ByteTrie::next(const char *s, int32_t sLength) {
    if(sLength<0 ? *s==0 : sLength==0) {
        // Empty input.
        return current();
    }
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return UDICTTRIE_NO_MATCH;
    }
    int32_t length=remainingMatchLength_;  // Actual remaining match length minus 1.
    for(;;) {
        // Fetch the next input byte, if there is one.
        // Continue a linear-match node without rechecking sLength<0.
        int32_t inByte;
        if(sLength<0) {
            for(;;) {
                if((inByte=*s++)==0) {
                    remainingMatchLength_=length;
                    pos_=pos;
                    int32_t node;
                    return (length<0 && (node=*pos)>=kMinValueLead) ?
                        (UDictTrieResult)(UDICTTRIE_HAS_VALUE+(node&kValueIsFinal)) : UDICTTRIE_NO_VALUE;
                }
                if(length<0) {
                    remainingMatchLength_=length;
                    break;
                }
                if(inByte!=*pos) {
                    stop();
                    return UDICTTRIE_NO_MATCH;
                }
                ++pos;
                --length;
            }
        } else {
            for(;;) {
                if(sLength==0) {
                    remainingMatchLength_=length;
                    pos_=pos;
                    int32_t node;
                    return (length<0 && (node=*pos)>=kMinValueLead) ?
                        (UDictTrieResult)(UDICTTRIE_HAS_VALUE+(node&kValueIsFinal)) : UDICTTRIE_NO_VALUE;
                }
                inByte=*s++;
                --sLength;
                if(length<0) {
                    remainingMatchLength_=length;
                    break;
                }
                if(inByte!=*pos) {
                    stop();
                    return UDICTTRIE_NO_MATCH;
                }
                ++pos;
                --length;
            }
        }
        for(;;) {
            int32_t node=*pos++;
            if(node<kMinLinearMatch) {
                UDictTrieResult result=branchNext(pos, node, inByte);
                if(result==UDICTTRIE_NO_MATCH) {
                    return UDICTTRIE_NO_MATCH;
                }
                // Fetch the next input byte, if there is one.
                if(sLength<0) {
                    if((inByte=*s++)==0) {
                        return result;
                    }
                } else {
                    if(sLength==0) {
                        return result;
                    }
                    inByte=*s++;
                    --sLength;
                }
                if(result==UDICTTRIE_HAS_FINAL_VALUE) {
                    // No further matching bytes.
                    stop();
                    return UDICTTRIE_NO_MATCH;
                }
                pos=pos_;  // branchNext() advanced pos and wrote it to pos_ .
            } else if(node<kMinValueLead) {
                // Match length+1 bytes.
                length=node-kMinLinearMatch;  // Actual match length minus 1.
                if(inByte!=*pos) {
                    stop();
                    return UDICTTRIE_NO_MATCH;
                }
                ++pos;
                --length;
                break;
            } else if(node&kValueIsFinal) {
                // No further matching bytes.
                stop();
                return UDICTTRIE_NO_MATCH;
            } else {
                // Skip intermediate value.
                pos=skipValue(pos, node);
                // The next node must not also be a value node.
                U_ASSERT(*pos<kMinValueLead);
            }
        }
    }
}

const uint8_t *
ByteTrie::findUniqueValueFromBranch(const uint8_t *pos, int32_t length,
                                    UBool haveUniqueValue, int32_t &uniqueValue) {
    while(length>kMaxBranchLinearSubNodeLength) {
        ++pos;  // ignore the comparison byte
        if(NULL==findUniqueValueFromBranch(jumpByDelta(pos), length>>1, haveUniqueValue, uniqueValue)) {
            return NULL;
        }
        length=length-(length>>1);
        pos=skipDelta(pos);
    }
    do {
        ++pos;  // ignore a comparison byte
        // handle its value
        int32_t node=*pos++;
        UBool isFinal=(UBool)(node&kValueIsFinal);
        int32_t value=readValue(pos, node>>1);
        pos=skipValue(pos, node);
        if(isFinal) {
            if(haveUniqueValue) {
                if(value!=uniqueValue) {
                    return NULL;
                }
            } else {
                uniqueValue=value;
                haveUniqueValue=TRUE;
            }
        } else {
            if(!findUniqueValue(pos+value, haveUniqueValue, uniqueValue)) {
                return NULL;
            }
            haveUniqueValue=TRUE;
        }
    } while(--length>1);
    return pos+1;  // ignore the last comparison byte
}

UBool
ByteTrie::findUniqueValue(const uint8_t *pos, UBool haveUniqueValue, int32_t &uniqueValue) {
    for(;;) {
        int32_t node=*pos++;
        if(node<kMinLinearMatch) {
            if(node==0) {
                node=*pos++;
            }
            pos=findUniqueValueFromBranch(pos, node+1, haveUniqueValue, uniqueValue);
            if(pos==NULL) {
                return FALSE;
            }
            haveUniqueValue=TRUE;
        } else if(node<kMinValueLead) {
            // linear-match node
            pos+=node-kMinLinearMatch+1;  // Ignore the match bytes.
        } else {
            UBool isFinal=(UBool)(node&kValueIsFinal);
            int32_t value=readValue(pos, node>>1);
            if(haveUniqueValue) {
                if(value!=uniqueValue) {
                    return FALSE;
                }
            } else {
                uniqueValue=value;
                haveUniqueValue=TRUE;
            }
            if(isFinal) {
                return TRUE;
            }
            pos=skipValue(pos, node);
        }
    }
}

int32_t
ByteTrie::getNextBytes(ByteSink &out) const {
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return 0;
    }
    if(remainingMatchLength_>=0) {
        append(out, *pos);  // Next byte of a pending linear-match node.
        return 1;
    }
    int32_t node=*pos++;
    if(node>=kMinValueLead) {
        if(node&kValueIsFinal) {
            return 0;
        } else {
            pos=skipValue(pos, node);
            node=*pos++;
            U_ASSERT(node<kMinValueLead);
        }
    }
    if(node<kMinLinearMatch) {
        if(node==0) {
            node=*pos++;
        }
        getNextBranchBytes(pos, ++node, out);
        return node;
    } else {
        // First byte of the linear-match node.
        append(out, *pos);
        return 1;
    }
}

void
ByteTrie::getNextBranchBytes(const uint8_t *pos, int32_t length, ByteSink &out) {
    while(length>kMaxBranchLinearSubNodeLength) {
        ++pos;  // ignore the comparison byte
        getNextBranchBytes(jumpByDelta(pos), length>>1, out);
        length=length-(length>>1);
        pos=skipDelta(pos);
    }
    do {
        append(out, *pos++);
        pos=skipValue(pos);
    } while(--length>1);
    append(out, *pos);
}

void
ByteTrie::append(ByteSink &out, int c) {
    char ch=(char)c;
    out.Append(&ch, 1);
}

U_NAMESPACE_END
