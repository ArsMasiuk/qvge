/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

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
	//connect(scene, SIGNAL(destroyed(QObject*)), this, SLOT(onSceneDied(QObject*)));
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

    ui->NodesBox->setTitle(tr("Nodes (%1)").arg(nodes.count()));

    if (nodes.count())
    {
        auto node = nodes.first();

        ui->NodeColor->setColor(node->getAttribute("color").value<QColor>());
        ui->NodeShape->selectAction(node->getAttribute("shape"));
        ui->NodeSize->setValue(node->getAttribute("size").toSize().width());
    }

    QList<CItem*> nodeItems;
    for (auto item: nodes) nodeItems << item;
    int attrCount = ui->NodeAttrEditor->setupFromItems(*m_scene, nodeItems);
	ui->NodeAttrBox->setTitle(tr("Custom Attributes (%1)").arg(attrCount));
	//ui->NodeAttrBox->setChecked(attrCount > 0);


    ui->EdgesBox->setTitle(tr("Edges (%1)").arg(edges.count()));

    if (edges.count())
    {
        auto edge = edges.first();

        ui->EdgeColor->setColor(edge->getAttribute("color").value<QColor>());
        ui->EdgeWeight->setValue(edge->getAttribute("weight").toDouble());
        ui->EdgeStyle->selectAction(edge->getAttribute("style"));
		ui->EdgeDirection->selectAction(edge->getAttribute("direction"));
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

