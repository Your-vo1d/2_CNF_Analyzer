#include "MemoryOperations.h"
#include "GraphBuilder.h"
#include <iostream>

// ─── Shared helpers ─────────────────────────────────────────────────────────

static const QStringList ALLOC_OPS = {
    "malloc", "calloc", "realloc", "aligned_alloc", "new", "new[]"
};
static const QStringList FREE_OPS = { "free", "delete", "delete[]" };

static void addLeafToRoot(Tree* t, const QString& leafName,
                           TreeElement* leafEl, TreeElement* rootEl,
                           size_t maxBits, int clauseId)
{
    BoolVector p(maxBits), n(maxBits);
    p.setBit(rootEl->getPosition());
    n.setBit(leafEl->getPosition());
    t->addConnection(Connection(leafName, t->getName(), p, n, clauseId));
}

static void removeAllEdgesOf(Tree* t, const QString& node)
{
    for (const Connection& c : t->getConnectionsFrom(node))
        t->removeConnection(c.getFrom(), c.getTo());
    for (const Connection& c : t->getConnectionsTo(node))
        t->removeConnection(c.getFrom(), c.getTo());
}

static void removeMemoryKeepChildren(Tree* t, const QString& memName)
{
    const QString L = memName + ".left";
    const QString R = memName + ".right";
    removeAllEdgesOf(t, memName);
    removeAllEdgesOf(t, L);
    removeAllEdgesOf(t, R);
    t->removeElement(L);
    t->removeElement(R);
    t->removeElement(memName);
    std::cout << "  Removed memory block: " << memName.toStdString()
              << " (and fields " << L.toStdString() << ", " << R.toStdString() << ")\n";
}

// ─── MallocOperation ────────────────────────────────────────────────────────

bool MallocOperation::handles(const QJsonValue& value) const
{
    return value.isString() && ALLOC_OPS.contains(value.toString());
}

void MallocOperation::execute(Tree* tree, const QString& varName, int clauseId,
                               const QJsonValue& value, GraphBuilder& builder,
                               bool& /*hasMemoryLeak*/)
{
    const QString operationType = value.toString();
    TreeElement* root = tree->getRoot();
    if (!root) return;

    TreeElement* element = tree->findElement(varName);
    if (!element) {
        element = new TreeElement(varName, TreeElement::ROOT, builder.nextPosition());
        tree->addElement(varName, element);
    }

    builder.removeRootLinkClauses(tree, tree->getName(), varName);

    QList<Connection> prevConnections = tree->getConnectionsFrom(varName);
    for (const Connection& c : prevConnections) {
        tree->removeConnection(c.getFrom(), c.getTo());
        std::cout << "Removed previous allocation connection: "
                  << c.getFrom().toStdString() << " -> " << c.getTo().toStdString() << std::endl;
    }

    // Create memory node
    const QString memName = "M" + QString::number(builder.nextMemoryVarIndex());
    TreeElement* memElement = new TreeElement(memName, TreeElement::MEMORY,
                                               builder.nextPosition());
    tree->addElement(memName, memElement);
    builder.removeRootLinkClauses(tree, tree->getName(), memName);

    // varName → memName
    {
        size_t bits = builder.maxBytesCount() * 8;
        BoolVector posVec(bits), negVec(bits);
        posVec.setBit(memElement->getPosition());
        negVec.setBit(element->getPosition());
        tree->addConnection(Connection(varName, memName, posVec, negVec, clauseId));
    }

    // Ensure left/right fields
    auto ensureField = [&](const QString& suffix, int typeEnum) -> QString {
        const QString fieldName = memName + "." + suffix;
        TreeElement* fieldEl = tree->findElement(fieldName);
        if (!fieldEl) {
            fieldEl = new TreeElement(fieldName, typeEnum, builder.nextPosition());
            tree->addElement(fieldName, fieldEl);

            size_t bits = builder.maxBytesCount() * 8;
            BoolVector p(bits), n(bits);
            p.setBit(fieldEl->getPosition());
            n.setBit(memElement->getPosition());
            tree->addConnection(Connection(memName, fieldName, p, n, clauseId));
            builder.removeRootLinkClauses(tree, tree->getName(), fieldName);
            std::cout << "Auto-created field: " << fieldName.toStdString() << std::endl;
        }
        return fieldName;
    };

    const QString leftField  = ensureField("left",  TreeElement::LEFT_VARIABLE);
    const QString rightField = ensureField("right", TreeElement::RIGHT_VARIABLE);

    tree->removeConnection(memName, tree->getName());

    size_t bits = builder.maxBytesCount() * 8;
    TreeElement* leftEl  = tree->findElement(leftField);
    TreeElement* rightEl = tree->findElement(rightField);
    if (leftEl)  addLeafToRoot(tree, leftField,  leftEl,  root, bits, clauseId);
    if (rightEl) addLeafToRoot(tree, rightField, rightEl, root, bits, clauseId);

    std::cout << "Allocated memory (" << operationType.toStdString() << "): "
              << varName.toStdString() << " -> " << memName.toStdString()
              << " at clause " << clauseId << std::endl;
}

// ─── FreeOperation ──────────────────────────────────────────────────────────

bool FreeOperation::handles(const QJsonValue& value) const
{
    return value.isString() && FREE_OPS.contains(value.toString());
}

void FreeOperation::execute(Tree* tree, const QString& varName, int /*clauseId*/,
                             const QJsonValue& /*value*/, GraphBuilder& builder,
                             bool& /*hasMemoryLeak*/)
{
    TreeElement* element = tree->findElement(varName);
    if (!element) {
        std::cout << "Error: Element '" << varName.toStdString() << "' not found for Free" << std::endl;
        return;
    }
    TreeElement* root = tree->getRoot();
    if (!root) return;

    const bool isField = (varName.endsWith(".left") || varName.endsWith(".right"));
    const QString parentName = isField ? varName.left(varName.lastIndexOf('.')) : QString();

    std::cout << "Executing Free for: " << varName.toStdString() << std::endl;
    builder.removeRootLinkClauses(tree, tree->getName(), varName);

    QList<Connection> outgoing = tree->getConnectionsFrom(varName);
    for (const Connection& conn : outgoing) {
        const QString to = conn.getTo();
        tree->removeConnection(conn.getFrom(), conn.getTo());
        TreeElement* target = tree->findElement(to);
        if (target && target->isMemory())
            removeMemoryKeepChildren(tree, to);
    }

    QList<Connection> incoming = tree->getConnectionsTo(varName);
    for (const Connection& conn : incoming) {
        if (isField && conn.getFrom() == parentName) continue;
        tree->removeConnection(conn.getFrom(), conn.getTo());
    }

    // After free, variable becomes NULL — restore leaf-to-root link
    auto ensureLeafToRoot = [&](const QString& leafName) {
        for (const Connection& c : tree->getConnectionsFrom(leafName))
            if (c.getTo() == tree->getName()) return;
        size_t bits = builder.maxBytesCount() * 8;
        BoolVector p(bits), n(bits);
        p.setBit(root->getPosition());
        n.setBit(element->getPosition());
        tree->addConnection(Connection(leafName, tree->getName(), p, n, -1));
        std::cout << "  Restored leaf->root: " << leafName.toStdString()
                  << " -> " << tree->getName().toStdString() << std::endl;
    };

    if (isField) {
        ensureLeafToRoot(varName);
    } else {
        if (varName != tree->getName())
            tree->removeElement(varName);
    }

    std::cout << "Free completed for: " << varName.toStdString() << std::endl;
}

// ─── NullOperation ──────────────────────────────────────────────────────────

bool NullOperation::handles(const QJsonValue& value) const
{
    return value.isNull();
}

void NullOperation::execute(Tree* tree, const QString& varName, int clauseId,
                             const QJsonValue& /*value*/, GraphBuilder& builder,
                             bool& hasMemoryLeak)
{
    TreeElement* element = tree->findElement(varName);
    if (!element) {
        std::cout << "Error: Element '" << varName.toStdString()
                  << "' not found for Null assignment" << std::endl;
        return;
    }

    std::cout << "Executing Null assignment for: " << varName.toStdString() << std::endl;
    builder.removeRootLinkClauses(tree, tree->getName(), varName);

    QList<Connection> prevOut = tree->getConnectionsFrom(varName);
    for (const Connection& conn : prevOut) {
        TreeElement* target = tree->findElement(conn.getTo());
        if (target && target->isMemory()) {
            std::cout << "MEMORY LEAK: '" << varName.toStdString()
                      << "' pointed to memory '" << target->getName().toStdString()
                      << "' but is being set to NULL without free" << std::endl;
            hasMemoryLeak = true;
        }
        tree->removeConnection(conn.getFrom(), conn.getTo());
    }

    const QString nullName = "N" + QString::number(builder.nextMemoryVarIndex());
    TreeElement* nullEl = new TreeElement(nullName, TreeElement::NULL_ELEMENT,
                                           builder.nextPosition());
    tree->addElement(nullName, nullEl);
    builder.removeRootLinkClauses(tree, tree->getName(), nullName);

    size_t bits = builder.maxBytesCount() * 8;
    {
        BoolVector posVec(bits), negVec(bits);
        posVec.setBit(nullEl->getPosition());
        negVec.setBit(element->getPosition());
        tree->addConnection(Connection(varName, nullName, posVec, negVec, clauseId));
    }

    TreeElement* rootEl = tree->getRoot();
    if (rootEl) {
        BoolVector rp(bits), rn(bits);
        rp.setBit(rootEl->getPosition());
        rn.setBit(nullEl->getPosition());
        tree->addConnection(Connection(nullName, tree->getName(), rp, rn, clauseId));
    }

    std::cout << "Null assignment: " << varName.toStdString()
              << " -> " << nullName.toStdString() << std::endl;
}

// ─── SpecialPtrOperation ────────────────────────────────────────────────────

bool SpecialPtrOperation::handles(const QJsonValue& value) const
{
    if (!value.isDouble()) return false;
    double v = value.toDouble();
    return (v == 4294967295.0 || v == 0.0);
}

void SpecialPtrOperation::execute(Tree* tree, const QString& varName, int clauseId,
                                   const QJsonValue& value, GraphBuilder& builder,
                                   bool& hasMemoryLeak)
{
    TreeElement* element = tree->findElement(varName);
    if (!element) {
        std::cout << "Error: Element '" << varName.toStdString()
                  << "' not found for special pointer assignment" << std::endl;
        return;
    }

    const uint32_t magicValue = static_cast<uint32_t>(value.toDouble());
    std::cout << "Executing Special Pointer (0x" << std::hex << magicValue << std::dec
              << ") assignment for: " << varName.toStdString() << std::endl;

    QList<Connection> prevOut = tree->getConnectionsFrom(varName);
    for (const Connection& conn : prevOut) {
        TreeElement* target = tree->findElement(conn.getTo());
        if (target && target->isMemory()) {
            std::cout << "MEMORY LEAK DETECTED: '" << varName.toStdString()
                      << "' pointed to memory block '" << target->getName().toStdString()
                      << "' but is now overwritten with 0x"
                      << std::hex << magicValue << std::dec << std::endl;
            hasMemoryLeak = true;
        }
    }

    builder.removeRootLinkClauses(tree, tree->getName(), varName);
    for (const Connection& prev : prevOut)
        tree->removeConnection(prev.getFrom(), prev.getTo());

    const QString markerName = "SPEC_" + QString::number(magicValue, 16).toUpper()
                               + "_" + QString::number(builder.nextMemoryVarIndex());
    TreeElement* markerEl = new TreeElement(markerName, TreeElement::NULL_ELEMENT,
                                             builder.nextPosition());
    tree->addElement(markerName, markerEl);
    builder.removeRootLinkClauses(tree, tree->getName(), markerName);

    size_t bits = builder.maxBytesCount() * 8;
    {
        BoolVector posVec(bits), negVec(bits);
        posVec.setBit(markerEl->getPosition());
        negVec.setBit(element->getPosition());
        tree->addConnection(Connection(varName, markerName, posVec, negVec, clauseId));
    }

    TreeElement* rootEl = tree->getRoot();
    if (rootEl) {
        BoolVector rp(bits), rn(bits);
        rp.setBit(rootEl->getPosition());
        rn.setBit(markerEl->getPosition());
        tree->addConnection(Connection(markerName, tree->getName(), rp, rn, clauseId));
    }

    std::cout << "Special pointer assigned: " << varName.toStdString()
              << " -> " << markerName.toStdString() << std::endl;
}
