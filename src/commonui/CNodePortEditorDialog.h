#pragma once

#include <QDialog>

namespace Ui {
	class CNodePortEditorDialog;
}

class CNode;
class CNodePort;


class CNodePortEditorDialog : public QDialog 
{
	Q_OBJECT

public:
	CNodePortEditorDialog();
	~CNodePortEditorDialog();

	int exec(CNodePort &port);

private Q_SLOTS:
	void on_Anchor_currentIndexChanged(int index);
	void on_OffsetX_valueChanged(int v);
	void on_OffsetY_valueChanged(int v);
	void on_Color_activated(const QColor &color);

private:
	void doUpdate();

	Ui::CNodePortEditorDialog *ui;
	
	CNodePort *m_port = nullptr;
	CNode *m_node = nullptr;
};

