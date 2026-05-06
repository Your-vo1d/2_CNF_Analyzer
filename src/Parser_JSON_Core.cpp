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

bool Parser_JSON::parseJsonFile(const QString &file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        std::cout << "Error: Failed to open file: " << file_path.toStdString() << std::endl;
        has_error_ = true;
        return false;
    }
    QJsonParseError parse_error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse_error);
    file.close();
    if (doc.isNull())
    {
        std::cout << "Error: JSON parse error: " << parse_error.errorString().toStdString() << std::endl;
        has_error_ = true;
        return false;
    }
    if (!doc.isObject())
    {
        std::cout << "Error: Invalid JSON structure" << std::endl;
        has_error_ = true;
        return false;
    }
    QJsonObject root_obj = doc.object();

    functions_ = root_obj.value("functions").toObject();
    stacks_    = root_obj.value("stacks").toObject();

    if (!root_obj.contains("code") || !root_obj["code"].isObject())
    {
        std::cout << "Error: Missing 'code' object" << std::endl;
        has_error_ = true;
        return false;
    }
    QJsonObject code_obj = root_obj["code"].toObject();
    if (!code_obj.contains("rows") || !code_obj["rows"].isArray())
    {
        std::cout << "Error: Missing 'rows' array" << std::endl;
        has_error_ = true;
        return false;
    }

    Tree *current_tree = nullptr;
    QJsonArray rows = code_obj["rows"].toArray();
    std::cout << "=== Starting parsing of " << rows.size() << " rows ===" << std::endl;

    for (const QJsonValue &row : rows)
    {
        if (!row.isObject())
        {
            std::cout << "Error: Invalid row structure" << std::endl;
            has_error_ = true;
            return false;
        }
        QJsonObject row_obj = row.toObject();
        if (!row_obj.contains("id"))
        {
            std::cout << "Error: Row missing id" << std::endl;
            has_error_ = true;
            return false;
        }
        int clause_id = row_obj["id"].toInt();
        std::cout << "\n=== Processing row " << clause_id << " ===" << std::endl;

        processRowObject(row_obj, current_tree, clause_id);

        std::cout << "\n=== Solving CNF for ALL trees after row " << clause_id << " ===" << std::endl;
        QMap<QString, Tree *> all_trees = treeManager_->getAllTrees();
        std::cout << "Total trees: " << all_trees.size() << std::endl;
        bool any_leak_detected_current_row = false;
        for (auto it = all_trees.constBegin(); it != all_trees.constEnd(); ++it)
        {
            Tree *tree = it.value();
            std::cout << "\n--- Analyzing tree: " << tree->getName().toStdString() << " ---" << std::endl;
            QList<Connection> connections = tree->getConnections();
            std::cout << "Connections (" << connections.size() << "):" << std::endl;
            for (const Connection &conn : connections)
            {
                std::cout << " " << conn.getFrom().toStdString() << " -> " << conn.getTo().toStdString();
                std::cout << " [pos:" << conn.getPosition() << "]" << std::endl;
                std::cout << " Positive positions (TO " << conn.getTo().toStdString() << "): ";
                for (size_t i = 0; i < conn.getPositiveVectorSize(); ++i)
                    if (conn.getPositiveBit(i)) std::cout << i << " ";
                std::cout << std::endl;
                std::cout << " Negative positions (FROM " << conn.getFrom().toStdString() << "): ";
                for (size_t i = 0; i < conn.getNegativeVectorSize(); ++i)
                    if (conn.getNegativeBit(i)) std::cout << i << " ";
                std::cout << std::endl;
            }
            QMap<QString, TreeElement *> elements = tree->getElements();
            std::cout << "Elements (" << elements.size() << "):" << std::endl;
            for (auto elem_it = elements.constBegin(); elem_it != elements.constEnd(); ++elem_it)
            {
                TreeElement *element = elem_it.value();
                std::cout << " " << elem_it.key().toStdString()
                          << " [type:" << element->getTypeName().toStdString()
                          << ", pos:" << element->getPosition() << "]" << std::endl;
            }
            bool tree_has_leak = solveCNF(tree);
            if (tree_has_leak)
            {
                std::cout << "MEMORY LEAK DETECTED in tree '"
                          << tree->getName().toStdString() << "' at row " << clause_id << std::endl;
                any_leak_detected_current_row = true;
            }
            else
            {
                std::cout << "No memory leak in tree '"
                          << tree->getName().toStdString() << "' at row " << clause_id << std::endl;
            }
        }
        if (any_leak_detected_current_row)
        {
            has_memory_leak_ = true;
            std::cout << "\nOVERALL: Memory leak detected at row " << clause_id << std::endl;
        }
        else
        {
            std::cout << "\nOVERALL: No memory leaks detected at row " << clause_id << std::endl;
        }
        if (has_error_)
        {
            std::cout << "Error: Parsing error at row: " << clause_id << std::endl;
            return false;
        }
        std::cout << "=== Completed row " << clause_id << " ===\n" << std::endl;
    }

    std::cout << "=== Final Analysis of ALL Trees ===" << std::endl;
    QMap<QString, Tree *> all_trees = treeManager_->getAllTrees();
    std::cout << "Total trees: " << all_trees.size() << std::endl;
    bool final_leak_detected = false;
    for (auto it = all_trees.constBegin(); it != all_trees.constEnd(); ++it)
    {
        Tree *tree = it.value();
        std::cout << "\n--- Final state of tree: " << tree->getName().toStdString() << " ---" << std::endl;
        std::cout << "Elements: "    << tree->getElementCount()    << std::endl;
        std::cout << "Connections: " << tree->getConnectionCount() << std::endl;
        std::cout << "Valid: " << (tree->isValid() ? "Yes" : "No") << std::endl;
        bool final_leak = solveCNF(tree);
        if (final_leak)
        {
            std::cout << "FINAL WARNING: Memory leak in tree '"
                      << tree->getName().toStdString() << "'" << std::endl;
            final_leak_detected = true;
        }
        else
        {
            std::cout << "FINAL: No memory leak in tree '"
                      << tree->getName().toStdString() << "'" << std::endl;
        }
    }
    has_memory_leak_ = final_leak_detected;
    if (has_memory_leak_)
        std::cout << "\nFINAL RESULT: Memory leaks detected in the program" << std::endl;
    else
        std::cout << "\nFINAL RESULT: No memory leaks detected" << std::endl;

    return true;
}

void Parser_JSON::printElements() const
{
    QMap<QString, Tree *> all_trees = treeManager_->getAllTrees();
    for (auto it = all_trees.constBegin(); it != all_trees.constEnd(); ++it)
    {
        Tree *tree = it.value();
        std::cout << "=== Tree: " << tree->getName().toStdString() << " ===" << std::endl;
        QMap<QString, TreeElement *> elements = tree->getElements();
        for (auto elem_it = elements.constBegin(); elem_it != elements.constEnd(); ++elem_it)
        {
            TreeElement *element = elem_it.value();
            std::cout << "Element: " << elem_it.key().toStdString()
                      << " Type: " << element->getType()
                      << " Position: " << element->getPosition() << std::endl;
        }
        QList<Connection> connections = tree->getConnections();
        for (const Connection &conn : connections)
        {
            std::cout << "Connection: " << conn.getFrom().toStdString()
                      << " -> " << conn.getTo().toStdString() << std::endl;
        }
        std::cout << "------------------------" << std::endl;
    }
    if (has_memory_leak_)
        std::cout << "WARNING: Memory leaks detected!" << std::endl;
    else
        std::cout << "No memory leaks detected." << std::endl;
}

void Parser_JSON::printCNFAsVectors() const
{
    QMap<QString, Tree *> all_trees = treeManager_->getAllTrees();
    for (auto it = all_trees.constBegin(); it != all_trees.constEnd(); ++it)
    {
        Tree *tree = it.value();
        std::cout << "=== CNF for tree: " << tree->getName().toStdString() << " ===" << std::endl;
        QList<Connection> connections = tree->getConnections();
        int clause_num = 0;
        for (const Connection &conn : connections)
        {
            std::cout << "Clause " << clause_num++ << " (" << conn.getFrom().toStdString()
                      << " -> " << conn.getTo().toStdString() << "): ";
            std::cout << "pos=[";
            for (size_t i = 0; i < conn.getPositiveVectorSize(); ++i)
                if (conn.getPositiveBit(i)) std::cout << i << " ";
            std::cout << "] neg=[";
            for (size_t i = 0; i < conn.getNegativeVectorSize(); ++i)
                if (conn.getNegativeBit(i)) std::cout << i << " ";
            std::cout << "]" << std::endl;
        }
        std::cout << "------------------------" << std::endl;
    }
}
