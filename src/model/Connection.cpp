#include "Connection.h"
#include <QDebug>

Connection::Connection(const QString& from, const QString& to,
                       const BoolVector& positiveVector, const BoolVector& negativeVector,
                       int position)
    : from_(from), to_(to), positiveVector_(positiveVector),
    negativeVector_(negativeVector), position_(position)
{
}

Connection::Connection(const Connection& other)
    : from_(other.from_), to_(other.to_),
    positiveVector_(other.positiveVector_),
    negativeVector_(other.negativeVector_),
    position_(other.position_)
{
}


Connection& Connection::operator=(const Connection& other)
{
    if (this != &other) {
        from_ = other.from_;
        to_ = other.to_;
        positiveVector_ = other.positiveVector_;
        negativeVector_ = other.negativeVector_;

    }
    return *this;
}

// Методы для работы с битами положительного вектора
void Connection::setPositiveBit(size_t bitIndex)
{
    positiveVector_.setBit(bitIndex);
}

void Connection::clearPositiveBit(size_t bitIndex)
{
    positiveVector_.clearBit(bitIndex);
}

bool Connection::getPositiveBit(size_t bitIndex) const
{
    return positiveVector_.getBit(bitIndex);
}

void Connection::resizePositiveVector(size_t newSize)
{
    positiveVector_.resize(newSize);
}

size_t Connection::getPositiveVectorSize() const
{
    return positiveVector_.size();
}

// Методы для работы с битами отрицательного вектора
void Connection::setNegativeBit(size_t bitIndex)
{
    negativeVector_.setBit(bitIndex);
}

void Connection::clearNegativeBit(size_t bitIndex)
{
    negativeVector_.clearBit(bitIndex);
}

bool Connection::getNegativeBit(size_t bitIndex) const
{
    return negativeVector_.getBit(bitIndex);
}

void Connection::resizeNegativeVector(size_t newSize)
{
    negativeVector_.resize(newSize);
}

size_t Connection::getNegativeVectorSize() const
{
    return negativeVector_.size();
}

bool Connection::isValid() const
{
    // Соединение считается валидным, если:
    // 1. Имена from и to не пустые
    // 2. Имена from и to разные
    // 3. Векторы имеют одинаковый размер (опционально, зависит от логики приложения)
    return !from_.isEmpty() &&
           !to_.isEmpty() &&
           from_ != to_ &&
           positiveVector_.size() == negativeVector_.size();
}

QString Connection::toString() const
{
    QString result = QString("Connection[from: '%1', to: '%2', positiveSize: %3, negativeSize: %4]")
                         .arg(from_)
                         .arg(to_)
                         .arg(positiveVector_.size())
                         .arg(negativeVector_.size());

    // Добавляем информацию о битах, если векторы небольшие
    if (positiveVector_.size() <= 16) {
        QString positiveBits, negativeBits;
        for (size_t i = 0; i < positiveVector_.size(); ++i) {
            positiveBits += positiveVector_.getBit(i) ? "1" : "0";
            negativeBits += negativeVector_.getBit(i) ? "1" : "0";
        }
        result += QString("\n  Positive: %1\n  Negative: %2").arg(positiveBits).arg(negativeBits);
    }

    return result;
}
