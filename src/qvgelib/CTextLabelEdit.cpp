/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include "CTextLabelEdit.h"
#include "CItem.h"

#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QMenu>
#include <QKeyEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QTimer>
#include <QApplication>


CTextLabelEdit::CTextLabelEdit()
{
	setTextInteractionFlags(Qt::TextEditorInteraction);

	connect(document(), &QTextDocument::contentsChanged, this, &CTextLabelEdit::updateGeometry);
}

CTextLabelEdit::~CTextLabelEdit()
{
}


void CTextLabelEdit::updateGeometry()
{
	if (m_item)
	{
		QPointF center = m_item->getLabelCenter();
		double w = boundingRect().width();
		double h = boundingRect().height();
		setPos(center.x() - w/2, center.y() - h/2);
	}
}


bool CTextLabelEdit::onKeyPressed(CEditorScene& scene, QKeyEvent *keyEvent)
{
	return QGraphicsTextItem::sceneEvent(keyEvent);
}


bool CTextLabelEdit::onKeyReleased(CEditorScene& scene, QKeyEvent *keyEvent)
{
	if (keyEvent->matches(QKeySequence::Cancel) && !m_menuActive)				// Esc
	{
		finishEdit(true);
		return true;
	}

	return QGraphicsTextItem::sceneEvent(keyEvent);
}


bool CTextLabelEdit::sceneEvent(QEvent *event)
{
	if (event->type() == QEvent::FocusOut && !m_menuActive)
	{
		finishEdit(true);
		return true;
	}

	return QGraphicsTextItem::sceneEvent(event);
}


void CTextLabelEdit::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	event->accept();

	m_menuActive = true;

	QMenu menu;
	bool hasSelection = textCursor().hasSelection();

	auto cutAction = menu.addAction(QIcon(":/Icons/Cut"), tr("Cut"), this, [&]
	{
		static QKeyEvent ke(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier);
		QApplication::sendEvent(scene(), &ke);
	});
	cutAction->setEnabled(hasSelection);
	cutAction->setShortcut(QKeySequence::Cut);
	
	auto copyAction = menu.addAction(QIcon(":/Icons/Copy"), tr("Copy"), this, [&]
	{
		static QKeyEvent ke(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
		QApplication::sendEvent(scene(), &ke);
	});
	copyAction->setEnabled(hasSelection);
	copyAction->setShortcut(QKeySequence::Copy);
	
	auto pasteAction = menu.addAction(QIcon(":/Icons/Paste"), tr("Paste"), this, [&]
	{
		static QKeyEvent ke(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
		QApplication::sendEvent(scene(), &ke);
	});
	pasteAction->setShortcut(QKeySequence::Paste);

	menu.addSeparator();

	auto selectAllAction = menu.addAction(tr("Select all"), this, [&] 
	{ 
		QTextCursor c(this->document());
		c.select(QTextCursor::Document);
		this->setTextCursor(c);
	});
	selectAllAction->setShortcut(QKeySequence::SelectAll);

	menu.exec(event->screenPos());

	// delayed call to "m_menuActive = false" to get the event processed before
	QTimer::singleShot(100, this, [&] { m_menuActive = false; });
}


void CTextLabelEdit::startEdit(CItem *item)
{
	m_item = item;

	if (m_item == nullptr)
		return;

	auto scene = m_item->getScene();
	if (scene == nullptr)
		return;

	scene->selectItem(m_item);

	m_storedText = m_item->getAttribute("label").toString();

	setPlainText(m_storedText);
	setFont(m_item->getAttribute("label.font").value<QFont>());
	setDefaultTextColor(m_item->getAttribute("label.color").value<QColor>());

	updateGeometry();

	QTextCursor c(document());
	c.select(QTextCursor::Document);
	setTextCursor(c);
	
	setFocus();

	scene->addItem(this);
	show();

	Q_EMIT editingStarted(m_item);
}


void CTextLabelEdit::onItemLayout()
{
	if (m_item == nullptr)
		return;

	// update attributes
	setFont(m_item->getAttribute("label.font").value<QFont>());
	//setDefaultTextColor(m_item->getAttribute("label.color").value<QColor>());

	updateGeometry();
}


void CTextLabelEdit::finishEdit(bool accept)
{
	if (m_item == nullptr)
		return;

	Q_EMIT editingFinished(m_item, !accept);

	auto scene = m_item->getScene();
	if (scene == nullptr)
		return;

	QString text = toPlainText();
	if (accept && m_storedText != text)
	{
		m_item->setAttribute("label", text);

		scene->addUndoState();
	}

	m_item = nullptr;
	scene->removeItem(this);
}
