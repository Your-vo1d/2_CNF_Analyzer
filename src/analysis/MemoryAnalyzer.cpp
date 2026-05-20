#include "MemoryAnalyzer.h"
#include "MemoryOperations.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

MemoryAnalyzer::MemoryAnalyzer(std::unique_ptr<ISATSolver> solver)
    : solver_(std::move(solver)),
      treeManager_(new TreeManager()),
      graphBuilder_(new GraphBuilder(treeManager_.get())),
      has_error_(false),
      has_memory_leak_(false)
{
    registerDefaultOperations();
    rowProcessor_ = std::make_unique<RowProcessor>(
        treeManager_.get(), graphBuilder_.get(), operations_);
}

MemoryAnalyzer::~MemoryAnalyzer()
{
    qDeleteAll(operations_);
}

void MemoryAnalyzer::registerDefaultOperations()
{
    operations_.append(new NullOperation());
    operations_.append(new MallocOperation());
    operations_.append(new FreeOperation());
    operations_.append(new SpecialPtrOperation());
}

bool MemoryAnalyzer::analyze(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cout << "Error: Failed to open file: " << filePath.toStdString() << std::endl;
        has_error_ = true;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        std::cout << "Error: JSON parse error: "
                  << parseError.errorString().toStdString() << std::endl;
        has_error_ = true;
        return false;
    }

    QJsonObject rootObj = doc.object();
    rowProcessor_->setFunctions(rootObj.value("functions").toObject(),
                                 rootObj.value("stacks").toObject());

    QJsonObject codeObj = rootObj.value("code").toObject();
    if (!codeObj.contains("rows") || !codeObj["rows"].isArray()) {
        std::cout << "Error: Missing 'rows' array" << std::endl;
        has_error_ = true;
        return false;
    }

    QJsonArray rows = codeObj["rows"].toArray();
    std::cout << "=== Starting parsing of " << rows.size() << " rows ===" << std::endl;

    Tree* currentTree = nullptr;
    for (const QJsonValue& row : rows) {
        if (!row.isObject()) { has_error_ = true; return false; }
        QJsonObject rowObj = row.toObject();
        if (!rowObj.contains("id")) { has_error_ = true; return false; }

        int clauseId = rowObj["id"].toInt();
        std::cout << "\n=== Processing row " << clauseId << " ===" << std::endl;

        rowProcessor_->processRow(rowObj, currentTree, clauseId);

        if (rowProcessor_->hasMemoryLeak())
            has_memory_leak_ = true;

        analyzeAllTrees(clauseId);

        if (rowProcessor_->hasError()) {
            has_error_ = true;
            std::cout << "Error: Parsing error at row: " << clauseId << std::endl;
            return false;
        }
        std::cout << "=== Completed row " << clauseId << " ===\n" << std::endl;
    }

    std::cout << "=== Final Analysis ===" << std::endl;
    analyzeAllTrees(-1);

    if (has_memory_leak_)
        std::cout << "\nFINAL RESULT: Memory leaks detected in the program" << std::endl;
    else
        std::cout << "\nFINAL RESULT: No memory leaks detected" << std::endl;

    return true;
}

void MemoryAnalyzer::analyzeAllTrees(int rowId)
{
    QMap<QString, Tree*> allTrees = treeManager_->getAllTrees();
    std::cout << "\n=== Solving CNF for ALL trees after row " << rowId << " ===" << std::endl;
    std::cout << "Total trees: " << allTrees.size() << std::endl;

    for (auto it = allTrees.constBegin(); it != allTrees.constEnd(); ++it) {
        Tree* tree = it.value();
        std::cout << "\n--- Analyzing tree: " << tree->getName().toStdString() << " ---" << std::endl;

        bool leak = solver_->solve(tree->getConnections(), tree->getElements());
        if (leak) {
            std::cout << "MEMORY LEAK DETECTED in tree '"
                      << tree->getName().toStdString() << "' at row " << rowId << std::endl;
            has_memory_leak_ = true;
        } else {
            std::cout << "No memory leak in tree '"
                      << tree->getName().toStdString() << "'" << std::endl;
        }
    }
}

void MemoryAnalyzer::printElements() const
{
    QMap<QString, Tree*> allTrees = treeManager_->getAllTrees();
    for (auto it = allTrees.constBegin(); it != allTrees.constEnd(); ++it) {
        Tree* tree = it.value();
        std::cout << "=== Tree: " << tree->getName().toStdString() << " ===" << std::endl;
        for (auto elem_it = tree->getElements().constBegin();
             elem_it != tree->getElements().constEnd(); ++elem_it)
        {
            TreeElement* elem = elem_it.value();
            std::cout << "Element: " << elem_it.key().toStdString()
                      << " Type: " << elem->getType()
                      << " Position: " << elem->getPosition() << std::endl;
        }
        for (const Connection& conn : tree->getConnections()) {
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
