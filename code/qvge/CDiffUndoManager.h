/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "IUndoManager.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

class CEditorScene;


class CDiffUndoManager : public IUndoManager
{
public:
	CDiffUndoManager(CEditorScene &scene);

	// reimp
	virtual void reset();
	virtual void addState();
	virtual void undo();
	virtual void redo();
	virtual int availableUndoCount() const;
	virtual int availableRedoCount() const;

private:
	CEditorScene *m_scene;
	QList<QByteArray> m_stateStack;
	int m_stackIndex;
};
