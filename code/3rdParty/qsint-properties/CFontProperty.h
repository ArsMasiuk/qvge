#ifndef CFONTPROPERTY_H
#define CFONTPROPERTY_H

#include "CBaseProperty.h"
#include "CButtonBasedEditor.h"

#include <QFontComboBox>


class CFontProperty : public CBaseProperty
{
public:
    CFontProperty(const QByteArray& id, const QString &name, const QFont& font,
                  const QFontComboBox::FontFilters filters = QFontComboBox::AllFonts,
                  const QFontDatabase::WritingSystem writeSystem = QFontDatabase::Any);

    CFontProperty(CBaseProperty *top, const QByteArray& id, const QString &name, const QFont& font,
                  const QFontComboBox::FontFilters filters = QFontComboBox::AllFonts,
                  const QFontDatabase::WritingSystem writeSystem = QFontDatabase::Any);

    void setFont(const QFont& font);
    const QFont& getFont() const;

    // reimp
    virtual QVariant getVariantValue() const;
    virtual void displayValue();

    virtual QWidget* createEditor() const;
    virtual void valueToEditor();
    virtual void valueFromEditor();

protected:
    void init();

    class CFontButtonEditor : public TButtonBasedEditor<QFontComboBox>
    {
    public:
        CFontButtonEditor(QFontComboBox* fontComboEditor, CFontProperty* property);

    protected:
        virtual void onEditButtonActivated();

        CFontProperty* m_property;
    };

    mutable QFont m_font;

    QFontComboBox::FontFilters m_filters;
    QFontDatabase::WritingSystem m_writeSystem;

    mutable QFontComboBox m_fontCombo;
};

#endif // CFONTPROPERTY_H
