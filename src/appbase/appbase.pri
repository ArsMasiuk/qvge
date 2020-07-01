SOURCES     += $$files($$PWD/*.cpp)
HEADERS     += $$files($$PWD/*.h)
FORMS       += $$files($$PWD/*.ui)
RESOURCES   += $$files($$PWD/*.qrc)

unix{
    !haiku{
        QT += x11extras
        LIBS += -lX11

        SOURCES += $$PWD/../3rdParty/qprocessinfo/qprocessinfo.cpp
        HEADERS += $$PWD/../3rdParty/qprocessinfo/qprocessinfo.h
        INCLUDEPATH += $$PWD/../3rdParty/qprocessinfo
    }
}
