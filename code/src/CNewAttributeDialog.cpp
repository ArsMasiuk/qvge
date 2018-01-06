/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

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

QString CNewAttributeDialog::getId() const
{
    return ui->Id->text();
}

int CNewAttributeDialog::getType() const
{
    switch (ui->Type->currentIndex())
    {
    case 0:     return QVariant::Int;
    case 1:     return QVariant::Double;
    case 2:     return QVariant::Bool;
    default:    return QVariant::String;
    }
}

QVariant CNewAttributeDialog::getValue() const
{
    switch (ui->Type->currentIndex())
    {
    case 0:     return int(0);
    case 1:     return double(0.0);
    case 2:     return bool(true);
    default:    return QString();
    }
}

void CNewAttributeDialog::on_Id_textChanged(const QString &text)
{
	ui->ButtonBox->button(QDialogButtonBox::Ok)->setEnabled(text.size());
}
