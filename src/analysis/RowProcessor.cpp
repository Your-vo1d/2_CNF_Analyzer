#include "RowProcessor.h"
#include <QRegularExpression>
#include <iostream>

RowProcessor::RowProcessor(TreeManager* manager,
                             GraphBuilder* builder,
                             QList<IMemoryOperation*> operations)
    : manager_(manager),
      builder_(builder),
      operations_(std::move(operations)),
      has_error_(false),
      has_memory_leak_(false)
{}

void RowProcessor::setFunctions(const QJsonObject& functions, const QJsonObject& stacks)
{
    functions_ = functions;
    stacks_    = stacks;
}

// ─── Tree creation ───────────────────────────────────────────────────────────

Tree* RowProcessor::getOrCreateTree(const QString& name)
{
    if (!manager_->containsTree(name)) {
        Tree* newTree = new Tree(name);
        manager_->addTree(name, newTree);
        TreeElement* rootEl = new TreeElement(name, TreeElement::ROOT,
                                               builder_->nextPosition());
        newTree->addElement(name, rootEl);
        newTree->setRoot(rootEl);
        std::cout << "Created new tree: " << name.toStdString() << std::endl;
    }
    return manager_->getTree(name);
}

// ─── Memory operation dispatch ───────────────────────────────────────────────

void RowProcessor::dispatchMemoryOp(Tree* tree, const QString& varName,
                                     int clauseId, const QJsonValue& value)
{
    for (IMemoryOperation* op : operations_) {
        if (op->handles(value)) {
            op->execute(tree, varName, clauseId, value, *builder_, has_memory_leak_);
            return;
        }
    }
    if (value.isString())
        std::cout << "Warning: Unknown operation: " << value.toString().toStdString() << std::endl;
    else if (value.isDouble())
        std::cout << "Warning: Unsupported numeric pointer value: " << value.toDouble() << std::endl;
}

// ─── Row processing ──────────────────────────────────────────────────────────

void RowProcessor::processRow(const QJsonObject& row_obj, Tree*& current_tree, int clause_id)
{
    if (row_obj.contains("operation") && row_obj.value("operation").isString()) {
        const QString op = row_obj.value("operation").toString();
        if (op == "call") {
            processCallRow(row_obj, current_tree, clause_id);
            return;
        }
        std::cout << "Warning: Unknown string operation: " << op.toStdString()
                  << " at row " << clause_id << std::endl;
        return;
    }

    if (row_obj.contains("variable")) {
        QString var_name = row_obj.value("variable").toString();
        std::cout << "Processing variable: " << var_name.toStdString() << std::endl;

        if (tryProcessArrowExpression(current_tree, var_name, row_obj, clause_id))
            return;

        current_tree = getOrCreateTree(var_name);

        if (row_obj.contains("value")) {
            QJsonValue val = row_obj.value("value");
            if (val.isNull() || val.isString() || val.isDouble()) {
                dispatchMemoryOp(current_tree, var_name, clause_id, val);
                return;
            }
            if (val.isObject()) {
                std::cout << "Processing nested value for: " << var_name.toStdString() << std::endl;
                processNestedValue(val.toObject(), current_tree, var_name, clause_id);
                return;
            }
        }

        if (row_obj.contains("field")) {
            std::cout << "Processing structure field for: " << var_name.toStdString() << std::endl;
            processStructureField(row_obj.value("field"), current_tree, var_name, clause_id);
            return;
        }
        return;
    }

    if (row_obj.contains("operation") && row_obj.value("operation").isObject()) {
        std::cout << "Processing operation" << std::endl;
        if (!current_tree) {
            if (!manager_->getAllTrees().isEmpty())
                current_tree = manager_->getAllTrees().first();
        }
        if (current_tree) {
            processOperation(row_obj.value("operation").toObject(), current_tree, clause_id);
        } else {
            std::cout << "Error: No tree available for operation" << std::endl;
            has_error_ = true;
        }
        return;
    }

    std::cout << "Warning: Row " << clause_id
              << " doesn't contain variable or operation" << std::endl;
}

void RowProcessor::processCallRow(const QJsonObject& row_obj, Tree*& current_tree,
                                   int call_clause_id)
{
    const QString funcId   = row_obj.value("function_id").toString();
    const QJsonArray params = row_obj.value("params").toArray();
    const QString context  = row_obj.value("context").toString();

    if (funcId.isEmpty() || !functions_.contains(funcId) ||
        !functions_.value(funcId).isObject())
    {
        std::cout << "Error: function_id not found or invalid: " << funcId.toStdString()
                  << " at row " << call_clause_id << std::endl;
        has_error_ = true;
        return;
    }

    if (!context.isEmpty() && stacks_.contains(context) &&
        stacks_.value(context).isObject())
    {
        QJsonObject st = stacks_.value(context).toObject();
        const QString treeName = st.value("tree").toString();
        if (!treeName.isEmpty() && manager_->containsTree(treeName))
            current_tree = manager_->getTree(treeName);
    }

    const QJsonObject funcObj  = functions_.value(funcId).toObject();
    const QJsonArray  funcRows = funcObj.value("rows").toArray();

    std::cout << "Calling function '" << funcId.toStdString()
              << "' with " << params.size() << " param(s)" << std::endl;

    for (const QJsonValue& rv : funcRows) {
        if (!rv.isObject()) continue;
        QJsonObject rr = substituteParams(rv, params).toObject();
        int inner_id    = rr.value("id").toInt();
        int composed_id = call_clause_id * 1000 + inner_id;
        std::cout << "  -> function row id " << inner_id
                  << " (composed " << composed_id << ")" << std::endl;
        processRow(rr, current_tree, composed_id);
        if (has_error_) return;
    }
}

QString RowProcessor::substituteParamsInString(const QString& s,
                                                const QJsonArray& params) const
{
    QString out = s;
    QRegularExpression re(R"(\$param(\d+))");
    QRegularExpressionMatchIterator it = re.globalMatch(out);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        int idx = m.captured(1).toInt();
        if (idx >= 0 && idx < params.size() && params[idx].isString())
            out.replace(m.captured(0), params[idx].toString());
    }
    return out;
}

QJsonValue RowProcessor::substituteParams(const QJsonValue& v,
                                           const QJsonArray& params) const
{
    if (v.isString())
        return substituteParamsInString(v.toString(), params);
    if (v.isArray()) {
        QJsonArray a = v.toArray();
        for (int i = 0; i < a.size(); ++i)
            a[i] = substituteParams(a[i], params);
        return a;
    }
    if (v.isObject()) {
        QJsonObject o = v.toObject();
        for (auto it = o.begin(); it != o.end(); ++it)
            it.value() = substituteParams(it.value(), params);
        return o;
    }
    return v;
}

bool RowProcessor::tryProcessArrowExpression(Tree*& current_tree,
                                              const QString& var_expr,
                                              const QJsonObject& row_obj,
                                              int clause_id)
{
    if (!var_expr.contains("->")) return false;

    QStringList parts = var_expr.split("->", Qt::SkipEmptyParts);
    if (parts.size() < 2) return false;

    const QString baseVar = parts.takeFirst();
    if (baseVar.isEmpty()) return false;

    current_tree = getOrCreateTree(baseVar);

    bool hasField = row_obj.contains("field");
    bool hasValue = row_obj.contains("value");
    if (!hasField && !hasValue) {
        std::cout << "Warning: Arrow variable without value/field at row "
                  << clause_id << std::endl;
        return true;
    }

    QJsonObject terminal;
    terminal.insert("f", parts.last());
    if (hasField)
        terminal.insert("value", row_obj.value("field"));
    else
        terminal.insert("value", row_obj.value("value"));

    QJsonObject chain = terminal;
    for (int i = parts.size() - 2; i >= 0; --i) {
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

// ─── Body / branch / loop / operation ────────────────────────────────────────

void RowProcessor::processBody(const QJsonValue& body_value, Tree* current_tree,
                                int clause_id)
{
    if (body_value.isObject()) {
        QJsonObject body_obj = body_value.toObject();
        if (body_obj.contains("variable")) {
            QString var_name = body_obj["variable"].toString();
            builder_->findOrCreateElement(current_tree, var_name, TreeElement::ROOT);
            if (body_obj.contains("value")) {
                QJsonValue val = body_obj["value"];
                if (val.isNull() || val.isString()) {
                    dispatchMemoryOp(current_tree, var_name, clause_id, val);
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
        for (const QJsonValue& item : body_array)
            processBody(item, current_tree, clause_id);
    }
}

void RowProcessor::processBranch(const QJsonObject& branch_obj, Tree* current_tree,
                                  int clause_id)
{
    if (branch_obj.contains("body"))
        processBody(branch_obj["body"], current_tree, clause_id);
}

void RowProcessor::processOperation(const QJsonObject& op_object, Tree* current_tree,
                                     int clause_id)
{
    if (!op_object.contains("op")) {
        has_error_ = true;
        return;
    }
    const QString op_type = op_object["op"].toString();
    if (op_type == "loop") {
        processLoop(op_object, current_tree, clause_id);
    } else {
        if (op_object.contains("branch true")) {
            QJsonValue bt = op_object["branch true"];
            if (bt.isObject())
                processBranch(bt.toObject(), current_tree, clause_id);
        }
        if (op_object.contains("branch false")) {
            QJsonValue bf = op_object["branch false"];
            if (bf.isObject())
                processBranch(bf.toObject(), current_tree, clause_id);
        }
    }
}

void RowProcessor::processLoop(const QJsonObject& loop_object, Tree* current_tree,
                                int clause_id)
{
    if (!loop_object.contains("condition") || !loop_object.contains("body")) {
        has_error_ = true;
        std::cout << "Error: Loop missing condition or body" << std::endl;
        return;
    }
    std::cout << "Warning: Loop processed as single iteration" << std::endl;
    processBody(loop_object["body"], current_tree, clause_id);
}

// ─── Structure field navigation ──────────────────────────────────────────────

void RowProcessor::processStructureField(const QJsonValue& field_value,
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

    builder_->removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    if (field_obj.contains("f") && field_obj["f"].isString()) {
        std::cout << "Navigating to field: "
                  << field_obj["f"].toString().toStdString() << std::endl;
        QString target = navigateByField(current_tree, current_var_name,
                                          field_obj["f"].toString(), clause_id);
        if (target.isEmpty()) {
            std::cout << "Failed to navigate to field" << std::endl;
            return;
        }
        current_var_name = target;
        std::cout << "Successfully navigated to: " << current_var_name.toStdString() << std::endl;
        builder_->removeRootLinkClauses(current_tree, current_tree->getName(), current_var_name);
    }

    if (!current_tree->findElement(current_var_name)) {
        std::cout << "Error: Element '" << current_var_name.toStdString()
                  << "' not found after navigation" << std::endl;
        has_error_ = true;
        return;
    }

    if (field_obj.contains("op"))
        processStructureField(field_obj["op"], current_tree, current_var_name, clause_id);

    if (field_obj.contains("value")) {
        QJsonValue val = field_obj["value"];
        if (val.isNull() || val.isString()) {
            dispatchMemoryOp(current_tree, current_var_name, clause_id, val);
        } else if (val.isObject()) {
            processNestedValue(val.toObject(), current_tree, current_var_name, clause_id);
        }
    } else {
        std::cout << "No value specified for field, navigation completed" << std::endl;
    }
}

QString RowProcessor::navigateByField(Tree* tree, const QString& current_name,
                                       const QString& field_direction, int /*clause_id*/)
{
    TreeElement* current_element = tree->findElement(current_name);
    if (!current_element) {
        std::cout << "Error: Current element '" << current_name.toStdString()
                  << "' not found" << std::endl;
        has_error_ = true;
        return QString();
    }

    int required_type;
    if (field_direction == "left")
        required_type = TreeElement::LEFT_VARIABLE;
    else if (field_direction == "right")
        required_type = TreeElement::RIGHT_VARIABLE;
    else {
        std::cout << "Error: Unknown field direction '" << field_direction.toStdString()
                  << "'" << std::endl;
        has_error_ = true;
        return QString();
    }

    // Direct connection to required type
    for (const Connection& conn : tree->getConnectionsFrom(current_name)) {
        TreeElement* target = tree->findElement(conn.getTo());
        if (target && target->getType() == required_type)
            return target->getName();
    }

    // Via memory node
    for (const Connection& conn : tree->getConnectionsFrom(current_name)) {
        TreeElement* intermediate = tree->findElement(conn.getTo());
        if (intermediate && intermediate->isMemory()) {
            for (const Connection& mc : tree->getConnectionsFrom(intermediate->getName())) {
                TreeElement* target = tree->findElement(mc.getTo());
                if (target && target->getType() == required_type)
                    return target->getName();
            }
        }
    }

    std::cout << "Error: No " << field_direction.toStdString()
              << " variable found for element '" << current_name.toStdString() << "'" << std::endl;
    has_error_ = true;
    return QString();
}

void RowProcessor::processNestedValue(const QJsonObject& value_obj,
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
              << " = " << nested_var_name.toStdString() << std::endl;

    TreeElement* target_element = current_tree->findElement(var_name);
    if (!target_element) {
        std::cout << "Error: Target element '" << var_name.toStdString()
                  << "' not found" << std::endl;
        has_error_ = true;
        return;
    }

    builder_->removeRootLinkClauses(current_tree, current_tree->getName(), var_name);

    QList<Connection> prev_out = current_tree->getConnectionsFrom(var_name);
    for (const Connection& prev : prev_out)
        current_tree->removeConnection(prev.getFrom(), prev.getTo());

    TreeElement* source_element = current_tree->findElement(nested_var_name);
    if (source_element) {
        size_t bits = builder_->maxBytesCount() * 8;
        BoolVector pos_vec(bits), neg_vec(bits);
        pos_vec.setBit(source_element->getPosition());
        neg_vec.setBit(target_element->getPosition());
        current_tree->addConnection(
            Connection(var_name, nested_var_name, pos_vec, neg_vec, clause_id));
        std::cout << "Connection created: " << var_name.toStdString()
                  << " -> " << nested_var_name.toStdString() << std::endl;
    } else {
        // Source in another tree — merge
        std::cout << "Source element not found in current tree, checking other trees..." << std::endl;
        Tree* source_tree = manager_->getTree(nested_var_name);
        if (!source_tree || source_tree == current_tree) {
            std::cout << "Error: Source tree '" << nested_var_name.toStdString()
                      << "' not found" << std::endl;
            has_error_ = true;
            return;
        }

        TreeElement* source_memory = builder_->findMemoryElement(source_tree);
        if (!source_memory) {
            std::cout << "Error: No memory element found in source tree" << std::endl;
            has_error_ = true;
            return;
        }
        QString source_mem_name = source_memory->getName();

        builder_->mergeTrees(current_tree, source_tree);

        TreeElement* source_mem_now = current_tree->findElement(source_mem_name);
        if (!source_mem_now) {
            std::cout << "Error: Source memory not found after merge" << std::endl;
            has_error_ = true;
            return;
        }
        size_t bits = builder_->maxBytesCount() * 8;
        BoolVector pos_vec(bits), neg_vec(bits);
        pos_vec.setBit(source_mem_now->getPosition());
        neg_vec.setBit(target_element->getPosition());
        current_tree->addConnection(
            Connection(var_name, source_mem_name, pos_vec, neg_vec, clause_id));
        std::cout << "Connection created after merge: " << var_name.toStdString()
                  << " -> " << source_mem_name.toStdString() << std::endl;
    }

    if (value_obj.contains("field")) {
        QString current_name = nested_var_name;
        processValueField(value_obj["field"], current_tree, current_name, 1);
    }
}

void RowProcessor::processValueField(const QJsonValue& field_value,
                                      Tree* current_tree,
                                      QString& last_name,
                                      int nesting_level)
{
    if (!field_value.isObject()) {
        has_error_ = true;
        std::cout << "Error: Field value is not an object at nesting level "
                  << nesting_level << std::endl;
        return;
    }
    QJsonObject field_obj = field_value.toObject();
    if (!field_obj.contains("f")) {
        has_error_ = true;
        std::cout << "Error: Field object missing 'f' at nesting level "
                  << nesting_level << std::endl;
        return;
    }
    QString field_direction = field_obj["f"].toString();

    QString new_name = navigateByField(current_tree, last_name, field_direction, -1);
    if (new_name.isEmpty()) {
        has_error_ = true;
        return;
    }
    last_name = new_name;
    std::cout << "Navigated to nested variable: " << last_name.toStdString() << std::endl;

    if (field_obj.contains("op"))
        processValueField(field_obj["op"], current_tree, last_name, nesting_level + 1);
}

void RowProcessor::processNestedVariable(QString& current_name,
                                          const QString& field_direction)
{
    static QRegularExpression regex("^N(\\d+)$");
    QRegularExpressionMatch match = regex.match(current_name);
    if (!match.hasMatch()) {
        has_error_ = true;
        return;
    }
    int number = match.captured(1).toInt();
    if (field_direction == "left")
        current_name = "N" + QString::number(number + 1);
    else if (field_direction == "right")
        current_name = "N" + QString::number(number + 2);
}
