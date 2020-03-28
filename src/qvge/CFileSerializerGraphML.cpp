/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerGraphML.h"
#include "CAttribute.h"
#include "CNode.h"
#include "CDirectEdge.h"

#include <qvgeio/CFormatGraphML.h>

#include <QFile>
#include <QDebug>


// reimp

bool CFileSerializerGraphML::load(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	CFormatGraphML graphML;
	Graph graphModel;

	if (graphML.load(fileName, graphModel, lastError))
		return scene.fromGraph(graphModel);
	else
		return false;
}


bool CFileSerializerGraphML::save(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	CFormatGraphML graphML;
	Graph graphModel;

	if (scene.toGraph(graphModel))
		return graphML.save(fileName, graphModel, lastError);
	else
		return false;
}

