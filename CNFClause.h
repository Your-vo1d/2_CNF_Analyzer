#ifndef CNFCLAUSE_H
#define CNFCLAUSE_H

#include "BoolVector.h"
#include <QString>
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

    void addClause(const CNFClause& newClause);
    void resizeAll(size_t newSize);
    void resize(size_t newSize);
};

#endif // CNFCLAUSE_H
