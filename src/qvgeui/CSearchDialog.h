/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once 

#include <QDialog>


class CNodeEditorScene;


namespace Ui {
class CSearchDialog;
}

class CSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CSearchDialog(QWidget *parent = 0);
    ~CSearchDialog();

public Q_SLOTS:
	void exec(CNodeEditorScene &scene);

private Q_SLOTS:
	void updateButtons();
	void on_Find_clicked();
	void on_Results_itemSelectionChanged();

private:
    Ui::CSearchDialog *ui;

	CNodeEditorScene *m_scene = 0;
};

