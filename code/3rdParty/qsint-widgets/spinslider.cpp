#include "spinslider.h"


namespace QSint
{


SpinSlider::SpinSlider(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);

    m_editor = new QSpinBox(this);
    mainLayout->addWidget(m_editor);

    m_unitLabel = new QLabel(this);
    m_unitLabel->hide();
    mainLayout->addWidget(m_unitLabel);

    m_minButton = new QToolButton(this);
    mainLayout->addWidget(m_minButton);

    m_slider = new QSlider(this);
    m_slider->setOrientation(Qt::Horizontal);
    mainLayout->addWidget(m_slider);

    m_maxButton = new QToolButton(this);
    mainLayout->addWidget(m_maxButton);

    m_sliderMultiplier = 1;

    UpdateConstrains();

    connect(m_editor, SIGNAL(valueChanged(int)), this, SLOT(OnEditorValueChanged(int)));
    connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(OnSliderMoved(int)));
    connect(m_minButton, SIGNAL(clicked()), this, SLOT(OnMinButtonClicked()));
    connect(m_maxButton, SIGNAL(clicked()), this, SLOT(OnMaxButtonClicked()));
}


void SpinSlider::setMinimum(int val)
{
    m_editor->setMinimum(val);

    UpdateConstrains();
}


void SpinSlider::setMaximum(int val)
{
    m_editor->setMaximum(val);

    UpdateConstrains();
}


void SpinSlider::setSliderMultiplier(int val)
{
    if (val > 0){
        m_sliderMultiplier = val;

        UpdateConstrains();
    }
}


void SpinSlider::enableTicks(bool on)
{
    m_slider->setTickPosition(on ? QSlider::TicksBelow : QSlider::NoTicks);
}


void SpinSlider::expandVertically(bool on)
{
    if (on){
        m_editor->setSizePolicy(m_editor->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
        m_minButton->setSizePolicy(m_minButton->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
        m_maxButton->setSizePolicy(m_maxButton->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
    }
    else{
        m_editor->setSizePolicy(m_editor->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
        m_minButton->setSizePolicy(m_minButton->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
        m_maxButton->setSizePolicy(m_maxButton->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    }
}


void SpinSlider::setEditorWidth(int width)
{
    m_editor->setFixedWidth(width);
}


void SpinSlider::setUnitLabelWidth(int width)
{
    m_unitLabel->setFixedWidth(width);
    m_unitLabel->setVisible(width > 0);
}


void SpinSlider::setUnitText(const QString &val)
{
    m_unitLabel->setText(val);
    m_unitLabel->setVisible(!val.isEmpty());
}


// protected members

void SpinSlider::UpdateConstrains()
{
    m_minButton->setText(QString::number(m_editor->minimum()));
    m_maxButton->setText(QString::number(m_editor->maximum()));

    m_slider->blockSignals(true);
    m_slider->setRange(m_editor->minimum() / m_sliderMultiplier, m_editor->maximum() / m_sliderMultiplier);
    m_slider->blockSignals(false);
}


// protected slots

void SpinSlider::OnEditorValueChanged(int val)
{
    m_slider->blockSignals(true);
    m_slider->setValue(val / m_sliderMultiplier);
    m_slider->blockSignals(false);

    m_minButton->setEnabled(val != minimum());
    m_maxButton->setEnabled(val != maximum());
}


void SpinSlider::OnSliderMoved(int val)
{
    m_editor->setValue(val * m_sliderMultiplier);
}


void SpinSlider::OnMinButtonClicked()
{
    m_editor->setValue(minimum());
}


void SpinSlider::OnMaxButtonClicked()
{
    m_editor->setValue(maximum());
}



} // namespace
