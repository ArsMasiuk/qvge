#include "QColorComboBox.h"

#include <QPainter>
#include <QPen>
#include <QLineEdit>
#include <QCompleter>
#include <QDebug>


QColorComboBox::QColorComboBox(QWidget *parent) :
    QComboBox(parent),
    m_listOnly(false)
{
    setDuplicatesEnabled(false);
    setInsertPolicy(QComboBox::NoInsert);

    setColorsList(QColor::colorNames());

    //connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));

    setEditable(true);
    connect(lineEdit(), SIGNAL(editingFinished()), this, SLOT(onEdited()));

    // workaround of QCompleter bug (QTBUG-49165)
    connect(lineEdit(), SIGNAL(selectionChanged()), this, SLOT(onSelectionTextChanged()));
    connect(lineEdit(), SIGNAL(textChanged(QString&)), this, SLOT(onSelectionTextChanged()));
}


void QColorComboBox::setColorsList(const QStringList &colorNames)
{
    blockSignals(true);

    QColor current = currentColor();

    clear();

    for (int i = 0; i < colorNames.count(); i++)
    {
        QColor col(colorNames.at(i));

        // add to the list
        addItem(colorIcon(col), colorNames.at(i), col);
    }

    setCurrentColor(current);

    blockSignals(false);
}


void QColorComboBox::setColorsList(const QList<QColor> &colors)
{
    blockSignals(true);

    QColor current = currentColor();

    clear();

    for (int i = 0; i < colors.count(); i++)
    {
        QColor col(colors.at(i));

        // add to the list
        addItem(colorIcon(col), col.name(), col);
    }

    setCurrentColor(current);

    blockSignals(false);
}


QColor QColorComboBox::currentColor() const
{
    if (currentIndex() >= 0)
        return currentData().value<QColor>();

    if (m_listOnly)
        return QColor();

    return QColor(currentText());
}


void QColorComboBox::allowListColorsOnly(bool on)
{
    if (on != m_listOnly)
    {
        m_listOnly = on;

        if (m_listOnly && currentIndex() < 0 && count())
        {
            setCurrentIndex(0);
        }
    }
}


void QColorComboBox::setCurrentColor(const QColor &color)
{
    int index = findData(color);
    if (index >= 0)
    {
        setCurrentIndex(index);
    }
    else
    {
        if (m_listOnly && count())
        {
            setCurrentIndex(0);
            return;
        }

        setCurrentIndex(-1);
        setCurrentText(color.name());
    }
}


QIcon QColorComboBox::colorIcon(const QColor &color, int size)
{
    // create icon
    QPixmap pm(size, size);
    pm.fill(color);
    QPen p(Qt::black, 1);
    QPainter pt(&pm);
    pt.drawRect(QRect(0,0,size-1,size-1));
    return QIcon(pm);
}


QStringList QColorComboBox::defaultColors()
{
    return QColor::colorNames();
}


QStringList QColorComboBox::baseColors()
{
    static QStringList list;

    if (list.isEmpty())
    {
        list << "white" << "lightGray" << "gray" << "darkGray" << "black"
                << "cyan" << "darkCyan" << "red" << "darkRed"
                << "magenta" << "darkMagenta" << "green" << "darkGreen"
                << "yellow" << /*"darkYellow"*/ "#808000" << "blue" << "darkBlue";
    }

    return list;
}


QString QColorComboBox::colorName(const QColor &color) const
{
    int index = findData(color);
    if (index >= 0)
        return itemText(index);

    return color.name();
}


void QColorComboBox::onCurrentIndexChanged(int index)
{
    if (index >= 0)
    {
        Q_EMIT(currentColorChanged(currentColor()));
    }
}


void QColorComboBox::onEdited()
{
    QString colorName = lineEdit()->text();
//    qDebug() << colorName;

    if (QColor::isValidColor(colorName))
    {
        if (m_listOnly)
        {
            int index = findData(QColor(colorName));
            if (index >= 0)
            {
                setCurrentIndex(index);
            }
            else if (count())
            {
                setCurrentIndex(0);
            }
        }
        else
        {
            setCurrentIndex(-1);
            setCurrentText(colorName);
        }

//        qDebug() << currentColor();

        Q_EMIT(currentColorChanged(currentColor()));
    }
}


void QColorComboBox::onSelectionTextChanged()
{
    // workaround of QCompleter bug (QTBUG-49165)
    if (completer())
    {
        completer()->setCompletionPrefix(currentText());
    }
}

