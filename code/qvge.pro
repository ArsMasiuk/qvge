# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.


TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += qtpropertybrowser
qtpropertybrowser.file = $$PWD/3rdParty/qtpropertybrowser/qtpropertybrowser.pro

SUBDIRS += ogdf
ogdf.file = $$PWD/3rdParty/ogdf/ogdf.pro

SUBDIRS += qsint
qsint.file = $$PWD/3rdParty/qsint-widgets/qsint-widgets.pro

SUBDIRS += src
src.file = $$PWD/src/src.pro
