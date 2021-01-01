# This file is a part of
# QVGE - Qt Visual Graph Editor
#
# (c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)
#
# It can be used freely, maintaining the information above.


TEMPLATE = subdirs
CONFIG += ordered

include($$PWD/config.pri)

SUBDIRS += qtpropertybrowser
qtpropertybrowser.file = $$PWD/3rdParty/qtpropertybrowser/qtpropertybrowser.pro

SUBDIRS += qsint
qsint.file = $$PWD/3rdParty/qsint-widgets/qsint-widgets.pro

SUBDIRS += qvgeio
qvgeio.file = $$PWD/qvgeio/qvgeio.pro

SUBDIRS += qvgelib
qvgelib.file = $$PWD/qvgelib/qvgelib.pro

SUBDIRS += commonui
commonui.file = $$PWD/commonui/commonui.pro

SUBDIRS += qvgeioui
qvgeioui.file = $$PWD/qvgeioui/qvgeioui.pro

SUBDIRS += qvgeui
qvgeui.file = $$PWD/qvgeui/qvgeui.pro

SUBDIRS += qvge
qvge.file = $$PWD/qvge/qvge.pro

SUBDIRS += qdot
qdot.file= $$PWD/qdot/qdot.pro
