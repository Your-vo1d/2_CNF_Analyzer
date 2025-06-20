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
    CNFClause cnf;

    parseJsonFile(args.first(), elements, error, cnf);
    qDebug() << "\nFinal elements:";
    printElements(elements);
    cnf.print();
    std::cout << "-----------------------" <<std::endl;
    BoolVector pos1(16);
    BoolVector neg1(16);
    pos1.setBit(0);  // 10000000 00000000
    neg1.setBit(1);  // 01000000 00000000

    // Создаем клаузу 2: (x3 ∨ ¬x1)
    BoolVector pos2(16);
    BoolVector neg2(16);
    pos2.setBit(1);  // 00010000 00000000
    neg2.setBit(0);  // 01000000 00000000

    // Создаем связанный список клауз
    CNFClause* clause1 = new CNFClause();
    clause1->positiveVars = pos1;
    clause1->negativeVars = neg1;
    clause1->print();
    CNFClause* clause2 = new CNFClause();
    clause2->positiveVars = pos2;
    clause2->negativeVars = neg2;
    clause2->print();
    clause1->next = clause2;
    clause2->next = nullptr;

    // Решаем КНФ
    solveCNF(&cnf,elements);
    if (error) {
        qCritical() << "Error:";
        return 1;
    }

    return 0;
}
