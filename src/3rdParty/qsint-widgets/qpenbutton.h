#pragma once

#include "qsplitbutton.h"

#include <QPen>


namespace QSint
{


class QPenButton : public QSplitButton
{
    Q_OBJECT

public:
    QPenButton(QWidget *parent = Q_NULLPTR);

    void setUsedRange(Qt::PenStyle start, Qt::PenStyle end);

	static QPixmap drawPixmap(Qt::PenStyle style, int width = 2, QSize size = QSize(24,24));

public Q_SLOTS:
	void setPenStyle(Qt::PenStyle style);

Q_SIGNALS:
	void activated(Qt::PenStyle style);

protected Q_SLOTS:
	virtual void onAction(QAction* act);

private:
    virtual void init();
};


}

