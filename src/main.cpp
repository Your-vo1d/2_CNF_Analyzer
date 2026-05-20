#include <QCoreApplication>
#include <QFileInfo>
#include <iostream>
#include <memory>
#include "IMemoryAnalyzer.h"
#include "MemoryAnalyzer.h"
#include "MinisatSolver.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <json_file_path>" << std::endl;
        return 1;
    }

    QString filePath = QString::fromUtf8(argv[1]);
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        std::cout << "Error: file not found: " << filePath.toStdString() << std::endl;
        std::cout << "Usage: " << argv[0] << " <json_file_path>" << std::endl;
        return 1;
    }

    std::unique_ptr<ISATSolver> solver = std::make_unique<MinisatSolver>();
    std::unique_ptr<IMemoryAnalyzer> analyzer =
        std::make_unique<MemoryAnalyzer>(std::move(solver));

    bool success = analyzer->analyze(filePath);

    if (success) {
        std::cout << "\n=== Analysis completed successfully ===" << std::endl;
        analyzer->printElements();
        if (analyzer->hasMemoryLeak())
            std::cout << "WARNING: Memory leaks detected!" << std::endl;
        else
            std::cout << "No memory leaks detected." << std::endl;
    } else {
        std::cout << "\n=== Analysis failed ===" << std::endl;
        if (analyzer->hasError())
            std::cout << "Errors occurred during analysis." << std::endl;
        return 2;
    }

    return 0;
}
