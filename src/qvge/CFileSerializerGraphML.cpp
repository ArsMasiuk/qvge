/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

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

bool CFileSerializerGraphML::load(const QString& fileName, CEditorScene& scene) const
{
	CFormatGraphML graphML;
	Graph graphModel;
	if (graphML.load(fileName, graphModel))
	{
		return scene.fromGraph(graphModel);
	}
	else
		return false;
}
