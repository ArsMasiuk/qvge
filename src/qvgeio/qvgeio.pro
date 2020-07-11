TEMPLATE = lib
CONFIG += static
QT += core xml

TARGET = qvgeio


# compiler stuff
win32-msvc*{
  QMAKE_CXXFLAGS += /MP
}

CONFIG += c++11


# input
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
#FORMS += $$files($$PWD/*.ui)
#RESOURCES += $$files($$PWD/*.qrc)

HEADERS += $$files($$PWD/../3rdParty/boost/graph/*.hpp)
SOURCES += $$files($$PWD/../3rdParty/boost/graph/detail/*.cpp)
SOURCES += $$files($$PWD/../3rdParty/boost/libs/regex/src/*.cpp)


INCLUDEPATH += $$PWD $$PWD/.. $$PWD/../3rdParty


# output
CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../lib
}
