#ifndef IMEMORYOPERATION_H
#define IMEMORYOPERATION_H

#include <QString>
#include <QJsonValue>
#include "Tree.h"

class GraphBuilder;

class IMemoryOperation
{
public:
    virtual ~IMemoryOperation() = default;

    // Returns true if this operation handles the given JSON value
    virtual bool handles(const QJsonValue& value) const = 0;

    virtual void execute(Tree* tree,
                         const QString& varName,
                         int clauseId,
                         const QJsonValue& value,
                         GraphBuilder& builder,
                         bool& hasMemoryLeak) = 0;
};

#endif // IMEMORYOPERATION_H
