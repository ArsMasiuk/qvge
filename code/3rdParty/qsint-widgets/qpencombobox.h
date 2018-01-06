#ifndef QPENCOMBOBOX_H
#define QPENCOMBOBOX_H

#include <QComboBox>
#include <QPen>


namespace QSint
{


class QPenComboBox: public QComboBox
{
public:
    QPenComboBox(QWidget *parent = 0);

    void setCurrentStyle(Qt::PenStyle);
    Qt::PenStyle currentStyle() const;
};


}

#endif // QPENCOMBOBOX_H
