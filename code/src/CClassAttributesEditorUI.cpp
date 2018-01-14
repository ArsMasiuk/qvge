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

#include <QMessageBox>


CClassAttributesEditorUI::CClassAttributesEditorUI(QWidget *parent) :
	QWidget(parent),
    ui(new Ui::CClassAttributesEditorUI)
{
	ui->setupUi(this);
}

CClassAttributesEditorUI::~CClassAttributesEditorUI()
{
	delete ui;
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

