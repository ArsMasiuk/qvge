#ifndef ROUNDPROGRESSBAR_H
#define ROUNDPROGRESSBAR_H

#include <QWidget>


namespace QSint
{


/**
 * @brief The RoundProgressBar class represents a circular progress bar and maintains its API
 * similar to the *QProgressBar*.
 * \since 0.3
 *
 * ### Styles
 * RoundProgressBar currently supports Donut, Pie, Expand and Line styles. See setBarStyle() for more details.
 *
 * ### Colors
 * Generally RoundProgressBar uses its palette and font attributes to define how it will look.
 *
 * The following \a QPalette members are considered:
 * - *QPalette::Window*   background of the whole widget (normally should be set to Qt::NoBrush)
 * - *QPalette::Base*     background of the non-filled progress bar area (should be set to Qt::NoBrush to make it transparent)
 * - *QPalette::AlternateBase*  background of the central circle where the text is shown (for \a Donut style)
 * - *QPalette::Shadow*         foreground of the non-filled progress bar area (i.e. border color)
 * - *QPalette::Highlight*      background of the filled progress bar area
 * - *QPalette::Text*           color of the text shown in the center
 *
 * Create a \a QPalette with given attributes and apply it via `setPalette()`.
 *
 * ### Color gradient
 * \a Donut, \a Expand and \a Pie styles allow to use color gradient for currernt value area instead of plain brush fill.
 * See setDataColors() for more details.
 *
 * ### Value text
 * Value text is generally drawn inside the RoundProgressBar using its `font()` and \a QPalette::Text role from its `palette()`.
 *
 * To define pattern of the text, use setFormat() function (see Qt's \a QProgressBar for more details).
 *
 * To define number of decimals to be shown, use setDecimals() function.
 *
 * ### Font
 * To use own font for value text, apply it via `setFont()`.
 *
 * By default, font size will be adjusted automatically to fit the inner circle of the widget.
 */
class RoundProgressBar : public QWidget
{
    Q_OBJECT
public:    
    explicit RoundProgressBar(QWidget *parent = 0);

    static const int PositionLeft = 180;
    static const int PositionTop = 90;
    static const int PositionRight = 0;
    static const int PositionBottom = -90;

    /**
     * @brief Return position (in degrees) of minimum value.
     * \sa setNullPosition
     */
    double nullPosition() const { return m_nullPosition; }
    /**
     * @brief Defines position of minimum value.
     * @param position position on the circle (in degrees) of minimum value
     * \sa nullPosition
     */
    void setNullPosition(double position);

    /**
     * @brief The BarStyle enum defines general look of the progress bar.
     */
    enum BarStyle
    {
        /// Donut style (filled torus around the text)
        StyleDonut,
        /// Pie style (filled pie segment with the text in center)
        StylePie,
        /// Line style (thin round line around the text)
        StyleLine,
        /// Expand style (circle filled from inside to outline)
        StyleExpand
    };
    /**
     * @brief Sets visual style of the widget.
     * \sa barStyle
     */
    void setBarStyle(BarStyle style);
    /**
     * @brief Returns current progree bar style.
     * \sa setBarStyle
     */
    BarStyle barStyle() const { return m_barStyle; }

    /**
     * @brief Sets width of the outline circle pen.
     * @param penWidth width of the outline circle pen (in pixels)
     */
    void setOutlinePenWidth(double penWidth);
    /**
     * @brief Returns width of the outline circle pen.
     */
    double outlinePenWidth() const { return m_outlinePenWidth; }

    /**
     * @brief Sets width of the data circle pen.
     * @param penWidth width of the data circle pen (in pixels)
     */
    void setDataPenWidth(double penWidth);
    /**
     * @brief Returns width of the data circle pen.
     */
    double dataPenWidth() const { return m_dataPenWidth; }

    /**
     * @brief Sets colors of the visible data and makes gradient brush from them.
     * Gradient colors can be set for \a Donut and \a Pie styles (see setBarStyle() function).
     *
     * *Warning*: this function will override widget's `palette()` to set dynamically created gradient brush.
     *
     * @param stopPoints List of colors (should have at least 2 values, see Qt's \a QGradientStops for more details).
     * Color value at point 0 corresponds to the minimum() value, while color value at point 1
     * corresponds to the maximum(). Other colors will be distributed accordingly to the defined ranges (see setRange()).
     */
    void setDataColors(const QGradientStops& stopPoints);

    /**
     * @brief Defines the string used to generate the current text.
     * If no format is set, no text will be shown.
     * @param format see \a QProgressBar's format description
     * \sa setDecimals
     */
    void setFormat(const QString& format);
    /**
     * @brief Sets format string to empty string. No text will be shown therefore.
     * See setFormat() for more information.
     */
    void resetFormat();
    /**
     * @brief Returns the string used to generate the current text.
     */
    QString	format() const { return m_format; }

    /**
     * @brief Sets number of decimals to show after the comma (default is 1).
     * \sa setFormat
     */
    void setDecimals(int count);
    /**
     * @brief Returns number of decimals to show after the comma (default is 1).
     * \sa setFormat, setDecimals
     */
    int decimals() const { return m_decimals; }

    /**
     * @brief Returns current value shown on the widget.
     * \sa setValue()
     */
    double value() const { return m_value; }
    /**
     * @brief Returns minimum of the allowed value range.
     * \sa setMinimum, setRange
     */
    double minimum() const { return m_min; }
    /**
     * @brief Returns maximum of the allowed value range.
    * \sa setMaximum, setRange
     */
    double maximum() const { return m_max; }

public Q_SLOTS:
    /**
     * @brief Defines minimum und maximum of the allowed value range.
     * If the current value does not fit into the range, it will be automatically adjusted.
     * @param min minimum of the allowed value range
     * @param max maximum of the allowed value range
     */
    void setRange(double min, double max);
    /**
     * @brief Defines minimum of the allowed value range.
     * If the current value does not fit into the range, it will be automatically adjusted.
     * @param min minimum of the allowed value range
     * \sa setRange
     */
    void setMinimum(double min);
    /**
     * @brief Defines maximum of the allowed value range.
     * If the current value does not fit into the range, it will be automatically adjusted.
     * @param max maximum of the allowed value range
     * \sa setRange
     */
    void setMaximum(double max);
    /**
     * @brief Sets a value which will be shown on the widget.
     * @param val must be between minimum() and maximum()
     */
    void setValue(double val);
    /**
     * @brief Integer version of the previous slot.
     * @param val must be between minimum() and maximum()
     */
    void setValue(int val);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void drawBackground(QPainter& p, const QRectF& baseRect);
    virtual void drawBase(QPainter& p, const QRectF& baseRect);
    virtual void drawValue(QPainter& p, const QRectF& baseRect, double value, double delta);
    virtual void calculateInnerRect(const QRectF& baseRect, double outerRadius, QRectF& innerRect, double& innerRadius);
    virtual void drawInnerBackground(QPainter& p, const QRectF& innerRect);
    virtual void drawText(QPainter& p, const QRectF& innerRect, double innerRadius, double value);
    virtual QString valueToText(double value) const;
    virtual void valueFormatChanged();

    virtual QSize minimumSizeHint() const { return QSize(32,32); }

    virtual bool hasHeightForWidth() const { return true; }
    virtual int heightForWidth(int w) const { return w; }

    void rebuildDataBrushIfNeeded();

    double m_min, m_max;
    double m_value;

    double m_nullPosition;
    BarStyle m_barStyle;
    double m_outlinePenWidth, m_dataPenWidth;

    QGradientStops m_gradientData;
    bool m_rebuildBrush;

    QString m_format;
    int m_decimals;

    static const int UF_VALUE = 1;
    static const int UF_PERCENT = 2;
    static const int UF_MAX = 4;
    int m_updateFlags;
};


} // namespace QSint


#endif // ROUNDPROGRESSBAR_H
