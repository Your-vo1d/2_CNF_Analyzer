#ifndef BOOLVECTOR_H
#define BOOLVECTOR_H

#include <stddef.h>
#include <stdbool.h>

class BoolVector {
     friend class CNFClause;
private:
    unsigned char* vector;
    size_t bits;
    size_t bytes;

public:
    static size_t calculateBytes(size_t bits);
    BoolVector();
    BoolVector(size_t input_bits);
    ~BoolVector();
    BoolVector(const BoolVector& other);
    BoolVector& operator=(const BoolVector& other);
    const unsigned char* getVector() const { return vector; }
    void print() const;
    void setBit(size_t position);
    void clearBit(size_t position);
    bool isZero() const;
    size_t size() const;
    size_t count_bytes() const;
    void resize(size_t new_size);
     void resizeBytes(size_t new_size);
    bool getBit(size_t position) const {
        if (position >= bits) return false;
        size_t bytePos = position / 8;
        size_t bitPos = 7 - (position % 8); // Старший бит первый
        return (vector[bytePos] >> bitPos) & 1;
    }
};

#endif // BOOLVECTOR_H
