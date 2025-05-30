#include "CNFClause.h"
#include <stdio.h>

CNFClause::CNFClause(const char* posStr, const char* negStr) 
    : positiveVars(posStr), negativeVars(negStr), next(nullptr) {}

CNFClause::~CNFClause() {
    if (next) delete next;
}

void CNFClause::print() const {
    printf("Positive vars: ");
    positiveVars.print();
    printf("Negative vars: ");
    negativeVars.print();
}

// Установка бита в векторе положительных переменных
void CNFClause::setPositiveBit(size_t position) {
    // Если позиция вне текущего размера вектора, увеличиваем его
    if (position >= positiveVars.size()) {
        BoolVector newVector(position + 1, false); // Создаём вектор нужного размера
        // Копируем старые данные
        for (size_t i = 0; i < positiveVars.size(); i++) {
            if (positiveVars.get_bit(i)) {
                newVector.set_bit(i);
            }
        }
        positiveVars = std::move(newVector); // Заменяем старый вектор новым
    }
    positiveVars.set_bit(position); // Устанавливаем бит
}

// Очистка бита в векторе положительных переменных
void CNFClause::clearPositiveBit(size_t position) {
    // Если позиция вне текущего размера, создаём увеличенный вектор
    if (position >= positiveVars.size()) {
        BoolVector newVector(position + 1, false);
        // Копируем старые данные
        for (size_t i = 0; i < positiveVars.size(); i++) {
            if (positiveVars.get_bit(i)) {
                newVector.set_bit(i);
            }
        }
        positiveVars = std::move(newVector);
    }
    positiveVars.clear_bit(position); // Очищаем бит
}

// Установка бита в векторе отрицательных переменных
void CNFClause::setNegativeBit(size_t position) {
    // Аналогично setPositiveBit
    if (position >= negativeVars.size()) {
        BoolVector newVector(position + 1, false);
        for (size_t i = 0; i < negativeVars.size(); i++) {
            if (negativeVars.get_bit(i)) {
                newVector.set_bit(i);
            }
        }
        negativeVars = std::move(newVector);
    }
    negativeVars.set_bit(position);
}

// Очистка бита в векторе отрицательных переменных
void CNFClause::clearNegativeBit(size_t position) {
    // Аналогично clearPositiveBit
    if (position >= negativeVars.size()) {
        BoolVector newVector(position + 1, false);
        for (size_t i = 0; i < negativeVars.size(); i++) {
            if (negativeVars.get_bit(i)) {
                newVector.set_bit(i);
            }
        }
        negativeVars = std::move(newVector);
    }
    negativeVars.clear_bit(position);
}