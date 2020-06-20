/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "CEdge.h"


class CDirectEdge : public CEdge
{
public:
	typedef CEdge Super;

	CDirectEdge(QGraphicsItem *parent = Q_NULLPTR);

	void setBendFactor(int bf);

	// reimp
	static QByteArray factoryId() { return "CDirectEdge"; }
	virtual QByteArray typeId() const { return this->factoryId(); }
	virtual QByteArray classId() const { return "edge"; }
	virtual QByteArray superClassId() const { return Super::classId(); }

	virtual CItem* create() const { return new CDirectEdge(parentItem()); }
	CEdge* clone();

	virtual QPointF getLabelCenter() const {
		return m_controlPoint;
	}

protected:
	// reimp
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);
	virtual void updateLabelPosition();

	// callbacks 
	virtual void onParentGeometryChanged();

private:
	void createCurvedPath(bool intersected, 
		const QLineF& shortLine, const QLineF& fullLine,
		const QPointF& p1, const QPointF& lp, const QPointF& rp, const QPointF& p2,
		double arrowSize);

protected:
	int m_bendFactor;
	QPointF m_controlPoint, m_controlPos;
};
