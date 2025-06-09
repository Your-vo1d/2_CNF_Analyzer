#include "BoolVector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t BoolVector::calculateBytes(size_t bits) const {
    return ((bits - 1) / 8) + 1;
}

BoolVector::BoolVector() : vector(nullptr), bits(0), bytes(0) {}

BoolVector::BoolVector(size_t input_bits) {
    bits = input_bits;
    bytes = calculateBytes(bits);
    vector = (unsigned char*)malloc(bytes);

    if (vector) {
        for (size_t i = 0; i < bytes; i++)
            vector[i] = 0;
    }
}

BoolVector::BoolVector(const char* str) : vector(nullptr), bits(0), bytes(0) {
    if (str) {
        bits = strlen(str);
        bytes = calculateBytes(bits);
        vector = (unsigned char*)malloc(bytes);
        
        if (vector) {
            for (size_t i = 0; i < bytes; i++)
                vector[i] = 0;

            size_t ix = 0;
            for (size_t i = 0; i < bytes; i++) {
                unsigned char mask = 1 << 7;
                for (size_t j = 0; j < 8 && ix < bits; j++) {
                    if (str[ix] != '0')
                        vector[i] |= mask;
                    mask >>= 1;
                    ix++;
                }
            }
        }
    }
}

BoolVector::~BoolVector() {
    if (vector) free(vector);
}

BoolVector::BoolVector(const BoolVector& other) : bits(other.bits), bytes(other.bytes) {
    if (other.vector && bytes > 0) {
        vector = (unsigned char*)malloc(bytes);
        if (vector) {
            memcpy(vector, other.vector, bytes);
        }
    } else {
        vector = nullptr;
    }
}

BoolVector& BoolVector::operator=(const BoolVector& other) {
    if (this != &other) {
        if (vector) free(vector);
        
        bits = other.bits;
        bytes = other.bytes;
        
        if (other.vector && bytes > 0) {
            vector = (unsigned char*)malloc(bytes);
            if (vector) {
                memcpy(vector, other.vector, bytes);
            }
        } else {
            vector = nullptr;
        }
    }
    return *this;
}

void BoolVector::print() const {
    if (vector && bits > 0) {
        for (size_t i = 0; i < bytes; i++) {
            unsigned char mask = 1 << 7;
            for (size_t j = 0; j < 8 && (i * 8 + j) < bits; j++) {
                printf("%d", (vector[i] & mask) ? 1 : 0);
                mask >>= 1;
            }
            printf(" ");
        }
    } else {
        printf("Empty vector");
    }
    printf("\n");
}

BoolVector BoolVector::logicOR(const BoolVector& other) const {
    if (bits == 0 || other.bits == 0) return BoolVector();
    
    size_t maxBits = bits > other.bits ? bits : other.bits;
    size_t maxBytes = calculateBytes(maxBits);
    
    BoolVector result;
    result.bits = maxBits;
    result.bytes = maxBytes;
    result.vector = (unsigned char*)malloc(maxBytes);
    
    if (result.vector) {
        memset(result.vector, 0, maxBytes);
        
        size_t copyBytes = bytes < maxBytes ? bytes : maxBytes;
        if (vector) {
            memcpy(result.vector, vector, copyBytes);
        }
        
        size_t otherCopyBytes = other.bytes < maxBytes ? other.bytes : maxBytes;
        for (size_t i = 0; i < otherCopyBytes; i++) {
            result.vector[i] |= other.vector[i];
        }
    }
    
    return result;
}

BoolVector BoolVector::logicAND(const BoolVector& other) const {
    if (bits == 0 || other.bits == 0) return BoolVector();
    
    size_t maxBits = bits > other.bits ? bits : other.bits;
    size_t maxBytes = calculateBytes(maxBits);
    
    BoolVector result;
    result.bits = maxBits;
    result.bytes = maxBytes;
    result.vector = (unsigned char*)malloc(maxBytes);
    
    if (result.vector) {
        memset(result.vector, 0, maxBytes);
        
        size_t copyBytes = bytes < maxBytes ? bytes : maxBytes;
        if (vector) {
            memcpy(result.vector, vector, copyBytes);
        }
        
        size_t otherCopyBytes = other.bytes < maxBytes ? other.bytes : maxBytes;
        for (size_t i = 0; i < otherCopyBytes; i++) {
            result.vector[i] &= other.vector[i];
        }
    }
    
    return result;
}

BoolVector BoolVector::logicXOR(const BoolVector& other) const {
    if (bits == 0 || other.bits == 0) return BoolVector();
    
    size_t maxBits = bits > other.bits ? bits : other.bits;
    size_t maxBytes = calculateBytes(maxBits);
    
    BoolVector result;
    result.bits = maxBits;
    result.bytes = maxBytes;
    result.vector = (unsigned char*)malloc(maxBytes);
    
    if (result.vector) {
        memset(result.vector, 0, maxBytes);
        
        size_t copyBytes = bytes < maxBytes ? bytes : maxBytes;
        if (vector) {
            memcpy(result.vector, vector, copyBytes);
        }
        
        size_t otherCopyBytes = other.bytes < maxBytes ? other.bytes : maxBytes;
        for (size_t i = 0; i < otherCopyBytes; i++) {
            result.vector[i] ^= other.vector[i];
        }
    }
    
    return result;
}

void BoolVector::inversion() {
    if (vector && bits > 0) {
        size_t shift = 8 * bytes - bits;
        for (size_t i = 0; i < bytes; i++) {
            vector[i] = ~vector[i];
        }
        unsigned char mask = 255;
        mask = mask << shift;
        vector[bytes - 1] = vector[bytes - 1] & mask;
    }
}

void BoolVector::shiftRight(size_t shift) {
    if (vector && bits > 0) {
        size_t bytesShift = ((shift - 1) / 8) + 1;
        unsigned char carry = 0;
        
        for (size_t i = 0; i < bytes; i++) {
            unsigned char currentByte = vector[i];
            if (i + 1 == bytesShift && bytesShift > 1) {
                vector[i] = (currentByte >> (shift % 8)) | carry;
                carry = currentByte;
            }
            else if (i + 1 > bytesShift && bytesShift > 1) {
                vector[i] = carry;
                carry = currentByte;
            }
            else {
                vector[i] = (currentByte >> shift) | carry;
                carry = currentByte << (8 - shift);
            }
        }
    }
}

void BoolVector::shiftLeft(size_t shift) {
    if (vector && bits > 0) {
        size_t bytesShift = ((shift - 1) / 8) + 1;
        unsigned char carry = 0;
        
        for (int i = bytes - 1; i >= 0; i--) {
            unsigned char currentByte = vector[i];
            if (bytes - 1 - i == bytesShift - 1 && bytesShift > 1) {
                vector[i] = (currentByte << (shift % 8)) | carry;
                carry = currentByte;
            }
            else if (i < (int)bytesShift && bytesShift > 1) {
                vector[i] = carry;
                carry = currentByte;
            }
            else {
                vector[i] = (currentByte << shift) | carry;
                carry = currentByte >> (8 - shift);
            }
        }
    }
}

void BoolVector::setBit(size_t position) {
    if (vector && position < bits) {
        size_t bytePosition = position / 8;
        size_t bitPosition = 7 - (position % 8);
        unsigned char mask = 1 << bitPosition;
        vector[bytePosition] |= mask;
    }
}

void BoolVector::clearBit(size_t position) {
    if (vector && position < bits) {
        size_t bytePosition = position / 8;
        size_t bitPosition = 7 - (position % 8);
        unsigned char mask = ~(1 << bitPosition);
        vector[bytePosition] &= mask;
    }
}

bool BoolVector::isZero() const {
    if (vector && bits > 0) {
        for (size_t i = 0; i < bytes; i++) {
            if (vector[i] != 0) {
                return false;
            }
        }
        return true;
    }
    return true;
}
