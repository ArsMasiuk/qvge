#include "CPropertyHeader.h"


CPropertyHeader::CPropertyHeader(const QByteArray &id, const QString &name)
    : CBaseProperty(id, name)
{
}


CPropertyHeader::CPropertyHeader(CBaseProperty *top, const QByteArray &id, const QString &name)
    : CBaseProperty(top, id, name)
{
}


void CPropertyHeader::onAdded()
{
    setBackground(Qt::darkGray);
    setTextColor(Qt::white);

    // important to call this AFTER adding
    setFirstColumnSpanned(true);
}


