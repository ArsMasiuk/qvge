include(../common.pri)

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

    SOURCES += $$PWD/qtpropertybrowser.cpp \
            $$PWD/qtpropertymanager.cpp \
            $$PWD/qteditorfactory.cpp \
            $$PWD/qtvariantproperty.cpp \
            $$PWD/qttreepropertybrowser.cpp \
            $$PWD/qtbuttonpropertybrowser.cpp \
            $$PWD/qtgroupboxpropertybrowser.cpp \
            $$PWD/qtpropertybrowserutils.cpp \
			$$PWD/lineedit.cpp 
    HEADERS += $$PWD/qtpropertybrowser.h \
            $$PWD/qtpropertymanager.h \
            $$PWD/qteditorfactory.h \
            $$PWD/qtvariantproperty.h \
            $$PWD/qttreepropertybrowser.h \
            $$PWD/qtbuttonpropertybrowser.h \
            $$PWD/qtgroupboxpropertybrowser.h \
            $$PWD/qtpropertybrowserutils_p.h \
			$$PWD/lineedit.h
    RESOURCES += $$PWD/qtpropertybrowser.qrc
