include(../lib.pri)


# compiler stuff
win32-msvc*{
  	QMAKE_CXXFLAGS += /MP
}


# output
CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../../lib
}
