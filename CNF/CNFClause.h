#ifndef CNFCLAUSE_H
#define CNFCLAUSE_H

#include "BoolVector.h"

class CNFClause {
public:
    BoolVector positiveVars;
    BoolVector negativeVars;
    CNFClause* next;

    CNFClause();
    CNFClause(const char* posStr, const char* negStr);
    ~CNFClause();
    
    void print() const;
    
    // Новые методы для работы с битами
    void setPositiveBit(size_t position);
    void clearPositiveBit(size_t position);
    void setNegativeBit(size_t position);
    void clearNegativeBit(size_t position);

    void addClause(const CNFClause& newClause);
};

#endif // CNFCLAUSE_H
