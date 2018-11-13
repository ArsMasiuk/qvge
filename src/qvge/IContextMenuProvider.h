#pragma once

#include <QMenu>
#include <QList>
#include <QGraphicsItem>


class IContextMenuProvider
{
public:
	virtual bool populateMenu(QMenu& menu, const QList<QGraphicsItem*>& selectedItems) = 0;
};
