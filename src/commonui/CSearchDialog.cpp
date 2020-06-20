/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CSearchDialog.h"
#include "ui_CSearchDialog.h"

#include <qvge/CNodeEditorScene.h>
#include <qvge/CItem.h>
#include <qvge/CNode.h>
#include <qvge/CEdge.h>

#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QMap>
#include <QVariant>


static bool look(const QString& where, const QString& what, Qt::CaseSensitivity sens, bool word)
{
	if (word)
		return (where.compare(what, sens) == 0);
	else
		return where.contains(what, sens);
}


CSearchDialog::CSearchDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CSearchDialog)
{
	ui->setupUi(this);

	connect(ui->Text, &QLineEdit::textChanged, this, &CSearchDialog::updateButtons);
	connect(ui->NamesScope, &QCheckBox::toggled, this, &CSearchDialog::updateButtons);
	connect(ui->AttrNamesScope, &QCheckBox::toggled, this, &CSearchDialog::updateButtons);
	connect(ui->AttrValuesScope, &QCheckBox::toggled, this, &CSearchDialog::updateButtons);
}


CSearchDialog::~CSearchDialog()
{
	delete ui;
}


void CSearchDialog::exec(CNodeEditorScene &scene)
{
	m_scene = &scene;

	ui->Text->setFocus();
	ui->Text->selectAll();

	updateButtons();

	show();
}


void CSearchDialog::updateButtons()
{
	bool isOk = false;
	isOk |= ui->NamesScope->isChecked();
	isOk |= ui->AttrNamesScope->isChecked();
	isOk |= ui->AttrValuesScope->isChecked();

	isOk &= !ui->Text->text().isEmpty();
	
	ui->Find->setEnabled(isOk);
}


void CSearchDialog::on_Find_clicked()
{
	ui->Results->setUpdatesEnabled(false);
	ui->Results->clear();

	auto items =
		ui->EdgesOnly->isChecked() ? m_scene->getItems<CItem, CEdge>() :
		ui->NodesOnly->isChecked() ? m_scene->getItems<CItem, CNode>() :
		m_scene->getItems<CItem>();

	bool lookNames = ui->NamesScope->isChecked();
	bool lookAttrNames = ui->AttrNamesScope->isChecked();
	bool lookAttrs = ui->AttrValuesScope->isChecked();
	QString text = ui->Text->text();
	Qt::CaseSensitivity sens = ui->CaseSense->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
	bool word = ui->WholeWords->isChecked();

	for (const CItem* item : items)
	{
		QString textToShow;

		if (lookNames)
		{
			QString id = item->getId();
			if (look(id, text, sens, word))
			{
				textToShow = "ID:" + id;
			}
		}

		if (lookAttrNames || lookAttrs)
		{
			const auto& attrMap = item->getLocalAttributes();
			for (auto it = attrMap.constBegin(); it != attrMap.constEnd(); it++)
			{
				QString key(it.key());
				QString val(it.value().toString());

				if (
						(lookAttrNames && look(key, text, sens, word)) 
							|| 
						(lookAttrs && look(val, text, sens, word))	
					)
				{
					if (textToShow.size())
						textToShow += " | ";
					textToShow += key + ": " + val;
				}
			}
		}

		if (textToShow.isEmpty())
			continue;

		QStringList res;
		res << item->typeId() << item->getId() << textToShow;
		auto *ritem = new QTreeWidgetItem(res);

		bool isNode = (dynamic_cast<const CNode*>(item) != NULL);
		ritem->setData(0, Qt::UserRole, isNode);

		ui->Results->addTopLevelItem(ritem);
	}

	ui->Results->setUpdatesEnabled(true);
}


void CSearchDialog::on_Results_itemSelectionChanged()
{
	auto ritems = ui->Results->selectedItems();

	QList<CItem*> selected;

	for (const auto* ritem : ritems)
	{
		QString id = ritem->text(1);
		bool isNode = ritem->data(0, Qt::UserRole).toBool();
		if (isNode)
		{
			auto itemList = m_scene->getItemsById<CNode>(id);
			if (!itemList.isEmpty())
				selected << itemList.first();
		}
		else
		{
			auto itemList = m_scene->getItemsById<CEdge>(id);
			if (!itemList.isEmpty())
				selected << itemList.first();
		}
	}

	m_scene->selectItems(selected);
	m_scene->ensureSelectionVisible();
}

