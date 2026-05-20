#ifndef MEMORYANALYZER_H
#define MEMORYANALYZER_H

#include <memory>
#include <QList>
#include "IMemoryAnalyzer.h"
#include "ISATSolver.h"
#include "TreeManager.h"
#include "GraphBuilder.h"
#include "RowProcessor.h"
#include "IMemoryOperation.h"

class MemoryAnalyzer : public IMemoryAnalyzer
{
public:
    explicit MemoryAnalyzer(std::unique_ptr<ISATSolver> solver);
    ~MemoryAnalyzer() override;

    bool analyze(const QString& filePath) override;
    bool hasError() const override      { return has_error_; }
    bool hasMemoryLeak() const override { return has_memory_leak_; }
    void printElements() const override;

private:
    std::unique_ptr<ISATSolver>   solver_;
    std::unique_ptr<TreeManager>  treeManager_;
    std::unique_ptr<GraphBuilder> graphBuilder_;
    std::unique_ptr<RowProcessor> rowProcessor_;
    QList<IMemoryOperation*>      operations_;
    bool has_error_;
    bool has_memory_leak_;

    void registerDefaultOperations();
    void analyzeAllTrees(int rowId);
};

#endif // MEMORYANALYZER_H
