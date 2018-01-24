# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.


TEMPLATE = app
TARGET = qvge
DESTDIR = bin

QT += core gui widgets xml opengl network printsupport
CONFIG += c++11

# app sources
SOURCES += $$files($$PWD/src/*.cpp)
HEADERS += $$files($$PWD/src/*.h)
FORMS += $$files($$PWD/src/*.ui)
RESOURCES += $$files($$PWD/src/*.qrc)

# base sources
include($$PWD/base/base.pri)
include($$PWD/qvge/qvge.pri)

# 3rd party sources
SOURCES += $$files($$PWD/3rdParty/qsint-widgets/*.cpp)
HEADERS += $$files($$PWD/3rdParty/qsint-widgets/*.h)

SOURCES += $$files($$PWD/3rdParty/qtpropertybrowser/*.cpp)
HEADERS += $$files($$PWD/3rdParty/qtpropertybrowser/*.h)
RESOURCES += $$files($$PWD/3rdParty/qtpropertybrowser/*.qrc)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

include($$PWD/3rdParty/ogdf/ogdf.pri)

# includes & libs
INCLUDEPATH += $$PWD $$PWD/.. $$PWD/src $$PWD/3rdParty/qsint-widgets $$PWD/3rdParty/qtpropertybrowser

win32{
    LIBS += -lopengl32 -lglu32 -lshell32 -luser32 -lpsapi

    RC_FILE = $$PWD/win32/icon.rc
}
