/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include "CDiffUndoManager.h"
#include "CEditorScene.h"

#include <QDataStream>


CDiffUndoManager::CDiffUndoManager(CEditorScene & scene)
    : m_scene(&scene)
{
}

void CDiffUndoManager::reset()
{
	m_redoStack.clear();
	m_undoStack.clear();
	m_redoStackTemp.clear();
	m_undoStackTemp.clear();
	m_lastState.clear();
}

void CDiffUndoManager::addState()
{
	// drop temp stacks
	m_redoStack.clear();
	m_undoStackTemp.clear();

	// serialize & compress
	QByteArray snap;
	QDataStream ds(&snap, QIODevice::WriteOnly);
	m_scene->storeTo(ds, true);

	// check if 1st store
	if (m_lastState.isEmpty() && m_undoStack.isEmpty() && m_redoStack.isEmpty())
	{
		m_lastState = snap;
		return;
	}

	// push states into stacks
	int leftDiffIndex = 0;
	int len = qMin(snap.size(), m_lastState.size());
	while (leftDiffIndex < len && snap[leftDiffIndex] == m_lastState[leftDiffIndex])
		++leftDiffIndex;

	int rightDiffIndex1 = m_lastState.size() - 1;
	int rightDiffIndex2 = snap.size() - 1;
	while (rightDiffIndex1 >= 0 && rightDiffIndex2 >= 0 
		&& rightDiffIndex1 > leftDiffIndex && rightDiffIndex2 > leftDiffIndex 
		&& snap[rightDiffIndex2] == m_lastState[rightDiffIndex1])
			--rightDiffIndex1, --rightDiffIndex2;

	int len1 = rightDiffIndex1 - leftDiffIndex + 1;
	int len2 = rightDiffIndex2 - leftDiffIndex + 1;

	Command cUndo = { leftDiffIndex, len2, qCompress(m_lastState.mid(leftDiffIndex, len1)) };
	Command cRedo = { leftDiffIndex, len1, qCompress(snap.mid(leftDiffIndex, len2)) };

	m_undoStack << cUndo;
	m_redoStackTemp << cRedo;

	// write last state
	m_lastState = snap;
}

void CDiffUndoManager::revertState()
{
	QDataStream ds(&m_lastState, QIODevice::ReadOnly);
	m_scene->restoreFrom(ds, true);
}

void CDiffUndoManager::undo()
{
	if (availableUndoCount())
	{
		Command cUndo = m_undoStack.takeLast();
		m_lastState.replace(cUndo.index, cUndo.sizeToReplace, qUncompress(cUndo.data));
		QDataStream ds(&m_lastState, QIODevice::ReadOnly);
		m_scene->restoreFrom(ds, true);

		m_redoStack << m_redoStackTemp.takeLast();
		m_undoStackTemp << cUndo;
	}
}

void CDiffUndoManager::redo()
{
	if (availableRedoCount())
	{
		Command cRedo = m_redoStack.takeLast();
		m_lastState.replace(cRedo.index, cRedo.sizeToReplace, qUncompress(cRedo.data));
		QDataStream ds(&m_lastState, QIODevice::ReadOnly);
		m_scene->restoreFrom(ds, true);

		m_undoStack << m_undoStackTemp.takeLast();
		m_redoStackTemp << cRedo;
	}
}

int CDiffUndoManager::availableUndoCount() const
{
	return m_undoStack.size();
}

int CDiffUndoManager::availableRedoCount() const
{
	return m_redoStack.size();
}
