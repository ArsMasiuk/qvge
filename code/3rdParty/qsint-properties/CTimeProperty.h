#ifndef CTIMEPROPERTY_H
#define CTIMEPROPERTY_H

#include "CBaseProperty.h"

#include <QTime>


class CTimeProperty : public CBaseProperty
{
public:
    CTimeProperty(const QByteArray& id, const QString &name, const QTime& value, const QTime& defaultValue = QTime::currentTime());
    CTimeProperty(CBaseProperty *top, const QByteArray& id, const QString &name, const QTime& value, const QTime& defaultValue = QTime::currentTime());

    void setTime(const QTime& value);
    QTime getTime() const;

    void setMaximumTime(const QTime& value);
    QTime getMaximumTime() const;
    void setMinimumTime(const QTime& value);
    QTime getMinimumTime() const;
    void setTimeRange(const QTime& min, const QTime& max);

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
    mutable QTime m_value;
    QTime m_defaultValue, m_maxTime, m_minTime;
    QString m_format;
};

#endif // CTIMEPROPERTY_H
