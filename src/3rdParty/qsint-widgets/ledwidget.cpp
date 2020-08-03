#include "ledwidget.h"

#include <QtGui/QPainter>


namespace QSint
{


LedWidget::LedWidget(QWidget *parent) :
    QWidget(parent)
{
    setColor(Qt::gray);
}


void LedWidget::setColor(const QColor &ledColor)
{
    m_gradient.setColorAt(0.0, Qt::white);
    m_gradient.setColorAt(1.0, ledColor);
}


void LedWidget::setColors(const QColor &ledColor, const QColor &highlightColor)
{
    m_gradient.setColorAt(0.0, highlightColor);
    m_gradient.setColorAt(1.0, ledColor);
}


void LedWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);

    p.setPen(QPen(Qt::black));
    p.setRenderHint(QPainter::Antialiasing);

    int radius = qMin(rect().width(), rect().height()) / 2 - 2;

    m_gradient.setCenter(rect().center());
    m_gradient.setFocalPoint(rect().center() - QPoint(radius / 2, radius / 2));
    m_gradient.setRadius(radius);

    p.setBrush(m_gradient);

    p.drawEllipse(rect().center(), radius, radius);
}


}

