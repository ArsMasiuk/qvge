#ifndef LEDWIDGET_H
#define LEDWIDGET_H

#include <QWidget>


namespace QSint
{


/**
    \brief Round LED-style widget with gradient fill.

        The gradient consists of two colors: led color (main color of the widget)
        and highlight color at the top-left corner (typically, white).

    \since 0.2.2

    \image html LedWidget.png An example of LedWidget
*/
class LedWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LedWidget(QWidget *parent = 0);

    virtual QSize minimumSizeHint() const
    { return QSize(12,12); }

    virtual int heightForWidth(int w) const
    { return w; }

public Q_SLOTS:
    /**
     * @brief setColor Funtion sets color of LED to \a ledColor. Highlight color is set to Qt::white.
     * @param ledColor Color to set (Qt::gray is the default value).
     */
    void setColor(const QColor &ledColor);
    /**
     * @brief setColors Funtion sets color of LED to \a ledColor and its highlight color to \a blickColor.
     * @param ledColor (Qt::gray is the default value).
     * @param highlightColor (Qt::white is the default value).
     */
    void setColors(const QColor &ledColor, const QColor &highlightColor);

protected:
    virtual void paintEvent(QPaintEvent *event);

    QRadialGradient m_gradient;
};


}


#endif // LEDWIDGET_H
