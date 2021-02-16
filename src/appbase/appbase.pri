SOURCES     += $$PWD/CPlatformServices.cpp $$PWD/CMainWindow.cpp $$PWD/CStartPage.cpp
HEADERS     += $$PWD/CPlatformServices.h $$PWD/CMainWindow.h $$PWD/CStartPage.h

FORMS       += $$files($$PWD/*.ui)
RESOURCES   += $$files($$PWD/*.qrc)

unix{
    !if(haiku|mac){
        QT += x11extras
        LIBS += -lX11
    }
    !haiku {
        SOURCES += $$PWD/../3rdParty/qprocessinfo/qprocessinfo.cpp
        HEADERS += $$PWD/../3rdParty/qprocessinfo/qprocessinfo.h
        INCLUDEPATH += $$PWD/../3rdParty/qprocessinfo
    }
}

win32{
        SOURCES += $$PWD/CPlatformWin32.cpp
        HEADERS += $$PWD/CPlatformWin32.h
}
