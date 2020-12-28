/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QGraphicsRectItem>

#include "IInteractive.h"


class CNode;


class CNodePort : public QGraphicsRectItem, public IInteractive
{
public:
	typedef QGraphicsRectItem Shape;

	explicit CNodePort(CNode *node, const QByteArray& portId = QByteArray(), int align = 0, double xoff = 0, double yoff = 0);
	virtual ~CNodePort();

	CNode* getNode() const				{ return m_node;	}
	const QByteArray& getId() const		{ return m_id;		}
	int getAlign() const				{ return m_align;	}
	double getX() const					{ return m_xoff;	}
	double getY() const					{ return m_yoff;	}
	QColor getColor() const;

	void setId(const QByteArray& portId);
	void setAlign(int newAlign);
	void setOffset(double xoff, double yoff);
	void setColor(const QColor& color);

	void copyDataFrom(const CNodePort &port);

	// serialization 
	virtual bool storeTo(QDataStream& out, quint64 version64) const;
	//virtual bool restoreFrom(QDataStream& out, quint64 version64);

	// callbacks
	void onParentDeleted();
	virtual void onParentGeometryChanged();

	virtual void onClick(QGraphicsSceneMouseEvent* /*mouseEvent*/);

	virtual ItemDragTestResult acceptDragFromItem(QGraphicsItem* draggedItem);
	virtual void leaveDragFromItem(QGraphicsItem* draggedItem);

	//virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	//virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

protected:
	CNode *m_node = nullptr;

	QByteArray m_id;
	int m_align;
	double m_xoff, m_yoff;
};

