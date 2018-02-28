## Qt Visual Graph Editor 
[![Github All Releases](https://img.shields.io/github/downloads/ArsMasiuk/qvge/total.svg?style=for-the-badge)](https://github.com/ArsMasiuk/qvge/releases/latest)


**qvge** is a multiplatform graph editor written in C++/Qt. Its main goal is to make possible visually edit two-dimensional graphs
in a simple and intuitive way.

Please note that **qvge** is not a replacement for such a software like Gephi, Graphvis, Dot, yEd, Dia and so on. It is neither a tool for "big data analysis" nor a math application. It is really just a simple graph editor :)

![Screenshot1](https://a.fsdn.com/con/app/proj/qvge/screenshots/qvge%20screenshot.PNG/245/183/2)
![Screenshot2](https://a.fsdn.com/con/app/proj/qvge/screenshots/qvge%20screenshot%202.PNG/245/183/2)

### Main Features

- Easy creation and parameterising of small-sized till middle-sized graphs
- Dynamically maintained list of commutations between nodes
- Directed, undirected and mixed graphs supported
- Auto-creation and auto-layout of graphs (via OGDF)
- Native graph persistence format (XGR)
- Reading GraphML, GML and GEXF files
- Export into PDF and popular image formats
- Export into GraphViz DOT format
- Export into GEXF format

### Installation

Prebuild Windows binaries can be loaded from here:

[![Github All Releases](https://img.shields.io/github/downloads/ArsMasiuk/qvge/total.svg?style=for-the-badge)](https://github.com/ArsMasiuk/qvge/releases/latest)

Or you can get qvge's sources and build them by yourself. In this case you need to have installed Qt 5.x toolkit and corresponding C++ compiler with C++11 support. qvge uses native Qt build system (main project file is qvgeapp.pro) so it should look like:

~~~~
cd <directory-with-qvgeapp.pro>
qmake -r
~~~~

Linux GCC:
~~~~          
make 
~~~~

or Windows MinGW:
~~~~
mingw32-make 
~~~~

or Windows MSVC:
~~~~
nmake 
~~~~

or by Jom:
~~~~
jom 
~~~~

### Supported compilers

**qvge** has been built with:
- Microsoft Visual Studio 2015 (Community Edition)
- MinGW 5.3
- GCC 4.8 & GCC 5.3 (Linux)
- GCC 6.4.0 (Cygwin) 
- Clang C++ (FreeBSD)

Hopefully it will be compiled with others compilers, too. If not please do not hesitate to provide description of the issue.

### Supported OSes

**qvge** has been tested on Microsoft Windows 10 and several Linux (Mint, Mageia etc). Theoretically it should run on (almost) any OS which have Qt 5.x installed.
It can be compiled & started under Cygwin as well.

### Credits

**qvge** uses following 3rd party components:

- [Qt](https://www.qt.io) (c) Qt Company 
- [Qt property browser](https://github.com/qtproject/qt-solutions) (c) Qt Company 
- [QSint widgets library](https://sourceforge.net/projects/qsint) (c) Sintegrial Technologies
- [OGDF - Open Graph Drawing Framework](http://www.ogdf.net)
- [Inkscape](https://inkscape.org) (SVG icons)
- [read_proc library](https://daknuett.eu/personal) (c) Daniel Knuettel

### Extenal Links

**qvge** at [![Download qvge](https://sourceforge.net/sflogo.php?type=13&group_id=2914953)](https://sourceforge.net/p/qvge/)

**qvge** at [OpenDesktop.org](https://www.opendesktop.org/p/1211078)

