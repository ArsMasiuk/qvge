include(../common.pri)
include(./gvgraph.pri)

TARGET = gvgraph
QT += core 

SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)

SOURCES += $$files($$PWD/graphviz/cgraph/*.c)
HEADERS += $$files($$PWD/graphviz/cgraph/*.h)

SOURCES += $$files($$PWD/graphviz/cdt/*.c)
HEADERS += $$files($$PWD/graphviz/cdt/*.h)

SOURCES += $$files($$PWD/graphviz/gvc/*.c)
HEADERS += $$files($$PWD/graphviz/gvc/*.h)

SOURCES += $$files($$PWD/graphviz/common/*.c)
HEADERS += $$files($$PWD/graphviz/common/*.h)

SOURCES += $$files($$PWD/graphviz/pathplan/*.c)
HEADERS += $$files($$PWD/graphviz/pathplan/*.h)

SOURCES += $$files($$PWD/graphviz/sparse/*.c)
HEADERS += $$files($$PWD/graphviz/sparse/*.h)

SOURCES += $$files($$PWD/graphviz/pack/*.c)
HEADERS += $$files($$PWD/graphviz/pack/*.h)

SOURCES += $$files($$PWD/graphviz/ast/*.c)
HEADERS += $$files($$PWD/graphviz/ast/*.h)

SOURCES += $$files($$PWD/graphviz/sfio/*.c)
HEADERS += $$files($$PWD/graphviz/sfio/*.h)

SOURCES += $$files($$PWD/graphviz/label/*.c)
HEADERS += $$files($$PWD/graphviz/label/*.h)

SOURCES += $$files($$PWD/graphviz/xdot/*.c)
HEADERS += $$files($$PWD/graphviz/xdot/*.h)

SOURCES += $$files($$PWD/graphviz/fdpgen/*.c)
HEADERS += $$files($$PWD/graphviz/fdpgen/*.h)

SOURCES += $$files($$PWD/graphviz/neatogen/*.c)
HEADERS += $$files($$PWD/graphviz/neatogen/*.h)

HEADERS += $$files($$PWD/include/*.h)

HEADERS += $$files($$PWD/boost/*.h)
HEADERS += $$files($$PWD/boost/*.hpp)
HEADERS += $$files($$PWD/boost/regex/src/*.hpp)
SOURCES += $$files($$PWD/boost/regex/src/*.cpp)

SOURCES += $$files($$PWD/graphviz/tools/*.c)
#HEADERS += $$files($$PWD/graphviz/tools/*.h)
