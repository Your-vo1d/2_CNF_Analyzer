#include "Parser_JSON.h"

void Parser_JSON::mergeTrees(Tree *target_tree, Tree *source_tree)
{
    if (!target_tree || !source_tree || target_tree == source_tree)
        return;

    std::cout << "Merging tree '" << source_tree->getName().toStdString()
              << "' into '" << target_tree->getName().toStdString() << "'" << std::endl;

    TreeElement *target_root = target_tree->getRoot();
    if (!target_root)
    {
        std::cout << "Error: Target tree has no root" << std::endl;
        return;
    }

    QString source_root_name = source_tree->getName();

    // 1. убираем связи лист -> корень источника
    QList<Connection> source_connections = source_tree->getConnections();
    QList<Connection> connections_to_remove;
    for (const Connection &conn : source_connections)
    {
        if (conn.getTo() == source_root_name)
        {
            std::cout << "Removing leaf-to-source-root connection: "
                      << conn.getFrom().toStdString() << " -> " << conn.getTo().toStdString() << std::endl;
            connections_to_remove.append(conn);
        }
    }
    for (const Connection &conn : connections_to_remove)
        source_tree->removeConnection(conn.getFrom(), conn.getTo());

    // 2. переносим элементы (корень источника не берём)
    QMap<QString, TreeElement *> source_elements = source_tree->getElements();
    for (auto it = source_elements.constBegin(); it != source_elements.constEnd(); ++it)
    {
        if (it.key() == source_root_name) continue;
        if (!target_tree->findElement(it.key()))
        {
            target_tree->addElement(it.key(), new TreeElement(*(it.value())));
            std::cout << " Copied element: " << it.key().toStdString() << std::endl;
        }
        else
        {
            std::cout << " Element already exists: " << it.key().toStdString() << std::endl;
        }
    }

    // 3. переносим рёбра (кроме тех, что касаются корня источника)
    for (const Connection &conn : source_connections)
    {
        if (conn.getFrom() != source_root_name && conn.getTo() != source_root_name)
        {
            if (!target_tree->findConnection(conn.getFrom(), conn.getTo()))
            {
                target_tree->addConnection(conn);
                std::cout << " Copied connection: " << conn.getFrom().toStdString()
                          << " -> " << conn.getTo().toStdString() << std::endl;
            }
            else
            {
                std::cout << " Connection already exists: " << conn.getFrom().toStdString()
                          << " -> " << conn.getTo().toStdString() << std::endl;
            }
        }
    }

    // 4. листья источника подключаем к корню цели
    QList<QString> source_leaves = findLeaves(source_tree);
    std::cout << " Found " << source_leaves.size() << " leaves in source tree: ";
    for (const QString &leaf : source_leaves) std::cout << leaf.toStdString() << " ";
    std::cout << std::endl;

    for (const QString &leaf_name : source_leaves)
    {
        TreeElement *leaf_element = target_tree->findElement(leaf_name);
        if (leaf_element)
        {
            BoolVector pos_vec(max_bytes_count_ * 8);
            BoolVector neg_vec(max_bytes_count_ * 8);
            pos_vec.setBit(target_root->getPosition());
            neg_vec.setBit(leaf_element->getPosition());
            target_tree->addConnection(Connection(leaf_name, target_tree->getName(), pos_vec, neg_vec, -1));
            std::cout << " Added leaf-to-target-root connection: " << leaf_name.toStdString()
                      << " -> " << target_tree->getName().toStdString() << std::endl;
        }
    }

    // 5. удаляем дерево источника из менеджера
    treeManager_->removeTree(source_root_name);
    std::cout << "Successfully merged trees" << std::endl;
}

QList<QString> Parser_JSON::findLeaves(Tree *tree)
{
    QList<QString> leaves;
    if (!tree) return leaves;

    QMap<QString, TreeElement *> elements = tree->getElements();
    QSet<QString> has_outgoing;
    QList<Connection> connections = tree->getConnections();
    for (const Connection &conn : connections)
        has_outgoing.insert(conn.getFrom());

    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it)
    {
        if (!has_outgoing.contains(it.key()))
        {
            leaves.append(it.key());
            std::cout << " Leaf found: " << it.key().toStdString() << std::endl;
        }
    }
    return leaves;
}

QList<QString> Parser_JSON::findElementsWithRootConnection(Tree *tree)
{
    QList<QString> elements;
    if (!tree) return elements;

    QString root_name = tree->getName();
    QList<Connection> connections = tree->getConnections();
    for (const Connection &conn : connections)
    {
        if (conn.getFrom() == root_name && !elements.contains(conn.getTo()))
            elements.append(conn.getTo());
        if (conn.getTo() == root_name && !elements.contains(conn.getFrom()))
            elements.append(conn.getFrom());
    }
    std::cout << " Elements with root connection in tree '" << root_name.toStdString() << "': ";
    for (const QString &elem : elements) std::cout << elem.toStdString() << " ";
    std::cout << std::endl;
    return elements;
}

TreeElement *Parser_JSON::findMemoryElement(Tree *tree)
{
    if (!tree) return nullptr;

    QMap<QString, TreeElement *> elements = tree->getElements();
    std::cout << "Looking for memory element in tree: " << tree->getName().toStdString() << std::endl;
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it)
    {
        if (it.value()->isMemory())
        {
            std::cout << "Found memory element: " << it.key().toStdString() << std::endl;
            return it.value();
        }
    }
    // запасной вариант: берём первый не-корневой элемент
    QString root_name = tree->getName();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it)
    {
        if (it.key() != root_name)
        {
            std::cout << "Using non-root element as memory: " << it.key().toStdString() << std::endl;
            return it.value();
        }
    }
    std::cout << "No suitable memory element found" << std::endl;
    return nullptr;
}

TreeElement *Parser_JSON::findOrCreateElement(Tree *tree, const QString &name, const QString &type)
{
    TreeElement *element = tree->findElement(name);
    if (!element)
    {
        removeRootLinkClauses(tree, tree->getName(), name);
        int type_code;
        if (type == "Memory")
            type_code = TreeElement::MEMORY;
        else
            type_code = TreeElement::ROOT;
        element = new TreeElement(name, type_code, current_position_++);
        tree->addElement(name, element);
        max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
        std::cout << "Created element: " << name.toStdString()
                  << " type: " << element->getTypeName().toStdString() << std::endl;
    }
    return element;
}

Connection *Parser_JSON::findOrCreateConnection(Tree *tree, const QString &from, const QString &to, int position)
{
    removeRootLinkClauses(tree, tree->getName(), from);
    removeRootLinkClauses(tree, tree->getName(), to);
    Connection *conn = tree->findConnection(from, to);
    if (!conn)
    {
        BoolVector pos_vec(max_bytes_count_ * 8);
        BoolVector neg_vec(max_bytes_count_ * 8);
        tree->addConnection(Connection(from, to, pos_vec, neg_vec, position));
        conn = tree->findConnection(from, to);
    }
    return conn;
}

void Parser_JSON::removeRootLinkClauses(Tree *tree, const QString &root_name, const QString &var_name)
{
    if (!tree) return;
    TreeElement *root_element = tree->getRoot();
    TreeElement *var_element  = tree->findElement(var_name);
    if (!root_element || !var_element) return;

    std::cout << "Checking root connections for: " << var_name.toStdString() << std::endl;
    bool found_connections = false;

    QList<Connection> from_root = tree->getConnectionsFrom(root_name);
    for (const Connection &conn : from_root)
    {
        if (conn.getTo() == var_name)
        {
            tree->removeConnection(conn.getFrom(), conn.getTo());
            std::cout << "Removed root connection: " << root_name.toStdString()
                      << " -> " << var_name.toStdString() << std::endl;
            found_connections = true;
        }
    }

    QList<Connection> to_root = tree->getConnectionsTo(root_name);
    for (const Connection &conn : to_root)
    {
        if (conn.getFrom() == var_name)
        {
            tree->removeConnection(conn.getFrom(), conn.getTo());
            std::cout << "Removed root connection: " << var_name.toStdString()
                      << " -> " << root_name.toStdString() << std::endl;
            found_connections = true;
        }
    }

    if (!found_connections)
        std::cout << "No root connections found for: " << var_name.toStdString() << std::endl;
}

size_t Parser_JSON::findSetBitPosition(const BoolVector &bit_vector)
{
    for (size_t i = 0; i < bit_vector.size(); ++i)
        if (bit_vector.getBit(i)) return i;
    return static_cast<size_t>(-1);
}
