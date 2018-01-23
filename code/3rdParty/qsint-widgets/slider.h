#ifndef SLIDER_H
#define SLIDER_H

#include <QSlider>


namespace QSint
{


/**
    \brief A slider allowing more precise user control by a mouse.

    See:
    - setPreciseMovement()
    - setClickJump()

    \since 0.3
*/

class Slider : public QSlider
{
    Q_OBJECT

public:
    Slider(QWidget* parent = 0);

    /// \return true if presice movement has been activated.
    /// \sa setPreciseMovement()
    /// \since 0.3
    bool preciseMovementActive() const
        { return m_precise; }

    /// \return true if click jump has been activated.
    /// \sa setClickJump()
    /// \since 0.3
    bool clickJumpActive() const
        { return m_clickJump; }

public Q_SLOTS:
    /// Activates precise movement of the slider. Its value will be always rounded up according to singleStep()
    /// when dragging & scrolling.
    /// Clicking on the slider will round up its value according to pageStep().
    ///
    /// Enabled by default.
    /// \since 0.3
    void setPreciseMovement(bool on = true);

    /// When activated, clicking on the slider will move its handle to the click position immediately.
    ///
    /// Not enabled by default.
    /// \since 0.3
    void setClickJump(bool on = true);

protected Q_SLOTS:
    void onActionTriggered(int id);

protected:
    virtual void mousePressEvent(QMouseEvent *event);

private:
    bool m_precise;
    bool m_clickJump;
};


}

#endif // SLIDER_H
