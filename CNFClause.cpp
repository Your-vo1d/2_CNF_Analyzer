#include "CNFClause.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
CNFClause::CNFClause()
    : positiveVars(),  // Явный вызов конструктора по умолчанию BoolVector
    negativeVars(),  // Явный вызов конструктора по умолчанию BoolVector
    next(nullptr)    // Инициализация указателя как nullptr
{
}


CNFClause::CNFClause(size_t bits)
    : positiveVars(bits),  // Явный вызов конструктора по умолчанию BoolVector
    negativeVars(bits),  // Явный вызов конструктора по умолчанию BoolVector
    next(nullptr)    // Инициализация указателя как nullptr
{
}

CNFClause::~CNFClause() {
    if (next) delete next;
}

void CNFClause::print() const {
    const CNFClause* current = this;  // Начинаем с текущей клаузы
    size_t clause_num = 1;           // Номер текущей клаузы

    while (current != nullptr) {      // Пока не дошли до конца списка
        printf("Clause %zu:\n", clause_num++);
        printf("  Positive vars: ");
        current->positiveVars.print();
        printf("  Negative vars: ");
        current->negativeVars.print();
        printf("\n");

        current = current->next;     // Переходим к следующей клаузе
    }
}

void CNFClause::setPositiveBit(size_t position, const QString& varType, size_t& max_bytes) {
    // Вычисляем требуемые байты
    size_t required_bytes = ((position) / 8) + 1;

    // Определяем новый размер в зависимости от типа
    size_t size_increment = (varType == "variable") ? 1 : 2;
    size_t new_bits = position + size_increment;

    // Проверяем и расширяем при необходимости
    if (required_bytes > positiveVars.bytes || positiveVars.bits == 0) {
        if (required_bytes > max_bytes) {
                    max_bytes = required_bytes;
        }
        positiveVars.resize(new_bits);
        negativeVars.resizeBytes(max_bytes);
    }

    // Устанавливаем бит
    positiveVars.setBit(position);
}

void CNFClause::setNegativeBit(size_t position, const QString& varType, size_t& max_bytes) {
    size_t required_bytes = ((position) / 8) + 1;
    size_t size_increment = (varType == "variable") ? 1 : 2;
    size_t new_bits = position + size_increment;

    if (required_bytes > negativeVars.bytes || negativeVars.bits == 0) {
        if (required_bytes > max_bytes) {
            max_bytes = required_bytes;
        }
        negativeVars.resize(new_bits);
        positiveVars.resizeBytes(max_bytes);
    }

    negativeVars.setBit(position);
}

void CNFClause::clearPositiveBit(size_t position, const QString& varType, size_t& max_bytes) {
    size_t required_bytes = ((position) / 8) + 1;
    size_t size_increment = (varType == "variable") ? 1 : 2;
    size_t new_bits = position + size_increment;

    if (required_bytes > positiveVars.bytes || positiveVars.bits == 0) {
        if (required_bytes > max_bytes) {
            max_bytes = required_bytes;
        }
        positiveVars.resize(new_bits);
        negativeVars.resizeBytes(max_bytes);
    }

    positiveVars.clearBit(position);
}

void CNFClause::clearNegativeBit(size_t position, const QString& varType, size_t& max_bytes) {
    size_t required_bytes = ((position) / 8) + 1;
    size_t size_increment = (varType == "variable") ? 1 : 2;
    size_t new_bits = position + size_increment;

    if (required_bytes > negativeVars.bytes || negativeVars.bits == 0) {
        if (required_bytes > max_bytes) {
            max_bytes = required_bytes;
        }
        negativeVars.resize(new_bits);
                        positiveVars.resizeBytes(max_bytes);
    }

    negativeVars.clearBit(position);
}
// Изменённый метод addClause
void CNFClause::addClause(const CNFClause& newClause) {
    CNFClause* current = this;
    while (current->next != nullptr) {
        current = current->next;
    }
    current->next = new CNFClause();
    // Получаем размеры через const-корректные методы
    size_t posSize = newClause.positiveVars.count_bytes() * 8; // Переводим байты в биты
    size_t negSize = newClause.negativeVars.count_bytes() * 8;

    // Создаём векторы нужного размера
    current->next->positiveVars = BoolVector(posSize);
    current->next->negativeVars = BoolVector(negSize);
    for (size_t i = 0; i < newClause.positiveVars.count_bytes(); i++) {
        current->next->positiveVars.vector[i] = newClause.positiveVars.vector[i];
    }
    for (size_t i = 0; i < newClause.negativeVars.count_bytes(); i++) {
        current->next->negativeVars.vector[i] = newClause.negativeVars.vector[i];
    }
}

// Изменённый метод resizeAll
void CNFClause::resizeAll(size_t newSize) {
    CNFClause* current = this;  // начинаем с текущей клаузы

    while (current != nullptr) {  // пока не дошли до конца списка
        current->positiveVars.resizeBytes(newSize);
        current->negativeVars.resizeBytes(newSize);
        current = current->next;   // переходим к следующей
    }
}


void CNFClause::resize(size_t newSize) {
    positiveVars.resize(newSize);
    negativeVars.resize(newSize);

}
