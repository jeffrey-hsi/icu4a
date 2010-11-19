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
#include "unicode/uobject.h"
#include "uassert.h"
#include "bytetrie.h"

U_NAMESPACE_BEGIN

const int8_t ByteTrie::bytesPerLead[kFiveByteLead+1]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5
};

UBool
ByteTrie::readCompactInt(int32_t leadByte) {
    UBool isFinal=(UBool)(leadByte&kValueIsFinal);
    leadByte>>=1;
    int numBytes=bytesPerLead[leadByte]-1;  // -1: lead byte was already consumed.
    switch(numBytes) {
    case 0:
        value=leadByte-kMinOneByteLead;
        break;
    case 1:
        value=((leadByte-kMinTwoByteLead)<<8)|*pos;
        break;
    case 2:
        value=((leadByte-kMinThreeByteLead)<<16)|(pos[0]<<8)|pos[1];
        break;
    case 3:
        value=(pos[0]<<16)|(pos[1]<<8)|pos[2];
        break;
    case 4:
        value=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
        break;
    }
    pos+=numBytes;
    return isFinal;
}

int32_t
ByteTrie::readFixedInt(int32_t bytesPerValue) {
    int32_t fixedInt;
    switch(bytesPerValue) {  // Actually number of bytes minus 1.
    case 0:
        fixedInt=*pos;
        break;
    case 1:
        fixedInt=(pos[0]<<8)|pos[1];
        break;
    case 2:
        fixedInt=(pos[0]<<16)|(pos[1]<<8)|pos[2];
        break;
    case 3:
        fixedInt=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
        break;
    }
    pos+=bytesPerValue+1;
    return fixedInt;
}

UBool
ByteTrie::next(int inByte) {
    if(pos==NULL) {
        return FALSE;
    }
    int32_t length=remainingMatchLength;  // Actual remaining match length minus 1.
    if(length>=0) {
        // Remaining part of a linear-match node.
        if(inByte==*pos) {
            remainingMatchLength=length-1;
            ++pos;
            return TRUE;
        } else {
            // No match.
            stop();
            return FALSE;
        }
    }
    int32_t node=*pos;
    if(node>=kMinValueLead) {
        if(node&kValueIsFinal) {
            // No further matching bytes.
            stop();
            return FALSE;
        } else {
            // Skip intermediate value.
            pos+=bytesPerLead[node>>1];
            // The next node must not also be a value node.
            node=*pos;
            U_ASSERT(node<kMinValueLead);
        }
    }
    ++pos;
    if(node<kMinLinearMatch) {
        // Branch according to the current byte.
        while(node<kMinListBranch) {
            // Branching on a byte value,
            // with a jump delta for less-than, a compact int for equals,
            // and continuing for greater-than.
            // The less-than and greater-than branches must lead to branch nodes again.
            uint8_t trieByte=*pos++;
            if(inByte<trieByte) {
                int32_t delta=readFixedInt(node);
                pos+=delta;
            } else {
                pos+=node+1;  // Skip fixed-width integer.
                node=*pos;
                U_ASSERT(node>=kMinValueLead);
                if(inByte==trieByte) {
                    if(node&kValueIsFinal) {
                        // Leave the final value for contains() to read.
                    } else {
                        // Use the non-final value as the jump delta.
                        ++pos;
                        readCompactInt(node);
                        pos+=value;
                    }
                    return TRUE;
                } else {  // inByte>trieByte
                    pos+=bytesPerLead[node>>1];
                }
            }
            node=*pos++;
            U_ASSERT(node<kMinLinearMatch);
        }
        // Branch node with a list of key-value pairs where
        // values are compact integers: either final values or jump deltas.
        // If the last key byte matches, just continue after it rather
        // than jumping.
        length=node-(kMinListBranch-1);  // Actual list length minus 1.
        for(;;) {
            uint8_t trieByte=*pos++;
            U_ASSERT(length==0 || *pos>=kMinValueLead);
            if(inByte==trieByte) {
                if(length>0) {
                    node=*pos;
                    if(node&kValueIsFinal) {
                        // Leave the final value for contains() to read.
                    } else {
                        // Use the non-final value as the jump delta.
                        ++pos;
                        readCompactInt(node);
                        pos+=value;
                    }
                }
                return TRUE;
            }
            if(inByte<trieByte || length--==0) {
                stop();
                return FALSE;
            }
            pos+=bytesPerLead[*pos>>1];
        }
    } else {
        // Match the first of length+1 bytes.
        length=node-kMinLinearMatch;  // Actual match length minus 1.
        if(inByte==*pos) {
            remainingMatchLength=length-1;
            ++pos;
            return TRUE;
        } else {
            // No match.
            stop();
            return FALSE;
        }
    }
}

UBool
ByteTrie::contains() {
    int32_t node;
    if(pos!=NULL && remainingMatchLength<0 && (node=*pos)>=kMinValueLead) {
        // Deliver value for the matching bytes.
        ++pos;
        if(readCompactInt(node)) {
            stop();
        }
        return TRUE;
    }
    return FALSE;
}

UBool
ByteTrie::containsNext(const char *s, int32_t length) {
    if(length<0) {
        // NUL-terminated
        int b;
        while((b=(uint8_t)*s++)!=0) {
            if(!next(b)) {
                return FALSE;
            }
        }
    } else {
        while(length>0) {
            if(!next((uint8_t)*s++)) {
                return FALSE;
            }
            --length;
        }
    }
    return contains();
}

U_NAMESPACE_END
