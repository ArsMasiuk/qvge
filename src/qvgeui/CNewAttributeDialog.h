/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CNewAttributeDialog_H
#define CNewAttributeDialog_H

#include <QDialog>

namespace Ui {
class CNewAttributeDialog;
}

class CNewAttributeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CNewAttributeDialog(QWidget *parent = 0);
    ~CNewAttributeDialog();

    QByteArray getId() const;
	void setId(const QString& id);

    int getType() const;
	int setType(int type);

    QVariant getValue() const;

private Q_SLOTS:
    void on_Id_textChanged(const QString& text);

private:
    Ui::CNewAttributeDialog *ui;
};

#endif // CNewAttributeDialog_H
