#include "colorbutton.h"
#include "colorgrid.h"

#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QPushButton>
#include <QLayout>
#include <QMenu>


namespace QSint
{


ColorButton::ColorButton(QWidget *parent)
	: QToolButton(parent),
    m_mode(PM_COLORGRID_DIALOG),
	m_tooltipMode(TM_NAMED_HEX_COLOR),
    m_labelMode(TM_NAMED_COLOR)
{
    //setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setPopupMode(QToolButton::MenuButtonPopup);

    // build up menu
    QMenu *menu = new QMenu(this);
    setMenu(menu);

    m_grid = new ColorGrid(this);
    m_grid->setPickByDrag(false);
    m_grid->setClickMode(ColorGrid::CM_RELEASE);
    connect(m_grid, SIGNAL(picked(const QColor&)), this, SLOT(setColor(const QColor&)));
    connect(m_grid, SIGNAL(accepted()), menu, SLOT(hide()));

	QPixmap npm(cellSize(), cellSize());
	drawColorItem(npm, QColor());
	m_noColorAction = menu->addAction(npm, tr("No Color"));
	connect(m_noColorAction, SIGNAL(triggered()), this, SLOT(onNoColorButton()));
	m_noColorAction->setVisible(false);	// disabled by default

    m_colorGridAction = new QWidgetAction(this);
    m_colorGridAction->setDefaultWidget(m_grid);
    menu->addAction(m_colorGridAction);

    m_colorDialogAction = menu->addAction(tr("Choose Color..."));
    connect(m_colorDialogAction, SIGNAL(triggered()), this, SLOT(onDialogButton()));

    connect(this, SIGNAL(colorChanged(QColor)), this, SIGNAL(activated(QColor)));
    connect(this, SIGNAL(clicked()), this, SLOT(onClicked()));

    setColorScheme(QSint::OpenOfficeColors());
    setColor(Qt::white);

    setPickMode(PM_COLORGRID_DIALOG);
}

ColorButton::~ColorButton()
{
}

void ColorButton::setColor(const QColor& color)
{
    if (m_color != color || text() != m_buttonText)
    {
        m_color = color;

        QPixmap pm(iconSize());
        drawColorItem(pm, m_color);
        setIcon(QIcon(pm));

        setText(m_buttonText = getColorName(m_labelMode, m_color));

        if (m_tooltipMode != TM_NONE)
            setToolTip(getColorName(m_tooltipMode, m_color));

        Q_EMIT colorChanged(color);
    }
}

void ColorButton::drawColorItem(QPixmap &pm, const QColor& color)
{
	QPainter p(&pm);

	if (!color.isValid())
	{
		p.setBrush(Qt::white);
		p.setPen(QPen(Qt::black, 1));
		p.drawRect(pm.rect().adjusted(0, 0, -1, -1));
		p.setPen(QPen(Qt::red, 1));
		p.drawLine(pm.rect().topLeft(), pm.rect().bottomRight());
	}
	else
	{
		p.setBrush(color);
		p.setPen(palette().color(QPalette::Shadow));
		p.drawRect(pm.rect().adjusted(0, 0, -1, -1));
    }
}

QString ColorButton::getColorName(ColorButton::TextMode tm, const QColor &color) const
{
    if (!color.isValid())
    {
        return tr("None");
    }

    QString namedColor = m_colorScheme->colorName(color);

    switch (tm)
    {
    case TM_NAMED_COLOR:
        return namedColor;

    case TM_HEX_COLOR:
        return color.name();

    case TM_NAMED_HEX_COLOR:
        if (namedColor == color.name())
            return namedColor;
        else
            return namedColor + " (" + color.name() + ")";

    default:;
    }

    return "";
}

void ColorButton::setPickMode(PickMode mode)
{
    m_mode = mode;

    m_colorGridAction->setVisible(mode == PM_COLORGRID || mode == PM_COLORGRID_DIALOG);
    m_colorDialogAction->setVisible(mode == PM_COLORDIALOG || mode == PM_COLORGRID_DIALOG);
}

void ColorButton::resizeEvent(QResizeEvent * /*event*/)
{
    QPixmap pm(iconSize());
    drawColorItem(pm, m_color);
    setIcon(QIcon(pm));
}

void ColorButton::onDialogButton()
{
	QColor c = QColorDialog::getColor(m_color, this);
	if (c.isValid())
	{
		setColor(c);
        Q_EMIT colorChanged(c);
	}
}

void ColorButton::onNoColorButton()
{
	setColor(QColor());
}

void ColorButton::onClicked()
{
    Q_EMIT activated(m_color);
}

void ColorButton::setColorScheme(const NamedColorsScheme &scheme)
{
	m_colorScheme = &scheme;

    if (m_colorScheme->gridWidth > 0)
    {
        m_grid->setAutoSize(false);
        m_grid->setWidthInCells(m_colorScheme->gridWidth);
    }
    else
        m_grid->setAutoSize(true);

    m_grid->setScheme(&m_colorScheme->colors);
}

void ColorButton::setCellSize(int size)
{
    m_grid->setCellSize(size);
}

int ColorButton::cellSize() const
{
    return m_grid->cellSize();
}

void ColorButton::setTooltipMode(TextMode tm)
{
	m_tooltipMode = tm;
}

void ColorButton::setLabelMode(TextMode tm)
{
    m_labelMode = tm;
}

void ColorButton::enableNoColor(bool on)
{
	m_noColorAction->setVisible(on);
}


} // namespace
