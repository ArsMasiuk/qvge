/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QInputDialog>
#include <QMessageBox>

#include "CNodeEdgePropertiesUI.h"
#include "ui_CNodeEdgePropertiesUI.h"

#include <qvge/CNodeEditorScene.h>
#include <qvge/CConnection.h>
#include <qvge/CNode.h>
#include <qvge/CAttribute.h>


CNodeEdgePropertiesUI::CNodeEdgePropertiesUI(QWidget *parent) :
    QWidget(parent),
    m_scene(NULL),
    m_updateLock(false),
    ui(new Ui::CNodeEdgePropertiesUI)
{
    ui->setupUi(this);

    ui->NodeColor->setColorScheme(QSint::OpenOfficeColors());
    ui->NodeColor->setColor(Qt::green);

    ui->NodeShape->addAction(QIcon(":/Icons/Node-Disc"), tr("Disc"), "disc");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Square"), tr("Square"), "square");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Triangle"), tr("Triangle Up"), "triangle");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Diamond"), tr("Diamond"), "diamond");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Triangle-Down"), tr("Triangle Down"), "triangle2");

    ui->NodeAttrBox->setChecked(false);


	ui->EdgeDirection->addAction(QIcon(":/Icons/Edge-Directed"), tr("Directed (one end)"), "directed");
	ui->EdgeDirection->addAction(QIcon(":/Icons/Edge-Mutual"), tr("Mutual (both ends)"), "mutual");
	ui->EdgeDirection->addAction(QIcon(":/Icons/Edge-Undirected"), tr("None (no ends)"), "undirected");

    ui->EdgeColor->setColorScheme(QSint::OpenOfficeColors());
    ui->EdgeColor->setColor(Qt::red);

    ui->EdgeStyle->setUsedRange(Qt::SolidLine, Qt::DotLine);

    ui->EdgeAttrBox->setChecked(false);


    // update status & tooltips etc.
    ui->retranslateUi(this);
}


CNodeEdgePropertiesUI::~CNodeEdgePropertiesUI()
{
    delete ui;
}


void CNodeEdgePropertiesUI::setScene(CNodeEditorScene* scene)
{
    if (m_scene)
        onSceneDetached(m_scene);

    m_scene = scene;

    setEnabled(m_scene);

    if (m_scene)
        onSceneAttached(m_scene);
}


void CNodeEdgePropertiesUI::connectSignals(CEditorScene* scene)
{
    connect(scene, SIGNAL(sceneChanged()), this, SLOT(onSceneChanged()));
    connect(scene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
}


void CNodeEdgePropertiesUI::onSceneAttached(CEditorScene* scene)
{
    connectSignals(scene);

    onSceneChanged();
}


void CNodeEdgePropertiesUI::onSceneDetached(CEditorScene* scene)
{
    scene->disconnect(this);
}


void CNodeEdgePropertiesUI::onSceneChanged()
{
    // update active selections if any
    onSelectionChanged();
}


void CNodeEdgePropertiesUI::onSelectionChanged()
{
    if (m_updateLock || m_scene == NULL)
        return;

    m_updateLock = true;

    QList<CConnection*> edges = m_scene->getSelectedEdges();
    QList<CNode*> nodes = m_scene->getSelectedNodes();


    // nodes
    ui->NodesBox->setTitle(tr("Nodes (%1)").arg(nodes.count()));

    if (nodes.count())
    {
        auto node = nodes.first();

        ui->NodeColor->setColor(node->getAttribute("color").value<QColor>());
        ui->NodeShape->selectAction(node->getAttribute("shape"));
        ui->NodeSize->setValue(node->getAttribute("size").toSize().width());
    }

    if (nodes.count() == 1)
    {
        ui->NodeId->setEnabled(true);
        ui->NodeId->setText(tr("Node Id: %1").arg(nodes.first()->getId()));
    }
    else
    {
        ui->NodeId->setEnabled(false);
        ui->NodeId->setText(tr("Select single node to edit its Id"));
    }

    QList<CItem*> nodeItems;
    for (auto item: nodes) nodeItems << item;
    int attrCount = ui->NodeAttrEditor->setupFromItems(*m_scene, nodeItems);
	ui->NodeAttrBox->setTitle(tr("Custom Attributes (%1)").arg(attrCount));
	//ui->NodeAttrBox->setChecked(attrCount > 0);


    // edges
    ui->EdgesBox->setTitle(tr("Edges (%1)").arg(edges.count()));

    if (edges.count())
    {
        auto edge = edges.first();

        ui->EdgeColor->setColor(edge->getAttribute("color").value<QColor>());
        ui->EdgeWeight->setValue(edge->getAttribute("weight").toDouble());
        ui->EdgeStyle->selectAction(edge->getAttribute("style"));
		ui->EdgeDirection->selectAction(edge->getAttribute("direction"));
    }

    if (edges.count() == 1)
    {
        ui->EdgeId->setEnabled(true);
        ui->EdgeId->setText(tr("Edge Id: %1").arg(edges.first()->getId()));
    }
    else
    {
        ui->EdgeId->setEnabled(false);
        ui->EdgeId->setText(tr("Select single edge to edit its Id"));
    }

    QList<CItem*> edgeItems;
    for (auto item: edges) edgeItems << item;
	attrCount = ui->EdgeAttrEditor->setupFromItems(*m_scene, edgeItems);
	ui->EdgeAttrBox->setTitle(tr("Custom Attributes (%1)").arg(attrCount));


    // labels
    QList<CItem*> itemList;
    for (auto edgeItem: edges) itemList << edgeItem;
    for (auto nodeItem: nodes) itemList << nodeItem;
    for (auto item: itemList)
    {
        // skip empty labels
        if (item->getAttribute("label").toString().isEmpty())
            continue;

		QFont f(item->getAttribute("label.font").value<QFont>());
		int s(item->getAttribute("label.size").toInt());
		f.setPointSize(s);
        ui->LabelFont->setCurrentFont(f);
        ui->LabelSize->setValue(s);

        ui->LabelColor->setColor(item->getAttribute("label.color").value<QColor>());
        break;
    }

    // allow updates
    m_updateLock = false;
}


void CNodeEdgePropertiesUI::on_NodeColor_activated(const QColor &color)
{
    if (m_updateLock || m_scene == NULL)
        return;

    QList<CNode*> nodes = m_scene->getSelectedNodes();
    if (nodes.isEmpty())
        return;

    for (auto node: nodes)
    {
        node->setAttribute("color", color);
    }

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_NodeShape_activated(QVariant data)
{
    if (m_updateLock || m_scene == NULL)
        return;

    QList<CNode*> nodes = m_scene->getSelectedNodes();
    if (nodes.isEmpty())
        return;

    for (auto node: nodes)
    {
        node->setAttribute("shape", data);
    }

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_NodeSize_valueChanged(int value)
{
    if (m_updateLock || m_scene == NULL)
        return;

    QList<CNode*> nodes = m_scene->getSelectedNodes();
    if (nodes.isEmpty())
        return;

    for (auto node: nodes)
    {
        node->setAttribute("size", value);
    }

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_NodeId_clicked()
{
    QList<CNode*> nodes = m_scene->getSelectedNodes();
    if (nodes.count() != 1)
        return;

    QString id = nodes.first()->getId();
    QString editId = id;

_again:

    QString newId = QInputDialog::getText(this, tr("Change node Id"),
        tr("Specify new node Id:"), QLineEdit::Normal, editId);

    if (newId.isEmpty() || newId == id)
        return;

    auto items = m_scene->getItemsById(newId);
    for (auto item: items)
    {
        CNode* node = dynamic_cast<CNode*>(item);
        if (node == NULL || node == nodes.first())
            continue;

        if (node->getId() == newId)
        {
            int count = 0;
            QString nextFreeId = newId + QString::number(count++);
            while (m_scene->getItemsById(nextFreeId).count())
            {
                nextFreeId = newId + QString::number(count++);
            }

            QString autoId = QString(tr("Suggested Id: %1").arg(nextFreeId));

            int r = QMessageBox::warning(this, tr("Warning: Id is in use"),
                                 tr("Id %1 is already used by another node.").arg(newId),
                                 autoId,
                                 tr("Swap node Ids"),
                                 tr("Continue editing"), 0, 2);

            if (r == 2)
            {
                editId = newId;
                goto _again;
            }

            if (r == 1)
            {
                nodes.first()->setId(newId);
                node->setId(id);
                m_scene->addUndoState();
                return;
            }

            // r = 0
            editId = nextFreeId;
            goto _again;
        }
    }

    nodes.first()->setId(newId);
    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_EdgeColor_activated(const QColor &color)
{
    if (m_updateLock || m_scene == NULL)
        return;

    QList<CConnection*> edges = m_scene->getSelectedEdges();
    if (edges.isEmpty())
        return;

    for (auto edge: edges)
    {
        edge->setAttribute("color", color);
    }

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_EdgeWeight_valueChanged(double value)
{
    if (m_updateLock || m_scene == NULL)
        return;

    QList<CConnection*> edges = m_scene->getSelectedEdges();
    if (edges.isEmpty())
        return;

    for (auto edge: edges)
    {
        edge->setAttribute("weight", value);
    }

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_EdgeStyle_activated(QVariant data)
{
    if (m_updateLock || m_scene == NULL)
        return;

    QList<CConnection*> edges = m_scene->getSelectedEdges();
    if (edges.isEmpty())
        return;

    for (auto edge: edges)
    {
        edge->setAttribute("style", data);
    }

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_EdgeDirection_activated(QVariant data)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CConnection*> edges = m_scene->getSelectedEdges();
	if (edges.isEmpty())
		return;

	for (auto edge : edges)
	{
		edge->setAttribute("direction", data);
	}

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_EdgeId_clicked()
{
    QList<CConnection*> edges = m_scene->getSelectedEdges();
    if (edges.count() != 1)
        return;

    QString id = edges.first()->getId();
    QString editId = id;

_again:

    QString newId = QInputDialog::getText(this, tr("Change edge Id"),
        tr("Specify new edge Id:"), QLineEdit::Normal, editId);

    if (newId.isEmpty() || newId == id)
        return;

    auto items = m_scene->getItemsById(newId);
    for (auto item: items)
    {
        CConnection* edge = dynamic_cast<CConnection*>(item);
        if (edge == NULL || edge == edges.first())
            continue;

        if (edge->getId() == newId)
        {
            int count = 0;
            QString nextFreeId = newId + QString::number(count++);
            while (m_scene->getItemsById(nextFreeId).count())
            {
                nextFreeId = newId + QString::number(count++);
            }

            QString autoId = QString(tr("Suggested Id: %1").arg(nextFreeId));

            int r = QMessageBox::warning(this, tr("Warning: Id is in use"),
                                 tr("Id %1 is already used by another edge.").arg(newId),
                                 autoId,
                                 tr("Swap edge Ids"),
                                 tr("Continue editing"), 0, 2);

            if (r == 2)
            {
                editId = newId;
                goto _again;
            }

            if (r == 1)
            {
                edges.first()->setId(newId);
                edge->setId(id);
                m_scene->addUndoState();
                return;
            }

            // r = 0
            editId = nextFreeId;
            goto _again;
        }
    }

    edges.first()->setId(newId);
    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelFont_activated(const QFont &font)
{
    if (m_updateLock || m_scene == NULL)
        return;

    QList<CConnection*> edges = m_scene->getSelectedEdges();
	QList<CNode*> nodes = m_scene->getSelectedNodes();
	if (nodes.isEmpty() && edges.isEmpty())
		return;

	for (auto edge: edges)
    {
        edge->setAttribute("label.font", font);
		edge->setAttribute("label.size", font.pointSize());
    }

	for (auto node : nodes)
	{
		node->setAttribute("label.font", font);
		node->setAttribute("label.size", font.pointSize());
	}

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelColor_activated(const QColor &color)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CConnection*> edges = m_scene->getSelectedEdges();
	QList<CNode*> nodes = m_scene->getSelectedNodes();
	if (nodes.isEmpty() && edges.isEmpty())
		return;

	for (auto edge : edges)
	{
		edge->setAttribute("label.color", color);
	}

	for (auto node : nodes)
	{
		node->setAttribute("label.color", color);
	}

	m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelSize_valueChanged(int size)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CConnection*> edges = m_scene->getSelectedEdges();
	QList<CNode*> nodes = m_scene->getSelectedNodes();
	if (nodes.isEmpty() && edges.isEmpty())
		return;

	for (auto edge : edges)
	{
		edge->setAttribute("label.size", size);
	}

	for (auto node : nodes)
	{
		node->setAttribute("label.size", size);
	}

	m_scene->addUndoState();
}

