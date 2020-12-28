/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QtCore/QString>

class CEditorScene;


/**
	Common interface to file exporters.
*/
class IFileExport
{
public:
	virtual bool save(const QString& fileName, const CEditorScene& scene) const = 0;
};
