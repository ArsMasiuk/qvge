#include "CColorProperty.h"

#include <QColorDialog>
#include <QLineEdit>


CColorProperty::CColorProperty(const QByteArray& id, const QString &name, const QColor& color):
    CBaseProperty(id, name),
    m_color(color),
    m_listColorsOnly(false)
{
    setColor(m_color);
}


CColorProperty::CColorProperty(CBaseProperty *top, const QByteArray& id, const QString &name, const QColor& color):
    CBaseProperty(top, id, name),
    m_color(color),
    m_listColorsOnly(false)
{
    setColor(m_color);
}


void CColorProperty::setColor(const QColor &color)
{
    m_color = color;

    CBaseProperty::setValue();
}


const QColor &CColorProperty::getColor() const
{
    return m_color;
}


void CColorProperty::setColorsList(const QStringList &colorNames)
{
    m_colorEditor.setColorsList(colorNames);

    CBaseProperty::setValue();
}


void CColorProperty::allowListColorsOnly(bool on)
{
    m_listColorsOnly = on;

    CBaseProperty::setValue();
}


QVariant CColorProperty::getVariantValue() const
{
    return m_color;
}


void CColorProperty::validateValue()
{
    if (!m_color.isValid())
    {
        m_color = QColor(Qt::black);
    }

    if (m_listColorsOnly && m_colorEditor.count())
    {
        if (m_colorEditor.findData(m_color) < 0)
            m_color = QColor(m_colorEditor.itemText(0));
    }
}


void CColorProperty::displayValue()
{
    if (treeWidget())
        treeWidget()->blockSignals(true);

    QString colorNameExt = QString("HEX: %1\nRGB: %2,%3,%4")
            .arg(m_color.name())
            .arg(m_color.red()).arg(m_color.green()).arg(m_color.blue());

    setText(1, m_colorEditor.colorName(m_color));
    setIcon(1, QColorComboBox::colorIcon(m_color));
    setToolTip(1, colorNameExt);

    if (treeWidget())
        treeWidget()->blockSignals(false);
}


QWidget *CColorProperty::createEditor() const
{
    CColorButtonEditor* hostEditor = new CColorButtonEditor(&m_colorEditor, const_cast<CColorProperty*>(this));

    hostEditor->enableButton(!m_listColorsOnly);

    return hostEditor;
}


void CColorProperty::valueToEditor()
{
    if (m_colorEditor.isVisible())
    {
        m_colorEditor.allowListColorsOnly(m_listColorsOnly);
        m_colorEditor.setCurrentColor(m_color);
        m_colorEditor.lineEdit()->selectAll();
    }
}


void CColorProperty::valueFromEditor()
{
    QColor col = m_colorEditor.currentColor();
    if (col.isValid() && col != m_color)
    {
        setColor(col);

        emitValueChanged();
    }
}


// CColorButtonEditor

CColorProperty::CColorButtonEditor::CColorButtonEditor(QColorComboBox *colorComboEditor, CColorProperty *property)
    : TButtonBasedEditor<QColorComboBox>(colorComboEditor)
{
    m_property = property;
}


void CColorProperty::CColorButtonEditor::onEditButtonActivated()
{
    QColorDialog colorDialog(getEditor()->currentColor());

    if (colorDialog.exec() == QDialog::Accepted)
    {
        getEditor()->setCurrentColor(colorDialog.selectedColor());

        m_property->finishEdit();
    }
}
