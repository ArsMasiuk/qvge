# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.

#CONFIG += USE_OGDF
CONFIG += USE_GVGRAPH
#CONFIG += USE_BOOST


# external OGDF
USE_OGDF{
    DEFINES += USE_OGDF

    # system-specific OGDF setup
    OGDF_LIB_NAME = ogdf
    OGDF_LIB_PATH =
    OGDF_INCLUDE_PATH = /usr/share/ogdf/include
}

# external boost::graph
USE_BOOST{
    DEFINES += USE_BOOST

    # system-specific Boost setup
    BOOST_LIB_NAME = boost_graph
    BOOST_LIB_PATH =
    BOOST_INCLUDE_PATH = /usr/share/boost/include
}

# external GraphViz (only #define)
USE_GVGRAPH{
    DEFINES += USE_GVGRAPH
}


# compiler stuff
win32-msvc*{
    QMAKE_CXXFLAGS += /MP
}

gcc{
    QMAKE_CXXFLAGS += -Wno-unused-variable -Wno-unused-parameter -Wno-misleading-indentation
    QMAKE_CXXFLAGS += -isystem
}


# common config
QT += core gui widgets xml opengl network printsupport svg
CONFIG += c++14


# output
CONFIG(debug, debug|release){
    LIBS += -L$$OUT_PWD/../lib.debug
}
else{
    LIBS += -L$$OUT_PWD/../lib
}

# temp dirs (unix)
unix{
    MOC_DIR = $$OUT_PWD/_generated
    OBJECTS_DIR = $$OUT_PWD/_generated
    UI_DIR = $$OUT_PWD/_generated
    RCC_DIR = $$OUT_PWD/_generated
}
