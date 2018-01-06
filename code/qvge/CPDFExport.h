/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2017 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QString>

#include "CEditorScene.h"


class CPDFExport
{
public:
	static bool write(/*const*/ CEditorScene &scene, const QString &startPath = "");
};
