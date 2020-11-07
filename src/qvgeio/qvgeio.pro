TEMPLATE = lib
TARGET = qvgeio


# common config
CONFIG += static c++14
QT += core xml


# compiler stuff
win32-msvc*{
    QMAKE_CXXFLAGS += /MP
}

gcc{
    QMAKE_CXXFLAGS += -Wno-unused-variable -Wno-unused-parameter
    QMAKE_CXXFLAGS += -isystem
}


# input
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
#FORMS += $$files($$PWD/*.ui)
#RESOURCES += $$files($$PWD/*.qrc)

INCLUDEPATH += $$PWD $$PWD/..


# output
CONFIG(debug, debug|release){
    DESTDIR = $$OUT_PWD/../lib.debug
}
else{
    DESTDIR = $$OUT_PWD/../lib
}
