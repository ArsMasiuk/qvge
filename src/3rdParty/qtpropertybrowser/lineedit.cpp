/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/

#include "lineedit.h"
#include <QToolButton>
#include <QStyle>
#include <QInputDialog>

QInputDialog *LineEdit::inputDialog = NULL;

LineEdit::LineEdit(QWidget *parent)
	: QLineEdit(parent)
{
	editorButton = new QToolButton(this);
	editorButton->setText("...");
	editorButton->setCursor(Qt::ArrowCursor);
	editorButton->setEnabled(true);
	editorButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
	connect(editorButton, SIGNAL(clicked()), this, SLOT(showEditor()));

	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(editorButton->sizeHint().width() + frameWidth + 1));
	QSize msz = minimumSizeHint();
	setMinimumSize(qMax(msz.width(), editorButton->sizeHint().height() + frameWidth * 2 + 2),
		qMax(msz.height(), editorButton->sizeHint().height() + frameWidth * 2 + 2));

	if (!inputDialog)
	{
		inputDialog = new QInputDialog(NULL);
		inputDialog->setOption(QInputDialog::UsePlainTextEditForTextInput);
		inputDialog->setInputMode(QInputDialog::TextInput);
		inputDialog->setWindowTitle(tr("Text Property"));
		inputDialog->resize(480, 320);
	}
}
   
void LineEdit::resizeEvent(QResizeEvent *)
{
	QSize sz = editorButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	editorButton->move(rect().right() - frameWidth - sz.width(),
		(rect().bottom() + 1 - sz.height()) / 2);
}

void LineEdit::showEditor()
{
	inputDialog->setTextValue(text());
	if (inputDialog->exec() == QDialog::Accepted && inputDialog->textValue() != text())
	{
		setText(inputDialog->textValue());
		Q_EMIT editingFinished();
	}
}