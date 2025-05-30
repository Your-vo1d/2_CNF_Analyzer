#ifndef PARSER_JSON_H
#define PARSER_JSON_H

#include <QString>
#include <QHash>

void parseJsonFile(const QString& filePath,
                   QHash<int, QString>& elements,
                   QString& errorMessage);

#endif // PARSER_JSON_H
