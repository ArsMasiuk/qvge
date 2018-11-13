#pragma once

#include <QDialog>
#include <QMap>
#include <QList>
#include <QPointF>


namespace Ui {
	class CNodesFactorDialog;
}

class CNode;
class CNodeEditorScene;


class CNodesFactorDialog : public QDialog
{
	Q_OBJECT

public:
	CNodesFactorDialog();
	~CNodesFactorDialog();

	int exec(CNodeEditorScene& scene);

private Q_SLOTS:
	void on_FactorX_valueChanged(int v);
	void on_FactorY_valueChanged(int v);

private:
	Ui::CNodesFactorDialog *ui;

	QMap<CNode*, QPointF> m_sourceMap;
	QPointF m_sourceCenter;

	CNodeEditorScene *m_scene;
};

