/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/

#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class QToolButton;
class QInputDialog;

class LineEdit : public QLineEdit
{
	Q_OBJECT

public:
	LineEdit(QWidget *parent = 0);

protected:
	void resizeEvent(QResizeEvent *);

private Q_SLOTS:
	void showEditor();

private:
	QToolButton *editorButton;
	static QInputDialog *inputDialog;
};

#endif // LIENEDIT_H
