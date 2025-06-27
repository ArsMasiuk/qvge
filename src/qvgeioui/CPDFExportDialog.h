#pragma once

/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2025 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QDialog>
#include <QPageLayout>
#include <QPageSize>


namespace Ui {
	class CPDFExportDialog;
}


class CPDFExportDialog : public QDialog
{
	Q_OBJECT

public:
	CPDFExportDialog(QWidget *parent = 0);
	~CPDFExportDialog();

	void setup(int resolution, const QPageLayout& pageLayout);
	int resoulution() const;
	QPageLayout pageLayout() const;

private:
	Ui::CPDFExportDialog* ui = nullptr;
};

