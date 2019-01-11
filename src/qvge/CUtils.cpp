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
	{
		QFont f;
		f.fromString(text);
		return f;
	}

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


Qt::PenStyle CUtils::textToPenStyle(const QString& text, Qt::PenStyle def)
{
	static QMap<QString, Qt::PenStyle> s_penStyles =
	{	{ "none", Qt::NoPen }, 
		{ "solid", Qt::SolidLine }, 
		{ "dashed", Qt::DashLine },
		{ "dotted", Qt::DotLine },
		{ "dashdot", Qt::DashDotLine },
		{ "dashdotdot", Qt::DashDotDotLine } 
	};

	if (s_penStyles.contains(text))
		return s_penStyles[text];
	else
        return def;
}


QString CUtils::penStyleToText(int style)
{
	switch (style)
	{
		case Qt::SolidLine:         return QStringLiteral("solid");
		case Qt::DashLine:          return QStringLiteral("dashed");
		case Qt::DotLine:           return QStringLiteral("dotted");
		case Qt::DashDotLine:       return QStringLiteral("dashdot");
		case Qt::DashDotDotLine:    return QStringLiteral("dashdotdot");
		default:					return QStringLiteral("none");
	}
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

