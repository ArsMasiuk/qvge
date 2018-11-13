#include "qpenbutton.h"

#include <QPixmap>
#include <QPainter>
#include <QTimer>


namespace QSint
{


QPenButton::QPenButton(QWidget *parent) : QSplitButton(parent)
{
    init();
}


void QPenButton::init()
{
    QPen pen;
    pen.setWidth(2);

    for (int i = Qt::NoPen; i < Qt::CustomDashLine; i++)
    {
        QPixmap pixmap(iconSize() * 2);
        pixmap.fill(QColor(Qt::transparent));

        pen.setStyle(Qt::PenStyle(i));

        QPainter painter(&pixmap);
        painter.setPen(pen);
        painter.drawLine(0, int(pixmap.height() / 2.), pixmap.width(), int(pixmap.height() / 2.));

        switch (i)
        {
            case Qt::NoPen:             addAction(pixmap, tr("None"), i);			break;
            case Qt::SolidLine:         addAction(pixmap, tr("Solid"), i);			break;
            case Qt::DashLine:          addAction(pixmap, tr("Dashed"), i);			break;
            case Qt::DotLine:           addAction(pixmap, tr("Dotted"), i);			break;
            case Qt::DashDotLine:       addAction(pixmap, tr("Dash-Dot"), i);		break;
            case Qt::DashDotDotLine:    addAction(pixmap, tr("Dash-Dot-Dot"), i);	break;
            default:                    break;
        }
    }
}


void QPenButton::setUsedRange(Qt::PenStyle start, Qt::PenStyle end)
{
    auto list = actions();

    for (int i = 0; i <list.count(); i++)
    {
        list[i]->setVisible(i >= start && i <= end);
    }

    selectActionByIndex(start);
}


void QPenButton::setPenStyle(Qt::PenStyle style)
{
	selectAction((int)style);
}


void QPenButton::onAction(QAction* act)
{
	QSplitButton::onAction(act);

	int style = act->data().toInt();
	if (style < Qt::NoPen || style >= Qt::CustomDashLine)
		style = Qt::NoPen;

	Q_EMIT activated((Qt::PenStyle)style);
}


}

