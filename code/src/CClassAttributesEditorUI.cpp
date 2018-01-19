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

    ui->ClassId->setCurrentIndex(1);    // node by def.
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
        classId = ui->ClassId->currentText().toLatin1();

    auto attrs = m_scene->getClassAttributes(classId, true);
    for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it)
    {
		// skip not default
		if (it.value().noDefault)
			continue;

		QtVariantProperty* prop = NULL;

		// check for constrains
		auto conn = m_scene->getClassAttributeConstrains(classId, it.key());
		if (conn) {
			auto connList = dynamic_cast<CAttributeConstrainsList*>(conn);
			if (connList) {
				prop = m_manager.addProperty(QtVariantPropertyManager::enumTypeId(), it.key());
				prop->setAttribute(QLatin1String("enumNames"), connList->ids);
				//prop->setAttribute(QLatin1String("enumIcons"), connList->icons);
				int index = connList->ids.indexOf(it.value().defaultValue.toString());
				prop->setValue(index);
			}
		}

		// else custom property
		if (!prop)
		{
			prop = m_manager.addProperty(it.value().valueType, it.key());
			prop->setValue(it.value().defaultValue);
		}
        
        auto item = ui->Editor->addProperty(prop);
        ui->Editor->setExpanded(item, false);

		if (it.value().userDefined)
			prop->setModified(true);
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
        classId = ui->ClassId->currentText().toLatin1();

	auto attrId = property->propertyName().toLatin1();
	bool isSet = false;

	// check for constrains
	auto conn = m_scene->getClassAttributeConstrains(classId, attrId);
	if (conn) {
		auto connList = dynamic_cast<CAttributeConstrainsList*>(conn);
		if (connList) {
			int index = val.toInt();
			if (index >= 0 && index < connList->ids.size())
				m_scene->setClassAttribute(classId, attrId, connList->ids[index]);
			else
				m_scene->setClassAttribute(classId, attrId, connList->ids.first());

			isSet = true;
		}
	}

	// default
	if (!isSet)
		m_scene->setClassAttribute(classId, attrId, val);

    // store state
    m_scene->addUndoState();

    m_locked = false;
}


void CClassAttributesEditorUI::on_AddButton_clicked()
{
    if (!m_scene)
        return;

    QByteArray classId;
    if (ui->ClassId->currentIndex() > 0)
        classId = ui->ClassId->currentText().toLatin1();

    CNewAttributeDialog dialog;
    if (dialog.exec() == QDialog::Rejected)
        return;

    auto id = dialog.getId();
    if (id.isEmpty())
        return;

    if (m_scene->getClassAttributes(classId, false).contains(id))
    {
        QMessageBox::critical(this, tr("Attribute exists"),
                              tr("Class %1 already has attribute %2. Please pick another id.")
                              .arg(QString(classId), QString(id)),
                              QMessageBox::Ok);
        return;
    }

    auto v = dialog.getValue();
    CAttribute attr(id, id, v);
	attr.userDefined = true;
    m_scene->setClassAttribute(classId, attr);

    // store state
    m_scene->addUndoState();

    // rebuild tree
    rebuild();
}
