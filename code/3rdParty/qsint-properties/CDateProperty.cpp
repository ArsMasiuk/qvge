#include "CDateProperty.h"

#include <QDateEdit>
#include <QCalendarWidget>


CDateProperty::CDateProperty(const QByteArray &id, const QString &name, const QDate &value, const QDate &defaultValue):
    CBaseProperty(id, name),
    m_defaultValue(defaultValue)
{
    setDate(value);
}


CDateProperty::CDateProperty(CBaseProperty *top, const QByteArray &id, const QString &name, const QDate &value, const QDate &defaultValue):
    CBaseProperty(top, id, name),
    m_defaultValue(defaultValue)
{
    setDate(value);
}


void CDateProperty::setDate(const QDate &value)
{
    m_value = value;

    CBaseProperty::setValue();
}


QDate CDateProperty::getDate() const
{
    return m_value;
}


void CDateProperty::setMaximumDate(const QDate &value)
{
    m_maxDate = value;

    CBaseProperty::setValue();
}


QDate CDateProperty::getMaximumDate() const
{
    return m_maxDate;
}


void CDateProperty::setMinimumDate(const QDate &value)
{
    m_minDate = value;

    CBaseProperty::setValue();
}


QDate CDateProperty::getMinimumDate() const
{
    return m_minDate;
}


void CDateProperty::setDateRange(const QDate &min, const QDate &max)
{
    m_maxDate = max;
    m_minDate = min;

    CBaseProperty::setValue();
}


void CDateProperty::setDisplayFormat(const QString &format)
{
    m_format = format;

    displayValue();
}


QString CDateProperty::displayFormat() const
{
    return m_format;
}


// reimp

QVariant CDateProperty::getVariantValue() const
{
    return m_value;
}


void CDateProperty::displayValue()
{
    if (treeWidget())
        treeWidget()->blockSignals(true);

    QString dateString = m_format.isEmpty() ? m_value.toString(): m_value.toString(m_format);

    setText(1, dateString);
    setToolTip(1, dateString);

    if (treeWidget())
        treeWidget()->blockSignals(false);
}


void CDateProperty::validateValue()
{
    if (m_maxDate.isValid() && m_value > m_maxDate)
    {
        m_value = m_maxDate;
    }

    if (m_minDate.isValid() && m_value < m_minDate)
    {
        m_value = m_minDate;
    }
}


QWidget* CDateProperty::createEditor() const
{
    QDateEdit* dateEditor = new QDateEdit();
    dateEditor->setCalendarPopup(true);

    return dateEditor;
}


void CDateProperty::valueToEditor()
{
    QDateEdit* dateEditor = dynamic_cast<QDateEdit*>(getActiveEditor());
    if (dateEditor != NULL)
    {
        dateEditor->setDate(m_value);

        if (m_minDate.isValid())
            dateEditor->setMinimumDate(m_minDate);
        else
            dateEditor->clearMinimumDate();

        if (m_maxDate.isValid())
            dateEditor->setMaximumDate(m_maxDate);
        else
            dateEditor->clearMaximumDate();

        dateEditor->setDisplayFormat(m_format);
    }
}


void CDateProperty::valueFromEditor()
{
    QDateEdit* dateEditor = dynamic_cast<QDateEdit*>(getActiveEditor());
    if (dateEditor != NULL && dateEditor->date() != m_value)
    {
        setDate(dateEditor->date());

        emitValueChanged();
    }
}

