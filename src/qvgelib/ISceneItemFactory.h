/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QtCore/QByteArray>

class CItem;
class CEditorScene;


/**
	Common interface to scene item creation filters.
*/
class ISceneItemFactory
{
public:
	virtual CItem* createItemOfType(const QByteArray& typeId, const CEditorScene& scene) const = 0;
};
