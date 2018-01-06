#include "CBoolProperty.h"


CBoolProperty::CBoolProperty(const QByteArray &id, const QString &name, bool value, bool defaultValue):
    CBaseProperty(id, name),
    m_value(value),
    m_defaultValue(defaultValue)
{
    setValue(value);
}


CBoolProperty::CBoolProperty(CBaseProperty *top, const QByteArray &id, const QString &name, bool value, bool defaultValue):
    CBaseProperty(top, id, name),
    m_value(value),
    m_defaultValue(defaultValue)
{
    setValue(value);
}


void CBoolProperty::setValue(bool value)
{
    m_value = value;

    CBaseProperty::setValue();
}


bool CBoolProperty::getValue() const
{
    m_value = (checkState(1) == Qt::Checked);

    return m_value;
}


QVariant CBoolProperty::getVariantValue() const
{
    return getValue();
}


void CBoolProperty::displayValue()
{
    setCheckState(1, m_value ? Qt::Checked : Qt::Unchecked);
}


bool CBoolProperty::onKeyPressed(QKeyEvent *event, QWidget *)
{
    if (event->key() == Qt::Key_Return)
    {
        setValue(!m_value);

		emitValueChanged();

        return true;
    }

    return false;
}


