/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CClassAttributesEditorUI.h"
#include "ui_CClassAttributesEditorUI.h"
#include "CNewAttributeDialog.h"

#include <qvge/CEditorScene.h>
#include <qvge/CItem.h>

#include <CIntegerProperty.h>
#include <CDoubleProperty.h>
#include <CBoolProperty.h>
#include <CStringProperty.h>
#include <CColorProperty.h>
#include <CFontProperty.h>

#include <QMessageBox>


CClassAttributesEditorUI::CClassAttributesEditorUI(QWidget *parent) :
	QWidget(parent),
    ui(new Ui::CClassAttributesEditorUI),
	m_scene(NULL)
{
	ui->setupUi(this);
}

CClassAttributesEditorUI::~CClassAttributesEditorUI()
{
	delete ui;
}


void CClassAttributesEditorUI::setScene(CEditorScene* scene)
{
    if (m_scene)
        onSceneDetached(m_scene);

    m_scene = scene;

    setEnabled(m_scene);

    if (m_scene)
        onSceneAttached(m_scene);
}


void CClassAttributesEditorUI::connectSignals(CEditorScene* scene)
{
    connect(scene, SIGNAL(sceneChanged()), this, SLOT(onSceneChanged()), Qt::QueuedConnection);
}


void CClassAttributesEditorUI::onSceneAttached(CEditorScene* scene)
{
    connectSignals(scene);

    onSceneChanged();
}


void CClassAttributesEditorUI::onSceneDetached(CEditorScene* scene)
{
    scene->disconnect(this);
}


void CClassAttributesEditorUI::onSceneChanged()
{
    rebuild();
}


void CClassAttributesEditorUI::on_ClassId_currentIndexChanged(int)
{
    rebuild();
}


void CClassAttributesEditorUI::rebuild()
{
	if (!m_scene)
		return;

    QByteArray classId;
    if (ui->ClassId->currentIndex() > 0)
        classId = ui->ClassId->currentText().toLocal8Bit();

    ui->Editor->setUpdatesEnabled(false);
    ui->Editor->clear();

    auto attrs = m_scene->getClassAttributes(classId, true);
    for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it)
    {
        CBaseProperty* prop = NULL;

        switch (it.value().valueType)
        {
            case QVariant::Bool:
            {
                prop = new CBoolProperty(
                    it.key(),
                    it.key(),
                    it.value().defaultValue.toBool());
                break;
            }

            case QVariant::Int:
            case QVariant::UInt:
            case QVariant::LongLong:
            case QVariant::ULongLong:
            {
                prop = new CIntegerProperty(
                    it.key(),
                    it.key(),
                    it.value().defaultValue.toInt());
                break;
            }

            case QVariant::Double:
            {
                prop = new CDoubleProperty(
                    it.key(),
                    it.key(),
                    it.value().defaultValue.toDouble());
                break;
            }

            case QVariant::String:
            {
                prop = new CStringProperty(
                    it.key(),
                    it.key(),
                    it.value().defaultValue.toString());
                break;
            }

			case QVariant::Color:
			{
				prop = new CColorProperty(
					it.key(),
					it.key(),
					it.value().defaultValue.value<QColor>());
				break;
			}

			case QVariant::Font:
			{
				prop = new CFontProperty(
					it.key(),
					it.key(),
					it.value().defaultValue.value<QFont>());
				break;
			}

            default:;   //  unknown
        }

        if (prop)
        {
            ui->Editor->add(prop);

            if (!it.value().defaultValue.isValid())
                prop->setText(0, prop->text(0).prepend("*"));
        }
		else
		{
			auto id = it.key();
			auto v = it.value().defaultValue;
		}
    }

    ui->Editor->setUpdatesEnabled(true);

    ui->Editor->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}


//void CClassAttributesEditorUI::on_Editor_valueChanged(CBaseProperty *prop, const QVariant &v)
//{
//	if (!m_scene || m_items.isEmpty())
//		return;

//	for (auto sceneItem : m_items)
//	{
//		sceneItem->setAttribute(prop->getId(), v);
//	}

//	// store state
//	m_scene->addUndoState();
//}

