#include "Parser_JSON.h"

// соединяет лист с корнем дерева
static void addLeafToRoot(Tree *t,
                          const QString &leafName,
                          TreeElement *leafEl,
                          TreeElement *rootEl,
                          size_t maxBits,
                          int clause_id)
{
    BoolVector p(maxBits);
    BoolVector n(maxBits);
    p.setBit(rootEl->getPosition());
    n.setBit(leafEl->getPosition());
    Connection c(leafName, t->getName(), p, n, clause_id);
    t->addConnection(c);
}

void Parser_JSON::handleMemoryAllocation(Tree *current_tree,
                                         const QString &var_name,
                                         int clause_id,
                                         const QString &operation_type)
{
    TreeElement *root = current_tree->getRoot();
    if (!root)
    {
        has_error_ = true;
        return;
    }

    TreeElement *element = current_tree->findElement(var_name);
    if (!element)
    {
        element = new TreeElement(var_name, TreeElement::ROOT, current_position_++);
        current_tree->addElement(var_name, element);
        max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
    }

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    QList<Connection> prev_connections = current_tree->getConnectionsFrom(var_name);
    for (const Connection &prev_conn : prev_connections)
    {
        current_tree->removeConnection(prev_conn.getFrom(), prev_conn.getTo());
        std::cout << "Removed previous allocation connection: "
                  << prev_conn.getFrom().toStdString() << " -> "
                  << prev_conn.getTo().toStdString() << std::endl;
    }

    // создаём узел памяти
    QString mem_name = "M" + QString::number(memory_var_index_++);
    TreeElement *mem_element = new TreeElement(mem_name, TreeElement::MEMORY, current_position_++);
    current_tree->addElement(mem_name, mem_element);
    max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));

    removeRootLinkClauses(current_tree, current_tree->getName(), mem_name);

    // переменная -> блок памяти
    {
        BoolVector pos_vec(max_bytes_count_ * 8);
        BoolVector neg_vec(max_bytes_count_ * 8);
        pos_vec.setBit(mem_element->getPosition());
        neg_vec.setBit(element->getPosition());
        current_tree->addConnection(Connection(var_name, mem_name, pos_vec, neg_vec, clause_id));
    }

    // создаём поля left/right если их ещё нет
    auto ensureField = [&](const QString &suffix, int typeEnum) -> QString
    {
        const QString field_name = mem_name + "." + suffix;
        TreeElement *field_el = current_tree->findElement(field_name);
        if (!field_el)
        {
            field_el = new TreeElement(field_name, typeEnum, current_position_++);
            current_tree->addElement(field_name, field_el);
            max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));

            BoolVector p(max_bytes_count_ * 8);
            BoolVector n(max_bytes_count_ * 8);
            p.setBit(field_el->getPosition());
            n.setBit(mem_element->getPosition());
            current_tree->addConnection(Connection(mem_name, field_name, p, n, clause_id));

            removeRootLinkClauses(current_tree, current_tree->getName(), field_name);
            std::cout << "Auto-created field: " << field_name.toStdString()
                      << " and connection " << mem_name.toStdString()
                      << " -> " << field_name.toStdString() << std::endl;
        }
        return field_name;
    };

    const QString leftField  = ensureField("left",  TreeElement::LEFT_VARIABLE);
    const QString rightField = ensureField("right", TreeElement::RIGHT_VARIABLE);

    // если поля есть, связь mem -> root лишняя
    current_tree->removeConnection(mem_name, current_tree->getName());

    TreeElement *leftEl  = current_tree->findElement(leftField);
    TreeElement *rightEl = current_tree->findElement(rightField);
    if (leftEl)
        addLeafToRoot(current_tree, leftField,  leftEl,  root, max_bytes_count_ * 8, clause_id);
    if (rightEl)
        addLeafToRoot(current_tree, rightField, rightEl, root, max_bytes_count_ * 8, clause_id);

    std::cout << "Allocated memory (" << operation_type.toStdString() << "): "
              << var_name.toStdString() << " -> " << mem_name.toStdString()
              << " at clause " << clause_id << std::endl;
}

void Parser_JSON::removeMemoryKeepChildren(Tree *t, const QString &memName)
{
    if (!t) return;

    const QString L = memName + ".left";
    const QString R = memName + ".right";

    auto removeAllEdgesOf = [&](const QString &node)
    {
        QList<Connection> out = t->getConnectionsFrom(node);
        for (const Connection &c : out)
            t->removeConnection(c.getFrom(), c.getTo());
        QList<Connection> in = t->getConnectionsTo(node);
        for (const Connection &c : in)
            t->removeConnection(c.getFrom(), c.getTo());
    };

    removeAllEdgesOf(memName);
    removeAllEdgesOf(L);
    removeAllEdgesOf(R);

    t->removeElement(L);
    t->removeElement(R);
    t->removeElement(memName);

    std::cout << "  Removed memory block (keep children): " << memName.toStdString()
              << " (and fields " << L.toStdString() << ", " << R.toStdString() << ")\n";
}

void Parser_JSON::handleFree(Tree *current_tree, const QString &var_name)
{
    TreeElement *element = current_tree->findElement(var_name);
    if (!element)
    {
        has_error_ = true;
        std::cout << "Error: Element '" << var_name.toStdString() << "' not found for Free" << std::endl;
        return;
    }

    TreeElement *root = current_tree->getRoot();
    if (!root)
    {
        has_error_ = true;
        std::cout << "Error: Root not found for Free" << std::endl;
        return;
    }

    const bool is_field = (var_name.endsWith(".left") || var_name.endsWith(".right"));
    const QString parent_name = is_field ? var_name.left(var_name.lastIndexOf('.')) : QString();

    std::cout << "Executing Free for: " << var_name.toStdString() << std::endl;

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    QList<Connection> outgoing_connections = current_tree->getConnectionsFrom(var_name);
    for (const Connection &conn : outgoing_connections)
    {
        const QString to = conn.getTo();
        std::cout << " Removing outgoing connection: " << conn.getFrom().toStdString()
                  << " -> " << to.toStdString() << std::endl;
        current_tree->removeConnection(conn.getFrom(), conn.getTo());

        TreeElement *target_element = current_tree->findElement(to);
        if (target_element && target_element->isMemory())
            removeMemoryKeepChildren(current_tree, to);
    }

    QList<Connection> incoming_connections = current_tree->getConnectionsTo(var_name);
    for (const Connection &conn : incoming_connections)
    {
        if (is_field && conn.getFrom() == parent_name)
            continue;
        std::cout << " Removing incoming connection: " << conn.getFrom().toStdString()
                  << " -> " << conn.getTo().toStdString() << std::endl;
        current_tree->removeConnection(conn.getFrom(), conn.getTo());
    }

    // после free переменная становится NULL — восстанавливаем связь с корнем
    auto ensureLeafToRoot = [&](const QString &leaf_name)
    {
        QList<Connection> outs = current_tree->getConnectionsFrom(leaf_name);
        for (const Connection &c : outs)
            if (c.getTo() == current_tree->getName()) return;

        BoolVector p(max_bytes_count_ * 8);
        BoolVector n(max_bytes_count_ * 8);
        p.setBit(root->getPosition());
        n.setBit(element->getPosition());
        current_tree->addConnection(Connection(leaf_name, current_tree->getName(), p, n, -1));
        std::cout << "  Restored leaf->root: " << leaf_name.toStdString()
                  << " -> " << current_tree->getName().toStdString() << std::endl;
    };

    if (is_field)
    {
        ensureLeafToRoot(var_name);
        std::cout << "  Field preserved and set to NULL (no outgoing edge): "
                  << var_name.toStdString() << std::endl;
    }
    else
    {
        if (var_name != current_tree->getName())
        {
            current_tree->removeElement(var_name);
            std::cout << "  Removed element: " << var_name.toStdString() << std::endl;
        }
        else
        {
            std::cout << "  Skipped removal of root element: " << var_name.toStdString() << std::endl;
        }
    }

    std::cout << "Free completed for: " << var_name.toStdString() << std::endl;
}

void Parser_JSON::handleNullAssignment(Tree *current_tree, const QString &var_name, int clause_id)
{
    TreeElement *element = current_tree->findElement(var_name);
    if (!element)
    {
        has_error_ = true;
        std::cout << "Error: Element '" << var_name.toStdString() << "' not found for Null assignment" << std::endl;
        return;
    }

    std::cout << "Executing Null assignment for: " << var_name.toStdString() << std::endl;

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    // удаляем старые исходящие рёбра; если указывали на память — это утечка
    QList<Connection> prev_out = current_tree->getConnectionsFrom(var_name);
    for (const Connection &conn : prev_out)
    {
        TreeElement *target = current_tree->findElement(conn.getTo());
        if (target && target->isMemory())
        {
            std::cout << "MEMORY LEAK: '" << var_name.toStdString()
                      << "' pointed to memory '" << target->getName().toStdString()
                      << "' but is being set to NULL without free" << std::endl;
            has_memory_leak_ = true;
        }
        current_tree->removeConnection(conn.getFrom(), conn.getTo());
    }

    QString null_name = "N" + QString::number(memory_var_index_++);
    TreeElement *null_element = new TreeElement(null_name, TreeElement::NULL_ELEMENT, current_position_++);
    current_tree->addElement(null_name, null_element);

    removeRootLinkClauses(current_tree, current_tree->getName(), null_name);

    BoolVector pos_vec(max_bytes_count_ * 8);
    BoolVector neg_vec(max_bytes_count_ * 8);
    pos_vec.setBit(null_element->getPosition());
    neg_vec.setBit(element->getPosition());
    current_tree->addConnection(Connection(var_name, null_name, pos_vec, neg_vec, clause_id));

    TreeElement *root_element = current_tree->getRoot();
    if (root_element)
    {
        BoolVector rp(max_bytes_count_ * 8);
        BoolVector rn(max_bytes_count_ * 8);
        rp.setBit(root_element->getPosition());
        rn.setBit(null_element->getPosition());
        current_tree->addConnection(Connection(null_name, current_tree->getName(), rp, rn, clause_id));
    }

    max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
    std::cout << "Null assignment: " << var_name.toStdString() << " -> " << null_name.toStdString()
              << " at clause " << clause_id << std::endl;
}

void Parser_JSON::handleSpecialPointerAssignment(Tree *current_tree,
                                                 const QString &var_name,
                                                 int clause_id,
                                                 uint32_t magicValue)
{
    TreeElement *element = current_tree->findElement(var_name);
    if (!element)
    {
        has_error_ = true;
        std::cout << "Error: Element '" << var_name.toStdString() << "' not found for special pointer assignment" << std::endl;
        return;
    }

    std::cout << "Executing Special Pointer (0x" << std::hex << magicValue << std::dec
              << ") assignment for: " << var_name.toStdString() << std::endl;

    // если переменная указывала на память, а мы её перезаписываем — утечка
    QList<Connection> prev_out = current_tree->getConnectionsFrom(var_name);
    for (const Connection &conn : prev_out)
    {
        TreeElement *target = current_tree->findElement(conn.getTo());
        if (target && target->isMemory())
        {
            std::cout << "MEMORY LEAK DETECTED: '" << var_name.toStdString()
                      << "' pointed to memory block '" << target->getName().toStdString()
                      << "' but is now overwritten with 0x" << std::hex << magicValue << std::dec << std::endl;
            has_memory_leak_ = true;
        }
    }

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);
    for (const Connection &prev : prev_out)
        current_tree->removeConnection(prev.getFrom(), prev.getTo());

    QString marker_name = "SPEC_" + QString::number(magicValue, 16).toUpper()
                          + "_" + QString::number(memory_var_index_++);
    TreeElement *marker_element = new TreeElement(marker_name, TreeElement::NULL_ELEMENT, current_position_++);
    current_tree->addElement(marker_name, marker_element);
    removeRootLinkClauses(current_tree, current_tree->getName(), marker_name);

    BoolVector pos_vec(max_bytes_count_ * 8);
    BoolVector neg_vec(max_bytes_count_ * 8);
    pos_vec.setBit(marker_element->getPosition());
    neg_vec.setBit(element->getPosition());
    current_tree->addConnection(Connection(var_name, marker_name, pos_vec, neg_vec, clause_id));

    TreeElement *root_element = current_tree->getRoot();
    if (root_element)
    {
        BoolVector rp(max_bytes_count_ * 8), rn(max_bytes_count_ * 8);
        rp.setBit(root_element->getPosition());
        rn.setBit(marker_element->getPosition());
        current_tree->addConnection(Connection(marker_name, current_tree->getName(), rp, rn, clause_id));
    }

    max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
    std::cout << "Special pointer assigned: " << var_name.toStdString()
              << " -> " << marker_name.toStdString() << std::endl;
}

void Parser_JSON::removeMemoryBlock(Tree *t, const QString &mem_name)
{
    if (!t) return;

    QList<Connection> out = t->getConnectionsFrom(mem_name);
    for (const Connection &c : out)
        t->removeConnection(c.getFrom(), c.getTo());

    QList<Connection> in = t->getConnectionsTo(mem_name);
    for (const Connection &c : in)
        t->removeConnection(c.getFrom(), c.getTo());

    const QString l = mem_name + ".left";
    const QString r = mem_name + ".right";

    QList<Connection> outL = t->getConnectionsFrom(l);
    for (const Connection &c : outL) t->removeConnection(c.getFrom(), c.getTo());
    QList<Connection> inL  = t->getConnectionsTo(l);
    for (const Connection &c : inL)  t->removeConnection(c.getFrom(), c.getTo());

    QList<Connection> outR = t->getConnectionsFrom(r);
    for (const Connection &c : outR) t->removeConnection(c.getFrom(), c.getTo());
    QList<Connection> inR  = t->getConnectionsTo(r);
    for (const Connection &c : inR)  t->removeConnection(c.getFrom(), c.getTo());

    t->removeElement(l);
    t->removeElement(r);
    t->removeConnection(mem_name, t->getName());
    t->removeElement(mem_name);
}
