/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QInputDialog>
#include <QMessageBox>

#include "CAttributesEditorUI.h"
#include "CPropertyEditorUIBase.h"
#include "CNodeEdgePropertiesUI.h"
#include "ui_CNodeEdgePropertiesUI.h"

#include <qvge/CNodeEditorScene.h>
#include <qvge/CNode.h>
#include <qvge/CEdge.h>
#include <qvge/CDirectEdge.h>
#include <qvge/CPolyEdge.h>
#include <qvge/CAttribute.h>
#include <qvge/CEditorSceneDefines.h>


CNodeEdgePropertiesUI::CNodeEdgePropertiesUI(QWidget *parent) :
    QWidget(parent),
    m_scene(NULL),
    m_updateLock(false),
    ui(new Ui::CNodeEdgePropertiesUI)
{
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

	ui->EdgeType->addAction(QIcon(":/Icons/Edge-Line"), tr("Direct line"), "line");
	ui->EdgeType->addAction(QIcon(":/Icons/Edge-Polyline"), tr("Polyline"), "polyline");

    ui->EdgeColor->setColorScheme(QSint::OpenOfficeColors());

    ui->EdgeStyle->setUsedRange(Qt::SolidLine, Qt::DashDotDotLine);
	ui->StrokeStyle->setUsedRange(Qt::SolidLine, Qt::DashDotDotLine);

    ui->EdgeAttrBox->setChecked(false);


	// font size
	QList<int> fontSizes = { 5,6,7,8,9,10,11,12,14,16,18,20,24,28,32,36,40,44,48,54,60,66,72,80,88,96 };
	ui->LabelFontSize->setValueList(fontSizes);


	// node size
	QList<int> nodeSizes = { 5,10,15,20,30,40,50,75,100,150,200 };
	ui->NodeSizeX->setValueList(nodeSizes);
	ui->NodeSizeY->setValueList(nodeSizes);


	// node outline
	QList<double> strokeSizes = { 1,2,3,4,5,6,7,8,9,10 };
	ui->StrokeSize->setValueList(strokeSizes);


	// edge weight
	QList<double> edgeSizes = { 1,2,3,4,5,6,7,8,9,10 };
	ui->EdgeWeight->setValueList(edgeSizes);


    // update status & tooltips etc.
    ui->retranslateUi(this);
}


CNodeEdgePropertiesUI::~CNodeEdgePropertiesUI()
{
    delete ui;
}


void CNodeEdgePropertiesUI::doReadSettings(QSettings& settings)
{
	int pos = settings.value("nodes/splitterPosition", -1).toInt();
	if (pos >= 0)
		ui->NodeAttrEditor->getEditor()->setSplitterPosition(pos);

	/*int*/ pos = settings.value("edges/splitterPosition", -1).toInt();
	if (pos >= 0)
		ui->EdgeAttrEditor->getEditor()->setSplitterPosition(pos);
}


void CNodeEdgePropertiesUI::doWriteSettings(QSettings& settings)
{
	settings.setValue("nodes/splitterPosition", ui->NodeAttrEditor->getEditor()->splitterPosition());
	settings.setValue("edges/splitterPosition", ui->EdgeAttrEditor->getEditor()->splitterPosition());
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
	ui->NodeSizeY->setVisible(size.width() != size.height());
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
	m_nodeFactory = m_scene->getNodesFactory();
	m_edgeFactory = m_scene->getEdgesFactory();

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

    QList<CEdge*> edges = m_scene->getSelectedEdges();
    QList<CNode*> nodes = m_scene->getSelectedNodes();


    // nodes
    ui->NodesBox->setTitle(tr("Nodes: %1").arg(nodes.count()));

    if (nodes.count())
    {
        auto node = nodes.first();

        ui->NodeColor->setColor(node->getAttribute("color").value<QColor>());
        ui->NodeShape->selectAction(node->getAttribute("shape"));
		
		QSize size = node->getAttribute("size").toSize();
		ui->NodeSizeSwitch->setChecked(size.width() == size.height());
		ui->NodeSizeY->setVisible(size.width() != size.height());
		ui->NodeSizeX->setValue(size.width());
		ui->NodeSizeY->setValue(size.height());

		ui->StrokeColor->setColor(node->getAttribute("stroke.color").value<QColor>());
		ui->StrokeStyle->setPenStyle(CUtils::textToPenStyle(node->getAttribute("stroke.style").toString()));
		ui->StrokeSize->setValue(node->getAttribute("stroke.size").toDouble());
    }

    QList<CItem*> nodeItems;
    for (auto item: nodes) nodeItems << item;
    int attrCount = ui->NodeAttrEditor->setupFromItems(*m_scene, nodeItems);
	ui->NodeAttrBox->setTitle(tr("Custom Attributes: %1").arg(attrCount));


    // edges
    ui->EdgesBox->setTitle(tr("Edges: %1").arg(edges.count()));

    if (edges.count())
    {
        auto edge = edges.first();

        ui->EdgeColor->setColor(edge->getAttribute("color").value<QColor>());
        ui->EdgeWeight->setValue(edge->getAttribute("weight").toDouble());
		ui->EdgeStyle->setPenStyle(CUtils::textToPenStyle(edge->getAttribute("style").toString()));
		ui->EdgeDirection->selectAction(edge->getAttribute("direction"));
    }

    QList<CItem*> edgeItems;
    for (auto item: edges) edgeItems << item;
	attrCount = ui->EdgeAttrEditor->setupFromItems(*m_scene, edgeItems);
	ui->EdgeAttrBox->setTitle(tr("Custom Attributes: %1").arg(attrCount));


    // labels
    QList<CItem*> itemList;
    for (auto edgeItem: edges) itemList << edgeItem;
    for (auto nodeItem: nodes) itemList << nodeItem;
    for (auto item: itemList)
    {
		QFont f(item->getAttribute("label.font").value<QFont>());
        ui->LabelFont->setCurrentFont(f);
		ui->LabelFontSize->setValue(f.pointSize());
		ui->LabelFontBold->setChecked(f.bold());
		ui->LabelFontItalic->setChecked(f.italic());
		ui->LabelFontUnderline->setChecked(f.underline());
		ui->LabelColor->setColor(item->getAttribute("label.color").value<QColor>());
        break;
    }

    // allow updates
    m_updateLock = false;
}


void CNodeEdgePropertiesUI::setNodesAttribute(const QByteArray& attrId, const QVariant& v)
{
	//if (m_nodeFactory)
	//	m_nodeFactory->setAttribute(attrId, v);

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
	//if (m_edgeFactory)
	//	m_edgeFactory->setAttribute(attrId, v);

	if (m_updateLock || m_scene == NULL)
		return;

	QList<CEdge*> edges = m_scene->getSelectedEdges();
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
	ui->NodeSizeY->setVisible(!on);

	if (on)
	{
		ui->NodeSizeY->setValue(ui->NodeSizeX->value());
		//ui->NodeSizeX->setFocus();
	}
	//else
	//	ui->NodeSizeY->setFocus();
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


void CNodeEdgePropertiesUI::on_EdgeType_activated(QVariant data)
{
	if (m_scene)
	{
		if (data.toString() == "line")
			m_scene->setEdgesFactory(m_scene->factory<CDirectEdge>());
		else
		if (data.toString() == "polyline")
			m_scene->setEdgesFactory(m_scene->factory<CPolyEdge>());

		m_edgeFactory = m_scene->getEdgesFactory();
	}
}


void CNodeEdgePropertiesUI::on_LabelFont_activated(const QFont &font)
{
	ui->LabelFontSize->blockSignals(true);
	ui->LabelFontSize->setValue(font.pointSize());
	ui->LabelFontSize->blockSignals(false);

    if (m_updateLock || m_scene == NULL)
        return;

	QList<CItem*> items = m_scene->getSelectedNodesEdges();
	if (items.isEmpty())
		return;

	for (auto item : items)
	{
		item->setAttribute(attr_label_font, font);
	}

    m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelColor_activated(const QColor &color)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CItem*> items = m_scene->getSelectedNodesEdges();
	if (items.isEmpty())
		return;

	for (auto item : items)
	{
		item->setAttribute(attr_label_color, color);
	}

	m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelFontSize_valueChanged(int value)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CItem*> items = m_scene->getSelectedNodesEdges();
	if (items.isEmpty())
		return;

	bool set = false;

	for (auto item : items)
	{
		QFont font = item->getAttribute(attr_label_font).value<QFont>();
		if (font.pointSize() != value)
		{
			font.setPointSize(value);
			item->setAttribute(attr_label_font, font);
			set = true;
		}
	}

	if (set)
		m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelFontBold_toggled(bool on)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CItem*> items = m_scene->getSelectedNodesEdges();
	if (items.isEmpty())
		return;

	bool set = false;

	for (auto item : items)
	{
		QFont font = item->getAttribute(attr_label_font).value<QFont>();
		if (font.bold() != on)
		{
			font.setBold(on);
			item->setAttribute(attr_label_font, font);
			set = true;
		}
	}

	if (set)
		m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelFontItalic_toggled(bool on)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CItem*> items = m_scene->getSelectedNodesEdges();
	if (items.isEmpty())
		return;

	bool set = false;

	for (auto item : items)
	{
		QFont font = item->getAttribute(attr_label_font).value<QFont>();
		if (font.italic() != on)
		{
			font.setItalic(on);
			item->setAttribute(attr_label_font, font);
			item->updateLabelContent();
			set = true;
		}
	}

	if (set)
		m_scene->addUndoState();
}


void CNodeEdgePropertiesUI::on_LabelFontUnderline_toggled(bool on)
{
	if (m_updateLock || m_scene == NULL)
		return;

	QList<CItem*> items = m_scene->getSelectedNodesEdges();
	if (items.isEmpty())
		return;

	bool set = false;

	for (auto item : items)
	{
		QFont font = item->getAttribute(attr_label_font).value<QFont>();
		if (font.underline() != on)
		{
			font.setUnderline(on);
			item->setAttribute(attr_label_font, font);
			set = true;
		}
	}

	if (set)
		m_scene->addUndoState();
}
