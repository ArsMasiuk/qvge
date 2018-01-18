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

#include <QMessageBox>


CClassAttributesEditorUI::CClassAttributesEditorUI(QWidget *parent) :
	QWidget(parent),
    ui(new Ui::CClassAttributesEditorUI),
    m_scene(NULL),
    m_locked(false)
{
	ui->setupUi(this);

    ui->Editor->setFactoryForManager(&m_manager, &m_factory);

    connect(&m_manager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
            this, SLOT(onValueChanged(QtProperty*, const QVariant&)));
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
    connect(scene, SIGNAL(sceneChanged()), this, SLOT(onSceneChanged()));
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
    if (!m_scene || m_locked)
		return;

	ui->Editor->setUpdatesEnabled(false);
	ui->Editor->clear();

    m_manager.blockSignals(true);
    m_manager.clear();

    QByteArray classId;
    if (ui->ClassId->currentIndex() > 0)
        classId = ui->ClassId->currentText().toLocal8Bit();

    auto attrs = m_scene->getClassAttributes(classId, true);
    for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it)
    {
		// skip not default
		if (it.value().noDefault)
			continue;

        auto prop = m_manager.addProperty(it.value().valueType, it.key());
        prop->setValue(it.value().defaultValue);
        auto item = ui->Editor->addProperty(prop);
        ui->Editor->setExpanded(item, false);
    }

    ui->Editor->setUpdatesEnabled(true);

    m_manager.blockSignals(false);
}


void CClassAttributesEditorUI::onValueChanged(QtProperty *property, const QVariant &val)
{
    if (!m_scene || m_locked)
        return;

    m_locked = true;

    QByteArray classId;
    if (ui->ClassId->currentIndex() > 0)
        classId = ui->ClassId->currentText().toLocal8Bit();

    m_scene->setClassAttribute(classId, property->propertyName().toLatin1(), val);

    // store state
    m_scene->addUndoState();

    m_locked = false;
}

