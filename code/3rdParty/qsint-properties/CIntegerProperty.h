#ifndef CINTEGERPROPERTY_H
#define CINTEGERPROPERTY_H

#include "CBaseProperty.h"


class CIntegerProperty : public CBaseProperty
{
public:
    CIntegerProperty(const QByteArray& id, const QString &name, int value, int defaultValue = 0, int min = INT_MIN, int max = INT_MAX);
    CIntegerProperty(CBaseProperty *top, const QByteArray& id, const QString &name, int value, int defaultValue = 0, int min = INT_MIN, int max = INT_MAX);

    void setValue(int value);
    int getValue() const;

    void setRange(int min, int max);

    // reimp
    virtual QVariant getVariantValue() const;
    virtual void displayValue();
    virtual void validateValue();

    virtual QWidget* createEditor() const;
    virtual void valueToEditor();
    virtual void valueFromEditor();
    virtual void startEdit();

protected:
    mutable int m_value;
    int m_defaultValue;
    int m_min, m_max;
};

#endif // CINTEGERPROPERTY_H
