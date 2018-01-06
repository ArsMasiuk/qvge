#include "CStringProperty.h"

#include <QDebug>
#include <QLineEdit>


CStringProperty::CStringProperty(const QByteArray &id, const QString &name, const QString &value, const QString &defaultValue):
    CBaseProperty(id, name),
    m_value(value),
    m_defaultValue(defaultValue)
{
    setValue(value);
}


CStringProperty::CStringProperty(CBaseProperty *top, const QByteArray &id, const QString &name, const QString &value, const QString &defaultValue):
    CBaseProperty(top, id, name),
    m_value(value),
    m_defaultValue(defaultValue)
{
    setValue(value);
}


void CStringProperty::setValue(const QString &value)
{
    m_value = value;

    CBaseProperty::setValue();
}


const QString &CStringProperty::getValue() const
{
    m_value = text(1);

    return m_value;
}


QVariant CStringProperty::getVariantValue() const
{
    return getValue();
}


void CStringProperty::displayValue()
{
    setText(1, m_value);
}


void CStringProperty::startEdit()
{
    setFlags(flags() | Qt::ItemIsEditable);

    treeWidget()->editItem(this, 1);

    QLineEdit* editor = dynamic_cast<QLineEdit*>(getActiveEditor());
    if (editor != NULL)
    {
        editor->selectAll();
    }
}


void CStringProperty::finishEdit(bool cancel)
{
//    qDebug() << "CStringProperty::finishEdit()  " << text(1) << " : " << getActiveEditor();

    if (!cancel)
    {
        QLineEdit* editor = dynamic_cast<QLineEdit*>(getActiveEditor());
        if (editor != NULL)
        {
            setValue(editor->text());

			emitValueChanged();
        }
    }

    setFlags(flags() & ~Qt::ItemIsEditable);
}

