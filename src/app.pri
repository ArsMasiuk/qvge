# includes & libs
INCLUDEPATH += $$PWD $$PWD/3rdParty/qtpropertybrowser $$PWD/3rdParty/qsint-widgets


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
}

unix{
    !haiku{
        LIBS += -lQt5X11Extras -lX11
    }
}


# install
unix{
    isEmpty(PREFIX) {
        PREFIX = /usr/local/bin
    }

    target.path = $$PREFIX/
}
