TEMPLATE=lib
CONFIG+=static

win32-msvc*{
  QMAKE_CXXFLAGS += /MP
}
