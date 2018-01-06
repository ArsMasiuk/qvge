#include "CTimeProperty.h"

#include <QTimeEdit>


CTimeProperty::CTimeProperty(const QByteArray &id, const QString &name, const QTime &value, const QTime &defaultValue):
    CBaseProperty(id, name),
    m_defaultValue(defaultValue),
    m_format("HH:mm:ss")
{
    setTime(value);
}


CTimeProperty::CTimeProperty(CBaseProperty *top, const QByteArray &id, const QString &name, const QTime &value, const QTime &defaultValue):
    CBaseProperty(top, id, name),
    m_defaultValue(defaultValue),
    m_format("HH:mm:ss")
{
    setTime(value);
}


void CTimeProperty::setTime(const QTime &value)
{
    m_value = value;

    CBaseProperty::setValue();
}


QTime CTimeProperty::getTime() const
{
    return m_value;
}


void CTimeProperty::setMaximumTime(const QTime &value)
{
    m_maxTime = value;

    CBaseProperty::setValue();
}


QTime CTimeProperty::getMaximumTime() const
{
    return m_maxTime;
}


void CTimeProperty::setMinimumTime(const QTime &value)
{
    m_minTime = value;

    CBaseProperty::setValue();
}


QTime CTimeProperty::getMinimumTime() const
{
    return m_minTime;
}


void CTimeProperty::setTimeRange(const QTime &min, const QTime &max)
{
    m_maxTime = max;
    m_minTime = min;

    CBaseProperty::setValue();
}


void CTimeProperty::setDisplayFormat(const QString &format)
{
    m_format = format;

    displayValue();
}


QString CTimeProperty::displayFormat() const
{
    return m_format;
}


// reimp

QVariant CTimeProperty::getVariantValue() const
{
    return m_value;
}


void CTimeProperty::displayValue()
{
    if (treeWidget())
        treeWidget()->blockSignals(true);

    QString timeString = m_format.isEmpty() ? m_value.toString(): m_value.toString(m_format);

    setText(1, timeString);
    setToolTip(1, timeString);

    if (treeWidget())
        treeWidget()->blockSignals(false);
}


void CTimeProperty::validateValue()
{
    if (m_maxTime.isValid() && m_value > m_maxTime)
    {
        m_value = m_maxTime;
    }

    if (m_minTime.isValid() && m_value < m_minTime)
    {
        m_value = m_minTime;
    }
}


QWidget* CTimeProperty::createEditor() const
{
    QTimeEdit* timeEditor = new QTimeEdit();

    return timeEditor;
}


void CTimeProperty::valueToEditor()
{
    QTimeEdit* timeEditor = dynamic_cast<QTimeEdit*>(getActiveEditor());
    if (timeEditor != NULL)
    {
        timeEditor->setTime(m_value);
        timeEditor->setTimeRange(m_minTime, m_maxTime);
        timeEditor->setDisplayFormat(m_format);
    }
}


void CTimeProperty::valueFromEditor()
{
    QTimeEdit* timeEditor = dynamic_cast<QTimeEdit*>(getActiveEditor());
    if (timeEditor != NULL && timeEditor->time() != m_value)
    {
        setTime(timeEditor->time());

        emitValueChanged();
    }
}
