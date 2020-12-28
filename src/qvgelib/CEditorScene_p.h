#pragma once

#include "CTextLabelEdit.h"
#include "CTransformRect.h"


// pimpl for CEditorScene

class CEditorScene_p
{
public:
	CEditorScene_p(class CEditorScene *scene);

	//void attachScene(class CEditorScene *scene);
	//void detachScene(class CEditorScene *scene);

	CTextLabelEdit m_labelEditor;
	CTransformRect m_transformRect;

private:
	class CEditorScene *m_scene = nullptr;
};

