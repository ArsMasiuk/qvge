/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CSCENEOPTIONSDIALOG_H
#define CSCENEOPTIONSDIALOG_H

#include <QDialog>

#include <qvge/CEditorScene.h>
#include <qvge/CEditorView.h>


namespace Ui {
class CSceneOptionsDialog;
}


struct OptionsData
{
	bool newGraphDialogOnStart = true;
	int backupPeriod = 10;
};


class CSceneOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CSceneOptionsDialog(QWidget *parent = 0);
    ~CSceneOptionsDialog();

public Q_SLOTS:
	virtual int exec(CEditorScene &scene, CEditorView &view, OptionsData &data);

private:
    Ui::CSceneOptionsDialog *ui;
};

#endif // CSCENEOPTIONSDIALOG_H
