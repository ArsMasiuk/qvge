/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CCommutationTable_H
#define CCommutationTable_H

#include <QWidget>

class CEditorScene;
class CNodeEditorScene;
struct CAttribute;
class CConnection;

#include "ui_CCommutationTable.h"


class CCommutationTable : public QWidget
{
	Q_OBJECT

public:
	CCommutationTable(QWidget *parent = 0);
	~CCommutationTable();

	void setScene(CNodeEditorScene* scene);

protected:
	void connectSignals(CEditorScene* scene);
	void onSceneAttached(CEditorScene* scene);
	void onSceneDetached(CEditorScene* scene);

protected Q_SLOTS:
	void onSceneChanged();
	void onSelectionChanged();
	void on_Table_itemSelectionChanged();
	void on_Table_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
	Ui::CCommutationTable ui;

	CNodeEditorScene *m_scene;

	QMap<CConnection*, QTreeWidgetItem*> m_edgeItemMap;
};

#endif // CCommutationTable_H
