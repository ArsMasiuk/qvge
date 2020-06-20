/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CCommutationTable.h"

#include <qvge/CNodeEditorScene.h>
#include <qvge/CEdge.h>
#include <qvge/CNode.h>

#include <QDebug>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QScrollBar>
#include <QMessageBox>


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


// fixed section Ids

enum FixedSectionIds 
{
	StartNodeId, EndNodeId, EdgeId,
	CustomId
};


// CCommutationTable

CCommutationTable::CCommutationTable(QWidget *parent)
	: QWidget(parent),
	m_scene(NULL)
{
	ui.setupUi(this);

	ui.Table->header()->setSortIndicator(2, Qt::AscendingOrder);

	ui.Table->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.Table, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));

	ui.Table->header()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.Table->header(), SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));
}


CCommutationTable::~CCommutationTable()
{
}


void CCommutationTable::doReadSettings(QSettings& settings)
{
	QByteArray extraSections = settings.value("userColumns").toByteArray();
	if (!extraSections.isEmpty())
	{
		m_extraSectionIds = extraSections.split(';');
		onSceneChanged();
	}

	auto *header = ui.Table->header();
	QByteArray headerState = settings.value("headerState").toByteArray();
	if (!headerState.isNull())
	{
		header->restoreState(headerState);
	}
}


void CCommutationTable::doWriteSettings(QSettings& settings)
{
	auto *header = ui.Table->header();
	settings.setValue("headerState", header->saveState());
	settings.setValue("userColumns", m_extraSectionIds.join(';'));
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

	ui.Table->setColumnCount(m_extraSectionIds.size() + CustomId);

	int extraSectionIndex = CustomId;
	for (const auto& paramId : m_extraSectionIds)
	{
		ui.Table->headerItem()->setText(extraSectionIndex++, paramId);
	}

	QList<CEdge*> edges = m_scene->getItems<CEdge>();
	for (auto edge : edges)
	{
		auto item = new NumSortItem();
		ui.Table->addTopLevelItem(item);

		m_edgeItemMap[edge] = item;

		if (edge->firstPortId().size())
			item->setText(StartNodeId, edge->firstNode()->getId() + ":" + edge->firstPortId());
		else
			item->setText(StartNodeId, edge->firstNode()->getId());
		
		if (edge->lastPortId().size())
			item->setText(EndNodeId, edge->lastNode()->getId() + ":" + edge->lastPortId());
		else
			item->setText(EndNodeId, edge->lastNode()->getId());

		item->setText(EdgeId, edge->getId());

		int extraSectionIndex = CustomId;
		for (const auto& paramId : m_extraSectionIds)
		{
			QString val = edge->getAttribute(paramId).toString();
			item->setText(extraSectionIndex++, val);
		}
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

    QList<CEdge*> edges = m_scene->getSelectedEdges();

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

	QList<CEdge*> edges = m_scene->getItems<CEdge>();
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
		auto edges = m_scene->getItemsById<CEdge>(item->text(column));
		if (edges.count())
		{
			m_scene->deselectAll();
			edges.first()->setSelected(true);
			edges.first()->ensureVisible();
			return;
		}
	}
}


void CCommutationTable::onCustomContextMenu(const QPoint& pos)
{
	QMenu contextMenu;

	int sectionIndex = ui.Table->header()->logicalIndexAt(pos);
	if (sectionIndex >= CustomId) 
	{
		QAction* act = contextMenu.addAction(
			tr("Remove Column [%1]").arg(ui.Table->headerItem()->text(sectionIndex)), 
			this, 
			SLOT(onRemoveSection()));

		act->setData(sectionIndex - CustomId);

		contextMenu.addSeparator();
	}

	QAction* act = contextMenu.addAction(tr("Add Column..."), this, SLOT(onAddSection()));
	act->setData(pos);

	contextMenu.exec(ui.Table->mapToGlobal(pos));
}


void CCommutationTable::on_AddColumnButton_clicked()
{
	onAddSection();
}


void CCommutationTable::on_RestoreButton_clicked()
{
	if (!m_extraSectionIds.isEmpty()) {
		int r = QMessageBox::question(NULL, tr("Restore Default Columns"), tr("Are you sure to reset all the custom columns?"));
		if (r == QMessageBox::Yes)
		{
			m_extraSectionIds.clear();
			onSceneChanged();
		}
		else
			return;
	}

	for (int i = 0; i < ui.Table->header()->count(); ++i)
		ui.Table->header()->moveSection(ui.Table->header()->visualIndex(i), i);

	ui.Table->header()->setSortIndicator(2, Qt::AscendingOrder);
}


void CCommutationTable::onAddSection()
{
	QByteArrayList paramIdsList = m_scene->getClassAttributes("edge", true).keys();
	QStringList paramIds;
	for (const auto& id : paramIdsList)
		if (!m_extraSectionIds.contains(id))
			paramIds << id;

	QInputDialog dialog;
	dialog.setComboBoxItems(paramIds);
	dialog.setComboBoxEditable(true);
	dialog.setWindowTitle(tr("Add Column"));
	dialog.setLabelText(tr("Enter edge attribute ID:"));
	dialog.setInputMode(QInputDialog::TextInput);
	
	if (dialog.exec() != QDialog::Accepted)
		return;

	QByteArray paramId = dialog.textValue().toLocal8Bit();
	if (paramId.size() && !m_extraSectionIds.contains(paramId))
	{
		int sectionIndex = ui.Table->header()->count() - 1;

		QAction* act = dynamic_cast<QAction*>(sender());
		if (act)
		{
			QPoint pos = act->data().toPoint();
			sectionIndex = ui.Table->header()->logicalIndexAt(pos);
		}
		
		int listIndex = sectionIndex - CustomId;

		m_extraSectionIds.insert(listIndex+1, paramId);

		onSceneChanged();

		if (ui.Table->horizontalScrollBar())
		{
			int x = ui.Table->header()->sectionPosition(sectionIndex+1);
			ui.Table->horizontalScrollBar()->setSliderPosition(x);
		}
	}
}


void CCommutationTable::onRemoveSection()
{
	QAction* act = (QAction*)sender();
	int listIndex = act->data().toInt();
	m_extraSectionIds.removeAt(listIndex);
	onSceneChanged();
}
