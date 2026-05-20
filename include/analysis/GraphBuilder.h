#ifndef GRAPHBUILDER_H
#define GRAPHBUILDER_H

#include <QString>
#include <QList>
#include <QSet>
#include "Tree.h"
#include "TreeElement.h"
#include "Connection.h"
#include "BoolVector.h"
#include "TreeManager.h"

class GraphBuilder
{
public:
    explicit GraphBuilder(TreeManager *manager);

    TreeElement *findOrCreateElement(Tree *tree, const QString &name, int type);
    Connection *findOrCreateConnection(Tree *tree, const QString &from,
                                       const QString &to, int position);
    void removeRootLinkClauses(Tree *tree, const QString &rootName,
                               const QString &varName);
    void mergeTrees(Tree *target, Tree *source);
    QList<QString> findLeaves(Tree *tree) const;
    TreeElement *findMemoryElement(Tree *tree) const;
    static size_t findSetBitPosition(const BoolVector &bv);

    int nextPosition();
    int nextMemoryVarIndex();
    size_t maxBytesCount() const;

private:
    TreeManager *manager_;
    int current_position_;
    int memory_var_index_;
    size_t max_bytes_count_;

    void updateMaxBytes();
};

#endif // GRAPHBUILDER_H
