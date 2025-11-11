#include "Parser_JSON.h"

Parser_JSON::Parser_JSON()
    : treeManager_(new TreeManager()),
    has_error_(false),
    has_memory_leak_(false),
    current_position_(0),
    memory_var_index_(0),
    max_bytes_count_(1)
{
}
Parser_JSON::~Parser_JSON()
{
    delete treeManager_;
}
bool Parser_JSON::parseJsonFile(const QString& file_path)
{
    QFile file(file_path);
    //Проверка открытия файла
    if (!file.open(QIODevice::ReadOnly)) {
        std::cout << "Error: Failed to open file: " << file_path.toStdString() << std::endl;
        has_error_ = true;
        return false;
    }
    QJsonParseError parse_error;
    // Чтение и преобразование прочитанных данных в JSON-документ
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse_error);
    file.close();
    // Проверка документа на пустоту
    if (doc.isNull()) {
        std::cout << "Error: JSON parse error: " << parse_error.errorString().toStdString() << std::endl;
        has_error_ = true;
        return false;
    }
    // Проверка, что документ является объектом
    if (!doc.isObject()) {
        std::cout << "Error: Invalid JSON structure" << std::endl;
        has_error_ = true;
        return false;
    }
    QJsonObject root_obj = doc.object();
    // Проверка, что документ содержит объект "code"
    if (!root_obj.contains("code") || !root_obj["code"].isObject()) {
        std::cout << "Error: Missing 'code' object" << std::endl;
        has_error_ = true;
        return false;
    }
    // Проверка, что "code" содержит массив "rows"
    QJsonObject code_obj = root_obj["code"].toObject();
    if (!code_obj.contains("rows") || !code_obj["rows"].isArray()) {
        std::cout << "Error: Missing 'rows' array" << std::endl;
        has_error_ = true;
        return false;
    }
    Tree* current_tree = nullptr;
    QJsonArray rows = code_obj["rows"].toArray();
    std::cout << "=== Starting parsing of " << rows.size() << " rows ===" << std::endl;
    for (const QJsonValue& row : rows) {
        if (!row.isObject()) {
            std::cout << "Error: Invalid row structure" << std::endl;
            has_error_ = true;
            return false;
        }
        QJsonObject row_obj = row.toObject();
        if (!row_obj.contains("id")) {
            std::cout << "Error: Row missing id" << std::endl;
            has_error_ = true;
            return false;
        }
        int clause_id = row_obj["id"].toInt();
        std::cout << "\n=== Processing row " << clause_id << " ===" << std::endl;
        // Обработка переменной
        if (row_obj.contains("variable")) {
            QString var_name = row_obj["variable"].toString();
            std::cout << "Processing variable: " << var_name.toStdString() << std::endl;
            // Создаем или получаем дерево для этой переменной
            if (!treeManager_->containsTree(var_name)) {
                Tree* new_tree = new Tree(var_name);
                treeManager_->addTree(var_name, new_tree);
                // Создаем корневой элемент
                TreeElement* root_element = new TreeElement(var_name, TreeElement::ROOT, current_position_++);
                new_tree->addElement(var_name, root_element);
                new_tree->setRoot(root_element);
                max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
                std::cout << "Created new tree: " << var_name.toStdString() << std::endl;
            }
            current_tree = treeManager_->getTree(var_name);
            if (row_obj.contains("value")) {
                QJsonValue val = row_obj["value"];
                if (val.isNull()) {
                    std::cout << "Assigning null to variable" << std::endl;
                    handleNullAssignment(current_tree, var_name, clause_id);
                    continue;
                } else if (val.isString()) {
                    QString operation = val.toString();
                    std::cout << "Handling memory operation '" << operation.toStdString()
                              << "' for: " << var_name.toStdString() << std::endl;
                    if (operation == "malloc") {
                        handleMemoryAllocation(current_tree, var_name, clause_id, "malloc");
                    } else if (operation == "calloc") {
                        handleMemoryAllocation(current_tree, var_name, clause_id, "calloc");
                    } else if (operation == "realloc") {
                        handleMemoryAllocation(current_tree, var_name, clause_id, "realloc");
                    } else if (operation == "aligned_alloc") {
                        handleMemoryAllocation(current_tree, var_name, clause_id, "aligned_alloc");
                    } else if (operation == "new") {
                        handleMemoryAllocation(current_tree, var_name, clause_id, "new");
                    } else if (operation == "new[]") {
                        handleMemoryAllocation(current_tree, var_name, clause_id, "new[]");
                    } else if (operation == "free") {
                        handleFree(current_tree, var_name);
                    } else if (operation == "delete") {
                        handleFree(current_tree, var_name);
                    } else if (operation == "delete[]") {
                        handleFree(current_tree, var_name);
                    } else {
                        std::cout << "Warning: Unknown memory operation: " << operation.toStdString() << std::endl;
                    }
                } else if (val.isObject()) {
                    std::cout << "Processing nested value for: " << var_name.toStdString() << std::endl;
                    processNestedValue(val.toObject(), current_tree, var_name, clause_id);
                }
            } else if (row_obj.contains("field")) {
                std::cout << "Processing structure field for: " << var_name.toStdString() << std::endl;
                processStructureField(row_obj["field"], current_tree, var_name, clause_id);
            }
        }
        // Обработка операции
        else if (row_obj.contains("operation")) {
            std::cout << "Processing operation" << std::endl;
            if (!current_tree) {
                std::cout << "Warning: Operation without current tree, using first available tree" << std::endl;
                if (!treeManager_->getAllTrees().isEmpty()) {
                    current_tree = treeManager_->getAllTrees().first();
                }
            }
            if (current_tree) {
                processOperation(row_obj["operation"].toObject(), current_tree, clause_id);
            } else {
                std::cout << "Error: No tree available for operation" << std::endl;
                has_error_ = true;
            }
        } else {
            std::cout << "Warning: Row " << clause_id << " doesn't contain variable or operation" << std::endl;
        }
        // После обработки каждой строки выполняем solveCNF для ВСЕХ деревьев
        std::cout << "\n=== Solving CNF for ALL trees after row " << clause_id << " ===" << std::endl;
        QMap<QString, Tree*> all_trees = treeManager_->getAllTrees();
        std::cout << "Total trees: " << all_trees.size() << std::endl;
        bool any_leak_detected_current_row = false; // ИСПРАВЛЕНО: локальная переменная для текущей строки
        for (auto it = all_trees.constBegin(); it != all_trees.constEnd(); ++it) {
            Tree* tree = it.value();
            std::cout << "\n--- Analyzing tree: " << tree->getName().toStdString() << " ---" << std::endl;
            // Выводим все соединения текущего дерева с правильной логикой векторов
            QList<Connection> connections = tree->getConnections();
            std::cout << "Connections (" << connections.size() << "):" << std::endl;
            for (const Connection& conn : connections) {
                std::cout << " " << conn.getFrom().toStdString() << " -> " << conn.getTo().toStdString();
                std::cout << " [pos:" << conn.getPosition() << "]" << std::endl;
                // Выводим векторы с правильными пояснениями
                std::cout << " Positive positions (TO " << conn.getTo().toStdString() << "): ";
                for (size_t i = 0; i < conn.getPositiveVectorSize(); ++i) {
                    if (conn.getPositiveBit(i)) {
                        std::cout << i << " ";
                    }
                }
                std::cout << std::endl;
                std::cout << " Negative positions (FROM " << conn.getFrom().toStdString() << "): ";
                for (size_t i = 0; i < conn.getNegativeVectorSize(); ++i) {
                    if (conn.getNegativeBit(i)) {
                        std::cout << i << " ";
                    }
                }
                std::cout << std::endl;
            }
            // Выводим все элементы
            QMap<QString, TreeElement*> elements = tree->getElements();
            std::cout << "Elements (" << elements.size() << "):" << std::endl;
            for (auto elem_it = elements.constBegin(); elem_it != elements.constEnd(); ++elem_it) {
                TreeElement* element = elem_it.value();
                std::cout << " " << elem_it.key().toStdString()
                          << " [type:" << element->getTypeName().toStdString()
                          << ", pos:" << element->getPosition()
                          << "]" << std::endl;
            }
            // Решаем CNF для этого дерева
            bool tree_has_leak = solveCNF(tree);
            if (tree_has_leak) {
                std::cout << "❌ MEMORY LEAK DETECTED in tree '"
                          << tree->getName().toStdString() << "' at row " << clause_id << std::endl;
                any_leak_detected_current_row = true; // ИСПРАВЛЕНО: только локальная переменная
            } else {
                std::cout << "✅ No memory leak in tree '"
                          << tree->getName().toStdString() << "' at row " << clause_id << std::endl;
            }
        }
        // ИСПРАВЛЕНО: устанавливаем глобальный флаг только если в ЭТОЙ строке обнаружена утечка
        if (any_leak_detected_current_row) {
            has_memory_leak_ = true;
            std::cout << "\nOVERALL: Memory leak detected at row " << clause_id << std::endl;
        } else {
            std::cout << "\nOVERALL: No memory leaks detected at row " << clause_id << std::endl;
        }
        if (has_error_) {
            std::cout << "Error: Parsing error at row: " << clause_id << std::endl;
            return false;
        }
        std::cout << "=== Completed row " << clause_id << " ===\n" << std::endl;
    }
    // Финальная проверка всех деревьев
    std::cout << "=== Final Analysis of ALL Trees ===" << std::endl;
    QMap<QString, Tree*> all_trees = treeManager_->getAllTrees();
    std::cout << "Total trees: " << all_trees.size() << std::endl;
    bool final_leak_detected = false;
    for (auto it = all_trees.constBegin(); it != all_trees.constEnd(); ++it) {
        Tree* tree = it.value();
        std::cout << "\n--- Final state of tree: " << tree->getName().toStdString() << " ---" << std::endl;
        std::cout << "Elements: " << tree->getElementCount() << std::endl;
        std::cout << "Connections: " << tree->getConnectionCount() << std::endl;
        std::cout << "Valid: " << (tree->isValid() ? "Yes" : "No") << std::endl;
        // Финальная проверка CNF
        bool final_leak = solveCNF(tree);
        if (final_leak) {
            std::cout << "FINAL WARNING: Memory leak in tree '"
                      << tree->getName().toStdString() << "'" << std::endl;
            final_leak_detected = true;
        } else {
            std::cout << "FINAL: No memory leak in tree '"
                      << tree->getName().toStdString() << "'" << std::endl;
        }
    }
    // ИСПРАВЛЕНО: определяем финальный результат на основе финального анализа
    has_memory_leak_ = final_leak_detected;
    if (has_memory_leak_) {
        std::cout << "\nFINAL RESULT: Memory leaks detected in the program" << std::endl;
    } else {
        std::cout << "\nFINAL RESULT: No memory leaks detected" << std::endl;
    }
    return true;
}
void Parser_JSON::processBody(const QJsonValue& body_value, Tree* current_tree, int clause_id)
{
    if (body_value.isObject()) {
        QJsonObject body_obj = body_value.toObject();
        if (body_obj.contains("variable")) {
            QString var_name = body_obj["variable"].toString();
            TreeElement* element = findOrCreateElement(current_tree, var_name, "Variable");
            if (body_obj.contains("value")) {
                QJsonValue val = body_obj["value"];
                if (val.isNull()) {
                    handleNullAssignment(current_tree, var_name, clause_id);
                    return;
                } else if (val.isString()) {
                    QString operation = val.toString();
                    if (operation == "malloc" || operation == "calloc" || operation == "realloc" ||
                        operation == "new" || operation == "new[]") {
                        handleMemoryAllocation(current_tree, var_name, clause_id, operation);
                    } else if (operation == "free" || operation == "delete" || operation == "delete[]") {
                        handleFree(current_tree, var_name);
                    }
                } else if (val.isObject()) {
                    processNestedValue(val.toObject(), current_tree, var_name, clause_id);
                }
            } else if (body_obj.contains("field")) {
                processStructureField(body_obj["field"], current_tree, var_name, clause_id);
            }
        } else if (body_obj.contains("operation")) {
            processOperation(body_obj["operation"].toObject(), current_tree, clause_id);
        }
    } else if (body_value.isArray()) {
        QJsonArray body_array = body_value.toArray();
        for (const QJsonValue& item : body_array) {
            processBody(item, current_tree, clause_id);
        }
    }
}
void Parser_JSON::processBranch(const QJsonObject& branch_obj, Tree* current_tree, int clause_id)
{
    if (branch_obj.contains("body")) {
        processBody(branch_obj["body"], current_tree, clause_id);
    }
}
void Parser_JSON::processOperation(const QJsonObject& op_object, Tree* current_tree, int clause_id)
{
    if (!op_object.contains("op")) {
        has_error_ = true;
        return;
    }
    QString op_type = op_object["op"].toString();
    if (op_type == "loop") {
        processLoop(op_object, current_tree, clause_id);
    } else {
        if (op_object.contains("branch true")) {
            QJsonValue branch_true = op_object["branch true"];
            if (branch_true.isObject()) {
                processBranch(branch_true.toObject(), current_tree, clause_id);
            }
        }
        if (op_object.contains("branch false")) {
            QJsonValue branch_false = op_object["branch false"];
            if (branch_false.isObject()) {
                processBranch(branch_false.toObject(), current_tree, clause_id);
            }
        }
    }
}
void Parser_JSON::processLoop(const QJsonObject& loop_object, Tree* current_tree, int clause_id)
{
    if (!loop_object.contains("condition") || !loop_object.contains("body")) {
        has_error_ = true;
        std::cout << "Error: Loop missing condition or body" << std::endl;
        return;
    }
    // Обрабатываем тело цикла как одну итерацию
    std::cout << "Warning: Loop processed as single iteration" << std::endl;
    processBody(loop_object["body"], current_tree, clause_id);
}
void Parser_JSON::processStructureField(const QJsonValue& field_value,
                                        Tree* current_tree,
                                        const QString& var_name,
                                        int clause_id)
{
    if (!field_value.isObject()) {
        std::cout << "Error: Field value is not an object" << std::endl;
        has_error_ = true;
        return;
    }
    QJsonObject field_obj = field_value.toObject();
    QString current_var_name = var_name;
    std::cout << "Starting structure field processing for: " << var_name.toStdString() << std::endl;
    // УДАЛЯЕМ СВЯЗИ С КОРНЕМ для исходной переменной
    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);
    if (field_obj.contains("f")) {
        QJsonValue f_value = field_obj["f"];
        if (f_value.isString()) {
            std::cout << "Navigating to field: " << f_value.toString().toStdString() << std::endl;
            // Переходим по полю (left или right) и получаем целевую переменную
            QString target_var_name = navigateByField(current_tree, current_var_name, f_value.toString(), clause_id);
            if (target_var_name.isEmpty()) {
                std::cout << "Failed to navigate to field" << std::endl;
                return;
            }
            current_var_name = target_var_name;
            std::cout << "Successfully navigated to: " << current_var_name.toStdString() << std::endl;
            // УДАЛЯЕМ СВЯЗИ С КОРНЕМ для целевой переменной
            removeRootLinkClauses(current_tree, current_tree->getName(), current_var_name);
        }
    }
    TreeElement* element = current_tree->findElement(current_var_name);
    if (!element) {
        std::cout << "Error: Element '" << current_var_name.toStdString() << "' not found after navigation" << std::endl;
        has_error_ = true;
        return;
    }
    // ОБРАБАТЫВАЕМ ПОЛЕ "op" - рекурсивно вызываем processStructureField
    if (field_obj.contains("op")) {
        std::cout << "Processing op field for: " << current_var_name.toStdString() << std::endl;
        processStructureField(field_obj["op"], current_tree, current_var_name, clause_id);
    }
    // Обрабатываем поле "value" только если нет "op"
    if (field_obj.contains("value")) {
        QJsonValue value = field_obj["value"];
        if (value.isNull()) {
            std::cout << "Assigning null to field" << std::endl;
            handleNullAssignment(current_tree, current_var_name, clause_id);
            return;
        } else if (value.isString()) {
            QString operation = value.toString();
            if (operation == "malloc" || operation == "calloc" || operation == "realloc" ||
                operation == "new" || operation == "new[]") {
                handleMemoryAllocation(current_tree, current_var_name, clause_id, operation);
            } else if (operation == "free" || operation == "delete" || operation == "delete[]") {
                handleFree(current_tree, current_var_name);
            }
        } else if (value.isObject()) {
            std::cout << "Processing nested value for: " << current_var_name.toStdString() << std::endl;
            processNestedValue(value.toObject(), current_tree, current_var_name, clause_id);
        }
    } else {
        std::cout << "No value specified for field, navigation completed" << std::endl;
    }
}
QString Parser_JSON::navigateByField(Tree* tree, const QString& current_name, const QString& field_direction, int clause_id)
{
    TreeElement* current_element = tree->findElement(current_name);
    if (!current_element) {
        std::cout << "Error: Current element '" << current_name.toStdString() << "' not found" << std::endl;
        has_error_ = true;
        return QString();
    }
    std::cout << "Looking for " << field_direction.toStdString() << " field from " << current_name.toStdString()
              << " [type: " << current_element->getTypeName().toStdString() << "]" << std::endl;
    // Определяем нужный тип в зависимости от направления
    int required_type;
    if (field_direction == "left") {
        required_type = TreeElement::LEFT_VARIABLE;
    } else if (field_direction == "right") {
        required_type = TreeElement::RIGHT_VARIABLE;
    } else {
        std::cout << "Error: Unknown field direction '" << field_direction.toStdString() << "'" << std::endl;
        has_error_ = true;
        return QString();
    }
    // Сначала ищем ПРЯМЫЕ связи от текущего элемента к элементам нужного типа
    QList<Connection> connections_from = tree->getConnectionsFrom(current_name);
    std::cout << " Direct connections from " << current_name.toStdString() << ": ";
    for (const Connection& conn : connections_from) {
        std::cout << conn.getTo().toStdString() << " ";
    }
    std::cout << std::endl;
    for (const Connection& conn : connections_from) {
        TreeElement* target_element = tree->findElement(conn.getTo());
        if (target_element && target_element->getType() == required_type) {
            std::cout << "Found direct " << field_direction.toStdString()
                      << " connection: " << current_name.toStdString()
                      << " -> " << target_element->getName().toStdString()
                      << " [type: " << target_element->getTypeName().toStdString() << "]" << std::endl;
            return target_element->getName();
        }
    }
    // Если прямых связей нет, ищем через память (MEMORY элементы)
    for (const Connection& conn : connections_from) {
        TreeElement* intermediate_element = tree->findElement(conn.getTo());
        if (intermediate_element && intermediate_element->isMemory()) {
            std::cout << "Found memory element: " << intermediate_element->getName().toStdString() << std::endl;
            // Ищем от memory элемента связи к элементам нужного типа
            QList<Connection> memory_connections = tree->getConnectionsFrom(intermediate_element->getName());
            for (const Connection& mem_conn : memory_connections) {
                TreeElement* target_element = tree->findElement(mem_conn.getTo());
                if (target_element && target_element->getType() == required_type) {
                    std::cout << "Found " << field_direction.toStdString()
                              << " via memory: " << current_name.toStdString()
                              << " -> " << intermediate_element->getName().toStdString()
                              << " -> " << target_element->getName().toStdString()
                              << " [type: " << target_element->getTypeName().toStdString() << "]" << std::endl;
                    return target_element->getName();
                }
            }
        }
    }
    // Если не нашли подходящий элемент - это ошибка
    std::cout << "Error: No " << field_direction.toStdString()
              << " variable found for element '" << current_name.toStdString() << "'" << std::endl;
    // Выводим отладочную информацию о доступных элементах
    QMap<QString, TreeElement*> all_elements = tree->getElements();
    std::cout << " Available elements in tree: ";
    for (auto it = all_elements.constBegin(); it != all_elements.constEnd(); ++it) {
        if (it.value()->getType() == required_type) {
            std::cout << it.key().toStdString() << "[" << it.value()->getTypeName().toStdString() << "] ";
        }
    }
    std::cout << std::endl;
    has_error_ = true;
    return QString();
}
void Parser_JSON::processNestedValue(const QJsonObject& value_obj,
                                     Tree* current_tree,
                                     const QString& var_name,
                                     int clause_id)
{
    if (!value_obj.contains("variable")) {
        std::cout << "Error: Nested value missing 'variable' field" << std::endl;
        has_error_ = true;
        return;
    }
    QString nested_var_name = value_obj["variable"].toString();
    std::cout << "Processing nested variable assignment: " << var_name.toStdString()
              << " = " << nested_var_name.toStdString() << " at clause " << clause_id << std::endl;
    TreeElement* target_element = current_tree->findElement(var_name);
    if (!target_element) {
        std::cout << "Error: Target element '" << var_name.toStdString() << "' not found" << std::endl;
        has_error_ = true;
        return;
    }
    std::cout << "Target element: " << target_element->getName().toStdString()
              << " [pos: " << target_element->getPosition() << "]" << std::endl;
    // Удаляем связи с корнем для целевого элемента
    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);
    // Remove previous pointer connections
    QList<Connection> prev_out = current_tree->getConnectionsFrom(var_name);
    for (const Connection& prev : prev_out) {
        current_tree->removeConnection(prev.getFrom(), prev.getTo());
        std::cout << "Removed previous assignment: " << var_name.toStdString() << " -> " << prev.getTo().toStdString() << std::endl;
    }
    // Проверяем, существует ли nested_var_name в текущем дереве
    TreeElement* source_element = current_tree->findElement(nested_var_name);
    if (source_element) {
        std::cout << "Source element found in current tree: " << source_element->getName().toStdString() << std::endl;
        // Если переменная существует в текущем дереве - создаем связь между ними
        std::cout << "Creating connection within same tree: " << var_name.toStdString()
                  << " -> " << nested_var_name.toStdString() << std::endl;
        BoolVector pos_vec(max_bytes_count_ * 8);
        BoolVector neg_vec(max_bytes_count_ * 8);
        pos_vec.setBit(source_element->getPosition()); // TO: source (положительный)
        neg_vec.setBit(target_element->getPosition()); // FROM: target (отрицательный)
        Connection* conn = new Connection(var_name, nested_var_name, pos_vec, neg_vec, clause_id);
        current_tree->addConnection(*conn);
        delete conn;
        std::cout << "Connection created successfully: " << var_name.toStdString()
                  << " -> " << nested_var_name.toStdString() << std::endl;
    } else {
        std::cout << "Source element not found in current tree, checking other trees..." << std::endl;
        // Если переменная не найдена в текущем дереве, проверяем другие деревья
        Tree* source_tree = treeManager_->getTree(nested_var_name);
        if (source_tree && source_tree != current_tree) {
            std::cout << "Found source tree: " << source_tree->getName().toStdString() << std::endl;
            // Находим память (элемент MEMORY) в source_tree для создания связи
            TreeElement* source_memory_element = findMemoryElement(source_tree);
            if (!source_memory_element) {
                std::cout << "Error: No memory element found in source tree" << std::endl;
                has_error_ = true;
                return;
            }
            std::cout << "Source memory element: " << source_memory_element->getName().toStdString()
                      << " [pos: " << source_memory_element->getPosition() << "]" << std::endl;
            QString source_mem_name = source_memory_element->getName();
            // Объединяем деревья сначала
            std::cout << "Starting merge process..." << std::endl;
            mergeTrees(current_tree, source_tree);
            // Теперь создаем связь после объединения
            TreeElement* source_mem_now = current_tree->findElement(source_mem_name);
            if (!source_mem_now) {
                std::cout << "Error: Source memory not found after merge" << std::endl;
                has_error_ = true;
                return;
            }
            std::cout << "Creating connection: " << var_name.toStdString()
                      << " -> " << source_mem_name.toStdString() << std::endl;
            BoolVector pos_vec(max_bytes_count_ * 8);
            BoolVector neg_vec(max_bytes_count_ * 8);
            pos_vec.setBit(source_mem_now->getPosition()); // TO: source memory
            neg_vec.setBit(target_element->getPosition()); // FROM: target
            Connection* conn = new Connection(var_name, source_mem_name,
                                              pos_vec, neg_vec, clause_id);
            current_tree->addConnection(*conn);
            delete conn;
            std::cout << "Connection created: " << var_name.toStdString()
                      << " -> " << source_mem_name.toStdString() << std::endl;
            // ПРОВЕРЯЕМ, что связь сохранилась после объединения (теперь не нужна, но для отладки)
            Connection* check_conn = current_tree->findConnection(var_name, source_mem_name);
            if (check_conn) {
                std::cout << "Connection verified after merge: " << var_name.toStdString()
                          << " -> " << source_mem_name.toStdString() << std::endl;
            } else {
                std::cout << "ERROR: Connection lost after merge!" << std::endl;
            }
        } else {
            std::cout << "Error: Source tree '" << nested_var_name.toStdString() << "' not found" << std::endl;
            has_error_ = true;
            return;
        }
    }
    // Обрабатываем поле, если оно есть
    if (value_obj.contains("field")) {
        QString current_name = nested_var_name;
        processValueField(value_obj["field"], current_tree, current_name, 1);
    }
}
// Вспомогательный метод для поиска элемента памяти в дереве
TreeElement* Parser_JSON::findMemoryElement(Tree* tree)
{
    if (!tree) return nullptr;
    QMap<QString, TreeElement*> elements = tree->getElements();
    std::cout << "Looking for memory element in tree: " << tree->getName().toStdString() << std::endl;
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        if (it.value()->isMemory()) {
            std::cout << "Found memory element: " << it.key().toStdString() << std::endl;
            return it.value();
        }
    }
    // Если не нашли MEMORY, возвращаем первый не-корневой элемент
    QString root_name = tree->getName();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        if (it.key() != root_name) {
            std::cout << "Using non-root element as memory: " << it.key().toStdString() << std::endl;
            return it.value();
        }
    }
    std::cout << "No suitable memory element found" << std::endl;
    return nullptr;
}
void Parser_JSON::processValueField(const QJsonValue& field_value,
                                    Tree* current_tree,
                                    QString& last_name,
                                    int nesting_level)
{
    if (!field_value.isObject()) {
        has_error_ = true;
        return;
    }
    QJsonObject field_obj = field_value.toObject();
    if (field_obj.contains("f")) {
        QJsonValue f_value = field_obj["f"];
        if (f_value.isString()) {
            // Рекурсивно переходим по полям (только по существующим элементам)
            for (int i = 0; i < nesting_level; ++i) {
                QString target_name = navigateByField(current_tree, last_name, f_value.toString(), -1);
                if (target_name.isEmpty()) {
                    std::cout << "Error: Failed to navigate in processValueField at level " << i << std::endl;
                    has_error_ = true;
                    return;
                }
                last_name = target_name;
                std::cout << "Recursive navigation level " << i << ": now at " << last_name.toStdString() << std::endl;
            }
        }
    }
}
void Parser_JSON::handleMemoryAllocation(Tree* current_tree, const QString& var_name, int clause_id, const QString& operation_type)
{
    TreeElement* element = current_tree->findElement(var_name);
    if (!element) {
        has_error_ = true;
        std::cout << "Error: Element '" << var_name.toStdString() << "' not found for "
                  << operation_type.toStdString() << std::endl;
        return;
    }
    std::cout << "Executing " << operation_type.toStdString() << " for: " << var_name.toStdString() << std::endl;
    // 1. Проверяем и удаляем связь с корнем, если она существует
    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);
    // Remove previous pointer connections
    QList<Connection> prev_out = current_tree->getConnectionsFrom(var_name);
    for (const Connection& prev : prev_out) {
        current_tree->removeConnection(prev.getFrom(), prev.getTo());
        std::cout << "Removed previous assignment: " << var_name.toStdString() << " -> " << prev.getTo().toStdString() << std::endl;
    }
    // 2. Создаем memory variable
    QString mem_name = "N" + QString::number(memory_var_index_++);
    TreeElement* mem_element = new TreeElement(mem_name, TreeElement::MEMORY, current_position_++);
    current_tree->addElement(mem_name, mem_element);
    // УДАЛЯЕМ СВЯЗИ С КОРНЕМ для новой memory variable
    removeRootLinkClauses(current_tree, current_tree->getName(), mem_name);
    // Создаем соединение: var_name -> mem_name
    BoolVector pos_vec(max_bytes_count_ * 8);
    BoolVector neg_vec(max_bytes_count_ * 8);
    pos_vec.setBit(mem_element->getPosition()); // TO: mem_name (положительный)
    neg_vec.setBit(element->getPosition()); // FROM: var_name (отрицательный)
    Connection* conn = new Connection(var_name, mem_name, pos_vec, neg_vec, clause_id);
    current_tree->addConnection(*conn);
    delete conn;
    max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
    // 3. Создаем два дополнительных элемента с соответствующими типами
    QString left_child_name = "N" + QString::number(memory_var_index_++);
    QString right_child_name = "N" + QString::number(memory_var_index_++);
    TreeElement* left_child = new TreeElement(left_child_name, TreeElement::LEFT_VARIABLE, current_position_++);
    TreeElement* right_child = new TreeElement(right_child_name, TreeElement::RIGHT_VARIABLE, current_position_++);
    current_tree->addElement(left_child_name, left_child);
    current_tree->addElement(right_child_name, right_child);
    // 4. Создаем связи: mem_name -> left_child и mem_name -> right_child
    BoolVector pos_vec1(max_bytes_count_ * 8);
    BoolVector neg_vec1(max_bytes_count_ * 8);
    pos_vec1.setBit(left_child->getPosition()); // TO: left_child (положительный)
    neg_vec1.setBit(mem_element->getPosition()); // FROM: mem_name (отрицательный)
    Connection* conn1 = new Connection(mem_name, left_child_name, pos_vec1, neg_vec1, clause_id);
    current_tree->addConnection(*conn1);
    delete conn1;
    BoolVector pos_vec2(max_bytes_count_ * 8);
    BoolVector neg_vec2(max_bytes_count_ * 8);
    pos_vec2.setBit(right_child->getPosition()); // TO: right_child (положительный)
    neg_vec2.setBit(mem_element->getPosition()); // FROM: mem_name (отрицательный)
    Connection* conn2 = new Connection(mem_name, right_child_name, pos_vec2, neg_vec2, clause_id);
    current_tree->addConnection(*conn2);
    delete conn2;
    // 5. Создаем связи: left_child -> root и right_child -> root
    TreeElement* root_element = current_tree->getRoot();
    if (root_element) {
        // left_child -> root
        BoolVector root_pos_vec1(max_bytes_count_ * 8);
        BoolVector root_neg_vec1(max_bytes_count_ * 8);
        root_pos_vec1.setBit(root_element->getPosition()); // TO: root (положительный)
        root_neg_vec1.setBit(left_child->getPosition()); // FROM: left_child (отрицательный)
        Connection* root_conn1 = new Connection(left_child_name, current_tree->getName(),
                                                root_pos_vec1, root_neg_vec1, clause_id);
        current_tree->addConnection(*root_conn1);
        delete root_conn1;
        // right_child -> root
        BoolVector root_pos_vec2(max_bytes_count_ * 8);
        BoolVector root_neg_vec2(max_bytes_count_ * 8);
        root_pos_vec2.setBit(root_element->getPosition()); // TO: root (положительный)
        root_neg_vec2.setBit(right_child->getPosition()); // FROM: right_child (отрицательный)
        Connection* root_conn2 = new Connection(right_child_name, current_tree->getName(),
                                                root_pos_vec2, root_neg_vec2, clause_id);
        current_tree->addConnection(*root_conn2);
        delete root_conn2;
    }
    max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
    std::cout << operation_type.toStdString() << ": " << var_name.toStdString() << " -> " << mem_name.toStdString()
              << " with left: " << left_child_name.toStdString()
              << ", right: " << right_child_name.toStdString()
              << " at position " << clause_id << std::endl;
    // Особые случаи для разных типов операций
    if (operation_type == "calloc") {
        std::cout << "Note: calloc operation - memory initialized to zero" << std::endl;
    } else if (operation_type == "realloc") {
        std::cout << "Note: realloc operation - previous memory block should be freed" << std::endl;
    } else if (operation_type == "new[]") {
        std::cout << "Note: new[] operation - array allocation, requires delete[]" << std::endl;
    }
}

void Parser_JSON::handleFree(Tree* current_tree, const QString& var_name)
{
    TreeElement* element = current_tree->findElement(var_name);
    if (!element) {
        has_error_ = true;
        std::cout << "Error: Element '" << var_name.toStdString() << "' not found for free operation" << std::endl;
        return;
    }

    std::cout << "Free operation: " << var_name.toStdString() << std::endl;

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    QList<Connection> connections_from = current_tree->getConnectionsFrom(var_name);
    QList<Connection> connections_to = current_tree->getConnectionsTo(var_name);

    std::cout << "  Removing " << connections_from.size() << " outgoing connections" << std::endl;
    std::cout << "  Removing " << connections_to.size() << " incoming connections" << std::endl;

    TreeElement* memory_element = nullptr;
    for (const Connection& conn : connections_from) {
        TreeElement* target_element = current_tree->findElement(conn.getTo());
        if (target_element && target_element->isMemory()) {
            memory_element = target_element;
            break;
        }
    }

    if (memory_element) {
        QString memory_name = memory_element->getName();
        std::cout << "  Found memory element: " << memory_name.toStdString() << std::endl;

        QList<Connection> memory_connections = current_tree->getConnectionsFrom(memory_name);
        QList<QString> child_elements_to_remove;

        for (const Connection& mem_conn : memory_connections) {
            TreeElement* child_element = current_tree->findElement(mem_conn.getTo());
            if (child_element && (child_element->isLeftVariable() || child_element->isRightVariable())) {
                child_elements_to_remove.append(child_element->getName());
                std::cout << "    Found child element to remove: " << child_element->getName().toStdString() << std::endl;
            }
        }

        for (const Connection& mem_conn : memory_connections) {
            current_tree->removeConnection(mem_conn.getFrom(), mem_conn.getTo());
            std::cout << "    Removed memory connection: " << mem_conn.getFrom().toStdString()
                      << " -> " << mem_conn.getTo().toStdString() << std::endl;
        }

        for (const QString& child_name : child_elements_to_remove) {
            removeRootLinkClauses(current_tree, current_tree->getName(), child_name);

            QList<Connection> child_connections_from = current_tree->getConnectionsFrom(child_name);
            QList<Connection> child_connections_to = current_tree->getConnectionsTo(child_name);

            for (const Connection& child_conn : child_connections_from) {
                current_tree->removeConnection(child_conn.getFrom(), child_conn.getTo());
            }
            for (const Connection& child_conn : child_connections_to) {
                current_tree->removeConnection(child_conn.getFrom(), child_conn.getTo());
            }

            current_tree->removeElement(child_name);
            std::cout << "    Removed child element: " << child_name.toStdString() << std::endl;
        }

        current_tree->removeElement(memory_name);
        std::cout << "    Removed memory element: " << memory_name.toStdString() << std::endl;
    } else {
        std::cout << "  No memory element found for: " << var_name.toStdString() << std::endl;
    }

    for (const Connection& conn : connections_from) {
        current_tree->removeConnection(conn.getFrom(), conn.getTo());
        std::cout << "    Removed connection: " << conn.getFrom().toStdString()
                  << " -> " << conn.getTo().toStdString() << std::endl;
    }

    for (const Connection& conn : connections_to) {
        current_tree->removeConnection(conn.getFrom(), conn.getTo());
        std::cout << "    Removed connection: " << conn.getFrom().toStdString()
                  << " -> " << conn.getTo().toStdString() << std::endl;
    }

    if (element != current_tree->getRoot()) {
        current_tree->removeElement(var_name);
        std::cout << "  Removed element: " << var_name.toStdString() << std::endl;
    } else {
        std::cout << "  Skipped removal of root element: " << var_name.toStdString() << std::endl;
    }

    std::cout << "Free completed for: " << var_name.toStdString() << std::endl;
}

Connection* Parser_JSON::findOrCreateConnection(Tree* tree, const QString& from, const QString& to, int position)
{
    // Перед созданием проверяем и удаляем возможные связи с корнем для обоих элементов
    removeRootLinkClauses(tree, tree->getName(), from);
    removeRootLinkClauses(tree, tree->getName(), to);
    Connection* conn = tree->findConnection(from, to);
    if (!conn) {
        BoolVector pos_vec(max_bytes_count_ * 8);
        BoolVector neg_vec(max_bytes_count_ * 8);
        Connection new_conn(from, to, pos_vec, neg_vec, position);
        tree->addConnection(new_conn);
        conn = tree->findConnection(from, to);
    }
    return conn;
}
bool Parser_JSON::solveCNF(Tree* tree)
{
    if (!tree) return false;
    // ЕСЛИ ДЕРЕВО ПУСТОЕ (все элементы освобождены) - УТЕЧКИ НЕТ
    if (tree->getElementCount() == 0 && tree->getConnectionCount() == 0) {
        std::cout << "Tree is empty (all memory freed) - no memory leak" << std::endl;
        return false; // Нет утечки
    }
    Minisat::Solver solver;
    // Находим максимальную позицию среди элементов
    size_t maxPos = 0;
    QMap<QString, TreeElement*> elements = tree->getElements();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        size_t pos = it.value()->getPosition();
        if (pos > maxPos) maxPos = pos;
    }
    size_t totalVariables = maxPos + 1;
    // ЕСЛИ НЕТ ПЕРЕМЕННЫХ - УТЕЧКИ НЕТ
    if (totalVariables == 0) {
        std::cout << "No variables in CNF - no memory leak" << std::endl;
        return false;
    }
    // Создаем переменные в решателе
    std::vector<Minisat::Var> vars(totalVariables);
    for (size_t i = 0; i < totalVariables; ++i) {
        vars[i] = solver.newVar();
    }
    // Преобразуем соединения в клаузы Minisat
    QList<Connection> connections = tree->getConnections();
    // ЕСЛИ НЕТ СОЕДИНЕНИЙ, НО ЕСТЬ ЭЛЕМЕНТЫ - ЭТО УТЕЧКА
    if (connections.size() == 0 && elements.size() > 0) {
        std::cout << "No connections but elements exist - potential memory leak" << std::endl;
        return true; // Утечка есть
    }
    for (const Connection& conn : connections) {
        Minisat::vec<Minisat::Lit> clause;
        // Обрабатываем положительные литералы
        for (size_t i = 0; i < conn.getPositiveVectorSize(); ++i) {
            if (conn.getPositiveBit(i)) {
                if (i < totalVariables) {
                    clause.push(Minisat::mkLit(vars[i], false)); // Положительный литерал
                }
            }
        }
        // Обрабатываем отрицательные литералы
        for (size_t i = 0; i < conn.getNegativeVectorSize(); ++i) {
            if (conn.getNegativeBit(i)) {
                if (i < totalVariables) {
                    clause.push(Minisat::mkLit(vars[i], true)); // Отрицательный литерал
                }
            }
        }
        if (clause.size() > 0) {
            solver.addClause(clause);
        }
    }
    // Добавляем клаузу с положительными литералами всех переменных (x10 ∨ x11 ∨ x12 ∨ x13)
    Minisat::vec<Minisat::Lit> all_positive_clause;
    for (size_t i = 0; i < totalVariables; ++i) {
        bool used = false;
        for (const Connection& conn : connections) {
            if ((i < conn.getPositiveVectorSize() && conn.getPositiveBit(i)) ||
                (i < conn.getNegativeVectorSize() && conn.getNegativeBit(i))) {
                used = true;
                break;
            }
        }
        if (used) {
            all_positive_clause.push(Minisat::mkLit(vars[i], false)); // x_i
        }
    }
    if (all_positive_clause.size() > 0) {
        solver.addClause(all_positive_clause);
    }
    // Добавляем клаузу с отрицательными литералами всех переменных (¬x10 ∨ ¬x11 ∨ ¬x12 ∨ ¬x13)
    Minisat::vec<Minisat::Lit> all_negative_clause;
    for (size_t i = 0; i < totalVariables; ++i) {
        bool used = false;
        for (const Connection& conn : connections) {
            if ((i < conn.getPositiveVectorSize() && conn.getPositiveBit(i)) ||
                (i < conn.getNegativeVectorSize() && conn.getNegativeBit(i))) {
                used = true;
                break;
            }
        }
        if (used) {
            all_negative_clause.push(Minisat::mkLit(vars[i], true)); // ¬x_i
        }
    }
    if (all_negative_clause.size() > 0) {
        solver.addClause(all_negative_clause);
    }
    // Решаем КНФ
    bool result = solver.solve();
    bool has_leak = result; // ИСПРАВЛЕНО: Утечка, если КНФ разрешима (SAT)
    if (result) {
        std::cout << "SAT - solution found for tree '" << tree->getName().toStdString() << "' - MEMORY LEAK DETECTED" << std::endl;
        for (size_t i = 0; i < totalVariables; ++i) {
            bool used = false;
            for (const Connection& conn : connections) {
                if ((i < conn.getPositiveVectorSize() && conn.getPositiveBit(i)) ||
                    (i < conn.getNegativeVectorSize() && conn.getNegativeBit(i))) {
                    used = true;
                    break;
                }
            }
            if (used) {
                Minisat::lbool value = solver.modelValue(vars[i]);
                QString var_name = "Unknown";
                for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
                    if (it.value()->getPosition() == (int)i) {
                        var_name = it.key();
                        break;
                    }
                }
                std::cout << var_name.toStdString() << " (x" << i << ") = "
                          << (value == Minisat::lbool((uint8_t)0) ? "true" : "false") << std::endl;
            }
        }
    } else {
        std::cout << "UNSAT - no solution found for tree '" << tree->getName().toStdString() << "' - NO memory leak" << std::endl;
    }
    return has_leak;
}
void Parser_JSON::mergeTrees(Tree* target_tree, Tree* source_tree)
{
    if (!target_tree || !source_tree || target_tree == source_tree) {
        return;
    }
    std::cout << "Merging tree '" << source_tree->getName().toStdString()
              << "' into '" << target_tree->getName().toStdString() << "'" << std::endl;
    TreeElement* target_root = target_tree->getRoot();
    if (!target_root) {
        std::cout << "Error: Target tree has no root" << std::endl;
        return;
    }
    QString source_root_name = source_tree->getName();
    // 1. УДАЛЯЕМ СВЯЗИ ЛИСТЬЕВ source дерева с ИХ корнем
    QList<Connection> source_connections = source_tree->getConnections();
    QList<Connection> connections_to_remove;
    for (const Connection& conn : source_connections) {
        // Удаляем связи, которые идут от листьев к корню source дерева
        if (conn.getTo() == source_root_name) {
            std::cout << "Removing leaf-to-source-root connection: "
                      << conn.getFrom().toStdString() << " -> " << conn.getTo().toStdString() << std::endl;
            connections_to_remove.append(conn);
        }
    }
    // Удаляем найденные связи
    for (const Connection& conn : connections_to_remove) {
        source_tree->removeConnection(conn.getFrom(), conn.getTo());
    }
    // 2. Копируем элементы из source в target (кроме корня source)
    QMap<QString, TreeElement*> source_elements = source_tree->getElements();
    for (auto it = source_elements.constBegin(); it != source_elements.constEnd(); ++it) {
        if (it.key() == source_root_name) {
            continue; // Пропускаем корень source дерева
        }
        // Проверяем, не существует ли уже элемент с таким именем
        if (!target_tree->findElement(it.key())) {
            TreeElement* new_element = new TreeElement(*(it.value()));
            target_tree->addElement(it.key(), new_element);
            std::cout << " Copied element: " << it.key().toStdString() << std::endl;
        } else {
            std::cout << " Element already exists: " << it.key().toStdString() << std::endl;
        }
    }
    // 3. Копируем соединения (кроме тех, что связаны с корнем source)
    for (const Connection& conn : source_connections) {
        if (conn.getFrom() != source_root_name && conn.getTo() != source_root_name) {
            // Проверяем, не существует ли уже такое соединение
            if (!target_tree->findConnection(conn.getFrom(), conn.getTo())) {
                target_tree->addConnection(conn);
                std::cout << " Copied connection: " << conn.getFrom().toStdString()
                          << " -> " << conn.getTo().toStdString() << std::endl;
            } else {
                std::cout << " Connection already exists: " << conn.getFrom().toStdString()
                          << " -> " << conn.getTo().toStdString() << std::endl;
            }
        }
    }
    // 4. НАХОДИМ ЛИСТЬЯ source дерева и добавляем связи к корню TARGET дерева
    QList<QString> source_leaves = findLeaves(source_tree);
    std::cout << " Found " << source_leaves.size() << " leaves in source tree: ";
    for (const QString& leaf : source_leaves) {
        std::cout << leaf.toStdString() << " ";
    }
    std::cout << std::endl;
    for (const QString& leaf_name : source_leaves) {
        TreeElement* leaf_element = target_tree->findElement(leaf_name);
        if (leaf_element) {
            // Создаем связь от листа source дерева к корню TARGET дерева
            BoolVector pos_vec(max_bytes_count_ * 8);
            BoolVector neg_vec(max_bytes_count_ * 8);
            pos_vec.setBit(target_root->getPosition()); // TO: target root - положительный
            neg_vec.setBit(leaf_element->getPosition()); // FROM: leaf - отрицательный
            Connection* conn = new Connection(leaf_name, target_tree->getName(),
                                              pos_vec, neg_vec, -1);
            target_tree->addConnection(*conn);
            delete conn;
            std::cout << " Added leaf-to-target-root connection: " << leaf_name.toStdString()
                      << " -> " << target_tree->getName().toStdString() << std::endl;
        }
    }
    // 6. Удаляем source tree из менеджера
    treeManager_->removeTree(source_root_name);
    std::cout << "Successfully merged trees" << std::endl;
}
// Вспомогательный метод для поиска элементов, которые имеют связь с корнем
QList<QString> Parser_JSON::findElementsWithRootConnection(Tree* tree)
{
    QList<QString> elements;
    if (!tree) return elements;
    QString root_name = tree->getName();
    QList<Connection> connections = tree->getConnections();
    // Ищем элементы, которые имеют связь с корнем (в любую сторону)
    for (const Connection& conn : connections) {
        // Если связь от корня к элементу
        if (conn.getFrom() == root_name) {
            if (!elements.contains(conn.getTo())) {
                elements.append(conn.getTo());
            }
        }
        // Если связь от элемента к корню
        if (conn.getTo() == root_name) {
            if (!elements.contains(conn.getFrom())) {
                elements.append(conn.getFrom());
            }
        }
    }
    std::cout << " Elements with root connection in tree '" << root_name.toStdString() << "': ";
    for (const QString& elem : elements) {
        std::cout << elem.toStdString() << " ";
    }
    std::cout << std::endl;
    return elements;
}
// Вспомогательный метод для поиска листьев в дереве
QList<QString> Parser_JSON::findLeaves(Tree* tree)
{
    QList<QString> leaves;
    if (!tree) return leaves;
    QMap<QString, TreeElement*> elements = tree->getElements();
    QSet<QString> has_outgoing_connections;
    // Собираем все элементы, которые имеют исходящие соединения
    QList<Connection> connections = tree->getConnections();
    for (const Connection& conn : connections) {
        has_outgoing_connections.insert(conn.getFrom());
    }
    // Листья - это элементы без исходящих соединений
    QString root_name = tree->getName();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        if (!has_outgoing_connections.contains(it.key())) {
            leaves.append(it.key());
            std::cout << " Leaf found: " << it.key().toStdString() << std::endl;
        }
    }
    return leaves;
}
TreeElement* Parser_JSON::findOrCreateElement(Tree* tree, const QString& name, const QString& type)
{
    TreeElement* element = tree->findElement(name);
    if (!element) {
        // Перед созданием проверяем и удаляем возможные связи с корнем
        removeRootLinkClauses(tree, tree->getName(), name);
        int type_code;
        if (type == "Variable") {
            type_code = TreeElement::ROOT; // Основные переменные становятся корнями
        } else if (type == "Memory") {
            type_code = TreeElement::MEMORY;
        } else {
            type_code = TreeElement::ROOT; // По умолчанию
        }
        element = new TreeElement(name, type_code, current_position_++);
        tree->addElement(name, element);
        max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
        std::cout << "Created element: " << name.toStdString() << " type: " << element->getTypeName().toStdString() << std::endl;
    }
    return element;
}
void Parser_JSON::processNestedVariable(QString& current_name, const QString& field_direction)
{
    static QRegularExpression regex("^N(\\d+)$");
    QRegularExpressionMatch match = regex.match(current_name);
    if (!match.hasMatch()) {
        has_error_ = true;
        return;
    }
    int number = match.captured(1).toInt();
    if (field_direction == "left") {
        current_name = "N" + QString::number(number + 1);
    } else if (field_direction == "right") {
        current_name = "N" + QString::number(number + 2);
    }
}
void Parser_JSON::printElements() const
{
    QMap<QString, Tree*> all_trees = treeManager_->getAllTrees();
    for (auto it = all_trees.constBegin(); it != all_trees.constEnd(); ++it) {
        Tree* tree = it.value();
        std::cout << "=== Tree: " << tree->getName().toStdString() << " ===" << std::endl;
        QMap<QString, TreeElement*> elements = tree->getElements();
        for (auto elem_it = elements.constBegin(); elem_it != elements.constEnd(); ++elem_it) {
            TreeElement* element = elem_it.value();
            std::cout << "Element: " << elem_it.key().toStdString()
                      << " Type: " << element->getType()
                      << " Position: " << element->getPosition()
                      << std::endl;
        }
        QList<Connection> connections = tree->getConnections();
        for (const Connection& conn : connections) {
            std::cout << "Connection: " << conn.getFrom().toStdString()
                      << " -> " << conn.getTo().toStdString()
                      << std::endl;
        }
        std::cout << "------------------------" << std::endl;
    }
    // ВЫВОДИТЬ РЕЗУЛЬТАТ АНАЛИЗА, НО НЕ УСТАНАВЛИВАТЬ ФЛАГ
    if (has_memory_leak_) {
        std::cout << "WARNING: Memory leaks detected!" << std::endl;
    } else {
        std::cout << "No memory leaks detected." << std::endl;
    }
}

void Parser_JSON::handleNullAssignment(Tree* current_tree, const QString& var_name, int clause_id)
{
    TreeElement* element = current_tree->findElement(var_name);
    if (!element) {
        has_error_ = true;
        std::cout << "Error: Element '" << var_name.toStdString() << "' not found for Null assignment" << std::endl;
        return;
    }

    std::cout << "Executing Null assignment for: " << var_name.toStdString() << std::endl;

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    QString null_name = "N" + QString::number(memory_var_index_++);
    TreeElement* null_element = new TreeElement(null_name, TreeElement::NULL_ELEMENT, current_position_++);
    current_tree->addElement(null_name, null_element);

    removeRootLinkClauses(current_tree, current_tree->getName(), null_name);

    BoolVector pos_vec(max_bytes_count_ * 8);
    BoolVector neg_vec(max_bytes_count_ * 8);

    pos_vec.setBit(null_element->getPosition());
    neg_vec.setBit(element->getPosition());

    Connection* conn = new Connection(var_name, null_name, pos_vec, neg_vec, clause_id);
    current_tree->addConnection(*conn);
    delete conn;

    TreeElement* root_element = current_tree->getRoot();
    if (root_element) {
        BoolVector root_pos_vec(max_bytes_count_ * 8);
        BoolVector root_neg_vec(max_bytes_count_ * 8);
        root_pos_vec.setBit(root_element->getPosition());
        root_neg_vec.setBit(null_element->getPosition());

        Connection* root_conn = new Connection(null_name, current_tree->getName(),
                                               root_pos_vec, root_neg_vec, clause_id);
        current_tree->addConnection(*root_conn);
        delete root_conn;
    }

    max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));

    std::cout << "Null assignment: " << var_name.toStdString() << " -> " << null_name.toStdString()
              << " at position " << clause_id << std::endl;
}

// Заглушки для методов, которые требуют дополнительной реализации
void Parser_JSON::removeRootLinkClauses(Tree* tree, const QString& root_name, const QString& var_name)
{
    if (!tree) return;
    TreeElement* root_element = tree->getRoot();
    TreeElement* var_element = tree->findElement(var_name);
    if (!root_element || !var_element) return;
    std::cout << "Checking root connections for: " << var_name.toStdString() << std::endl;
    // Ищем и удаляем все соединения между корнем и переменной в обе стороны
    bool found_connections = false;
    // Удаляем соединения от корня к переменной
    QList<Connection> connections_from_root = tree->getConnectionsFrom(root_name);
    for (const Connection& conn : connections_from_root) {
        if (conn.getTo() == var_name) {
            tree->removeConnection(conn.getFrom(), conn.getTo());
            std::cout << "Removed root connection: " << root_name.toStdString()
                      << " -> " << var_name.toStdString() << std::endl;
            found_connections = true;
        }
    }
    // Удаляем соединения от переменной к корню
    QList<Connection> connections_to_root = tree->getConnectionsTo(root_name);
    for (const Connection& conn : connections_to_root) {
        if (conn.getFrom() == var_name) {
            tree->removeConnection(conn.getFrom(), conn.getTo());
            std::cout << "Removed root connection: " << var_name.toStdString()
                      << " -> " << root_name.toStdString() << std::endl;
            found_connections = true;
        }
    }
    if (!found_connections) {
        std::cout << "No root connections found for: " << var_name.toStdString() << std::endl;
    }
}
