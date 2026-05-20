QT = core
CONFIG += c++17 cmdline
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TreeProgram

INCLUDEPATH += \
    include/model \
    include/solver \
    include/operations \
    include/analysis \
    third_party/minisat

DEFINES += __STDC_LIMIT_MACROS __STDC_FORMAT_MACROS

# Model
HEADERS += \
    include/model/BoolVector.h \
    include/model/TreeElement.h \
    include/model/Connection.h \
    include/model/Tree.h \
    include/model/TreeManager.h

SOURCES += \
    src/model/BoolVector.cpp \
    src/model/TreeElement.cpp \
    src/model/Connection.cpp \
    src/model/Tree.cpp \
    src/model/TreeManager.cpp

# Solver
HEADERS += \
    include/solver/ISATSolver.h \
    include/solver/MinisatSolver.h

SOURCES += \
    src/solver/MinisatSolver.cpp

# Memory operations
HEADERS += \
    include/operations/IMemoryOperation.h \
    include/operations/MemoryOperations.h

SOURCES += \
    src/operations/MemoryOperations.cpp

# Analysis
HEADERS += \
    include/analysis/IMemoryAnalyzer.h \
    include/analysis/GraphBuilder.h \
    include/analysis/RowProcessor.h \
    include/analysis/MemoryAnalyzer.h

SOURCES += \
    src/analysis/GraphBuilder.cpp \
    src/analysis/RowProcessor.cpp \
    src/analysis/MemoryAnalyzer.cpp

# Entry point
SOURCES += src/main.cpp

# Third-party
SOURCES += \
    third_party/minisat/minisat/core/Solver.cc \
    third_party/minisat/minisat/utils/Options.cc \
    third_party/minisat/minisat/utils/System.cc
