/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QGraphicsRectItem>


class CItem;


class CControlPoint : public QObject, public QGraphicsRectItem
{
	Q_OBJECT

public:
	typedef QGraphicsRectItem Shape;

	explicit CControlPoint(CItem *parent);
	virtual ~CControlPoint() {}

protected Q_SLOTS:
	void onActionDelete();

protected:
	// reimp 
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

	CItem *m_parentItem;
};

