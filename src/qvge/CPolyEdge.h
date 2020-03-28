/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "CDirectEdge.h"


class CControlPoint;


class CPolyEdge : public CDirectEdge
{
public:
	typedef CDirectEdge Super;

	CPolyEdge(QGraphicsItem *parent = Q_NULLPTR);

	void setPoints(const QList<QPointF> &points);
	bool insertPointAt(const QPointF &pos);

	// reimp
	static QByteArray factoryId() { return "CPolyEdge"; }
	virtual QByteArray typeId() const { return this->factoryId(); }
	virtual QByteArray classId() const { return "polyedge"; }
	virtual QByteArray superClassId() const { return Super::classId(); }

	virtual CItem* create() const { return new CPolyEdge(parentItem()); }
	CEdge* clone();

	virtual void reverse();

	// serialization 
	virtual bool storeTo(QDataStream& out, quint64 version64) const;
	virtual bool restoreFrom(QDataStream& out, quint64 version64);

	// mousing
	virtual bool onDoubleClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos);
	virtual void onControlPointMoved(CControlPoint* controlPoint, const QPointF& pos);

	// deleting
	virtual void onControlPointDelete(CControlPoint* controlPoint);

	// selection
	virtual void onItemMoved(const QPointF& delta);
	virtual void onItemSelected(bool state);

protected:
	// reimp
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

	// callbacks 
	virtual void onParentGeometryChanged();

private:
	void dropControlPoints();
	void createControlPoints();
	void updateShapeFromPoints();

private:
	// data model
	QList<QPointF> m_polyPoints;

	// visual control points
	QList<CControlPoint*> m_controlPoints;
};
