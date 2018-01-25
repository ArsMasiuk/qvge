#ifndef COGDFNEWGRAPHDIALOG_H
#define COGDFNEWGRAPHDIALOG_H

#include <QDialog>

class CNodeEditorScene;

namespace Ui {
class COGDFNewGraphDialog;
}


class COGDFNewGraphDialog : public QDialog
{
    Q_OBJECT

public:
    explicit COGDFNewGraphDialog(QWidget *parent = 0);
    ~COGDFNewGraphDialog();

    bool exec(CNodeEditorScene &scene);

private Q_SLOTS:
    void on_List_currentRowChanged(int currentRow);

private:
    Ui::COGDFNewGraphDialog *ui;
};

#endif // COGDFNEWGRAPHDIALOG_H
