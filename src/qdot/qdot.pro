# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.


TEMPLATE = app
TARGET = qdot

win32{
	VERSION = 0.0.1.0
	RC_ICONS = $$PWD/win32/icon.ico
	QMAKE_TARGET_COPYRIGHT = (C) 2016-2020 Ars L. Masiuk
	QMAKE_TARGET_DESCRIPTION = Qt Visual GraphViz Assistent
	QMAKE_TARGET_PRODUCT = qdot
}


include($$PWD/../config.pri)
include($$PWD/../app.pri)


# app sources
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS += $$files($$PWD/*.ui)
RESOURCES += $$files($$PWD/*.qrc)


# includes & libs
INCLUDEPATH += $$PWD


# install
unix{
    PREFIX_DIR = $${PREFIX}

#message($$PREFIX_DIR)

    isEmpty(PREFIX_DIR) {
        PREFIX_DIR = /usr/local
    }

#message($$PREFIX_DIR)

    target.path = $$PREFIX_DIR/bin

#message($$target.path)

    INSTALLS += target

    desktop.path = $$PREFIX_DIR/share/applications/
    desktop.files = $$PWD/linux/qdot.desktop
    INSTALLS += desktop

    icon.path = $$PREFIX_DIR/share/pixmaps
    icon.files = $$PWD/linux/qdot.png
    INSTALLS += icon

    appdata.path = $$PREFIX_DIR/share/appdata/
    appdata.files = $$PWD/linux/qdot.appdata.xml
    INSTALLS += appdata
}

