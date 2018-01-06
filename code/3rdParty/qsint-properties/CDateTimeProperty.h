#ifndef CDATETIMEPROPERTY_H
#define CDATETIMEPROPERTY_H


#include "CBaseProperty.h"

#include <QDateTime>


class CDateTimeProperty : public CBaseProperty
{
public:
    CDateTimeProperty(const QByteArray& id, const QString &name, const QDateTime& value, const QDateTime& defaultValue = QDateTime::currentDateTime());
    CDateTimeProperty(CBaseProperty *top, const QByteArray& id, const QString &name, const QDateTime& value, const QDateTime& defaultValue = QDateTime::currentDateTime());

    void setDateTime(const QDateTime& value);
    QDateTime getDateTime() const;

    void setMaximumDateTime(const QDateTime& value);
    QDateTime getMaximumDateTime() const;
    void setMinimumDateTime(const QDateTime& value);
    QDateTime getMinimumDateTime() const;
    void setDateTimeRange(const QDateTime& min, const QDateTime& max);

    void setDisplayFormat(const QString& format);
    QString	displayFormat() const;

    // reimp
    virtual QVariant getVariantValue() const;
    virtual void displayValue();
    virtual void validateValue();

    virtual QWidget* createEditor() const;
    virtual void valueToEditor();
    virtual void valueFromEditor();

protected:
    mutable QDateTime m_value;
    QDateTime m_defaultValue, m_maxDateTime, m_minDateTime;
    QString m_format;
};


#endif // CDATETIMEPROPERTY_H
