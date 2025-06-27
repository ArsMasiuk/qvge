/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2025 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CPDFExportDialog.h"
#include "ui_CPDFExportDialog.h"


CPDFExportDialog::CPDFExportDialog(QWidget *parent): 
	QDialog(parent),
	ui(new Ui::CPDFExportDialog)
{
	ui->setupUi(this);

	ui->pageSizeComboBox->clear();

	// QPageSize::id() is from 0 to QPageSize::LastPageSize
	for (int i = 0; i <= QPageSize::LastPageSize; ++i) {
		QPageSize::PageSizeId id = static_cast<QPageSize::PageSizeId>(i);
		QPageSize pageSize(id);
		if (pageSize.isValid()) {
			ui->pageSizeComboBox->addItem(pageSize.name(), QVariant::fromValue(id));
		}
	}
}


CPDFExportDialog::~CPDFExportDialog()
{
}


void CPDFExportDialog::setup(int resolution, const QPageLayout& pageLayout)
{
	ui->dpiSpinBox->setValue(resolution);
	ui->pageSizeComboBox->setCurrentIndex(ui->pageSizeComboBox->findData(pageLayout.pageSize().id()));
	ui->orientationComboBox->setCurrentIndex(int(pageLayout.orientation()));
	
	auto margins = pageLayout.margins();
	ui->marginSpinBox->setValue(margins.left());
	ui->marginSpinBox->setValue(margins.right());
	ui->marginSpinBox->setValue(margins.top());
	ui->marginSpinBox->setValue(margins.bottom());
}


int CPDFExportDialog::resoulution() const 
{ 
	return ui->dpiSpinBox->value(); 
}


QPageLayout CPDFExportDialog::pageLayout() const 
{
	QPageSize pageSize(
		static_cast<QPageSize::PageSizeId>(ui->pageSizeComboBox->currentData().toInt())
	);

	QMarginsF margins(
		ui->marginSpinBox->value(),
		ui->marginSpinBox->value(),
		ui->marginSpinBox->value(),
		ui->marginSpinBox->value()
	);

	QPageLayout layout(
		pageSize,
		static_cast<QPageLayout::Orientation>(ui->orientationComboBox->currentIndex()),
		margins
	);

	return layout;
}