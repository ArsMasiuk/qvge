#ifndef CLISTPROPERTY_H
#define CLISTPROPERTY_H

#include <QList>
#include <QIcon>
#include <QComboBox>

#include "CBaseProperty.h"


struct CListDataItem
{
    CListDataItem(const QString& text, const QIcon& icon = QIcon(), const QVariant& data = QVariant())
        : m_text(text),
          m_icon(icon),
          m_userData(data)
    {
    }

    QString m_text;
    QIcon m_icon;
    QVariant m_userData;
};

typedef QList<CListDataItem> CListData;


class CListProperty : public CBaseProperty
{
public:
    CListProperty(const QByteArray& id, const QString &name, const CListData& list, int index, int defaultIndex = 0);
    CListProperty(CBaseProperty *top, const QByteArray& id, const QString &name, const CListData& list, int index, int defaultIndex = 0);

    void setIndex(int index);
    int getIndex() const;

    void setList(const CListData& list);

    // event handlers
    virtual void onShowEditor(QWidget *editWidget);

    // reimp
    virtual QVariant getVariantValue() const;
    virtual void displayValue();
    virtual void validateValue();

    virtual QWidget* createEditor() const;
    virtual void valueToEditor();
    virtual void valueFromEditor();

protected:
    mutable int m_index;
    CListData m_listData;

    int m_defaultIndex;
};

#endif // CLISTPROPERTY_H
