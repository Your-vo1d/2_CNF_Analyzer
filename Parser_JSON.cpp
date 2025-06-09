#include "Parser_JSON.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSet>
#include <iostream>
#include "minisat/core/Solver.h"

size_t findSetBitPosition(const BoolVector& bv) {
    if (bv.isZero())
        throw std::runtime_error("No set bit found in BoolVector");

    const auto* data = bv.getVector();
    const size_t byteCount = bv.count_bytes();

    for (size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex) {
        unsigned char byte = data[byteIndex];
        if (!byte) continue;

        for (size_t bitOffset = 0; bitOffset < 8; ++bitOffset) {
            size_t bitPosInByte = 7 - bitOffset;
            if (byte & (1 << bitPosInByte)) {
                return byteIndex * 8 + bitOffset;
            }
        }
    }
    throw std::runtime_error("No set bit found in BoolVector");
}

void solveCNF(CNFClause* head, size_t totalVariables) {
    Minisat::Solver solver;
    std::vector<Minisat::Var> vars(totalVariables);

    for (size_t i = 0; i < totalVariables; ++i)
        vars[i] = solver.newVar();

    CNFClause* current = head;
    while (current) {
        Minisat::vec<Minisat::Lit> clause;

        for (size_t i = 0; i < current->positiveVars.size(); ++i)
            if (current->positiveVars.getBit(i))
                clause.push(Minisat::mkLit(vars[i], false));

        for (size_t i = 0; i < current->negativeVars.size(); ++i)
            if (current->negativeVars.getBit(i))
                clause.push(Minisat::mkLit(vars[i], true));

        solver.addClause(clause);
        current = current->next;
    }

    Minisat::vec<Minisat::Lit> allPos, allNeg;
    for (size_t i = 0; i < totalVariables; ++i) {
        allPos.push(Minisat::mkLit(vars[i], false));
        allNeg.push(Minisat::mkLit(vars[i], true));
    }
    solver.addClause(allPos);
    solver.addClause(allNeg);

    if (solver.solve()) {
        std::cout << "Решение найдено:\n";
        for (size_t i = 0; i < totalVariables; ++i) {
            Minisat::lbool val = solver.modelValue(vars[i]);
            std::cout << "x" << i << " = " << (val == Minisat::lbool((uint8_t)0)) << "\n";
        }
    } else {
        std::cout << "Решение не существует\n";
    }
}

void printElements(const QHash<QString, QHash<QString, QVariant>>& elements) {
    qDebug() << "----------------------------------";
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        const auto& props = it.value();
        qDebug() << "Name:" << it.key()
                 << "\n  Type:" << props["type"].toString()
                 << "\n  Position:" << props["position"].toInt()
                 << "\n  Memory:" << props["memory"].toString()
                 << "\n----------------------------------";
    }
}

void setElement(QHash<QString, QHash<QString, QVariant>>& elements,
                const QString& name, const QString& type, int position) {
    elements[name] = {
        {"type", type},
        {"position", position},
        {"memory", "NULL"}
    };
}

void processNestedVariable(QString& current_name, bool& error, const QString& fieldDirection) {
    static QRegularExpression re("^N(\\d+)$");
    QRegularExpressionMatch match = re.match(current_name);

    if (!match.hasMatch()) {
        error = true;
        return;
    }

    int number = match.captured(1).toInt();
    if (fieldDirection == "left")
        current_name = "N" + QString::number(number + 1);
    else if (fieldDirection == "right")
        current_name = "N" + QString::number(number + 2);
}

void processValueField(const QJsonValue& fieldValue,
                       QHash<QString, QHash<QString, QVariant>>& elements,
                       QString& last_name, int nesting, bool& error,
                       CNFClause& current, size_t max_bytes) {
    if (!fieldValue.isObject()) {
        error = true;
        return;
    }

    QJsonObject fieldObj = fieldValue.toObject();
    if (fieldObj.contains("f")) {
        for (int i = 0; i < nesting; ++i) {
            if (!elements.contains(last_name)) {
                error = true;
                return;
            }
            last_name = elements[last_name]["memory"].toString();
        }
        processNestedVariable(last_name, error, fieldObj["f"].toString());
    }

    if (elements.contains(last_name)) {
        size_t position = elements[last_name]["position"].toInt();
        current.setPositiveBit(position, "mem_var", max_bytes);
    } else {
        error = true;
    }
}

void handleMalloc(CNFClause& cnf, CNFClause& temp, int& memoryVarIndex, int& currentPosition,
                  QHash<QString, QHash<QString, QVariant>>& elements,
                  const QString& varName, int clauseId, int& isFirstClause, size_t& max_bytes) {
    elements[varName]["memory"] = "N" + QString::number(memoryVarIndex);
    setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
    if (clauseId == 63) {
        qDebug() << "temp";
    }
    size_t parentId = currentPosition;
    memoryVarIndex++;
    currentPosition++;

    // Добавление двух дочерних переменных
    for (int i = 0; i < 2; ++i) {
        setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
        temp.position = clauseId;
        temp.setNegativeBit(parentId, "mem_var", max_bytes);
        temp.setPositiveBit(currentPosition, "mem_var", max_bytes);

        if (isFirstClause == 0) {
            cnf = temp;
            isFirstClause = 1;
        } else {
            cnf.addClause(temp);
        }

        // Сброс временной клаузы
        temp = CNFClause();
        memoryVarIndex++;
        currentPosition++;
    }
}

void processNestedValue(const QJsonObject& valueObj,
                        QHash<QString, QHash<QString, QVariant>>& elements,
                        CNFClause& currentClause, QString varName, bool& error, size_t& max_bytes) {
    if (!valueObj.contains("variable")) {
        error = true;
        return;
    }

    QString nestedVarName = valueObj["variable"].toString();
    if (!elements.contains(nestedVarName)) {
        error = true;
        return;
    }

    QString memRef = elements[nestedVarName]["memory"].toString();
    if (memRef == "NULL") {
        if (valueObj.contains("field")) {
            error = true;
            return;
        }
        size_t position = elements[nestedVarName]["position"].toInt();
        currentClause.setPositiveBit(position, "variable", max_bytes);
        return;
    }

    if (valueObj.contains("field")) {
        int nesting = 1;
        processValueField(valueObj["field"], elements, nestedVarName,
                          nesting, error, currentClause, max_bytes);
    } else {
        size_t position = elements[memRef]["position"].toInt();
        currentClause.setPositiveBit(position, "variable", max_bytes);
    }
}

void processStructureField(const QJsonValue& fieldValue, bool& error, QString& lastName,
                           int& memoryVarIndex, int& nestingLevel,
                           QHash<QString, QHash<QString, QVariant>>& elements,
                           CNFClause& currentClause, int& currentPosition, size_t& max_bytes,
                           CNFClause& cnf, int& isFirstClause, int clauseId) {
    if (!fieldValue.isObject()) {
        error = true;
        return;
    }

    QJsonObject fieldObj = fieldValue.toObject();
    CNFClause temp;

    // Обработка поля
    if (fieldObj.contains("f")) {
        QJsonValue fValue = fieldObj["f"];
        if (fValue.isString()) {
            for (int i = 0; i < nestingLevel; ++i) {
                if (!elements.contains(lastName)) {
                    error = true;
                    return;
                }
                lastName = elements[lastName]["memory"].toString();
            }
            processNestedVariable(lastName, error, fValue.toString());
        }
    }

    // Обработка значения
    if (fieldObj.contains("value")) {
        if (!elements.contains(lastName)) {
            error = true;
            return;
        }

        size_t varPosition = elements[lastName]["position"].toInt();
        currentClause.setNegativeBit(varPosition, "mem_var", max_bytes);

        QJsonValue value = fieldObj["value"];
        if (value.isNull()) {
            currentClause.setPositiveBit(0, "Variable", max_bytes);
        }
        else if (value.isString() && value.toString() == "malloc") {
            currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);
            handleMalloc(cnf, temp, memoryVarIndex, currentPosition, elements,
                         lastName, clauseId, isFirstClause, max_bytes);
        }
        else if (value.isObject()) {
            processNestedValue(value.toObject(), elements, currentClause, lastName, error, max_bytes);
        }
        return;
    }

    // Рекурсивная обработка операций
    if (fieldObj.contains("op")) {
        nestingLevel = 1;
        processStructureField(fieldObj["op"], error, lastName, memoryVarIndex,
                              nestingLevel, elements, currentClause, currentPosition,
                              max_bytes, cnf, isFirstClause, clauseId);
    }
}

void processOperation(const QJsonObject& opObj, bool& error,
                      QHash<QString, QHash<QString, QVariant>>& elements,
                      int& currentPosition, int& memoryVarIndex,
                      CNFClause& currentClause, size_t& max_bytes,
                      CNFClause& cnf, int& isFirstClause, int clauseId);

void processBody(const QJsonValue& bodyValue, bool& error,
                 QHash<QString, QHash<QString, QVariant>>& elements,
                 int& currentPosition, int& memoryVarIndex,
                 CNFClause& currentClause, size_t& max_bytes,
                 CNFClause& cnf, int& isFirstClause, int clauseId) {
    if (bodyValue.isObject()) {
        QJsonObject bodyObj = bodyValue.toObject();
        if (!bodyObj.contains("id"))
            return;

        int id = bodyObj["id"].toInt();
        currentClause.position = id;
        CNFClause temp;

        if (bodyObj.contains("variable")) {
            QString varName = bodyObj["variable"].toString();

            if (!elements.contains(varName)) {
                setElement(elements, varName, "Variable", currentPosition);
                currentPosition++;
            }

            if (bodyObj.contains("value")) {
                QJsonValue val = bodyObj["value"];
                size_t position = elements[varName]["position"].toInt();

                if (val.isNull()) {
                    currentClause.setNegativeBit(position, "Variable", max_bytes);
                    currentClause.setPositiveBit(0, "Variable", max_bytes);
                }
                else if (val.isString() && val.toString() == "malloc") {
                    currentClause.setNegativeBit(position, "Variable", max_bytes);
                    currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);
                    handleMalloc(cnf, temp, memoryVarIndex, currentPosition,
                                 elements, varName, id, isFirstClause, max_bytes);
                }
                else if (val.isObject()) {
                    currentClause.setNegativeBit(position, "Variable", max_bytes);
                    processNestedValue(val.toObject(), elements, currentClause, varName, error, max_bytes);
                }
            }
            else if (bodyObj.contains("field")) {
                bool isValueFound = false;
                int nesting = 1;
                processStructureField(bodyObj["field"], error, varName, memoryVarIndex,
                                      nesting, elements, currentClause, currentPosition,
                                      max_bytes, cnf, isFirstClause, id);
            }
        }
        else if (bodyObj.contains("operation")) {
            processOperation(bodyObj["operation"].toObject(), error, elements,
                             currentPosition, memoryVarIndex, currentClause,
                             max_bytes, cnf, isFirstClause, id);
        }
    }
    else if (bodyValue.isArray()) {
        QJsonArray bodyArray = bodyValue.toArray();
        for (const QJsonValue& item : bodyArray) {
            processBody(item, error, elements, currentPosition, memoryVarIndex,
                        currentClause, max_bytes, cnf, isFirstClause, clauseId);
        }
    }
}

void processBranch(const QJsonObject& branchObj, bool& error,
                   QHash<QString, QHash<QString, QVariant>>& elements,
                   int& currentPosition, int& memoryVarIndex,
                   CNFClause& currentClause, size_t& max_bytes,
                   CNFClause& cnf, int& isFirstClause, int clauseId) {
    if (branchObj.contains("body")) {
        processBody(branchObj["body"], error, elements, currentPosition,
                    memoryVarIndex, currentClause, max_bytes, cnf, isFirstClause, clauseId);
    }
}

void processOperation(const QJsonObject& opObj, bool& error,
                      QHash<QString, QHash<QString, QVariant>>& elements,
                      int& currentPosition, int& memoryVarIndex,
                      CNFClause& currentClause, size_t& max_bytes,
                      CNFClause& cnf, int& isFirstClause, int clauseId) {
    if (!opObj.contains("op")) {
        error = true;
        return;
    }

    if (opObj.contains("branch true")) {
        QJsonValue branchTrue = opObj["branch true"];
        if (branchTrue.isObject()) {
            processBranch(branchTrue.toObject(), error, elements, currentPosition,
                          memoryVarIndex, currentClause, max_bytes, cnf, isFirstClause, clauseId);
        }
    }

    if (opObj.contains("branch false")) {
        QJsonValue branchFalse = opObj["branch false"];
        if (branchFalse.isObject()) {
            processBranch(branchFalse.toObject(), error, elements, currentPosition,
                          memoryVarIndex, currentClause, max_bytes, cnf, isFirstClause, clauseId);
        }
    }
}

void parseJsonFile(const QString& filePath,
                   QHash<QString, QHash<QString, QVariant>>& elements,
                   bool& error, CNFClause& cnf) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << filePath;
        error = true;
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (doc.isNull()) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        error = true;
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "Invalid JSON structure";
        error = true;
        return;
    }

    QJsonObject rootObject = doc.object();
    if (!rootObject.contains("code") || !rootObject["code"].isObject()) {
        qWarning() << "Missing 'code' object";
        error = true;
        return;
    }

    QJsonObject codeObject = rootObject["code"].toObject();
    if (!codeObject.contains("rows") || !codeObject["rows"].isArray()) {
        qWarning() << "Missing 'rows' array";
        error = true;
        return;
    }

    size_t max_bytes = 1;
    size_t current_bytes = 1;
    int currentPosition = 1;
    int memoryVarIndex = 0;
    int isFirstClause = 0;
    QJsonArray rows = codeObject["rows"].toArray();
    CNFClause temp;

    for (const QJsonValue& row : rows) {
        if (!row.isObject()) continue;

        QJsonObject rowObj = row.toObject();
        if (!rowObj.contains("id")) continue;

        int clauseId = rowObj["id"].toInt();
        CNFClause currentClause;
        currentClause.position = clauseId;
        if (clauseId == 63){
            qDebug() << "Temp";
        }
        if (rowObj.contains("variable")) {
            QString varName = rowObj["variable"].toString();

            if (!elements.contains(varName)) {
                setElement(elements, varName, "Variable", currentPosition);
                currentPosition++;
            }

            if (rowObj.contains("value")) {
                QJsonValue val = rowObj["value"];
                size_t varPos = elements[varName]["position"].toInt();

                if (val.isNull()) {
                    currentClause.setNegativeBit(varPos, "Variable", max_bytes);
                    currentClause.setPositiveBit(0, "Variable", max_bytes);
                }
                else if (val.isString() && val.toString() == "malloc") {
                    currentClause.setNegativeBit(varPos, "Variable", max_bytes);
                    currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);
                    handleMalloc(cnf, temp, memoryVarIndex, currentPosition,
                                 elements, varName, clauseId, isFirstClause, max_bytes);
                }
                else if (val.isObject()) {
                    currentClause.setNegativeBit(varPos, "Variable", max_bytes);
                    processNestedValue(val.toObject(), elements, currentClause, varName, error, max_bytes);
                }
            }
            else if (rowObj.contains("field")) {
                bool isValueFound = false;
                int nestingLevel = 1;
                processStructureField(rowObj["field"], error, varName, memoryVarIndex,
                                      nestingLevel, elements, currentClause, currentPosition,
                                      max_bytes, cnf, isFirstClause, clauseId);
            }
        }
        else if (rowObj.contains("operation")) {
            processOperation(rowObj["operation"].toObject(), error, elements,
                             currentPosition, memoryVarIndex, currentClause,
                             max_bytes, cnf, isFirstClause, clauseId);
        }
        if (isFirstClause == 0) {
            cnf = currentClause;
            isFirstClause = 1;
        } else {
            cnf.addClause(currentClause);
        }

        if (max_bytes > current_bytes) {
            cnf.resizeAll(max_bytes);
        }
    }
}
