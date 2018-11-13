#include "slider.h"

#include <QtWidgets/QStyleOptionSlider>
#include <QtGui/QMouseEvent>


namespace QSint
{


Slider::Slider(QWidget *parent): QSlider(parent),
    m_precise(false),
    m_clickJump(false)
{
    setPreciseMovement(true);
}


void Slider::setPreciseMovement(bool on)
{
    if (m_precise != on)
    {
        m_precise = on;

        if (m_precise)
            connect(this, SIGNAL(actionTriggered(int)), this, SLOT(onActionTriggered(int)));
        else
            disconnect(this);
    }
}


void Slider::setClickJump(bool on)
{
    if (m_clickJump != on)
    {
        m_clickJump = on;
    }
}


void Slider::onActionTriggered(int action)
{
    if (action == QAbstractSlider::SliderMove)
    {
        int v = (sliderPosition() / singleStep()) * singleStep();
        setValue(v);
    }
    else
    if (action == QAbstractSlider::SliderPageStepAdd || action == QAbstractSlider::SliderPageStepSub)
    {
        int v = (sliderPosition() / pageStep()) * pageStep();
        setValue(v);
    }
}


void Slider::mousePressEvent(QMouseEvent *event)
{
    if (m_clickJump)
    {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

        if (event->button() == Qt::LeftButton && !sr.contains(event->pos()))
        {
            int v = minimum();

            if (orientation() == Qt::Vertical)
                v += ((maximum()-minimum()) * (height()-event->y())) / height();
            else
                v += ((maximum()-minimum()) * event->x()) / width();

            if (m_precise)
                v = (v / singleStep()) * singleStep();

            if (invertedAppearance())
                v = (maximum() - v + 1);

            setValue(v);
        }
    }

    QSlider::mousePressEvent(event);
}


}

