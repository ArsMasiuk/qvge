Name:		qvge
Version:	0.6.1
Release:	1
Summary:	Qt Visual Graph Editor
License:	MIT
URL:		https://github.com/ArsMasiuk/qvge
BuildRequires:	qt5-devel >= 5.9.0

%description
QVGE is a multiplatform graph editor written in C++/Qt. Its main goal is to make possible visually edit two-dimensional graphs in a simple and intuitive way.

QVGE is designed for viewing and manipulating small till middle-sized graphs. It supports several formats (GraphML, GEXF, GML, GraphViz/Dot).

%install
mkdir -p %{buildroot}/usr/local/bin
cp /usr/local/bin/qvgeapp %buildroot/usr/local/bin

mkdir -p %{buildroot}/usr/local/share/applications
cp /usr/local/share/applications/qvge.desktop %buildroot/usr/local/share/applications

mkdir -p %{buildroot}/usr/local/share/pixmaps
cp /usr/local/share/pixmaps/qvge.png %buildroot/usr/local/share/pixmaps

mkdir -p %{buildroot}/usr/local/share/appdata
cp /usr/local/share/appdata/qvge.appdata.xml %buildroot/usr/local/share/appdata

mkdir -p %{buildroot}/usr/local/share/mime/packages
cp /usr/local/share/mime/packages/application-xgr.xml %buildroot/usr/local/share/mime/packages


%files
/usr/local/bin/qvgeapp
/usr/local/share/applications/qvge.desktop
/usr/local/share/pixmaps/qvge.png
/usr/local/share/appdata/qvge.appdata.xml
/usr/local/share/mime/packages/application-xgr.xml

%changelog
* Mon Nov 11 2020 Ars Masiuk <ars.masiuk@gmail.com> 0.6.1
- Tighter integration with GraphViz
- Various bugfixes and minor improvements


