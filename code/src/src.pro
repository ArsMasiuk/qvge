# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.


TEMPLATE = app
TARGET = qvge
DESTDIR = $$OUT_PWD/../bin

QT += core gui widgets xml opengl network printsupport
CONFIG += c++11

# app sources
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS += $$files($$PWD/*.ui)
RESOURCES += $$files($$PWD/*.qrc)

# base sources
include($$PWD/../base/base.pri)
include($$PWD/../qvge/qvge.pri)

# includes & libs
INCLUDEPATH += $$PWD $$PWD/.. $$PWD/../3rdParty/qsint-widgets $$PWD/../3rdParty/qtpropertybrowser $$PWD/../3rdParty/ogdf/include

LIBS += -L$$OUT_PWD/../lib -logdf -lqtpropertybrowser -lqsint-widgets

win32{
    LIBS += -lopengl32 -lglu32 -lshell32 -luser32 -lpsapi

    RC_FILE = $$PWD/../win32/icon.rc
}
