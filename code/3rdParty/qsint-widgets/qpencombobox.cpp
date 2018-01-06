#include "qpencombobox.h"

#include <QPainter>
#include <QPixmap>


namespace QSint
{


QPenComboBox::QPenComboBox(QWidget *parent) : QComboBox(parent)
{
	setEditable(false);

	QPen pen;
	pen.setWidth(2);

    for (int i = Qt::SolidLine; i < Qt::CustomDashLine; i++)
	{
		QPixmap pixmap(24, 24);
		pixmap.fill(QColor(Qt::transparent));

		pen.setStyle(Qt::PenStyle(i));

		QPainter painter(&pixmap);
		painter.setPen(pen);
		painter.drawLine(0, int(pixmap.height() / 2.), pixmap.width(), int(pixmap.height() / 2.));

		switch (i)
		{
			case Qt::NoPen:				addItem(pixmap, tr("None"));			break;
			case Qt::SolidLine:			addItem(pixmap, tr("Solid"));			break;
			case Qt::DashLine:			addItem(pixmap, tr("Dash"));			break;
			case Qt::DotLine:			addItem(pixmap, tr("Dot"));				break;
			case Qt::DashDotLine:		addItem(pixmap, tr("Dash Dot"));		break;
			case Qt::DashDotDotLine:    addItem(pixmap, tr("Dash Dot Dot"));	break;
			default:					addItem(tr("Custom"));					break;
		}
	}

	setCurrentIndex(1);

	setFixedSize(sizeHint());
}


void QPenComboBox::setCurrentStyle(Qt::PenStyle newStyle)
{
    setCurrentIndex((int)newStyle);
}


Qt::PenStyle QPenComboBox::currentStyle() const
{
	return Qt::PenStyle(currentIndex());
}


}
