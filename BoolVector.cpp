#include "BoolVector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
size_t BoolVector::calculateBytes(size_t bits){
    if (bits >= 1) {
    return ((bits - 1) / 8) + 1;
    }
    return 0;
}

BoolVector::BoolVector() : vector(nullptr), bits(0), bytes(0) {}

BoolVector::BoolVector(size_t input_bits) {
    bits = input_bits;
    bytes = ((bits - 1) / 8) + 1;
    vector = (unsigned char*)malloc(sizeof(unsigned char) * bytes);

    if (vector) {
        for (size_t i = 0; i < bytes; i++)
            vector[i] = 0;
    }
}


BoolVector::~BoolVector() {
    if (vector) free(vector);
}

BoolVector& BoolVector::operator=(const BoolVector& other) {
    if (this != &other) {
        if (vector) free(vector);
        
        bits = other.bits;
        bytes = other.bytes;
        
        if (other.vector && bytes > 0) {
            vector = (unsigned char*)malloc(sizeof(unsigned char) * bytes);
            if (vector) {
                memcpy(vector, other.vector, bytes);
            }
        } else {
            vector = nullptr;
        }
    }
    return *this;
}

size_t BoolVector::size() const{
    return bits;
}

size_t BoolVector::count_bytes() const{
    return bytes;
}
void BoolVector::print() const {
    if (vector)
    {
        for (size_t i = 0; i < bytes; i++)
        {
            unsigned char mask = 1 << 7; // Создаем маску, начиная с самого левого бита
            // Цикл для обработки каждого бита в байте
            for (size_t j = 0; j < 8; j++)
            {
                if ((vector[i] & mask) != 0)
                    printf("1"); // Выводим "1", если бит установлен
                else
                    printf("0");  // Выводим "0", если бит не установлен
                mask = mask >> 1; // Сдвигаем маску вправо для проверки следующего бита
            }
            printf(" ");
        }
    }
    else
    {
        std::cout << bits <<std::endl;
        printf("Can't print vector."); // Если указатель на вектор пуст, выводим сообщение об ошибке
    }
    printf("\n");
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
    return false;
}

void BoolVector::resizeBytes(size_t nbytes) {
    if (vector)
    {
        size_t old_bytes = bytes;
        size_t new_bytes = nbytes;
        unsigned char *newVector = (unsigned char *)malloc(sizeof(unsigned char) * new_bytes); // выделение памяти под вектор
        if (newVector)
        {
            for (size_t i = 0; i < new_bytes; i++)
            { // Обнуление вектора
                newVector[i] = 0;
            }
            for (size_t i = 0; i < old_bytes; i++)
            { // Перенос значения на другой вектор
                newVector[i] = vector[i];
            }

        }
        vector = newVector;
        bytes = new_bytes;
    }
}

void BoolVector::resize(size_t new_size) {
    if (this->bits == 0) {
        this->vector = (unsigned char *)malloc(sizeof(unsigned char) * new_size);
    }
    if (vector)
    {
        size_t old_bytes = bytes;
        size_t new_bytes = ((new_size - 1) / 8) + 1;
        unsigned char *newVector = (unsigned char *)malloc(sizeof(unsigned char) * new_bytes); // выделение памяти под вектор
        if (newVector)
        {
            for (size_t i = 0; i < new_bytes; i++)
            { // Обнуление вектора
                newVector[i] = 0;
            }
            for (size_t i = 0; i < old_bytes; i++)
            { // Перенос значения на другой вектор
                newVector[i] = vector[i];
            }

        }
        vector = newVector;
        bytes = new_bytes;
        bits = new_size;
    }
}
