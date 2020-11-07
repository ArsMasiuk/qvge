/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <qvgeio/CGraphBase.h>


class CFormatDOT
{
public:
	bool load(const QString& fileName, Graph& graph, QString* lastError = nullptr) const;
	bool save(const QString& fileName, Graph& graph, QString* lastError = nullptr) const;
};



