/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CClassAttributesEditorUI.h"
#include "CNewAttributeDialog.h"

#include <qvge/CEditorScene.h>
#include <qvge/CItem.h>

#include <QMessageBox>
#include <QDebug>


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

	connect(ui->NodeButton, SIGNAL(clicked()), this, SLOT(rebuild()));
	connect(ui->EdgeButton, SIGNAL(clicked()), this, SLOT(rebuild()));
	connect(ui->GraphButton, SIGNAL(clicked()), this, SLOT(rebuild()));
}


CClassAttributesEditorUI::~CClassAttributesEditorUI()
{
	// important to avoid crash
	ui->Editor->disconnect(this);
	disconnect(this);

	delete ui;
}


void CClassAttributesEditorUI::doReadSettings(QSettings& settings)
{
	int pos = settings.value("splitterPosition", -1).toInt();
	if (pos >= 0)
		ui->Editor->setSplitterPosition(pos);
}


void CClassAttributesEditorUI::doWriteSettings(QSettings& settings)
{
	settings.setValue("splitterPosition", ui->Editor->splitterPosition());
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

//
//void CClassAttributesEditorUI::on_ClassId_currentIndexChanged(int)
//{
//    rebuild();
//}


void CClassAttributesEditorUI::onValueChanged(QtProperty *property, const QVariant &val)
{
	ui->Editor->updateTooltip(dynamic_cast<QtVariantProperty*>(property));

    if (!m_scene || m_locked)
        return;

	// reject changes from subproperties
	if (ui->Editor->topLevelItem(property) == nullptr)
		return;

    m_locked = true;

	QByteArray classId = getClassId();

	auto attrId = property->propertyName().toLatin1();
	bool isSet = false;

	// check for constrains
	auto conn = m_scene->getClassAttributeConstrains(classId, attrId);
	if (conn) 
	{	
		if (auto connList = dynamic_cast<CAttributeConstrainsList*>(conn)) 
		{
			int index = val.toInt();
			if (index >= 0 && index < connList->ids.size())
				m_scene->setClassAttribute(classId, attrId, connList->ids[index]);
			else
				m_scene->setClassAttribute(classId, attrId, connList->ids.first());

			isSet = true;
		}

		else 
			
		if (auto enumList = dynamic_cast<CAttributeConstrainsEnum*>(conn)) 
		{
			int index = val.toInt();
			if (index >= 0 && index < enumList->ids.size())
				m_scene->setClassAttribute(classId, attrId, (QVariant) enumList->ids[index]);
			else
				m_scene->setClassAttribute(classId, attrId, (QVariant) enumList->ids.first());

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


void CClassAttributesEditorUI::on_Editor_currentItemChanged(QtBrowserItem* item)
{
	if (item)
	{
		// only custom attrs can be removed or changed
		ui->RemoveButton->setEnabled(item->property()->isModified());
		ui->ChangeButton->setEnabled(item->property()->isModified());
	}
	else
	{
		ui->RemoveButton->setEnabled(false);
		ui->ChangeButton->setEnabled(false);
	}
}


void CClassAttributesEditorUI::on_AddButton_clicked()
{
    if (!m_scene)
        return;

    CNewAttributeDialog dialog;
    if (dialog.exec() == QDialog::Rejected)
        return;

    auto id = dialog.getId();
    if (id.isEmpty())
        return;

	QByteArray classId = getClassId();

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
    m_scene->setClassAttribute(classId, attr);

    // store state
    m_scene->addUndoState();

	// update
	ui->Editor->selectItemByName(id);

	ui->Editor->setFocus();
}


void CClassAttributesEditorUI::on_ChangeButton_clicked()
{
	if (!m_scene)
		return;

	QByteArray attrId = ui->Editor->getCurrentTopPropertyName().toLatin1();
	//if (attrId.isEmpty())
	//	return;

	QByteArray classId = getClassId();
	auto attr = m_scene->getClassAttribute(classId, attrId, false);

	CNewAttributeDialog dialog;
	dialog.setWindowTitle(tr("Change Attribute"));
	dialog.setId(attrId);
	dialog.setType(attr.valueType);
	if (dialog.exec() == QDialog::Rejected)
		return;

	QByteArray newId = dialog.getId();
	if (newId.isEmpty())
		return;

	int newType = dialog.getType();
	if (newType == attr.valueType && newId == attrId)
		return;

	// check for name duplicate
	if (newId != attrId && m_scene->getClassAttributes(classId, false).contains(newId))
	{
		QMessageBox::critical(this, tr("Attribute exists"),
			tr("Class %1 already has attribute %2. Please pick another id.")
			.arg(QString(classId), QString(newId)),
			QMessageBox::Ok);

		return;
	}

	// remove old one and add new
	CAttribute newAttr(attr);
	newAttr.id = newId;
	newAttr.name = newId;
	newAttr.valueType = newType;
	m_scene->removeClassAttribute(classId, attrId);
	m_scene->setClassAttribute(classId, newAttr);

	// store state
	m_scene->addUndoState();

	// update
	ui->Editor->selectItemByName(newId);

	ui->Editor->setFocus();
}


void CClassAttributesEditorUI::on_RemoveButton_clicked()
{
	if (!m_scene)
		return;

	auto prop = ui->Editor->getCurrentTopProperty();
	if (!prop)
		return;

	QByteArray attrId = prop->propertyName().toLatin1();

	QByteArray classId = getClassId();

	int r = QMessageBox::question(NULL,
		tr("Remove Attribute"),
		tr("Remove attribute %1 from class %2?").arg(QString(attrId), QString(classId)),
		QMessageBox::Yes, QMessageBox::Cancel);

	if (r == QMessageBox::Cancel)
		return;

	// remove prop
	m_locked = true;
	delete prop;

	m_scene->removeClassAttribute(classId, attrId);

	// store state
	m_scene->addUndoState();

	m_locked = false;

	ui->Editor->setFocus();
}


// internal stuff

void CClassAttributesEditorUI::rebuild()
{
	if (!m_scene || m_locked)
		return;

	QString oldName = ui->Editor->getCurrentTopPropertyName();

	on_Editor_currentItemChanged(NULL);

	ui->Editor->setUpdatesEnabled(false);
	ui->Editor->clear();

	m_manager.blockSignals(true);
	m_manager.clear();

	QByteArray classId = getClassId();

	auto attrs = m_scene->getClassAttributes(classId, true);
	for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it)
	{
		// skip not default
		if (it.value().flags & ATTR_NODEFAULT)
			continue;

		QtVariantProperty* prop = NULL;

		// check for constrains
		auto conn = m_scene->getClassAttributeConstrains(classId, it.key());
		if (conn) 
		{
			if (auto connList = dynamic_cast<CAttributeConstrainsList*>(conn)) 
			{
				prop = m_manager.addProperty(QtVariantPropertyManager::enumTypeId(), it.key());
				prop->setAttribute(QLatin1String("enumNames"), connList->names);
				QVariant v;
				qVariantSetValue(v, connList->iconsAsMap());
				prop->setAttribute(QLatin1String("enumIcons"), v);
				int index = connList->ids.indexOf(it.value().defaultValue.toString());
				prop->setValue(index);
			}
			else
			if (auto enumList = dynamic_cast<CAttributeConstrainsEnum*>(conn))
			{
				prop = m_manager.addProperty(QtVariantPropertyManager::enumTypeId(), it.key());
				prop->setAttribute(QLatin1String("enumNames"), enumList->names);
				QVariant v;
				qVariantSetValue(v, enumList->iconsAsMap());
				prop->setAttribute(QLatin1String("enumIcons"), v);
				int index = enumList->ids.indexOf(it.value().defaultValue.toInt());
				prop->setValue(index);
			}
		}

		// else custom property
		if (!prop)
		{
			int type = it.value().valueType;
			if (type == QMetaType::Float)
				type = QMetaType::Double;

			prop = m_manager.addProperty(type, it.key());
			Q_ASSERT(prop != NULL);
			if (!prop)
				continue;	// ignore

			// add 13 commas if double
			if (type == QMetaType::Double)
				prop->setAttribute("decimals", 13);

			prop->setValue(it.value().defaultValue);
		}

		ui->Editor->updateTooltip(prop);

		auto item = ui->Editor->addProperty(prop);
		ui->Editor->setExpanded(item, false);

		if (it.value().isUserDefined())
			prop->setModified(true);

		if (ui->Editor->currentItem() == NULL)
			ui->Editor->setCurrentItem(item);
	}

	ui->Editor->setUpdatesEnabled(true);

	m_manager.blockSignals(false);

	// restore selection
	if (oldName.size())
		ui->Editor->selectItemByName(oldName);
}


QByteArray CClassAttributesEditorUI::getClassId() const
{
	if (ui->NodeButton->isChecked())
		return QByteArrayLiteral("node");
	else
	if (ui->EdgeButton->isChecked())
		return QByteArrayLiteral("edge");
	else
		return QByteArrayLiteral("");
}


