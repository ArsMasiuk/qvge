/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerPlainDOT.h"
#include "CAttribute.h"
#include "CNode.h"
#include "CDirectEdge.h"

#include <qvgeio/CFormatPlainDOT.h>

#include <QFile>
#include <QDebug>


// reimp

bool CFileSerializerPlainDOT::load(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	CFormatPlainDOT graphFormat;
	Graph graphModel;

	if (graphFormat.load(fileName, graphModel, lastError))
		return scene.fromGraph(graphModel);
	else
		return false;
}


bool CFileSerializerPlainDOT::save(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	CFormatPlainDOT graphFormat;
	Graph graphModel;

	if (scene.toGraph(graphModel))
		return graphFormat.save(fileName, graphModel, lastError);
	else
		return false;
}

