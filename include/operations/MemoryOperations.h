#ifndef MEMORYOPERATIONS_H
#define MEMORYOPERATIONS_H

#include "IMemoryOperation.h"

class MallocOperation : public IMemoryOperation
{
public:
    bool handles(const QJsonValue& value) const override;
    void execute(Tree* tree, const QString& varName, int clauseId,
                 const QJsonValue& value, GraphBuilder& builder,
                 bool& hasMemoryLeak) override;
};

class FreeOperation : public IMemoryOperation
{
public:
    bool handles(const QJsonValue& value) const override;
    void execute(Tree* tree, const QString& varName, int clauseId,
                 const QJsonValue& value, GraphBuilder& builder,
                 bool& hasMemoryLeak) override;
};

class NullOperation : public IMemoryOperation
{
public:
    bool handles(const QJsonValue& value) const override;
    void execute(Tree* tree, const QString& varName, int clauseId,
                 const QJsonValue& value, GraphBuilder& builder,
                 bool& hasMemoryLeak) override;
};

class SpecialPtrOperation : public IMemoryOperation
{
public:
    bool handles(const QJsonValue& value) const override;
    void execute(Tree* tree, const QString& varName, int clauseId,
                 const QJsonValue& value, GraphBuilder& builder,
                 bool& hasMemoryLeak) override;
};

#endif // MEMORYOPERATIONS_H
