#ifndef BOOLVECTOR_H
#define BOOLVECTOR_H

#include <stddef.h>
#include <stdbool.h>

class BoolVector {
private:
    unsigned char* vector;
    size_t bits;
    size_t bytes;

    size_t calculateBytes(size_t bits) const;

public:
    BoolVector();
    BoolVector(const char* str);
    ~BoolVector();
    BoolVector(const BoolVector& other);
    BoolVector& operator=(const BoolVector& other);

    void print() const;
    BoolVector logicOR(const BoolVector& other) const;
    BoolVector logicAND(const BoolVector& other) const;
    BoolVector logicXOR(const BoolVector& other) const;
    void inversion();
    void shiftRight(size_t shift);
    void shiftLeft(size_t shift);
    void setBit(size_t position);
    void clearBit(size_t position);
    bool isZero() const;
};

#endif // BOOLVECTOR_H