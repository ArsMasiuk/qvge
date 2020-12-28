/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QMenu>
#include <QList>
#include <QGraphicsItem>


class IContextMenuProvider
{
public:
	virtual bool populateMenu(QMenu& menu, const QList<QGraphicsItem*>& selectedItems) = 0;
};
