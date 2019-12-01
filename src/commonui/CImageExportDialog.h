#pragma once

/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2019 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QDialog>


namespace Ui {
	class CImageExportDialog;
}

class CEditorScene;


class CImageExportDialog : public QDialog
{
	Q_OBJECT

public:
	CImageExportDialog(QWidget *parent = 0);
	~CImageExportDialog();

	void setScene(CEditorScene& scene);

	bool writeBackground() const;
	int resolution() const;

private Q_SLOTS:
	void updateTargetSize();

private:
	Ui::CImageExportDialog *ui;

	//const CNodeEditorScene* m_scene;
	QSize m_size;
	int m_dpi;
};

