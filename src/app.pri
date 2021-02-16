# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.


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


# workaround for #126
CONFIG += no_lflags_merge
LIBS += -lcommonui -lqvgeioui -lcommonui -lqvge -lqvgeio -lqtpropertybrowser -lqsint-widgets


USE_OGDF{
	LIBS += -L$$OGDF_LIB_PATH -l$$OGDF_LIB_NAME
	INCLUDEPATH += $$OGDF_INCLUDE_PATH
}


USE_BOOST{
	LIBS += -L$$BOOST_LIB_PATH -l$$BOOST_LIB_NAME
	INCLUDEPATH += $$BOOST_INCLUDE_PATH
}


win32{
    LIBS += -lopengl32 -lglu32 -lshell32 -luser32 -lpsapi
}


unix{
    !if(haiku|mac){
        QT += x11extras
        LIBS += -lX11
    }
}

