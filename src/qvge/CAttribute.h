/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QDataStream>
#include <QList>
#include <QIcon>

#include <qvgeio/CGraphBase.h>


// attribute class

struct CAttribute: public AttrInfo
{
	CAttribute();
	CAttribute(const QByteArray& attrId, const QString& attrName = QString());
    CAttribute(const QByteArray& attrId, const QString& attrName, const QVariant& defaultValue, const int attrFlags = ATTR_NONE);

	/*const*/ int flags = ATTR_NONE;

	bool isUserDefined() const
	{
		return !(flags & ATTR_FIXED);
	}

	// serialization 
	virtual bool storeTo(QDataStream& out, quint64 version64) const;
	virtual bool restoreFrom(QDataStream& out, quint64 version64);

	// dummy member
	QByteArray classId;
};

typedef QMap<QByteArray, CAttribute> AttributesMap;		// <attr.id, attr>

typedef QMap<QByteArray, AttributesMap> ClassAttributesMap;		// <class.id, attr.map>


// attribute constrains

typedef QPair<QByteArray, QByteArray> ClassAttrIndex;

struct CAttributeConstrains;
typedef QMap<ClassAttrIndex, CAttributeConstrains*> AttributeConstrainsMap;

struct CAttributeConstrains
{
	virtual ~CAttributeConstrains();
};


// integer values

struct CIntegerConstrains : public CAttributeConstrains
{
	CIntegerConstrains(int minV = INT_MIN, int maxV = INT_MAX)
	{
		minValue = minV;
		maxValue = maxV;
	}

	CIntegerConstrains(CAttributeConstrains *ptr = NULL)
	{
		CIntegerConstrains *dptr = dynamic_cast<CIntegerConstrains*>(ptr);
		if (dptr)
		{
			minValue = dptr->minValue;
			maxValue = dptr->maxValue;
		}
	}

	int minValue = INT_MIN;
	int maxValue = INT_MAX;
};


// double values

struct CDoubleConstrains : public CAttributeConstrains
{
	CDoubleConstrains(double minV, double maxV, int decs = 4)
	{
		minValue = minV;
		maxValue = maxV;
		decPoints = decs;
	}

	CDoubleConstrains(CAttributeConstrains *ptr = NULL)
	{
		CDoubleConstrains *dptr = dynamic_cast<CDoubleConstrains*>(ptr);
		if (dptr) 
		{
			minValue = dptr->minValue;
			maxValue = dptr->maxValue;
			decPoints = dptr->decPoints;
		}
	}

	double minValue = std::numeric_limits<double>::lowest();
	double maxValue = std::numeric_limits<double>::max();
	int decPoints = 4;
};


// list of id items

typedef QList<QIcon> IconsList;


struct CAttributeConstrainsListBase : public CAttributeConstrains
{
	QStringList names;
	IconsList icons;

	// convenience method to conform property browser API
	QMap<int, QIcon> iconsAsMap() const
	{
		QMap<int, QIcon> result;

		for (int i = 0; i < icons.size(); ++i)
			result[i] = icons[i];

		return result;
	}
};


struct CAttributeConstrainsList: public CAttributeConstrainsListBase
{
	QStringList ids;
};


struct CAttributeConstrainsEnum : public CAttributeConstrainsListBase
{
	QList<int> ids;
};

