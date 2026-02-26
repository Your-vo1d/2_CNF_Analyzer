#include "TreeManager.h"

TreeManager::TreeManager()
    : treeCount_(0)
{
}

TreeManager::~TreeManager()
{
    clear();
}

void TreeManager::addTree(const QString& name, Tree* tree)
{
    if (tree && !name.isEmpty()) {
        if (!trees_.contains(name)) {
            treeCount_++;
        }
        trees_.insert(name, tree);
    }
}

void TreeManager::removeTree(const QString& name)
{
    Tree* tree = trees_.value(name, nullptr);
    if (tree) {
        delete tree;
        trees_.remove(name);
        treeCount_--;
    }
}

Tree* TreeManager::getTree(const QString& name) const
{
    return trees_.value(name, nullptr);
}

bool TreeManager::containsTree(const QString& name) const
{
    return trees_.contains(name);
}

void TreeManager::clear()
{
    for (Tree* tree : trees_) {
        delete tree;
    }
    trees_.clear();
    treeCount_ = 0;
}
