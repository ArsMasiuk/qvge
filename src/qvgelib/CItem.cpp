/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include "CItem.h"
#include "CEditorSceneDefines.h"

#include <QGraphicsSceneMouseEvent>
#include <QMenu>


bool CItem::s_duringRestore = false;


CItem::CItem()
{
	m_labelItem = NULL;

	// default item flags
	m_itemFlags = IF_DeleteAllowed | IF_FramelessSelection;
	m_internalStateFlags = IS_Attribute_Changed | IS_Need_Update;
}


CItem::~CItem()
{
	CEditorScene *scene = getScene();
	if (scene)
		scene->onItemDestroyed(this);
}


// IO

bool CItem::storeTo(QDataStream &out, quint64 version64) const
{
	if (version64 >= 2)
	{
		out << m_attributes;
	}

	if (version64 >= 4)
	{
		out << m_id;
	}

	return true;
}


bool CItem::restoreFrom(QDataStream &out, quint64 version64)
{
	if (!out.atEnd())
	{
		if (version64 >= 2)
		{
			out >> m_attributes;
		}
		else
			m_attributes.clear();

		if (version64 >= 4)
		{
			out >> m_id;
		}

		return true;
	}

	return false;
}


// attributes

bool CItem::hasLocalAttribute(const QByteArray& attrId) const
{
	if (attrId == "id")
		return true;
	else
		return m_attributes.contains(attrId);
}


bool CItem::setAttribute(const QByteArray& attrId, const QVariant& v)
{
	setItemStateFlag(IS_Attribute_Changed);

	if (attrId == "id")
	{
		m_id = v.toString();
		return true;
	}

	// real attributes
	m_attributes[attrId] = v;

	return true;
}


bool CItem::removeAttribute(const QByteArray& attrId)
{
	if (m_attributes.remove(attrId))
	{
		setItemStateFlag(IS_Attribute_Changed);
		return true;
	}
	else
		return false;
}


QVariant CItem::getAttribute(const QByteArray& attrId) const
{
	if (attrId == "id")
		return m_id;

	if (m_attributes.contains(attrId))
		return m_attributes[attrId];

	if (auto scene = getScene())
		return scene->getClassAttribute(classId(), attrId, true).defaultValue;

	return QVariant();
}


QSet<QByteArray> CItem::getVisibleAttributeIds(int flags) const
{
	QSet<QByteArray> result;

	if (flags == VF_ANY || flags == VF_TOOLTIP)
        result = getLocalAttributes().keys().toSet();

	if (flags == VF_LABEL)
		result += "label";

	if (auto scene = getScene())
	{
		if (flags == VF_ANY || flags == VF_TOOLTIP)
			result += scene->getClassAttributes(classId(), false).keys().toSet();
		else
			result += scene->getVisibleClassAttributes(classId(), false);
	}

    return result;
}


bool CItem::setDefaultId()
{
	if (m_id.isEmpty())
	{
		m_id = createNewId();
		return true;
	}

	return false;
}


// scene stuff

CEditorScene* CItem::getScene() const
{
	if (auto sceneItem = getSceneItem())
		return dynamic_cast<CEditorScene*>(sceneItem->scene());
	else
		return NULL;
}


void CItem::addUndoState()
{
	if (auto scene = getScene())
		scene->addUndoState();
}


// cloning

void CItem::copyDataFrom(CItem* from)
{
	m_itemFlags = from->m_itemFlags;
	
	// copy attrs
	m_attributes = from->m_attributes;

	updateCachedItems();
}


// painting

void CItem::updateLabelContent()
{
	auto scene = getScene();
	if (!scene)
		return;

	if (!(m_internalStateFlags & IS_Attribute_Changed) && 
		!(scene->itemLabelsEnabled()) &&
		!(scene->itemLabelsNeedUpdate())
	)
		return;

	resetItemStateFlag(IS_Attribute_Changed);

	if (!m_labelItem)
		return;

	QString labelToShow;
	auto idsToShow = getVisibleAttributeIds(CItem::VF_LABEL);
	
	QMap<QByteArray, QString> visibleLabels;
	for (const QByteArray& id : idsToShow)
	{
        QString text = CUtils::variantToText(getAttribute(id));
		if (text.size())
			visibleLabels[id] = text;
	}

	// ids first
	if (idsToShow.contains("id"))
	{
		labelToShow = "[" + visibleLabels["id"] + "]";
		visibleLabels.remove("id");
	}

	// other labels
	if (visibleLabels.size() == 1 && idsToShow.contains("label"))
	{
		if (labelToShow.size())
			labelToShow += "\n";

		labelToShow += visibleLabels.values().first();
	}
	else
	{
		for (auto it = visibleLabels.constBegin(); it != visibleLabels.constEnd(); ++it)
		{
			if (labelToShow.size())
				labelToShow += "\n";

			labelToShow += QString("%1: %2").arg(QString(it.key())).arg(it.value());
		}
	}

	// text
	setLabelText(labelToShow);


    // label attrs
	m_labelItem->setBrush(getAttribute(attr_label_color).value<QColor>());
	
	QFont f(getAttribute(attr_label_font).value<QFont>());

	if (!scene->isFontAntialiased())
		f.setStyleStrategy(QFont::NoAntialias);

    m_labelItem->setFont(f);


	// update exlicitly
	m_labelItem->update();
}


void CItem::updateLabelDecoration()
{
	if (!m_labelItem)
		return;

	if (m_internalStateFlags & IS_Selected)
		m_labelItem->setOpacity(0.6);
	else
		m_labelItem->setOpacity(1.0);

	//if (m_internalStateFlags & IS_Selected)
	//	m_labelItem->setBrush(QColor(QStringLiteral("orange")));
	//else
	//	m_labelItem->setBrush(getAttribute(QByteArrayLiteral("label.color")).value<QColor>());
}


void CItem::setLabelText(const QString& text)
{
	if (m_labelItem)
		m_labelItem->setText(text);
}


void CItem::showLabel(bool on)
{
	if (m_labelItem)
	{
		m_labelItem->setVisible(on);

		if (on)
			updateLabelDecoration();
	}
}


QRectF CItem::getSceneLabelRect() const 
{
	if (!m_labelItem)
		return QRectF();
	else
		return m_labelItem->mapRectToScene(m_labelItem->boundingRect());
}


QPointF CItem::getLabelCenter() const
{
	if (m_labelItem)
		return getSceneLabelRect().center();
	else 
	if (auto sceneItem = getSceneItem())
		return sceneItem->boundingRect().center();
	else
		return QPointF();
}


// callbacks

void CItem::onItemRestored()
{
	updateCachedItems();
}


void CItem::onItemSelected(bool state)
{
	if (state)
		m_internalStateFlags |= IS_Selected;
	else
		m_internalStateFlags &= ~IS_Selected;

	updateLabelDecoration();
}


void CItem::onHoverEnter(QGraphicsItem* sceneItem, QGraphicsSceneHoverEvent*)
{
	/*
	// update tooltip
	auto idsToShow = getVisibleAttributeIds(CItem::VF_TOOLTIP).toList();
	if (idsToShow.isEmpty())
	{
		sceneItem->setToolTip("");
		return;
	}

	qSort(idsToShow);

	QString tooltipToShow("<table columns=2>");

	for (const QByteArray& id : idsToShow)
	{
        QString text = CUtils::variantToText(getAttribute(id));
		tooltipToShow += QString("<tr><td><b>%1</b>: </td> <td> %2</td>").arg(QString(id)).arg(text);
	}

	tooltipToShow += "</table>";
	*/

	QString tooltipToShow = CUtils::variantToText(getAttribute("tooltip"));

	sceneItem->setToolTip(tooltipToShow);
}


// internal

void CItem::updateCachedItems()
{
	setItemStateFlag(IS_Attribute_Changed);

	// update text label
	if (getScene() && getScene()->itemLabelsEnabled())
	{
		updateLabelContent();
		updateLabelPosition();
		updateLabelDecoration();
	}
}
