#ifndef CPROPERTYHEADER_H
#define CPROPERTYHEADER_H

#include "CBaseProperty.h"


class CPropertyHeader : public CBaseProperty
{
public:
    CPropertyHeader(const QByteArray& id, const QString &name);
    CPropertyHeader(CBaseProperty *top, const QByteArray& id, const QString &name);

    // reimp
    virtual void onAdded();
    virtual QVariant getVariantValue() const { return QVariant(); }
};


#endif // CPROPERTYHEADER_H
