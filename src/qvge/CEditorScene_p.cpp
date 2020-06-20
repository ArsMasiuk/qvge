/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CEditorScene_p.h"
#include "CEditorScene.h"
#include "CItem.h"


CEditorScene_p::CEditorScene_p(class CEditorScene *scene): 
	m_scene(scene),
	m_transformRect()
{
	//QObject::connect(&m_labelEditor, &CTextLabelEdit::editingStarted, m_scene, &CEditorScene::onItemEditingStarted);
	QObject::connect(&m_labelEditor, &CTextLabelEdit::editingFinished, m_scene, &CEditorScene::onItemEditingFinished);
}
