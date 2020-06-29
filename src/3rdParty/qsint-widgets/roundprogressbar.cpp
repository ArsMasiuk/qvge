#include "roundprogressbar.h"

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>


namespace QSint
{


RoundProgressBar::RoundProgressBar(QWidget *parent) :
    QWidget(parent),
    m_min(0), m_max(100),
    m_value(25),
    m_nullPosition(PositionTop),
    m_barStyle(StyleDonut),
    m_outlinePenWidth(1),
    m_dataPenWidth(1),
    m_rebuildBrush(false),
    m_format("%p%"),
    m_decimals(1),
    m_updateFlags(UF_PERCENT)
{
    QPalette p(palette());
    p.setBrush(QPalette::Window, Qt::transparent);
    setPalette(p);
}

void RoundProgressBar::setRange(double min, double max)
{
    m_min = min;
    m_max = max;

    if (m_max < m_min)
        qSwap(m_max, m_min);

    if (m_value < m_min)
        m_value = m_min;
    else if (m_value > m_max)
        m_value = m_max;

    m_rebuildBrush = true;

    update();
}

void RoundProgressBar::setMinimum(double min)
{
    setRange(min, m_max);
}

void RoundProgressBar::setMaximum(double max)
{
    setRange(m_min, max);
}

void RoundProgressBar::setValue(double val)
{
    if (m_value != val)
    {
        if (val < m_min)
            m_value = m_min;
        else if (val > m_max)
            m_value = m_max;
        else
            m_value = val;

        update();
    }
}

void RoundProgressBar::setValue(int val)
{
    setValue(double(val));
}

void RoundProgressBar::setNullPosition(double position)
{
    if (position != m_nullPosition)
    {
        m_nullPosition = position;

        m_rebuildBrush = true;

        update();
    }
}

void RoundProgressBar::setBarStyle(RoundProgressBar::BarStyle style)
{
    if (style != m_barStyle)
    {
        m_barStyle = style;

        m_rebuildBrush = true;

        update();
    }
}

void RoundProgressBar::setOutlinePenWidth(double penWidth)
{
    if (penWidth != m_outlinePenWidth)
    {
        m_outlinePenWidth = penWidth;

        update();
    }
}

void RoundProgressBar::setDataPenWidth(double penWidth)
{
    if (penWidth != m_dataPenWidth)
    {
        m_dataPenWidth = penWidth;

        update();
    }
}

void RoundProgressBar::setDataColors(const QGradientStops &stopPoints)
{
    if (stopPoints != m_gradientData)
    {
        m_gradientData = stopPoints;
        m_rebuildBrush = true;

        update();
    }
}

void RoundProgressBar::setFormat(const QString &format)
{
    if (format != m_format)
    {
        m_format = format;

        valueFormatChanged();
    }
}

void RoundProgressBar::resetFormat()
{
    m_format = QString::null;

    valueFormatChanged();
}

void RoundProgressBar::setDecimals(int count)
{
    if (count >= 0 && count != m_decimals)
    {
        m_decimals = count;

        valueFormatChanged();
    }
}

void RoundProgressBar::paintEvent(QPaintEvent* /*event*/)
{
    double outerRadius = qMin(width(), height());
    QRectF baseRect(1, 1, outerRadius-2, outerRadius-2);

    QImage buffer(outerRadius, outerRadius, QImage::Format_ARGB32_Premultiplied);
    buffer.fill(Qt::transparent);

    QPainter p(&buffer);
    p.setRenderHint(QPainter::Antialiasing);

    // data brush
    rebuildDataBrushIfNeeded();

    // background
    drawBackground(p, buffer.rect());

    // base circle
    drawBase(p, baseRect);

    // data circle
    double delta = (m_max - m_min) / (m_value - m_min);
    drawValue(p, baseRect, m_value, delta);

    // center circle
    double innerRadius(0);
    QRectF innerRect;
    calculateInnerRect(baseRect, outerRadius, innerRect, innerRadius);
    drawInnerBackground(p, innerRect);

    // text
    drawText(p, innerRect, innerRadius, m_value);

    // finally draw the bar
    p.end();

    QPainter painter(this);
    painter.fillRect(baseRect, Qt::transparent);
    painter.drawImage((width() - outerRadius) / 2, (height() - outerRadius) / 2, buffer);
}

void RoundProgressBar::drawBackground(QPainter &p, const QRectF &baseRect)
{
    p.fillRect(baseRect, palette().background());
}

void RoundProgressBar::drawBase(QPainter &p, const QRectF &baseRect)
{
    switch (m_barStyle)
    {
    case StyleDonut:
        p.setPen(QPen(palette().shadow().color(), m_outlinePenWidth));
        p.setBrush(palette().base());
        p.drawEllipse(baseRect);
        break;

    case StylePie:
    case StyleExpand:
        p.setPen(QPen(palette().base().color(), m_outlinePenWidth));
        p.setBrush(palette().base());
        p.drawEllipse(baseRect);
        break;

    case StyleLine:
        p.setPen(QPen(palette().base().color(), m_outlinePenWidth));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(baseRect.adjusted(m_outlinePenWidth/2, m_outlinePenWidth/2, -m_outlinePenWidth/2, -m_outlinePenWidth/2));
        break;

    default:;
    }

}

void RoundProgressBar::drawValue(QPainter &p, const QRectF &baseRect, double value, double delta)
{
    // nothing to draw
    if (value == m_min)
        return;

    // for Expand style
    if (m_barStyle == StyleExpand)
    {
        p.setBrush(palette().highlight());
        p.setPen(QPen(palette().shadow().color(), m_dataPenWidth));

        double radius = (baseRect.height() / 2) / delta;
        p.drawEllipse(baseRect.center(), radius, radius);

        return;
    }


    // for Line style
    if (m_barStyle == StyleLine)
    {
        p.setPen(QPen(palette().highlight().color(), m_dataPenWidth));
        p.setBrush(Qt::NoBrush);

        if (value == m_max)
        {
            p.drawEllipse(
                baseRect.adjusted(m_outlinePenWidth/2, m_outlinePenWidth/2, -m_outlinePenWidth/2, -m_outlinePenWidth/2));
        }
        else
        {
            double arcLength = 360.0 / delta;

            p.drawArc(
                baseRect.adjusted(m_outlinePenWidth/2, m_outlinePenWidth/2, -m_outlinePenWidth/2, -m_outlinePenWidth/2),
                m_nullPosition * 16,
                -arcLength * 16);
        }

        return;
    }


    // for Pie and Donut styles
    QPainterPath dataPath;
    dataPath.setFillRule(Qt::WindingFill);

    // pie segment outer
    if (value == m_max)
    {
        dataPath.addEllipse(baseRect);
    }
    else
    {
        double arcLength = 360.0 / delta;

        dataPath.moveTo(baseRect.center());
        dataPath.arcTo(baseRect, m_nullPosition, -arcLength);
        dataPath.lineTo(baseRect.center());
    }

    p.setBrush(palette().highlight());
    p.setPen(QPen(palette().shadow().color(), m_dataPenWidth));
    p.drawPath(dataPath);
}

void RoundProgressBar::calculateInnerRect(const QRectF &/*baseRect*/, double outerRadius, QRectF &innerRect, double &innerRadius)
{
    // for Line and Expand styles
    if (m_barStyle == StyleLine || m_barStyle == StyleExpand)
    {
        innerRadius = outerRadius - m_outlinePenWidth;
    }
    else    // for Pie and Donut styles
    {
        innerRadius = outerRadius * 0.75;
    }

    double delta = (outerRadius - innerRadius) / 2;
    innerRect = QRectF(delta, delta, innerRadius, innerRadius);
}

void RoundProgressBar::drawInnerBackground(QPainter &p, const QRectF &innerRect)
{
    if (m_barStyle == StyleDonut)
    {
        p.setBrush(palette().alternateBase());
        p.drawEllipse(innerRect);
    }
}

void RoundProgressBar::drawText(QPainter &p, const QRectF &innerRect, double innerRadius, double value)
{
    if (m_format.isEmpty())
        return;

    // !!! to revise
    QFont f(font());
    f.setPixelSize(10);
    QFontMetricsF fm(f);
    double maxWidth = fm.width(valueToText(m_max));
    double delta = innerRadius / maxWidth;
    double fontSize = f.pixelSize() * delta * 0.75;
    f.setPixelSize(fontSize);
    //f.setPixelSize(innerRadius * qMax(0.05, (0.5 - (double)m_decimals * 0.2)));
    p.setFont(f);

    QRectF textRect(innerRect);
    p.setPen(palette().text().color());
    p.drawText(textRect, Qt::AlignCenter, valueToText(value));
}

QString RoundProgressBar::valueToText(double value) const
{
    QString textToDraw(m_format);

    if (m_updateFlags & UF_VALUE)
        textToDraw.replace("%v", QString::number(value, 'f', m_decimals));

    if (m_updateFlags & UF_PERCENT)
    {
        double procent = (value - m_min) / (m_max - m_min) * 100.0;
        textToDraw.replace("%p", QString::number(procent, 'f', m_decimals));
    }

    if (m_updateFlags & UF_MAX)
        textToDraw.replace("%m", QString::number(m_max - m_min + 1, 'f', m_decimals));

    return textToDraw;
}

void RoundProgressBar::valueFormatChanged()
{
    m_updateFlags = 0;

    if (m_format.contains("%v"))
        m_updateFlags |= UF_VALUE;

    if (m_format.contains("%p"))
        m_updateFlags |= UF_PERCENT;

    if (m_format.contains("%m"))
        m_updateFlags |= UF_MAX;

    update();
}

void RoundProgressBar::rebuildDataBrushIfNeeded()
{
    if (!m_rebuildBrush)
        return;

    if (m_gradientData.isEmpty())
        return;

    if (m_barStyle == StyleLine)
        return;

    m_rebuildBrush = false;

    QPalette p(palette());

    if (m_barStyle == StyleExpand)
    {
        QRadialGradient dataBrush(0.5,0.5, 0.5, 0.5,0.5);
        dataBrush.setCoordinateMode(QGradient::StretchToDeviceMode);

        // set colors
        for (int i = 0; i < m_gradientData.count(); i++)
            dataBrush.setColorAt(m_gradientData.at(i).first, m_gradientData.at(i).second);

        p.setBrush(QPalette::Highlight, dataBrush);
    }
    else
    {
        QConicalGradient dataBrush(QPointF(0.5,0.5), m_nullPosition);
        dataBrush.setCoordinateMode(QGradient::StretchToDeviceMode);

        // invert colors
        for (int i = 0; i < m_gradientData.count(); i++)
            dataBrush.setColorAt(1.0 - m_gradientData.at(i).first, m_gradientData.at(i).second);

        p.setBrush(QPalette::Highlight, dataBrush);
    }

    setPalette(p);
}


} // namespace QSint

