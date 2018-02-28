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
#include <qvge/CNode.h>
#include <qvge/CConnection.h>
#include <qvge/CDirectConnection.h>
#include <qvge/CAttribute.h>


CNodeEdgePropertiesUI::CNodeEdgePropertiesUI(QWidget *parent) :
    QWidget(parent),
    m_scene(NULL),
    m_updateLock(false),
    ui(new Ui::CNodeEdgePropertiesUI)
{
	m_nodeFactory = new CNode;
	m_edgeFactory = new CDirectConnection;


    ui->setupUi(this);

    ui->NodeColor->setColorScheme(QSint::OpenOfficeColors());
	ui->NodeColor->enableNoColor(true);

    ui->NodeShape->addAction(QIcon(":/Icons/Node-Disc"), tr("Disc"), "disc");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Square"), tr("Square"), "square");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Triangle"), tr("Triangle Up"), "triangle");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Diamond"), tr("Diamond"), "diamond");
    ui->NodeShape->addAction(QIcon(":/Icons/Node-Triangle-Down"), tr("Triangle Down"), "triangle2");
	ui->NodeShape->addAction(QIcon(":/Icons/Node-Hexagon"), tr("Hexagon"), "hexagon");

    ui->NodeAttrBox->setChecked(false);


	ui->EdgeDirection->addAction(QIcon(":/Icons/Edge-Directed"), tr("Directed (one end)"), "directed");
	ui->EdgeDirection->addAction(QIcon(":/Icons/Edge-Mutual"), tr("Mutual (both ends)"), "mutual");
	ui->EdgeDirection->addAction(QIcon(":/Icons/Edge-Undirected"), tr("None (no ends)"), "undirected");

    ui->EdgeColor->setColorScheme(QSint::OpenOfficeColors());

    ui->EdgeStyle->setUsedRange(Qt::SolidLine, Qt::DashDotDotLine);
	ui->StrokeStyle->setUsedRange(Qt::SolidLine, Qt::DashDotDotLine);

    ui->EdgeAttrBox->setChecked(false);


    // update status & tooltips etc.
    ui->retranslateUi(this);
}


CNodeEdgePropertiesUI::~CNodeEdgePropertiesUI()
{
	delete m_nodeFactory;
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


void CNodeEdgePropertiesUI::updateFromScene(CEditorScene* scene)
{
	// default attrs
	auto nodeAttrs = scene->getClassAttributes("node", false);
	ui->NodeColor->setColor(nodeAttrs["color"].defaultValue.value<QColor>());
	ui->NodeShape->selectAction(nodeAttrs["shape"].defaultValue);
	QSize size = nodeAttrs["size"].defaultValue.toSize();
	ui->NodeSizeSwitch->setChecked(size.width() == size.height());
	ui->NodeSizeY->setEnabled(size.width() != size.height());
	ui->NodeSizeX->setValue(size.width());
	ui->NodeSizeY->setValue(size.height());
	ui->StrokeColor->setColor(nodeAttrs["stroke.color"].defaultValue.value<QColor>());
	ui->StrokeStyle->setPenStyle(CUtils::textToPenStyle(nodeAttrs["stroke.style"].defaultValue.toString()));
	ui->StrokeSize->setValue(nodeAttrs["stroke.size"].defaultValue.toDouble());

	auto edgeAttrs = scene->getClassAttributes("edge", false);
	ui->EdgeColor->setColor(edgeAttrs["color"].defaultValue.value<QColor>());
	ui->EdgeWeight->setValue(edgeAttrs["weight"].defaultValue.toDouble());
	ui->EdgeStyle->setPenStyle(CUtils::textToPenStyle(edgeAttrs["style"].defaultValue.toString()));
	ui->EdgeDirection->selectAction(edgeAttrs["direction"].defaultValue);

	QFont f(edgeAttrs["label.font"].defaultValue.value<QFont>());
	ui->LabelFont->setCurrentFont(f);
	ui->LabelFontSize->setValue(f.pointSize());
	ui->LabelColor->setColor(edgeAttrs["label.color"].defaultValue.value<QColor>());
}


void CNodeEdgePropertiesUI::onSceneAttached(CEditorScene* scene)
{
	// factories for new items
	scene->setActiveItemFactory(m_nodeFactory);
	scene->setActiveItemFactory(m_edgeFactory);

	// default attrs
	updateFromScene(scene);

	// connect & go
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
		
		QSize size = node->getAttribute("size").toSize();
		ui->NodeSizeSwitch->setChecked(size.width() == size.height());
		ui->NodeSizeY->setEnabled(size.width() != size.height());
        ui->NodeSizeX->setValue(size.width());
		ui->NodeSizeY->setValue(size.height());

		ui->StrokeColor->setColor(node->getAttribute("stroke.color").value<QColor>());
		ui->StrokeStyle->setPenStyle(CUtils::textToPenStyle(node->getAttribute("stroke.style").toString()));
		ui->StrokeSize->setValue(node->getAttribute("stroke.size").toDouble());
    }

    if (nodes.count() == 1)
    {
        ui->NodeId->setEnabled(true);
        ui->NodeId->setText(tr("Node id: %1").arg(nodes.first()->getId()));

		ui->NodeLabel->setVisible(true);
    }
    else
    {
        ui->NodeId->setEnabled(false);
        ui->NodeId->setText(tr("Select single node to edit its id && text"));

		ui->NodeLabel->setVisible(false);
    }

    QList<CItem*> nodeItems;
    for (auto item: nodes) nodeItems << item;
    int attrCount = ui->NodeAttrEditor->setupFromItems(*m_scene, nodeItems);
	ui->NodeAttrBox->setTitle(tr("Custom Attributes (%1)").arg(attrCount));


    // edges
    ui->EdgesBox->setTitle(tr("Edges (%1)").arg(edges.count()));

    if (edges.count())
    {
        auto edge = edges.first();

        ui->EdgeColor->setColor(edge->getAttribute("color").value<QColor>());
        ui->EdgeWeight->setValue(edge->getAttribute("weight").toDouble());
		ui->EdgeStyle->setPenStyle(CUtils::textToPenStyle(edge->getAttribute("style").toString()));
		ui->EdgeDirection->selectAction(edge->getAttribute("direction"));
    }

    if (edges.count() == 1)
    {
        ui->EdgeId->setEnabled(true);
        ui->EdgeId->setText(tr("Edge id: %1").arg(edges.first()->getId()));

		ui->EdgeLabel->setVisible(true);
    }
    else
    {
        ui->EdgeId->setEnabled(false);
        ui->EdgeId->setText(tr("Select single edge to edit its id and text"));

		ui->EdgeLabel->setVisible(false);
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
		QFont f(item->getAttribute("label.font").value<QFont>());
        ui->LabelFont->setCurrentFont(f);
		ui->LabelFontSize->setValue(f.pointSize());
        ui->LabelColor->setColor(item->getAttribute("label.color").value<QColor>());
        break;
    }

    // allow updates
    m_updateLock = false;
}


void CNodeEdgePropertiesUI::setNodesAttribute(const QByteArray& attrId, const QVariant& v)
{
	if (m_nodeFactory)
		m_nodeFactory->setAttribute(attrId, v);

	if (m_updateLock || m_scene == NULL)
		return;

	QList<CNode*> nodes = m_scene->getSelectedNodes();
	if (nodes.isEmpty())
		return;

	for (auto node : nodes)
		node->setAttribute(attrId, v);

	m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::setEdgesAttribute(const QByteArray& attrId, const QVariant& v)
{
	if (m_edgeFactory)
		m_edgeFactory->setAttribute(attrId, v);

	if (m_updateLock || m_scene == NULL)
		return;

	QList<CConnection*> edges = m_scene->getSelectedEdges();
	if (edges.isEmpty())
		return;

	for (auto edge : edges)
		edge->setAttribute(attrId, v);

	m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_NodeColor_activated(const QColor &color)
{
	setNodesAttribute("color", color);
}


void CNodeEdgePropertiesUI::on_NodeShape_activated(QVariant data)
{
	setNodesAttribute("shape", data);
}


void CNodeEdgePropertiesUI::on_NodeSizeX_valueChanged(int /*value*/)
{
	ui->NodeSizeX->blockSignals(true);
	ui->NodeSizeY->blockSignals(true);

	if (ui->NodeSizeSwitch->isChecked())
		ui->NodeSizeY->setValue(ui->NodeSizeX->value());

	QSize size(ui->NodeSizeX->value(), ui->NodeSizeY->value());

	setNodesAttribute("size", size);

 	ui->NodeSizeX->blockSignals(false);
	ui->NodeSizeY->blockSignals(false);
}


void CNodeEdgePropertiesUI::on_NodeSizeY_valueChanged(int value)
{
	on_NodeSizeX_valueChanged(value);
}


void CNodeEdgePropertiesUI::on_NodeSizeSwitch_toggled(bool on)
{
	ui->NodeSizeY->setEnabled(!on);

	if (on)
	{
		ui->NodeSizeY->setValue(ui->NodeSizeX->value());
		ui->NodeSizeX->setFocus();
	}
	else
		ui->NodeSizeY->setFocus();
}


void CNodeEdgePropertiesUI::on_NodeLabel_clicked()
{
	QList<CNode*> nodes = m_scene->getSelectedNodes();
	if (nodes.count() != 1)
		return;

	m_scene->onActionEditLabel(nodes.first());
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


void CNodeEdgePropertiesUI::on_StrokeColor_activated(const QColor &color)
{
	setNodesAttribute("stroke.color", color);
}


void CNodeEdgePropertiesUI::on_StrokeStyle_activated(QVariant data)
{
	QString style = CUtils::penStyleToText(data.toInt());

	setNodesAttribute("stroke.style", style);
}


void CNodeEdgePropertiesUI::on_StrokeSize_valueChanged(double value)
{
	setNodesAttribute("stroke.size", value);
}


void CNodeEdgePropertiesUI::on_EdgeColor_activated(const QColor &color)
{
	setEdgesAttribute("color", color);
}


void CNodeEdgePropertiesUI::on_EdgeWeight_valueChanged(double value)
{
	setEdgesAttribute("weight", value);
}


void CNodeEdgePropertiesUI::on_EdgeStyle_activated(QVariant data)
{
	QString style = CUtils::penStyleToText(data.toInt());
	setEdgesAttribute("style", style);
}


void CNodeEdgePropertiesUI::on_EdgeDirection_activated(QVariant data)
{
	setEdgesAttribute("direction", data);
}


void CNodeEdgePropertiesUI::on_EdgeLabel_clicked()
{
	QList<CConnection*> edges = m_scene->getSelectedEdges();
	if (edges.count() != 1)
		return;

	m_scene->onActionEditLabel(edges.first());
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
	ui->LabelFontSize->blockSignals(true);
	ui->LabelFontSize->setValue(font.pointSize());
	ui->LabelFontSize->blockSignals(false);

    if (m_updateLock || m_scene == NULL)
        return;

    QList<CConnection*> edges = m_scene->getSelectedEdges();
	QList<CNode*> nodes = m_scene->getSelectedNodes();
	if (nodes.isEmpty() && edges.isEmpty())
		return;

	for (auto edge: edges)
    {
        edge->setAttribute("label.font", font);
    }

	for (auto node : nodes)
	{
		node->setAttribute("label.font", font);
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


void CNodeEdgePropertiesUI::on_LabelFontSize_valueChanged(int value)
{
	QFont f = ui->LabelFont->font();
	if (f.pointSize() != value)
	{
		f.setPointSize(value);
		ui->LabelFont->setFont(f);
		on_LabelFont_activated(f);
	}
}
