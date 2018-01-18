## Qt Visual Graph Editor

qvge is a multiplatform graph editor written in C++/Qt. Its main goal is to make possible visually edit two-dimensional graphs
in a simple and intuitive way.

Please note that qvge is not a replacement for such a software like Gephi, Graphvis, Dot, yEd, Dia and so on. It is neither a tool for "big data analysis" nor a math application. It is really just a simple graph editor :)

### Main Features

- Easy creation and parametrising of small till middle-sized graphs
- Directed, undirected and mixed graphs supported
- Loading GraphML and GEXF files (as well as native XGR format)
- Export into PDF and popular image formats
- List of commutations between nodes

### Installation

Prebuild Windows binaries can be loaded from [SourceForge](https://sourceforge.net/projects/qvge/files/).

Or you can get qvge's sources and build them by yourself. In this case you need to have installed Qt 5.x toolkit and corresponding C++ compiler with C++11 support. qvge uses native Qt build system (main project file is qvgeapp.pro) so it should look like:

*cd* <directory-with-qvgeapp.pro>

*qmake*

*make* (linux) or *nmake* (windows)

### Supported OSes

qvge has been tested on Microsoft Windows 10 and several Linux OSes. Theoretically it should run on (almost) any OS which have Qt 5.x installed.


