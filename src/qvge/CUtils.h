/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QPointF>
#include <QLineF>
#include <QPolygonF>
#include <QMap>
#include <QSet>
#include <QPen>
#include <QGraphicsItem>


class CUtils
{
public:
	static QString variantToText(const QVariant& v, int type = -1);
    static QVariant textToVariant(const QString& text, int type = QVariant::String);

    static Qt::PenStyle textToPenStyle(const QString& text, Qt::PenStyle def = Qt::NoPen);
	static QString penStyleToText(int style);

	static QString visToString(const QSet<QByteArray>& visIds);
	static QSet<QByteArray> visFromString(const QString& text);
	static QStringList byteArraySetToStringList(const QSet<QByteArray>& visIds);

	template<class X>
	static void insertUnique(X& dest, const X& from);

	static QPointF closestIntersection(const QLineF& line, const QPolygonF& with);

	static QString cutLastSuffix(const QString& fileName);

	static QRectF getBoundingRect(const QList<QGraphicsItem*>& items);

	static QLineF extendLine(const QLineF& line, float fromStart, float fromEnd);
};


template<class X>
void CUtils::insertUnique(X& dest, const X& from)
{
	for (auto it = from.constBegin(); it != from.constEnd(); ++it)
	{
		if (!dest.contains(it.key()))
			dest[it.key()] = it.value();
	}
}
