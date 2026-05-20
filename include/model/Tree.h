#ifndef TREE_H
#define TREE_H

#include <QString>
#include <QMap>
#include <QList>
#include <QSharedPointer>
#include "TreeElement.h"
#include "Connection.h"

class Tree
{
public:
    Tree(const QString& name = QString());
    Tree(const Tree& other);
    Tree& operator=(const Tree& other);
    ~Tree();

    // Геттеры
    QString getName() const { return name_; }
    TreeElement* getRoot() const { return root_; }
    QMap<QString, TreeElement*> getElements() const { return elements_; }
    QList<Connection> getConnections() const { return connections_; }

    // Сеттеры
    void setName(const QString& name) { name_ = name; }
    void setRoot(TreeElement* root);

    // Методы для работы с элементами
    bool addElement(const QString& name, TreeElement* element);
    bool addElement(const TreeElement& element);
    bool removeElement(const QString& name);
    TreeElement* findElement(const QString& name) const;
    bool containsElement(const QString& name) const;
    int getElementCount() const { return elements_.size(); }
    void clearElements();

    // Методы для работы с соединениями
    void addConnection(const Connection& connection);
    void addConnection(const QString& from, const QString& to,
                       const BoolVector& positiveVector, const BoolVector& negativeVector);
    bool removeConnection(const QString& from, const QString& to);
    void removeConnection(int index);
    Connection* findConnection(const QString& from, const QString& to);
    const Connection* findConnection(const QString& from, const QString& to) const;
    bool containsConnection(const QString& from, const QString& to) const;
    int getConnectionCount() const { return connections_.size(); }
    void clearConnections();

    // Методы для работы с деревом в целом
    bool isEmpty() const;
    void clear();
    bool isValid() const;
    QString toString() const;

    // Поиск и обход
    QList<TreeElement*> findElementsByType(int type) const;
    QList<Connection> getConnectionsFrom(const QString& elementName) const;
    QList<Connection> getConnectionsTo(const QString& elementName) const;

    // Получить все соединения, где указанный элемент является отправителем или получателем
    QList<Connection> getConnectionsInvolving(const QString& elementName) const;

    // Проверить, есть ли связь с корнем у элемента
    bool hasConnectionToRoot(const QString& elementName) const;
private:
    QString name_;
    QMap<QString, TreeElement*> elements_;  // Словарь: имя элемента -> указатель на элемент
    QList<Connection> connections_;         // Список соединений
    TreeElement* root_;                     // Указатель на корневой элемент

    void printDebugInfo(int row_number) const;
    void deepCopyElements(const QMap<QString, TreeElement*>& otherElements);
};

#endif // TREE_H
