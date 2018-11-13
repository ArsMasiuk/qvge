#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include "colordefs.h"

#include <QToolButton>
#include <QWidgetAction>


namespace QSint
{


class ColorGrid;
class ColorPopup;

/**
    \brief A tool button to select a color from color dialog or grid.

    \image html ColorButton.png An example of ColorButton

    ColorButton is an extension of QToolButton designed to allow the user
    interactively choose a color from popping up dialog window.

    Depending on PickMode, different kinds of color dialogs will be shown when
    user clicks left or right mouse button over the ColorButton (see setPickModeLeft()
    and setPickModeRight() functions).

    You can modify the outlook of ColorButton by changing appropriate QToolButton
    properties. For instance, show text along with color box (ToolButtonStyle property),
    or show drop-down menu arrow (PopupMode property).

    \image html ColorButtons.png Different types of ColorButton
*/
class ColorButton : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(int cellSize READ cellSize WRITE setCellSize)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(PickMode pickMode READ pickMode WRITE setPickMode)

public:
    /// \brief Defines color dialog type.
    enum PickMode 
	{
        /// no dialog
        PM_NONE,
        /// standard system color dialog
        PM_COLORDIALOG,
        /// color grid based dialog
        PM_COLORGRID,
		/// color grid with standard color selector
		PM_COLORGRID_DIALOG
    };

    enum TextMode
	{
		TM_NONE,
		TM_NAMED_COLOR,
		TM_HEX_COLOR,
		TM_NAMED_HEX_COLOR
	};

    /** Constructor.
      */
    ColorButton(QWidget *parent = 0);
    /** Destructor.
      */
    virtual ~ColorButton();

    /** Returns currently selected color.
      */
    inline QColor color() const { return m_color; }

    /** Returns type of color dialog shown on left mouse click (PM_COLORGRID by default).
      \sa setPickMode()
      */
    inline PickMode pickMode() const { return m_mode; }
    /** Sets type of color dialog shown on left mouse click to \a mode.
      */
    void setPickMode(PickMode mode);

    /** Returns currently active color scheme (by default, defaultColors() is used).
      \sa setColorScheme()
      */
    inline const NamedColorsScheme& colorScheme() const { return *m_colorScheme; }
    /** Sets color scheme to \a scheme.
      */
    void setColorScheme(const NamedColorsScheme &scheme);

    /** Returns size of a color cell in pixels.
      */
    int cellSize() const;
    /** Sets size of a color cell in pixels to \a size (must be > 0).
      */
    void setCellSize(int size);

    void setTooltipMode(TextMode tm);

    void setLabelMode(TextMode tm);

	/** If \a on is true, it makes possible to select an empty (invalid) color from the drop-down menu.
	*/
	void enableNoColor(bool on);

public Q_SLOTS:
    /** Sets current color to \a color.
      */
    void setColor(const QColor& color);

protected Q_SLOTS:
	void onNoColorButton();
	void onDialogButton();
    void onClicked();

Q_SIGNALS:
    /** Emitted when user selects a color from the dialog.

      \a color is the picked color value.
      */
    void colorChanged(const QColor &color);

    void activated(const QColor &color);

protected:
    virtual void drawColorItem(QPixmap &pm, const QColor& color);
    virtual QString getColorName(TextMode tm, const QColor& color) const;

    virtual void resizeEvent(QResizeEvent *event);

    QColor m_color;
	QString m_buttonText;
    PickMode m_mode;
    TextMode m_tooltipMode, m_labelMode;

    ColorGrid *m_grid;
    QWidgetAction *m_colorGridAction;
    QAction *m_colorDialogAction;
	QAction *m_noColorAction;

	const NamedColorsScheme *m_colorScheme;
};


} // namespace

#endif // COLORBUTTON_H
