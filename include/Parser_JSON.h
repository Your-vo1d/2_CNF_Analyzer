#ifndef PARSER_JSON_H
#define PARSER_JSON_H

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSet>
#include <iostream>
#include <minisat/core/Solver.h>
#include <stdexcept>

#include "Tree.h"
#include "TreeManager.h"
#include "Connection.h"
#include "BoolVector.h"

class Parser_JSON
{
public:
    Parser_JSON();
    ~Parser_JSON();

    // Основной метод парсинга
    bool parseJsonFile(const QString& file_path);

    // Получение результатов
    TreeManager* getTreeManager() const { return treeManager_; }
    bool hasError() const { return has_error_; }
    bool hasMemoryLeak() const { return has_memory_leak_; }

    // Вспомогательные методы
    void printElements() const;
    void printCNFAsVectors() const;
    bool solveCNF(Tree* tree);

private:
    // Основные структуры данных
    TreeManager* treeManager_;
    bool has_error_;
    bool has_memory_leak_;

    // Временные переменные для парсинга
    int current_position_;
    int memory_var_index_;
    size_t max_bytes_count_;

    // Вспомогательные методы парсинга
    void processBody(const QJsonValue& body_value, Tree* current_tree, int clause_id);
    void processBranch(const QJsonObject& branch_obj, Tree* current_tree, int clause_id);
    void processOperation(const QJsonObject& op_object, Tree* current_tree, int clause_id);
    void processLoop(const QJsonObject& loop_object, Tree* current_tree, int clause_id);
    void processStructureField(const QJsonValue& field_value,
                               Tree* current_tree,
                               const QString& var_name,
                               int clause_id);
    void processNestedValue(const QJsonObject& value_obj,
                            Tree* current_tree,
                            const QString& var_name,
                            int clause_id);
    void processValueField(const QJsonValue& field_value,
                           Tree* current_tree,
                           QString& last_name,
                           int nesting_level);

    // Методы для работы с памятью
    void handleMalloc(Tree* current_tree, const QString& var_name, int clause_id);
    void handleFree(Tree* current_tree, const QString& var_name);
    void handleNullAssignment(Tree* current_tree, const QString& var_name, int clause_id);
    // Методы для работы с деревьями
    void mergeTrees(Tree* target_tree, Tree* source_tree);
    QList<QString> findLeaves(Tree* tree);
    QList<QString> findElementsWithRootConnection(Tree* tree);
    TreeElement* findMemoryElement(Tree* tree);
    // Вспомогательные методы
    TreeElement* findOrCreateElement(Tree* tree, const QString& name, const QString& type);
    Connection* findOrCreateConnection(Tree* tree, const QString& from, const QString& to, int position);
    void processNestedVariable(QString& current_name, const QString& field_direction);
    void handleMemoryAllocation(Tree* current_tree, const QString& var_name, int clause_id, const QString& operation_type);
    // Методы для маршрутизации по полям
    QString navigateByField(Tree* tree, const QString& current_name, const QString& field_direction, int clause_id);
    // Методы для работы с CNF
    void removeRootLinkClauses(Tree* tree, const QString& root_name, const QString& var_name);
    size_t findSetBitPosition(const BoolVector& bit_vector);
};

#endif // PARSER_JSON_H
