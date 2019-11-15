SOURCES += $$files($$PWD/*.cpp)
HEADERS  += $$files($$PWD/*.h)
FORMS    += $$files($$PWD/*.ui)
RESOURCES += $$files($$PWD/*.qrc)

unix{
    !haiku{
        QT += x11extras
        LIBS += -lX11

        SOURCES += $$PWD/../3rdParty/readproc/read_proc.c
        SOURCES += $$PWD/../3rdParty/readproc/struct.c
        HEADERS += $$files($$PWD/../3rdParty/readproc/*.h)

        INCLUDEPATH += $$PWD/../3rdParty/readproc
    }
}
