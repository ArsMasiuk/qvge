#ifndef CDOUBLEPROPERTY_H
#define CDOUBLEPROPERTY_H

#include <float.h>

#include "CBaseProperty.h"


class CDoubleProperty : public CBaseProperty
{
public:
    CDoubleProperty(const QByteArray& id, const QString &name, double value, double defaultValue = 0, double min = DBL_MIN, double max = DBL_MAX);
    CDoubleProperty(CBaseProperty *top, const QByteArray& id, const QString &name, double value, double defaultValue = 0, double min = DBL_MIN, double max = DBL_MAX);

    void setValue(double value);
    double getValue() const;

    void setRange(double min, double max);

    // reimp
    virtual QVariant getVariantValue() const;
    virtual void displayValue();
    virtual void validateValue();

    virtual QWidget* createEditor() const;
    virtual void valueToEditor();
    virtual void valueFromEditor();
    virtual void startEdit();

protected:
    mutable double m_value;
    double m_defaultValue;
    double m_min, m_max;
};

#endif // CDOUBLEPROPERTY_H
