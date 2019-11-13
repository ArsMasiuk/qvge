# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2019 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.


TEMPLATE = app
TARGET = qvgeapp

win32{
        VERSION = 0.5.4.0
	RC_ICONS = $$PWD/win32/icon.ico
        QMAKE_TARGET_COPYRIGHT = (C) 2016-2019 Ars L. Masiuk
	QMAKE_TARGET_DESCRIPTION = Qt Visual Graph Editor
	QMAKE_TARGET_PRODUCT = qvge
}


include(../config.pri)
include(../app.pri)


# app sources
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS += $$files($$PWD/*.ui)
RESOURCES += $$files($$PWD/*.qrc)


# includes & libs
INCLUDEPATH += $$PWD


# install
INSTALLS += target

