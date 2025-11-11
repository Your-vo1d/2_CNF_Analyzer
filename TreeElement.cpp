#include "TreeElement.h"
#include <QDebug>

TreeElement::TreeElement(const QString& name, int type, int position)
    : name_(name), type_(type), position_(position)
{
}

TreeElement::TreeElement(const TreeElement& other)
    : name_(other.name_), type_(other.type_), position_(other.position_)
{
}

TreeElement::~TreeElement()
{
}

TreeElement& TreeElement::operator=(const TreeElement& other)
{
    // Проверка на самоприсваивание
    if (this != &other) {
        name_ = other.name_;
        type_ = other.type_;
        position_ = other.position_;
    }
    return *this;
}

QString TreeElement::getTypeName() const
{
    switch (type_) {
    case ROOT: return "ROOT";
    case MEMORY: return "MEMORY";
    case NULL_ELEMENT: return "NULL_ELEMENT";
    case LEFT_VARIABLE: return "LEFT_VARIABLE";
    case RIGHT_VARIABLE: return "RIGHT_VARIABLE";
    default: return "UNKNOWN";
    }
}

bool TreeElement::operator==(const TreeElement& other) const
{
    return (name_ == other.name_) &&
           (type_ == other.type_) &&
           (position_ == other.position_);
}

bool TreeElement::operator!=(const TreeElement& other) const
{
    return !(*this == other);
}

bool TreeElement::isValid() const
{
    // Элемент считается валидным, если имя не пустое
    // и позиция не отрицательная
    return !name_.isEmpty() && position_ >= 0;
}

QString TreeElement::toString() const
{
    return QString("TreeElement[name: '%1', type: %2 (%3), position: %4]")
        .arg(name_)
        .arg(type_)
        .arg(getTypeName())
        .arg(position_);
}
