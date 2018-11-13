include(../lib.pri)

CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../lib
}

TARGET = commonui
QT += core gui widgets printsupport xml

include($$PWD/commonui.pri)