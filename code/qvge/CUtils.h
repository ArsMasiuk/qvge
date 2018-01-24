/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QString>
#include <QVariant>
#include <QPointF>
#include <QLineF>
#include <QPolygonF>


class CUtils
{
public:
	static QString variantToText(const QVariant& v);
    static QVariant textToVariant(const QString& text, int type = QVariant::String);

	template<class X>
	static void insertUnique(X& dest, const X& from);

	static QPointF closestIntersection(const QLineF& line, const QPolygonF& with);
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
