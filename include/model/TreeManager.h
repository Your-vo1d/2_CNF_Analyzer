#ifndef TREEMANAGER_H
#define TREEMANAGER_H

#include <QString>
#include <QMap>
#include "Tree.h"

class TreeManager
{
public:
    TreeManager();
    ~TreeManager();

    // Добавление/удаление деревьев
    void addTree(const QString& name, Tree* tree);
    void removeTree(const QString& name);
    Tree* getTree(const QString& name) const;
    bool containsTree(const QString& name) const;

    // Получение информации
    int getTreeCount() const { return treeCount_; }
    QMap<QString, Tree*> getAllTrees() const { return trees_; }

    // Очистка менеджера
    void clear();

private:
    QMap<QString, Tree*> trees_;  // Словарь: имя дерева -> указатель на дерево
    int treeCount_;               // Счетчик деревьев
};

#endif // TREEMANAGER_H
