/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CAttributesEditorUI.h"
#include "ui_CAttributesEditorUI.h"
#include "CNewAttributeDialog.h"

#include <qvge/CEditorScene.h>
#include <qvge/CItem.h>

#include <QMessageBox>


CAttributesEditorUI::CAttributesEditorUI(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::CAttributesEditorUI)
{
	ui->setupUi(this);

    ui->Editor->setFactoryForManager(&m_manager, &m_factory);

    connect(&m_manager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
            this, SLOT(onValueChanged(QtProperty*, const QVariant&)));
}

CAttributesEditorUI::~CAttributesEditorUI()
{
	delete ui;
}


int CAttributesEditorUI::setupFromItems(CEditorScene& scene, QList<CItem*> &items)
{
	// order of clear() is important!
	ui->Editor->setUpdatesEnabled(false);
	ui->Editor->clear();

	m_manager.blockSignals(true);
	m_manager.clear();

	m_scene = &scene;
	m_items = items;

	struct AttrData
	{
		QVariant data;
        int dataType = -1;
		bool isSet = false;
	};

	QSet<QByteArray> ids;
	QMap<QByteArray, AttrData> attrs;

	// merge ids
	for (auto item : items)
	{
		auto localAttrs = item->getLocalAttributes();
		ids = ids.unite(localAttrs.keys().toSet());
	}

	// merge attrs
	for (auto id : ids)
	{
		for (auto item : items)
		{
			if (item->hasLocalAttribute(id))
			{
				auto itemValue = item->getAttribute(id);

				// trick: float -> double
				if (itemValue.type() == QMetaType::Float)
					attrs[id].dataType = QMetaType::Double;
				else
					attrs[id].dataType = itemValue.type();


                if (attrs[id].isSet && attrs[id].data != itemValue)
                {
                    attrs[id].data = QVariant();
                    break;
                }
				else
				{
					attrs[id].data = itemValue;
					attrs[id].isSet = true;
				}
			}
			else
			{
                attrs[id].data = QVariant();
				attrs[id].isSet = true;
			}
		}
	}

	int topCount = 0;

	for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it)
	{
        auto prop = m_manager.addProperty(it.value().dataType, it.key());
		Q_ASSERT(prop != NULL);

		// add as string if unknown
		if (!prop)
			prop = m_manager.addProperty(QMetaType::QString, it.key());

		if (!prop)
			continue;	// ignore

        prop->setValue(it.value().data);
        auto item = ui->Editor->addProperty(prop);
        ui->Editor->setExpanded(item, false);

        if (!it.value().data.isValid())
            prop->setModified(true);

		topCount++;
	}

	ui->Editor->setUpdatesEnabled(true);

    m_manager.blockSignals(false);

	// force update
	on_Editor_currentItemChanged(ui->Editor->currentItem());

    return topCount;
}


void CAttributesEditorUI::on_AddButton_clicked()
{
	if (!m_scene || m_items.isEmpty())
		return;

    CNewAttributeDialog dialog;
    if (dialog.exec() == QDialog::Rejected)
        return;

    auto id = dialog.getId();
    if (id.isEmpty())
        return;

    auto v = dialog.getValue();

	bool used = false;

	for (auto sceneItem : m_items)
	{
        if (sceneItem->hasLocalAttribute(id))
			continue;

        sceneItem->setAttribute(id, v);
		used = true;
	}

	if (!used)
		return;

	// store state
	m_scene->addUndoState();

	// rebuild tree
	setupFromItems(*m_scene, m_items);

	// select item
	QList<QtBrowserItem*> items = ui->Editor->topLevelItems();
	for (auto item : items)
	{
		if (item->property()->propertyName().toLocal8Bit() == id)
		{
			ui->Editor->setCurrentItem(item);
			break;
		}
	}

	ui->Editor->setFocus();
}


void CAttributesEditorUI::on_RemoveButton_clicked()
{
	if (!m_scene || m_items.isEmpty())
		return;

	auto item = (ui->Editor->currentItem());
	if (!item)
		return;

	// no subprops
	if (item->parent())
		return;

	auto prop = item->property();
	QString attrId = prop->propertyName();
	if (attrId.isEmpty())
		return;

	int r = QMessageBox::question(NULL,
		tr("Remove Attribute"),
		tr("Remove attribute '%1' from selected item(s)?").arg(attrId),
		QMessageBox::Yes, QMessageBox::Cancel);

	if (r == QMessageBox::Cancel)
		return;

	delete prop;

	bool used = false;

	for (auto sceneItem : m_items)
	{
		bool ok = sceneItem->removeAttribute(attrId.toLatin1());
		if (ok)
			sceneItem->getSceneItem()->update();

		used |= ok;
	}

	if (!used)
		return;

	// store state
	m_scene->addUndoState();

	// rebuild tree
	//setupFromItems(*m_scene, m_items);

	ui->Editor->setFocus();
}


void CAttributesEditorUI::on_Editor_currentItemChanged(QtBrowserItem* item)
{
	ui->RemoveButton->setEnabled(item != NULL);
}


void CAttributesEditorUI::onValueChanged(QtProperty *property, const QVariant &val)
{
	if (!m_scene || m_items.isEmpty())
		return;

	// no subprops
	if (!ui->Editor->topLevelItem(property))
		return;

	auto attrId = property->propertyName().toLatin1();

	for (auto sceneItem : m_items)
	{
        sceneItem->setAttribute(attrId, val);
	}

	// store state
	m_scene->addUndoState();
}

