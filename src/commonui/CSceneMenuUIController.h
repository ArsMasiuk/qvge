#pragma once

#include <QObject>
#include <QMenu>

#include <qvge/ISceneMenuController.h>

class CNodeEditorScene;


class CSceneMenuUIController : public QObject, public ISceneMenuController
{
	Q_OBJECT

public:
	CSceneMenuUIController(QObject *parent = nullptr);
	virtual ~CSceneMenuUIController();

	// called before \a menu is about to show.
	virtual void fillMenu(QMenu &menu, CEditorScene *scene, QGraphicsItem *triggerItem, QGraphicsSceneContextMenuEvent *contextMenuEvent);

	// reimp: ISceneMenuController
	virtual bool exec(CEditorScene *scene, QGraphicsItem *triggerItem, QGraphicsSceneContextMenuEvent *contextMenuEvent);

Q_SIGNALS:
	// emitted before \a menu is about to show.
	void onContextMenu(QMenu &menu);

private:
	CNodeEditorScene* m_scene = nullptr;
};


