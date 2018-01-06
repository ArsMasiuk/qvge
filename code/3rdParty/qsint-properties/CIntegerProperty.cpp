#include "CIntegerProperty.h"

//#include "CPropertyEditor.h" // ?

#include <QDebug>
#include <QSpinBox>


CIntegerProperty::CIntegerProperty(const QByteArray &id, const QString &name, int value, int defaultValue, int min, int max):
    CBaseProperty(id, name),
    m_value(value),
    m_defaultValue(defaultValue),
    m_min(min), m_max(max)
{
    setValue(m_value);
}


CIntegerProperty::CIntegerProperty(CBaseProperty *top, const QByteArray &id, const QString &name, int value, int defaultValue, int min, int max):
    CBaseProperty(top, id, name),
    m_value(value),
    m_defaultValue(defaultValue),
    m_min(min), m_max(max)
{
    setValue(m_value);
}


void CIntegerProperty::setValue(int value)
{
    m_value = value;

    CBaseProperty::setValue();

    //qDebug() << "CIntegerProperty::setValue() " << m_value;
}


int CIntegerProperty::getValue() const
{
    // handle editor...

    //m_value = text(1).toInt();

    return m_value;
}


void CIntegerProperty::setRange(int min, int max)
{
    m_min = min;
    m_max = max;

    CBaseProperty::setValue();
}


// event handlers

QVariant CIntegerProperty::getVariantValue() const
{
    return m_value;
}


void CIntegerProperty::displayValue()
{
    setText(1, QString::number(m_value));
}


void CIntegerProperty::validateValue()
{
    if (m_value < m_min)
        m_value = m_min;

    if (m_value > m_max)
        m_value = m_max;
}


QWidget *CIntegerProperty::createEditor() const
{
    return new QSpinBox();
}


void CIntegerProperty::valueToEditor()
{
    QSpinBox* spinEditor = dynamic_cast<QSpinBox*>(getActiveEditor());
    if (spinEditor != NULL)
    {
        spinEditor->setRange(m_min, m_max);
        spinEditor->setValue(m_value);
    }
}


void CIntegerProperty::valueFromEditor()
{
    QSpinBox* spinEditor = dynamic_cast<QSpinBox*>(getActiveEditor());
    if (spinEditor != NULL)
    {
        int newValue = spinEditor->value();
        if (newValue != m_value)
        {
            setValue(newValue);

			emitValueChanged();
        }
    }
}


void CIntegerProperty::startEdit()
{
//    qDebug() << "CIntegerProperty::onEdit()";

    CBaseProperty::startEdit();

    QSpinBox* spinEditor = dynamic_cast<QSpinBox*>(getActiveEditor());
    if (spinEditor != NULL)
    {
        spinEditor->selectAll();
    }
}
