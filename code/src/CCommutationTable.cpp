/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CCommutationTable.h"

#include <qvge/CNodeEditorScene.h>
#include <qvge/CConnection.h>
#include <qvge/CNode.h>

#include <QDebug>
#include <QElapsedTimer>


// NumSortItem: numeric sorting by ids

class NumSortItem : public QTreeWidgetItem
{
public:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int col = treeWidget()->sortColumn();
		bool b1, b2;
		int i1 = text(col).toInt(&b1);
		int i2 = other.text(col).toInt(&b2);
		if (b1 && b2)
			return i1 < i2;

		return QTreeWidgetItem::operator < (other);
	}
};


// CCommutationTable

CCommutationTable::CCommutationTable(QWidget *parent)
	: QWidget(parent),
	m_scene(NULL)
{
	ui.setupUi(this);
	ui.Table->setUniformRowHeights(true);
}

CCommutationTable::~CCommutationTable()
{

}

void CCommutationTable::setScene(CNodeEditorScene* scene)
{
	ui.Table->clear();

	if (m_scene)
		onSceneDetached(m_scene);

	m_scene = scene;

	setEnabled(m_scene);

	if (m_scene)
		onSceneAttached(m_scene);
}

void CCommutationTable::connectSignals(CEditorScene* scene)
{
    connect(scene, SIGNAL(sceneChanged()), this, SLOT(onSceneChanged()), Qt::QueuedConnection);
    connect(scene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()), Qt::QueuedConnection);
}

void CCommutationTable::onSceneAttached(CEditorScene* scene)
{
	connectSignals(scene);

	onSceneChanged();
}

void CCommutationTable::onSceneDetached(CEditorScene* scene)
{
	scene->disconnect(this);
}

void CCommutationTable::onSceneChanged()
{
	ui.Table->setUpdatesEnabled(false);
	ui.Table->blockSignals(true);

	ui.Table->clear();
	m_edgeItemMap.clear();

	QList<CConnection*> edges = m_scene->getItems<CConnection>();
	for (auto edge : edges)
	{
		auto item = new NumSortItem();
		ui.Table->addTopLevelItem(item);

		m_edgeItemMap[edge] = item;

		item->setText(0, edge->firstNode()->getId());
		item->setText(1, edge->lastNode()->getId());
		item->setText(2, edge->getId());
	}

	ui.Table->setUpdatesEnabled(true);
	ui.Table->blockSignals(false);

	// update active selections if any
	onSelectionChanged();
}

void CCommutationTable::onSelectionChanged()
{
	ui.Table->setUpdatesEnabled(false);
	ui.Table->blockSignals(true);

	ui.Table->clearSelection();

	QTreeWidgetItem* scrollItem = NULL;

    QList<CConnection*> edges = m_scene->getSelectedEdges();

	//QElapsedTimer tm;
	//tm.start();

	QItemSelection selection;

	for (auto edge : edges)
	{
		if (m_edgeItemMap.contains(edge))
		{
			auto item = m_edgeItemMap[edge];
			scrollItem = item;

			//item->setSelected(true);
			// version with QModelIndex is many ways faster...
			int row = ui.Table->indexOfTopLevelItem(item);

			QModelIndex leftIndex = ui.Table->model()->index(row, 0);
			QModelIndex rightIndex = ui.Table->model()->index(row, ui.Table->columnCount() - 1);

			QItemSelection rowSelection(leftIndex, rightIndex);
			selection.append(rowSelection);
			//selection.merge(rowSelection, QItemSelectionModel::Select);	// slow
		}
	}

	ui.Table->selectionModel()->select(selection, QItemSelectionModel::Select);

	//qDebug() << "CCommutationTable::onSelectionChanged(): " << tm.elapsed();

	if (scrollItem)
		ui.Table->scrollToItem(scrollItem);

	ui.Table->setUpdatesEnabled(true);
	ui.Table->blockSignals(false);
}

void CCommutationTable::on_Table_itemSelectionChanged()
{
	if (!m_scene)
		return;

	ui.Table->blockSignals(true);

	m_scene->beginSelection();

	m_scene->deselectAll();

	auto selTableItems = ui.Table->selectedItems();
	QSet<QString> selIds;
	for (auto item : selTableItems)
	{
		selIds.insert(item->text(2));
	}

	QList<CConnection*> edges = m_scene->getItems<CConnection>();
	for (auto edge : edges)
	{
		if (selIds.contains(edge->getId()))
		{
			edge->setSelected(true);
			edge->ensureVisible();
		}
	}

	ui.Table->blockSignals(false);

	m_scene->endSelection();
}

void CCommutationTable::on_Table_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	if (!m_scene || !item)
		return;

	if (column < 2)
	{
		auto nodes = m_scene->getItemsById<CNode>(item->text(column));
		if (nodes.count())
		{
			m_scene->deselectAll();
			nodes.first()->setSelected(true);
			nodes.first()->ensureVisible();
			return;
		}
	}

	if (column == 2)
	{
		auto edges = m_scene->getItemsById<CConnection>(item->text(column));
		if (edges.count())
		{
			m_scene->deselectAll();
			edges.first()->setSelected(true);
			edges.first()->ensureVisible();
			return;
		}
	}
}
