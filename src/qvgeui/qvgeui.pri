# config
include($$PWD/../config.pri)

# app sources
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS += $$files($$PWD/*.ui)
RESOURCES += $$files($$PWD/*.qrc)

# base sources
include($$PWD/../appbase/appbase.pri)

# includes 
INCLUDEPATH += $$PWD $$PWD/.. 
INCLUDEPATH += $$PWD/../3rdParty
INCLUDEPATH += $$PWD/../3rdParty/qsint-widgets 
INCLUDEPATH += $$PWD/../3rdParty/qtpropertybrowser 

USE_OGDF{
    INCLUDEPATH += $$OGDF_INCLUDE_PATH

    SOURCES += $$files($$PWD/ogdf/*.cpp)
    HEADERS += $$files($$PWD/ogdf/*.h)
    FORMS += $$files($$PWD/ogdf/*.ui)
}

USE_BOOST{
    INCLUDEPATH += $$BOOST_INCLUDE_PATH
}

USE_GVGRAPH{
    SOURCES += $$files($$PWD/gvgraph/*.cpp)
    HEADERS += $$files($$PWD/gvgraph/*.h)
}
