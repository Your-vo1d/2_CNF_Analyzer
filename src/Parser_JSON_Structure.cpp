#include "Parser_JSON.h"

void Parser_JSON::processBody(const QJsonValue &body_value, Tree *current_tree, int clause_id)
{
    if (body_value.isObject())
    {
        QJsonObject body_obj = body_value.toObject();
        if (body_obj.contains("variable"))
        {
            QString var_name = body_obj["variable"].toString();
            findOrCreateElement(current_tree, var_name, "Variable");
            if (body_obj.contains("value"))
            {
                QJsonValue val = body_obj["value"];
                if (val.isNull())
                {
                    handleNullAssignment(current_tree, var_name, clause_id);
                    return;
                }
                if (val.isString())
                {
                    QString operation = val.toString();
                    if (operation == "malloc" || operation == "calloc" || operation == "realloc" ||
                        operation == "new"  || operation == "new[]")
                    {
                        handleMemoryAllocation(current_tree, var_name, clause_id, operation);
                    }
                    else if (operation == "free" || operation == "delete" || operation == "delete[]")
                    {
                        handleFree(current_tree, var_name);
                    }
                }
                else if (val.isObject())
                {
                    processNestedValue(val.toObject(), current_tree, var_name, clause_id);
                }
            }
            else if (body_obj.contains("field"))
            {
                processStructureField(body_obj["field"], current_tree, var_name, clause_id);
            }
        }
        else if (body_obj.contains("operation"))
        {
            processOperation(body_obj["operation"].toObject(), current_tree, clause_id);
        }
    }
    else if (body_value.isArray())
    {
        QJsonArray body_array = body_value.toArray();
        for (const QJsonValue &item : body_array)
            processBody(item, current_tree, clause_id);
    }
}

void Parser_JSON::processBranch(const QJsonObject &branch_obj, Tree *current_tree, int clause_id)
{
    if (branch_obj.contains("body"))
        processBody(branch_obj["body"], current_tree, clause_id);
}

void Parser_JSON::processOperation(const QJsonObject &op_object, Tree *current_tree, int clause_id)
{
    if (!op_object.contains("op"))
    {
        has_error_ = true;
        return;
    }
    QString op_type = op_object["op"].toString();
    if (op_type == "loop")
    {
        processLoop(op_object, current_tree, clause_id);
    }
    else
    {
        if (op_object.contains("branch true"))
        {
            QJsonValue branch_true = op_object["branch true"];
            if (branch_true.isObject())
                processBranch(branch_true.toObject(), current_tree, clause_id);
        }
        if (op_object.contains("branch false"))
        {
            QJsonValue branch_false = op_object["branch false"];
            if (branch_false.isObject())
                processBranch(branch_false.toObject(), current_tree, clause_id);
        }
    }
}

void Parser_JSON::processLoop(const QJsonObject &loop_object, Tree *current_tree, int clause_id)
{
    if (!loop_object.contains("condition") || !loop_object.contains("body"))
    {
        has_error_ = true;
        std::cout << "Error: Loop missing condition or body" << std::endl;
        return;
    }
    std::cout << "Warning: Loop processed as single iteration" << std::endl;
    processBody(loop_object["body"], current_tree, clause_id);
}

void Parser_JSON::processStructureField(const QJsonValue &field_value,
                                        Tree *current_tree,
                                        const QString &var_name,
                                        int clause_id)
{
    if (!field_value.isObject())
    {
        std::cout << "Error: Field value is not an object" << std::endl;
        has_error_ = true;
        return;
    }
    QJsonObject field_obj = field_value.toObject();
    QString current_var_name = var_name;
    std::cout << "Starting structure field processing for: " << var_name.toStdString() << std::endl;

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    if (field_obj.contains("f"))
    {
        QJsonValue f_value = field_obj["f"];
        if (f_value.isString())
        {
            std::cout << "Navigating to field: " << f_value.toString().toStdString() << std::endl;
            QString target_var_name = navigateByField(current_tree, current_var_name, f_value.toString(), clause_id);
            if (target_var_name.isEmpty())
            {
                std::cout << "Failed to navigate to field" << std::endl;
                return;
            }
            current_var_name = target_var_name;
            std::cout << "Successfully navigated to: " << current_var_name.toStdString() << std::endl;
            removeRootLinkClauses(current_tree, current_tree->getName(), current_var_name);
        }
    }

    TreeElement *element = current_tree->findElement(current_var_name);
    if (!element)
    {
        std::cout << "Error: Element '" << current_var_name.toStdString() << "' not found after navigation" << std::endl;
        has_error_ = true;
        return;
    }

    if (field_obj.contains("op"))
    {
        std::cout << "Processing op field for: " << current_var_name.toStdString() << std::endl;
        processStructureField(field_obj["op"], current_tree, current_var_name, clause_id);
    }

    if (field_obj.contains("value"))
    {
        QJsonValue value = field_obj["value"];
        if (value.isNull())
        {
            std::cout << "Assigning null to field" << std::endl;
            handleNullAssignment(current_tree, current_var_name, clause_id);
            return;
        }
        if (value.isString())
        {
            QString operation = value.toString();
            if (operation == "malloc" || operation == "calloc" || operation == "realloc" ||
                operation == "new"  || operation == "new[]")
            {
                handleMemoryAllocation(current_tree, current_var_name, clause_id, operation);
            }
            else if (operation == "free" || operation == "delete" || operation == "delete[]")
            {
                handleFree(current_tree, current_var_name);
            }
        }
        else if (value.isObject())
        {
            std::cout << "Processing nested value for: " << current_var_name.toStdString() << std::endl;
            processNestedValue(value.toObject(), current_tree, current_var_name, clause_id);
        }
    }
    else
    {
        std::cout << "No value specified for field, navigation completed" << std::endl;
    }
}

QString Parser_JSON::navigateByField(Tree *tree,
                                     const QString &current_name,
                                     const QString &field_direction,
                                     int clause_id)
{
    TreeElement *current_element = tree->findElement(current_name);
    if (!current_element)
    {
        std::cout << "Error: Current element '" << current_name.toStdString() << "' not found" << std::endl;
        has_error_ = true;
        return QString();
    }
    std::cout << "Looking for " << field_direction.toStdString() << " field from " << current_name.toStdString()
              << " [type: " << current_element->getTypeName().toStdString() << "]" << std::endl;

    int required_type;
    if (field_direction == "left")
        required_type = TreeElement::LEFT_VARIABLE;
    else if (field_direction == "right")
        required_type = TreeElement::RIGHT_VARIABLE;
    else
    {
        std::cout << "Error: Unknown field direction '" << field_direction.toStdString() << "'" << std::endl;
        has_error_ = true;
        return QString();
    }

    QList<Connection> connections_from = tree->getConnectionsFrom(current_name);
    std::cout << " Direct connections from " << current_name.toStdString() << ": ";
    for (const Connection &conn : connections_from)
        std::cout << conn.getTo().toStdString() << " ";
    std::cout << std::endl;

    for (const Connection &conn : connections_from)
    {
        TreeElement *target_element = tree->findElement(conn.getTo());
        if (target_element && target_element->getType() == required_type)
        {
            std::cout << "Found direct " << field_direction.toStdString()
                      << " connection: " << current_name.toStdString()
                      << " -> " << target_element->getName().toStdString()
                      << " [type: " << target_element->getTypeName().toStdString() << "]" << std::endl;
            return target_element->getName();
        }
    }

    for (const Connection &conn : connections_from)
    {
        TreeElement *intermediate_element = tree->findElement(conn.getTo());
        if (intermediate_element && intermediate_element->isMemory())
        {
            std::cout << "Found memory element: " << intermediate_element->getName().toStdString() << std::endl;
            QList<Connection> memory_connections = tree->getConnectionsFrom(intermediate_element->getName());
            for (const Connection &mem_conn : memory_connections)
            {
                TreeElement *target_element = tree->findElement(mem_conn.getTo());
                if (target_element && target_element->getType() == required_type)
                {
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

    std::cout << "Error: No " << field_direction.toStdString()
              << " variable found for element '" << current_name.toStdString() << "'" << std::endl;
    QMap<QString, TreeElement *> all_elements = tree->getElements();
    std::cout << " Available elements in tree: ";
    for (auto it = all_elements.constBegin(); it != all_elements.constEnd(); ++it)
        if (it.value()->getType() == required_type)
            std::cout << it.key().toStdString() << "[" << it.value()->getTypeName().toStdString() << "] ";
    std::cout << std::endl;
    has_error_ = true;
    return QString();
}

void Parser_JSON::processNestedValue(const QJsonObject &value_obj,
                                     Tree *current_tree,
                                     const QString &var_name,
                                     int clause_id)
{
    if (!value_obj.contains("variable"))
    {
        std::cout << "Error: Nested value missing 'variable' field" << std::endl;
        has_error_ = true;
        return;
    }
    QString nested_var_name = value_obj["variable"].toString();
    std::cout << "Processing nested variable assignment: " << var_name.toStdString()
              << " = " << nested_var_name.toStdString() << " at clause " << clause_id << std::endl;

    TreeElement *target_element = current_tree->findElement(var_name);
    if (!target_element)
    {
        std::cout << "Error: Target element '" << var_name.toStdString() << "' not found" << std::endl;
        has_error_ = true;
        return;
    }
    std::cout << "Target element: " << target_element->getName().toStdString()
              << " [pos: " << target_element->getPosition() << "]" << std::endl;

    removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    QList<Connection> prev_out = current_tree->getConnectionsFrom(var_name);
    for (const Connection &prev : prev_out)
    {
        current_tree->removeConnection(prev.getFrom(), prev.getTo());
        std::cout << "Removed previous assignment: " << var_name.toStdString()
                  << " -> " << prev.getTo().toStdString() << std::endl;
    }

    TreeElement *source_element = current_tree->findElement(nested_var_name);
    if (source_element)
    {
        std::cout << "Source element found in current tree: " << source_element->getName().toStdString() << std::endl;
        BoolVector pos_vec(max_bytes_count_ * 8);
        BoolVector neg_vec(max_bytes_count_ * 8);
        pos_vec.setBit(source_element->getPosition());
        neg_vec.setBit(target_element->getPosition());
        Connection conn(var_name, nested_var_name, pos_vec, neg_vec, clause_id);
        current_tree->addConnection(conn);
        std::cout << "Connection created successfully: " << var_name.toStdString()
                  << " -> " << nested_var_name.toStdString() << std::endl;
    }
    else
    {
        std::cout << "Source element not found in current tree, checking other trees..." << std::endl;
        Tree *source_tree = treeManager_->getTree(nested_var_name);
        if (source_tree && source_tree != current_tree)
        {
            std::cout << "Found source tree: " << source_tree->getName().toStdString() << std::endl;
            TreeElement *source_memory_element = findMemoryElement(source_tree);
            if (!source_memory_element)
            {
                std::cout << "Error: No memory element found in source tree" << std::endl;
                has_error_ = true;
                return;
            }
            QString source_mem_name = source_memory_element->getName();
            std::cout << "Starting merge process..." << std::endl;
            mergeTrees(current_tree, source_tree);

            TreeElement *source_mem_now = current_tree->findElement(source_mem_name);
            if (!source_mem_now)
            {
                std::cout << "Error: Source memory not found after merge" << std::endl;
                has_error_ = true;
                return;
            }
            BoolVector pos_vec(max_bytes_count_ * 8);
            BoolVector neg_vec(max_bytes_count_ * 8);
            pos_vec.setBit(source_mem_now->getPosition());
            neg_vec.setBit(target_element->getPosition());
            Connection conn(var_name, source_mem_name, pos_vec, neg_vec, clause_id);
            current_tree->addConnection(conn);
            std::cout << "Connection created: " << var_name.toStdString()
                      << " -> " << source_mem_name.toStdString() << std::endl;

            if (!current_tree->findConnection(var_name, source_mem_name))
                std::cout << "ERROR: Connection lost after merge!" << std::endl;
        }
        else
        {
            std::cout << "Error: Source tree '" << nested_var_name.toStdString() << "' not found" << std::endl;
            has_error_ = true;
            return;
        }
    }

    if (value_obj.contains("field"))
    {
        QString current_name = nested_var_name;
        processValueField(value_obj["field"], current_tree, current_name, 1);
    }
}

void Parser_JSON::processValueField(const QJsonValue &field_value,
                                    Tree *current_tree,
                                    QString &last_name,
                                    int nesting_level)
{
    if (!field_value.isObject())
    {
        has_error_ = true;
        std::cout << "Error: Field value is not an object at nesting level " << nesting_level << std::endl;
        return;
    }
    QJsonObject field_obj = field_value.toObject();
    if (!field_obj.contains("f"))
    {
        has_error_ = true;
        std::cout << "Error: Field object missing 'f' field at nesting level " << nesting_level << std::endl;
        return;
    }
    QString field_direction = field_obj["f"].toString();
    std::cout << "Processing nested field: " << field_direction.toStdString()
              << " for variable: " << last_name.toStdString()
              << " at nesting level: " << nesting_level << std::endl;

    QString new_name = navigateByField(current_tree, last_name, field_direction, -1);
    if (new_name.isEmpty())
    {
        has_error_ = true;
        std::cout << "Error: Failed to navigate nested field" << std::endl;
        return;
    }
    last_name = new_name;
    std::cout << "Navigated to nested variable: " << last_name.toStdString() << std::endl;

    if (field_obj.contains("op"))
        processValueField(field_obj["op"], current_tree, last_name, nesting_level + 1);
}

void Parser_JSON::processNestedVariable(QString &current_name, const QString &field_direction)
{
    static QRegularExpression regex("^N(\\d+)$");
    QRegularExpressionMatch match = regex.match(current_name);
    if (!match.hasMatch())
    {
        has_error_ = true;
        return;
    }
    int number = match.captured(1).toInt();
    if (field_direction == "left")
        current_name = "N" + QString::number(number + 1);
    else if (field_direction == "right")
        current_name = "N" + QString::number(number + 2);
}
