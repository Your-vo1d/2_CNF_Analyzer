#include "Tree.h"
#include <QDebug>
#include <algorithm>
#include <iostream>

Tree::Tree(const QString& name)
    : name_(name), root_(nullptr)
{

}

Tree::Tree(const Tree& other)
    : name_(other.name_), connections_(other.connections_), root_(nullptr)
{
    // Глубокое копирование элементов
    deepCopyElements(other.elements_);

}

Tree& Tree::operator=(const Tree& other)
{
    if (this != &other) {
        clear();

        name_ = other.name_;
        connections_ = other.connections_;

        // Глубокое копирование элементов
        deepCopyElements(other.elements_);


    }
    return *this;
}

Tree::~Tree()
{
    clear();
}

void Tree::deepCopyElements(const QMap<QString, TreeElement*>& otherElements)
{
    // Очищаем текущие элементы
    clearElements();

    // Копируем элементы
    for (auto it = otherElements.constBegin(); it != otherElements.constEnd(); ++it) {
        TreeElement* newElement = new TreeElement(*(it.value()));
        elements_.insert(it.key(), newElement);

        // Если это корневой элемент, устанавливаем указатель
        if (root_ == it.value()) {
            root_ = newElement;
        }
    }
}

void Tree::setRoot(TreeElement* root)
{
    if (root && elements_.values().contains(root)) {
        root_ = root;

    } else if (root) {

    } else {
        root_ = nullptr;

    }
}

bool Tree::addElement(const QString& name, TreeElement* element)
{
    if (!element || name.isEmpty()) {

        return false;
    }

    if (elements_.contains(name)) {

        return false;
    }

    elements_.insert(name, element);


    // Если это первый элемент, устанавливаем его как корень
    if (elements_.size() == 1 && !root_) {
        root_ = element;

    }

    return true;
}

bool Tree::addElement(const TreeElement& element)
{
    if (element.getName().isEmpty()) {

        return false;
    }

    return addElement(element.getName(), new TreeElement(element));
}

bool Tree::removeElement(const QString& name)
{
    if (!elements_.contains(name)) {
        return false;
    }

    TreeElement* element = elements_.value(name);

    // Удаляем все соединения, связанные с этим элементом
    QList<Connection> connectionsToRemove;
    for (const Connection& conn : connections_) {
        if (conn.getFrom() == name || conn.getTo() == name) {
            connectionsToRemove.append(conn);
        }
    }

    for (const Connection& conn : connectionsToRemove) {
        removeConnection(conn.getFrom(), conn.getTo());
    }

    // Если удаляемый элемент - корень, сбрасываем корень
    if (root_ == element) {
        root_ = nullptr;
    }

    // Удаляем элемент
    delete element;
    elements_.remove(name);

    return true;
}

TreeElement* Tree::findElement(const QString& name) const
{
    return elements_.value(name, nullptr);
}

bool Tree::containsElement(const QString& name) const
{
    return elements_.contains(name);
}

void Tree::clearElements()
{
    // Удаляем все соединения, так как они ссылаются на элементы
    clearConnections();

    // Удаляем все элементы
    for (TreeElement* element : elements_) {
        delete element;
    }
    elements_.clear();
    root_ = nullptr;

}

void Tree::addConnection(const Connection& connection)
{
    // Проверяем существование элементов
    if (!containsElement(connection.getFrom())) {
        return;
    }

    if (!containsElement(connection.getTo())) {
        return;
    }

    // Проверяем, нет ли уже такого соединения
    if (containsConnection(connection.getFrom(), connection.getTo())) {
        return;
    }

    connections_.append(connection);
}

void Tree::addConnection(const QString& from, const QString& to,
                         const BoolVector& positiveVector, const BoolVector& negativeVector)
{
    addConnection(Connection(from, to, positiveVector, negativeVector));
}

bool Tree::removeConnection(const QString& from, const QString& to)
{
    for (int i = 0; i < connections_.size(); ++i) {
        if (connections_[i].getFrom() == from && connections_[i].getTo() == to) {
            connections_.removeAt(i);
            return true;
        }
    }

    return false;
}

void Tree::removeConnection(int index)
{
    if (index >= 0 && index < connections_.size()) {
        Connection conn = connections_.at(index);
        connections_.removeAt(index);
    } else {
    }
}

Connection* Tree::findConnection(const QString& from, const QString& to)
{
    for (int i = 0; i < connections_.size(); ++i) {
        if (connections_[i].getFrom() == from && connections_[i].getTo() == to) {
            return &connections_[i];
        }
    }
    return nullptr;
}

const Connection* Tree::findConnection(const QString& from, const QString& to) const
{
    for (int i = 0; i < connections_.size(); ++i) {
        if (connections_[i].getFrom() == from && connections_[i].getTo() == to) {
            return &connections_[i];
        }
    }
    return nullptr;
}

bool Tree::containsConnection(const QString& from, const QString& to) const
{
    return findConnection(from, to) != nullptr;
}

void Tree::clearConnections()
{
    connections_.clear();
}

bool Tree::isEmpty() const
{
    return elements_.isEmpty();
}

void Tree::clear()
{
    clearConnections();
    clearElements();
    name_.clear();
}

bool Tree::isValid() const
{
    // Дерево считается валидным, если:
    // 1. Есть имя
    // 2. Есть корневой элемент
    // 3. Все соединения ссылаются на существующие элементы
    if (name_.isEmpty() || !root_) {
        return false;
    }

    // Проверяем соединения
    for (const Connection& conn : connections_) {
        if (!containsElement(conn.getFrom()) || !containsElement(conn.getTo())) {
            return false;
        }
    }

    return true;
}

QString Tree::toString() const
{
    QString result;
    result += QString("Tree: '%1'\n").arg(name_);
    result += QString("  Elements: %1\n").arg(elements_.size());
    result += QString("  Connections: %1\n").arg(connections_.size());
    result += QString("  Root: %1\n").arg(root_ ? root_->getName() : "None");
    result += QString("  Valid: %1").arg(isValid() ? "Yes" : "No");
    return result;
}

QList<TreeElement*> Tree::findElementsByType(int type) const
{
    QList<TreeElement*> result;
    for (TreeElement* element : elements_) {
        if (element->getType() == type) {
            result.append(element);
        }
    }
    return result;
}

QList<Connection> Tree::getConnectionsFrom(const QString& elementName) const
{
    QList<Connection> result;
    for (const Connection& conn : connections_) {
        if (conn.getFrom() == elementName) {
            result.append(conn);
        }
    }
    return result;
}

QList<Connection> Tree::getConnectionsTo(const QString& elementName) const
{
    QList<Connection> result;
    for (const Connection& conn : connections_) {
        if (conn.getTo() == elementName) {
            result.append(conn);
        }
    }
    return result;
}

QList<Connection> Tree::getConnectionsInvolving(const QString& elementName) const
{
    QList<Connection> result;

    for (const Connection& conn : connections_) {
        if (conn.getFrom() == elementName || conn.getTo() == elementName) {
            result.append(conn);
        }
    }

    return result;
}

bool Tree::hasConnectionToRoot(const QString& elementName) const
{
    if (!root_) return false;
    QString root_name = root_->getName();

    for (const Connection& conn : connections_) {
        if ((conn.getFrom() == root_name && conn.getTo() == elementName) ||
            (conn.getFrom() == elementName && conn.getTo() == root_name)) {
            return true;
        }
    }

    return false;
}

void Tree::printDebugInfo(int row_number) const
{
    std::cout << "=== Tree: " << name_.toStdString() << " at row " << row_number << " ===" << std::endl;
    std::cout << "Root: " << (root_ ? root_->getName().toStdString() : "None") << std::endl;
    std::cout << "Elements: " << elements_.size() << std::endl;
    std::cout << "Connections: " << connections_.size() << std::endl;

    std::cout << "--- Elements ---" << std::endl;
    for (auto it = elements_.constBegin(); it != elements_.constEnd(); ++it) {
        std::cout << "  " << it.key().toStdString() << " [type:" << it.value()->getType()
                  << ", pos:" << it.value()->getPosition() << "]" << std::endl;
    }

    std::cout << "--- Connections ---" << std::endl;
    for (const Connection& conn : connections_) {
        std::cout << "  " << conn.getFrom().toStdString() << " -> " << conn.getTo().toStdString()
                  << " [pos:" << conn.getPosition() << "]" << std::endl;
    }
    std::cout << "==================" << std::endl;
}
