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
    if (bv.isZero()) {
        throw std::runtime_error("No set bit found in BoolVector");
    }

    for (size_t i = 0; i < bv.size(); i++) {
        size_t byteIndex = i / 8;
        size_t bitOffset = i % 8;
        size_t bitPosInByte = 7 - bitOffset;  // Старший бит первый

        if (byteIndex >= bv.count_bytes()) break;

        unsigned char byte = bv.getVector()[byteIndex];
        unsigned char mask = 1 << bitPosInByte;

        if (byte & mask) {
            return i;
        }
    }
    throw std::runtime_error("No set bit found in BoolVector");
}

// Функция для решения CNF, представленной как связанный список CNFClause
void solveCNF(CNFClause* head, size_t totalVariables) {
    Minisat::Solver solver;

    // 1. Создаем переменные
    for (size_t i = 0; i < totalVariables; i++) {
        solver.newVar();
    }

    // 2. Добавляем пользовательские клаузы
    CNFClause* current = head;
    while (current) {
        Minisat::vec<Minisat::Lit> clause;

        // Положительные литералы
        for (size_t i = 0; i < current->positiveVars.size(); i++) {
            if (current->positiveVars.getBit(i)) {  // Теперь getBit() доступен
                clause.push(Minisat::mkLit(i, false));
            }
        }

        // Отрицательные литералы
        for (size_t i = 0; i < current->negativeVars.size(); i++) {
            if (current->negativeVars.getBit(i)) {  // Теперь getBit() доступен
                clause.push(Minisat::mkLit(i, true));
            }
        }

        solver.addClause(clause);
        current = current->next;
    }

    // 3. Добавляем обязательные клаузы
    Minisat::vec<Minisat::Lit> all_pos, all_neg;
    for (size_t i = 0; i < totalVariables; i++) {
        all_pos.push(Minisat::mkLit(i, false)); // Все положительные
        all_neg.push(Minisat::mkLit(i, true));  // Все отрицательные
    }
    solver.addClause(all_pos);
    solver.addClause(all_neg);

    // 4. Решаем и выводим
    if (solver.solve()) {
        std::cout << "Решение найдено:\n";
        for (size_t i = 0; i < totalVariables; i++) {
            std::cout << "x" << i << " = "
<< (solver.modelValue(i) == Minisat::lbool((uint8_t)0)) << "\n";
        }
    } else {
        std::cout << "Решение не существует\n";
    }
}

void printElements(QHash<QString, QHash<QString, QVariant>>& elements) {
    qDebug() << "----------------------------------";

    for (auto it = elements.constBegin(); it != elements.constEnd(); it++) {
        qDebug() << "Name:" << it.key();
        qDebug() << "  Type:" << it.value()["type"].toString();
        qDebug() << "  Position:" << it.value()["position"].toInt();
        qDebug() << "  Memory:" << it.value()["memory"].toString();
        qDebug() << "----------------------------------";
    }
}

// Добавление/обновление элемента
void setElement(QHash<QString, QHash<QString, QVariant>>& elements, const QString& name, const QString& type,
                int position) {
    QHash<QString, QVariant> props;
    props["type"] = type;
    props["position"] = position;
    props["memory"] = "NULL";

    elements[name] = props;
}

// Вспомогательная функция для обработки N-переменных
void processNestedVariable(QHash<QString, QHash<QString, QVariant>>& elements, QString& current_name, bool& error, QString& last_field_direction ) {
    if (current_name.startsWith('N')) {
        QRegularExpression re("^(N)(\\d+)$");
        QRegularExpressionMatch match = re.match(current_name);

        if (match.hasMatch()) {
            QString prefix = match.captured(1);
            int number = match.captured(2).toInt();

            // Модифицируем имя в зависимости от поля
            if (last_field_direction == "left") {
                current_name = prefix + QString::number(number + 1);
            }
            else if (last_field_direction == "right") {
                current_name = prefix + QString::number(number + 2);
            }
        } else {
            error = true;
        }
    } else {
        error = true;
    }
}

void processValue_field(const QJsonValue& fieldValue, QHash<QString, QHash<QString, QVariant>>& elements, QString& last_name,
                        int& nesting, bool& error, CNFClause& current, size_t& max_bytes) {
    if (!fieldValue.isObject()) {
        error = true;
        qDebug() << "Invalid field structure";
        return;
    }
    QJsonObject fieldObj = fieldValue.toObject();
    if (fieldObj.contains("f")) {
            QJsonValue fValue = fieldObj["f"];
        QString fieldName = fValue.toString();
        // Обработка имени поля
        for (size_t i = 0; i < nesting; i++) {
            auto it = elements.find(last_name);
            if (it == elements.end()) {
                error = true;
                return;
            }
            last_name = elements[last_name]["memory"].toString();
        }
        processNestedVariable(elements, last_name, error, fieldName);
        size_t position = elements[last_name]["position"].toInt();
        current.setPositiveBit(position, "mem_var", max_bytes);
    }
    else {
        size_t position = elements[last_name]["position"].toInt();
        current.setPositiveBit(position, "mem_var", max_bytes);
    }
    return;
}



void processStructureField(const QJsonValue& fieldValue, bool& error, QString& lastName,
                           int& memoryVarIndex, int& nestingLevel,
                           QHash<QString, QHash<QString, QVariant>>& elements,
                           bool& isValueFound, CNFClause& currentClause, int& currentPosition, size_t& max_bytes, CNFClause& cnf, int& isFirstClause, CNFClause& temp)
{
    // Проверка корректности типа входных данных
    if (!fieldValue.isObject()) {
        error = true;
        qDebug() << "Error: Field structure must be a JSON object";
        return;
    }

    QJsonObject fieldObject = fieldValue.toObject();
    QJsonObject nestedFieldObject;

    // Обработка поля структуры ("f")
    if (fieldObject.contains("f")) {
        QJsonValue fieldValue = fieldObject["f"];

         if (fieldValue.isString()) {
            // Если поле - строка, обрабатываем как имя поля
            QString fieldName = fieldValue.toString();

            // Проходим по цепочке вложенности
            for (size_t i = 0; i < nestingLevel; i++) {
                auto elementIt = elements.find(lastName);
                if (elementIt == elements.end()) {
                    error = true;
                    qDebug() << "Error: Element" << lastName << "not found";
                    return;
                }
                lastName = elements[lastName]["memory"].toString();
            }

            // Обрабатываем найденное поле
            processNestedVariable(elements, lastName, error, fieldName);
        }
    }

    // Обработка значения поля ("value")
    if (fieldObject.contains("value")) {
        size_t varPosition = elements[lastName]["position"].toInt();
        currentClause.setNegativeBit(varPosition, "mem_var", max_bytes);
        isValueFound = true;

        QJsonValue value = fieldObject["value"];
        bool isNullValue = value.isNull();
        bool isMalloc = (!isNullValue && value.isString() && value.toString() == "malloc");

        if (isNullValue) {
            // Обработка NULL-значения
            currentClause.setPositiveBit(0, "Variable", max_bytes);
        }
        else if (isMalloc) {
            // Обработка выделения памяти (malloc)
            currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);
            CNFClause temp;
            // Создаем временные переменные для работы с памятью
            elements[lastName]["memory"] = "N" + QString::number(memoryVarIndex);
            setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
            memoryVarIndex++;
            currentPosition++;

            setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
            size_t parentId = currentPosition - 1;
            // Добавляем еще две переменные памяти (возможно, для адреса и размера)
            setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
            temp.setNegativeBit(parentId, "mem_var", max_bytes);
            temp.setPositiveBit(currentPosition, "mem_var", max_bytes);
            if (isFirstClause == 0) {
                // Если это первая клауза - просто сохраняем ее как основу CNF
                cnf = temp;
                isFirstClause += 1;
            }
            else {
                // Для последующих клауз - добавляем их к существующей CNF
                cnf.addClause(temp);
            }
            temp.clearNegativeBit(parentId, "mem_var", max_bytes);
            temp.clearPositiveBit(currentPosition, "mem_var", max_bytes);
            memoryVarIndex++;
            currentPosition++;

            setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
            // Добавляем еще две переменные памяти (возможно, для адреса и размера)
            setElement(elements, "N" + QString::number(memoryVarIndex), "mem_var", currentPosition);
            temp.setNegativeBit(parentId, "mem_var", max_bytes);
            temp.setPositiveBit(currentPosition, "mem_var", max_bytes);
            if (isFirstClause == 0) {
                // Если это первая клауза - просто сохраняем ее как основу CNF
                cnf = temp;
                isFirstClause += 1;
            }
            else {
                // Для последующих клауз - добавляем их к существующей CNF
                cnf.addClause(temp);
            }
            temp.clearNegativeBit(parentId, "mem_var", max_bytes);
            temp.clearPositiveBit(currentPosition, "mem_var", max_bytes);
            memoryVarIndex++;
            currentPosition++;
        }
        else if (value.isObject()) {
            // Обработка вложенного объекта в значении
            QJsonObject valueObject = value.toObject();

            if (valueObject.contains("variable")) {
                QString nestedVarName = valueObject["variable"].toString();
                qDebug() << "Processing nested variable:" << nestedVarName;

                auto elementIt = elements.find(nestedVarName);
                if (elementIt == elements.end()) {
                    error = true;
                    return;
                }

                QString originalVarName = nestedVarName;
                if (nestedVarName == "current" || originalVarName == "current") {
                    qDebug() << "Found 'current' variable";
                }

                nestedVarName = elements[originalVarName]["memory"].toString();
                qDebug() << "Memory reference:" << nestedVarName;

                if (nestedVarName == "NULL") {
                    if (valueObject.contains("field")) {
                        error = true;
                        return;
                    }
                    varPosition = elements[nestedVarName]["position"].toInt();
                    currentClause.setPositiveBit(varPosition, "variable", max_bytes);
                    return;
                }

                if (valueObject.contains("field")) {
                    processValue_field(valueObject["field"], elements, originalVarName,
                                       nestingLevel, error, currentClause, max_bytes);
                                    qDebug() << "fds";
                }
                else {
                    varPosition = elements[nestedVarName]["position"].toInt();
                    currentClause.setPositiveBit(varPosition, "variable", max_bytes);
                }
                qDebug() << "fds";
            }
        }
        return;
    }

    // Рекурсивная обработка операций ("op")
    if (fieldObject.contains("op")) {
        nestingLevel = 1; // Сброс уровня вложенности для операций
        processStructureField(fieldObject["op"], error, lastName, memoryVarIndex,
                              nestingLevel, elements, isValueFound, currentClause, currentPosition, max_bytes, cnf, isFirstClause, temp);
    }
}

void processBody(const QJsonValue& bodyValue, bool& error,
                 QHash<QString, QHash<QString, QVariant>>& elements,
                 int& currentPosition, int& index_memory_var,
                 CNFClause& current, size_t& max_bytes, CNFClause& cnf, int& isFirstClause, CNFClause& temp) {

    if (bodyValue.isObject()) {
        QJsonObject bodyObj = bodyValue.toObject();
        // Обработка как обычного элемента
        if (bodyObj.contains("id")) {
            int id = bodyObj["id"].toInt();
            current.position = id;

            if (bodyObj.contains("variable")) {
                QString varName = bodyObj["variable"].toString();
                auto it = elements.find(varName);

                if (it == elements.end()) {
                    setElement(elements, varName, "Variable", currentPosition);
                    currentPosition += 1;
                }

                // Обработка value/field/operation
                if (bodyObj.contains("value")) {
                    QJsonValue val = bodyObj["value"];
                    auto isNullValue = val.isNull();
                    auto isMalloc = (!isNullValue && val.isString() && val.toString() == "malloc");
                    size_t position = elements[varName]["position"].toInt(); // Позиция переменной
                    if (isNullValue) {
                        current.setNegativeBit(position, "Variable", max_bytes);
                        current.setPositiveBit(0, "Variable", max_bytes);
                        // ...
                    }
                    else if (isMalloc) {
                        current.setNegativeBit(position, "Variable", max_bytes);
                        current.setPositiveBit(currentPosition, "mem_var", max_bytes);

                        elements[varName]["memory"] = "N" + QString::number(index_memory_var);
                        setElement(elements, "N" + QString::number(index_memory_var), "mem_var", currentPosition);
                        index_memory_var += 1;
                        currentPosition += 1;
                        setElement(elements, "N" + QString::number(index_memory_var), "mem_var", currentPosition);
                        size_t parentId = currentPosition - 1;
                        temp.setNegativeBit(parentId, "mem_var", max_bytes);
                        temp.setPositiveBit(currentPosition, "mem_var", max_bytes);
                        if (isFirstClause == 0) {
                            // Если это первая клауза - просто сохраняем ее как основу CNF
                            cnf = temp;
                            isFirstClause += 1;
                        }
                        else {
                            // Для последующих клауз - добавляем их к существующей CNF
                            cnf.addClause(temp);
                        }
                        temp.clearNegativeBit(parentId, "mem_var", max_bytes);
                        temp.clearPositiveBit(currentPosition, "mem_var", max_bytes);


                        index_memory_var += 1;
                        currentPosition += 1;
                        setElement(elements, "N" + QString::number(index_memory_var), "mem_var", currentPosition);

                        temp.setNegativeBit(parentId, "mem_var", max_bytes);
                        temp.setPositiveBit(currentPosition, "mem_var", max_bytes);
                        if (isFirstClause == 0) {
                            // Если это первая клауза - просто сохраняем ее как основу CNF
                            cnf = temp;
                            isFirstClause += 1;
                        }
                        else {
                            // Для последующих клауз - добавляем их к существующей CNF
                            cnf.addClause(temp);
                        }
                        temp.clearNegativeBit(parentId, "mem_var", max_bytes);
                        temp.clearPositiveBit(currentPosition, "mem_var", max_bytes);


                        index_memory_var += 1;
                        currentPosition += 1;
                        // ...
                    }


                    else if (val.isObject()) {
                        // Обработка вложенного объекта в value
                        int nesting = 1;
                        QJsonObject valueObj = val.toObject();
                        if (valueObj.contains("variable")) {
                            QString nestedVarName = valueObj["variable"].toString();
                            qDebug() << "Found nested variable:" << nestedVarName;
                            auto it = elements.find(nestedVarName);
                            if (it == elements.end()) {
                                error = true;
                                return;
                            }
                            QString nestedVarNameBefore = nestedVarName;
                            nestedVarName = elements[nestedVarName]["memory"].toString();
                            qDebug() << nestedVarName;
                            if (nestedVarName == "NULL") {
                                if (valueObj.contains("field")) {
                                    error = true;
                                    return;
                                }
                                position = elements[nestedVarName]["position"].toInt(); // Позиция переменной
                                current.setPositiveBit(position, "variable", max_bytes);
                                return;
                            }
                            if (valueObj.contains("field")) {
                                processValue_field(valueObj["field"], elements, nestedVarNameBefore,
                                                   nesting, error, current, max_bytes);
                            }
                            else {
                                position = elements[nestedVarName]["position"].toInt(); // Позиция переменной
                                current.setPositiveBit(position, "variable", max_bytes);
                            }
                        }
                    }
                }
                else if (bodyObj.contains("field")) {
                    bool isFind = false;
                    int nesting = 1;
                     processStructureField(bodyObj["field"], error, varName,
                                 index_memory_var, nesting, elements,
                                 isFind, current, currentPosition, max_bytes, cnf, isFirstClause, temp);
                }
            }
            else if (bodyObj.contains("operation")) {
                processOperation(bodyObj["operation"].toObject(), error,
                                 elements, currentPosition,
                                 index_memory_var, current, max_bytes, cnf, isFirstClause, temp);
            }

    }
    else if (bodyValue.isArray()) {
        // Обработка массива операций в body
        QJsonArray bodyArray = bodyValue.toArray();
        for (const QJsonValue& item : bodyArray) {
            processBody(item, error, elements, currentPosition,
                        index_memory_var, current, max_bytes, cnf, isFirstClause, temp);
        }
    }
}
}

void processOperation(const QJsonObject& opObj, bool& error,
                      QHash<QString, QHash<QString, QVariant>>& elements,
                      int& currentPosition, int& index_memory_var,
                      CNFClause& current, size_t& max_bytes, CNFClause& cnf, int& isFirstClause, CNFClause& temp) {

    if (!opObj.contains("op")) {
        error = true;
        return;
    }

    QString opType = opObj["op"].toString();
    qDebug() << "Processing operation:" << opType;

    // Обработка веток операций (if/else/while и т.д.)
    if (opObj.contains("branch true")) {
        QJsonValue branchTrue = opObj["branch true"];
        if (branchTrue.isObject()) {
            QJsonObject branchObj = branchTrue.toObject();
            if (branchObj.contains("body")) {
                processBody(branchObj["body"], error, elements,
                            currentPosition, index_memory_var, current, max_bytes, cnf, isFirstClause, temp);
            }
        }
    }

    if (opObj.contains("branch false")) {
        QJsonValue branchFalse = opObj["branch false"];
        if (branchFalse.isObject()) {
            QJsonObject branchObj = branchFalse.toObject();
            if (branchObj.contains("body")) {
                processBody(branchObj["body"], error, elements,
                            currentPosition, index_memory_var, current, max_bytes, cnf, isFirstClause, temp);
            }
        }
    }
}

void parseJsonFile(const QString& filePath,
                   QHash<QString, QHash<QString, QVariant>>& elements,
                   bool& error, CNFClause& cnf) {
    QFile file(filePath);

    // 1. Открытие файла
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << filePath;
        std::cout << "WARNING: Failed to open file: " << filePath.toStdString() << std::endl;
        error = true;
        return;
    }

    // 2. Чтение и парсинг JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (doc.isNull()) {
        qWarning() << "JSON parse error:" << parseError.errorString() << "at offset" << parseError.offset;
        std::cout << "WARNING: JSON parse error: " << std::endl;
        error = true;
        return;
    }

    // 3. Проверка структуры JSON
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON structure: expected root object";
        std::cout << "WARNING: Invalid JSON structure: expected root object" << std::endl;
        error = true;
        return;
    }

    QJsonObject rootObject = doc.object();
    if (!rootObject.contains("code")) {
        qWarning() << "Missing 'code' object in root";
        std::cout << "WARNING: Missing 'code' object in root" << std::endl;
        error = true;
        return;
    }

    if (!rootObject["code"].isObject()) {
        qWarning() << "'code' should be an object";
        std::cout << "WARNING: 'code' should be an object" << std::endl;
        error = true;
        return;
    }

    QJsonObject codeObject = rootObject["code"].toObject();
    if (!codeObject.contains("rows")) {
        qWarning() << "Missing 'rows' array in code object";
        std::cout << "WARNING: Missing 'rows' array in code object" << std::endl;
        error = true;
        return;
    }

    if (!codeObject["rows"].isArray()) {
        qWarning() << "'rows' should be an array";
        std::cout << "'rows' should be an array" << std::endl;
        error = true;
        return;
    }

    // 4. Обработка rows
    size_t max_bytes = 1;
    size_t current_bytes = 1;
    QJsonArray rows = codeObject["rows"].toArray();
    int currentPosition = 1;  // Текущая позиция для размещения переменных в CNF
    int memoryVariableIndex = 0;  // Счетчик для генерации уникальных имен переменных памяти
    int isFirstClause = 0;  // Флаг: 0 означает, что это первая клауза в CNF
    for (const QJsonValue& row : rows) {
        QJsonObject rowObject = row.toObject();  // Текущая строка как JSON-объект
        CNFClause currentClause;  // Создаем новую клаузу CNF для текущей строки
        CNFClause temp;
        // Проверяем наличие обязательного поля "id" в строке
        if (rowObject.contains("id")) {
            int clauseId = rowObject.value("id").toInt();  // Уникальный идентификатор строки
            if (clauseId == 71) {
                qDebug() << "fds";
            }

            currentClause.position = clauseId;  // Сохраняем ID в клаузе
            temp.position = clauseId;
            // Обработка случая, когда строка содержит объявление переменной
            if (rowObject.contains("variable")) {
                QString variableName = rowObject["variable"].toString();  // Имя переменной

                // Проверяем, встречалась ли уже такая переменная
                auto variableIt = elements.find(variableName);
                if (variableIt == elements.end()) {
                    // Если переменная новая - регистрируем ее в системе
                    setElement(elements, variableName, "Variable", currentPosition);
                    currentPosition += 1;  // Увеличиваем счетчик позиций
                }

                // Обработка значения переменной (если указано)
                if (rowObject.contains("value")) {
                    QJsonValue variableValue = rowObject["value"];
                    bool isNullValue = variableValue.isNull();  // Проверка на NULL
                    bool isMalloc = (!isNullValue && variableValue.isString() &&
                                     variableValue.toString() == "malloc");  // Проверка на malloc
                    size_t variablePosition = elements[variableName]["position"].toInt();  // Позиция переменной в CNF

if (!variableValue.isObject()) {
                    // Обработка NULL-значения
                    if (isNullValue) {
                        currentClause.setNegativeBit(variablePosition, "Variable", max_bytes);
                        currentClause.setPositiveBit(0, "Variable", max_bytes);
                    }
                    else if (isMalloc) {
                        CNFClause temp;
                        currentClause.setNegativeBit(variablePosition , "Variable", max_bytes);
                        currentClause.setPositiveBit(currentPosition, "mem_var", max_bytes);

                        // Генерируем уникальные имена для переменных памяти и регистрируем их
                        elements[variableName]["memory"] = "N" + QString::number(memoryVariableIndex);
                        setElement(elements, "N" + QString::number(memoryVariableIndex), "mem_var", currentPosition);
                        memoryVariableIndex += 1;
                        currentPosition += 1;
                        size_t parentId = currentPosition - 1;
                        temp.position = clauseId;
                        // Добавляем еще две переменные памяти (возможно, для адреса и размера)
                        setElement(elements, "N" + QString::number(memoryVariableIndex), "mem_var", currentPosition);
                        temp.setNegativeBit(parentId, "mem_var", max_bytes);
                        temp.setPositiveBit(currentPosition, "mem_var", max_bytes);
                        if (isFirstClause == 0) {
                            // Если это первая клауза - просто сохраняем ее как основу CNF
                            cnf = temp;
                            isFirstClause += 1;
                        }
                        else {
                            // Для последующих клауз - добавляем их к существующей CNF
                            cnf.addClause(temp);
                        }
                        temp.clearNegativeBit(parentId, "mem_var", max_bytes);
                        temp.clearPositiveBit(currentPosition, "mem_var", max_bytes);
                        memoryVariableIndex += 1;
                        currentPosition += 1;

                        setElement(elements, "N" + QString::number(memoryVariableIndex), "mem_var", currentPosition);
                        temp.setNegativeBit(parentId, "mem_var", max_bytes);
                        temp.setPositiveBit(currentPosition, "mem_var", max_bytes);
                            // Для последующих клауз - добавляем их к существующей CNF
                            cnf.addClause(temp);
                        temp.clearNegativeBit(parentId, "mem_var", max_bytes);
                        temp.clearPositiveBit(currentPosition, "mem_var", max_bytes);
                        memoryVariableIndex += 1;
                        currentPosition += 1;
                    }
                }
                    // Обработка случая, когда значение - сложный объект
                    else if (variableValue.isObject()) {
                        currentClause.setNegativeBit(variablePosition , "Variable", max_bytes);
                        QJsonObject variableValueObject = variableValue.toObject();
                        if (variableValueObject.contains ("variable")) {
                            QString variableValueName = variableValueObject["variable"].toString();
                            QString variableValueMemoryName = elements[variableValueName]["memory"].toString();
                            auto it = elements.find(variableValueMemoryName);
                            if (it == elements.end()) {
                                error = true;
                                return;
                            }
                            if (variableValueMemoryName == "NULL") {
                                if (variableValueObject.contains("field")) {
                                    error = true;
                                    return;
                                }
                                size_t position = elements[variableValueName]["position"].toInt(); // Позиция
                                currentClause.setPositiveBit(position, "variable", max_bytes);
                                elements[variableName]["memory"] = "N" + QString::number(position);
                                return;
                            }
                            if (variableValueObject.contains("field")) {
                                int nestingLevel = 1;
                                processValue_field(variableValueObject["field"], elements, variableValueName,
                                                   nestingLevel, error, currentClause, max_bytes);
                                size_t position = elements[variableValueName]["position"].toInt();
                                elements[variableName]["memory"] = "N" + QString::number(position);
                            }
                            else {
                                size_t position = elements[variableValueMemoryName]["position"].toInt(); // Позиция переменной
                                currentClause.setPositiveBit(position, "variable", max_bytes);
                                elements[variableName]["memory"] = "N" + QString::number(position);
                            }

                        }
                        else {
                            qDebug() << "Error: Unknown component";
                            error = true;
                            break;
                        }
                    }

                }
                // Обработка полей структуры
                else if (rowObject.contains("field")) {
                    bool isFieldFound = false;
                    int nestingLevel = 1;  // Уровень вложенности поля (для структур в структурах)
                     processStructureField(rowObject["field"], error, variableName, memoryVariableIndex,
                                 nestingLevel, elements, isFieldFound, currentClause, currentPosition, max_bytes, cnf, isFirstClause, temp);
                }
                else {
                    qDebug() << "Error: Unknown component";
                    error = true;
                    break;
                }
            }
            // Обработка операций (если строка содержит операцию, а не переменную)
            else if (rowObject.contains("operation")) {
                processOperation(rowObject["operation"].toObject(), error, elements,
                                 currentPosition, memoryVariableIndex, currentClause, max_bytes, cnf, isFirstClause, temp);
            }
            else {
                qDebug() << "Error: Row must contain either 'variable' or 'operation'";
                error = true;
                break;
            }
        }

        // Добавляем сформированную клаузу в результирующую CNF-формулу
        if (isFirstClause == 0) {
            // Если это первая клауза - просто сохраняем ее как основу CNF
            cnf = currentClause;
            isFirstClause += 1;
        }
        else {
            // Для последующих клауз - добавляем их к существующей CNF
            cnf.addClause(currentClause);
        }
        if (max_bytes > current_bytes) {
            cnf.resizeAll(max_bytes);
        }
    }
}
