#ifndef CNFCLAUSE_H
#define CNFCLAUSE_H

#include "BoolVector.h"
#include <QString>
#include <QVariant>
#include <QHash>

class CNFClause {
public:
    BoolVector positiveVars;
    BoolVector negativeVars;
    CNFClause* next;
    int position;

    CNFClause();
    CNFClause(size_t bits);
    CNFClause(const char* posStr, const char* negStr);
    ~CNFClause();
    
    void print() const;
    
    // Новые методы для работы с битами
    void setPositiveBit(size_t position, const QString& varType, size_t& max_bytes);
    void clearPositiveBit(size_t position, const QString& varType, size_t& max_bytes);
    void setNegativeBit(size_t position, const QString& varType, size_t& max_bytes);
    void clearNegativeBit(size_t position, const QString& varType, size_t& max_bytes);
    void delete_clause(size_t bit_position, int& currentPosition, QHash<QString, QHash<QString, QVariant>>& elements, const QString& varName, CNFClause& cnf);
    void addClause(const CNFClause& newClause);
    void resizeAll(size_t newSize);
    void resize(size_t newSize);
    void copyFrom(const CNFClause& other);
    void reset();
};

#endif // CNFCLAUSE_H
