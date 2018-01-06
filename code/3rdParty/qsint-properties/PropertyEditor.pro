#-------------------------------------------------
#
# Project created by QtCreator 2014-11-28T23:45:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PropertyEditor
TEMPLATE = app

include(./PropertyEditor.pri)

SOURCES  += main.cpp\
            testwidget.cpp
HEADERS  += testwidget.h
FORMS    += testwidget.ui
RESOURCES += testwidget.qrc

INCLUDEPATH += .

