# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.

CONFIG += USE_OGDF
USE_OGDF{
    DEFINES += USE_OGDF
    #message(USE_OGDF!)
}


# compiler stuff
win32-msvc*{
  QMAKE_CXXFLAGS += /MP
}


# common config
QT += core gui widgets xml opengl network printsupport
CONFIG += c++14



CONFIG(debug, debug|release){
        LIBS += -L$$OUT_PWD/../lib.debug
}
else{
        LIBS += -L$$OUT_PWD/../lib
}


