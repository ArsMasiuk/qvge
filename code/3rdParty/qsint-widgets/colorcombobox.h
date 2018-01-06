#ifndef QCOLORCOMBOBOX_H
#define QCOLORCOMBOBOX_H

#include <QComboBox>

#include "colordefs.h"


namespace QSint
{


class ColorComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit ColorComboBox(QWidget *parent = 0);

    /** Sets active color list to \a colorNames.
     * The names are shown in the way how they are stored in the \a colorNames.
     */
    void setColors(const QStringList& colorNames);

    /** Sets active color list to \a colors.
     * The names are shown in the QColor::name() format, i.e. "#RRGGBB"
     */
    void setColors(const ColorList& colors);

    /** Returns current color (selected or written).
     */
    QColor currentColor() const;

    void allowListColorsOnly(bool on);

    bool isListColorsOnly() const
        { return m_listOnly; }

    virtual void setEditable(bool editable);

    QString colorName(const QColor& color) const;

    static QIcon colorIcon(const QColor& color, int size = 14);

Q_SIGNALS:
    /** Emitted when current color has been changed to \a color.
     */
    void currentColorChanged(const QColor& color);

public Q_SLOTS:
    /** Sets current color to \a color (same as setCurrentColor()).
     */
    void setColor(const QColor& color)
        { setCurrentColor(color); }

    /** Sets current color to \a color.
     */
    void setCurrentColor(const QColor& color);

protected Q_SLOTS:
    void onCurrentIndexChanged(int index);
    void onEdited();
    void onSelectionTextChanged();

private:
    bool m_listOnly;
};


}

#endif // QCOLORCOMBOBOX_H
