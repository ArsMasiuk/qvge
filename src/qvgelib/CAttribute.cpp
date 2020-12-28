/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CAttribute.h"


// attributes

CAttribute::CAttribute():
	flags(ATTR_NONE)
{
	valueType = QVariant::String;
}


CAttribute::CAttribute(const QByteArray& attrId, const QString& attrName) : 
	flags(ATTR_NODEFAULT)
{
	id = attrId;
	name = attrName;
	if (name.isEmpty()) name = id;
	
	valueType = QVariant::String;
}


CAttribute::CAttribute(
	const QByteArray& attrId, 
	const QString& attrName, 
	const QVariant& defaultValue_,
	const int attrFlags_) :
	flags(attrFlags_)
{
	id = attrId;
	name = attrName.isEmpty() ? id : attrName;
	
	valueType = defaultValue_.type();
	defaultValue = (flags & ATTR_NODEFAULT) ? QVariant() : defaultValue_;
}


bool CAttribute::storeTo(QDataStream& out, quint64 /*version64*/) const
{
    out << id << name << defaultValue << true << true << valueType;

	return true;
}


bool CAttribute::restoreFrom(QDataStream& out, quint64 version64)
{
	out >> id;

    static bool dummy;

	if (version64 < 6)
		out >> classId;	// dummy value

    out >> name >> defaultValue >> dummy >> dummy;
	//attrFlags = ATTR_USER;

	// size must be converted
	if (version64 < 7)
	{
		if (id == "size")
			defaultValue = QSizeF(defaultValue.toDouble(), defaultValue.toDouble());
	}

	if (version64 < 10)
		valueType = defaultValue.type();
	else
		out >> valueType;

	return true;
}


// attribute constrains

CAttributeConstrains::~CAttributeConstrains()
{
	// dummy
}

