include($$PWD/../lib.pri)

TARGET = qvgeioui
QT += core gui widgets printsupport xml

# includes 
INCLUDEPATH += $$PWD $$PWD/.. 

# sources
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS += $$files($$PWD/*.ui)