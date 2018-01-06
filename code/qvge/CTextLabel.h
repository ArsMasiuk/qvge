/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QPointF> 
#include <QSizeF>
#include <QFont> 
#include <QString> 

class CTextLabel
{
public:
	CTextLabel();
	~CTextLabel();

	void setText(const QString& txt, const QFont& = QFont());

	const QSizeF& localSize() const { return m_size; }
	const QString& text() const { return m_text; }

private:
	QSizeF m_size;
	QString m_text;
};

