QT = core
CONFIG += c++17 cmdline
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TreeProgram

INCLUDEPATH += include third_party/minisat

DEFINES += __STDC_LIMIT_MACROS __STDC_FORMAT_MACROS

SOURCES += \
    src/main.cpp \
    src/Connection.cpp \
    src/Tree.cpp \
    src/TreeElement.cpp \
    src/TreeManager.cpp \
    src/BoolVector.cpp \
    src/CNFNode.cpp \
    src/Parser_JSON_Core.cpp \
    src/Parser_JSON_Row.cpp \
    src/Parser_JSON_Structure.cpp \
    src/Parser_JSON_Memory.cpp \
    src/Parser_JSON_Tree.cpp \
    src/Parser_JSON_CNF.cpp \
    third_party/minisat/minisat/core/Solver.cc \
    third_party/minisat/minisat/utils/Options.cc \
    third_party/minisat/minisat/utils/System.cc

HEADERS += \
    include/BoolVector.h \
    include/Connection.h \
    include/Parser_JSON.h \
    include/Tree.h \
    include/TreeElement.h \
    include/TreeManager.h \
    include/CNFNode.h
