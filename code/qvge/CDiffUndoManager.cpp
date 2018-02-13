/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include "CDiffUndoManager.h"
#include "CEditorScene.h"

#include <QDataStream>


CDiffUndoManager::CDiffUndoManager(CEditorScene & scene)
	: m_scene(&scene),
	m_stackIndex(-1)
{
}

void CDiffUndoManager::reset()
{
	m_stackIndex = -1;
	m_stateStack.clear();
}

void CDiffUndoManager::addState()
{
	// serialize & compress
	QByteArray snap;
	QDataStream ds(&snap, QIODevice::WriteOnly);
	m_scene->storeTo(ds, false);
	QByteArray compressedSnap = qCompress(snap);

	// push state into stack
	if (m_stateStack.size() == ++m_stackIndex)
	{
		m_stateStack.append(compressedSnap);
	}
	else
	{
		while (m_stateStack.size() > m_stackIndex)
			m_stateStack.takeLast();

		m_stateStack.append(compressedSnap);
	}
}

void CDiffUndoManager::undo()
{
	if (availableUndoCount())
	{
		QByteArray &compressedSnap = m_stateStack[--m_stackIndex];
		QByteArray snap = qUncompress(compressedSnap);
		QDataStream ds(&snap, QIODevice::ReadOnly);
		m_scene->restoreFrom(ds, false);
	}
}

void CDiffUndoManager::redo()
{
	if (availableRedoCount())
	{
		QByteArray &compressedSnap = m_stateStack[++m_stackIndex];
		QByteArray snap = qUncompress(compressedSnap);
		QDataStream ds(&snap, QIODevice::ReadOnly);
		m_scene->restoreFrom(ds, false);
	}
}

int CDiffUndoManager::availableUndoCount() const
{
	return (m_stackIndex > 0);
}

int CDiffUndoManager::availableRedoCount() const
{
	return (m_stackIndex >= 0) && (m_stackIndex < m_stateStack.size() - 1);
}
