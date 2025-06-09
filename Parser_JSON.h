#ifndef PARSER_JSON_H
#define PARSER_JSON_H

#include <QString>
#include <QHash>
#include "CNFClause.h"
#include <QVariant>

void parseJsonFile(const QString& filePath,
                   QHash<QString, QHash<QString, QVariant>>& elements, bool& error, CNFClause& cnf);

void printElements(QHash<QString, QHash<QString, QVariant>>& elements);
void processBody(const QJsonValue& bodyValue, bool& error,
                 QHash<QString, QHash<QString, QVariant>>& elements,
                 int& currentPosition, int& index_memory_var,
                 CNFClause& current, CNFClause& cnf, int& isFirstClause, CNFClause& temp) ;

void processOperation(const QJsonObject& opObj, bool& error,
                      QHash<QString, QHash<QString, QVariant>>& elements,
                      int& currentPosition, int& index_memory_var,
                      CNFClause& current, size_t& max_bytes, CNFClause& cnf, int& isFirstClause, CNFClause& temp);

void solveCNF(CNFClause* head, size_t totalVariables);

#endif // PARSER_JSON_H
