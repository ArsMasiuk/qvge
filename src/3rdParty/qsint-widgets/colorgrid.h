#ifndef COLORGRID_H
#define COLORGRID_H

#include "colordefs.h"

#include <QWidget>


namespace QSint
{


/**
    \brief Class for visual selection of a color from the color grid.

    \image html ColorGrid.png An example of ColorGrid

    ColorGrid can use a custom (user-defined) color scheme, as well as one of
    pre-defined ones (see \a setScheme() function). Also, the size of color cells
    and their layout can be customized.

    A color can be picked from the grid with the mouse or with the cursor and Enter keys
    (see setPickByDrag() and setClickMode() functions).

    Signal picked() is emitted when the user picks a color from the grid.
    Signal highlighted() is emitted when the user moves mouse cursor over the grid.
*/
class ColorGrid : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int cellSize READ cellSize WRITE setCellSize)
    Q_PROPERTY(int widthInCells READ widthInCells WRITE setWidthInCells)
    Q_PROPERTY(bool autoSize READ autoSize WRITE setAutoSize)
    Q_PROPERTY(bool pickByDrag READ pickByDrag WRITE setPickByDrag)
    Q_PROPERTY(ClickMode clickMode READ clickMode WRITE setClickMode)

public:
    /// \brief Defines color selection behavior.
    enum ClickMode {
        /// a color is picked when mouse button has been pressed
        CM_PRESS,
        /// a color is picked when mouse button has been released
        CM_RELEASE
    };

    /** Constructor.
      */
    ColorGrid(QWidget *parent = 0);
    /** Destructor.
      */
    virtual ~ColorGrid();

    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;


    /** Returns size of a color cell in pixels.
      */
    inline int cellSize() const { return m_cellSize; }
    /** Sets size of a color cell in pixels to \a size (must be > 0).
      */
    void setCellSize(int size);

    /** Returns width of the grid in cells.
      */
    inline int widthInCells() const { return m_widthInCells; }
    /** Sets width of the grid in cells to \a width (must be > 0).
      */
    void setWidthInCells(int width);

    /** Returns height of the grid in cells.
      */
    int heightInCells() const;

    /** Returns \a true if autosize has been enabled.
      \sa setAutoSize()
      */
    inline bool autoSize() const { return m_autoSize; }
    /** Sets autosize mode to \a autosize (disabled by default).

        If autosize mode is enabled, the size of the grid is calculated automatically
        based on number of colors in the current color scheme.
        In this mode, calls to setWidthInCells() are ignored.
      */
    void setAutoSize(bool autosize);

    /** Returns a recently highlighted color.
      \sa highlighted()
      */
    inline const QColor &lastHighlighted() const { return m_hlColor; }
    /** Returns a recently picked color.
      \sa picked()
      */
    inline const QColor &lastPicked() const { return m_selColor; }

    /** Returns \a true if pick-by-drag mode has been enabled.
      \sa setPickByDrag()
      */
    inline bool pickByDrag() const { return m_pickDrag; }
    /** Sets pick-by-drag mode to \a set (disabled by default).

        If pick-by-drag mode is enabled, then signal picked() is emitted when the user
        drags mouse over the grid with pressed mouse button.
      */
    inline void setPickByDrag(bool set) { m_pickDrag = set; }

    /** Returns currently active color picking mode.
      \sa setClickMode()
      */
    inline ClickMode clickMode() const { return m_clickMode; }
    /** Sets color picking mode to \a mode.
      */
    inline void setClickMode(ClickMode mode) { m_clickMode = mode; }

    /** Returns currently active color scheme (by default, defaultColors() is used).
      \sa setScheme()
      */
    inline const ColorList* scheme() const { return m_colors; }
    /** Sets color scheme to \a scheme.
      */
    void setScheme(const ColorList *scheme);

Q_SIGNALS:
    /** Emitted when user moves mouse cursor over the grid or changes selection
        with the cursor keys.

      \a color is the color value under the cursor.
      */
    void highlighted(const QColor &color);
    /** Emitted when user picks a color from the grid.

      \a color is the picked color value.
      */
    void picked(const QColor &color);
    void accepted();
    void rejected();

protected:
    virtual void paintEvent ( QPaintEvent * event );
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void mouseReleaseEvent ( QMouseEvent * event );
    virtual void leaveEvent ( QEvent * event );
    virtual void keyPressEvent ( QKeyEvent * event );

    void redraw();

    int index() const;

    int m_cellSize;
    int m_widthInCells;
    bool m_autoSize;
    int m_row, m_col, m_idx;
    QPixmap m_pix;
    bool m_pickDrag;
    ClickMode m_clickMode;
    QPoint m_pos;

    QColor m_hlColor, m_selColor;

    const ColorList *m_colors;
};


} // namespace

#endif // COLORGRID_H
