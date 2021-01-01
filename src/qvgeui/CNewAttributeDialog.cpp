/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CNewAttributeDialog.h"
#include "ui_CNewAttributeDialog.h"

#include <QPushButton>


CNewAttributeDialog::CNewAttributeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CNewAttributeDialog)
{
    ui->setupUi(this);

	ui->ButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

CNewAttributeDialog::~CNewAttributeDialog()
{
    delete ui;
}


QByteArray CNewAttributeDialog::getId() const
{
    return ui->Id->text().toLatin1();
}


void CNewAttributeDialog::setId(const QString& id)
{
	ui->Id->setText(id);
}


int CNewAttributeDialog::getType() const
{
    switch (ui->Type->currentIndex())
    {
    case 0:     return QVariant::Int;
    case 1:     return QVariant::Double;
    case 2:     return QVariant::Bool;
	case 3:     return QVariant::Color;
	case 4:     return QVariant::Font;
	default:    return QVariant::String;
    }
}


int CNewAttributeDialog::setType(int type)
{
	switch (type)
	{
	case QVariant::Int:		ui->Type->setCurrentIndex(0); break;
	case QVariant::Double:	ui->Type->setCurrentIndex(1); break;
	case QVariant::Bool:	ui->Type->setCurrentIndex(2); break;
	case QVariant::Color:	ui->Type->setCurrentIndex(3); break;
	case QVariant::Font:	ui->Type->setCurrentIndex(4); break;
	default:				ui->Type->setCurrentIndex(5); break;
	}

	return ui->Type->currentIndex();
}


QVariant CNewAttributeDialog::getValue() const
{
    switch (ui->Type->currentIndex())
    {
    case 0:     return int(0);
    case 1:     return double(0.0);
    case 2:     return bool(true);
	case 3:     return QColor();
	case 4:     return QFont();
    default:    return QString();
    }
}


void CNewAttributeDialog::on_Id_textChanged(const QString &text)
{
	ui->ButtonBox->button(QDialogButtonBox::Ok)->setEnabled(text.size());
}
