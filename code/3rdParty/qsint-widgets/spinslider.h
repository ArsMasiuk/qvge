#ifndef SPINSLIDER_H
#define SPINSLIDER_H

#include <QSlider>
#include <QLabel>
#include <QBoxLayout>
#include <QSpinBox>
#include <QToolButton>


namespace QSint
{


/**
    \brief Spin editor with additional buttons and slider for quick value edition.
    \since 0.2.2

    \image html SpinSlider.png An example of SpinSlider
*/
class SpinSlider : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(int sliderMultiplier READ sliderMultiplier WRITE setSliderMultiplier)
    Q_PROPERTY(bool showTicks READ ticksEnabled WRITE enableTicks)
    Q_PROPERTY(QString unitText READ unitText WRITE setUnitText)

public:
    /** Constructor.
      */
    explicit SpinSlider(QWidget *parent = 0);

    int value() const           { return m_editor->value(); }
    int minimum() const         { return m_editor->minimum(); }
    int maximum() const         { return m_editor->maximum(); }

    void setValue(int val)      { m_editor->setValue(val); }
    void setMinimum(int val);
    void setMaximum(int val);

    int sliderMultiplier() const         { return m_sliderMultiplier; }
    void setSliderMultiplier(int val);

    bool ticksEnabled() const                  { return m_slider->tickPosition() != QSlider::NoTicks; }
    void enableTicks(bool on = true);

    void expandVertically(bool on = true);

    void setEditorWidth(int width);
    void setUnitLabelWidth(int width);

    QString unitText() const                { return m_unitLabel->text(); }
    void setUnitText(const QString& val);

    void setSingleStep(int step)            { m_slider->setSingleStep(step); }
    void setPageStep(int step)              { m_slider->setPageStep(step); }
    void setOrientation(int)                {}

Q_SIGNALS:
    /** Signal emitted when the current index is changed either by the user or programmatically.
     */
    void valueChanged(int index);

protected Q_SLOTS:
    void OnEditorValueChanged(int val);
    void OnSliderMoved(int val);
    void OnMinButtonClicked();
    void OnMaxButtonClicked();
    
protected:
    void UpdateConstrains();

    QSpinBox *m_editor;
    QSlider *m_slider;
    QToolButton *m_minButton, *m_maxButton;
    QLabel *m_unitLabel;

    int m_sliderMultiplier;
};


} // namespace

#endif // SPINSLIDER_H
