#ifndef PARSER_JSON_H
#define PARSER_JSON_H

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSet>
#include <QRegularExpression>
#include <QMap>
#include <QList>
#include <QString>
#include <iostream>
#include <minisat/core/Solver.h>
#include <stdexcept>

#include "Tree.h"
#include "TreeManager.h"
#include "Connection.h"
#include "BoolVector.h"
#include "TreeElement.h"

/**
 * @brief Класс для парсинга JSON-файлов, описывающих структуру программ и их преобразование в деревья
 * 
 * Parser_JSON отвечает за загрузку и анализ JSON-файлов, содержащих описание программ,
 * их трансформацию в древовидные структуры и последующую обработку для анализа
 * корректности работы с памятью.
 */
class Parser_JSON
{
public:
    Parser_JSON();
    ~Parser_JSON();

    /**
     * @brief Основной метод парсинга JSON-файла
     * @param file_path Путь к JSON-файлу
     * @return true в случае успешного парсинга, false при ошибке
     */
    bool parseJsonFile(const QString& file_path);

    /**
     * @brief Получение менеджера деревьев после парсинга
     * @return Указатель на объект TreeManager
     */
    TreeManager* getTreeManager() const { return treeManager_; }
    
    /**
     * @brief Проверка наличия ошибок в процессе парсинга
     * @return true если были ошибки, false в противном случае
     */
    bool hasError() const { return has_error_; }
    
    /**
     * @brief Проверка наличия утечек памяти в проанализированной программе
     * @return true если обнаружены утечки, false в противном случае
     */
    bool hasMemoryLeak() const { return has_memory_leak_; }

    /**
     * @brief Отладочная печать информации об элементах
     */
    void printElements() const;
    
    /**
     * @brief Отладочная печать КНФ в виде векторов
     */
    void printCNFAsVectors() const;
    
    /**
     * @brief Решение КНФ для заданного дерева
     * @param tree Указатель на дерево для анализа
     * @return true если КНФ выполнима, false в противном случае
     */
    bool solveCNF(Tree* tree);

private:
    /// Управляющий объект для всех деревьев
    TreeManager* treeManager_;
    
    /// Флаг наличия ошибок при парсинге
    bool has_error_;
    
    /// Флаг наличия утечек памяти
    bool has_memory_leak_;

    /// Текущая позиция при парсинге
    int current_position_;
    
    /// Индекс переменной для работы с памятью
    int memory_var_index_;
    
    /// Максимальное количество байт
    size_t max_bytes_count_;

    /// Поддержка функций и вызовов функций (операция "call")
    QJsonObject functions_;  ///< Объект с определением функций
    QJsonObject stacks_;     ///< Объект со стеком вызовов

    /**
     * @brief Обработка отдельной строки (работает для code.rows и functions.*.rows)
     * @param row_obj JSON-объект строки
     * @param current_tree Текущее дерево
     * @param clause_id Идентификатор условия
     */
    void processRowObject(const QJsonObject& row_obj, Tree*& current_tree, int clause_id);
    
    /**
     * @brief Обработка строки с вызовом функции
     * @param row_obj JSON-объект строки
     * @param current_tree Текущее дерево
     * @param call_clause_id Идентификатор условия вызова
     */
    void processCallRow(const QJsonObject& row_obj, Tree*& current_tree, int call_clause_id);

    /**
     * @brief Подстановка параметров $paramN в JSON-значение
     * @param v Исходное JSON-значение
     * @param params Массив параметров для подстановки
     * @return JSON-значение с подставленными параметрами
     */
    QJsonValue substituteParams(const QJsonValue& v, const QJsonArray& params) const;
    
    /**
     * @brief Подстановка параметров $paramN в строку
     * @param s Исходная строка
     * @param params Массив параметров для подстановки
     * @return Строка с подставленными параметрами
     */
    QString substituteParamsInString(const QString& s, const QJsonArray& params) const;

    /**
     * @brief Обработка стрелочных выражений вида "root->left->right" в поле "variable"
     * @param current_tree Текущее дерево
     * @param var_expr Строка с выражением
     * @param row_obj JSON-объект строки
     * @param clause_id Идентификатор условия
     * @return true если выражение успешно обработано
     */
    bool tryProcessArrowExpression(Tree*& current_tree,
                                   const QString& var_expr,
                                   const QJsonObject& row_obj,
                                   int clause_id);

    /**
     * @brief Обработка тела функции или блока кода
     * @param body_value JSON-значение тела
     * @param current_tree Текущее дерево
     * @param clause_id Идентификатор условия
     */
    void processBody(const QJsonValue& body_value, Tree* current_tree, int clause_id);
    
    /**
     * @brief Обработка ветвления (if-else конструкций)
     * @param branch_obj JSON-объект ветвления
     * @param current_tree Текущее дерево
     * @param clause_id Идентификатор условия
     */
    void processBranch(const QJsonObject& branch_obj, Tree* current_tree, int clause_id);
    
    /**
     * @brief Обработка операции (присваивание, malloc, free и т.д.)
     * @param op_object JSON-объект операции
     * @param current_tree Текущее дерево
     * @param clause_id Идентификатор условия
     */
    void processOperation(const QJsonObject& op_object, Tree* current_tree, int clause_id);
    
    /**
     * @brief Обработка цикла
     * @param loop_object JSON-объект цикла
     * @param current_tree Текущее дерево
     * @param clause_id Идентификатор условия
     */
    void processLoop(const QJsonObject& loop_object, Tree* current_tree, int clause_id);

    /**
     * @brief Обработка поля структуры
     * @param field_value JSON-значение поля
     * @param current_tree Текущее дерево
     * @param var_name Имя переменной
     * @param clause_id Идентификатор условия
     */
    void processStructureField(const QJsonValue& field_value,
                               Tree* current_tree,
                               const QString& var_name,
                               int clause_id);

    /**
     * @brief Обработка вложенных значений вида {"variable":"x", "field":{...}}
     * @param value_obj JSON-объект значения
     * @param current_tree Текущее дерево
     * @param var_name Имя переменной
     * @param clause_id Идентификатор условия
     */
    void processNestedValue(const QJsonObject& value_obj,
                            Tree* current_tree,
                            const QString& var_name,
                            int clause_id);

    /**
     * @brief Вспомогательная обработка вложенного поля
     * @param field_value JSON-значение поля
     * @param current_tree Текущее дерево
     * @param last_name Последнее имя в цепочке
     * @param nesting_level Уровень вложенности
     */
    void processValueField(const QJsonValue& field_value,
                           Tree* current_tree,
                           QString& last_name,
                           int nesting_level);

    /**
     * @brief Навигация по полям left/right
     * @param tree Текущее дерево
     * @param current_name Текущее имя элемента
     * @param field_direction Направление навигации ("left" или "right")
     * @param clause_id Идентификатор условия
     * @return Имя элемента после навигации
     */
    QString navigateByField(Tree* tree,
                            const QString& current_name,
                            const QString& field_direction,
                            int clause_id);

    /**
     * @brief Обработка выделения памяти
     * @param current_tree Текущее дерево
     * @param var_name Имя переменной
     * @param clause_id Идентификатор условия
     * @param operation_type Тип операции (malloc, calloc)
     */
    void handleMemoryAllocation(Tree* current_tree,
                                const QString& var_name,
                                int clause_id,
                                const QString& operation_type);

    /**
     * @brief Обработка освобождения памяти
     * @param current_tree Текущее дерево
     * @param var_name Имя переменной
     */
    void handleFree(Tree* current_tree, const QString& var_name);
    
    /**
     * @brief Обработка присваивания NULL
     * @param current_tree Текущее дерево
     * @param var_name Имя переменной
     * @param clause_id Идентификатор условия
     */
    void handleNullAssignment(Tree* current_tree, const QString& var_name, int clause_id);

    /**
     * @brief Поиск или создание элемента дерева
     * @param tree Текущее дерево
     * @param name Имя элемента
     * @param type Тип элемента
     * @return Указатель на найденный или созданный элемент
     */
    TreeElement* findOrCreateElement(Tree* tree, const QString& name, const QString& type);
    
    /**
     * @brief Поиск или создание соединения между элементами
     * @param tree Текущее дерево
     * @param from Имя исходного элемента
     * @param to Имя целевого элемента
     * @param position Позиция соединения
     * @return Указатель на найденное или созданное соединение
     */
    Connection* findOrCreateConnection(Tree* tree, const QString& from, const QString& to, int position);

    /**
     * @brief Объединение двух деревьев
     * @param target_tree Целевое дерево
     * @param source_tree Дерево-источник
     */
    void mergeTrees(Tree* target_tree, Tree* source_tree);
    
    /**
     * @brief Поиск листьев в дереве
     * @param tree Указатель на дерево
     * @return Список имен листовых элементов
     */
    QList<QString> findLeaves(Tree* tree);
    
    /**
     * @brief Поиск элементов с корневым соединением
     * @param tree Указатель на дерево
     * @return Список имен элементов
     */
    QList<QString> findElementsWithRootConnection(Tree* tree);
    
    /**
     * @brief Поиск элемента памяти в дереве
     * @param tree Указатель на дерево
     * @return Указатель на найденный элемент или nullptr
     */
    TreeElement* findMemoryElement(Tree* tree);

    /**
     * @brief Обработка вложенной переменной
     * @param current_name Текущее имя (будет изменено)
     * @param field_direction Направление поля
     */
    void processNestedVariable(QString& current_name, const QString& field_direction);

    /**
     * @brief Удаление связей с корнем
     * @param tree Указатель на дерево
     * @param root_name Имя корневого элемента
     * @param var_name Имя переменной
     */
    void removeRootLinkClauses(Tree* tree, const QString& root_name, const QString& var_name);

    /**
     * @brief Поиск позиции установленного бита в векторе
     * @param bit_vector Вектор битов
     * @return Позиция установленного бита
     */
    size_t findSetBitPosition(const BoolVector& bit_vector);
    
    /**
     * @brief Удаление дочерних элементов памяти с сохранением родителя
     * @param t Указатель на дерево
     * @param memName Имя элемента памяти
     */
    void removeMemoryKeepChildren(Tree* t, const QString& memName);
    
    /**
     * @brief Полное удаление блока памяти
     * @param t Указатель на дерево
     * @param mem_name Имя элемента памяти
     */
    void removeMemoryBlock(Tree* t, const QString& mem_name);
};

#endif // PARSER_JSON_H