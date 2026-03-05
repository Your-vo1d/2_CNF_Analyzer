QT = core
CONFIG += c++17 cmdline
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TreeProgram

INCLUDEPATH += include /usr/local/include
LIBS += -lminisat

SOURCES += \
    src/main.cpp \
    src/Connection.cpp \
    src/Tree.cpp \
    src/TreeElement.cpp \
    src/TreeManager.cpp \
    src/BoolVector.cpp \
    src/Parser_JSON.cpp \
    src/CNFNode.cpp

HEADERS += \
    include/BoolVector.h \
    include/Connection.h \
    include/Parser_JSON.h \
    include/Tree.h \
    include/TreeElement.h \
    include/TreeManager.h \
    include/CNFNode.h
