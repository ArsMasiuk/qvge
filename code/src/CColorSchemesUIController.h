#ifndef CCOLORSCHEMESUICONTROLLER_H
#define CCOLORSCHEMESUICONTROLLER_H

#include <QObject>

class CColorSchemesUIController : public QObject
{
    Q_OBJECT
public:
    explicit CColorSchemesUIController(QObject *parent = nullptr);

signals:

public slots:
};

#endif // CCOLORSCHEMESUICONTROLLER_H