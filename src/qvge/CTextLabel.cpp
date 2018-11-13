/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include "CTextLabel.h"

#include <QFontMetricsF>


CTextLabel::CTextLabel()
{
}

CTextLabel::~CTextLabel()
{
}

void CTextLabel::setText(const QString& text, const QFont& font)
{
	m_text = text;
	m_size = QFontMetricsF(font).size(0, text);

	//Super::setText(txt.replace("\n", "<br>"));

	//m_size = Super::size();	// too long...
}

