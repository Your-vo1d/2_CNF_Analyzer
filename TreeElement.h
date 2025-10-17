#ifndef TREEELEMENT_H
#define TREEELEMENT_H

#include <QString>

class TreeElement
{
public:
    // Типы элементов
    enum Type {
        ROOT = 0,           // Корень дерева
        MEMORY = 1,         // Память (создается после malloc)
        LEFT_VARIABLE = 3,  // Левая переменная
        RIGHT_VARIABLE = 4  // Правая переменная
    };

    // Конструкторы
    TreeElement(const QString& name = QString(), int type = ROOT, int position = 0);
    TreeElement(const TreeElement& other);

    // Деструктор
    ~TreeElement();

    // Оператор присваивания
    TreeElement& operator=(const TreeElement& other);

    // Операторы сравнения
    bool operator==(const TreeElement& other) const;
    bool operator!=(const TreeElement& other) const;

    // Геттеры
    QString getName() const { return name_; }
    int getType() const { return type_; }
    int getPosition() const { return position_; }
    QString getTypeName() const;

    // Сеттеры
    void setName(const QString& name) { name_ = name; }
    void setType(int type) { type_ = type; }
    void setPosition(int position) { position_ = position; }

    // Вспомогательные методы
    bool isValid() const;
    QString toString() const;

    // Проверки типов
    bool isRoot() const { return type_ == ROOT; }
    bool isMemory() const { return type_ == MEMORY; }
    bool isLeftVariable() const { return type_ == LEFT_VARIABLE; }
    bool isRightVariable() const { return type_ == RIGHT_VARIABLE; }

private:
    QString name_;
    int type_;
    int position_;
};

#endif // TREEELEMENT_H
