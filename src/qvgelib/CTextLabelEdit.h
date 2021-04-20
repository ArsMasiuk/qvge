/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QGraphicsTextItem> 

#include "IContextMenuProvider.h"


class CItem;
class CEditorScene;


class CTextLabelEdit: public QGraphicsTextItem, public IContextMenuProvider
{
	Q_OBJECT

public:
	CTextLabelEdit();
	~CTextLabelEdit();

	void startEdit(CItem *item);
	void finishEdit(bool accept = true);

	virtual bool onKeyPressed(CEditorScene& scene, QKeyEvent *keyEvent);
	virtual bool onKeyReleased(CEditorScene& scene, QKeyEvent *keyEvent);

	// IContextMenuProvider
	virtual bool showMenu(QGraphicsSceneContextMenuEvent* contextMenuEvent, CEditorScene* scene, const QList<QGraphicsItem*>& selectedItems);

Q_SIGNALS:
	void editingStarted(CItem *item);
	void editingFinished(CItem *item, bool cancelled);

protected:
	virtual bool sceneEvent(QEvent *event);
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

private Q_SLOT:
	void updateGeometry();

private:
	CItem *m_item = nullptr;
	QString m_storedText;
	bool m_menuActive = false;
};

