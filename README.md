## Qt Visual Graph Editor
[![Download qvge](https://img.shields.io/sourceforge/dt/qvge.svg?style=for-the-badge)](https://sourceforge.net/projects/qvge/files/latest/download) 
[![Github All Releases](https://img.shields.io/github/downloads/ArsMasiuk/qvge/total.svg?style=for-the-badge)](https://github.com/ArsMasiuk/qvge/releases/latest)
![GitHub release](https://img.shields.io/github/release/ArsMasiuk/qvge.svg?style=for-the-badge)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg?style=for-the-badge)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=Z35EHHJ3729GG&source=url)

**QVGE** is a multiplatform graph editor written in C++/Qt. Its main goal is to make possible visually edit two-dimensional graphs
in a simple and intuitive way.

![Screenshot1](https://user-images.githubusercontent.com/19762856/89233004-f7846780-d5e8-11ea-8a18-ba395794e1d4.PNG)

Please note that **QVGE** is not a replacement for such a software like Gephi, Inkscape, yEd, Dia and so on. It is neither a tool for "big data analysis" nor a math application. It is really just a simple graph editor with some advanced features (i.e. GraphViz integration).

### Support

Since **QVGE** is a free software, it is developed in the free time on my own costs only. If you like the software and wish to support its further development, you could make a small donation using the button below:

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=Z35EHHJ3729GG&source=url)

Thank you!

### Main Features

- Easy creation and parameterising of small-sized till middle-sized graphs (1000+ nodes/edges)
- Common visual properties of nodes and edges: shapes, sizes, colors, labels etc.
- Directed, undirected and mixed graphs
- Node ports
- Straight and polygonal edges
- Custom (user-defined) attributes of graphs and their elements
- Dynamically maintained list of commutations between nodes
- Search among the graph elements and their attributes
- Windows: portable mode (no installation required)
- Auto-layout of graphs via [GraphViz](https://graphviz.org/) engines:
  - dot
  - neato
  - fdp
  - sfdp
  - circo
- Export of graphs into:
  - PDF
  - SVG
  - various image formats (BMP, PNG, JPG, TIFF etc.)
- Graph file formats supported:
  - XGR (native graph persistence format)
  - GEXF (read/write of common subset, except clusters and dynamic properties)
  - GraphML (read/write)
  - GML (read via OGDF, write via QVGE)
  - [GraphViz DOT](https://graphviz.org/) (read via GraphViz/boost/OGDF, write via QVGE)

### Some users' feedback

> "This graph editor is very promising for every day modeling." 

> "Qt Visual Graph Editor is a fairly straightforward, open-source tool that enables users to design relatively simple graphs for their projects. It comes with a decent set of features and is very intuitive."

> "It seems to me that my development have become more efficient after when I began using QVGE. This is much more useful than UML, because that I don't have to change sheets and to remember several usages and I can draw graphs swiftly."

> "Lightweight, multi-platform graph editor that allows users to edit two-dimensional graphs in a quick and intuitive way, as an alternative to more complex software."

> "Its user experience is very good. It's because how to operate is sophisticated so intuitive and very simple. A user can entirely concentrate on essence of content the whole time. Because of simpleness, the content is not noisy and easy to understand, and usable much generally to design, refactor and output a structure such as a organization, a software, logic, routes and all other relationships without learning usage separately."

> "I have been using QVGE for a few hours a week for over a year. It made my life better."

### Installation

Prebuild Windows binaries can be loaded from here:

[![Github All Releases](https://img.shields.io/github/downloads/ArsMasiuk/qvge/total.svg?style=for-the-badge)](https://github.com/ArsMasiuk/qvge/releases/latest)

Or you can get QVGE's sources and build them by yourself. In this case you need to have installed Qt 5.x toolkit and corresponding C++ compiler with C++11 support. QVGE uses native Qt build system (main project file is qvgeapp.pro) so it should look like the following for Windows and common Linux distributions:

~~~
cd <directory-with-qvgeapp.pro>
qmake -r
~~~

or, for Fedora-based distributions:
~~~
cd <directory-with-qvgeapp.pro>
qmake-qt5 -r
~~~

or, for macOS (with Homebrew and XCode Command Line Tools):
~~~
brew install qt@5
cd <directory-with-qvgeapp.pro>
/usr/local/Cellar/qt@5/5.*/bin/qmake -r
~~~

(Note: your path to Cellar might differ depending on your homebrew install)

Then run corresponding 'make' command to build the application: 

Linux GCC, macOS Clang:
~~~
make
~~~

or Windows MinGW:
~~~
mingw32-make
~~~

or Windows MSVC:
~~~
nmake
~~~

or by Jom:
~~~
jom
~~~

Linux only: once the application is compiled, install it using
~~~
sudo make install
~~~

### Enabling OGDF

Integration with [OGDF](https://ogdf.uos.de/) enables auto-creation and auto-layout of graphs using following algorithms:
  - Linear
  - Balloon
  - Circular
  - FMMM
  - Planar
  - Sugiyama
  - Davidson-Harel
  
#### WARNING: bundled OGDF support has been removed since QVGE 0.6.1 due to memory access issues

In order to build **QVGE** with external OGDF support (installed in your system):
before running qmake, open the file `src/config.pri` and uncomment the following option:

~~~
CONFIG += USE_EXTERNAL_OGDF
~~~

This will allow to link against OGDF library which is already present in the system.
In this case, the following lines have to be changed accordingly:

~~~
USE_EXTERNAL_OGDF{
	...
	
	# system-specific OGDF setup
	OGDF_LIB_NAME = <name of installed OGDF library>
	OGDF_LIB_PATH = <path to installed OGDF library file>
	OGDF_INCLUDE_PATH = <path to headers of the installed OGDF library>
}
~~~

Then run qmake + make as described in the step before. 

### Supported compilers

Recent version of **QVGE** has been built with:
- Microsoft Visual Studio 2017 (Community Edition)
- MinGW 7.3
- GCC 7.5 (Linux)
- GCC 6.4.0 (Cygwin)
- Clang C++ (FreeBSD, macOS)

Hopefully it can also be compiled with others compilers. If not please do not hesitate to provide description of the issue.

### Supported OS

**QVGE** has been tested on Microsoft Windows 10, several Linux distributions (Mint, Mageia, Fedora etc) and macOS 11.2 Big Sur. Theoretically it should run on (almost) any OS which have Qt 5.x installed.

**QVGE** can be compiled & run under Cygwin.

### Supported Qt

**QVGE** has been tested with Qt 5.9-5.14. But it should work with any newer 5.x version too.

### Credits

**QVGE** uses following 3rd party components:

- [Qt](https://www.qt.io) - licensed under LGPLv3
- [Qt property browser](https://github.com/qtproject/qt-solutions) - licensed under BSD
- [QProcessInfo](https://github.com/baldurk/qprocessinfo) - licensed under BSD
- [QSint widgets library](https://sourceforge.net/projects/qsint) - licensed under LGPLv3
- SVG icons from [Inkscape](https://inkscape.org) - licensed under GPL (more details here: https://inkscape.org/de/ueber/lizenzierung/)
- [GraphViz](https://graphviz.org) - licensed under CPLv1.0 (more details here: https://graphviz.org/license/)

Special thanks to:

- Dr. prof. [Vladimir A. Svjatnyj](https://wiki.donntu.edu.ua/view/%D0%A1%D0%B2%D1%8F%D1%82%D0%BD%D0%B8%D0%B9_%D0%92%D0%BE%D0%BB%D0%BE%D0%B4%D0%B8%D0%BC%D0%B8%D1%80_%D0%90%D0%BD%D0%B4%D1%80%D1%96%D0%B9%D0%BE%D0%B2%D0%B8%D1%87), head of [computer engineering chair](https://donntu.edu.ua/knt/kafedra-ki) at [DonNTU](https://donntu.edu.ua/en/donntu2020) and my scientific supervisor
- [Tatsuro Ueda](https://github.com/weed), founder of [Feel Physics](https://feel-physics.jp), for comrehensive testing, feedback and suggestions

### External Links

**QVGE** at [![Download qvge](https://sourceforge.net/sflogo.php?type=14&group_id=2914953)](https://sourceforge.net/p/qvge/) 

**QVGE** at [Softpedia.com](https://www.softpedia.com/get/Multimedia/Graphic/Graphic-Others/Qt-Visual-Graph-Editor.shtml) <a target="_blank" href="https://www.softpedia.com/get/Multimedia/Graphic/Graphic-Others/Qt-Visual-Graph-Editor.shtml"><img src="https://cdnssl.softpedia.com/_img/sp100free.png?1"/></a>

**QVGE** at [SoftX64.com](https://www.softx64.com/windows/qt-visual-graph-editor.html) <a target="_blank" href="https://www.softx64.com/windows/qt-visual-graph-editor.html" title="Qt Visual Graph Editor review"><img src="https://www.softx64.com/softx64-review.png" alt="Qt Visual Graph Editor review" /></a>

**QVGE** at [software-file.com](http://www.software-file.com/Qt_Visual_Graph_Editor-sfs-472477.html) <a href="http://www.software-file.com/Qt_Visual_Graph_Editor-sfs-472477.html" target="_blank"><img src="http://www.software-file.com/images/fivestar.png" alt="Qt Visual Graph Editor on Software-File.com" border=0></a>

**QVGE** at <a href="https://www.producthunt.com/posts/qt-visual-graph-editor?utm_source=badge-featured&utm_medium=badge&utm_souce=badge-qt-visual-graph-editor" target="_blank"><img src="https://api.producthunt.com/widgets/embed-image/v1/featured.svg?post_id=217842&theme=dark" alt="Qt Visual Graph Editor - Software to visually create and manipulate graphs | Product Hunt Embed" style="width: 250px; height: 54px;" width="250px" height="54px" /></a>


