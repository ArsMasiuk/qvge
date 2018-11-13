# includes & libs
INCLUDEPATH += $$PWD/3rdParty/qtpropertybrowser $$PWD/3rdParty/qsint-widgets


CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../bin.debug
        LIBS += -L$$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../bin
        LIBS += -L$$OUT_PWD/../lib
}


LIBS += -lcommonui -lqvge -lqvgeio -lqtpropertybrowser -lqsint-widgets

USE_OGDF{
    LIBS += -logdf
}

win32{
    LIBS += -lopengl32 -lglu32 -lshell32 -luser32 -lpsapi

    RC_FILE = $$PWD/win32/icon.rc
}

cygwin*{
    LIBS += -lopengl32 -lglu32 -lshell32 -luser32 -lpsapi
}

unix{
    LIBS += -lQt5X11Extras -lX11
}
