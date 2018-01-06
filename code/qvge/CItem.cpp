/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include "CItem.h"

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

	return getClassAttribute(attrId);
}

QSet<QByteArray> CItem::getVisibleAttributeIds(int flags) const
{
	QSet<QByteArray> result;

	if (flags == VF_ANY || flags == VF_TOOLTIP)
        result = getLocalAttributes().keys().toSet();

	if (auto scene = getScene())
	{
		if (flags == VF_ANY || flags == VF_TOOLTIP)
			result += scene->getClassAttributes(classId(), true).keys().toSet();
		else
			result += scene->getVisibleClassAttributes(classId(), true);
	}

    return result;
}

QVariant CItem::getClassAttribute(const QByteArray& attrId) const
{
	auto scene = getScene();
	if (scene)
	{
		QByteArray lookId = classId();

		while (!lookId.isEmpty())
		{
			auto v = scene->getClassAttribute(lookId, attrId);
			if (v.isValid()) 
				return v;

			lookId = scene->getSuperClassId(lookId);
		}
	}

	return QVariant();
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
	if (!(m_internalStateFlags & IS_Attribute_Changed) && 
		!(getScene()->itemLabelsEnabled()) &&
		!(getScene()->itemLabelsNeedUpdate())
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
		QString text = Utils::variantToText(getAttribute(id));
		if (text.size())
			visibleLabels[id] = text;
	}

	if (visibleLabels.size() == 1)
	{
		labelToShow = visibleLabels.values().first();
	}
    else
        if (visibleLabels.size() > 1)
            for (auto it = visibleLabels.constBegin(); it != visibleLabels.constEnd(); ++it)
            {
                if (labelToShow.size())
                    labelToShow += "\n";

                labelToShow += QString("%1: %2").arg(QString(it.key())).arg(it.value());
            }
	
	setLabelText(labelToShow);

    // label attrs
    QFont f = getAttribute("label.font").value<QFont>();

    bool ok = false;
    int s = getAttribute("label.size").toInt(&ok);
	if (ok && s > 0)
	{
		f.setPointSize(s);
	}

    m_labelItem->setFont(f);

    QColor c = getAttribute("label.color").value<QColor>();
    m_labelItem->setBrush(c);
}


void CItem::updateLabelDecoration()
{
	if (!m_labelItem)
		return;

	if (m_internalStateFlags & IS_Selected)
		m_labelItem->setBrush(QColor("orange"));
	else
	{
		QColor c = getAttribute("label.color").value<QColor>();
		//if (c.isValid())
			m_labelItem->setBrush(c);
	}
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
	
	return m_labelItem->mapRectToScene(m_labelItem->boundingRect());
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
	// update tooltip
	QString tooltipToShow;

	auto idsToShow = getVisibleAttributeIds(CItem::VF_TOOLTIP);
	for (const QByteArray& id : idsToShow)
	{
		QString text = Utils::variantToText(getAttribute(id));
		if (tooltipToShow.size()) tooltipToShow += "\n";
		tooltipToShow += QString("%1: \t%2").arg(QString(id)).arg(text);
	}

	sceneItem->setToolTip(tooltipToShow);
}
