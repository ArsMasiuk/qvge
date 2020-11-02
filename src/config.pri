# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.

#CONFIG += BUILD_OGDF
#CONFIG += USE_LOCAL_OGDF
#CONFIG += USE_EXTERNAL_OGDF

CONFIG += USE_GVGRAPH

BUILD_OGDF{
	CONFIG += USE_LOCAL_OGDF
	CONFIG -= USE_EXTERNAL_OGDF
	CONFIG += USE_OGDF
}

USE_LOCAL_OGDF{
	CONFIG += USE_OGDF
	CONFIG -= USE_EXTERNAL_OGDF
	
	# locally build OGDF
	OGDF_LIB_NAME = ogdf-2020
        OGDF_LIB_PATH = .
	OGDF_INCLUDE_PATH = $$PWD/3rdParty/ogdf-2020/include
}

USE_EXTERNAL_OGDF{
	CONFIG += USE_OGDF
	CONFIG -= USE_LOCAL_OGDF
	
	# system-specific OGDF setup
	OGDF_LIB_NAME = ogdf
	OGDF_LIB_PATH =
	OGDF_INCLUDE_PATH = /usr/share/ogdf/include
}

USE_OGDF{
	DEFINES += USE_OGDF
	#message(USE_OGDF!)
}

# compiler stuff
win32-msvc*{
  	QMAKE_CXXFLAGS += /MP
}

gcc{
    	QMAKE_CXXFLAGS += -Wno-unused-variable -Wno-unused-parameter
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
