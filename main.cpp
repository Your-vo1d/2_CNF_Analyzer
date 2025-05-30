#include <QCoreApplication>
#include <QCommandLineParser>
#include <QHash>
#include "Parser_JSON.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("JSON Field Parser");
    parser.addHelpOption();
    parser.addPositionalArgument("file", "Input JSON file");
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        qCritical() << "Error: No input file specified";
        return 1;
    }

    QHash<int, QString> elements;
    QString errorMessage;

    parseJsonFile(args.first(), elements, errorMessage);

    if (!errorMessage.isEmpty()) {
        qCritical() << "Error:" << errorMessage;
        return 1;
    }

    qDebug() << "\nFinal elements list (sorted by position):";
    QList<int> positions = elements.keys();
    std::sort(positions.begin(), positions.end());

    for (int pos : positions) {
        qDebug() << pos << ":" << elements[pos];
    }

    return 0;
}
