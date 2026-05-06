#include "Parser_JSON.h"

void Parser_JSON::processRowObject(const QJsonObject &row_obj, Tree *&current_tree, int clause_id)
{
    if (row_obj.contains("operation") && row_obj.value("operation").isString())
    {
        const QString op = row_obj.value("operation").toString();
        if (op == "call")
        {
            processCallRow(row_obj, current_tree, clause_id);
            return;
        }
        std::cout << "Warning: Unknown string operation: " << op.toStdString()
                  << " at row " << clause_id << std::endl;
        return;
    }

    if (row_obj.contains("variable"))
    {
        QString var_name = row_obj.value("variable").toString();
        std::cout << "Processing variable: " << var_name.toStdString() << std::endl;

        if (tryProcessArrowExpression(current_tree, var_name, row_obj, clause_id))
            return;

        if (!treeManager_->containsTree(var_name))
        {
            Tree *new_tree = new Tree(var_name);
            treeManager_->addTree(var_name, new_tree);
            TreeElement *root_element = new TreeElement(var_name, TreeElement::ROOT, current_position_++);
            new_tree->addElement(var_name, root_element);
            new_tree->setRoot(root_element);
            max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
            std::cout << "Created new tree: " << var_name.toStdString() << std::endl;
        }
        current_tree = treeManager_->getTree(var_name);

        if (row_obj.contains("value"))
        {
            QJsonValue val = row_obj.value("value");
            if (val.isNull())
            {
                std::cout << "Assigning null to variable" << std::endl;
                handleNullAssignment(current_tree, var_name, clause_id);
                return;
            }
            if (val.isString())
            {
                QString operation = val.toString();
                std::cout << "Handling memory operation '" << operation.toStdString()
                          << "' for: " << var_name.toStdString() << std::endl;

                if (operation == "malloc" || operation == "calloc" ||
                    operation == "realloc" || operation == "aligned_alloc" ||
                    operation == "new"  || operation == "new[]")
                {
                    handleMemoryAllocation(current_tree, var_name, clause_id, operation);
                }
                else if (operation == "free" || operation == "delete" || operation == "delete[]")
                {
                    handleFree(current_tree, var_name);
                }
                else
                {
                    std::cout << "Warning: Unknown memory operation: " << operation.toStdString() << std::endl;
                }
                return;
            }
            if (val.isObject())
            {
                std::cout << "Processing nested value for: " << var_name.toStdString() << std::endl;
                processNestedValue(val.toObject(), current_tree, var_name, clause_id);
                return;
            }
            if (val.isDouble())
            {
                double numVal = val.toDouble();
                if (numVal == 4294967295.0 || numVal == 0.0)
                {
                    std::cout << "Assigning special pointer value (0x"
                              << std::hex << (uint32_t)numVal << std::dec << ") to: "
                              << var_name.toStdString() << std::endl;
                    handleSpecialPointerAssignment(current_tree, var_name, clause_id, (uint32_t)numVal);
                }
                else
                {
                    std::cout << "Warning: Unsupported numeric pointer value: " << numVal << std::endl;
                }
                return;
            }
        }

        if (row_obj.contains("field"))
        {
            std::cout << "Processing structure field for: " << var_name.toStdString() << std::endl;
            processStructureField(row_obj.value("field"), current_tree, var_name, clause_id);
            return;
        }
        return;
    }

    if (row_obj.contains("operation") && row_obj.value("operation").isObject())
    {
        std::cout << "Processing operation" << std::endl;
        if (!current_tree)
        {
            std::cout << "Warning: Operation without current tree, using first available tree" << std::endl;
            if (!treeManager_->getAllTrees().isEmpty())
                current_tree = treeManager_->getAllTrees().first();
        }
        if (current_tree)
        {
            processOperation(row_obj.value("operation").toObject(), current_tree, clause_id);
        }
        else
        {
            std::cout << "Error: No tree available for operation" << std::endl;
            has_error_ = true;
        }
        return;
    }

    std::cout << "Warning: Row " << clause_id << " doesn't contain variable or operation" << std::endl;
}

void Parser_JSON::processCallRow(const QJsonObject &row_obj, Tree *&current_tree, int call_clause_id)
{
    const QString funcId  = row_obj.value("function_id").toString();
    const QJsonArray params = row_obj.value("params").toArray();
    const QString context = row_obj.value("context").toString();

    if (funcId.isEmpty() || !functions_.contains(funcId) || !functions_.value(funcId).isObject())
    {
        std::cout << "Error: function_id not found or invalid: " << funcId.toStdString()
                  << " at row " << call_clause_id << std::endl;
        has_error_ = true;
        return;
    }

    if (!context.isEmpty() && stacks_.contains(context) && stacks_.value(context).isObject())
    {
        QJsonObject st = stacks_.value(context).toObject();
        const QString treeName = st.value("tree").toString();
        if (!treeName.isEmpty() && treeManager_->containsTree(treeName))
            current_tree = treeManager_->getTree(treeName);
    }

    const QJsonObject funcObj  = functions_.value(funcId).toObject();
    const QJsonArray  funcRows = funcObj.value("rows").toArray();

    std::cout << "Calling function '" << funcId.toStdString() << "' with "
              << params.size() << " param(s)" << std::endl;

    for (const QJsonValue &rv : funcRows)
    {
        if (!rv.isObject()) continue;
        QJsonObject rr = substituteParams(rv, params).toObject();
        int inner_id    = rr.value("id").toInt();
        int composed_id = call_clause_id * 1000 + inner_id;
        std::cout << "  -> function row id " << inner_id
                  << " (composed " << composed_id << ")" << std::endl;
        processRowObject(rr, current_tree, composed_id);
        if (has_error_) return;
    }
}

QString Parser_JSON::substituteParamsInString(const QString &s, const QJsonArray &params) const
{
    QString out = s;
    QRegularExpression re(R"(\$param(\d+))");
    QRegularExpressionMatchIterator it = re.globalMatch(out);
    while (it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        int idx = m.captured(1).toInt();
        if (idx >= 0 && idx < params.size() && params[idx].isString())
            out.replace(m.captured(0), params[idx].toString());
    }
    return out;
}

QJsonValue Parser_JSON::substituteParams(const QJsonValue &v, const QJsonArray &params) const
{
    if (v.isString())
        return substituteParamsInString(v.toString(), params);
    if (v.isArray())
    {
        QJsonArray a = v.toArray();
        for (int i = 0; i < a.size(); ++i)
            a[i] = substituteParams(a[i], params);
        return a;
    }
    if (v.isObject())
    {
        QJsonObject o = v.toObject();
        for (auto it = o.begin(); it != o.end(); ++it)
            it.value() = substituteParams(it.value(), params);
        return o;
    }
    return v;
}

bool Parser_JSON::tryProcessArrowExpression(Tree *&current_tree,
                                            const QString &var_expr,
                                            const QJsonObject &row_obj,
                                            int clause_id)
{
    if (!var_expr.contains("->"))
        return false;

    QStringList parts = var_expr.split("->", Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return false;

    const QString baseVar = parts.takeFirst();
    if (baseVar.isEmpty())
        return false;

    if (!treeManager_->containsTree(baseVar))
    {
        Tree *new_tree = new Tree(baseVar);
        treeManager_->addTree(baseVar, new_tree);
        TreeElement *root_element = new TreeElement(baseVar, TreeElement::ROOT, current_position_++);
        new_tree->addElement(baseVar, root_element);
        new_tree->setRoot(root_element);
        max_bytes_count_ = std::max(max_bytes_count_, BoolVector::calculateBytes(current_position_));
        std::cout << "Created new tree (arrow base): " << baseVar.toStdString() << std::endl;
    }
    current_tree = treeManager_->getTree(baseVar);

    bool hasField = row_obj.contains("field");
    bool hasValue = row_obj.contains("value");
    if (!hasField && !hasValue)
    {
        std::cout << "Warning: Arrow variable without value/field at row " << clause_id << std::endl;
        return true;
    }

    QJsonObject terminal;
    terminal.insert("f", parts.last());
    if (hasField)
        terminal.insert("value", row_obj.value("field"));
    else
        terminal.insert("value", row_obj.value("value"));

    QJsonObject chain = terminal;
    for (int i = parts.size() - 2; i >= 0; --i)
    {
        QJsonObject wrapper;
        wrapper.insert("f", parts[i]);
        wrapper.insert("op", chain);
        chain = wrapper;
    }

    std::cout << "Processing arrow expression for base '" << baseVar.toStdString()
              << "' (depth " << parts.size() << ")" << std::endl;

    processStructureField(chain, current_tree, baseVar, clause_id);
    return true;
}
