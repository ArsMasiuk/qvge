/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include "CSimpleUndoManager.h"
#include "CEditorScene.h"

#include <QDataStream>


CSimpleUndoManager::CSimpleUndoManager(CEditorScene & scene)
	: m_scene(&scene),
	m_stackIndex(-1)
{
}

void CSimpleUndoManager::reset()
{
	m_stackIndex = -1;
	m_stateStack.clear();
}

void CSimpleUndoManager::addState()
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

void CSimpleUndoManager::revertState()
{
	if (availableUndoCount())
	{
		QByteArray &compressedSnap = m_stateStack[m_stackIndex];
		QByteArray snap = qUncompress(compressedSnap);
		QDataStream ds(&snap, QIODevice::ReadOnly);
		m_scene->restoreFrom(ds, false);
	}
}

void CSimpleUndoManager::undo()
{
	if (availableUndoCount())
	{
		QByteArray &compressedSnap = m_stateStack[--m_stackIndex];
		QByteArray snap = qUncompress(compressedSnap);
		QDataStream ds(&snap, QIODevice::ReadOnly);
		m_scene->restoreFrom(ds, false);
	}
}

void CSimpleUndoManager::redo()
{
	if (availableRedoCount())
	{
		QByteArray &compressedSnap = m_stateStack[++m_stackIndex];
		QByteArray snap = qUncompress(compressedSnap);
		QDataStream ds(&snap, QIODevice::ReadOnly);
		m_scene->restoreFrom(ds, false);
	}
}

int CSimpleUndoManager::availableUndoCount() const
{
	return (m_stackIndex > 0);
}

int CSimpleUndoManager::availableRedoCount() const
{
	return (m_stackIndex >= 0) && (m_stackIndex < m_stateStack.size() - 1);
}
