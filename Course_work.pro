QT = core

CONFIG += c++17 cmdline
CONFIG += debug
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lminisat
QMAKE_CXXFLAGS += -g
SOURCES += \
    main.cpp \
    Parser_JSON.cpp \
    CNFClause.cpp \
    BoolVector.cpp \
# TRANSLATIONS += \
#    Course_work_ru_RU.ts
# CONFIG += lrelease
# CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Parser_JSON.h \
    BoolVector.h \
    CNFClause.h \

