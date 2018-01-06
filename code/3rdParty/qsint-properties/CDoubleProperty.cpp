#include "CDoubleProperty.h"

#include <QDoubleSpinBox>


const int PRECISION = 8;


CDoubleProperty::CDoubleProperty(const QByteArray &id, const QString &name, double value, double defaultValue, double min, double max):
    CBaseProperty(id, name),
    m_value(value),
    m_defaultValue(defaultValue),
    m_min(min), m_max(max)
{
    setValue(m_value);
}


CDoubleProperty::CDoubleProperty(CBaseProperty *top, const QByteArray &id, const QString &name, double value, double defaultValue, double min, double max):
    CBaseProperty(top, id, name),
    m_value(value),
    m_defaultValue(defaultValue),
    m_min(min), m_max(max)
{
    setValue(m_value);
}


void CDoubleProperty::setValue(double value)
{
    m_value = value;

    CBaseProperty::setValue();
}


double CDoubleProperty::getValue() const
{
    return m_value;
}


void CDoubleProperty::setRange(double min, double max)
{
    m_min = min;
    m_max = max;

    CBaseProperty::setValue();
}


// reimp

QVariant CDoubleProperty::getVariantValue() const
{
    return m_value;
}


void CDoubleProperty::displayValue()
{
    setText(1, QString::number(m_value, 'f', PRECISION));
}


void CDoubleProperty::validateValue()
{
    if (m_value < m_min)
        m_value = m_min;

    if (m_value > m_max)
        m_value = m_max;
}


QWidget *CDoubleProperty::createEditor() const
{
    return new QDoubleSpinBox();
}


void CDoubleProperty::valueToEditor()
{
    QDoubleSpinBox* spinEditor = dynamic_cast<QDoubleSpinBox*>(getActiveEditor());
    if (spinEditor != NULL)
    {
        spinEditor->setDecimals(PRECISION);
        spinEditor->setRange(m_min, m_max);
        spinEditor->setValue(m_value);
    }
}


void CDoubleProperty::valueFromEditor()
{
    QDoubleSpinBox* spinEditor = dynamic_cast<QDoubleSpinBox*>(getActiveEditor());
    if (spinEditor != NULL)
    {
        double newValue = spinEditor->value();
        if (newValue != m_value)
        {
            setValue(newValue);

			emitValueChanged();
        }
    }
}


void CDoubleProperty::startEdit()
{
    CBaseProperty::startEdit();

    QDoubleSpinBox* spinEditor = dynamic_cast<QDoubleSpinBox*>(getActiveEditor());
    if (spinEditor != NULL)
    {
        spinEditor->selectAll();
    }
}
