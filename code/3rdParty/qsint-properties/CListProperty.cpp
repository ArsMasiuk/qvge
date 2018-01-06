#include <QDebug>

#include "CListProperty.h"


CListProperty::CListProperty(const QByteArray &id, const QString &name, const CListData &list, int index, int defaultIndex):
    CBaseProperty(id, name),
    m_index(index),
    m_listData(list),
    m_defaultIndex(defaultIndex)
{
    setIndex(m_index);
}


CListProperty::CListProperty(CBaseProperty *top, const QByteArray &id, const QString &name, const CListData &list, int index, int defaultIndex):
    CBaseProperty(top, id, name),
    m_index(index),
    m_listData(list),
    m_defaultIndex(defaultIndex)
{
    setIndex(m_index);
}


void CListProperty::setIndex(int index)
{
    m_index = index;

    CBaseProperty::setValue();
}


int CListProperty::getIndex() const
{
    return m_index;
}


void CListProperty::setList(const CListData &list)
{
    // handle editor...

    m_listData = list;

    CBaseProperty::setValue();
}


void CListProperty::onShowEditor(QWidget *editWidget)
{
    QComboBox* comboEditor = dynamic_cast<QComboBox*>(editWidget);
    if (comboEditor != NULL)
    {
        if (comboEditor->count() == 0)
        {
            for (int i = 0; i < m_listData.size(); i++)
            {
                const CListDataItem& data = m_listData.at(i);
                comboEditor->addItem(data.m_icon, data.m_text, data.m_userData);
            }
        }
    }
}


void CListProperty::validateValue()
{
    if (m_index < 0)
        m_index = 0;

    if (m_index >= m_listData.size())
        m_index = m_listData.size() - 1;
}


void CListProperty::displayValue()
{
    if (treeWidget())
        treeWidget()->blockSignals(true);

    if (m_listData.isEmpty())
    {
        setText(1, QObject::tr("<empty>"));
        setIcon(1, QIcon());
    }
    else
    if (m_index < 0)
    {
        setText(1, QObject::tr("<unknown>"));
        setIcon(1, QIcon());
    }
    else
    {
        const CListDataItem& data = m_listData.at(m_index);
        setText(1, data.m_text);
        setIcon(1, data.m_icon);
    }

    if (treeWidget())
        treeWidget()->blockSignals(false);
}


QVariant CListProperty::getVariantValue() const
{
    return m_index;
}


QWidget *CListProperty::createEditor() const
{
    return new QComboBox();
}


void CListProperty::valueToEditor()
{
    QComboBox* comboEditor = dynamic_cast<QComboBox*>(getActiveEditor());
    if (comboEditor != NULL)
    {
        //qDebug() << "CListProperty::valueToEditor()" << m_index << m_listData.size();

        comboEditor->setCurrentIndex(m_index);
    }
}


void CListProperty::valueFromEditor()
{
    QComboBox* comboEditor = dynamic_cast<QComboBox*>(getActiveEditor());
    if (comboEditor != NULL)
    {
        int newIndex = comboEditor->currentIndex();
        if (newIndex != m_index)
        {
            setIndex(newIndex);

            emitValueChanged();
        }
    }
}



