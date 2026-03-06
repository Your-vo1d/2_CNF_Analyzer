#include <QCoreApplication>
#include <QFileInfo>
#include <iostream>
#include "Parser_JSON.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <json_file_path>" << std::endl;
        return 1;
    }

    // argv в Linux обычно
    QString file_path = QString::fromUtf8(argv[1]);

    // Доп.диагностика, чтобы сразу видеть проблему с путём
    QFileInfo fi(file_path);
    if (!fi.exists() || !fi.isFile()) {
        std::cout << "Error: file not found: " << file_path.toStdString() << std::endl;
        std::cout << "Usage: " << argv[0] << " <json_file_path>" << std::endl;
        return 1;
    }

    Parser_JSON parser;
    bool success = parser.parseJsonFile(file_path);

    if (success) {
        std::cout << "\n=== Parsing completed successfully ===" << std::endl;
        parser.printElements();

        if (parser.hasMemoryLeak()) {
            std::cout << "WARNING: Memory leaks detected!" << std::endl;
        } else {
            std::cout << "No memory leaks detected." << std::endl;
        }
    } else {
        std::cout << "\n=== Parsing failed ===" << std::endl;
        if (parser.hasError()) {
            std::cout << "Errors occurred during parsing." << std::endl;
        }
        return 2;
    }

    return 0;
}
