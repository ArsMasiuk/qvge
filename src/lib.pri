TEMPLATE=lib
CONFIG+=static

include($$PWD/config.pri)

CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../lib
}

