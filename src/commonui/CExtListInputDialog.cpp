#include "CExtListInputDialog.h"
#include "ui_CExtListInputDialog.h"

#include <QAbstractItemView>

CExtListInputDialog::CExtListInputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CExtListInputDialog)
{
    ui->setupUi(this);

	ui->comboBox->view()->setAlternatingRowColors(true);
	ui->comboBox->setStyleSheet("QAbstractItemView::item { height: 32px;}");
	ui->comboBox->setIconSize(QSize(24, 24));
}


CExtListInputDialog::~CExtListInputDialog()
{
    delete ui;
}


int CExtListInputDialog::getItemIndex(const QString& title,
                                      const QString& label,
                                      const QStringList &texts,
                                      const QList<QIcon>& icons,
                                      int selectedIndex)
{
    static CExtListInputDialog* dialog = new CExtListInputDialog(nullptr);

    dialog->setWindowTitle(title);
    dialog->ui->label->setText(label);

    dialog->ui->comboBox->clear();
    dialog->ui->comboBox->addItems(texts);
    while (dialog->ui->comboBox->count() < icons.size())
    {
        dialog->ui->comboBox->addItem("");
    }

    for (int i = 0; i < icons.size(); i++)
    {
        dialog->ui->comboBox->setItemIcon(i, icons.at(i));
    }

    dialog->ui->comboBox->setCurrentIndex(selectedIndex);

    if (dialog->exec() == QDialog::Rejected)
        return -1;

    return dialog->ui->comboBox->currentIndex();
}


int CExtListInputDialog::getItemIndex(const QString& title,
                                      const QString& label,
                                      const QStringList &texts,
                                      int selectedIndex)
{
    static CExtListInputDialog* dialog = new CExtListInputDialog(nullptr);

    dialog->setWindowTitle(title);
    dialog->ui->label->setText(label);

    dialog->ui->comboBox->clear();
    dialog->ui->comboBox->addItems(texts);

    dialog->ui->comboBox->setCurrentIndex(selectedIndex);

    if (dialog->exec() == QDialog::Rejected)
        return -1;

    return dialog->ui->comboBox->currentIndex();
}

