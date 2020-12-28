/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CSIMPLEUNDOMANAGER_H
#define CSIMPLEUNDOMANAGER_H

#include "IUndoManager.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

class CEditorScene;


class CSimpleUndoManager : public IUndoManager
{
public:
	CSimpleUndoManager(CEditorScene &scene);

	// reimp
	virtual void reset();
	virtual void addState();
	virtual void revertState();
	virtual void undo();
	virtual void redo();
	virtual int availableUndoCount() const;
	virtual int availableRedoCount() const;

private:
	CEditorScene *m_scene;
	QList<QByteArray> m_stateStack;
	int m_stackIndex;
};

#endif // CUNDOMANAGER_H
