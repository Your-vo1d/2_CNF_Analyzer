#include <QCoreApplication>
#include <QCommandLineParser>
#include <QHash>
#include "Parser_JSON.h"
#include "CNFClause.h"
#include <QVariant>
#include <iostream>
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Memory-Aware JSON Parser");
    parser.addHelpOption();
    parser.addPositionalArgument("file", "Input JSON file");
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        qCritical() << "Error: No input file specified";
        return 1;
    }
    QHash<QString, QHash<QString, QVariant>> elements;
    bool error = false;
    bool memory_leak = false;
    CNFClause cnf;

    parseJsonFile(args.first(), elements, error, cnf, memory_leak);
    // Решаем КНФ
    if (error) {
        qCritical() << "Error:";
        return 1;
    }
    if (!memory_leak) {
        std::cout << "Ошибок с памятью не найдено" << std::endl;
    }
    return 0;
}
