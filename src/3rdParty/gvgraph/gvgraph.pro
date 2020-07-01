include(../common.pri)

TARGET = gvgraph
QT += core 

SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)

INCLUDEPATH += .
