#pragma once

/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QDialog>


namespace Ui {
	class CDOTExportDialog;
}


class CDOTExportDialog : public QDialog
{
	Q_OBJECT

public:
	CDOTExportDialog(QWidget *parent = 0);
	~CDOTExportDialog();

	bool writeBackground() const;
	bool writeAttributes() const;

private:
	Ui::CDOTExportDialog *ui;
};

