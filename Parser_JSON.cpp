#include "Parser_JSON.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSet>
#include <iostream>
#include <minisat/core/Solver.h>

void removeRootLinkClauses(CNFClause& head, size_t rootPos, size_t varPos) {
    CNFClause* prev = nullptr;
    CNFClause* current = &head;

    while (current) {
        bool condition = false;
        bool validRoot = (rootPos < current->positiveVars.size());
        bool validVar = (varPos < current->negativeVars.size());

        if (validRoot && validVar) {
            condition = current->positiveVars.getBit(rootPos) &&
                        current->negativeVars.getBit(varPos);
        }

        if (condition) {
            if (prev) {
                // Удаление из середины/конца списка
                prev->next = current->next;
                current->next = nullptr;  // Важно: обнуляем next перед удалением
                delete current;
                current = prev->next;
            } else {
                // Удаление первого элемента
                if (current->next) {
                    // Копируем данные из следующего узла
                    CNFClause* nextNode = current->next;
                    current->position = nextNode->position;
                    current->positiveVars = nextNode->positiveVars;
                    current->negativeVars = nextNode->negativeVars;

                    // Обновляем связи
                    current->next = nextNode->next;

                    // Изолируем и удаляем следующий узел
                    nextNode->next = nullptr;
                    delete nextNode;
                } else {
                    // Сброс единственного элемента
                    current->position = -1;
                    current->positiveVars = BoolVector();
                    current->negativeVars = BoolVector();
                }
            }
            break;  // Удаляем только одну клаузу
        } else {
            prev = current;
            current = current->next;
        }
    }
}
// Находит позицию первого установленного бита в битовом векторе
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

void printCNF(CNFClause* head, const QHash<QString, QHash<QString, QVariant>>& elements) {
    // Создаем обратное отображение позиций в имена переменных
    QHash<int, QString> varNames;
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        int pos = it.value()["position"].toInt();
        varNames[pos] = it.key();
    }

    std::cout << "КНФ в формате DIMACS:\n";
    std::cout << "c Сгенерированная КНФ-формула\n";

    // Подсчет количества переменных и клауз
    int varCount = elements.size();
    int clauseCount = 0;
    CNFClause* current = head;
    while (current) {
        clauseCount++;
        current = current->next;
    }

    std::cout << "p cnf " << varCount << " " << clauseCount << "\n";

    // Вывод всех клауз
    current = head;
    while (current) {
        // Положительные литералы
        for (size_t i = 0; i < current->positiveVars.size(); ++i) {
            if (current->positiveVars.getBit(i)) {
                std::cout << (i+1) << " "; // DIMACS использует 1-based индексацию
            }
        }

        // Отрицательные литералы
        for (size_t i = 0; i < current->negativeVars.size(); ++i) {
            if (current->negativeVars.getBit(i)) {
                std::cout << "-" << (i+1) << " "; // Отрицание в DIMACS
            }
        }

        std::cout << "0\n"; // Конец клаузы
        current = current->next;
    }

    // Дополнительный вывод с именами переменных
    std::cout << "\nСоответствие переменных:\n";
    for (int i = 1; i < varCount + 1; ++i) {
        if (varNames.contains(i)) {
            std::cout << (i+1) << " -> " << varNames[i].toStdString() << "\n";
        } else {
            std::cout << (i+1) << " -> N" << (i - elements.size()) << "\n";
        }
    }
}

void solveCNF(CNFClause* head, const QHash<QString, QHash<QString, QVariant>>& elements) {
    Minisat::Solver solver;
    printCNF(head, elements);
    // Находим максимальную позицию среди элементов
    size_t maxPos = 0;
    for (const auto& elem : elements) {
        size_t pos = elem["position"].toUInt();
        if (pos > maxPos) maxPos = pos;
    }
    size_t totalVariables = maxPos + 1; // +1 потому что позиции начинаются с 0

    qDebug() << "Total variables:" << totalVariables;

    // Создаем переменные в решателе
    std::vector<Minisat::Var> vars(totalVariables);
    for (size_t i = 0; i < totalVariables; ++i) {
        vars[i] = solver.newVar();
    }

    CNFClause* current = head;
    while (current) {
        Minisat::vec<Minisat::Lit> clause;

        // Обрабатываем положительные литералы (биты с 1)
        for (size_t i = 1; i <= current->positiveVars.size(); ++i) {
            if (current->positiveVars.getBit(i-1)) { // getBit использует 0-based индекс
                if (i-1 < totalVariables) { // Проверяем границы
                    clause.push(Minisat::mkLit(vars[i-1], false));
                }
            }
        }

        // Обрабатываем отрицательные литералы (биты с 1)
        for (size_t i = 1; i <= current->negativeVars.size(); ++i) {
            if (current->negativeVars.getBit(i-1)) {
                if (i-1 < totalVariables) {
                    clause.push(Minisat::mkLit(vars[i-1], true));
                }
            }
        }

        if (clause.size() > 0) {
            solver.addClause(clause);
        }
        current = current->next;
    }

    // Добавляем тестовые клаузы (без позиции 0)
    Minisat::vec<Minisat::Lit> allPos, allNeg;
    for (size_t i = 1; i < totalVariables; ++i) {
        allPos.push(Minisat::mkLit(vars[i], false));
        allNeg.push(Minisat::mkLit(vars[i], true));
    }

    if (allPos.size() > 0) solver.addClause(allPos);
    if (allNeg.size() > 0) solver.addClause(allNeg);

    if (solver.solve()) {
        std::cout << "Решение найдено:\n";
        for (size_t i = 0; i < totalVariables; ++i) {
            // Пропускаем вывод, если переменная не используется
            bool used = false;
            CNFClause* tmp = head;
            while (tmp) {
                if ((i < tmp->positiveVars.size() && tmp->positiveVars.getBit(i)) ||
                    (i < tmp->negativeVars.size() && tmp->negativeVars.getBit(i))) {
                    used = true;
                    break;
                }
                tmp = tmp->next;
            }

            if (used) {
                Minisat::lbool val = solver.modelValue(vars[i]);
                std::cout << "x" << i << " = " << (val == Minisat::lbool((uint8_t)0)) << "\n";
            }
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
                  const QString& varName, int clauseId, int& isFirstClause, size_t& max_bytes, QString& rootName) {
    elements[varName]["memory"] = "N" + QString::number(memoryVarIndex);
    setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
    size_t negative_bit_position = elements["N" + QString::number(memoryVarIndex)]["position"].toInt();
    size_t parentId = elements[varName]["position"].toInt();

    // Удаление старых клауз связывающих с корнем
    size_t parentVarPos = elements[rootName]["position"].toInt();
    CNFClause* prev = nullptr;
    CNFClause* current = &cnf;
    std::cout << parentVarPos << " " << parentId <<std::endl;
    if (rootName != varName) {
        std::cout <<"ff" <<std::endl;
                removeRootLinkClauses(cnf, parentVarPos, parentId);
    }
    memoryVarIndex++;
    currentPosition++;
    CNFClause loop_cnf;
    size_t rootID = elements[rootName]["position"].toInt();
    // Добавление двух дочерних переменных
    for (int i = 0; i < 2; ++i) {
        setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
        temp.position = clauseId;
        temp.setNegativeBit(negative_bit_position, "mem_var", max_bytes);
        temp.setPositiveBit(currentPosition, "mem_var", max_bytes);
        loop_cnf.setNegativeBit(currentPosition, "mem_var", max_bytes);
        loop_cnf.setPositiveBit(rootID, "variable", max_bytes);
        if (isFirstClause == 0) {
            cnf = temp;
            isFirstClause = 1;
        } else {
            cnf.addClause(temp);
        }

        cnf.addClause(loop_cnf);
        loop_cnf = CNFClause();
        // Сброс временной клаузы
        temp = CNFClause();
        memoryVarIndex++;
        currentPosition++;
    }
}

//void delete_con_clauses()

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
                           CNFClause& cnf, int& isFirstClause, int clauseId, QString& rootName) {
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
            currentClause = CNFClause();
            currentClause.position = -1;
            return;
            currentClause.setPositiveBit(0, "Variable", max_bytes);
        }
        else if (value.isString() && value.toString() == "malloc") {
            currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);
            handleMalloc(cnf, temp, memoryVarIndex, currentPosition, elements,
                         lastName, clauseId, isFirstClause, max_bytes, rootName);
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
                              max_bytes, cnf, isFirstClause, clauseId, rootName);
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
            QString rootName = varName;
            if (!elements.contains(varName)) {
                setElement(elements, varName, "Variable", currentPosition);
                currentPosition++;
            }

            if (bodyObj.contains("value")) {
                QJsonValue val = bodyObj["value"];
                size_t position = elements[varName]["position"].toInt();

                if (val.isNull()) {
                    currentClause = CNFClause();
                    currentClause.position = -1;
                    return;
                }
                else if (val.isString() && val.toString() == "malloc") {
                    currentClause.setNegativeBit(position, "Variable", max_bytes);
                    currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);
                    handleMalloc(cnf, temp, memoryVarIndex, currentPosition,
                                 elements, varName, id, isFirstClause, max_bytes, rootName);
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
                                      max_bytes, cnf, isFirstClause, id, rootName);
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
    int clauseId;
    QJsonArray rows = codeObject["rows"].toArray();
    CNFClause temp;

    for (const QJsonValue& row : rows) {
        if (!row.isObject()) continue;

        QJsonObject rowObj = row.toObject();
        if (!rowObj.contains("id")) continue;

        clauseId = rowObj["id"].toInt();
        CNFClause currentClause;
        currentClause.position = clauseId;
        if (clauseId == 63){
            qDebug() << "Temp";
        }
        if (rowObj.contains("variable")) {
            QString varName = rowObj["variable"].toString();
            QString rootName = varName;
            if (!elements.contains(varName)) {
                setElement(elements, varName, "Variable", currentPosition);
                currentPosition++;
            }

            if (rowObj.contains("value")) {
                QJsonValue val = rowObj["value"];
                size_t varPos = elements[varName]["position"].toInt();

                if (val.isNull()) {
                    currentClause = CNFClause();
                    currentClause.position = -1;
                    continue;
                }
                else if (val.isString() && val.toString() == "malloc") {
                    currentClause.setNegativeBit(varPos, "Variable", max_bytes);
                    currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);
                    handleMalloc(cnf, temp, memoryVarIndex, currentPosition,
                                 elements, varName, clauseId, isFirstClause, max_bytes, rootName);
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
                                      max_bytes, cnf, isFirstClause, clauseId, rootName);
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
        }
        else if (currentClause.position == -1) {
            continue;
        }
        else {
            cnf.addClause(currentClause);
        }

        if (max_bytes > current_bytes) {
            cnf.resizeAll(max_bytes);
        }
        printElements(elements);
        cnf.print();
        solveCNF(&cnf,elements);
    }
}
