#include "CDateTimeProperty.h"

#include <QDateTimeEdit>
#include <QCalendarWidget>


CDateTimeProperty::CDateTimeProperty(const QByteArray &id, const QString &name, const QDateTime &value, const QDateTime &defaultValue):
    CBaseProperty(id, name),
    m_defaultValue(defaultValue)
{
    setDateTime(value);
}


CDateTimeProperty::CDateTimeProperty(CBaseProperty *top, const QByteArray &id, const QString &name, const QDateTime &value, const QDateTime &defaultValue):
    CBaseProperty(top, id, name),
    m_defaultValue(defaultValue)
{
    setDateTime(value);
}


void CDateTimeProperty::setDateTime(const QDateTime &value)
{
    m_value = value;

    CBaseProperty::setValue();
}


QDateTime CDateTimeProperty::getDateTime() const
{
    return m_value;
}


void CDateTimeProperty::setMaximumDateTime(const QDateTime &value)
{
    m_maxDateTime = value;

    CBaseProperty::setValue();
}


QDateTime CDateTimeProperty::getMaximumDateTime() const
{
    return m_maxDateTime;
}


void CDateTimeProperty::setMinimumDateTime(const QDateTime &value)
{
    m_minDateTime = value;

    CBaseProperty::setValue();
}


QDateTime CDateTimeProperty::getMinimumDateTime() const
{
    return m_minDateTime;
}


void CDateTimeProperty::setDateTimeRange(const QDateTime &min, const QDateTime &max)
{
    m_maxDateTime = max;
    m_minDateTime = min;

    CBaseProperty::setValue();
}


void CDateTimeProperty::setDisplayFormat(const QString &format)
{
    m_format = format;

    displayValue();
}


QString CDateTimeProperty::displayFormat() const
{
    return m_format;
}


// reimp

QVariant CDateTimeProperty::getVariantValue() const
{
    return m_value;
}


void CDateTimeProperty::displayValue()
{
    if (treeWidget())
        treeWidget()->blockSignals(true);

    QString dateString = m_format.isEmpty() ? m_value.toString(): m_value.toString(m_format);

    setText(1, dateString);
    setToolTip(1, dateString);

    if (treeWidget())
        treeWidget()->blockSignals(false);
}


void CDateTimeProperty::validateValue()
{
    if (m_maxDateTime.isValid() && m_value > m_maxDateTime)
    {
        m_value = m_maxDateTime;
    }

    if (m_minDateTime.isValid() && m_value < m_minDateTime)
    {
        m_value = m_minDateTime;
    }
}


QWidget* CDateTimeProperty::createEditor() const
{
    QDateTimeEdit* dateEditor = new QDateTimeEdit();
    dateEditor->setCalendarPopup(true);

    return dateEditor;
}


void CDateTimeProperty::valueToEditor()
{
    QDateTimeEdit* dateEditor = dynamic_cast<QDateTimeEdit*>(getActiveEditor());
    if (dateEditor != NULL)
    {
        dateEditor->setDateTime(m_value);

        if (m_minDateTime.isValid())
            dateEditor->setMinimumDateTime(m_minDateTime);
        else
            dateEditor->clearMinimumDateTime();

        if (m_maxDateTime.isValid())
            dateEditor->setMaximumDateTime(m_maxDateTime);
        else
            dateEditor->clearMaximumDateTime();

        dateEditor->setDisplayFormat(m_format);
    }
}


void CDateTimeProperty::valueFromEditor()
{
    QDateTimeEdit* dateEditor = dynamic_cast<QDateTimeEdit*>(getActiveEditor());
    if (dateEditor != NULL && dateEditor->dateTime() != m_value)
    {
        setDateTime(dateEditor->dateTime());

        emitValueChanged();
    }
}

