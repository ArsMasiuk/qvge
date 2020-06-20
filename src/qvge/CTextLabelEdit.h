/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QGraphicsTextItem> 

class CItem;


class CTextLabelEdit: public QGraphicsTextItem
{
	Q_OBJECT

public:
	CTextLabelEdit();
	~CTextLabelEdit();

	void startEdit(CItem *item);
	void finishEdit(bool accept = true);

Q_SIGNALS:
	void editingStarted(CItem *item);
	void editingFinished(CItem *item, bool cancelled);

protected:
	virtual bool sceneEvent(QEvent *event);

private Q_SLOT:
	void updateGeometry();

private:
	CItem *m_item = nullptr;
	QString m_storedText;
};

