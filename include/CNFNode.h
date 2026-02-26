#ifndef CNFNode_H
#define CNFNode_H

#include "BoolVector.h"
#include <QString>
#include <QVariant>
#include <QHash>

class CNFNode {
public:
    BoolVector positiveVars;
    BoolVector negativeVars;
    CNFNode* next;
    int position;

    CNFNode();
    CNFNode(size_t bits);
    CNFNode(const char* posStr, const char* negStr);
    ~CNFNode();
    
    void print() const;
    
    // Новые методы для работы с битами
    void setPositiveBit(size_t position, const QString& varType, size_t& max_bytes);
    void clearPositiveBit(size_t position, const QString& varType, size_t& max_bytes);
    void setNegativeBit(size_t position, const QString& varType, size_t& max_bytes);
    void clearNegativeBit(size_t position, const QString& varType, size_t& max_bytes);
    void delete_clause(size_t bit_position, int& currentPosition, QHash<QString, QHash<QString, QVariant>>& elements, const QString& varName, CNFNode& cnf);
    void addClause(const CNFNode& newClause);
    void resizeAll(size_t newSize);
    void resize(size_t newSize);
    void copyFrom(const CNFNode& other);
    void reset();
};

#endif // CNFNode_H
