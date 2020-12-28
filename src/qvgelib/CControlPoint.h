/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QGraphicsRectItem>

#include "IContextMenuProvider.h"


class CItem;


class CControlPoint : public QObject, public QGraphicsRectItem, public IContextMenuProvider
{
	Q_OBJECT
public:
	typedef QGraphicsRectItem Shape;

	explicit CControlPoint(CItem *parent);
	virtual ~CControlPoint() {}

	// menu
	virtual bool populateMenu(QMenu& menu, const QList<QGraphicsItem*>& selectedItems);

protected Q_SLOTS:
	void onActionDelete();

protected:
	// reimp 
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);

	CItem *m_parentItem;
};

