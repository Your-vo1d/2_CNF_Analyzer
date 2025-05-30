#include "Parser_JSON.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSet>

void addElement(QHash<int, QString>& elements,
                QSet<QString>& uniqueNames,
                int& currentPosition,
                const QString& name)
{
    if (!uniqueNames.contains(name)) {
        elements.insert(++currentPosition, name);
        uniqueNames.insert(name);
        qDebug() << "[" << currentPosition << "]" << name;
    }
}

void processField(const QJsonObject& field,
                  QHash<int, QString>& elements,
                  int& currentPosition,
                  QSet<QString>& uniqueNames,
                  const QString& baseName,
                  int depth = 0)
{
    QString indent(depth * 2, ' ');

    if (!field.contains("f")) {
        qDebug().noquote() << indent << "Field missing 'f' property";
        return;
    }

    QString fieldType = field["f"].toString();
    QString fullName = baseName + "_" + fieldType;

    // Добавляем только само поле (например, root_left)
    addElement(elements, uniqueNames, currentPosition, fullName);

    // Обработка значения поля
    if (field.contains("value")) {
        QJsonValue value = field["value"];

        if (value.isObject()) {
            // Если значение - объект, обрабатываем рекурсивно
            QJsonObject valueObj = value.toObject();
            if (valueObj.contains("field")) {
                processField(valueObj["field"].toObject(), elements, currentPosition,
                             uniqueNames, fullName, depth + 1);
            }
        }
        // Не создаем дополнительные поля для строковых значений
    }

    // Обработка вложенных операций
    if (field.contains("op")) {
        processField(field["op"].toObject(), elements, currentPosition,
                     uniqueNames, fullName, depth + 1);
    }
}

void parseJsonFile(const QString& filePath,
                   QHash<int, QString>& elements,
                   QString& errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = "Failed to open file";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        errorMessage = "Invalid JSON";
        return;
    }

    int currentPosition = 0;
    QSet<QString> uniqueNames;
    QJsonArray rows = doc.object()["code"].toObject()["rows"].toArray();

    for (const QJsonValue& row : rows) {
        QJsonObject obj = row.toObject();

        if (obj.contains("variable")) {
            QString varName = obj["variable"].toString();
            addElement(elements, uniqueNames, currentPosition, varName);

            // Автоматически создаём базовые поля
            addElement(elements, uniqueNames, currentPosition, varName + "_left");
            addElement(elements, uniqueNames, currentPosition, varName + "_right");

            // Обработка поля, если есть
            if (obj.contains("field")) {
                processField(obj["field"].toObject(), elements, currentPosition,
                             uniqueNames, varName);
            }
        }
    }
}
