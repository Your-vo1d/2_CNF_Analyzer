#include "GraphBuilder.h"
#include <iostream>

GraphBuilder::GraphBuilder(TreeManager* manager)
    : manager_(manager),
      current_position_(0),
      memory_var_index_(0),
      max_bytes_count_(1)
{}

int GraphBuilder::nextPosition()
{
    int pos = current_position_++;
    updateMaxBytes();
    return pos;
}

int GraphBuilder::nextMemoryVarIndex()
{
    return memory_var_index_++;
}

size_t GraphBuilder::maxBytesCount() const
{
    return max_bytes_count_;
}

void GraphBuilder::updateMaxBytes()
{
    size_t needed = BoolVector::calculateBytes(current_position_);
    if (needed > max_bytes_count_)
        max_bytes_count_ = needed;
}

TreeElement* GraphBuilder::findOrCreateElement(Tree* tree, const QString& name, int type)
{
    TreeElement* element = tree->findElement(name);
    if (!element) {
        removeRootLinkClauses(tree, tree->getName(), name);
        element = new TreeElement(name, type, nextPosition());
        tree->addElement(name, element);
        std::cout << "Created element: " << name.toStdString()
                  << " type: " << element->getTypeName().toStdString() << std::endl;
    }
    return element;
}

Connection* GraphBuilder::findOrCreateConnection(Tree* tree, const QString& from,
                                                  const QString& to, int position)
{
    removeRootLinkClauses(tree, tree->getName(), from);
    removeRootLinkClauses(tree, tree->getName(), to);
    Connection* conn = tree->findConnection(from, to);
    if (!conn) {
        BoolVector posVec(max_bytes_count_ * 8);
        BoolVector negVec(max_bytes_count_ * 8);
        tree->addConnection(Connection(from, to, posVec, negVec, position));
        conn = tree->findConnection(from, to);
    }
    return conn;
}

void GraphBuilder::removeRootLinkClauses(Tree* tree, const QString& rootName,
                                          const QString& varName)
{
    if (!tree) return;
    if (!tree->getRoot() || !tree->findElement(varName)) return;

    std::cout << "Checking root connections for: " << varName.toStdString() << std::endl;

    QList<Connection> fromRoot = tree->getConnectionsFrom(rootName);
    for (const Connection& conn : fromRoot) {
        if (conn.getTo() == varName) {
            tree->removeConnection(conn.getFrom(), conn.getTo());
            std::cout << "Removed root connection: " << rootName.toStdString()
                      << " -> " << varName.toStdString() << std::endl;
        }
    }

    QList<Connection> toRoot = tree->getConnectionsTo(rootName);
    for (const Connection& conn : toRoot) {
        if (conn.getFrom() == varName) {
            tree->removeConnection(conn.getFrom(), conn.getTo());
            std::cout << "Removed root connection: " << varName.toStdString()
                      << " -> " << rootName.toStdString() << std::endl;
        }
    }
}

void GraphBuilder::mergeTrees(Tree* target, Tree* source)
{
    if (!target || !source || target == source) return;

    std::cout << "Merging tree '" << source->getName().toStdString()
              << "' into '" << target->getName().toStdString() << "'" << std::endl;

    TreeElement* targetRoot = target->getRoot();
    if (!targetRoot) {
        std::cout << "Error: Target tree has no root" << std::endl;
        return;
    }

    const QString sourceRootName = source->getName();

    // Remove leaf-to-source-root connections before copying
    QList<Connection> sourceConnections = source->getConnections();
    QList<Connection> toRemove;
    for (const Connection& conn : sourceConnections)
        if (conn.getTo() == sourceRootName)
            toRemove.append(conn);
    for (const Connection& conn : toRemove)
        source->removeConnection(conn.getFrom(), conn.getTo());

    // Copy elements (skip source root)
    QMap<QString, TreeElement*> sourceElements = source->getElements();
    for (auto it = sourceElements.constBegin(); it != sourceElements.constEnd(); ++it) {
        if (it.key() == sourceRootName) continue;
        if (!target->findElement(it.key())) {
            target->addElement(it.key(), new TreeElement(*(it.value())));
            std::cout << " Copied element: " << it.key().toStdString() << std::endl;
        }
    }

    // Copy edges (skip those touching source root)
    for (const Connection& conn : sourceConnections) {
        if (conn.getFrom() != sourceRootName && conn.getTo() != sourceRootName) {
            if (!target->findConnection(conn.getFrom(), conn.getTo())) {
                target->addConnection(conn);
                std::cout << " Copied connection: " << conn.getFrom().toStdString()
                          << " -> " << conn.getTo().toStdString() << std::endl;
            }
        }
    }

    // Connect source leaves to target root
    QList<QString> leaves = findLeaves(source);
    for (const QString& leafName : leaves) {
        TreeElement* leafEl = target->findElement(leafName);
        if (leafEl) {
            BoolVector posVec(max_bytes_count_ * 8);
            BoolVector negVec(max_bytes_count_ * 8);
            posVec.setBit(targetRoot->getPosition());
            negVec.setBit(leafEl->getPosition());
            target->addConnection(
                Connection(leafName, target->getName(), posVec, negVec, -1));
            std::cout << " Added leaf-to-target-root: " << leafName.toStdString()
                      << " -> " << target->getName().toStdString() << std::endl;
        }
    }

    manager_->removeTree(sourceRootName);
    std::cout << "Successfully merged trees" << std::endl;
}

QList<QString> GraphBuilder::findLeaves(Tree* tree) const
{
    QList<QString> leaves;
    if (!tree) return leaves;

    QSet<QString> hasOutgoing;
    for (const Connection& conn : tree->getConnections())
        hasOutgoing.insert(conn.getFrom());

    QMap<QString, TreeElement*> elements = tree->getElements();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        if (!hasOutgoing.contains(it.key())) {
            leaves.append(it.key());
            std::cout << " Leaf found: " << it.key().toStdString() << std::endl;
        }
    }
    return leaves;
}

TreeElement* GraphBuilder::findMemoryElement(Tree* tree) const
{
    if (!tree) return nullptr;

    QMap<QString, TreeElement*> elements = tree->getElements();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it)
        if (it.value()->isMemory()) return it.value();

    // Fallback: first non-root element
    const QString rootName = tree->getName();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it)
        if (it.key() != rootName) return it.value();

    return nullptr;
}

size_t GraphBuilder::findSetBitPosition(const BoolVector& bv)
{
    for (size_t i = 0; i < bv.size(); ++i)
        if (bv.getBit(i)) return i;
    return static_cast<size_t>(-1);
}
