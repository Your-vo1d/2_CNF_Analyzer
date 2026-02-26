#ifndef CONNECTION_H
#define CONNECTION_H

#include <QString>
#include "BoolVector.h"

class Connection
{
public:
    Connection(const QString& from = QString(),
               const QString& to = QString(),
               const BoolVector& positiveVector = BoolVector(),
               const BoolVector& negativeVector = BoolVector(),
               int position = -1);
    Connection(const Connection& other);
    Connection& operator=(const Connection& other);

    // Геттеры
    QString getFrom() const { return from_; }
    QString getTo() const { return to_; }
    BoolVector getPositiveVector() const { return positiveVector_; }
    BoolVector getNegativeVector() const { return negativeVector_; }
    int getPosition() const { return position_; }

    // Сеттеры
    void setFrom(const QString& from) { from_ = from; }
    void setTo(const QString& to) { to_ = to; }
    void setPositiveVector(const BoolVector& vector) { positiveVector_ = vector; }
    void setNegativeVector(const BoolVector& vector) { negativeVector_ = vector; }
    void setPosition(int position) { position_ = position; }

    // Методы для работы с битами положительного вектора
    void setPositiveBit(size_t bitIndex);
    void clearPositiveBit(size_t bitIndex);
    bool getPositiveBit(size_t bitIndex) const;
    void resizePositiveVector(size_t newSize);
    size_t getPositiveVectorSize() const;

    // Методы для работы с битами отрицательного вектора
    void setNegativeBit(size_t bitIndex);
    void clearNegativeBit(size_t bitIndex);
    bool getNegativeBit(size_t bitIndex) const;
    void resizeNegativeVector(size_t newSize);
    size_t getNegativeVectorSize() const;

    // Вспомогательные методы
    bool isValid() const;
    QString toString() const;

private:
    QString from_;
    QString to_;
    BoolVector positiveVector_;
    BoolVector negativeVector_;
    int position_;  // Позиция (id из парсинга)
};

#endif // CONNECTION_H
