QT = core

CONFIG += c++17 cmdline
CONFIG -= app_bundle
TEMPLATE = app

TARGET = TreeProgram  # Явно задаем имя программы

INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lminisat

SOURCES += \
    Connection.cpp \
    Tree.cpp \
    TreeElement.cpp \
    TreeManager.cpp \
    main.cpp \
    BoolVector.cpp \
    Parser_JSON.cpp

HEADERS += \
    BoolVector.h \
    Connection.h \
    Parser_JSON.h \
    Tree.h \
    TreeElement.h \
    TreeManager.h

# УДАЛИТЬ ВСЕ что связано с запуском и копированием!
