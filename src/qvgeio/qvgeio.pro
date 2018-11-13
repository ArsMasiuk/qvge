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

INCLUDEPATH += $$PWD $$PWD/..


# output
CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../lib
}
