#ifndef ROWPROCESSOR_H
#define ROWPROCESSOR_H

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QString>
#include "Tree.h"
#include "TreeManager.h"
#include "GraphBuilder.h"
#include "IMemoryOperation.h"

class RowProcessor
{
public:
    RowProcessor(TreeManager* manager,
                 GraphBuilder* builder,
                 QList<IMemoryOperation*> operations);

    void setFunctions(const QJsonObject& functions, const QJsonObject& stacks);

    void processRow(const QJsonObject& row, Tree*& currentTree, int clauseId);

    bool hasError() const      { return has_error_; }
    bool hasMemoryLeak() const { return has_memory_leak_; }

private:
    TreeManager* manager_;
    GraphBuilder* builder_;
    QList<IMemoryOperation*> operations_;
    QJsonObject functions_;
    QJsonObject stacks_;
    bool has_error_;
    bool has_memory_leak_;

    Tree* getOrCreateTree(const QString& name);

    void processCallRow(const QJsonObject& row, Tree*& currentTree, int clauseId);
    QJsonValue substituteParams(const QJsonValue& v, const QJsonArray& params) const;
    QString substituteParamsInString(const QString& s, const QJsonArray& params) const;

    bool tryProcessArrowExpression(Tree*& currentTree, const QString& varExpr,
                                   const QJsonObject& row, int clauseId);

    void processBody(const QJsonValue& bodyValue, Tree* currentTree, int clauseId);
    void processBranch(const QJsonObject& branchObj, Tree* currentTree, int clauseId);
    void processOperation(const QJsonObject& opObject, Tree* currentTree, int clauseId);
    void processLoop(const QJsonObject& loopObject, Tree* currentTree, int clauseId);

    void processStructureField(const QJsonValue& fieldValue, Tree* currentTree,
                               const QString& varName, int clauseId);
    QString navigateByField(Tree* tree, const QString& currentName,
                            const QString& fieldDirection, int clauseId = -1);
    void processNestedValue(const QJsonObject& valueObj, Tree* currentTree,
                            const QString& varName, int clauseId);
    void processValueField(const QJsonValue& fieldValue, Tree* currentTree,
                           QString& lastName, int nestingLevel);
    void processNestedVariable(QString& currentName, const QString& fieldDirection);

    // Dispatches to the first IMemoryOperation whose handles() returns true
    void dispatchMemoryOp(Tree* tree, const QString& varName, int clauseId,
                          const QJsonValue& value);
};

#endif // ROWPROCESSOR_H
