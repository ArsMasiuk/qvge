#ifndef CExtListInputDialog_H
#define CExtListInputDialog_H

#include <QDialog>

namespace Ui {
class CExtListInputDialog;
}

class CExtListInputDialog : public QDialog
{
    Q_OBJECT

public:
    ~CExtListInputDialog();

    static int getItemIndex(const QString &title,
                            const QString &label,
                            const QStringList& texts,
                            const QList<QIcon>& icons,
                            int selectedIndex = 0);

    static int getItemIndex(const QString &title,
                            const QString &label,
                            const QStringList& texts,
                            int selectedIndex = 0);

private:
    explicit CExtListInputDialog(QWidget *parent = nullptr);
	
	Ui::CExtListInputDialog *ui;
};


#endif // CExtListInputDialog_H
