#include "CFontProperty.h"

#include <QFontDialog>


CFontProperty::CFontProperty(const QByteArray &id, const QString &name, const QFont &font,
                             const QFontComboBox::FontFilters filters, const QFontDatabase::WritingSystem writeSystem):
    CBaseProperty(id, name),
    m_font(font),
    m_filters(filters),
    m_writeSystem(writeSystem)
{
    init();
}


CFontProperty::CFontProperty(CBaseProperty *top, const QByteArray &id, const QString &name, const QFont &font,
                             const QFontComboBox::FontFilters filters, const QFontDatabase::WritingSystem writeSystem):
    CBaseProperty(top, id, name),
    m_font(font),
    m_filters(filters),
    m_writeSystem(writeSystem)
{
    init();
}


void CFontProperty::init()
{
    setFont(m_font);
}


void CFontProperty::setFont(const QFont &font)
{
    m_font = font;

    CBaseProperty::setValue();
}


const QFont& CFontProperty::getFont() const
{
    return m_font;
}


QVariant CFontProperty::getVariantValue() const
{
    return getFont();
}


void CFontProperty::displayValue()
{
    if (treeWidget())
        treeWidget()->blockSignals(true);

    QString fontString = QString("%1, %2pt")
            .arg(m_font.family())
            .arg(m_font.pointSizeF());

    if (m_font.bold())      fontString += ", bold";
    if (m_font.italic())    fontString += ", italic";
    if (m_font.underline()) fontString += ", underline";

    setText(1, fontString);
    setToolTip(1, fontString);

    QFont showFont(m_font);
    showFont.setPointSize(font(0).pointSize());
    QTreeWidgetItem::setFont(1, showFont);

    if (treeWidget())
        treeWidget()->blockSignals(false);
}


QWidget* CFontProperty::createEditor() const
{
    m_fontCombo.setWritingSystem(m_writeSystem);
    m_fontCombo.setFontFilters(m_filters);

    CFontButtonEditor* hostEditor = new CFontButtonEditor(&m_fontCombo, const_cast<CFontProperty*>(this));
    return hostEditor;
}


void CFontProperty::valueToEditor()
{
    if (m_fontCombo.isVisible())
        m_fontCombo.setCurrentFont(m_font);
}


void CFontProperty::valueFromEditor()
{
    if (m_fontCombo.currentFont() != m_font)
    {
        setFont(m_fontCombo.currentFont());

        emitValueChanged();
    }
}


// CFontButtonEditor

CFontProperty::CFontButtonEditor::CFontButtonEditor(QFontComboBox *fontComboEditor, CFontProperty *property)
    : TButtonBasedEditor<QFontComboBox>(fontComboEditor),
      m_property(property)
{
}


void CFontProperty::CFontButtonEditor::onEditButtonActivated()
{
    QFontDialog fontDialog(getEditor()->currentFont());

    if (fontDialog.exec() == QDialog::Accepted)
    {
        getEditor()->setCurrentFont(fontDialog.currentFont());

        m_property->finishEdit();
    }
}
