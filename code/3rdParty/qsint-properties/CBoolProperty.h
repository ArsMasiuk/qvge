#ifndef CBOOLPROPERTY_H
#define CBOOLPROPERTY_H

#include "CBaseProperty.h"

#include <QKeyEvent>


class CBoolProperty : public CBaseProperty
{
public:
    CBoolProperty(const QByteArray& id, const QString &name, bool value, bool defaultValue = false);
    CBoolProperty(CBaseProperty *top, const QByteArray& id, const QString &name, bool value, bool defaultValue = false);

    void setValue(bool value);
    bool getValue() const;

    // reimp
    virtual QVariant getVariantValue() const;
    virtual void displayValue();

    // handlers
    virtual bool onKeyPressed(QKeyEvent* /*event*/, QWidget* /*editWidget*/);

protected:
    mutable bool m_value;
    bool m_defaultValue;
};

#endif // CBOOLPROPERTY_H
