#pragma once

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

