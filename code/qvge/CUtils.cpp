#include "CUtils.h"

#include <QPoint>
#include <QPointF>
#include <QColor>
#include <QFont>


QVariant CUtils::textToVariant(const QString& text, int type)
{
    switch (type)
    {
    case QVariant::Int:
        return text.toInt();

    case QVariant::Double:
        return text.toDouble();

	case QMetaType::Float:
		return text.toFloat();

    case QVariant::Bool:
        if (text.toLower() == "true")
            return true;
        else
            return false;

	case QVariant::Color:
		return QColor(text);

	case QVariant::Font:
		return QFont(text);

    default:
        return text;    // string
    }
}


QString CUtils::variantToText(const QVariant& v)
{
	switch (v.type())
	{
	case QVariant::Point:
		return QString("%1;%2").arg(v.toPoint().x()).arg(v.toPoint().y());

	case QVariant::PointF:
		return QString("%1;%2").arg(v.toPointF().x()).arg(v.toPointF().y());

	case QVariant::Size:
		return QString("%1:%2").arg(v.toSize().width()).arg(v.toSize().height());

	case QVariant::SizeF:
		return QString("%1:%2").arg(v.toSizeF().width()).arg(v.toSizeF().height());

	case QVariant::Bool:
		return v.toBool() ? "true" : "false";

	case QVariant::Double:
		return QString::number(v.toDouble(), 'f', 4);

	case QMetaType::Float:
		return QString::number(v.value<float>(), 'f', 4);


	//case QVariant::UInt:
	//	return QString::number(v.toUInt());

	//case QVariant::Int:
	//	return QString::number(v.toInt());

	//case QVariant::ULongLong:
	//	return QString::number(v.toULongLong());

	//case QVariant::LongLong:
	//	return QString::number(v.toLongLong());

	default:;
        return v.toString();
    }
}


int CUtils::textToPenStyle(const QString& text, int def)
{
	static QMap<QString, int> s_penStyles =
	{ { "none",0}, { "solid",1 }, { "dashed",2 },{ "dotted",3 } ,{ "dashdot",4 } ,{ "dashdotdot",5 } };

	if (s_penStyles.contains(text))
		return s_penStyles[text];
	else
        return def;
}


QPointF CUtils::closestIntersection(const QLineF& line, const QPolygonF& endPolygon)
{
	QPointF intersectPoint;

	QPointF p1 = endPolygon.first();
	QPointF p2;

	for (int i = 1; i < endPolygon.count(); ++i) 
	{
		p2 = endPolygon.at(i);
		QLineF polyLine = QLineF(p1, p2);
		QLineF::IntersectType intersectType = polyLine.intersect(line, &intersectPoint);
		if (intersectType == QLineF::BoundedIntersection)
			break;

		p1 = p2;
	}

	return intersectPoint;
}


QString CUtils::cutLastSuffix(const QString& fileName)
{
	int idx = fileName.lastIndexOf(".");
	if (idx < 0)
		return fileName;
	else
		return fileName.left(idx);
}

