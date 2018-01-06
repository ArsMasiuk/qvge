/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "CConnection.h"


class CDirectConnection : public CConnection
{
public:
	typedef CConnection Super;

	CDirectConnection(QGraphicsItem *parent = Q_NULLPTR);

	void setBendFactor(int bf);

	// reimp
	static QByteArray factoryId() { return "CDirectConnection"; }
	virtual QByteArray typeId() const { return this->factoryId(); }
	virtual QByteArray classId() const { return "edge"; }
	virtual QByteArray superClassId() const { return Super::classId(); }

	virtual CItem* create() const { return new CDirectConnection(parentItem()); }
	CConnection* clone();

protected:
	// reimp
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);
	virtual void updateLabelPosition();

	// callbacks 
	virtual void onParentGeometryChanged();

protected:
	int m_bendFactor;
	QPointF m_controlPoint, m_controlPos;
};
